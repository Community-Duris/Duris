#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
clause=$("${MYSQL[@]}" -e "SELECT LOWER(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),CHAR(10),''))
FROM information_schema.table_constraints t JOIN information_schema.check_constraints c
ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
WHERE t.constraint_schema=DATABASE() AND t.table_name='zone_story_quest_state'
AND t.constraint_name='chk_zone_story_quest_state_version' AND t.constraint_type='CHECK';")
[[ "$clause" == '(state_versionin(1,2))' || "$clause" == 'state_versionin(1,2)' ]] || {
    echo 'FAILED: zone-story state version contract differs' >&2; exit 1;
}
clause=$("${MYSQL[@]}" -e "SELECT LOWER(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),CHAR(10),''))
FROM information_schema.table_constraints t JOIN information_schema.check_constraints c
ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
WHERE t.constraint_schema=DATABASE() AND t.table_name='zone_story_quest_state'
AND t.constraint_name='chk_zone_story_quest_state_singleton' AND t.constraint_type='CHECK';")
# Engines add different parentheses; compare the same expression after stripping.
clause=${clause//\(/}; clause=${clause//\)/}
[[ "$clause" == 'state_id>=1andstate_version=2orstate_id=1' ]] || {
    echo 'FAILED: zone-story state bucket contract differs' >&2; exit 1;
}
echo 'discovered-zone daily state versions and buckets verified'
