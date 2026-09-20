#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }

shape=$(scalar "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='critical_operation_inbox' AND table_type='BASE TABLE';")
[[ "$shape" == "InnoDB:utf8mb4_unicode_ci" ]] || {
    echo "FAILED: critical_operation_inbox engine/collation differs: $shape" >&2
    exit 1
}

column=$(scalar "SELECT COUNT(*)
FROM information_schema.columns stage
JOIN information_schema.columns result_code
  ON result_code.table_schema=stage.table_schema
 AND result_code.table_name=stage.table_name
 AND result_code.column_name='result_code'
WHERE stage.table_schema=DATABASE()
  AND stage.table_name='critical_operation_inbox'
  AND stage.column_name='failure_stage'
  AND stage.ordinal_position=result_code.ordinal_position+1
  AND stage.data_type='smallint'
  AND stage.column_type LIKE '%unsigned'
  AND stage.is_nullable='NO'
  AND stage.column_default='0';")
[[ "$column" == 1 ]] || {
    echo 'FAILED: failure_stage must be SMALLINT UNSIGNED NOT NULL DEFAULT 0 immediately after result_code' >&2
    exit 1
}

printf 'critical failure-stage migration verified: additive column shape and placement are exact\n'
