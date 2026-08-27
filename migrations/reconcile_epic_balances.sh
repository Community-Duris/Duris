#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -f "$PROJECT_ROOT/.env" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}"
: "${DB_NAME:?DB_NAME is required}"

export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

missing=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM player_data p LEFT JOIN epic_balance_baseline b ON b.pid=p.pid WHERE b.pid IS NULL;")
mismatched=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM player_data p JOIN epic_balance_baseline b ON b.pid=p.pid LEFT JOIN (SELECT pid,SUM(delta) delta FROM epic_ledger GROUP BY pid) l ON l.pid=p.pid WHERE b.opening_balance+COALESCE(l.delta,0)<>p.epics;")
latest=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM epic_ledger l JOIN player_data p ON p.pid=l.pid WHERE l.epic_revision=p.epic_revision AND l.balance_after<>p.epics;")
printf 'epic reconciliation missing_baseline=%s balance_mismatch=%s latest_result_mismatch=%s\n' "$missing" "$mismatched" "$latest"
[[ "$missing" == 0 && "$mismatched" == 0 && "$latest" == 0 ]]
