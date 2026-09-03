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
  AND table_name='kingdom_garrison'
  AND engine='InnoDB'
  AND table_collation='utf8mb4_unicode_ci';")
column_contract=$("${MYSQL[@]}" -e "
SELECT CONCAT(
  COUNT(*), ':',
  SUM(column_name IN ('assoc_id','slot') AND data_type='int' AND is_nullable='NO'), ':',
  SUM(column_name IN ('guard_class','level')
      AND data_type='int' AND is_nullable='NO' AND column_default='0')
)
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name='kingdom_garrison';")
column_order=$("${MYSQL[@]}" -e "
SELECT GROUP_CONCAT(column_name ORDER BY ordinal_position SEPARATOR ',')
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name='kingdom_garrison';")
indexes=$("${MYSQL[@]}" -e "
SELECT CONCAT(
  COUNT(*), ':',
  SUM(index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1
      AND column_name='assoc_id'), ':',
  SUM(index_name='PRIMARY' AND non_unique=0 AND seq_in_index=2
      AND column_name='slot')
)
FROM information_schema.statistics
WHERE table_schema=DATABASE()
  AND table_name='kingdom_garrison';")

[[ "$table_contract" == 1 && "$column_contract" == "4:2:2" &&
   "$column_order" == "assoc_id,slot,guard_class,level" &&
   "$indexes" == "2:1:1" ]]
echo "kingdom garrison schema verified"
