#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}" "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
MYSQL_BIN=${MYSQL_BIN:-mysql}
if "$MYSQL_BIN" --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=("$MYSQL_BIN" "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }
check() {
    local label=$1 expected=$2 sql=$3 actual
    actual=$(scalar "SET SESSION group_concat_max_len=32768; $sql")
    if [[ "$actual" != "$expected" ]]; then
        printf 'FAILED: %s differs\nexpected: %s\nactual: %s\n' "$label" "$expected" "$actual" >&2
        exit 1
    fi
}

check projection_engine 'InnoDB:utf8mb4_unicode_ci' \
    "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='telemetry_reward_projection';"
check projection_columns '24' \
    "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_reward_projection';"
check projection_required_columns '24' \
    "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_reward_projection' AND column_name IN ('source_kind','operation_id','entry_index','participant_pid','source_table','source_created_at','authority_kind','reward_kind','gross_amount','net_amount','transfer_amount','parent_operation_id','economic_operation_id','reason_type','reason_id','source_site','is_transfer','is_creation','context_complete','status','quality_flags','source_payload_digest','cycle_id','projected_at');"
check projection_indexes 'PRIMARY|idx_reward_projection_economic|idx_reward_projection_participant|idx_reward_projection_scan|idx_reward_projection_status' \
    "SELECT GROUP_CONCAT(index_name ORDER BY BINARY index_name SEPARATOR '|') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_reward_projection';"
check projection_foreign_keys '' \
    "SELECT COALESCE(GROUP_CONCAT(constraint_name ORDER BY BINARY constraint_name SEPARATOR '|'),'') FROM information_schema.key_column_usage WHERE constraint_schema=DATABASE() AND table_name='telemetry_reward_projection' AND referenced_table_name IS NOT NULL;"

check source_scan_indexes 'idx_boon_reward_created_operation|idx_combat_frag_created_operation|idx_combat_outcome_created_operation|idx_currency_created_operation|idx_epic_created_operation|idx_zone_touch_outcome_created_operation' \
    "SELECT COALESCE(GROUP_CONCAT(DISTINCT index_name ORDER BY BINARY index_name SEPARATOR '|'),'') FROM information_schema.statistics WHERE table_schema=DATABASE() AND index_name IN ('idx_boon_reward_created_operation','idx_combat_frag_created_operation','idx_combat_outcome_created_operation','idx_currency_created_operation','idx_epic_created_operation','idx_zone_touch_outcome_created_operation');"

check state_engine 'InnoDB:utf8mb4_unicode_ci' \
    "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='telemetry_reward_projection_state';"
check state_columns '21' \
    "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_reward_projection_state';"
check state_required_columns '21' \
    "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_reward_projection_state' AND column_name IN ('source_kind','cycle_id','fast_cursor_created_at','fast_cursor_operation_id','fast_cursor_entry_index','fast_cursor_participant_pid','reconcile_cursor_created_at','reconcile_cursor_operation_id','reconcile_cursor_entry_index','reconcile_cursor_participant_pid','cycle_high_water','retention_floor','acknowledged_through','backlog_rows','quality_flags','provisional','last_fast_started_at','last_fast_completed_at','last_reconcile_started_at','last_reconcile_completed_at','updated_at');"
check state_indexes 'PRIMARY|idx_reward_projection_state_reconcile' \
    "SELECT GROUP_CONCAT(index_name ORDER BY BINARY index_name SEPARATOR '|') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_reward_projection_state';"
check state_foreign_keys '' \
    "SELECT COALESCE(GROUP_CONCAT(constraint_name ORDER BY BINARY constraint_name SEPARATOR '|'),'') FROM information_schema.key_column_usage WHERE constraint_schema=DATABASE() AND table_name='telemetry_reward_projection_state' AND referenced_table_name IS NOT NULL;"

printf 'telemetry reward projection schema verified\n'
