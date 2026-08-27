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
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then
    MYSQL_SSL=(--ssl-mode=PREFERRED)
else
    MYSQL_SSL=(--skip-ssl)
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" \
    -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

tables=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('personal_data_export_requests','personal_data_export_sections','personal_data_export_audit') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
request_columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='personal_data_export_requests' AND column_name IN ('request_id','request_key','account_name','account_scope_hash','policy_id','policy_schema_version','manifest_checksum','snapshot_id','status','attempt_count','expected_sections','completed_sections','excluded_sections','record_count','package_bytes','package_checksum','delivery_token_hash','last_error_code','requested_at','started_at','completed_at','cancelled_at','expires_at');")
section_columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='personal_data_export_sections' AND column_name IN ('request_id','store_id','disposition','status','snapshot_id','record_count','byte_count','section_checksum','exclusion_code','last_error_code','completed_at');")
audit_columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='personal_data_export_audit' AND column_name IN ('audit_id','request_id','event_type','status','section_count','record_count','byte_count','error_code','occurred_at');")
indexes=$(read_scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name FROM information_schema.statistics WHERE table_schema=DATABASE() AND ((table_name='personal_data_export_requests' AND index_name IN ('PRIMARY','uq_personal_export_request_key','idx_personal_export_account_rate','idx_personal_export_work','idx_personal_export_expiry')) OR (table_name='personal_data_export_sections' AND index_name IN ('PRIMARY','idx_personal_export_section_status')) OR (table_name='personal_data_export_audit' AND index_name IN ('PRIMARY','idx_personal_export_audit_request'))) GROUP BY table_name,index_name) exact_indexes;")
foreign_keys=$(read_scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND constraint_name IN ('personal_export_section_request_fk','personal_export_audit_request_fk');")

[[ "$tables" == "3" && "$request_columns" == "23" && \
   "$section_columns" == "11" && "$audit_columns" == "9" && \
   "$indexes" == "9" && "$foreign_keys" == "2" ]]
echo "personal data export schema verified"
