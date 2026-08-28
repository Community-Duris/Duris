#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

indexes=$("${MYSQL[@]}" -e "
SELECT COUNT(*) FROM (
  SELECT table_name,index_name,non_unique,
         GROUP_CONCAT(column_name ORDER BY seq_in_index) columns_signature,
         GROUP_CONCAT(COALESCE(sub_part,0) ORDER BY seq_in_index) prefix_signature
  FROM information_schema.statistics
  WHERE table_schema=DATABASE()
    AND index_name IN ('uk_item_descr','uk_pet_item_descr','uk_item_affect','uk_pet_item_affect')
  GROUP BY table_name,index_name,non_unique
  HAVING non_unique=0 AND (
    (table_name='player_item_extra_descr' AND index_name='uk_item_descr'
      AND columns_signature='item_id,keyword,description' AND prefix_signature='0,0,255') OR
    (table_name='player_pet_item_extra_descr' AND index_name='uk_pet_item_descr'
      AND columns_signature='item_id,keyword,description' AND prefix_signature='0,0,255') OR
    (table_name='player_item_affects' AND index_name='uk_item_affect'
      AND columns_signature='item_id,location,modifier' AND prefix_signature='0,0,0') OR
    (table_name='player_pet_item_affects' AND index_name='uk_pet_item_affect'
      AND columns_signature='item_id,location,modifier' AND prefix_signature='0,0,0')
  )
) verified_indexes;")

item_descr_duplicates=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM (SELECT item_id,keyword,LEFT(COALESCE(description,''),255) FROM player_item_extra_descr GROUP BY item_id,keyword,LEFT(COALESCE(description,''),255) HAVING COUNT(*)>1) duplicates;")
pet_descr_duplicates=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM (SELECT item_id,keyword,LEFT(COALESCE(description,''),255) FROM player_pet_item_extra_descr GROUP BY item_id,keyword,LEFT(COALESCE(description,''),255) HAVING COUNT(*)>1) duplicates;")
item_affect_duplicates=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM (SELECT item_id,location,modifier FROM player_item_affects GROUP BY item_id,location,modifier HAVING COUNT(*)>1) duplicates;")
pet_affect_duplicates=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM (SELECT item_id,location,modifier FROM player_pet_item_affects GROUP BY item_id,location,modifier HAVING COUNT(*)>1) duplicates;")

[[ "$indexes" == 4 && "$item_descr_duplicates" == 0 && "$pet_descr_duplicates" == 0 &&
   "$item_affect_duplicates" == 0 && "$pet_affect_duplicates" == 0 ]]
echo "player item metadata uniqueness verified"
