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

tables=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('lifecycle_archive_jobs','lifecycle_archive_batches','lifecycle_archive_rows','lifecycle_archive_evidence') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
job_columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='lifecycle_archive_jobs' AND column_name IN ('job_id','job_key','policy_id','policy_schema_version','manifest_checksum','store_id','action','dry_run','target_environment','approval_reference','status','source_cursor','source_upper_bound','row_budget','byte_budget','time_budget_usec','source_count','archive_count','source_checksum','archive_checksum','reconciliation_before','reconciliation_after','last_error_code');")
batch_columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='lifecycle_archive_batches' AND column_name IN ('batch_id','batch_key','job_id','sequence_number','status','cursor_start','cursor_end','source_count','archive_count','source_bytes','source_checksum','archive_checksum','reconciliation_before','reconciliation_after','attempt_count','last_error_code');")
row_columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='lifecycle_archive_rows' AND column_name IN ('batch_id','source_key','source_checksum','payload','payload_bytes','archived_at');")
indexes=$(read_scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name FROM information_schema.statistics WHERE table_schema=DATABASE() AND ((table_name='lifecycle_archive_jobs' AND index_name IN ('PRIMARY','uq_lifecycle_archive_job_key','idx_lifecycle_archive_job_claim','idx_lifecycle_archive_job_store')) OR (table_name='lifecycle_archive_batches' AND index_name IN ('PRIMARY','uq_lifecycle_archive_batch_key','uq_lifecycle_archive_batch_sequence','uq_lifecycle_archive_batch_job_id','idx_lifecycle_archive_batch_resume')) OR (table_name='lifecycle_archive_rows' AND index_name='PRIMARY') OR (table_name='lifecycle_archive_evidence' AND index_name IN ('PRIMARY','idx_lifecycle_archive_evidence_job'))) GROUP BY table_name,index_name) exact_indexes;")
foreign_keys=$(read_scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND constraint_name IN ('lifecycle_archive_batch_job_fk','lifecycle_archive_row_batch_fk','lifecycle_archive_evidence_job_fk','lifecycle_archive_evidence_batch_fk');")

[[ "$tables" == "4" && "$job_columns" == "23" && "$batch_columns" == "16" && \
   "$row_columns" == "6" && "$indexes" == "12" && "$foreign_keys" == "4" ]]
echo "lifecycle archive execution schema verified"
