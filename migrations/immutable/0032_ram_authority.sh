#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
for domain in shopkeeper corpse saved player_pet; do
  table="${domain}_item_runtime_state"
  parent="${domain}_items"
  shape=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='$table';")
  [[ "$shape" == 'InnoDB:utf8mb4_unicode_ci' ]]
  columns=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='$table' AND ((column_name='item_id' AND data_type='int' AND column_type LIKE '%unsigned' AND is_nullable='NO' AND column_key='PRI') OR (column_name='payload' AND data_type='mediumblob' AND is_nullable='NO'));")
  [[ "$columns" == 2 ]]
  fk=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND table_name='$table' AND referenced_table_name='$parent' AND delete_rule='CASCADE';")
  [[ "$fk" == 1 ]]
done
columns=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='player_data' AND column_name='base_stat_revision' AND data_type='bigint' AND column_type LIKE '%unsigned' AND is_nullable='NO') OR (table_name IN ('saved_items','shopkeeper_items') AND column_name='item_condition' AND data_type='smallint' AND is_nullable='NO'));" )
[[ "$columns" == 3 ]]
columns=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='shop_trade_state' AND ((column_name='shop_id' AND data_type='int' AND column_type LIKE '%unsigned' AND column_key='PRI') OR (column_name='revision' AND data_type='bigint' AND column_type LIKE '%unsigned'));")
[[ "$columns" == 2 ]]
printf 'RAM authority projection schema verified\n'
