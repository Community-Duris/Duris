#!/usr/bin/env bash
#
# Report disagreements between the item payload tables and the ownership ledger.
#
# Both directions matter, and neither is fatal to a character load any more - the
# load path skips what it cannot reconcile and counts it. That is exactly why this
# exists: what used to announce itself as a lockout is now silent, and the only way
# to see it is to look.
#
#   orphan payload rows   player_items / player_pet_items with no item_current_owner
#                         row. The item loads today but is dropped from the snapshot
#                         and DELETED by the next full save. This is item loss.
#
#   missing payload rows  item_current_owner rows for a player whose payload row is
#                         gone. The item cannot be rebuilt; the row is inert.
#
#   ./scripts/item_ownership_audit.sh            # summary counts
#   ./scripts/item_ownership_audit.sh --detail   # every offending row
#
# Read-only: this script issues SELECTs and nothing else. Repair is a separate,
# deliberate act - see the deletion guidance in docs/guides/ if you need it, and
# dump any row before removing it, because that copy is the only record of the item.
set -euo pipefail

cd "$(dirname "$0")/.." || exit 1

DETAIL=0
case "${1-}" in
  --detail) DETAIL=1 ;;
  -h|--help) sed -n '3,22p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
  "") ;;
  *) echo "Unknown option: $1" >&2; exit 2 ;;
esac

if [[ ! -f .env ]]; then
  echo "ERROR: .env not found; it holds the database credentials." >&2
  exit 1
fi
# shellcheck disable=SC1091
source .env

for required in DB_HOST DB_USER DB_PASSWD DB_NAME; do
  if [[ -z "${!required-}" ]]; then
    echo "ERROR: $required is not set in .env." >&2
    exit 1
  fi
done

run_sql() {
  MYSQL_PWD="$DB_PASSWD" mysql -h"$DB_HOST" -u"$DB_USER" "$DB_NAME" --batch --table -e "$1"
}

ACTIVE_STATE=1   # item_custody_state::active

echo "Database: $DB_NAME on $DB_HOST"
echo

echo "== Orphan payload rows (item loss: dropped at load, deleted at next save) =="
if (( DETAIL )); then
  run_sql "
    SELECT 'character' AS source, pi.pid, pi.id, pi.vnum, pi.obj_uid
      FROM player_items pi
      LEFT JOIN item_current_owner own ON own.item_uid = pi.obj_uid
     WHERE own.item_uid IS NULL
    UNION ALL
    SELECT 'pet' AS source, pp.owner_pid AS pid, ppi.id, ppi.vnum, ppi.obj_uid
      FROM player_pet_items ppi
      JOIN player_pets pp ON pp.id = ppi.pet_id
      LEFT JOIN item_current_owner own ON own.item_uid = ppi.obj_uid
     WHERE own.item_uid IS NULL
     ORDER BY pid, source, id;"
else
  run_sql "
    SELECT source, COUNT(*) AS orphan_rows, COUNT(DISTINCT pid) AS characters
      FROM (
        SELECT 'character' AS source, pi.pid AS pid
          FROM player_items pi
          LEFT JOIN item_current_owner own ON own.item_uid = pi.obj_uid
         WHERE own.item_uid IS NULL
        UNION ALL
        SELECT 'pet' AS source, pp.owner_pid AS pid
          FROM player_pet_items ppi
          JOIN player_pets pp ON pp.id = ppi.pet_id
          LEFT JOIN item_current_owner own ON own.item_uid = ppi.obj_uid
         WHERE own.item_uid IS NULL
      ) orphans
     GROUP BY source;"
fi
echo

echo "== Ownership rows whose payload row is gone (inert ledger entries) =="
run_sql "
  SELECT own.owner_id AS pid, COUNT(*) AS missing_payload_rows
    FROM item_current_owner own
    LEFT JOIN (
      SELECT pi.obj_uid FROM player_items pi
      UNION ALL
      SELECT ppi.obj_uid FROM player_pet_items ppi
        JOIN player_pets pp ON pp.id = ppi.pet_id
    ) payload ON payload.obj_uid = own.item_uid
   WHERE own.owner_type = 1
     AND own.owner_context_id = 0
     AND own.state = $ACTIVE_STATE
     AND payload.obj_uid IS NULL
   GROUP BY own.owner_id
   ORDER BY missing_payload_rows DESC;"
echo

echo "== Characters over the load-time skip cap (PLAYER_LOAD_ITEM_SKIP_MAX = 32) =="
echo "   These characters are REFUSED at login rather than losing items silently."
run_sql "
  SELECT pid, orphan_rows FROM (
    SELECT pi.pid AS pid, COUNT(*) AS orphan_rows
      FROM player_items pi
      LEFT JOIN item_current_owner own ON own.item_uid = pi.obj_uid
     WHERE own.item_uid IS NULL
     GROUP BY pi.pid
  ) per_character
   WHERE orphan_rows > 32
   ORDER BY orphan_rows DESC;"
