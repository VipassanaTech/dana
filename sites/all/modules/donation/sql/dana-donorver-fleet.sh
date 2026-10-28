#!/bin/bash
# =====================================================================
# dana-donorver-fleet.sh — fleet runner for Dana donor versioning rev 3 (outline, tested locally).
# Runs one phase file per DB over every dana_% schema that has dh_donation, as root via the unix socket.
#   Usage: dana-donorver-fleet.sh <cmd> [--execute] [--from DB] [--only DB[,DB...]]
#   cmds : preflight (D8) | B | P1 | C | D | D7 | D6 | smoke (D10) | E1 | E2 | R1
#   Without --execute it only lists the DBs and the file it would run (the dry run IS the one batch approval:
#   review it, then re-run the same command with --execute).
# Rules (register): one DB at a time; stop on the first error (no --force); retry lock waits (1205/1213) up to
# $RETRIES times, $RETRY_SLEEP s apart, after a PROCESSLIST/INNODB_TRX pre-check (L38); everything logged.
# Order (M10): B (all) -> deploy rev-3 code -> C (all, code gate) -> D.
#              Rollback: E0 template -> E1 (all; gate 0 left) -> redeploy HEAD -> E2 (all, + E3 dump) -> D7.
#              Re-enable: R1 (all) -> B (all) -> deploy rev-3 code -> C (all) -> D.
# =====================================================================
set -u -o pipefail

MYSQL=${MYSQL:-mysql}                     # prod: plain `mysql` as root (unix socket auth); never pass passwords on the CLI
MYSQLDUMP=${MYSQLDUMP:-mysqldump}
SQLDIR=${SQLDIR:-/dhamma/web/apps/dana/sites/all/modules/donation/sql}
LOGDIR=${LOGDIR:-/var/log/dana-donorver}
BACKUP_DIR=${BACKUP_DIR:-/root/backups/dana-donorver}
DB_LIKE=${DB_LIKE:-dana\\_%}
DB_EXCLUDE=${DB_EXCLUDE:-}                 # comma list of dana_ DBs never touched or checked (e.g. a kept backup copy)
RETRIES=${RETRIES:-5}
RETRY_SLEEP=${RETRY_SLEEP:-60}
LONG_TRX_SECS=${LONG_TRX_SECS:-30}
# code-version gate for C (M10): the deployed module must carry the rev-3 marker
CODE_FILE=${CODE_FILE:-/dhamma/web/apps/dana/sites/all/modules/donation/donation.module}
CODE_MARK=${CODE_MARK:-"define('DONATION_DONORVER_CODE', 3);"}   # = rev3-helpers.php line 8 (consistency review: was DANA_DONORVER_CODE, which the PHP never defines)
GATE_TOKEN='dana-donorver-3'

CMD=${1:-}; shift || true
EXECUTE=0; FROM=""; ONLY=""
while [ $# -gt 0 ]; do
  case "$1" in
    --execute) EXECUTE=1 ;;
    --from) FROM=$2; shift ;;
    --only) ONLY=$2; shift ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac; shift
done

TS=$(date +%Y%m%d%H%M)
mkdir -p "$LOGDIR" "$BACKUP_DIR"
LOG="$LOGDIR/$CMD-$TS.log"
log() { echo "$(date '+%F %T') $*" | tee -a "$LOG"; }
die() { log "STOP: $*"; exit 1; }

list_dbs() {
  local db
  for db in $($MYSQL -N -B -e "SELECT SCHEMA_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME LIKE '$DB_LIKE' ORDER BY 1"); do
    [ -n "$($MYSQL -N -B -e "SHOW TABLES FROM \`$db\` LIKE 'dh_donation'")" ] || continue
    if [ -n "$DB_EXCLUDE" ]; then case ",$DB_EXCLUDE," in *",$db,"*) continue ;; esac; fi
    if [ -n "$ONLY" ]; then case ",$ONLY," in *",$db,"*) ;; *) continue ;; esac; fi
    echo "$db"
  done
}

# L38: do not start DDL behind a long query or an old open transaction on this DB
wait_quiet() {
  local db=$1 n i
  for i in $(seq 1 "$RETRIES"); do
    n=$($MYSQL -N -B -e "SELECT
          (SELECT COUNT(*) FROM information_schema.PROCESSLIST WHERE DB = '$db' AND COMMAND NOT IN ('Sleep','Daemon') AND TIME > $LONG_TRX_SECS AND ID <> CONNECTION_ID())
        + (SELECT COUNT(*) FROM information_schema.INNODB_TRX t JOIN information_schema.PROCESSLIST p ON p.ID = t.trx_mysql_thread_id
            WHERE p.DB = '$db' AND t.trx_started < NOW() - INTERVAL $LONG_TRX_SECS SECOND)")
    [ "$n" = "0" ] && return 0
    log "$db: $n long query/transaction(s); waiting ${RETRY_SLEEP}s ($i/$RETRIES)"
    sleep "$RETRY_SLEEP"
  done
  return 1
}

# run one SQL file on one DB; retry only lock waits/deadlocks; any other error stops the fleet loop
run_file() {
  local db=$1 file=$2 init=${3:-} i rc out
  for i in $(seq 1 "$RETRIES"); do
    wait_quiet "$db" || die "$db: still busy after $RETRIES checks"
    out=$($MYSQL --show-warnings --batch ${init:+--init-command="$init"} "$db" < "$file" 2>&1); rc=$?
    echo "$out" >> "$LOG"
    if [ $rc -eq 0 ]; then log "$db: OK $(basename "$file")"; return 0; fi
    if echo "$out" | grep -qE 'ERROR (1205|1213)'; then
      log "$db: lock wait/deadlock on $(basename "$file"); retry in ${RETRY_SLEEP}s ($i/$RETRIES)"; sleep "$RETRY_SLEEP"; continue
    fi
    log "$db: FAILED $(basename "$file") rc=$rc: $(echo "$out" | grep -m1 ERROR)"
    return 1
  done
  return 1
}

# D7 fast (L39): SHOW TRIGGERS/TABLES/COLUMNS per DB, never correlated information_schema joins
phase_of() {
  local db=$1 trg tabs parked dfy
  trg=",$($MYSQL -N -B -e "SHOW TRIGGERS FROM \`$db\` LIKE 'dh\\_do%'" | cut -f1 | sort | paste -sd, -),"
  tabs=$($MYSQL -N -B -e "SHOW TABLES FROM \`$db\` WHERE Tables_in_$db IN ('dh_donor_version','dh_donor_fy')" | wc -l)
  parked=$($MYSQL -N -B -e "SHOW TABLES FROM \`$db\` LIKE 'dh\\_donor\\_%\\_rb\\_%'" | wc -l)
  dfy=$($MYSQL -N -B -e "SHOW COLUMNS FROM \`$db\`.dh_donation LIKE 'd_fy'" | wc -l)
  has() { case "$trg" in *",$1,"*) return 0 ;; *) return 1 ;; esac; }
  local legacy=ok; for t in dh_donor_au dh_don_bu dh_don_au; do has $t || legacy=MISSING_$t; done
  if [ "$tabs" = 2 ] && [ "$parked" = 0 ] && [ "$dfy" = 1 ] && has dh_don_bi_fy && has dh_don_bu_fy && has dh_donor_bd_guard; then
    if has dh_donor_bu_freeze; then echo "3 $legacy"; else echo "1 $legacy"; fi
  elif [ "$tabs" = 0 ] && [ "$parked" = 2 ] && ! has dh_donor_bu_freeze && ! has dh_donor_bd_guard; then echo "RB $legacy"
  elif [ "$tabs" = 0 ] && [ "$parked" = 0 ] && ! has dh_donor_bu_freeze && ! has dh_donor_bd_guard && ! has dh_don_bi_fy && ! has dh_don_bu_fy; then echo "0 $legacy"
  else echo "X $legacy"; fi
}

DBS=$(list_dbs)
[ -n "$DBS" ] || die "no dana_% DB with dh_donation found"
if [ -n "$FROM" ]; then DBS=$(echo "$DBS" | awk -v f="$FROM" '$0>=f'); fi

case "$CMD" in
  B)      FILE=$SQLDIR/B-phase1.sql ;;
  P1)     FILE=$SQLDIR/P1-dh_donor_au-charset.sql ;;
  C)      FILE=$SQLDIR/C-phase3-freeze.sql ;;
  D)      FILE=$SQLDIR/D-checks.sql ;;
  smoke)  FILE=$SQLDIR/D10-smoke.sql ;;
  E1)     FILE=$SQLDIR/E1-drop-freeze.sql ;;
  E2)     FILE=$SQLDIR/E2-park.sql ;;
  R1)     FILE=$SQLDIR/E-R1-rename-back.sql ;;
  UA)     FILE=$SQLDIR/UA-user-admin.sql ;;          # User Admin role + dh_user_log (2026-10-28), safe to re-run
  preflight) FILE=$SQLDIR/D8-preflight.sql ;;
  D7|D6)  FILE="" ;;
  *) echo "usage: $0 preflight|B|P1|C|D|D7|D6|smoke|E1|E2|R1|UA [--execute] [--from DB] [--only DB,...]" >&2; exit 2 ;;
esac
[ -z "$FILE" ] || [ -r "$FILE" ] || die "missing $FILE"

log "cmd=$CMD dbs=$(echo "$DBS" | wc -w) execute=$EXECUTE file=${FILE:-none}"
if [ "$EXECUTE" = 0 ] && ! echo " D D6 D7 preflight " | grep -q " $CMD "; then   # read-only cmds need no --execute
  echo "$DBS" | sed 's/^/  would run on: /'
  echo "dry run only; re-run with --execute"; exit 0
fi

case "$CMD" in
  preflight)
    $MYSQL --table < "$FILE" | tee -a "$LOG" ;;
  D7)
    bad=0
    for db in $DBS; do p=$(phase_of "$db"); echo "$db $p" | tee -a "$LOG"
      case "$p" in X*|*MISSING*) bad=1 ;; esac
      if [ -n "${EXPECT:-}" ] && [ "${p%% *}" != "$EXPECT" ]; then bad=1; fi
    done
    exit $bad ;;
  D6)
    for db in $DBS; do $MYSQL -N -B "$db" < "$SQLDIR/D-checks.sql" | awk -F'\t' '$2=="HASH"{print $5}'; done \
      | tr ' ' '\n' | grep -v '^$' | sort | uniq -c | tee -a "$LOG"
    n=$( (for db in $DBS; do $MYSQL -N -B "$db" < "$SQLDIR/D-checks.sql" | awk -F'\t' '$2=="HASH"{print $5}'; done) \
      | tr ' ' '\n' | grep -v '^$' | sort -u | cut -d= -f1 | uniq -d | wc -l)
    [ "$n" = 0 ] || die "D6: $n trigger(s) have more than one body/definer/mode variant across the fleet" ;;
  C)
    grep -qF "$CODE_MARK" "$CODE_FILE" || die "code gate: $CODE_FILE lacks $CODE_MARK (deploy the rev-3 code first)"
    log "code gate passed: $CODE_FILE"
    for db in $DBS; do run_file "$db" "$FILE" "SET @dana_c_gate='$GATE_TOKEN'" || die "C stopped at $db (resume: --from $db)"; done
    EXPECT=3 "$0" D7 --execute ${ONLY:+--only $ONLY} >/dev/null || die "D7 after C: not every DB is phase 3" ;;
  E1)
    for db in $DBS; do run_file "$db" "$FILE" || die "E1 stopped at $db"; done
    left=0; for db in $DBS; do
      [ -n "$($MYSQL -N -B -e "SHOW TRIGGERS FROM \`$db\` WHERE \`Trigger\` = 'dh_donor_bu_freeze'")" ] && { log "$db: freeze still present"; left=$((left+1)); }
    done
    [ "$left" = 0 ] || die "E1 gate: $left DB(s) still have dh_donor_bu_freeze — do NOT redeploy HEAD"
    log "E1 gate passed: 0 freeze triggers left; HEAD may be redeployed now" ;;
  E2)
    for db in $DBS; do
      run_file "$db" "$FILE" "SET @dana_rb_ts='$TS'" || die "E2 stopped at $db"
      if [ -n "$($MYSQL -N -B -e "SHOW TABLES FROM \`$db\` LIKE 'dh_donor_version_rb_$TS'")" ]; then
        out="$BACKUP_DIR/$TS-$db-donorver-parked.sql.gz"
        $MYSQLDUMP --single-transaction --skip-add-drop-table --skip-routines --skip-triggers --no-create-db \
          "$db" "dh_donor_version_rb_$TS" "dh_donor_fy_rb_$TS" | gzip > "$out" || die "E3 dump failed for $db"
        zcat "$out" | tail -1 | grep -q 'Dump completed' || die "E3 dump of $db incomplete"
        log "$db: E3 dump $out"
      fi
    done
    EXPECT=RB "$0" D7 --execute ${ONLY:+--only $ONLY} >/dev/null || log "WARN: some DBs are not in phase RB (new centres?) — see D7" ;;
  smoke)
    for db in $DBS; do
      out=$($MYSQL -N -B "$db" < "$FILE" 2>&1) || die "$db: D10 error: $out"
      echo "$out" | tee -a "$LOG"
      echo "$out" | grep -q $'\tD10\t.*\tFAIL$' && die "$db: D10 smoke test FAILED (the freeze did not run end to end)"
    done ;;
  D)
    bad=0
    for db in $DBS; do
      out=$($MYSQL -N -B ${SINCE:+--init-command="SET @dana_since='$SINCE'"} "$db" < "$FILE" 2>&1) || { log "$db: D-checks error: $out"; bad=1; continue; }
      echo "$out" >> "$LOG"
      echo "$out" | awk -F'\t' -v v="${VERBOSE:-0}" 'v==1 || $3=="FAIL" || $3=="WARN"'
      echo "$out" | awk -F'\t' '$3=="FAIL"{f=1} END{exit f}' || bad=1
    done
    exit $bad ;;
  *)
    for db in $DBS; do run_file "$db" "$FILE" || die "$CMD stopped at $db (resume: --from $db)"; done ;;
esac
log "done: $CMD"
