SET NAMES utf8mb4 COLLATE utf8mb4_general_ci;
SELECT VERSION() AS version, @@global.sql_mode AS global_sql_mode, @@global.time_zone AS tz, @@system_time_zone AS sys_tz,
       @@global.tx_isolation AS isolation, @@global.log_bin AS log_bin, @@global.binlog_format AS binlog_format,
       @@global.group_concat_max_len AS gc_max, @@global.innodb_lock_wait_timeout AS innodb_lwt, @@global.lock_wait_timeout AS mdl_lwt;
SELECT DEFAULT_CHARACTER_SET_NAME AS db_charset, DEFAULT_COLLATION_NAME AS db_collation, COUNT(*) AS dbs
  FROM information_schema.SCHEMATA WHERE SCHEMA_NAME LIKE 'dana\_%' GROUP BY 1, 2;
-- dh_donor shape across the fleet: expect ONE row per column (F6)
SELECT COLUMN_NAME, COLUMN_TYPE, IFNULL(COLLATION_NAME, '-') AS collation, IS_NULLABLE, COUNT(*) AS dbs
  FROM information_schema.COLUMNS WHERE TABLE_SCHEMA LIKE 'dana\_%' AND TABLE_NAME = 'dh_donor'
 GROUP BY 1, 2, 3, 4 ORDER BY 1, 5 DESC;
-- every trigger on dh_donor/dh_donation: definer, sql_mode, client charset, DB collation, normalised body hash (L36: dh_donor_au)
SELECT TRIGGER_NAME, DEFINER, SQL_MODE, CHARACTER_SET_CLIENT, DATABASE_COLLATION,
       LEFT(MD5(TRIM(REGEXP_REPLACE(REGEXP_REPLACE(ACTION_STATEMENT, '--[^\\n]*', ''), '\\s+', ' '))), 12) AS body12, COUNT(*) AS dbs
  FROM information_schema.TRIGGERS
 WHERE TRIGGER_SCHEMA LIKE 'dana\_%' AND EVENT_OBJECT_TABLE IN ('dh_donor', 'dh_donation')
 GROUP BY 1, 2, 3, 4, 5, 6 ORDER BY 1, 7 DESC;
-- versioning leftovers anywhere (expect none before phase 1)
SELECT TABLE_SCHEMA, TABLE_NAME FROM information_schema.TABLES
 WHERE TABLE_SCHEMA LIKE 'dana\_%' AND (TABLE_NAME LIKE 'dh\_donor\_version%' OR TABLE_NAME LIKE 'dh\_donor\_fy%');
