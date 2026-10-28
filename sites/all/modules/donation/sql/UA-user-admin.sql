-- User Admin role (spec "User Admin Role: Design Spec", 2026-10-28). Safe to run twice: every statement skips what exists.
-- Per site DB:  DB_EXCLUDE=dana_ahobilam_premerge ./dana-donorver-fleet.sh UA [--only DB] --execute
-- New sites:    create-dana-domain runs it after the template. Rollback: README.txt, "User Admin".
CREATE TABLE IF NOT EXISTS dh_user_log (
  ul_id          INT UNSIGNED NOT NULL AUTO_INCREMENT,
  ul_tstamp      DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  ul_actor_uid   INT UNSIGNED NOT NULL DEFAULT 0,
  ul_actor_name  VARCHAR(60) NOT NULL DEFAULT '',
  ul_target_uid  INT UNSIGNED NOT NULL DEFAULT 0,
  ul_target_name VARCHAR(60) NOT NULL DEFAULT '',
  ul_action      VARCHAR(20) NOT NULL DEFAULT '',
  ul_details     TEXT NOT NULL,
  ul_via         VARCHAR(20) NOT NULL DEFAULT '',
  ul_ip          VARCHAR(45) NOT NULL DEFAULT '',
  PRIMARY KEY (ul_id),
  KEY ul_target (ul_target_uid, ul_id),
  KEY ul_actor (ul_actor_uid, ul_id),
  KEY ul_tstamp (ul_tstamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- role.name is UNIQUE in Drupal 7, so IGNORE skips an existing role
INSERT IGNORE INTO role (name, weight)
  SELECT 'User Admin', COALESCE(MAX(weight), 0) + 1 FROM role;

-- role_permission's primary key is (rid, permission)
INSERT IGNORE INTO role_permission (rid, permission, module)
  SELECT rid, 'manage centre users', 'donation' FROM role WHERE name IN ('User Admin', 'administrator');
INSERT IGNORE INTO role_permission (rid, permission, module)
  SELECT rid, 'view user admin log', 'donation' FROM role WHERE name IN ('User Admin', 'administrator');

-- Password rules for every user (decision 2026-10-28): the site's "Default" policy (the only one on every site) also covers
-- 'authenticated user' (rid 2), so every role follows it from the next password set or change; existing passwords are not
-- touched (expiration stays as it is). A Default policy that was switched off (Laddha) is switched on.
INSERT IGNORE INTO password_policy_role (pid, rid)
  SELECT pid, 2 FROM password_policy WHERE name = 'Default';
UPDATE password_policy SET enabled = 1 WHERE name = 'Default' AND enabled = 0;

SELECT 'UA' AS phase,
  (SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'dh_user_log') AS log_table,
  (SELECT COUNT(*) FROM role WHERE name = 'User Admin') AS role_ok,
  (SELECT COUNT(*) FROM role_permission p JOIN role r ON r.rid = p.rid
     WHERE r.name IN ('User Admin', 'administrator') AND p.permission IN ('manage centre users', 'view user admin log')) AS perms,
  (SELECT COUNT(*) FROM password_policy_role pr JOIN password_policy p ON p.pid = pr.pid WHERE p.name = 'Default' AND pr.rid = 2) AS policy_all,
  (SELECT COUNT(*) FROM password_policy WHERE name = 'Default' AND enabled = 1) AS policy_on;
