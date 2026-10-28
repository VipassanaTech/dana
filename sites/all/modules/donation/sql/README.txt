Donor versioning (rev 3) - database phase files and the fleet runner.
Run only through dana-donorver-fleet.sh, as root on the database server, one phase at a time,
each with a dry run first (no --execute) and then --execute.

Rollout:   B (every site)  ->  deploy the code that defines DONATION_DONORVER_CODE 3  ->  C (site by site)
           After each C:   D (checks), D9 (with @T_code = when the code deploy finished), smoke (D10)
Rollback:  E1 (every site)  ->  redeploy the previous code  ->  E2 (sets the tables aside, with a dump)
Re-enable: R1  ->  B  ->  deploy the code  ->  C

  B-phase1.sql          new column dh_donation.d_fy, tables dh_donor_version + dh_donor_fy, d_fy and delete-guard triggers
  C-phase3-freeze.sql   the freeze trigger on dh_donor (needs the code gate)
  D-checks.sql          read-only checks, also nightly (dana-donorver-nightly.sh)
  D8-preflight.sql      read-only fleet preflight
  D9-cutover.sql        cut-over order check and the list of donor edits made between the code deploy and C
  D10-smoke.sql         a test freeze, rolled back
  E1/E2/E-R1            rollback and re-enable

User Admin role (2026-10-28) — UA-user-admin.sql
  Adds per site: table dh_user_log (permanent account-change log), role 'User Admin', and the permissions
  'manage centre users' + 'view user admin log' for User Admin and administrator. Safe to re-run.
  Also (decision 2026-10-28): the site's 'Default' password policy covers 'authenticated user' (every user, from their
  next password set or change; existing passwords untouched) and is switched on where it was off (Laddha).
  Order: UA on all sites -> deploy the code -> on every site: drush cc menu + drush ev 'module_implements("", FALSE, TRUE);'
  Run:   DB_EXCLUDE=dana_ahobilam_premerge ./dana-donorver-fleet.sh UA [--only DB]   (dry run), then add --execute
  New sites: create-dana-domain runs UA-user-admin.sql after the template (with B and C).
  Rollback: revert the code first (git revert, pull, drush cc menu on every site). Then, optionally, per site:
    DELETE FROM role_permission WHERE permission IN ('manage centre users','view user admin log');
    DELETE ur FROM users_roles ur JOIN role r ON r.rid = ur.rid WHERE r.name = 'User Admin';
    DELETE FROM role WHERE name = 'User Admin';
    DELETE FROM password_policy_role WHERE rid = 2;   (only if the rules should again cover just Donation Admin/Restricted)
  Keep dh_user_log: it is the audit trail.
