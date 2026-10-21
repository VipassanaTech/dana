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
