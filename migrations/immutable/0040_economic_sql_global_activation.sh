#!/usr/bin/env bash
set -euo pipefail

: "${DB_HOST:?DB_HOST is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}"
: "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then
    MYSQL_SSL=(--ssl-mode=PREFERRED)
else
    MYSQL_SSL=(--skip-ssl)
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp -h "$DB_HOST" -P "${DB_PORT:-3306}" \
    -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }

table=$(scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='economic_sql_global_activation' AND engine='InnoDB'")
columns=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='economic_sql_global_activation'")
binary_columns=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='economic_sql_global_activation' AND ((column_name IN ('lineage','epoch','installation_operation_id','baseline_operation_id') AND LOWER(column_type)='binary(16)') OR (column_name IN ('manifest_digest','audit_digest') AND LOWER(column_type)='binary(32)'))")
indexes=$(scalar "SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='economic_sql_global_activation'")
foreign_columns=$(scalar "SELECT COUNT(*) FROM information_schema.key_column_usage WHERE table_schema=DATABASE() AND table_name='economic_sql_global_activation' AND referenced_table_name IS NOT NULL")
checks=$(scalar "SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='economic_sql_global_activation' AND constraint_type='CHECK' AND constraint_name IN ('ck_economic_sql_global_activation_evidence','ck_economic_sql_global_activation_state')")
if [[ "$table" != 1 || "$columns" != 13 || "$binary_columns" != 6 ||
      "$indexes" != 5 || "$foreign_columns" != 4 || "$checks" != 2 ]]; then
    echo 'economic SQL global activation schema mismatch' >&2
    exit 1
fi
echo 'economic SQL global activation schema verified'
