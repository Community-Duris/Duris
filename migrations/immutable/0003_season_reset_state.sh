#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

schema=$("${MYSQL[@]}" -e "
SELECT COUNT(*)
FROM information_schema.tables t
JOIN information_schema.columns c
  ON c.table_schema=t.table_schema AND c.table_name=t.table_name
WHERE t.table_schema=DATABASE()
  AND t.table_name='season_reset_state'
  AND t.engine='InnoDB'
  AND t.table_collation='utf8mb4_unicode_ci'
  AND (c.column_name,c.data_type,c.is_nullable) IN (
    ('state_id','tinyint','NO'),
    ('season_epoch','bigint','NO'),
    ('reset_status','enum','NO'),
    ('reset_started_at','datetime','YES'),
    ('reset_completed_at','datetime','YES')
  );")
primary_key=$("${MYSQL[@]}" -e "
SELECT COUNT(*) FROM information_schema.statistics
WHERE table_schema=DATABASE() AND table_name='season_reset_state'
  AND index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1
  AND column_name='state_id';")
singleton=$("${MYSQL[@]}" -e "
SELECT COUNT(*) FROM season_reset_state
WHERE state_id=1 AND season_epoch>=1 AND reset_status IN ('active','resetting');")
rows=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM season_reset_state;")

[[ "$schema" == 5 && "$primary_key" == 1 && "$singleton" == 1 && "$rows" == 1 ]]
echo "season reset state verified"
