#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='player_item_runtime_state';")
[[ "$shape" == 'InnoDB:utf8mb4_unicode_ci' ]]
columns=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_item_runtime_state' AND ((column_name='item_id' AND data_type='int' AND column_type LIKE '%unsigned' AND is_nullable='NO' AND column_key='PRI') OR (column_name='payload' AND data_type='mediumblob' AND is_nullable='NO'));")
[[ "$columns" == 2 ]]
fk=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND table_name='player_item_runtime_state' AND referenced_table_name='player_items' AND delete_rule='CASCADE';")
[[ "$fk" == 1 ]]
printf 'player item runtime state migration verified\n'
