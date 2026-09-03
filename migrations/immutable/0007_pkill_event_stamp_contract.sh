#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

# MySQL reports the converged default as CURRENT_TIMESTAMP and MariaDB as
# current_timestamp(); both satisfy the prefix match.
column_contract=$("${MYSQL[@]}" -e "
SELECT COUNT(*)
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name='pkill_event'
  AND column_name='stamp'
  AND data_type='datetime'
  AND is_nullable='NO'
  AND UPPER(column_default) LIKE 'CURRENT_TIMESTAMP%';")
zero_rows=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM pkill_event WHERE YEAR(stamp)=0;")

[[ "$column_contract" == 1 && "$zero_rows" == 0 ]]
echo "pkill event stamp contract verified"
