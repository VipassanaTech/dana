SET NAMES utf8mb4 COLLATE utf8mb4_general_ci;
SET SESSION lock_wait_timeout = 5;
DELIMITER //
BEGIN NOT ATOMIC
  IF CURRENT_USER() <> 'root@localhost' THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'E2: run as root@localhost';
  END IF;
  IF @dana_rb_ts IS NULL OR CAST(@dana_rb_ts AS CHAR) NOT REGEXP '^[0-9]{12}$' THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'E2: set @dana_rb_ts to YYYYMMDDHHMM (the runner does this)';
  END IF;
  IF EXISTS (SELECT 1 FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA = DATABASE() AND TRIGGER_NAME = 'dh_donor_bu_freeze') THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'E2: dh_donor_bu_freeze still present: run E1 on every DB first';
  END IF;
  IF (SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()
       AND TABLE_NAME IN ('dh_donor_version','dh_donor_fy')) = 2 THEN
    -- E2a report what stops being shown ('correct'/'apply' rows exist nowhere in dh_donor)
    SELECT DATABASE() AS db, dv_source, COUNT(*) AS snapshots FROM dh_donor_version GROUP BY dv_source;
    SELECT DATABASE() AS db, COUNT(*) AS bridged_fys, COUNT(DISTINCT df_donor) AS donors FROM dh_donor_fy;
    -- E2b the delete guard reads the tables: drop it before they move (else every donor DELETE fails with 1146)
    DROP TRIGGER IF EXISTS dh_donor_bd_guard;
    -- E2c one atomic two-table rename
    EXECUTE IMMEDIATE CONCAT('RENAME TABLE dh_donor_version TO dh_donor_version_rb_', @dana_rb_ts,
                             ', dh_donor_fy TO dh_donor_fy_rb_', @dana_rb_ts);
    SELECT DATABASE() AS db, CONCAT('E2 ok: parked as *_rb_', @dana_rb_ts) AS e2;
  ELSEIF (SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()
           AND TABLE_NAME IN ('dh_donor_version','dh_donor_fy')) = 0 THEN
    DROP TRIGGER IF EXISTS dh_donor_bd_guard;
    SELECT DATABASE() AS db, 'E2: nothing to park (already parked or never created)' AS e2;
  ELSE
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'E2: only one versioning table present: stop and investigate';
  END IF;
END//
DELIMITER ;
