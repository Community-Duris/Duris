#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

if [[ -z "${DB_HOST:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWD:-}" || -z "${DB_NAME:-}" ]]; then
    if [[ -f "$PROJECT_ROOT/.env" ]]; then
        # shellcheck disable=SC1091
        source "$PROJECT_ROOT/.env"
    fi
fi

: "${DB_HOST:?DB_HOST is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}"
: "${DB_NAME:?DB_NAME is required}"

export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

tables=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('critical_operation_inbox','critical_test_state','critical_outbox','critical_outbox_delivery_dedupe') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='critical_operation_inbox' AND column_name IN ('operation_id','command_hash','keys_hash','command_type','schema_version','payload_version','status','result_code','durable_revision','result_payload','created_at','committed_at')) OR (table_name='critical_test_state' AND column_name IN ('entity_type','entity_id','value','revision','updated_at')) OR (table_name='critical_outbox' AND column_name IN ('outbox_id','operation_id','event_index','destination','event_type','payload_version','payload','status','attempt_count','next_attempt_at','created_at','delivered_at','dead_lettered_at','last_error_code')) OR (table_name='critical_outbox_delivery_dedupe' AND column_name IN ('consumer_id','outbox_id','delivered_at'))); ")
operation_id_columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='critical_operation_inbox' AND column_name='operation_id' AND column_type='binary(16)') OR (table_name='critical_outbox' AND column_name='operation_id' AND column_type='binary(16)')); ")
payload_bounds=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='critical_operation_inbox' AND column_name='result_payload' AND column_type='varbinary(4096)') OR (table_name='critical_outbox' AND column_name='payload' AND column_type='blob')); ")
indexes=$(read_scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name,non_unique,GROUP_CONCAT(column_name ORDER BY seq_in_index) signature FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name IN ('critical_operation_inbox','critical_test_state','critical_outbox','critical_outbox_delivery_dedupe') GROUP BY table_name,index_name,non_unique HAVING (table_name='critical_operation_inbox' AND index_name='PRIMARY' AND signature='operation_id') OR (table_name='critical_operation_inbox' AND index_name='idx_critical_inbox_status_created' AND signature='status,created_at') OR (table_name='critical_test_state' AND index_name='PRIMARY' AND signature='entity_type,entity_id') OR (table_name='critical_outbox' AND index_name='PRIMARY' AND signature='outbox_id') OR (table_name='critical_outbox' AND index_name='uq_critical_outbox_operation_event' AND non_unique=0 AND signature='operation_id,event_index') OR (table_name='critical_outbox' AND index_name='idx_critical_outbox_claim' AND signature='status,next_attempt_at,outbox_id') OR (table_name='critical_outbox' AND index_name='idx_critical_outbox_age' AND signature='status,created_at') OR (table_name='critical_outbox_delivery_dedupe' AND index_name='PRIMARY' AND signature='consumer_id,outbox_id')) exact_indexes;")
foreign_keys=$(read_scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND ((table_name='critical_outbox' AND constraint_name='critical_outbox_operation_fk' AND update_rule='RESTRICT' AND delete_rule='RESTRICT') OR (table_name='critical_outbox_delivery_dedupe' AND constraint_name='critical_outbox_delivery_fk' AND update_rule='RESTRICT' AND delete_rule='CASCADE')); ")

[[ "$tables" == 4 ]] || { echo "FAILED: expected 4 critical command InnoDB tables; found $tables" >&2; exit 1; }
[[ "$columns" == 34 ]] || { echo "FAILED: expected 34 named columns; found $columns" >&2; exit 1; }
[[ "$operation_id_columns" == 2 ]] || { echo 'FAILED: operation IDs must be binary(16)' >&2; exit 1; }
[[ "$payload_bounds" == 2 ]] || { echo 'FAILED: bounded result/outbox payload columns missing' >&2; exit 1; }
[[ "$indexes" == 8 ]] || { echo "FAILED: expected 8 exact critical indexes; found $indexes" >&2; exit 1; }
[[ "$foreign_keys" == 2 ]] || { echo "FAILED: expected 2 exact critical foreign keys; found $foreign_keys" >&2; exit 1; }
printf 'critical command schema verified: 4 tables, 34 columns, 8 indexes, 2 foreign keys\n'
