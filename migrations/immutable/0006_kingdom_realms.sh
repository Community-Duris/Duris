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
  AND table_name='kingdom_realms'
  AND engine='InnoDB'
  AND table_collation='utf8mb4_unicode_ci';")
column_contract=$("${MYSQL[@]}" -e "
SELECT CONCAT(
  COUNT(*), ':',
  SUM(column_name='assoc_id' AND data_type='int' AND is_nullable='NO'), ':',
  SUM(column_name IN ('realm_id','hall_vnum','highest_claim','arrears','missed_cycles')
      AND data_type='int' AND is_nullable='NO' AND column_default='0'), ':',
  SUM(column_name IN ('res_mineral','res_wood','res_fibre','res_water',
                      'upkeep_paid_through')
      AND data_type='bigint' AND is_nullable='NO' AND column_default='0')
)
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name='kingdom_realms';")
column_order=$("${MYSQL[@]}" -e "
SELECT GROUP_CONCAT(column_name ORDER BY ordinal_position SEPARATOR ',')
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name='kingdom_realms';")
indexes=$("${MYSQL[@]}" -e "
SELECT CONCAT(
  COUNT(*), ':',
  SUM(index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1
      AND column_name='assoc_id')
)
FROM information_schema.statistics
WHERE table_schema=DATABASE()
  AND table_name='kingdom_realms';")

[[ "$table_contract" == 1 && "$column_contract" == "11:1:5:5" &&
   "$column_order" == "assoc_id,realm_id,hall_vnum,highest_claim,res_mineral,res_wood,res_fibre,res_water,upkeep_paid_through,arrears,missed_cycles" &&
   "$indexes" == "1:1" ]]
echo "kingdom realm schema verified"
