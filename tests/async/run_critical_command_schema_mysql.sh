#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# The production outbox query references each table more than once, which
# MySQL forbids for TEMPORARY tables. Use real tables in our own container so
# retry/reconciliation checks run without touching the configured database.
NAME="duris-critical-command-$$-$RANDOM"
PASSWORD="critical-command-$$-$RANDOM"
IMAGE="${CRITICAL_DB_IMAGE:-mysql:8.0}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then
    PASSWORD_ENV=MARIADB_ROOT_PASSWORD
else
    PASSWORD_ENV=MYSQL_ROOT_PASSWORD
fi
docker run --rm -d --name "$NAME" -p 127.0.0.1::3306 \
    -e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
export ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT="${mapping##*:}"
export DB_USER=root DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
export DB_NAME=critical_command_test CRITICAL_TEST_DB_NAME=critical_command_test
export DB_ALLOWED_TARGETS=127.0.0.1/critical_command_test
MYSQL=(mysql --protocol=tcp -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -N -B)
ready=0
for _ in $(seq 1 90); do
    if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]]
"${MYSQL[@]}" -e "CREATE DATABASE $CRITICAL_TEST_DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"

"${MYSQL[@]}" "$CRITICAL_TEST_DB_NAME" < "$ROOT/migrations/critical_command_inbox_outbox.sql"
"${MYSQL[@]}" "$CRITICAL_TEST_DB_NAME" < "$ROOT/migrations/critical_command_inbox_outbox.sql"
DB_NAME="$CRITICAL_TEST_DB_NAME" "$ROOT/migrations/verify_critical_command_schema.sh"

mkdir -p "$ROOT/bin/tests"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/critical_command_mysql_harness.cpp \
	 src/persistence/critical_command.c src/world/epic_command.c src/economy/currency_command.c \
	 src/item/item_transfer_command.c src/item/item_transfer_repository.c src/economy/auction_command.c \
	 src/economy/auction_repository.c src/combat/combat_outcome_command.c src/combat/combat_outcome_repository.c \
	 src/guild/artifact_guild_command.c src/guild/artifact_guild_repository.c \
	 src/economy/boon_reward_command.c src/economy/boon_reward_repository.c \
	 src/world/zone_touch_command.c src/world/zone_touch_repository.c \
	 src/account/session_audit_command.c src/account/session_audit_repository.c \
	 src/economy/coin_transfer_command.c src/player/player_snapshot_codec.c \
    src/persistence/critical_command_repository.c \
    "${MYSQL_LIBS[@]}" -lcrypto \
    -o "$ROOT/bin/tests/critical_command_mysql_harness"
"$ROOT/bin/tests/critical_command_mysql_harness"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/critical_outbox_mysql_harness.cpp \
    src/persistence/critical_outbox.c "${MYSQL_LIBS[@]}" \
    -o "$ROOT/bin/tests/critical_outbox_mysql_harness"
"$ROOT/bin/tests/critical_outbox_mysql_harness"
printf 'critical command and outbox isolated MySQL transactional checks passed\n'
