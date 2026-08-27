#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -z "${DB_HOST:-}" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

tables=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('combat_frag_baseline','combat_outcome','combat_outcome_participant','combat_frag_ledger') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_data' AND column_name='frag_revision' AND data_type='bigint' AND column_type LIKE '%unsigned';")
indexes=$(read_scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name FROM information_schema.statistics WHERE table_schema=DATABASE() AND ((table_name='combat_outcome' AND index_name='PRIMARY') OR (table_name='combat_outcome_participant' AND index_name IN ('PRIMARY','uq_combat_participant')) OR (table_name='combat_frag_ledger' AND index_name IN ('PRIMARY','uq_combat_frag_pid_revision'))) GROUP BY table_name,index_name) exact_indexes;")
[[ "$tables" == "4" && "$columns" == "1" && "$indexes" == "5" ]]
echo "combat outcome schema verified"
