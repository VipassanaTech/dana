-- =====================================================================
-- B) PHASE 1 — inert objects (fleet; any time; no behaviour change under HEAD code)
-- Dana donor versioning rev 3 ("dana-donorver-3").
-- Creates: dh_donor_version, dh_donor_fy, dh_donation.d_fy + idx_donor_fy, triggers
--          dh_don_bi_fy / dh_don_bu_fy (d_fy), dh_donor_bd_guard (P2 delete guard); backfills d_fy.
-- Run ONLY through the fleet runner (dana-donorver-fleet.sh B) or by hand as:
--   mysql --show-warnings dana_<sub> < B-phase1.sql        (as root@localhost via the unix socket)
-- Default stop-on-error; never --force. Re-runnable: every step checks before it changes anything,
-- so a re-run takes no table locks except the CREATE OR REPLACE of the three triggers (lock_wait_timeout=5).
-- Register items: L37 (INPLACE/LOCK=NONE, one change per statement), L38 (lock_wait_timeout=5; the runner
-- retries 1205/1213 up to 5 times, 60 s apart), L8/L9/L10/M11 (B0 preflight), P2/L11 (B4 delete guard), M9 (B5 post-check).
-- =====================================================================
SET NAMES utf8mb4 COLLATE utf8mb4_general_ci;     -- pinned: stored as the triggers' character_set_client/collation_connection
SET SESSION sql_mode = 'STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';  -- stored with the triggers
SET SESSION lock_wait_timeout = 5;                 -- metadata locks (DDL, CREATE TRIGGER)
SET SESSION innodb_lock_wait_timeout = 5;          -- row locks (B3 backfill chunks)
SET SESSION group_concat_max_len = 65536;          -- shape signatures below are > 1024 bytes

-- Expected shapes (generated on MariaDB 10.6.23; identical I_S format on 10.7.3 — rehearse on a 10.7.3 clone).
-- Any later column change goes into B as ALTER ... ADD COLUMN IF NOT EXISTS *before* these checks, and these
-- strings are updated in the same commit (register L10).
SET @dana_sig_dv = 'InnoDB/utf8mb3_unicode_ci|dv_id:int(10) unsigned:-:NO:-:auto_increment,dv_donor:int(10) unsigned:-:NO:-:,dv_name:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_gender:varchar(20):utf8mb3_unicode_ci:NO:\'\':,dv_address:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_pincode:varchar(20):utf8mb3_unicode_ci:NO:\'\':,dv_contact:varchar(60):utf8mb3_unicode_ci:NO:\'\':,dv_email:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_occupation:varchar(100):utf8mb3_unicode_ci:NO:\'\':,dv_country_code:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_country:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_state_code:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_state:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_city_id:bigint(20):-:NO:0:,dv_city:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_id_type:int(11):-:NO:0:,dv_id_type_name:varchar(100):utf8mb3_unicode_ci:NO:\'\':,dv_id_no:varchar(60):utf8mb3_unicode_ci:NO:\'\':,dv_address_proof:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_course:bigint(20):-:NO:0:,dv_confirmation:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_source:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_created:datetime:-:NO:current_timestamp():,dv_updatedby:int(11):-:NO:0:|dv_donor_id:1:dv_donor:1,dv_donor_id:2:dv_id:1,dv_id_no:1:dv_id_no:1,dv_name:1:dv_name:1,PRIMARY:1:dv_id:0';
SET @dana_sig_df = 'InnoDB/utf8mb3_unicode_ci|df_donor:int(10) unsigned:-:NO:-:,df_fy:smallint(6):-:NO:-:,df_version:int(10) unsigned:-:NO:-:,df_updated:datetime:-:NO:current_timestamp():on update current_timestamp(),df_updatedby:int(11):-:NO:0:|df_version:1:df_version:1,PRIMARY:1:df_donor:0,PRIMARY:2:df_fy:0';

-- ---------------------------------------------------------------------
-- B0 PREFLIGHT — abort this DB (SIGNAL) on any unexpected state. Changes nothing.
-- ---------------------------------------------------------------------
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE v_n INT DEFAULT 0;
  IF CURRENT_USER() <> 'root@localhost' THEN       -- M11: the triggers are DEFINER=root@localhost
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: run as root@localhost';
  END IF;
  IF (SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()
       AND TABLE_NAME IN ('dh_donor','dh_donation','dh_log')) <> 3 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: not a Dana DB (dh_donor/dh_donation/dh_log missing)';
  END IF;
  IF EXISTS (SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE()
              AND TABLE_NAME = 'dh_donor_version' AND COLUMN_NAME = 'dv_fy') THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: eager dh_donor_version present: run A-local-cleanup first';
  END IF;
  IF EXISTS (SELECT 1 FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()
              AND (TABLE_NAME LIKE 'dh\_donor\_version\_rb\_%' OR TABLE_NAME LIKE 'dh\_donor\_fy\_rb\_%')) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: parked *_rb_* tables present: run the re-enable rename (E-R1) first';   -- L8
  END IF;
  SELECT COUNT(*) INTO v_n FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME IN ('dh_donor_version','dh_donor_fy');
  IF v_n = 1 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: only one of dh_donor_version/dh_donor_fy exists: stop and investigate';   -- L8
  END IF;
  IF v_n = 0 AND EXISTS (SELECT 1 FROM dh_log WHERE l_module = 'dh_donor' AND l_event LIKE 'FY %' LIMIT 1) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: FY freeze/correction logs exist but the tables are missing (restore/park?): stop';   -- L9
  END IF;
  -- L10: existing versioning tables must have exactly the rev-3 shape (columns, types, collation, defaults, indexes, engine)
  IF v_n = 2 AND (
       (SELECT CONCAT(t.ENGINE,'/',t.TABLE_COLLATION,'|',
          (SELECT GROUP_CONCAT(CONCAT_WS(':', c.COLUMN_NAME, c.COLUMN_TYPE, IFNULL(c.COLLATION_NAME,'-'), c.IS_NULLABLE, IFNULL(c.COLUMN_DEFAULT,'-'), c.EXTRA) ORDER BY c.ORDINAL_POSITION SEPARATOR ',')
             FROM information_schema.COLUMNS c WHERE c.TABLE_SCHEMA = DATABASE() AND c.TABLE_NAME = 'dh_donor_version'),'|',
          (SELECT GROUP_CONCAT(CONCAT(s.INDEX_NAME,':',s.SEQ_IN_INDEX,':',s.COLUMN_NAME,':',s.NON_UNIQUE) ORDER BY s.INDEX_NAME, s.SEQ_IN_INDEX SEPARATOR ',')
             FROM information_schema.STATISTICS s WHERE s.TABLE_SCHEMA = DATABASE() AND s.TABLE_NAME = 'dh_donor_version'))
          FROM information_schema.TABLES t WHERE t.TABLE_SCHEMA = DATABASE() AND t.TABLE_NAME = 'dh_donor_version') <> @dana_sig_dv
    OR (SELECT CONCAT(t.ENGINE,'/',t.TABLE_COLLATION,'|',
          (SELECT GROUP_CONCAT(CONCAT_WS(':', c.COLUMN_NAME, c.COLUMN_TYPE, IFNULL(c.COLLATION_NAME,'-'), c.IS_NULLABLE, IFNULL(c.COLUMN_DEFAULT,'-'), c.EXTRA) ORDER BY c.ORDINAL_POSITION SEPARATOR ',')
             FROM information_schema.COLUMNS c WHERE c.TABLE_SCHEMA = DATABASE() AND c.TABLE_NAME = 'dh_donor_fy'),'|',
          (SELECT GROUP_CONCAT(CONCAT(s.INDEX_NAME,':',s.SEQ_IN_INDEX,':',s.COLUMN_NAME,':',s.NON_UNIQUE) ORDER BY s.INDEX_NAME, s.SEQ_IN_INDEX SEPARATOR ',')
             FROM information_schema.STATISTICS s WHERE s.TABLE_SCHEMA = DATABASE() AND s.TABLE_NAME = 'dh_donor_fy'))
          FROM information_schema.TABLES t WHERE t.TABLE_SCHEMA = DATABASE() AND t.TABLE_NAME = 'dh_donor_fy') <> @dana_sig_df) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: existing dh_donor_version/dh_donor_fy shape differs from rev 3 (see D-checks SHAPE row)';
  END IF;
  -- the 15 captured dh_donor columns (+ d_id, d_updatedby, d_updated) must fit the snapshot columns byte-for-byte:
  -- same type family, width <= snapshot width, utf8mb3_unicode_ci (same as the snapshot, so COALESCE reads never hit 1267)
  SELECT COUNT(*) INTO v_n FROM (
            SELECT 'd_name' col, 'varchar' typ, 255 maxw UNION ALL SELECT 'd_gender','varchar',20
  UNION ALL SELECT 'd_address','varchar',255   UNION ALL SELECT 'd_pincode','varchar',20
  UNION ALL SELECT 'd_contact','varchar',60    UNION ALL SELECT 'd_email','varchar',255
  UNION ALL SELECT 'd_occupation','varchar',100 UNION ALL SELECT 'd_country','varchar',10
  UNION ALL SELECT 'd_state','varchar',10      UNION ALL SELECT 'd_city','bigint',0
  UNION ALL SELECT 'd_id_type','int',0         UNION ALL SELECT 'd_id_no','varchar',60
  UNION ALL SELECT 'd_address_proof','varchar',255 UNION ALL SELECT 'd_course','bigint',0
  UNION ALL SELECT 'd_confirmation','varchar',10   UNION ALL SELECT 'd_id','bigint',0
  UNION ALL SELECT 'd_updatedby','int',0       UNION ALL SELECT 'd_updated','datetime',0) m
    LEFT JOIN information_schema.COLUMNS c
           ON c.TABLE_SCHEMA = DATABASE() AND c.TABLE_NAME = 'dh_donor' AND c.COLUMN_NAME = m.col
   WHERE c.COLUMN_NAME IS NULL OR c.DATA_TYPE <> m.typ
      OR (m.typ = 'varchar' AND (c.CHARACTER_MAXIMUM_LENGTH > m.maxw OR c.COLLATION_NAME <> 'utf8mb3_unicode_ci'));
  IF v_n <> 0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: dh_donor column types/widths/collation differ from the expected prod shape';
  END IF;
  -- an existing d_fy / idx_donor_fy (local eager build) must already have the rev-3 shape
  IF EXISTS (SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'dh_donation'
              AND COLUMN_NAME = 'd_fy' AND NOT (COLUMN_TYPE = 'smallint(6)' AND IS_NULLABLE = 'NO' AND COLUMN_DEFAULT = '0')) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: dh_donation.d_fy exists with a different type/default';
  END IF;
  IF EXISTS (SELECT 1 FROM information_schema.STATISTICS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'dh_donation'
              AND INDEX_NAME = 'idx_donor_fy')
     AND (SELECT GROUP_CONCAT(COLUMN_NAME ORDER BY SEQ_IN_INDEX) FROM information_schema.STATISTICS
           WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'dh_donation' AND INDEX_NAME = 'idx_donor_fy') <> 'd_donor,d_fy' THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B0: dh_donation.idx_donor_fy exists with different columns';
  END IF;
END//
DELIMITER ;

-- ---------------------------------------------------------------------
-- B1 TABLES. Explicit utf8mb3_unicode_ci = dh_donor, so COALESCE(dv.x, don.x) stays case-insensitive (no 1267).
-- No FOREIGN KEYs (the schema has none). Snapshot widths >= dh_donor widths (checked in B0); NOT NULL so a
-- bridged year never falls back to live through COALESCE (NULL and '' are the same value for the freeze).
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS dh_donor_version (
  dv_id            INT UNSIGNED NOT NULL AUTO_INCREMENT,
  dv_donor         INT UNSIGNED NOT NULL                 COMMENT 'dh_donor.d_id (= dh_donation.d_donor type)',
  dv_name          VARCHAR(255) NOT NULL DEFAULT '',
  dv_gender        VARCHAR(20)  NOT NULL DEFAULT '',
  dv_address       VARCHAR(255) NOT NULL DEFAULT '',
  dv_pincode       VARCHAR(20)  NOT NULL DEFAULT '',
  dv_contact       VARCHAR(60)  NOT NULL DEFAULT '',
  dv_email         VARCHAR(255) NOT NULL DEFAULT '',
  dv_occupation    VARCHAR(100) NOT NULL DEFAULT '',
  dv_country_code  VARCHAR(10)  NOT NULL DEFAULT '',
  dv_country       VARCHAR(255) NOT NULL DEFAULT ''      COMMENT 'name resolved at freeze time (renames never change frozen years)',
  dv_state_code    VARCHAR(10)  NOT NULL DEFAULT '',
  dv_state         VARCHAR(255) NOT NULL DEFAULT '',
  dv_city_id       BIGINT       NOT NULL DEFAULT 0,
  dv_city          VARCHAR(255) NOT NULL DEFAULT '',
  dv_id_type       INT          NOT NULL DEFAULT 0       COMMENT 'it_code (10BD ID Code), verbatim (0 stays 0)',
  dv_id_type_name  VARCHAR(100) NOT NULL DEFAULT '',
  dv_id_no         VARCHAR(60)  NOT NULL DEFAULT '',
  dv_address_proof VARCHAR(255) NOT NULL DEFAULT '',
  dv_course        BIGINT       NOT NULL DEFAULT 0,
  dv_confirmation  VARCHAR(10)  NOT NULL DEFAULT '',
  dv_source        VARCHAR(10)  NOT NULL DEFAULT ''      COMMENT 'freeze|correct|apply|seed',
  dv_created       DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'IST; the trigger writes UTC_TIMESTAMP()+330 min',
  dv_updatedby     INT          NOT NULL DEFAULT 0       COMMENT 'uid that caused it (0 = DIPI/system)',
  PRIMARY KEY (dv_id),
  KEY dv_donor_id (dv_donor, dv_id),
  KEY dv_name (dv_name),
  KEY dv_id_no (dv_id_no)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_unicode_ci
  COMMENT='Immutable donor snapshots (dana-donorver-3); never UPDATEd or DELETEd';

CREATE TABLE IF NOT EXISTS dh_donor_fy (
  df_donor     INT UNSIGNED NOT NULL,
  df_fy        SMALLINT     NOT NULL                  COMMENT 'FY start year; always < current FY at creation',
  df_version   INT UNSIGNED NOT NULL                  COMMENT 'dh_donor_version.dv_id (must have dv_donor = df_donor)',
  df_updated   DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  df_updatedby INT          NOT NULL DEFAULT 0,
  PRIMARY KEY (df_donor, df_fy),
  KEY df_version (df_version)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_unicode_ci
  COMMENT='Bridge (donor, FY) -> snapshot (dana-donorver-3)';

-- B1b post-create shape check (covers a table created by an older file between B0 and B1)
DELIMITER //
BEGIN NOT ATOMIC
  IF (SELECT CONCAT(t.ENGINE,'/',t.TABLE_COLLATION,'|',
        (SELECT GROUP_CONCAT(CONCAT_WS(':', c.COLUMN_NAME, c.COLUMN_TYPE, IFNULL(c.COLLATION_NAME,'-'), c.IS_NULLABLE, IFNULL(c.COLUMN_DEFAULT,'-'), c.EXTRA) ORDER BY c.ORDINAL_POSITION SEPARATOR ',')
           FROM information_schema.COLUMNS c WHERE c.TABLE_SCHEMA = DATABASE() AND c.TABLE_NAME = 'dh_donor_version'),'|',
        (SELECT GROUP_CONCAT(CONCAT(s.INDEX_NAME,':',s.SEQ_IN_INDEX,':',s.COLUMN_NAME,':',s.NON_UNIQUE) ORDER BY s.INDEX_NAME, s.SEQ_IN_INDEX SEPARATOR ',')
           FROM information_schema.STATISTICS s WHERE s.TABLE_SCHEMA = DATABASE() AND s.TABLE_NAME = 'dh_donor_version'))
        FROM information_schema.TABLES t WHERE t.TABLE_SCHEMA = DATABASE() AND t.TABLE_NAME = 'dh_donor_version') <> @dana_sig_dv
  OR (SELECT CONCAT(t.ENGINE,'/',t.TABLE_COLLATION,'|',
        (SELECT GROUP_CONCAT(CONCAT_WS(':', c.COLUMN_NAME, c.COLUMN_TYPE, IFNULL(c.COLLATION_NAME,'-'), c.IS_NULLABLE, IFNULL(c.COLUMN_DEFAULT,'-'), c.EXTRA) ORDER BY c.ORDINAL_POSITION SEPARATOR ',')
           FROM information_schema.COLUMNS c WHERE c.TABLE_SCHEMA = DATABASE() AND c.TABLE_NAME = 'dh_donor_fy'),'|',
        (SELECT GROUP_CONCAT(CONCAT(s.INDEX_NAME,':',s.SEQ_IN_INDEX,':',s.COLUMN_NAME,':',s.NON_UNIQUE) ORDER BY s.INDEX_NAME, s.SEQ_IN_INDEX SEPARATOR ',')
           FROM information_schema.STATISTICS s WHERE s.TABLE_SCHEMA = DATABASE() AND s.TABLE_NAME = 'dh_donor_fy'))
        FROM information_schema.TABLES t WHERE t.TABLE_SCHEMA = DATABASE() AND t.TABLE_NAME = 'dh_donor_fy') <> @dana_sig_df THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B1b: dh_donor_version/dh_donor_fy shape differs from rev 3 after create';
  END IF;
END//
DELIMITER ;

-- ---------------------------------------------------------------------
-- B2 d_fy column + index: one change per statement, online (L37). Each ALTER runs only when needed,
-- so a re-run never queues behind a long query for the metadata lock.
-- FYX(x) below = IF(x IS NULL OR YEAR(x) = 0 OR MONTH(x) = 0, 0, IF(MONTH(x) >= 4, YEAR(x), YEAR(x) - 1))
-- ---------------------------------------------------------------------
DELIMITER //
BEGIN NOT ATOMIC
  IF NOT EXISTS (SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE()
                  AND TABLE_NAME = 'dh_donation' AND COLUMN_NAME = 'd_fy') THEN
    ALTER TABLE dh_donation
      ADD COLUMN d_fy SMALLINT NOT NULL DEFAULT 0
        COMMENT 'FY start year from d_date (Apr-Mar); 0 = no valid date; set by triggers dh_don_bi_fy/dh_don_bu_fy',
      ALGORITHM=INPLACE, LOCK=NONE;
  END IF;
  IF NOT EXISTS (SELECT 1 FROM information_schema.STATISTICS WHERE TABLE_SCHEMA = DATABASE()
                  AND TABLE_NAME = 'dh_donation' AND INDEX_NAME = 'idx_donor_fy') THEN
    ALTER TABLE dh_donation ADD INDEX idx_donor_fy (d_donor, d_fy), ALGORITHM=INPLACE, LOCK=NONE;
  END IF;
END//
DELIMITER ;

-- B2b d_fy triggers (before the backfill, so rows written during it are already right). Explicit DEFINER (M11).
-- dh_don_bu_fy is a second BEFORE UPDATE trigger on dh_donation (MariaDB >= 10.2.3); order vs dh_don_bu is irrelevant
-- (d_fy is in neither dh_don_bu's nor dh_don_au's compare list: no d_update_count bump, no dh_log row).
DELIMITER //
CREATE OR REPLACE DEFINER=`root`@`localhost` TRIGGER dh_don_bi_fy BEFORE INSERT ON dh_donation FOR EACH ROW
  SET NEW.d_fy = IF(NEW.d_date IS NULL OR YEAR(NEW.d_date) = 0 OR MONTH(NEW.d_date) = 0, 0,
                    IF(MONTH(NEW.d_date) >= 4, YEAR(NEW.d_date), YEAR(NEW.d_date) - 1))//
CREATE OR REPLACE DEFINER=`root`@`localhost` TRIGGER dh_don_bu_fy BEFORE UPDATE ON dh_donation FOR EACH ROW
  SET NEW.d_fy = IF(NEW.d_date IS NULL OR YEAR(NEW.d_date) = 0 OR MONTH(NEW.d_date) = 0, 0,
                    IF(MONTH(NEW.d_date) >= 4, YEAR(NEW.d_date), YEAR(NEW.d_date) - 1))//
DELIMITER ;

-- ---------------------------------------------------------------------
-- B3 backfill, chunked by 5,000-row d_id ranges; each UPDATE commits on its own (autocommit inside the block),
-- so row locks are held for one chunk only (L37). Only wrong rows are written: a re-run writes nothing.
-- ---------------------------------------------------------------------
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE v_lo  BIGINT DEFAULT 0;
  DECLARE v_max BIGINT DEFAULT 0;
  SELECT IFNULL(MIN(d_id), 0), IFNULL(MAX(d_id), 0) INTO v_lo, v_max FROM dh_donation;
  WHILE v_lo <= v_max DO
    UPDATE dh_donation
       SET d_fy = IF(d_date IS NULL OR YEAR(d_date) = 0 OR MONTH(d_date) = 0, 0, IF(MONTH(d_date) >= 4, YEAR(d_date), YEAR(d_date) - 1))
     WHERE d_id >= v_lo AND d_id < v_lo + 5000
       AND d_fy <> IF(d_date IS NULL OR YEAR(d_date) = 0 OR MONTH(d_date) = 0, 0, IF(MONTH(d_date) >= 4, YEAR(d_date), YEAR(d_date) - 1));
    SET v_lo = v_lo + 5000;
  END WHILE;
END//
DELIMITER ;

-- ---------------------------------------------------------------------
-- B4 P2 delete guard (L11): refuse a hard DELETE (or a REPLACE that deletes) of a donor that has versioning rows.
-- Dana only soft-deletes donors (donor_delete sets d_deleted = 1), so this is inert for the app. Dropped by E2 before
-- the tables are parked (otherwise every donor DELETE would fail with 1146).
-- ---------------------------------------------------------------------
DELIMITER //
CREATE OR REPLACE DEFINER=`root`@`localhost` TRIGGER dh_donor_bd_guard BEFORE DELETE ON dh_donor FOR EACH ROW
BEGIN
  DECLARE v_ver VARCHAR(20)  CHARACTER SET ascii DEFAULT 'dana-donorver-3';
  DECLARE v_msg VARCHAR(128) CHARACTER SET ascii DEFAULT NULL;
  IF EXISTS (SELECT 1 FROM dh_donor_fy WHERE df_donor = OLD.d_id)
     OR EXISTS (SELECT 1 FROM dh_donor_version WHERE dv_donor = OLD.d_id) THEN
    SET v_msg = CONCAT('dh_donor_bd_guard: donor ', OLD.d_id, ' has FY versions; soft-delete it (d_deleted=1) instead');
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = v_msg;
  END IF;
END//
DELIMITER ;

-- ---------------------------------------------------------------------
-- B5 POST-CHECK (M9/L1): this DB must now be complete for phase 1. SIGNAL otherwise (the runner stops).
-- ---------------------------------------------------------------------
DELIMITER //
BEGIN NOT ATOMIC
  IF (SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA = DATABASE()
       AND TRIGGER_NAME IN ('dh_don_bi_fy','dh_don_bu_fy','dh_donor_bd_guard') AND DEFINER = 'root@localhost') <> 3 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B5: phase-1 triggers missing or wrong DEFINER';
  END IF;
  IF EXISTS (SELECT 1 FROM dh_donation
              WHERE d_fy <> IF(d_date IS NULL OR YEAR(d_date) = 0 OR MONTH(d_date) = 0, 0, IF(MONTH(d_date) >= 4, YEAR(d_date), YEAR(d_date) - 1))
              LIMIT 1) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'B5: d_fy backfill incomplete (D1 <> 0): re-run B';
  END IF;
  SELECT DATABASE() AS db, 'B ok' AS phase1,
         (SELECT COUNT(*) FROM dh_donation) AS donations,
         (SELECT COUNT(*) FROM dh_donation WHERE d_fy = 0) AS no_valid_date,
         (SELECT COUNT(*) FROM dh_donor_version) AS snapshots,
         (SELECT COUNT(*) FROM dh_donor_fy) AS bridges;
END//
DELIMITER ;
