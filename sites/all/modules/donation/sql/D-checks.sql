-- =====================================================================
-- D) CHECKS — read-only per DB (creates only a session TEMPORARY table). Rev 3.
-- Run before/after every phase and nightly (23:50 root cron, dana-donorver-nightly.sh).
--   mysql -N --batch dana_<sub> < D-checks.sql
-- Optional session inputs (set with --init-command):
--   @dana_since          start of the reconciliation / bypass / JSON window (default NOW() - 26 h; weekly run: '2000-01-01')
--   @dana_expect_phase   '0' | '1' | '3' | 'RB'  -> D7x FAIL when the detected phase differs
-- Output: one row per check: db, chk, status (OK|FAIL|WARN|INFO), val, detail. The cron prints FAIL and WARN only.
-- Works in every phase: statements touching dh_donor_version/dh_donor_fy run only when both tables exist.
-- Fleet-level checks (D6 drift across DBs, D7 fleet completeness via SHOW TRIGGERS, D8 preflight) live in the
-- shell runner / D8-preflight.sql; D9 (rollout order) and D10 (smoke test) are separate files.
-- Register items: M9, L5, L6, L10, L11, L13, L17, L39, M11.
-- =====================================================================
SET NAMES utf8mb4 COLLATE utf8mb4_general_ci;
SET SESSION time_zone = '+05:30';             -- l_tstamp/CREATED displayed in IST whatever the server default
SET SESSION group_concat_max_len = 65536;
SET @dana_since = IFNULL(@dana_since, NOW() - INTERVAL 26 HOUR);
SET @dana_sig_dv = 'InnoDB/utf8mb3_unicode_ci|dv_id:int(10) unsigned:-:NO:-:auto_increment,dv_donor:int(10) unsigned:-:NO:-:,dv_name:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_gender:varchar(20):utf8mb3_unicode_ci:NO:\'\':,dv_address:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_pincode:varchar(20):utf8mb3_unicode_ci:NO:\'\':,dv_contact:varchar(60):utf8mb3_unicode_ci:NO:\'\':,dv_email:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_occupation:varchar(100):utf8mb3_unicode_ci:NO:\'\':,dv_country_code:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_country:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_state_code:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_state:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_city_id:bigint(20):-:NO:0:,dv_city:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_id_type:int(11):-:NO:0:,dv_id_type_name:varchar(100):utf8mb3_unicode_ci:NO:\'\':,dv_id_no:varchar(60):utf8mb3_unicode_ci:NO:\'\':,dv_address_proof:varchar(255):utf8mb3_unicode_ci:NO:\'\':,dv_course:bigint(20):-:NO:0:,dv_confirmation:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_source:varchar(10):utf8mb3_unicode_ci:NO:\'\':,dv_created:datetime:-:NO:current_timestamp():,dv_updatedby:int(11):-:NO:0:|dv_donor_id:1:dv_donor:1,dv_donor_id:2:dv_id:1,dv_id_no:1:dv_id_no:1,dv_name:1:dv_name:1,PRIMARY:1:dv_id:0';
SET @dana_sig_df = 'InnoDB/utf8mb3_unicode_ci|df_donor:int(10) unsigned:-:NO:-:,df_fy:smallint(6):-:NO:-:,df_version:int(10) unsigned:-:NO:-:,df_updated:datetime:-:NO:current_timestamp():on update current_timestamp(),df_updatedby:int(11):-:NO:0:|df_version:1:df_version:1,PRIMARY:1:df_donor:0,PRIMARY:2:df_fy:0';
SET @dana_strict = 'STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';

DROP TEMPORARY TABLE IF EXISTS tmp_dana_checks;
CREATE TEMPORARY TABLE tmp_dana_checks (
  seq INT AUTO_INCREMENT PRIMARY KEY, chk VARCHAR(12), status VARCHAR(4), val BIGINT NULL, detail TEXT
) DEFAULT CHARSET=utf8mb4;

DELIMITER //
BEGIN NOT ATOMIC
  DECLARE v_now    DATETIME;
  DECLARE v_cur    SMALLINT;
  DECLARE v_tabs   INT DEFAULT 0;
  DECLARE v_parked INT DEFAULT 0;
  DECLARE v_dfy    INT DEFAULT 0;
  DECLARE v_trg    TEXT;
  DECLARE v_phase  VARCHAR(4);
  DECLARE v_n      BIGINT DEFAULT 0;
  DECLARE v_d      TEXT;
  DECLARE v_since  DATETIME;
  DECLARE v_fcreated DATETIME;

  SET v_now = UTC_TIMESTAMP() + INTERVAL 330 MINUTE;
  SET v_cur = IF(MONTH(v_now) >= 4, YEAR(v_now), YEAR(v_now) - 1);

  -- ---- D7 (this DB): detected phase. Constant schema filter: opens only this DB's files (L39). ----
  SELECT COUNT(*) INTO v_tabs FROM information_schema.TABLES
   WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME IN ('dh_donor_version','dh_donor_fy');
  SELECT COUNT(*) INTO v_parked FROM information_schema.TABLES
   WHERE TABLE_SCHEMA = DATABASE() AND (TABLE_NAME LIKE 'dh\_donor\_version\_rb\_%' OR TABLE_NAME LIKE 'dh\_donor\_fy\_rb\_%');
  SELECT COUNT(*) INTO v_dfy FROM information_schema.COLUMNS
   WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'dh_donation' AND COLUMN_NAME = 'd_fy';
  SELECT CONCAT(',', IFNULL(GROUP_CONCAT(TRIGGER_NAME ORDER BY TRIGGER_NAME), ''), ',') INTO v_trg
    FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA = DATABASE();
  SET v_phase = CASE
    WHEN v_tabs = 2 AND v_parked = 0 AND v_dfy = 1 AND v_trg LIKE '%,dh_don_bi_fy,%' AND v_trg LIKE '%,dh_don_bu_fy,%'
         AND v_trg LIKE '%,dh_donor_bd_guard,%' AND v_trg LIKE '%,dh_donor_bu_freeze,%' THEN '3'
    WHEN v_tabs = 2 AND v_parked = 0 AND v_dfy = 1 AND v_trg LIKE '%,dh_don_bi_fy,%' AND v_trg LIKE '%,dh_don_bu_fy,%'
         AND v_trg LIKE '%,dh_donor_bd_guard,%' AND v_trg NOT LIKE '%,dh_donor_bu_freeze,%' THEN '1'
    WHEN v_tabs = 0 AND v_parked = 2 AND v_trg NOT LIKE '%,dh_donor_bu_freeze,%' AND v_trg NOT LIKE '%,dh_donor_bd_guard,%' THEN 'RB'
    WHEN v_tabs = 0 AND v_parked = 0 AND v_trg NOT LIKE '%,dh_donor_bu_freeze,%' AND v_trg NOT LIKE '%,dh_donor_bd_guard,%'
         AND v_trg NOT LIKE '%,dh_don_bi_fy,%' AND v_trg NOT LIKE '%,dh_don_bu_fy,%' THEN '0'
    ELSE 'X' END;
  INSERT INTO tmp_dana_checks (chk, status, val, detail)
    VALUES ('D7', IF(v_phase = 'X', 'FAIL', 'OK'), NULL,
            CONCAT('phase=', v_phase, ' tables=', v_tabs, ' parked=', v_parked, ' d_fy=', v_dfy, ' triggers=', TRIM(BOTH ',' FROM v_trg)));
  IF @dana_expect_phase IS NOT NULL AND BINARY CAST(@dana_expect_phase AS CHAR) <> BINARY v_phase THEN
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('D7x', 'FAIL', NULL, CONCAT('expected phase ', @dana_expect_phase, ', found ', v_phase));
  END IF;
  -- legacy audit/count triggers must survive every phase and every restore (M9: a --skip-triggers restore drops them)
  IF v_trg NOT LIKE '%,dh_donor_au,%' OR v_trg NOT LIKE '%,dh_don_bu,%' OR v_trg NOT LIKE '%,dh_don_au,%' THEN
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('D7L', 'FAIL', NULL, 'legacy trigger dh_donor_au/dh_don_bu/dh_don_au missing');
  END IF;

  -- ---- D5: our triggers: pinned DEFINER, strict sql_mode, rev-3 marker where applicable (M11) ----
  SELECT COUNT(*), GROUP_CONCAT(CONCAT(TRIGGER_NAME, '(', DEFINER, ' ', SQL_MODE, ')') SEPARATOR '; ') INTO v_n, v_d
    FROM information_schema.TRIGGERS
   WHERE TRIGGER_SCHEMA = DATABASE() AND TRIGGER_NAME IN ('dh_don_bi_fy','dh_don_bu_fy','dh_donor_bd_guard','dh_donor_bu_freeze')
     AND (DEFINER <> 'root@localhost' OR SQL_MODE <> @dana_strict
          OR (TRIGGER_NAME IN ('dh_donor_bd_guard','dh_donor_bu_freeze') AND ACTION_STATEMENT NOT LIKE '%dana-donorver-3%'));
  INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('D5', IF(v_n = 0, 'OK', 'FAIL'), v_n, v_d);

  -- ---- D6b: every trigger's DEFINER account exists (else every write to that table fails with 1449) ----
  SELECT COUNT(*), GROUP_CONCAT(CONCAT(t.TRIGGER_NAME, ':', t.DEFINER) SEPARATOR ', ') INTO v_n, v_d
    FROM information_schema.TRIGGERS t
   WHERE t.TRIGGER_SCHEMA = DATABASE()
     AND NOT EXISTS (SELECT 1 FROM mysql.user u WHERE CONCAT(u.User, '@', u.Host) = t.DEFINER);
  INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('D6b', IF(v_n = 0, 'OK', 'FAIL'), v_n, v_d);

  -- ---- HASH: normalised body hash (comments and whitespace removed, L6) for the fleet drift check D6 ----
  SELECT GROUP_CONCAT(CONCAT(TRIGGER_NAME, '=',
           LEFT(MD5(TRIM(REGEXP_REPLACE(REGEXP_REPLACE(ACTION_STATEMENT, '--[^\\n]*', ''), '\\s+', ' '))), 12), '/', DEFINER, '/',
           CHARACTER_SET_CLIENT, '/', COLLATION_CONNECTION, '/', SQL_MODE) ORDER BY TRIGGER_NAME SEPARATOR ' ') INTO v_d
    FROM information_schema.TRIGGERS
   WHERE TRIGGER_SCHEMA = DATABASE() AND TRIGGER_NAME IN ('dh_don_bi_fy','dh_don_bu_fy','dh_donor_bd_guard','dh_donor_bu_freeze');
  INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('HASH', 'INFO', NULL, IFNULL(v_d, ''));

  -- ---- TZ (L13): the DB clock must be IST; the trigger itself does not depend on it ----
  INSERT INTO tmp_dana_checks (chk, status, val, detail)
    VALUES ('TZ', IF(@@global.time_zone IN ('+05:30') OR (@@global.time_zone = 'SYSTEM' AND @@system_time_zone = 'IST'), 'OK', 'WARN'), NULL,
            CONCAT('global=', @@global.time_zone, ' system=', @@system_time_zone));

  IF v_dfy = 1 THEN
    -- ---- D1: d_fy consistent with d_date (expect 0) ----
    SELECT COUNT(*) INTO v_n FROM dh_donation
     WHERE d_fy <> IF(d_date IS NULL OR YEAR(d_date) = 0 OR MONTH(d_date) = 0, 0, IF(MONTH(d_date) >= 4, YEAR(d_date), YEAR(d_date) - 1));
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('D1', IF(v_n = 0, 'OK', 'FAIL'), v_n, 'd_fy <> FYX(d_date)');
    -- ---- D2: donations without a valid date (never frozen; the centre should fix d_date) ----
    SELECT COUNT(*), LEFT(GROUP_CONCAT(d_id ORDER BY d_id), 200) INTO v_n, v_d FROM dh_donation WHERE d_fy <= 0;
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('D2', IF(v_n = 0, 'OK', 'INFO'), v_n, IFNULL(CONCAT('d_id ', v_d), ''));   -- weekly list, not a nightly alarm
  END IF;

  IF v_tabs = 2 THEN
    -- ---- SHAPE (L10) ----
    IF (SELECT CONCAT(t.ENGINE,'/',t.TABLE_COLLATION,'|',
          (SELECT GROUP_CONCAT(CONCAT_WS(':', c.COLUMN_NAME, c.COLUMN_TYPE, IFNULL(c.COLLATION_NAME,'-'), c.IS_NULLABLE, IFNULL(c.COLUMN_DEFAULT,'-'), c.EXTRA) ORDER BY c.ORDINAL_POSITION SEPARATOR ',')
             FROM information_schema.COLUMNS c WHERE c.TABLE_SCHEMA = DATABASE() AND c.TABLE_NAME = 'dh_donor_version'),'|',
          (SELECT GROUP_CONCAT(CONCAT(s.INDEX_NAME,':',s.SEQ_IN_INDEX,':',s.COLUMN_NAME,':',s.NON_UNIQUE) ORDER BY s.INDEX_NAME, s.SEQ_IN_INDEX SEPARATOR ',')
             FROM information_schema.STATISTICS s WHERE s.TABLE_SCHEMA = DATABASE() AND s.TABLE_NAME = 'dh_donor_version'))
          FROM information_schema.TABLES t WHERE t.TABLE_SCHEMA = DATABASE() AND t.TABLE_NAME = 'dh_donor_version') = @dana_sig_dv
     AND (SELECT CONCAT(t.ENGINE,'/',t.TABLE_COLLATION,'|',
          (SELECT GROUP_CONCAT(CONCAT_WS(':', c.COLUMN_NAME, c.COLUMN_TYPE, IFNULL(c.COLLATION_NAME,'-'), c.IS_NULLABLE, IFNULL(c.COLUMN_DEFAULT,'-'), c.EXTRA) ORDER BY c.ORDINAL_POSITION SEPARATOR ',')
             FROM information_schema.COLUMNS c WHERE c.TABLE_SCHEMA = DATABASE() AND c.TABLE_NAME = 'dh_donor_fy'),'|',
          (SELECT GROUP_CONCAT(CONCAT(s.INDEX_NAME,':',s.SEQ_IN_INDEX,':',s.COLUMN_NAME,':',s.NON_UNIQUE) ORDER BY s.INDEX_NAME, s.SEQ_IN_INDEX SEPARATOR ',')
             FROM information_schema.STATISTICS s WHERE s.TABLE_SCHEMA = DATABASE() AND s.TABLE_NAME = 'dh_donor_fy'))
          FROM information_schema.TABLES t WHERE t.TABLE_SCHEMA = DATABASE() AND t.TABLE_NAME = 'dh_donor_fy') = @dana_sig_df THEN
      INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('SHAPE', 'OK', NULL, 'rev 3');
    ELSE
      INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('SHAPE', 'FAIL', NULL, 'dh_donor_version/dh_donor_fy differ from the rev-3 signature');
    END IF;

    -- ---- D3: no current or future FY bridged (IST clock) ----
    SELECT COUNT(*) INTO v_n FROM dh_donor_fy WHERE df_fy >= v_cur;
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('D3', IF(v_n = 0, 'OK', 'FAIL'), v_n, CONCAT('df_fy >= ', v_cur));
    -- ---- D4: every bridge row points at a snapshot of the SAME donor ----
    SELECT COUNT(*) INTO v_n FROM dh_donor_fy m
      LEFT JOIN dh_donor_version v ON (v.dv_id = m.df_version AND v.dv_donor = m.df_donor) WHERE v.dv_id IS NULL;
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('D4', IF(v_n = 0, 'OK', 'FAIL'), v_n, 'bridge -> snapshot of another donor / missing');
    -- ---- D4b: versioning rows of donors that no longer exist (hard delete, wipe, partial restore) (L11) ----
    SELECT (SELECT COUNT(*) FROM dh_donor_fy m WHERE NOT EXISTS (SELECT 1 FROM dh_donor o WHERE o.d_id = m.df_donor))
         + (SELECT COUNT(*) FROM dh_donor_version v WHERE NOT EXISTS (SELECT 1 FROM dh_donor o WHERE o.d_id = v.dv_donor)) INTO v_n;
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('D4b', IF(v_n = 0, 'OK', 'FAIL'), v_n, 'bridge/snapshot rows without a dh_donor row');
    -- ---- IDREUSE: a snapshot older than its donor row = the id was reused (wipe/restore/explicit id) (L11) ----
    SELECT COUNT(DISTINCT v.dv_donor), LEFT(GROUP_CONCAT(DISTINCT v.dv_donor ORDER BY v.dv_donor), 200) INTO v_n, v_d
      FROM dh_donor_version v JOIN dh_donor o ON o.d_id = v.dv_donor
     WHERE v.dv_created < COALESCE(LEAST(o.d_created, o.d_tstamp), o.d_tstamp, o.d_created) - INTERVAL 5 MINUTE;
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('IDREUSE', IF(v_n = 0, 'OK', 'FAIL'), v_n, IFNULL(CONCAT('donors ', v_d), ''));
    -- ---- ORPHAN (info): snapshots no bridge points at (superseded corrections, a lost race) — never read ----
    SELECT COUNT(*) INTO v_n FROM dh_donor_version v WHERE NOT EXISTS (SELECT 1 FROM dh_donor_fy m WHERE m.df_version = v.dv_id);
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('ORPHAN', 'INFO', v_n, 'unreferenced snapshots');
    -- ---- JSON (L17): 'FY freeze' version must be a JSON number ----
    SELECT COUNT(*) INTO v_n FROM dh_log
     WHERE l_module = 'dh_donor' AND l_event = 'FY freeze' AND l_tstamp >= @dana_since
       AND IFNULL(JSON_TYPE(JSON_EXTRACT(l_msg, '$.version')), '') <> 'INTEGER';
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('JSON', IF(v_n = 0, 'OK', 'FAIL'), v_n, 'FY freeze rows whose version is not an INTEGER');
    -- ---- BYPASS (L16): freezes skipped through @dana_skip_freeze = 1 ----
    SELECT COUNT(*), LEFT(GROUP_CONCAT(CONCAT(l_identifier, '@', l_tstamp, ' u', l_user, ' ', l_msg) ORDER BY l_id SEPARATOR ' | '), 1000) INTO v_n, v_d
      FROM dh_log WHERE l_module = 'dh_donor' AND l_event = 'FY freeze skipped' AND l_tstamp >= @dana_since;
    INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('BYPASS', IF(v_n = 0, 'OK', 'WARN'), v_n, IFNULL(v_d, ''));

    -- ---- RECON (L6/M9): a captured 'Donor Updated' since the freeze was switched on that left a past FY
    --      (with a donation created before the edit) unbridged = a freeze that did not happen. ----
    -- window start = the latest 'freeze on' phase marker written by C (not the trigger's CREATED time, which a
    -- re-run or an accidental drop + re-create would move past the gap); skipped after 'freeze off' (E1)
    SELECT IF(l_event = 'freeze on', l_tstamp, NULL) INTO v_fcreated
      FROM dh_log WHERE l_module = 'dana-donorver' ORDER BY l_id DESC LIMIT 1;
    IF v_fcreated IS NOT NULL THEN
      SET v_since = GREATEST(@dana_since, v_fcreated);
      SELECT COUNT(*), LEFT(GROUP_CONCAT(CONCAT('donor ', x.donor, ' l_id ', x.l_id, ' @', x.l_tstamp, ' u', x.l_user, ' FY', x.d_fy)
                                         ORDER BY x.l_id SEPARATOR '; '), 1000) INTO v_n, v_d
        FROM (SELECT DISTINCT l.l_id, l.l_identifier donor, l.l_tstamp, l.l_user, d.d_fy
                FROM dh_log l
                JOIN dh_donation d ON d.d_donor = l.l_identifier
               WHERE l.l_module = 'dh_donor' AND l.l_event = 'Donor Updated' AND l.l_tstamp >= v_since
                 AND d.d_fy > 0
                 AND d.d_fy < IF(MONTH(l.l_tstamp) >= 4, YEAR(l.l_tstamp), YEAR(l.l_tstamp) - 1)
                 AND (d.d_created IS NULL OR d.d_created < l.l_tstamp)
                 AND NOT EXISTS (SELECT 1 FROM dh_donor_fy m WHERE m.df_donor = d.d_donor AND m.df_fy = d.d_fy)
                 AND NOT EXISTS (SELECT 1 FROM dh_log k WHERE k.l_module = 'dh_donor' AND k.l_identifier = l.l_identifier
                                    AND k.l_event = 'FY freeze skipped'
                                    AND k.l_tstamp BETWEEN l.l_tstamp - INTERVAL 2 SECOND AND l.l_tstamp + INTERVAL 2 SECOND)
                 -- the no-ID rule: d_id_type 0 <-> 99 alone is not a captured change (L21)
                 AND NOT (JSON_LENGTH(JSON_EXTRACT(l.l_msg, '$.new')) = 1
                          AND JSON_CONTAINS_PATH(l.l_msg, 'one', '$.new.d_id_type')
                          AND JSON_EXTRACT(l.l_msg, '$.old.d_id_type') IN (0, 99)
                          AND JSON_EXTRACT(l.l_msg, '$.new.d_id_type') IN (0, 99))) x;
      INSERT INTO tmp_dana_checks (chk, status, val, detail) VALUES ('RECON', IF(v_n = 0, 'OK', 'FAIL'), v_n, IFNULL(v_d, CONCAT('since ', v_since)));
    END IF;
  END IF;
END//
DELIMITER ;

SELECT DATABASE() AS db, chk, status, val, detail FROM tmp_dana_checks ORDER BY seq;
DROP TEMPORARY TABLE IF EXISTS tmp_dana_checks;
