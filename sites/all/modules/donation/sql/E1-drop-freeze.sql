SET SESSION lock_wait_timeout = 5;
DROP TRIGGER IF EXISTS dh_donor_bu_freeze;
-- phase marker (see C4): the reconciliation check stops expecting freezes from here on
INSERT INTO dh_log (l_module, l_identifier, l_event, l_msg, l_user)
  SELECT 'dana-donorver', 3, 'freeze off', JSON_OBJECT('by', CURRENT_USER()), 0 FROM DUAL
   WHERE IFNULL((SELECT l_event FROM dh_log WHERE l_module = 'dana-donorver' ORDER BY l_id DESC LIMIT 1), '') = 'freeze on';
SELECT DATABASE() AS db, 'E1 ok' AS e1,
       (SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA = DATABASE() AND TRIGGER_NAME = 'dh_donor_bu_freeze') AS freeze_left;
