#!/bin/bash
# =====================================================================
# dana-donorver-nightly.sh — nightly checks (register L6, M9). Prints ONLY problems; cron mails any output.
# /etc/cron.d/dana-donorver :
#   MAILTO=<owner address>
#   50 23 * * * root /usr/local/sbin/dana-donorver-nightly.sh
# 23:50 = after dana-setup (23:10) and before the date changes (reconciliation window = last 26 h).
# Sunday: full window (@dana_since = 2000-01-01) + a heartbeat line even when clean, so silence is never ambiguous.
# Exit 1 when anything FAILs (cron mail subject shows it).
# Expected phase comes from /etc/dana-donorver.phase (one of 0, 1, 3, RB), updated by the operator at each phase.
# =====================================================================
set -u -o pipefail
MYSQL=${MYSQL:-mysql}
SQLDIR=${SQLDIR:-/dhamma/web/apps/dana/sites/all/modules/donation/sql}
FLEET=${FLEET:-/usr/local/sbin/dana-donorver-fleet.sh}
PHASE_FILE=${PHASE_FILE:-/etc/dana-donorver.phase}
DIPI_LOG=${DIPI_LOG:-/var/log/dipi/dipi-sync.log}          # placeholder: the DIPI sync log path on vri-1
DOW=${DOW:-$(date +%u)}                                     # 7 = Sunday
EXPECT=$(cat "$PHASE_FILE" 2>/dev/null || echo "")
problems=0; out=""

say() { out="$out$*"$'\n'; }

# 1. fleet completeness per DB (fast SHOW TRIGGERS loop) against the expected phase
d7=$(EXPECT="$EXPECT" "$FLEET" D7 2>&1); rc=$?
if [ $rc -ne 0 ]; then problems=1; say "D7 (expected phase '${EXPECT:-?}'):"; say "$(echo "$d7" | grep '^dana_' | awk -v e="$EXPECT" '$2!=e || $3!="ok"')"; fi

# 2. per-DB checks (D1 D2 D3 D4 D4b IDREUSE SHAPE D5 D6b JSON BYPASS RECON TZ)
if [ "$DOW" = 7 ]; then SINCE='2000-01-01'; else SINCE=''; fi
d=$(SINCE="$SINCE" "$FLEET" D 2>&1); rc=$?
[ $rc -eq 0 ] || problems=1
[ -z "$(echo "$d" | grep -E $'\t(FAIL|WARN)\t')" ] || { say "Per-DB checks (FAIL/WARN only):"; say "$(echo "$d" | grep -E $'\t(FAIL|WARN)\t')"; }

# 3. fleet drift D6 (one body/definer/mode per trigger) — only meaningful from phase 1 on
if [ "$EXPECT" = 1 ] || [ "$EXPECT" = 3 ]; then
  d6=$("$FLEET" D6 2>&1) || { problems=1; say "D6 drift:"; say "$d6"; }
fi

# 4. today's DIPI write errors (a failed donor UPDATE inside the freeze shows up here first)
if [ -r "$DIPI_LOG" ]; then
  e=$(grep "$(date +%F)" "$DIPI_LOG" | grep -iE 'exec_query|SQLSTATE|ERROR 1[0-9]{3}' | tail -20)
  [ -z "$e" ] || { problems=1; say "DIPI errors today:"; say "$e"; }
fi

if [ $problems -ne 0 ] || [ -n "$out" ]; then
  echo "dana-donorver nightly $(date '+%F %T') — PROBLEMS"; echo "$out"
elif [ "$DOW" = 7 ]; then
  echo "dana-donorver weekly heartbeat $(date '+%F %T'): $(echo "$d7" | grep -c '^dana_') DBs checked, phase ${EXPECT:-?}, all checks OK (full window)."
fi
if [ "$DOW" = 7 ]; then   # weekly worklist: donations without a valid date (never frozen) per DB
  VERBOSE=1 SINCE="$SINCE" "$FLEET" D 2>/dev/null | awk -F'\t' '$2=="D2" && $3=="INFO"{print "  D2 " $1 ": " $4 " donation(s) without a valid date: " $5}'
fi
exit $problems
