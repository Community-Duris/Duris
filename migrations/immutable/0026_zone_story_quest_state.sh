#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }

shape=$(scalar "SELECT COUNT(*)
FROM information_schema.tables
WHERE table_schema=DATABASE()
  AND table_name='zone_story_quest_state'
  AND engine='InnoDB'
  AND table_collation='utf8mb4_unicode_ci';")
[[ "$shape" == 1 ]] || { echo 'FAILED: zone-story state table engine/collation differs' >&2; exit 1; }

columns=$(scalar "SELECT COUNT(*)
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name='zone_story_quest_state'
  AND ((column_name='state_id' AND data_type='tinyint' AND column_type LIKE '%unsigned' AND is_nullable='NO')
    OR (column_name='state_version' AND data_type='int' AND column_type LIKE '%unsigned' AND is_nullable='NO')
    OR (column_name='catalog_revision' AND data_type='int' AND column_type LIKE '%unsigned' AND is_nullable='NO')
    OR (column_name='state_blob' AND data_type='mediumtext' AND is_nullable='NO')
    OR (column_name='updated_at' AND data_type='timestamp' AND is_nullable='NO'));")
[[ "$columns" == 5 ]] || { echo "FAILED: zone-story state columns differ; found $columns of 5" >&2; exit 1; }

primary=$(scalar "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index)
FROM information_schema.statistics
WHERE table_schema=DATABASE()
  AND table_name='zone_story_quest_state'
  AND index_name='PRIMARY';")
[[ "$primary" == 'state_id' ]] || { echo "FAILED: zone-story state primary key is $primary" >&2; exit 1; }

checks=$(scalar "SELECT COUNT(*)
FROM information_schema.table_constraints
WHERE constraint_schema=DATABASE()
  AND table_name='zone_story_quest_state'
  AND constraint_type='CHECK'
  AND constraint_name IN ('chk_zone_story_quest_state_singleton',
                          'chk_zone_story_quest_state_version',
                          'chk_zone_story_quest_state_revision');")
[[ "$checks" == 3 ]] || { echo "FAILED: zone-story state checks differ; found $checks of 3" >&2; exit 1; }

echo 'zone-story quest state schema verified'
