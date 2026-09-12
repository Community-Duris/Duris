#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
shape=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_pets' AND ((column_name='restore_state' AND data_type='mediumtext' AND is_nullable='YES') OR (column_name='hold_reason' AND data_type='int' AND column_type LIKE '%unsigned%' AND is_nullable='NO' AND column_default='0'));")
[[ "$shape" == 2 ]] || { echo 'FAILED: pet restore state columns differ' >&2; exit 1; }
echo 'pet restore state schema verified'
