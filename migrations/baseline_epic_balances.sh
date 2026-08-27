#!/usr/bin/env bash
set -euo pipefail

[[ "${1:-}" == "--apply" ]] || {
    echo 'usage: baseline_epic_balances.sh --apply' >&2
    exit 2
}
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
read_scalar() { "${MYSQL[@]}" -e "$1"; }

ledger_rows=$(read_scalar 'SELECT COUNT(*) FROM epic_ledger;')
advanced=$(read_scalar 'SELECT COUNT(*) FROM player_data WHERE epic_revision<>0;')
[[ "$ledger_rows" == 0 && "$advanced" == 0 ]] || {
    echo 'refusing epic baseline: ledger rows or advanced revisions already exist' >&2
    exit 1
}
"${MYSQL[@]}" -e 'INSERT IGNORE INTO epic_balance_baseline(pid,opening_balance,opening_revision) SELECT pid,epics,epic_revision FROM player_data;'
missing=$(read_scalar 'SELECT COUNT(*) FROM player_data p LEFT JOIN epic_balance_baseline b ON b.pid=p.pid WHERE b.pid IS NULL;')
[[ "$missing" == 0 ]] || { echo "FAILED: $missing players remain without epic baseline" >&2; exit 1; }
printf 'epic opening balances captured; existing baseline rows were preserved\n'
