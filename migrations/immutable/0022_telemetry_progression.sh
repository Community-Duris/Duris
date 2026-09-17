#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }

columns=$(scalar "SELECT COUNT(*)
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name='telemetry_interval'
  AND is_nullable='YES'
  AND (COALESCE(column_default, 'NULL') = 'NULL')
  AND ((column_name='progression_kind' AND data_type='tinyint' AND column_type LIKE '%unsigned')
    OR (column_name='progression_source' AND data_type='tinyint' AND column_type LIKE '%unsigned')
    OR (column_name='progression_reason' AND data_type='tinyint' AND column_type LIKE '%unsigned')
    OR (column_name='progression_observation_status' AND data_type='tinyint' AND column_type LIKE '%unsigned')
    OR (column_name='progression_modifier_flags' AND data_type='int' AND column_type LIKE '%unsigned')
    OR (column_name='progression_requested_xp' AND data_type='bigint' AND column_type NOT LIKE '%unsigned')
    OR (column_name='progression_computed_xp' AND data_type='bigint' AND column_type NOT LIKE '%unsigned')
    OR (column_name='progression_applied_xp' AND data_type='bigint' AND column_type NOT LIKE '%unsigned')
    OR (column_name='progression_before_exp' AND data_type='bigint' AND column_type NOT LIKE '%unsigned')
    OR (column_name='progression_after_exp' AND data_type='bigint' AND column_type NOT LIKE '%unsigned')
    OR (column_name='progression_before_level' AND data_type='smallint' AND column_type LIKE '%unsigned')
    OR (column_name='progression_after_level' AND data_type='smallint' AND column_type LIKE '%unsigned')
    OR (column_name='progression_threshold_xp' AND data_type='bigint' AND column_type LIKE '%unsigned'));")
[[ "$columns" == 13 ]] || { echo "FAILED: progression columns differ; found $columns of 13" >&2; exit 1; }

echo 'telemetry progression schema verified'
