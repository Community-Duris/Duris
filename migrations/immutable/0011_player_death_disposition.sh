#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

table=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='player_death_disposition';")
columns=$("${MYSQL[@]}" -e "SET SESSION group_concat_max_len=16384; SELECT GROUP_CONCAT(CONCAT_WS(':',column_name,data_type,IF(column_type LIKE '%unsigned',1,0),is_nullable,COALESCE(LOWER(column_default),'<none>'),IF(data_type='binary',character_maximum_length,'-'),COALESCE(datetime_precision,'-'),IF(LOWER(extra) LIKE '%auto_increment%' OR LOWER(extra) LIKE '%on update%',1,0),IF(COALESCE(generation_expression,'')='',0,1)) ORDER BY ordinal_position SEPARATOR '|') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_death_disposition';")
indexes=$("${MYSQL[@]}" -e "SELECT GROUP_CONCAT(signature ORDER BY BINARY signature SEPARATOR '|') FROM (SELECT CONCAT(index_name,':',non_unique,':',index_type,':',SUM(sub_part IS NOT NULL),':',GROUP_CONCAT(column_name ORDER BY seq_in_index)) AS signature FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='player_death_disposition' GROUP BY index_name,non_unique,index_type) shapes;")
[[ "$table" == 'InnoDB:utf8mb4_unicode_ci' &&
   "$columns" == 'pid:int:0:NO:<none>:-:-:0:0|save_revision:bigint:1:NO:<none>:-:-:0:0|operation_id:binary:0:NO:<none>:16:-:0:0|corpse_item_uid:bigint:1:NO:<none>:-:-:0:0|corpse_room_vnum:int:0:NO:<none>:-:-:0:0|wallet_revision:bigint:1:NO:<none>:-:-:0:0|wallet_copper:int:0:NO:0:-:-:0:0|wallet_silver:int:0:NO:0:-:-:0:0|wallet_gold:int:0:NO:0:-:-:0:0|wallet_platinum:int:0:NO:0:-:-:0:0|wallet_pile_uid:bigint:1:NO:0:-:-:0:0|payload:mediumblob:0:NO:<none>:-:-:0:0|recorded_at:timestamp:0:NO:current_timestamp(6):-:6:0:0' &&
   "$indexes" == 'PRIMARY:0:BTREE:0:pid,save_revision|idx_player_death_disposition_corpse:1:BTREE:0:corpse_item_uid|idx_player_death_disposition_operation:1:BTREE:0:operation_id' ]] || { echo 'FAILED: player_death_disposition shape differs' >&2; printf '%s\n' "$table" "$columns" "$indexes" >&2; exit 1; }

table=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='player_death_custody';")
columns=$("${MYSQL[@]}" -e "SET SESSION group_concat_max_len=16384; SELECT GROUP_CONCAT(CONCAT_WS(':',column_name,data_type,IF(column_type LIKE '%unsigned',1,0),is_nullable,COALESCE(LOWER(column_default),'<none>'),IF(data_type='binary',character_maximum_length,'-'),COALESCE(datetime_precision,'-'),IF(LOWER(extra) LIKE '%auto_increment%' OR LOWER(extra) LIKE '%on update%',1,0),IF(COALESCE(generation_expression,'')='',0,1)) ORDER BY ordinal_position SEPARATOR '|') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_death_custody';")
indexes=$("${MYSQL[@]}" -e "SELECT GROUP_CONCAT(signature ORDER BY BINARY signature SEPARATOR '|') FROM (SELECT CONCAT(index_name,':',non_unique,':',index_type,':',SUM(sub_part IS NOT NULL),':',GROUP_CONCAT(column_name ORDER BY seq_in_index)) AS signature FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='player_death_custody' GROUP BY index_name,non_unique,index_type) shapes;")
[[ "$table" == 'InnoDB:utf8mb4_unicode_ci' &&
   "$columns" == 'pid:int:0:NO:<none>:-:-:0:0|save_revision:bigint:1:NO:<none>:-:-:0:0|item_uid:bigint:1:NO:<none>:-:-:0:0|root_item_uid:bigint:1:NO:<none>:-:-:0:0|parent_item_uid:bigint:1:NO:0:-:-:0:0|item_revision:bigint:1:NO:0:-:-:0:0|vnum:int:0:NO:0:-:-:0:0|state:tinyint:1:NO:0:-:-:0:0|owner_type:tinyint:1:NO:0:-:-:0:0|owner_id:bigint:1:NO:0:-:-:0:0|owner_context_id:bigint:1:NO:0:-:-:0:0|owner_revision:bigint:1:NO:0:-:-:0:0' &&
   "$indexes" == 'PRIMARY:0:BTREE:0:pid,save_revision,item_uid|idx_player_death_custody_item:1:BTREE:0:item_uid' ]] || { echo 'FAILED: player_death_custody shape differs' >&2; printf '%s\n' "$table" "$columns" "$indexes" >&2; exit 1; }
echo 'durable death disposition schema verified'
