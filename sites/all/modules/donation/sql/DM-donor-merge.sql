-- Duplicate donors (2026-11): one donor record per person for new donations; the look-alike copies are hidden
-- (d_deleted = 1, as Delete) and remembered here (which copy went into which kept record), so a clean-up run can be
-- undone. A merge moves no donation. Safe to run any number of times.
-- Per site DB:  DB_EXCLUDE=dana_ahobilam_premerge ./dana-donorver-fleet.sh DM [--only DB] --execute
-- New sites:    create-dana-domain runs it after the template.
CREATE TABLE IF NOT EXISTS dh_donor_merge (
  mg_id         INT UNSIGNED NOT NULL AUTO_INCREMENT,
  mg_kept       BIGINT NOT NULL,
  mg_by         INT UNSIGNED NOT NULL DEFAULT 0,
  mg_at         DATETIME NOT NULL,
  mg_filled     TEXT NOT NULL,
  mg_undone_at  DATETIME NULL,
  mg_undone_by  INT UNSIGNED NULL,
  PRIMARY KEY (mg_id),
  KEY mg_kept (mg_kept)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_unicode_ci;
CREATE TABLE IF NOT EXISTS dh_donor_merge_hidden (
  mh_merge            INT UNSIGNED NOT NULL,
  mh_donor            BIGINT NOT NULL,
  mh_prev_deleted_by  INT NULL,
  PRIMARY KEY (mh_merge, mh_donor),
  KEY mh_donor (mh_donor)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_unicode_ci;
CREATE TABLE IF NOT EXISTS dh_donor_notsame (
  ns_a   BIGINT NOT NULL,
  ns_b   BIGINT NOT NULL,
  ns_by  INT UNSIGNED NOT NULL DEFAULT 0,
  ns_at  DATETIME NOT NULL,
  PRIMARY KEY (ns_a, ns_b),
  KEY ns_b (ns_b)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_unicode_ci;
