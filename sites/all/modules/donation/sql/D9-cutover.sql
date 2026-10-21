SET NAMES utf8mb4 COLLATE utf8mb4_general_ci;
SET SESSION time_zone = '+05:30';
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE v_created DATETIME DEFAULT NULL;
  DECLARE v_first   DATETIME DEFAULT NULL;
  DECLARE v_off     DATETIME DEFAULT NULL;
  IF @T_code IS NULL THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'D9: set @T_code (code deploy end, IST) first';
  END IF;
  -- this cut-over's freeze-on = the first marker at/after the code deploy (G02: after a rollback + re-enable, an
  -- earlier rollout's marker must not be taken). C run before the code is still caught: no marker after T_code,
  -- so the trigger's CREATED (< T_code) is used and D9a fails.
  SELECT MIN(l_tstamp) INTO v_created FROM dh_log WHERE l_module = 'dana-donorver' AND l_event = 'freeze on' AND l_tstamp >= @T_code;
  IF v_created IS NULL THEN
    SELECT CREATED INTO v_created FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA = DATABASE() AND TRIGGER_NAME = 'dh_donor_bu_freeze';
  END IF;
  -- freezes of an earlier rollout (up to its freeze-off) are expected; only freezes since the last freeze-off count
  SELECT MAX(l_tstamp) INTO v_off FROM dh_log WHERE l_module = 'dana-donorver' AND l_event = 'freeze off' AND l_tstamp <= @T_code;
  SELECT MIN(l_tstamp) INTO v_first FROM dh_log WHERE l_module = 'dh_donor' AND l_event = 'FY freeze' AND l_tstamp >= IFNULL(v_off, '1000-01-01');
  SELECT DATABASE() AS db, 'D9a' AS chk,
         IF(v_created IS NOT NULL AND v_created >= @T_code AND (v_first IS NULL OR v_first >= @T_code), 'OK', 'FAIL') AS status,
         CONCAT('T_code=', @T_code, ' freeze_on=', IFNULL(v_created, 'none'), ' first_freeze=', IFNULL(v_first, 'none'),
                IF(v_off IS NULL, '', CONCAT(' (since freeze_off=', v_off, ')'))) AS detail;
  SELECT DATABASE() AS db, 'D9b' AS chk, l.l_id, l.l_identifier AS donor, l.l_tstamp, l.l_user, l.l_msg,
         GROUP_CONCAT(DISTINCT d.d_fy ORDER BY d.d_fy) AS past_fys_following_live
    FROM dh_log l JOIN dh_donation d ON d.d_donor = l.l_identifier
   WHERE l.l_module = 'dh_donor' AND l.l_event = 'Donor Updated'
     AND l.l_tstamp >= @T_code AND l.l_tstamp < IFNULL(v_created, NOW())
     AND d.d_fy > 0 AND d.d_fy < IF(MONTH(l.l_tstamp) >= 4, YEAR(l.l_tstamp), YEAR(l.l_tstamp) - 1)
     AND (d.d_created IS NULL OR d.d_created < l.l_tstamp)
     -- a year bridged BEFORE the edit kept its own details; a year bridged later (a freeze after C, before D9 ran)
     -- took the edited details, so it is still listed (G02). df_updated and l_tstamp are both IST.
     AND NOT EXISTS (SELECT 1 FROM dh_donor_fy m WHERE m.df_donor = d.d_donor AND m.df_fy = d.d_fy AND m.df_updated <= l.l_tstamp)
   GROUP BY l.l_id ORDER BY l.l_id;
END//
DELIMITER ;
