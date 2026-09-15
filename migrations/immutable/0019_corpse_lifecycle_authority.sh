#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }

revision=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='corpses' AND column_name='corpse_revision' AND data_type='bigint' AND column_type LIKE 'bigint%unsigned' AND is_nullable='NO' AND column_default='1';")
[[ "$revision" == 1 ]] || { echo 'FAILED: corpse revision column differs' >&2; exit 1; }

owner_index=$(scalar "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='corpses' AND index_name='idx_corpse_owner_save';")
[[ "$owner_index" == 'value3,save_id' ]] || { echo "FAILED: corpse owner index is $owner_index" >&2; exit 1; }

table_shape=$(scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='corpse_catalog_state' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
singleton=$(scalar "SELECT COUNT(*) FROM corpse_catalog_state WHERE state_id=1 AND catalog_revision>0;")
singleton_rows=$(scalar "SELECT COUNT(*) FROM corpse_catalog_state;")
[[ "$table_shape" == 1 && "$singleton" == 1 && "$singleton_rows" == 1 ]] || { echo 'FAILED: corpse catalog singleton is absent or invalid' >&2; exit 1; }

checks=$(scalar "SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='corpse_catalog_state' AND constraint_type='CHECK' AND constraint_name IN ('chk_corpse_catalog_singleton','chk_corpse_catalog_revision');")
[[ "$checks" == 2 ]] || { echo "FAILED: corpse catalog checks differ; found $checks of 2" >&2; exit 1; }

echo 'corpse lifecycle authority schema verified'
