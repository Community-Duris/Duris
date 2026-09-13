#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
shape=$("${MYSQL[@]}" -e "SELECT GROUP_CONCAT(CONCAT(column_name,':',REPLACE(column_type,'(20)',''),':',is_nullable) ORDER BY ordinal_position SEPARATOR '|') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='artifact_mana';")
[[ "$shape" == 'item_uid:bigint unsigned:NO|profile_id:bigint unsigned:NO|profile_revision:bigint unsigned:NO|version:bigint unsigned:NO|capacity:bigint unsigned:NO|regeneration:bigint unsigned:NO|reserve:bigint unsigned:NO|settled_at:bigint unsigned:NO' ]] || { echo 'FAILED: artifact_mana columns differ' >&2; exit 1; }
engine=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='artifact_mana';")
indexes=$("${MYSQL[@]}" -e "SELECT GROUP_CONCAT(CONCAT(index_name,':',column_name,':',non_unique) ORDER BY index_name,seq_in_index SEPARATOR '|') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='artifact_mana';")
[[ "$engine" == InnoDB:utf8mb4_unicode_ci && "$indexes" == PRIMARY:item_uid:0 ]] || { echo 'FAILED: artifact_mana keys differ' >&2; exit 1; }
echo 'artifact mana schema verified'
