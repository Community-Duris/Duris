#!/usr/bin/env bash
# Disposable SQL lifecycle owner regression; never loads the checkout .env.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
unset DB_SOCKET MYSQL_PWD
if [[ -v ECONOMIC_SQL_LIFECYCLE_MYSQL_IMAGE ]]; then
    echo 'Unsupported ECONOMIC_SQL_LIFECYCLE_MYSQL_IMAGE; use ECONOMIC_SQL_LIFECYCLE_DB_IMAGE' >&2
    exit 2
fi
NAME="duris-sql-lifecycle-$$-$RANDOM"
PASSWORD="$(python3 -c 'import secrets; print(secrets.token_hex(32))')"
CONTAINER_ID=""
DB_NAME="economic_lifecycle_test_$$_$RANDOM"
IMAGE="${ECONOMIC_SQL_LIFECYCLE_DB_IMAGE:-mariadb:10.11}"
if [[ "$IMAGE" == mariadb:* ]]; then
    PASSWORD_ENV=MARIADB_ROOT_PASSWORD
    DB_CLIENT=mariadb
elif [[ "$IMAGE" == mysql:* ]]; then
    PASSWORD_ENV=MYSQL_ROOT_PASSWORD
    DB_CLIENT=mysql
else
    echo "ECONOMIC_SQL_LIFECYCLE_DB_IMAGE must name mariadb:* or mysql:*" >&2
    exit 2
fi
TEMP="$(mktemp -d "${TMPDIR:-/tmp}/economic-sql-lifecycle-XXXXXXXX")"
cleanup() {
    status=$?
    trap - EXIT HUP INT TERM
    if [[ -n "$CONTAINER_ID" ]]; then
        docker rm -f "$CONTAINER_ID" >/dev/null 2>&1 || true
        if removed=$(docker container inspect "$CONTAINER_ID" 2>&1); then
            echo 'Fixture container still exists after cleanup' >&2
            status=1
        elif [[ "$removed" == *'No such container'* || "$removed" == *'No such object'* ]]; then
            printf 'SQL_LIFECYCLE_FIXTURE_REMOVED container=%s\n' "$CONTAINER_ID"
        else
            echo 'Fixture container cleanup could not be verified' >&2
            status=1
        fi
    fi
    rm -rf -- "$TEMP"
    exit "$status"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# Keep generated credentials in process environments, never command arguments.
export "$PASSWORD_ENV=$PASSWORD"
CONTAINER_ID="$(docker run --pull=never -d --rm --name "$NAME" -p 127.0.0.1::3306 \
    -e "$PASSWORD_ENV" "$IMAGE")"
unset "$PASSWORD_ENV"
export MYSQL_PWD="$PASSWORD"
mapping="$(docker port "$CONTAINER_ID" 3306/tcp)"
DB_PORT="${mapping##*:}"
ready=0
for _ in $(seq 1 90); do
    if docker exec -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" --protocol=tcp -h 127.0.0.1 \
        -uroot -N -B -e 'SELECT 1' >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
[[ "$ready" == 1 ]]
server_version=$(docker exec -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" --protocol=tcp \
    -h 127.0.0.1 -uroot -N -B -e 'SELECT VERSION()')
if [[ "$DB_CLIENT" == mariadb ]]; then
    [[ "$server_version" == *MariaDB* ]]
else
    [[ "$server_version" == 8.0.* && "$server_version" != *MariaDB* ]]
fi
printf 'SQL_LIFECYCLE_FIXTURE image=%s version=%s container=%s\n' \
    "$IMAGE" "$server_version" "$CONTAINER_ID"
docker exec -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" --protocol=tcp -h 127.0.0.1 -uroot \
    -e "CREATE DATABASE \`$DB_NAME\` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci" >/dev/null
verify_schema() {
    DB_HOST=127.0.0.1 DB_PORT=3306 DB_NAME="$DB_NAME" DB_USER=root DB_PASSWD="$PASSWORD" \
        docker exec -i -e DB_HOST -e DB_PORT -e DB_NAME -e DB_USER -e DB_PASSWD \
        "$CONTAINER_ID" bash -s < migrations/immutable/0033_economic_sql_lifecycle_owner.sh
}
expect_schema_rejection() {
    if verify_schema >"$TEMP/schema-rejection.log" 2>&1; then
        printf 'Schema verifier accepted deliberate %s drift\n' "$1" >&2
        exit 1
    fi
    rejection=$(<"$TEMP/schema-rejection.log")
    [[ "$rejection" == *'schema metadata fingerprint mismatch'* ]] || {
        printf 'Unexpected schema verifier failure during %s probe\n' "$1" >&2
        exit 1
    }
    printf 'SQL_LIFECYCLE_SCHEMA_REJECTED drift=%s\n' "$1"
}
# Fresh schema, additive upgrade, and exact canonical replay must all verify.
docker exec -i -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    < migrations/bootstrap_multithread_safe.sql
docker exec -i -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    < migrations/immutable/0038_item_equipment_slot.sql
docker exec -i -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    < migrations/immutable/0043_shopkeeper_item_condition.sql
verify_schema
docker exec -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    -e 'DROP TABLE economic_sql_global_activation'
docker exec -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    -e 'DROP TABLE economic_sql_lifecycle_installation'
docker exec -i -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    < migrations/immutable/0033_economic_sql_lifecycle_owner.sql
verify_schema
docker exec -i -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    < migrations/economic_sql_lifecycle_owner.sql
verify_schema
docker exec -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    -e 'CREATE INDEX lifecycle_probe_extra ON economic_sql_lifecycle_installation(wallet_count)'
expect_schema_rejection extra-index
docker exec -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    -e 'DROP INDEX lifecycle_probe_extra ON economic_sql_lifecycle_installation'
verify_schema
docker exec -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    -e 'ALTER TABLE economic_sql_lifecycle_installation MODIFY wallet_count BIGINT UNSIGNED NOT NULL'
expect_schema_rejection wallet-count-type
docker exec -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    -e 'ALTER TABLE economic_sql_lifecycle_installation MODIFY wallet_count INT UNSIGNED NOT NULL'
verify_schema
DB_HOST="${ECONOMIC_SQL_LIFECYCLE_DB_HOST:-}"
docker exec -i -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    < migrations/immutable/0040_economic_sql_global_activation.sql
docker exec -i -e MYSQL_PWD "$CONTAINER_ID" "$DB_CLIENT" -uroot "$DB_NAME" \
    < migrations/immutable/0040_economic_sql_global_activation.sql
if [[ -z "$DB_HOST" ]]; then
    if python3 -c 'import socket; socket.gethostbyname("host.docker.internal")' >/dev/null 2>&1; then
        DB_HOST=host.docker.internal
    else
        DB_HOST=127.0.0.1
    fi
fi
export DB_HOST DB_PORT DB_NAME DB_USER=root DB_PASSWD="$PASSWORD"
export ECONOMIC_SQL_LIFECYCLE_DISPOSABLE_SCHEMA=1
bash migrations/immutable/0040_economic_sql_global_activation.sh
unset DB_SOCKET
# Preserve the historical schema-only probes above. Current native witnesses
# require the normal sealed EAB2 migration chain on this same owned fixture.
(
    [[ "$mapping" =~ ^127\.0\.0\.1:([0-9]+)$ ]] &&
        (( DB_PORT >= 1 && DB_PORT <= 65535 )) || {
        echo 'Migration qualification requires the owned loopback container port' >&2
        exit 1
    }
    export ENVIRONMENT=test DB_HOST=127.0.0.1
    export RUNTIME_COMPATIBILITY_MANIFEST="$ROOT/migrations/runtime_compatibility_manifest.json"
    unset DB_SOCKET
    python3 - <<'DURIS_LIFECYCLE_HEAD_PY'
import json
from pathlib import Path

history = json.loads(Path("migrations/migration_manifest.json").read_text())["migrations"]
if len(history) != 62 or history[-1]["sequence"] != 62 or \
        history[-1]["id"] != "0062_economic_pending_claim_consumption":
    raise RuntimeError("current lifecycle qualification requires sealed canonical schema62")
DURIS_LIFECYCLE_HEAD_PY
    python3 scripts/migration_runner.py adopt --kind fresh_bootstrap
    python3 scripts/migration_runner.py run
    head=$(mysql --no-defaults --protocol=tcp -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" \
        -N -B --raw "$DB_NAME" -e "SELECT COUNT(*),MAX(sequence_number),SUM(sequence_number=62 AND migration_id='0062_economic_pending_claim_consumption') FROM mud_schema_history")
    [[ "$head" == $'62\t62\t1' ]] || {
        echo 'Current lifecycle qualification did not reach the registered schema62 head' >&2
        exit 1
    }
    bash migrations/verify_runtime_compatibility.sh
)
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -O1 -g \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST \
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie \
    "${MYSQL_CFLAGS[@]}" -Isrc \
    tests/async/economic_sql_lifecycle_owner_mysql_harness.cpp \
    src/persistence/economic_sql_accounting_lifecycle_transaction.c \
    src/persistence/economic_sql_lifecycle_guard.c \
    src/persistence/economic_sql_cutover_capability.c \
    src/persistence/critical_command_coordinator.c \
    src/persistence/critical_command_journal.c \
    src/persistence/economic_sql_source_snapshot.c \
    src/economy/economic_sql_source_normalize.c \
    src/persistence/economic_sql_baseline_transaction.c \
    src/economy/shop_trade_recovery_manifest.c \
    src/persistence/economic_sql_auction_retained.c \
    src/persistence/economic_sql_auction_bid_transaction.c \
    src/persistence/economic_sql_pending_claim_source.c \
    src/persistence/economic_sql_auction_claim_endpoint.c \
    src/economy/auction_command.c \
    src/economy/auction_repository.c \
    src/economy/currency_command.c \
    src/economy/auction_accounting.c \
    src/economy/auction_money_claim_accounting.c \
    src/economy/auction_item_claim_accounting.c \
    src/economy/auction_settlement_accounting.c \
    src/persistence/economic_accounting_repository.c \
    src/persistence/economic_sql_auction_settlement_transaction.c \
    src/persistence/economic_sql_auction_item_claim_transaction.c \
    src/persistence/economic_sql_auction_money_claim_transaction.c \
    src/economy/economic_baseline_command.c \
    src/economy/economic_baseline_adapter.c \
    src/economy/economic_baseline_codec.c \
    src/economy/economic_accounting_intent.c \
    src/economy/economic_accounting_plan.c src/economy/economic_source_event.c \
    src/economy/economic_accounting_types.c \
    src/economy/economic_gameplay_authority.c \
    src/persistence/critical_command.c \
    src/item/item_transfer_command.c src/world/quest_mobile_native_reference.c src/item/craft_pouch_mutation.c src/combat/chaos_pouch_ledger.c \
    src/economy/coin_transfer_command.c \
    src/economy/coin_transfer_accounting.c \
    src/economy/item_transfer_accounting.c \
    src/item/economic_accounting_item_reference.c \
    src/player/player_snapshot_codec.c \
    "${MYSQL_LIBS[@]}" -lcrypto -lz -o "$TEMP/lifecycle-owner"
# The composed owner harness below also exercises faulted lease transfers.
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -O1 -g \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie \
    "${MYSQL_CFLAGS[@]}" -Isrc \
    tests/async/economic_sql_owned_cutover_capability.cpp \
    src/persistence/critical_command.c \
    src/persistence/critical_command_journal.c \
    src/persistence/critical_command_coordinator.c \
    src/persistence/economic_sql_lifecycle_guard.c \
    src/persistence/economic_sql_cutover_capability.c \
    "${MYSQL_LIBS[@]}" -lcrypto -lz -Wl,--wrap=close -Wl,--wrap=mysql_commit \
    -Wl,--wrap=mysql_rollback -Wl,--wrap=mysql_real_query -o "$TEMP/owned-cutover"
# Run on the untouched inactive schema, before the lifecycle install probes.
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
    "$TEMP/owned-cutover" "$TEMP/owned-cutover-journals"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
ECONOMIC_SQL_LIFECYCLE_JOURNAL_DIR="$TEMP/activation-journal" \
    "$TEMP/lifecycle-owner"
