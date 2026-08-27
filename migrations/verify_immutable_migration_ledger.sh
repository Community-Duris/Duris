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
scalar() { "${MYSQL[@]}" -e "$1"; }
tables=$(scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('mud_schema_baselines','mud_schema_history','mud_schema_migration_state') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
columns=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='mud_schema_baselines' AND column_name IN ('baseline_id','baseline_kind','schema_fingerprint','manifest_version','runner_version','adopted_at')) OR (table_name='mud_schema_history' AND column_name IN ('migration_id','sequence_number','description','apply_checksum','verify_checksum','compatibility','runner_version','applied_at')) OR (table_name='mud_schema_migration_state' AND column_name IN ('state_id','applied_count','history_checksum','updated_at')));")
indexes=$(scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name FROM information_schema.statistics WHERE table_schema=DATABASE() AND ((table_name='mud_schema_baselines' AND index_name IN ('PRIMARY','uq_mud_schema_baseline_kind')) OR (table_name='mud_schema_history' AND index_name IN ('PRIMARY','uq_mud_schema_history_sequence')) OR (table_name='mud_schema_migration_state' AND index_name='PRIMARY')) GROUP BY table_name,index_name) exact_indexes;")
state=$(scalar "SELECT CONCAT(applied_count,':',LOWER(HEX(history_checksum))) FROM mud_schema_migration_state WHERE state_id=1;")
[[ "$tables" == 3 && "$columns" == 18 && "$indexes" == 5 && "$state" == "0:$(printf '' | sha256sum | cut -d' ' -f1)" ]]
echo "immutable migration ledger schema verified"
