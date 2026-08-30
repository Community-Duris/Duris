#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

valid=$("${MYSQL[@]}" -e "
SELECT COUNT(*)
FROM level_cap
WHERE id=1
  AND most_frags>=0
  AND racewar_leader BETWEEN 0 AND 4
  AND level BETWEEN 1 AND 56
  AND next_update IS NOT NULL;")
rows=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM level_cap;")

[[ "$valid" == 1 && "$rows" == 1 ]]
echo "level cap singleton verified"
