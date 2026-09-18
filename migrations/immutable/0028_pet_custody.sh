#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
"${MYSQL[@]}" -e '
SELECT IF(
  (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE()
     AND TABLE_NAME="player_pets" AND COLUMN_NAME="pet_uid" AND DATA_TYPE="bigint"
     AND COLUMN_TYPE LIKE "%unsigned" AND IS_NULLABLE="YES")=1
  AND (SELECT COUNT(*) FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE()
     AND TABLE_NAME="player_pets" AND INDEX_NAME="uq_player_pets_pet_uid"
     AND NON_UNIQUE=0)=1
  AND (SELECT COUNT(*) FROM information_schema.check_constraints
       WHERE constraint_schema=DATABASE()
       AND constraint_name IN ("chk_item_owner_revision_type",
                               "chk_item_current_owner_type",
                               "chk_item_baseline_owner_type")
       AND LOWER(REPLACE(check_clause,CHAR(96),"")) REGEXP
           "owner_type[[:space:]]+between[[:space:]]+1[[:space:]]+and[[:space:]]+11")=3,
  "ok", "missing")' | grep -qx ok
"${MYSQL[@]}" -e 'START TRANSACTION;
  INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision)
  VALUES(11,18446744073709551614,0,0);
  ROLLBACK;'
