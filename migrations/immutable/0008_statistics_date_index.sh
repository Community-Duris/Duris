#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

index_contract=$("${MYSQL[@]}" -e "
SELECT CONCAT(COUNT(*), ':', SUM(non_unique=1 AND seq_in_index=1 AND column_name='date'))
FROM information_schema.statistics
WHERE table_schema=DATABASE()
  AND table_name='statistics'
  AND index_name='idx_statistics_date';")

[[ "$index_contract" == "1:1" ]]
echo "statistics date index verified"
