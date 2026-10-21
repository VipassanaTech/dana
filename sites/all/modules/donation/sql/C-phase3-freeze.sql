-- =====================================================================
-- C) PHASE 3 — THE FREEZE TRIGGER (fleet, in ONE cut-over slot, only AFTER the rev-3 code is live)
-- Dana donor versioning rev 3 ("dana-donorver-3").
-- Run ONLY through the fleet runner:  dana-donorver-fleet.sh C
--   The runner checks the deployed module code version first (code-version gate, M10) and then passes
--   --init-command="SET @dana_c_gate='dana-donorver-3'". C0 refuses to run without that token, so C cannot
--   be applied by hand ahead of the code (the rev-2 rollback/re-enable hazard).
-- Self-contained trigger: no CALL, no procedure, no text parameters; donor text is copied from OLD.* directly
-- (only ascii-declared text locals), so the DB default charset (latin1 on 43 DBs) never touches donor bytes.
-- Re-runnable: CREATE OR REPLACE. Travels with a default mysqldump (--triggers is on by default).
-- Register items: U1 (15 fields), U4 (no ID-fill branches), L21 (no-ID equivalence), L13 (IST clock), M8 (anti-joins
-- on OLD.d_id), L3 (no explicit locking reads; loop over the FY list; log only rows actually bridged), L15 (changed
-- list + uid), L16 (strict @dana_skip_freeze + 'FY freeze skipped' log), L17 (version as a JSON number),
-- M11 (DEFINER pinned), L1/M9 (C0 preflight), D6/I13 (cancelled donations count).
-- =====================================================================
SET NAMES utf8mb4 COLLATE utf8mb4_general_ci;
SET SESSION sql_mode = 'STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';  -- stored with the trigger (D19)
SET SESSION lock_wait_timeout = 5;
SET SESSION group_concat_max_len = 65536;
SET @dana_sig_dv = 'InnoDB/utf8mb3_unicode_ci|dv_id:int(10) unsigned:-:NO:-:auto_increment,dv_donor:int(10) unsigned:-:NO:-:,dv_name:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_gender:varchar(20):utf8mb3_unicode_ci:NO:\'\':,dv_address:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_pincode:varchar(20):utf8mb3_unicode_ci:NO:\'\':,dv_contact:varchar(60):utf8mb3_unicode_ci:NO:\'\':,dv_email:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_occupation:varchar(100):utf8mb3_unicode_ci:NO:\'\':,dv_country_code:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_country:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_state_code:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_state:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_city_id:bigint(20):-:NO:0:,dv_city:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_id_type:int(11):-:NO:0:,dv_id_type_name:varchar(100):utf8mb3_unicode_ci:NO:\'\':,dv_id_no:varchar(60):utf8mb3_unicode_ci:NO:\'\':,dv_address_proof:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_course:bigint(20):-:NO:0:,dv_confirmation:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_source:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_created:datetime:-:NO:current_timestamp():,dv_updatedby:int(11):-:NO:0:|dv_donor_id:1:dv_donor:1,dv_donor_id:2:dv_id:1,dv_id_no:1:dv_id_no:1,dv_name:1:dv_name:1,PRIMARY:1:dv_id:0';
SET @dana_sig_df = 'InnoDB/utf8mb3_unicode_ci|df_donor:int(10) unsigned:-:NO:-:,df_fy:smallint(6):-:NO:-:,df_version:int(10) unsigned:-:NO:-:,df_updated:datetime:-:NO:current_timestamp():on update current_timestamp(),df_updatedby:int(11):-:NO:0:|df_version:1:df_version:1,PRIMARY:1:df_donor:0,PRIMARY:2:df_fy:0';

-- ---------------------------------------------------------------------
-- C0 PREFLIGHT (L1, M9, M10, M11, L8, L10). Changes nothing; SIGNALs on any problem.
-- ---------------------------------------------------------------------
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE v_n INT DEFAULT 0;
  IF CURRENT_USER() <> 'root@localhost' THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: run as root@localhost';
  END IF;
  IF @dana_c_gate IS NULL OR BINARY CAST(@dana_c_gate AS CHAR) <> BINARY 'dana-donorver-3' THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: code gate not passed: run C only via dana-donorver-fleet.sh C after the rev-3 code deploy';
  END IF;
  IF EXISTS (SELECT 1 FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()
              AND (TABLE_NAME LIKE 'dh\_donor\_version\_rb\_%' OR TABLE_NAME LIKE 'dh\_donor\_fy\_rb\_%')) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: parked *_rb_* tables present: rename back and run B first';
  END IF;
  IF (SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()
       AND TABLE_NAME IN ('dh_donor_version','dh_donor_fy')) <> 2 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: phase 1 (B) has not run here: versioning tables missing';
  END IF;
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
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: dh_donor_version/dh_donor_fy shape differs from rev 3 (L10)';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'dh_donation'
                  AND COLUMN_NAME = 'd_fy' AND COLUMN_TYPE = 'smallint(6)' AND IS_NULLABLE = 'NO')
     OR IFNULL((SELECT GROUP_CONCAT(COLUMN_NAME ORDER BY SEQ_IN_INDEX) FROM information_schema.STATISTICS
                 WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'dh_donation' AND INDEX_NAME = 'idx_donor_fy'), '') <> 'd_donor,d_fy' THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: dh_donation.d_fy or idx_donor_fy missing/wrong: run B';
  END IF;
  IF (SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA = DATABASE()
       AND TRIGGER_NAME IN ('dh_don_bi_fy','dh_don_bu_fy','dh_donor_bd_guard') AND DEFINER = 'root@localhost') <> 3 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: phase-1 triggers missing or wrong DEFINER: run B';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA = DATABASE()
                  AND TRIGGER_NAME = 'dh_donor_au' AND EVENT_OBJECT_TABLE = 'dh_donor') THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: legacy audit trigger dh_donor_au missing (restore without triggers?)';   -- M9
  END IF;
  IF EXISTS (SELECT 1 FROM dh_donation
              WHERE d_fy <> IF(d_date IS NULL OR YEAR(d_date) = 0 OR MONTH(d_date) = 0, 0, IF(MONTH(d_date) >= 4, YEAR(d_date), YEAR(d_date) - 1))
              LIMIT 1) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: D1 <> 0 (d_fy inconsistent): re-run B';
  END IF;
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
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: dh_donor column types/widths/collation differ from the expected prod shape';
  END IF;
  IF (SELECT IFNULL(MAX(d_id), 0) FROM dh_donor) > 4294967295 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C0: dh_donor.d_id exceeds INT UNSIGNED (dv_donor/df_donor)';
  END IF;
END//
DELIMITER ;

-- C1 the rev-1 draft two-object form must never coexist with the trigger
DROP PROCEDURE IF EXISTS dh_donor_freeze;

-- ---------------------------------------------------------------------
-- C2 THE FREEZE. BEFORE UPDATE on dh_donor, same statement / transaction / row lock as the donor UPDATE.
-- In words:
--  1. Validate @dana_skip_freeze: NULL, '' or 0 = normal; exactly 1 = skip (logged); anything else = error.
--  2. Compare the 15 captured fields OLD vs NEW byte-exact (BINARY, NULL treated as ''/0). ID type compares with the
--     no-ID rule: type 0 or 99 with an empty number is one value ("no ID"). System fields are never compared.
--  3. No captured change -> do nothing (no read of any other table).
--  4. CUR = FY of the IST date (UTC_TIMESTAMP() + 330 min; never the session time_zone).
--  5. Loop over the DISTINCT past FYs (0 < d_fy < CUR) in which this donor has donations (cancelled ones too) and
--     that have no bridge row yet. Both lookups are keyed on OLD.d_id (M8: index range, linear in bulk UPDATEs).
--  6. For the first such FY insert ONE verbatim snapshot of OLD.* (names resolved from the codes), then INSERT one bridge row
--     per FY; an FY that a concurrent writer bridged first (duplicate key, handled) is skipped and not logged.
--  7. Log 'FY freeze' with {version (number), fys actually bridged, changed fields, case_only fields, uid, stamped}.
--     With @dana_skip_freeze = 1 nothing is written except 'FY freeze skipped' {fys that now follow live, changed, uid, reason}.
-- ---------------------------------------------------------------------
DELIMITER //
CREATE OR REPLACE DEFINER=`root`@`localhost` TRIGGER dh_donor_bu_freeze BEFORE UPDATE ON dh_donor FOR EACH ROW
BEGIN
  DECLARE v_ver     VARCHAR(20)   CHARACTER SET ascii DEFAULT 'dana-donorver-3';   -- version marker (kept in ACTION_STATEMENT)
  DECLARE v_skip    TINYINT       DEFAULT 0;
  DECLARE v_changed VARCHAR(255)  CHARACTER SET ascii DEFAULT '';
  DECLARE v_case    VARCHAR(255)  CHARACTER SET ascii DEFAULT '';
  DECLARE v_now     DATETIME      DEFAULT NULL;
  DECLARE v_cur     SMALLINT      DEFAULT 0;
  DECLARE v_uid     INT           DEFAULT 0;
  DECLARE v_vid     INT UNSIGNED  DEFAULT NULL;
  DECLARE v_fy      SMALLINT      DEFAULT 0;
  DECLARE v_fys     VARCHAR(1000) CHARACTER SET ascii DEFAULT '';
  DECLARE v_eof     TINYINT       DEFAULT 0;
  DECLARE v_dup     TINYINT       DEFAULT 0;
  DECLARE c_fy CURSOR FOR
    SELECT DISTINCT d.d_fy
      FROM dh_donation d
     WHERE d.d_donor = OLD.d_id AND d.d_fy > 0 AND d.d_fy < v_cur
       AND NOT EXISTS (SELECT 1 FROM dh_donor_fy m WHERE m.df_donor = OLD.d_id AND m.df_fy = d.d_fy)
     ORDER BY d.d_fy;
  DECLARE CONTINUE HANDLER FOR NOT FOUND SET v_eof = 1;

  IF @dana_skip_freeze IS NULL OR BINARY CAST(@dana_skip_freeze AS CHAR) IN (BINARY '', BINARY '0') THEN
    SET v_skip = 0;
  ELSEIF BINARY CAST(@dana_skip_freeze AS CHAR) = BINARY '1' THEN
    SET v_skip = 1;
  ELSE
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'dh_donor_bu_freeze: @dana_skip_freeze must be NULL, 0 or 1';
  END IF;

  SET v_changed = CONCAT_WS(',',
    IF(BINARY IFNULL(OLD.d_name,'')          <> BINARY IFNULL(NEW.d_name,''),          'd_name', NULL),
    IF(BINARY IFNULL(OLD.d_gender,'')        <> BINARY IFNULL(NEW.d_gender,''),        'd_gender', NULL),
    IF(BINARY IFNULL(OLD.d_address,'')       <> BINARY IFNULL(NEW.d_address,''),       'd_address', NULL),
    IF(BINARY IFNULL(OLD.d_pincode,'')       <> BINARY IFNULL(NEW.d_pincode,''),       'd_pincode', NULL),
    IF(BINARY IFNULL(OLD.d_contact,'')       <> BINARY IFNULL(NEW.d_contact,''),       'd_contact', NULL),
    IF(BINARY IFNULL(OLD.d_email,'')         <> BINARY IFNULL(NEW.d_email,''),         'd_email', NULL),
    IF(BINARY IFNULL(OLD.d_occupation,'')    <> BINARY IFNULL(NEW.d_occupation,''),    'd_occupation', NULL),
    IF(BINARY IFNULL(OLD.d_country,'')       <> BINARY IFNULL(NEW.d_country,''),       'd_country', NULL),
    IF(BINARY IFNULL(OLD.d_state,'')         <> BINARY IFNULL(NEW.d_state,''),         'd_state', NULL),
    IF(IFNULL(OLD.d_city,0)                  <> IFNULL(NEW.d_city,0),                  'd_city', NULL),
    IF(IF(IFNULL(OLD.d_id_type,0) IN (0,99) AND BINARY IFNULL(OLD.d_id_no,'') = BINARY '', 99, IFNULL(OLD.d_id_type,0))
    <> IF(IFNULL(NEW.d_id_type,0) IN (0,99) AND BINARY IFNULL(NEW.d_id_no,'') = BINARY '', 99, IFNULL(NEW.d_id_type,0)),
                                                                                       'd_id_type', NULL),
    IF(BINARY IFNULL(OLD.d_id_no,'')         <> BINARY IFNULL(NEW.d_id_no,''),         'd_id_no', NULL),
    IF(BINARY IFNULL(OLD.d_address_proof,'') <> BINARY IFNULL(NEW.d_address_proof,''), 'd_address_proof', NULL),
    IF(IFNULL(OLD.d_course,0)                <> IFNULL(NEW.d_course,0),                'd_course', NULL),
    IF(BINARY IFNULL(OLD.d_confirmation,'')  <> BINARY IFNULL(NEW.d_confirmation,''),  'd_confirmation', NULL));

  IF v_changed <> '' THEN
    -- text fields whose change is invisible to the column collation (case, accents, trailing spaces): dh_donor_au logs nothing for them
    SET v_case = CONCAT_WS(',',
      IF(FIND_IN_SET('d_name', v_changed)          AND IFNULL(OLD.d_name,'')          = IFNULL(NEW.d_name,''),          'd_name', NULL),
      IF(FIND_IN_SET('d_gender', v_changed)        AND IFNULL(OLD.d_gender,'')        = IFNULL(NEW.d_gender,''),        'd_gender', NULL),
      IF(FIND_IN_SET('d_address', v_changed)       AND IFNULL(OLD.d_address,'')       = IFNULL(NEW.d_address,''),       'd_address', NULL),
      IF(FIND_IN_SET('d_pincode', v_changed)       AND IFNULL(OLD.d_pincode,'')       = IFNULL(NEW.d_pincode,''),       'd_pincode', NULL),
      IF(FIND_IN_SET('d_contact', v_changed)       AND IFNULL(OLD.d_contact,'')       = IFNULL(NEW.d_contact,''),       'd_contact', NULL),
      IF(FIND_IN_SET('d_email', v_changed)         AND IFNULL(OLD.d_email,'')         = IFNULL(NEW.d_email,''),         'd_email', NULL),
      IF(FIND_IN_SET('d_occupation', v_changed)    AND IFNULL(OLD.d_occupation,'')    = IFNULL(NEW.d_occupation,''),    'd_occupation', NULL),
      IF(FIND_IN_SET('d_country', v_changed)       AND IFNULL(OLD.d_country,'')       = IFNULL(NEW.d_country,''),       'd_country', NULL),
      IF(FIND_IN_SET('d_state', v_changed)         AND IFNULL(OLD.d_state,'')         = IFNULL(NEW.d_state,''),         'd_state', NULL),
      IF(FIND_IN_SET('d_id_no', v_changed)         AND IFNULL(OLD.d_id_no,'')         = IFNULL(NEW.d_id_no,''),         'd_id_no', NULL),
      IF(FIND_IN_SET('d_address_proof', v_changed) AND IFNULL(OLD.d_address_proof,'') = IFNULL(NEW.d_address_proof,''), 'd_address_proof', NULL),
      IF(FIND_IN_SET('d_confirmation', v_changed)  AND IFNULL(OLD.d_confirmation,'')  = IFNULL(NEW.d_confirmation,''),  'd_confirmation', NULL));
    SET v_now = UTC_TIMESTAMP() + INTERVAL 330 MINUTE;              -- IST wall clock, whatever the session time_zone (L13)
    SET v_cur = IF(MONTH(v_now) >= 4, YEAR(v_now), YEAR(v_now) - 1);
    SET v_uid = IFNULL(NEW.d_updatedby, 0);

    OPEN c_fy;
    fy_loop: LOOP
      FETCH c_fy INTO v_fy;
      IF v_eof = 1 THEN
        LEAVE fy_loop;
      END IF;
      IF v_skip = 0 THEN
        IF v_vid IS NULL THEN
          INSERT INTO dh_donor_version (dv_donor, dv_name, dv_gender, dv_address, dv_pincode, dv_contact, dv_email, dv_occupation,
              dv_country_code, dv_country, dv_state_code, dv_state, dv_city_id, dv_city,
              dv_id_type, dv_id_type_name, dv_id_no, dv_address_proof, dv_course, dv_confirmation,
              dv_source, dv_created, dv_updatedby)
          VALUES (OLD.d_id, IFNULL(OLD.d_name,''), IFNULL(OLD.d_gender,''), IFNULL(OLD.d_address,''), IFNULL(OLD.d_pincode,''),
              IFNULL(OLD.d_contact,''), IFNULL(OLD.d_email,''), IFNULL(OLD.d_occupation,''),
              IFNULL(OLD.d_country,''), IFNULL((SELECT c.c_name FROM dh_country c WHERE c.c_code = OLD.d_country ORDER BY c.c_id LIMIT 1), ''),
              IFNULL(OLD.d_state,''),   IFNULL((SELECT s.s_name FROM dh_state s WHERE s.s_code = OLD.d_state AND s.s_country = OLD.d_country ORDER BY s.s_id LIMIT 1), ''),
              IFNULL(OLD.d_city,0),     IFNULL((SELECT t.c_city FROM dh_city t WHERE t.c_id = OLD.d_city LIMIT 1), ''),
              IFNULL(OLD.d_id_type,0),  IFNULL((SELECT i.it_name FROM dh_id_types i WHERE i.it_code = OLD.d_id_type ORDER BY i.it_id LIMIT 1), ''),
              IFNULL(OLD.d_id_no,''),   IFNULL(OLD.d_address_proof,''), IFNULL(OLD.d_course,0), IFNULL(OLD.d_confirmation,''),
              'freeze', v_now, v_uid);
          SET v_vid = LAST_INSERT_ID();      -- the caller's LAST_INSERT_ID() is restored when the trigger ends
        END IF;
        SET v_dup = 0;
        BEGIN
          -- a concurrent writer bridged this FY first: keep its row, do not log the FY (ROW_COUNT() is not usable
          -- inside a trigger, and INSERT IGNORE would also downgrade other strict-mode errors)
          DECLARE CONTINUE HANDLER FOR 1062 SET v_dup = 1;
          INSERT INTO dh_donor_fy (df_donor, df_fy, df_version, df_updated, df_updatedby)
            VALUES (OLD.d_id, v_fy, v_vid, v_now, v_uid);
        END;
        IF v_dup = 0 THEN
          SET v_fys = CONCAT_WS(',', NULLIF(v_fys, ''), v_fy);
        END IF;
      ELSE
        SET v_fys = CONCAT_WS(',', NULLIF(v_fys, ''), v_fy);
      END IF;
    END LOOP;
    CLOSE c_fy;

    IF v_fys <> '' AND v_skip = 0 THEN
      INSERT INTO dh_log (l_module, l_identifier, l_event, l_msg, l_user)
        VALUES ('dh_donor', OLD.d_id, 'FY freeze',
                JSON_OBJECT('version', CAST(v_vid AS UNSIGNED), 'fys', v_fys, 'changed', v_changed, 'case_only', v_case,
                            'uid', CAST(v_uid AS SIGNED),
                            'stamped', IF(NEW.d_updated <=> OLD.d_updated AND NEW.d_updatedby <=> OLD.d_updatedby, 0, 1),
                            'trigger', v_ver),
                v_uid);
    ELSEIF v_fys <> '' THEN
      INSERT INTO dh_log (l_module, l_identifier, l_event, l_msg, l_user)
        VALUES ('dh_donor', OLD.d_id, 'FY freeze skipped',
                JSON_OBJECT('fys', v_fys, 'changed', v_changed, 'uid', CAST(v_uid AS SIGNED),
                            'reason', REGEXP_REPLACE(LEFT(IFNULL(CAST(@dana_skip_reason AS CHAR), ''), 100), '[^-A-Za-z0-9 _.,:/#()]', ''),
                            'trigger', v_ver),
                v_uid);
    END IF;
  END IF;
END//
DELIMITER ;

-- ---------------------------------------------------------------------
-- C3 POST-CHECK: trigger present, pinned DEFINER, strict sql_mode, rev-3 marker.
-- C4 PHASE MARKER: one dh_log row ('dana-donorver', 3, 'freeze on') when the freeze is switched on (first install or
--    re-enable); a re-run of C writes nothing. E1 writes 'freeze off'. The RECON check and D9 use the latest marker,
--    so an accidental DROP + re-create of the trigger can never move the reconciliation window past the gap.
-- ---------------------------------------------------------------------
DELIMITER //
BEGIN NOT ATOMIC
  IF NOT EXISTS (SELECT 1 FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA = DATABASE()
                  AND TRIGGER_NAME = 'dh_donor_bu_freeze' AND DEFINER = 'root@localhost'
                  AND FIND_IN_SET('STRICT_TRANS_TABLES', SQL_MODE) > 0
                  AND ACTION_STATEMENT LIKE '%dana-donorver-3%') THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'C3: dh_donor_bu_freeze missing or not the rev-3 body';
  END IF;
  IF IFNULL((SELECT l_event FROM dh_log WHERE l_module = 'dana-donorver' ORDER BY l_id DESC LIMIT 1), '') <> 'freeze on' THEN
    INSERT INTO dh_log (l_module, l_identifier, l_event, l_msg, l_user)
      VALUES ('dana-donorver', 3, 'freeze on', JSON_OBJECT('trigger', 'dana-donorver-3', 'by', CURRENT_USER()), 0);
  END IF;
  SELECT DATABASE() AS db, 'C ok' AS phase3, TRIGGER_NAME, DEFINER, SQL_MODE, CREATED
    FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA = DATABASE() AND TRIGGER_NAME = 'dh_donor_bu_freeze';
END//
DELIMITER ;
