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

table=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='session_audit_outcome' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='session_audit_outcome' AND column_name IN ('operation_id','pid','event_type','occurred_at','created_at');")
indexes=$(read_scalar "SELECT COUNT(*) FROM (SELECT index_name FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='session_audit_outcome' AND index_name IN ('PRIMARY','idx_session_audit_player_time','idx_session_audit_event_time') GROUP BY index_name) exact_indexes;")
foreign_key=$(read_scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND constraint_name='session_audit_operation_fk';")
[[ "$table" == "1" && "$columns" == "5" && "$indexes" == "3" && "$foreign_key" == "1" ]]
echo "session audit outcome schema verified"
