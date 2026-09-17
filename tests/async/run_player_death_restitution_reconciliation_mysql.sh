#!/usr/bin/env bash
# Real disposable-DB positive and conflict/no-write artifact reconciliation gate.
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
DB_IMAGE=${DURIS_TEST_DB_IMAGE:-mariadb:10.11}
DB_CONTAINER="duris-issue-331-artifact-db-$$"
NETWORK="duris-issue-331-artifact-net-$$"
DB_NAME=duris_issue_331_test
DB_PASSWORD=issue331-artifact-test-only
TMP=$(mktemp -d)
MYSQL_WRAPPER="$TMP/mysql"
FIXTURE_BIN="$TMP/fixture"
PAYLOAD_FILE="$TMP/payloads"
INSPECT="$TMP/inspect.json"
PLAN="$TMP/plan.json"
UNAPPROVED_PLAN="$TMP/unapproved-plan.json"
PROOF="$TMP/proof"
NETWORK_CREATED=0
DB_CREATED=0

cleanup() {
    if [[ "$DB_CREATED" == 1 ]]; then docker rm -f "$DB_CONTAINER" >/dev/null 2>&1 || true; fi
    if [[ "$NETWORK_CREATED" == 1 ]]; then docker network rm "$NETWORK" >/dev/null 2>&1 || true; fi
    rm -rf "$TMP"
}
trap cleanup EXIT HUP INT TERM

docker network create "$NETWORK" >/dev/null
NETWORK_CREATED=1
if [[ "$DB_IMAGE" == mysql:* ]]; then
    ROOT_PASSWORD_ENV=MYSQL_ROOT_PASSWORD
    DATABASE_ENV=MYSQL_DATABASE
else
    ROOT_PASSWORD_ENV=MARIADB_ROOT_PASSWORD
    DATABASE_ENV=MARIADB_DATABASE
fi
docker create --name "$DB_CONTAINER" --network "$NETWORK" \
    -e "$ROOT_PASSWORD_ENV=$DB_PASSWORD" -e "$DATABASE_ENV=$DB_NAME" \
    "$DB_IMAGE" --event-scheduler=OFF >/dev/null
DB_CREATED=1
docker start "$DB_CONTAINER" >/dev/null
printf '%s\n' '#!/usr/bin/env bash' 'set -euo pipefail' \
    'exec docker exec -e MYSQL_PWD="${MYSQL_PWD:-}" -i "'"$DB_CONTAINER"'" mysql --protocol=TCP --host=127.0.0.1 --port=3306 "$@"' > "$MYSQL_WRAPPER"
chmod 700 "$MYSQL_WRAPPER"
export MYSQL_PWD="$DB_PASSWORD"
for attempt in $(seq 1 45); do
    if "$MYSQL_WRAPPER" -uroot -Nse 'SELECT 1' >/dev/null 2>&1; then break; fi
    if [[ "$attempt" -eq 45 ]]; then
        echo 'database container did not become ready' >&2
        exit 1
    fi
    sleep 1
done
"$MYSQL_WRAPPER" -uroot -e "CREATE DATABASE IF NOT EXISTS $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"

"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$ROOT/tests/async/player_death_restitution_test_schema.sql"
"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$ROOT/migrations/immutable/0020_player_death_restitution.sql"
"$MYSQL_WRAPPER" -uroot "$DB_NAME" <<'SQL'
CREATE TABLE artifact_domain_baseline (
    vnum INT NOT NULL PRIMARY KEY,
    opening_timer_epoch BIGINT NOT NULL,
    opening_bind_owner_pid INT NOT NULL,
    opening_bind_timer_epoch BIGINT NOT NULL,
    opening_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    captured_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
) ENGINE=InnoDB;
SQL

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Isrc \
    "$ROOT/tests/async/player_death_restitution_fixture.cpp" \
    "$ROOT/src/player/player_snapshot_codec.c" -o "$FIXTURE_BIN"
"$FIXTURE_BIN" > "$PAYLOAD_FILE"
mapfile -t PAYLOADS < "$PAYLOAD_FILE"
[[ "${#PAYLOADS[@]}" -eq 2 ]]
python3 "$ROOT/tests/async/player_death_restitution_seed.py" "${PAYLOADS[0]}" "${PAYLOADS[1]}" \
    | "$MYSQL_WRAPPER" -uroot "$DB_NAME"

# Convert the canonical positive fixture into the legacy-Unknown/missing-domain
# state.  Both legacy tables and the bind row remain the captured authority;
# the reconciliation must seed the absent canonical baseline/state additively.
"$MYSQL_WRAPPER" -uroot "$DB_NAME" <<'SQL'
DELETE FROM artifact_domain_baseline WHERE vnum=104;
DELETE FROM artifact_domain_state WHERE vnum=104;
INSERT INTO artifacts(vnum,owned,location,timer,type,lastUpdate,locType)
VALUES (104,'Y',-2,FROM_UNIXTIME(1700000456),2,FROM_UNIXTIME(1700000456),3);
UPDATE artifacts_mortal SET owned='Y',location=-2,locType=3 WHERE vnum=104;
SQL

export ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT=3306 DB_USER=root DB_PASSWD="$DB_PASSWORD" \
    DB_NAME="$DB_NAME" MYSQL_BIN="$MYSQL_WRAPPER"
printf '%s\n' \
    'format=duris-death-restitution-quiescence-v3' \
    "database=$DB_NAME" 'boundary=mysql-advisory-exclusion' \
    'guard=duris.player.death.restitution' \
    'expires_at=2099-01-01T00:00:00Z' > "$PROOF"
chmod 600 "$PROOF"

python3 "$ROOT/scripts/player_death_restitution.py" inspect --pid 42 --death-revision 7 \
    --recipient-pid 42 --artifact "$INSPECT"
python3 "$ROOT/scripts/player_death_restitution.py" plan --inspect "$INSPECT" \
    --artifact "$UNAPPROVED_PLAN"
python3 - "$UNAPPROVED_PLAN" <<'PY'
import json, sys
plan = json.load(open(sys.argv[1], encoding='utf-8'))
assert plan['artifact_reconciliation_count'] == 1, plan
assert plan['artifact_reconciliation_approved'] is False, plan
assert plan['applyable'] is False, plan
item = next(row for row in plan['items'] if row['item_uid'] == 1004)
assert item['eligible'] is True, item
assert item['classification'] == 'recoverable_artifact_reconciled', item
assert item['artifact_reconciliation_mode'] == 'missing_domain', item
assert item['artifact_legacy_unknown'] is True, item
print('legacy-Unknown plan requires explicit artifact reconciliation approval')
PY

python3 "$ROOT/scripts/player_death_restitution.py" plan --inspect "$INSPECT" \
    --approve-artifact-reconciliation --artifact "$PLAN"
python3 - "$PLAN" <<'PY'
import json, sys
plan = json.load(open(sys.argv[1], encoding='utf-8'))
assert plan['artifact_reconciliation_count'] == 1, plan
assert plan['artifact_reconciliation_approved'] is True, plan
assert plan['applyable'] is True, plan
item = next(row for row in plan['items'] if row['item_uid'] == 1004)
assert item['artifact_reconciliation']['legacy_unknown'] is True, item
assert item['artifact_reconciliation']['domain_before'] is None, item
assert item['artifact_reconciliation']['domain_seed']['timer_epoch'] == 1700000456, item
assert item['artifact_reconciliation']['domain_seed']['bind_timer_epoch'] == 654321, item
print('exact payload/custody/legacy authority produced an approved additive reconciliation plan')
PY

python3 "$ROOT/tests/async/player_death_restitution_review_mysql.py" "$PLAN" "$TMP"

# Add a live same-vnum UID after planning.  The plan must become stale before
# any receipt, projection, or canonical state is written.
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e \
    "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,vnum,item_revision,state) VALUES (9004,9004,NULL,1,99,0,104,1,3)"
BEFORE=$($MYSQL_WRAPPER -uroot -N -B "$DB_NAME" -e \
    "SELECT (SELECT COUNT(*) FROM player_death_restitution_receipt),(SELECT COUNT(*) FROM player_death_restitution_delivery),(SELECT COUNT(*) FROM player_items WHERE obj_uid BETWEEN 1000 AND 1004),(SELECT COUNT(*) FROM artifact_domain_state WHERE vnum=104)")
if python3 "$ROOT/scripts/player_death_restitution.py" apply --plan "$PLAN" \
    --offline-proof "$PROOF" --approve --approve-artifact-reconciliation \
    --actor artifact-test --reason competing-instance; then
    echo 'competing artifact instance was incorrectly accepted' >&2
    exit 1
fi
AFTER=$($MYSQL_WRAPPER -uroot -N -B "$DB_NAME" -e \
    "SELECT (SELECT COUNT(*) FROM player_death_restitution_receipt),(SELECT COUNT(*) FROM player_death_restitution_delivery),(SELECT COUNT(*) FROM player_items WHERE obj_uid BETWEEN 1000 AND 1004),(SELECT COUNT(*) FROM artifact_domain_state WHERE vnum=104)")
[[ "$BEFORE" == "$AFTER" ]]
[[ "$BEFORE" == $'0\t0\t0\t0' ]]
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'DELETE FROM item_current_owner WHERE item_uid=9004'
printf 'competing artifact instance refused with no database writes\n'

python3 "$ROOT/scripts/player_death_restitution.py" apply --plan "$PLAN" \
    --offline-proof "$PROOF" --approve --approve-artifact-reconciliation \
    --actor artifact-test --reason legacy-unknown-reconciliation
python3 "$ROOT/scripts/player_death_restitution.py" verify --plan "$PLAN"

CHECK=$($MYSQL_WRAPPER -uroot -N -B "$DB_NAME" -e \
    "SELECT (SELECT COUNT(*) FROM player_death_restitution_delivery),(SELECT COUNT(*) FROM artifact_domain_state d JOIN player_death_restitution_item r ON r.item_uid=1004 WHERE d.vnum=104 AND d.owned=1 AND d.loc_type=3 AND d.location=42 AND d.timer_epoch=r.artifact_delivered_timer_epoch AND r.artifact_usable_lifetime_seconds=456 AND r.artifact_source_timer_epoch=1700000456 AND r.artifact_loss_epoch=1700000000 AND d.artifact_type=2 AND d.bind_owner_pid=42 AND d.bind_timer_epoch=654321 AND d.item_uid=1004 AND d.item_revision=12 AND d.revision=1),(SELECT COUNT(*) FROM artifact_domain_baseline WHERE vnum=104 AND opening_timer_epoch=1700000456 AND opening_bind_owner_pid=42 AND opening_bind_timer_epoch=654321 AND opening_revision=0),(SELECT COUNT(*) FROM artifacts a JOIN player_death_restitution_item r ON r.item_uid=1004 WHERE a.vnum=104 AND a.owned='Y' AND a.locType=3 AND a.location=42 AND UNIX_TIMESTAMP(a.timer)=r.artifact_delivered_timer_epoch AND a.type=2),(SELECT COUNT(*) FROM artifacts_mortal a JOIN player_death_restitution_item r ON r.item_uid=1004 WHERE a.vnum=104 AND a.owned='Y' AND a.locType=3 AND a.location=42 AND UNIX_TIMESTAMP(a.timer)=r.artifact_delivered_timer_epoch AND a.type=2),(SELECT COUNT(*) FROM player_death_restitution_delivery WHERE item_uid=1004)")
[[ "$CHECK" == $'5\t1\t1\t1\t1\t1' ]]
printf 'legacy-Unknown canonical state, baseline, legacy timer/binding, exact payload, and global UID receipt verified\n'
printf 'player death restitution artifact reconciliation disposable DB test passed\n'
