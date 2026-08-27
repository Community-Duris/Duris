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
tables=$(scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('account_erasure_requests','account_erasure_stores','account_erasure_evidence','account_erasure_tombstones') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
columns=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='account_erasure_requests' AND column_name IN ('request_id','request_key','account_scope_hash','subject_token','policy_id','policy_schema_version','manifest_checksum','status','fence_revision','expected_stores','completed_stores','retained_stores','reconciliation_checksum','last_error_code','requested_at','confirmed_at','fenced_at','completed_at','cancelled_at')) OR (table_name='account_erasure_stores' AND column_name IN ('request_id','store_id','action','status','sequence_number','affected_count','remaining_direct_identifiers','evidence_checksum','last_error_code','completed_at')) OR (table_name='account_erasure_evidence' AND column_name IN ('evidence_id','request_id','store_id','event_type','status','affected_count','remaining_count','error_code','occurred_at')) OR (table_name='account_erasure_tombstones' AND column_name IN ('subject_token','request_id','account_scope_hash','policy_id','policy_schema_version','manifest_checksum','completed_at','last_restore_generation','restore_apply_count')));")
indexes=$(scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name FROM information_schema.statistics WHERE table_schema=DATABASE() AND ((table_name='account_erasure_requests' AND index_name IN ('PRIMARY','uq_account_erasure_request_key','idx_account_erasure_scope_rate','idx_account_erasure_work')) OR (table_name='account_erasure_stores' AND index_name IN ('PRIMARY','uq_account_erasure_store_sequence','idx_account_erasure_store_work')) OR (table_name='account_erasure_evidence' AND index_name IN ('PRIMARY','idx_account_erasure_evidence_request')) OR (table_name='account_erasure_tombstones' AND index_name IN ('PRIMARY','uq_account_erasure_tombstone_request','idx_account_erasure_tombstone_scope'))) GROUP BY table_name,index_name) exact_indexes;")
foreign_keys=$(scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND constraint_name IN ('account_erasure_store_request_fk','account_erasure_evidence_request_fk','account_erasure_tombstone_request_fk');")
[[ "$tables" == 4 && "$columns" == 47 && "$indexes" == 12 && "$foreign_keys" == 3 ]]
echo "account erasure schema verified"
