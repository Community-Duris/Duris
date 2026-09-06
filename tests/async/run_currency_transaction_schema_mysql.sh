#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# The coin failure matrix uses only synthetic data in its own disposable server.
# Never source the checkout's .env: it may point at the live game.
NAME="duris-currency-$$-$RANDOM"
PASSWORD="currency-$$-$RANDOM"
IMAGE="${CURRENCY_DB_IMAGE:-mariadb:10.11}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then PASSWORD_ENV=MARIADB_ROOT_PASSWORD; else PASSWORD_ENV=MYSQL_ROOT_PASSWORD; fi
docker run --rm -d --name "$NAME" -p 127.0.0.1::3306 -e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
export ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT="${mapping##*:}"
export DB_USER=root DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
export DB_NAME=currency_coin_test CURRENCY_TEST_DB_NAME=currency_coin_test
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -N -B)
ready=0
for _ in $(seq 1 90); do
    if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]]
"${MYSQL[@]}" -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/bootstrap_multithread_safe.sql"
# Use the current schema, including portable timestamp defaults required when
# the loader harness creates its connection-local fixture tables on MySQL 8.
for migration in "$ROOT"/migrations/immutable/*.sql; do
    "${MYSQL[@]}" "$DB_NAME" < "$migration"
done
# Exercise the upgrade from a pre-coin schema, then verify rerunning it is safe.
"${MYSQL[@]}" "$DB_NAME" -e 'ALTER TABLE item_current_owner DROP COLUMN coin_payload'
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0010_coin_custody_payload.sql"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0010_coin_custody_payload.sql"
"$ROOT/migrations/immutable/0010_coin_custody_payload.sh"
for definition in 'BLOB NOT NULL' 'TEXT NULL' 'MEDIUMBLOB NULL AFTER item_uid'; do
    "${MYSQL[@]}" "$DB_NAME" -e "ALTER TABLE item_current_owner MODIFY coin_payload $definition"
    if "${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0010_coin_custody_payload.sql" >/dev/null 2>&1; then
        echo "Damaged coin column passed migration: $definition" >&2
        exit 1
    fi
    if "$ROOT/migrations/immutable/0010_coin_custody_payload.sh" >/dev/null 2>&1; then
        echo "Damaged coin column passed verification: $definition" >&2
        exit 1
    fi
    "${MYSQL[@]}" "$DB_NAME" -e 'ALTER TABLE item_current_owner MODIFY coin_payload MEDIUMBLOB NULL AFTER state'
done
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0010_coin_custody_payload.sql"
"$ROOT/migrations/verify_item_ownership_schema.sh"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/currency_ledger.sql"
DB_NAME="$DB_NAME" "$ROOT/migrations/verify_currency_ledger_schema.sh"
export CURRENCY_TEST_DB_NAME="$DB_NAME"
mkdir -p "$ROOT/bin/tests"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/currency_transaction_mysql_harness.cpp \
    src/persistence/critical_command.c src/economy/currency_command.c src/world/epic_command.c \
    src/item/item_transfer_command.c src/item/item_transfer_repository.c src/economy/auction_command.c \
    src/economy/auction_repository.c src/combat/combat_outcome_command.c src/combat/combat_outcome_repository.c \
    src/guild/artifact_guild_command.c src/guild/artifact_guild_repository.c \
    src/economy/boon_reward_command.c src/economy/boon_reward_repository.c \
    src/world/zone_touch_command.c src/world/zone_touch_repository.c \
    src/account/session_audit_command.c src/account/session_audit_repository.c \
    src/economy/coin_transfer_command.c src/player/player_snapshot_codec.c \
    src/player/player_load_repository.c src/player/player_load_topology.c src/persistence/persistence_observability.c \
    src/persistence/critical_command_repository.c "${MYSQL_LIBS[@]}" -lcrypto \
    -o "$ROOT/bin/tests/currency_transaction_mysql_harness"
"$ROOT/bin/tests/currency_transaction_mysql_harness"
# The new coin query also runs on ordinary inventories; retain the full existing
# loader regression matrix alongside the crash-window coin cases above.
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/player_load_repository_mysql_harness.cpp \
    src/player/player_load_repository.c src/player/player_load_topology.c \
    src/player/player_snapshot_codec.c src/persistence/persistence_observability.c \
    "${MYSQL_LIBS[@]}" -o "$ROOT/bin/tests/player_load_repository_mysql_harness"
PLAYER_LOAD_DISPOSABLE_SCHEMA=1 GAME_ACCOUNT_NAME=coin_matrix_account GAME_ACCOUNT_CHARACTER_NAME=CoinMatrix \
    "$ROOT/bin/tests/player_load_repository_mysql_harness"
