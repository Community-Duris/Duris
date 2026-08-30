#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

table_contract=$("${MYSQL[@]}" -e "
SELECT COUNT(*)
FROM information_schema.tables
WHERE table_schema=DATABASE()
  AND table_name='server_reboots'
  AND engine='InnoDB'
  AND table_collation='utf8mb4_unicode_ci';")
column_contract=$("${MYSQL[@]}" -e "
SELECT CONCAT(
  COUNT(*), ':',
  SUM(column_name='record_id' AND data_type='bigint'
      AND column_type LIKE '%unsigned%'
      AND is_nullable='NO' AND extra='auto_increment'), ':',
  SUM(column_name IN ('boot_time','shutdown_time','uptime_seconds')
      AND data_type='bigint' AND column_type LIKE '%unsigned%'
      AND is_nullable='NO'), ':',
  SUM(column_name='shutdown_type' AND data_type='enum' AND is_nullable='NO'), ':',
  SUM(column_name='initiated_by' AND data_type='varchar'
      AND character_maximum_length=255 AND is_nullable='YES'), ':',
  SUM(column_name='reason' AND data_type='text' AND is_nullable='YES')
)
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name='server_reboots';")
indexes=$("${MYSQL[@]}" -e "
SELECT CONCAT(
  COUNT(*), ':',
  SUM(index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1
      AND column_name='record_id'), ':',
  SUM(index_name='idx_server_reboots_boot_time' AND non_unique=1
      AND seq_in_index=1 AND column_name='boot_time')
)
FROM information_schema.statistics
WHERE table_schema=DATABASE()
  AND table_name='server_reboots';")

[[ "$table_contract" == 1 && "$column_contract" == "7:1:3:1:1:1" &&
   "$indexes" == "2:1:1" ]]
echo "server reboot lifecycle schema verified"
