-- Standard donation-app rights of the standard roles (2026-10-28). ADD-ONLY: inserts what is missing, never removes
-- anything, so centre-specific extras and custom roles stay as they are. Safe to run any number of times.
-- Why: centres created before a new right was added to dana-template.sql miss that right.
-- When a new right is added to a standard role, add it here and run on every site.
-- Per site DB:  DB_EXCLUDE=dana_ahobilam_premerge ./dana-donorver-fleet.sh STD [--only DB] --execute
-- New sites:    create-dana-domain runs it after the template and UA-user-admin.sql.
INSERT IGNORE INTO role_permission (rid, permission, module)
SELECT r.rid, s.p, 'donation'
FROM (
            SELECT 'authenticated user' AS r, 'access donor' AS p
  UNION ALL SELECT 'Donation Admin', 'access donation'
  UNION ALL SELECT 'Donation Admin', 'access donation report'
  UNION ALL SELECT 'Donation Admin', 'access income tax report'
  UNION ALL SELECT 'Donation Admin', 'administer donation'
  UNION ALL SELECT 'Donation Admin', 'assign donation tags'
  UNION ALL SELECT 'Donation Admin', 'create donation tags'
  UNION ALL SELECT 'Donation Admin', 'delete donation tags'
  UNION ALL SELECT 'Donation Admin', 'delete donors'
  UNION ALL SELECT 'Donation Admin', 'delete transaction'
  UNION ALL SELECT 'Donation Admin', 'download donor master'
  UNION ALL SELECT 'Donation Admin', 'download receipt'
  UNION ALL SELECT 'Donation Admin', 'edit delete after tally sync'
  UNION ALL SELECT 'Donation Admin', 'edit donors'
  UNION ALL SELECT 'Donation Admin', 'process mail queue'
  UNION ALL SELECT 'Donation Admin', 'sync to tally'
  UNION ALL SELECT 'Donation Admin', 'unassign donation tags'
  UNION ALL SELECT 'Donation Admin', 'upload donor master'
  UNION ALL SELECT 'Donation Admin', 'view donation tags'
  UNION ALL SELECT 'Donation Admin', 'view donors'
  UNION ALL SELECT 'Donation Admin', 'view log'
  UNION ALL SELECT 'Donation Restricted', 'access donation'
  UNION ALL SELECT 'Donation Restricted', 'access donation report'
  UNION ALL SELECT 'Donation Restricted', 'access own donation'
  UNION ALL SELECT 'Donation Restricted', 'download receipt'
  UNION ALL SELECT 'Donation Restricted', 'edit own donation'
  UNION ALL SELECT 'Donation Restricted', 'view donors'
  UNION ALL SELECT 'Donation Restricted', 'view log'
  UNION ALL SELECT 'Dhamma Server', 'access donation'
  UNION ALL SELECT 'Dhamma Server', 'access donation report'
  UNION ALL SELECT 'Dhamma Server', 'access own donation'
  UNION ALL SELECT 'Dhamma Server', 'download receipt'
  UNION ALL SELECT 'Dhamma Server', 'view donors'
  UNION ALL SELECT 'Dhamma Server', 'view log'
  UNION ALL SELECT 'Auditor', 'access donation report'
  UNION ALL SELECT 'Auditor', 'access income tax report'
  UNION ALL SELECT 'Auditor', 'download donor master'
  UNION ALL SELECT 'Auditor', 'download receipt'
  UNION ALL SELECT 'Auditor', 'sync to tally'
  UNION ALL SELECT 'Auditor', 'view donors'
  UNION ALL SELECT 'Auditor', 'view log'
  UNION ALL SELECT 'User Admin', 'manage centre users'
  UNION ALL SELECT 'User Admin', 'view user admin log'
) s
JOIN role r ON r.name = s.r;

SELECT 'STD' AS phase, ROW_COUNT() AS rights_added_by_this_run;
