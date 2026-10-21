SET NAMES utf8mb4 COLLATE utf8mb4_general_ci;
SET SESSION lock_wait_timeout = 5;
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE v_ts VARCHAR(20) CHARACTER SET ascii DEFAULT NULL;
  DECLARE v_pairs INT DEFAULT 0;
  IF CURRENT_USER() <> 'root@localhost' THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'E-R1: run as root@localhost';
  END IF;
  SELECT COUNT(*), MAX(SUBSTRING(TABLE_NAME, 21)) INTO v_pairs, v_ts
    FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME LIKE 'dh\_donor\_version\_rb\_%';
  IF v_pairs = 0 THEN
    SELECT DATABASE() AS db, 'E-R1: nothing parked here (new centre or never versioned): B will create the tables' AS r1;
  ELSEIF v_pairs > 1 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'E-R1: more than one parked pair: choose one by hand';
  ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()
                      AND TABLE_NAME = CONCAT('dh_donor_fy_rb_', v_ts)) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'E-R1: parked pair incomplete (dh_donor_fy_rb_<ts> missing)';
  ELSEIF EXISTS (SELECT 1 FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()
                  AND TABLE_NAME IN ('dh_donor_version','dh_donor_fy')) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'E-R1: live versioning tables already exist next to the parked pair: merge by hand';
  ELSE
    EXECUTE IMMEDIATE CONCAT('RENAME TABLE dh_donor_version_rb_', v_ts, ' TO dh_donor_version, dh_donor_fy_rb_', v_ts, ' TO dh_donor_fy');
    SELECT DATABASE() AS db, CONCAT('E-R1 ok: renamed back from *_rb_', v_ts) AS r1;
  END IF;
END//
DELIMITER ;
