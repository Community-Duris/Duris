#!/usr/bin/env bash
set -euo pipefail

[[ "${1:-}" == "--apply" ]] || { echo 'usage: baseline_combat_frags.sh --apply' >&2; exit 2; }
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) && "${DB_NAME,,}" =~ (dev|local|test) ]] || {
    echo 'refusing combat baseline: environment and database must be development/local/test' >&2
    exit 1
}
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

advanced=$(read_scalar 'SELECT (SELECT COUNT(*) FROM combat_frag_ledger)+(SELECT COUNT(*) FROM player_data WHERE frag_revision<>0);')
if [[ "$advanced" != "0" ]]; then
    echo "combat frag authority already advanced; refusing baseline"
    exit 1
fi

"${MYSQL[@]}" -e 'INSERT IGNORE INTO combat_frag_baseline(pid,opening_frags,opening_revision) SELECT pid,frags,frag_revision FROM player_data;'
