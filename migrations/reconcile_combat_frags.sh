#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

mismatched=$(read_scalar 'SELECT COUNT(*) FROM player_data p JOIN combat_frag_baseline b ON b.pid=p.pid LEFT JOIN (SELECT pid,SUM(delta) delta FROM combat_frag_ledger GROUP BY pid) l ON l.pid=p.pid WHERE b.opening_frags+COALESCE(l.delta,0)<>p.frags;')
latest=$(read_scalar 'SELECT COUNT(*) FROM combat_frag_ledger l JOIN player_data p ON p.pid=l.pid WHERE l.frag_revision=p.frag_revision AND l.frags_after<>p.frags;')
echo "combat_frag_mismatches=$mismatched latest_revision_mismatches=$latest"
[[ "$mismatched" == "0" && "$latest" == "0" ]]
