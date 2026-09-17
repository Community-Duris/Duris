#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
IMAGE=${DURIS_TEST_TOOLS_IMAGE:-duris-issue-213-tools:latest}
DB_IMAGE=${DURIS_TEST_DB_IMAGE:-mariadb:11.4}
DB_CONTAINER="duris-issue-331-db-$$"
NETWORK="duris-issue-331-net-$$"
DB_NAME=duris_issue_331_test
DB_PASSWORD=issue331-test-only
TMP=$(mktemp -d)
MYSQL_WRAPPER="$TMP/mysql"
FIXTURE_BIN="$TMP/fixture"
PAYLOAD_FILE="$TMP/payloads"
INSPECT="$TMP/inspect.json"
PLAN="$TMP/plan.json"
PROOF="$TMP/proof"
BAD_PROOF="$TMP/bad-proof"
NETWORK_CREATED=0
DB_CREATED=0
TOOLS_CONTAINER="duris-issue-331-tools-$$"
TOOLS_CREATED=0

cleanup() {
    if [[ "$TOOLS_CREATED" == 1 ]]; then docker rm -f "$TOOLS_CONTAINER" >/dev/null 2>&1 || true; fi
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
    if "$MYSQL_WRAPPER" -uroot -Nse 'SELECT 1' >/dev/null 2>&1; then
        break
    fi
    if [ "$attempt" -eq 45 ]; then
        echo 'database container did not become ready' >&2
        exit 1
    fi
    sleep 1
done

"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$ROOT/tests/async/player_death_restitution_test_schema.sql"
"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$ROOT/migrations/immutable/0020_player_death_restitution.sql"
DB_HOST=127.0.0.1 DB_PORT=3306 DB_USER=root DB_PASSWD="$DB_PASSWORD" DB_NAME="$DB_NAME" \
    MYSQL_BIN="$MYSQL_WRAPPER" "$ROOT/migrations/immutable/0020_player_death_restitution.sh"

verify_schema() {
    DB_HOST=127.0.0.1 DB_PORT=3306 DB_USER=root DB_PASSWD="$DB_PASSWORD" DB_NAME="$DB_NAME" \
        MYSQL_BIN="$MYSQL_WRAPPER" bash "$ROOT/migrations/immutable/0020_player_death_restitution.sh"
}
reject_drift() {
    if verify_schema > "$TMP/drift.log" 2>&1; then
        printf 'schema verifier accepted drift: %s\n' "$1" >&2
        exit 1
    fi
    printf 'schema drift refused: %s\n' "$1"
}
# An existing receipt is preserved by replay; only this task's marker is removed.
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e "INSERT INTO player_death_restitution_receipt
 (restitution_id,source_pid,death_revision,recipient_pid,death_operation_id,evidence_digest,plan_digest,actor,reason)
 VALUES(UNHEX(REPEAT('ab',16)),42,1,42,UNHEX(REPEAT('cd',16)),UNHEX(REPEAT('ef',32)),UNHEX(REPEAT('01',32)),'schema-test','replay-preservation');"
"$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$ROOT/migrations/immutable/0020_player_death_restitution.sql"
verify_schema
[[ $("$MYSQL_WRAPPER" -uroot -N -B "$DB_NAME" -e "SELECT COUNT(*) FROM player_death_restitution_receipt WHERE restitution_id=UNHEX(REPEAT('ab',16)) AND actor='schema-test' AND reason='replay-preservation'") == 1 ]]
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e "DELETE FROM player_death_restitution_receipt WHERE restitution_id=UNHEX(REPEAT('ab',16));"
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'ALTER TABLE player_death_restitution_runtime MODIFY state_digest BINARY(16) NOT NULL'
reject_drift 'wrong digest width'
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'ALTER TABLE player_death_restitution_runtime MODIFY state_digest BINARY(32) NOT NULL'
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'ALTER TABLE player_death_restitution_runtime DROP INDEX idx_restitution_runtime_recipient, ADD INDEX idx_restitution_runtime_recipient (recipient_pid)'
reject_drift 'same-named wrong index'
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'ALTER TABLE player_death_restitution_runtime DROP INDEX idx_restitution_runtime_recipient, ADD INDEX idx_restitution_runtime_recipient (recipient_pid,item_uid)'
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'ALTER TABLE player_death_restitution_runtime DROP FOREIGN KEY restitution_runtime_delivery_fk'
reject_drift 'missing delivery foreign key'
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'ALTER TABLE player_death_restitution_runtime ADD CONSTRAINT restitution_runtime_delivery_fk FOREIGN KEY (item_uid) REFERENCES player_death_restitution_delivery(item_uid) ON UPDATE RESTRICT ON DELETE RESTRICT'
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'ALTER TABLE player_death_restitution_runtime ADD unexpected INT NULL'
reject_drift 'extra column'
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'ALTER TABLE player_death_restitution_runtime DROP COLUMN unexpected'
verify_schema
if [[ "${DURIS_RESTITUTION_SCHEMA_ONLY:-0}" == 1 ]]; then
    printf 'player death restitution schema replay and drift tests passed\n'
    exit 0
fi

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Isrc \
    "$ROOT/tests/async/player_death_restitution_fixture.cpp" \
    "$ROOT/src/player/player_snapshot_codec.c" -o "$FIXTURE_BIN"
"$FIXTURE_BIN" > "$PAYLOAD_FILE"
mapfile -t PAYLOADS < "$PAYLOAD_FILE"
[ "${#PAYLOADS[@]}" -eq 2 ]
python3 "$ROOT/tests/async/player_death_restitution_seed.py" "${PAYLOADS[0]}" "${PAYLOADS[1]}" \
    | "$MYSQL_WRAPPER" -uroot "$DB_NAME"

export ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT=3306 DB_USER=root DB_PASSWD="$DB_PASSWORD" \
    DB_NAME="$DB_NAME" MYSQL_BIN="$MYSQL_WRAPPER"
printf '%s\n' \
    'format=duris-death-restitution-quiescence-v3' \
    "database=$DB_NAME" 'boundary=mysql-advisory-exclusion' \
    'guard=duris.player.death.restitution' \
    'expires_at=2099-01-01T00:00:00Z' > "$PROOF"
chmod 600 "$PROOF"
printf '%s\n' \
    'format=duris-death-restitution-quiescence-v1' \
    "database=$DB_NAME" 'server_state=running' 'writers=drained' \
    'ownership_work=drained' 'currency_work=drained' \
    'expires_at=2099-01-01T00:00:00Z' > "$BAD_PROOF"
chmod 600 "$BAD_PROOF"

# Keep the invalid legacy location as a refusal test, not the positive fixture.
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'UPDATE artifacts_mortal SET location=999 WHERE vnum=104'
python3 "$ROOT/scripts/player_death_restitution.py" inspect --pid 42 --death-revision 7 \
    --recipient-pid 42 --artifact "$TMP/invalid-artifact.inspect.json"
python3 "$ROOT/scripts/player_death_restitution.py" plan --inspect "$TMP/invalid-artifact.inspect.json" \
    --artifact "$TMP/invalid-artifact.plan.json"
python3 - "$TMP/invalid-artifact.plan.json" <<'PY'
import json, sys
plan = json.load(open(sys.argv[1]))
assert plan['eligible_count'] == 4, plan['eligible_count']
item = next(item for item in plan['items'] if item['item_uid'] == 1004)
assert not item['eligible'] and item['classification'] == 'artifact_legacy_conflict', item['classification']
print('invalid legacy artifact location refused without inventing authority')
PY
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'UPDATE artifacts_mortal SET location=42 WHERE vnum=104'

python3 "$ROOT/scripts/player_death_restitution.py" inspect --pid 42 --death-revision 7 \
    --recipient-pid 42 --artifact "$INSPECT"
python3 "$ROOT/scripts/player_death_restitution.py" plan --inspect "$INSPECT" --artifact "$PLAN"
python3 - "$PLAN" <<'PY'
import collections, json, sys
plan = json.load(open(sys.argv[1]))
assert plan["applyable"] is True
assert plan["eligible_count"] == 5
classes = collections.Counter(row["classification"] for row in plan["items"])
assert classes["recoverable_topology_reconciled"] == 1
assert classes["currency_refused"] == 1
assert classes["missing_payload"] == 1
assert any(row["kind"] == "unique" and row["eligible"] for row in plan["items"])
assert any(row["kind"] == "artifact" and row["eligible"] for row in plan["items"])
print("fixture plan classifications verified")
PY

run_runtime_checker() {
    docker create --name "$TOOLS_CONTAINER" --network "container:$DB_CONTAINER" \
        -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
        -e DB_PASSWD="$DB_PASSWORD" -e DB_NAME="$DB_NAME" \
        -w /workspace "$IMAGE" sleep infinity >/dev/null
    TOOLS_CREATED=1
    docker start "$TOOLS_CONTAINER" >/dev/null
    docker exec "$TOOLS_CONTAINER" mkdir -p /workspace/tests/async
    # Explicit copies work with nested Docker; no assumed host bind mount.
    docker cp "$ROOT/src" "$TOOLS_CONTAINER:/workspace/src"
    docker cp "$ROOT/tests/async/player_death_restitution_runtime_check.cpp" "$TOOLS_CONTAINER:/workspace/tests/async/"
    docker cp "$ROOT/tests/async/player_death_restitution_guard_native.cpp" "$TOOLS_CONTAINER:/workspace/tests/async/"
    docker cp "$ROOT/tests/async/build_player_death_restitution_runtime.sh" "$TOOLS_CONTAINER:/workspace/tests/async/"
    docker exec "$TOOLS_CONTAINER" bash tests/async/build_player_death_restitution_runtime.sh /tmp/restitution-runtime-check
    docker exec "$TOOLS_CONTAINER" /tmp/restitution-runtime-check
    docker exec "$TOOLS_CONTAINER" /tmp/restitution-runtime-check-guard
}

if [[ "${DURIS_RESTITUTION_RUNTIME_ONLY:-0}" == 1 ]]; then
    # Component-only proof: seed from generated SQL without exercising apply's
    # lifecycle boundary. This must NEVER be reported as end-to-end CLI success.
    python3 - "$ROOT" "$PLAN" "$TMP/runtime-seed.sql" <<'PY'
import importlib.util, pathlib, sys
spec = importlib.util.spec_from_file_location('restitution', pathlib.Path(sys.argv[1]) / 'scripts/player_death_restitution.py')
cli = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cli)
plan = cli.load_plan(pathlib.Path(sys.argv[2]))
pathlib.Path(sys.argv[3]).write_text(cli.build_apply_sql(plan, 'runtime-component-test', 'synthetic-runtime-fixture') + "\nSELECT CONCAT('COMPONENT_DATABASE_QUIESCENT|',IFNULL(@database_quiescent,'NULL'));\nSELECT USER,COMMAND FROM information_schema.processlist WHERE ID<>CONNECTION_ID();\n")
PY
    # An enabled scheduler is another database writer, even with no game up.
    # Exercise refusal before restoring the explicitly offline fixture state.
    "$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'SET GLOBAL event_scheduler=ON'
    python3 - "$MYSQL_WRAPPER" "$DB_NAME" <<'PY'
import subprocess, sys, time
for attempt in range(40):
    value = subprocess.check_output([sys.argv[1], '-uroot', '-N', '-B', sys.argv[2], '-e', "SELECT COUNT(*) FROM information_schema.processlist WHERE USER='event_scheduler' AND COMMAND='Daemon'"], text=True).strip()
    if value == '1':
        break
    time.sleep(0.05)
else:
    raise AssertionError('fixture event scheduler never became observable')
PY
    "$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$TMP/runtime-seed.sql" > "$TMP/scheduler-refusal.result"
    python3 - "$TMP/scheduler-refusal.result" <<'PY'
import pathlib, sys
lines = pathlib.Path(sys.argv[1]).read_text().splitlines()
assert 'DURIS_RESULT|0|0|1|5' in lines, lines
assert 'COMPONENT_DATABASE_QUIESCENT|0' in lines, lines
print('scheduled database writer caused recovery refusal')
PY
    [[ $("$MYSQL_WRAPPER" -uroot -N -B "$DB_NAME" -e 'SELECT COUNT(*) FROM player_death_restitution_receipt') == 0 ]]
    [[ $("$MYSQL_WRAPPER" -uroot -N -B "$DB_NAME" -e 'SELECT COUNT(*) FROM player_death_restitution_delivery') == 0 ]]
    [[ $("$MYSQL_WRAPPER" -uroot -N -B "$DB_NAME" -e 'SELECT COUNT(*) FROM player_items WHERE obj_uid BETWEEN 1000 AND 1004') == 0 ]]
    "$MYSQL_WRAPPER" -uroot "$DB_NAME" -e 'SET GLOBAL event_scheduler=OFF'
    "$MYSQL_WRAPPER" -uroot "$DB_NAME" < "$TMP/runtime-seed.sql" > "$TMP/runtime-seed.result"
    python3 - "$TMP/runtime-seed.result" <<'PY'
import pathlib, sys
lines = pathlib.Path(sys.argv[1]).read_text().splitlines()
assert 'DURIS_RESULT|1|0|1|5' in lines, lines
print('generated SQL fixture committed; CLI lifecycle boundary NOT exercised')
PY
    python3 "$ROOT/scripts/player_death_restitution.py" verify --plan "$PLAN"
    run_runtime_checker
    printf 'player restitution runtime COMPONENT tests passed; guarded CLI apply remains unverified\n'
    exit 0
fi

# A revision fence must reject a stale plan without writing any receipt.
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e \
    'UPDATE item_current_owner SET item_revision=12 WHERE item_uid=1000'
if python3 "$ROOT/scripts/player_death_restitution.py" apply --plan "$PLAN" \
    --offline-proof "$PROOF" --approve --actor test-harness --reason stale-fence; then
    echo 'stale plan was incorrectly accepted' >&2
    exit 1
fi
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e \
    'UPDATE item_current_owner SET item_revision=11 WHERE item_uid=1000'

python3 "$ROOT/scripts/player_death_restitution.py" apply --plan "$PLAN" --offline-proof "$PROOF" \
    --approve --actor test-harness --reason issue331-fixture
python3 "$ROOT/scripts/player_death_restitution.py" verify --plan "$PLAN"
python3 "$ROOT/scripts/player_death_restitution.py" verify --plan "$PLAN" --mark-verified \
    --offline-proof "$PROOF"
# A second application is a no-op behind the global item-UID receipt guard.
python3 "$ROOT/scripts/player_death_restitution.py" apply --plan "$PLAN" --offline-proof "$PROOF" \
    --approve --actor test-harness --reason issue331-fixture | grep -F 'already applied' >/dev/null

if python3 "$ROOT/scripts/player_death_restitution.py" apply --plan "$PLAN" --offline-proof "$BAD_PROOF" \
    --approve --actor test-harness --reason bad-proof; then
    echo 'non-quiescent proof was incorrectly accepted' >&2
    exit 1
fi

CHECK="$TMP/check"
"$MYSQL_WRAPPER" -uroot -N -B "$DB_NAME" -e \
    "SELECT (SELECT COUNT(*) FROM player_items WHERE pid=42 AND obj_uid IN (1000,1001,1002,1003,1004)), (SELECT COUNT(*) FROM player_items WHERE pid=42 AND obj_uid=3000), (SELECT COUNT(*) FROM player_items WHERE pid=42 AND obj_uid=7000), (SELECT COUNT(*) FROM player_death_restitution_delivery), (SELECT COUNT(*) FROM player_death_restitution_runtime), (SELECT status FROM player_death_restitution_receipt LIMIT 1), (SELECT revision FROM item_owner_revision WHERE owner_type=1 AND owner_id=42 AND owner_context_id=0), (SELECT state FROM item_current_owner WHERE item_uid=7000), (SELECT owner_id FROM item_current_owner WHERE item_uid=1002), (SELECT parent_item_uid FROM item_current_owner WHERE item_uid=1002), (SELECT loc_type FROM artifact_domain_state WHERE vnum=104), (SELECT location FROM artifact_domain_state WHERE vnum=104), (SELECT timer_epoch FROM artifact_domain_state WHERE vnum=104), (SELECT artifact_delivered_timer_epoch FROM player_death_restitution_item WHERE item_uid=1004), (SELECT artifact_usable_lifetime_seconds FROM player_death_restitution_item WHERE item_uid=1004), (SELECT artifact_source_timer_epoch FROM player_death_restitution_item WHERE item_uid=1004), (SELECT artifact_loss_epoch FROM player_death_restitution_item WHERE item_uid=1004), (SELECT bind_timer_epoch FROM artifact_domain_state WHERE vnum=104), (SELECT location FROM artifacts_mortal WHERE vnum=104), (SELECT UNIX_TIMESTAMP(timer) FROM artifacts_mortal WHERE vnum=104)" > "$CHECK"
python3 - "$CHECK" <<'PY'
import sys
values = [int(value) for value in open(sys.argv[1]).read().split()]
assert values[:12] == [5, 1, 0, 5, 5, 3, 7, 3, 42, 1001, 3, 42], values
assert values[12] == values[13] == values[19], values
assert values[14:17] == [456, 1700000456, 1700000000], values
assert values[17:19] == [654321, 42], values
print("database receipt, topology, currency refusal, newer inventory, and artifact lifetime verified")
PY

# Runtime-facing exact payloads are decoded through the supported codec in the
# same disposable database, including generated fields and dynamic affects.
run_runtime_checker

# A second payload-bearing death reference to the same UID is ambiguous even
# after the first receipt; it cannot mint a second copy.
"$MYSQL_WRAPPER" -uroot "$DB_NAME" -e \
    "UPDATE player_death_disposition SET payload=UNHEX('${PAYLOADS[0]}') WHERE pid=42 AND save_revision=8"
CROSS_INSPECT="$TMP/cross-inspect.json"
CROSS_PLAN="$TMP/cross-plan.json"
python3 "$ROOT/scripts/player_death_restitution.py" inspect --pid 42 --death-revision 7 \
    --recipient-pid 42 --artifact "$CROSS_INSPECT"
python3 "$ROOT/scripts/player_death_restitution.py" plan --inspect "$CROSS_INSPECT" --artifact "$CROSS_PLAN"
python3 - "$CROSS_PLAN" <<'PY'
import collections, json, sys
plan = json.load(open(sys.argv[1]))
assert plan["applyable"] is False
assert collections.Counter(row["classification"] for row in plan["items"])["cross_death_payload_conflict"] == 5
assert plan["eligible_count"] == 0
print("cross-death duplicate prevention verified")
PY

printf 'player death restitution disposable MySQL test passed\n'
