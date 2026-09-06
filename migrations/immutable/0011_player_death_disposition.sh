#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
disposition=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_death_disposition' AND column_name IN ('pid','save_revision','operation_id','corpse_item_uid','corpse_room_vnum','wallet_revision','wallet_copper','wallet_silver','wallet_gold','wallet_platinum','wallet_pile_uid','payload');")
[[ "$disposition" == 12 ]] || { echo 'FAILED: player_death_disposition missing or incompatible' >&2; exit 1; }
custody=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_death_custody' AND column_name IN ('pid','save_revision','item_uid','root_item_uid','parent_item_uid','item_revision','vnum','state','owner_type','owner_id','owner_context_id','owner_revision');")
[[ "$custody" == 12 ]] || { echo 'FAILED: player_death_custody missing or incompatible' >&2; exit 1; }
payload=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_death_disposition' AND column_name='payload' AND data_type='mediumblob' AND is_nullable='NO';")
[[ "$payload" == 1 ]] || { echo 'FAILED: death disposition payload column missing or incompatible' >&2; exit 1; }
echo 'durable death disposition schema verified'
