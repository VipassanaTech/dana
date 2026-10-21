SET NAMES utf8mb4 COLLATE utf8mb4_general_ci;
SET SESSION innodb_lock_wait_timeout = 5;
SET @d10_now = UTC_TIMESTAMP() + INTERVAL 330 MINUTE;
SET @d10_cur = IF(MONTH(@d10_now) >= 4, YEAR(@d10_now), YEAR(@d10_now) - 1);
SET @d10 = (SELECT d.d_donor FROM dh_donation d JOIN dh_donor o ON o.d_id = d.d_donor
             WHERE d.d_fy > 0 AND d.d_fy < @d10_cur
               AND NOT EXISTS (SELECT 1 FROM dh_donor_fy m WHERE m.df_donor = d.d_donor AND m.df_fy = d.d_fy)
             ORDER BY d.d_donor LIMIT 1);
START TRANSACTION;
SET @d10_v0 = (SELECT COUNT(*) FROM dh_donor_version WHERE dv_donor = IFNULL(@d10, 0));
SET @d10_b0 = (SELECT COUNT(*) FROM dh_donor_fy WHERE df_donor = IFNULL(@d10, 0));
UPDATE dh_donor SET d_course = d_course + 1 WHERE d_id = IFNULL(@d10, 0);
SELECT DATABASE() AS db, 'D10' AS chk, @d10 AS donor,
       (SELECT COUNT(*) FROM dh_donor_version WHERE dv_donor = IFNULL(@d10, 0)) - @d10_v0 AS new_snapshots,
       (SELECT COUNT(*) FROM dh_donor_fy WHERE df_donor = IFNULL(@d10, 0)) - @d10_b0 AS new_bridges,
       CASE WHEN @d10 IS NULL THEN 'n/a (no donor with an unbridged past FY)'
            WHEN (SELECT COUNT(*) FROM dh_donor_version WHERE dv_donor = @d10) - @d10_v0 = 1
             AND (SELECT COUNT(*) FROM dh_donor_fy WHERE df_donor = @d10) - @d10_b0 >= 1 THEN 'OK' ELSE 'FAIL' END AS status;
ROLLBACK;
