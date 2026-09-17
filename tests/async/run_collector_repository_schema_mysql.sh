#!/usr/bin/env bash
# Runs only against a disposable database container and never reads .env.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
NAME="duris-collector-repository-$$-$RANDOM"
PASSWORD="collector-repository-$$-$RANDOM"
IMAGE="${COLLECTOR_REPOSITORY_DB_IMAGE:-mariadb:10.11}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then
	PASSWORD_ENV=MARIADB_ROOT_PASSWORD
else
	PASSWORD_ENV=MYSQL_ROOT_PASSWORD
fi
docker run -d --name "$NAME" -p 127.0.0.1::3306 \
	-e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
export ENVIRONMENT=test DB_HOST="${COLLECTOR_REPOSITORY_DB_HOST:-127.0.0.1}" \
	DB_PORT="${mapping##*:}"
export DB_USER=root DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
export DB_NAME=collector_repository_test
export COLLECTOR_TEST_DB_NAME="$DB_NAME"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then
	MYSQL_SSL=(--ssl-mode=PREFERRED)
else
	MYSQL_SSL=(--skip-ssl)
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp -h "$DB_HOST" -P "$DB_PORT" \
	-u "$DB_USER" -N -B)
ready=0
for _ in $(seq 1 90); do
	if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then
		ready=1
		break
	fi
	sleep 1
done
[[ "$ready" == 1 ]] || {
	echo "FAILED: $IMAGE did not accept connections on $DB_HOST:$DB_PORT" >&2
	exit 1
}
"${MYSQL[@]}" -e \
	"CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/bootstrap_multithread_safe.sql"

mkdir -p "$ROOT/bin/tests"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
	"${MYSQL_CFLAGS[@]}" tests/async/collector_repository_mysql_harness.cpp \
	src/persistence/critical_command.c src/world/epic_command.c \
	src/economy/currency_command.c src/item/item_transfer_command.c \
	src/item/item_transfer_repository.c src/economy/auction_command.c \
	src/economy/auction_repository.c src/combat/combat_outcome_command.c \
	src/combat/combat_outcome_repository.c src/guild/artifact_guild_command.c \
	src/guild/artifact_guild_repository.c src/economy/boon_reward_command.c \
	src/economy/boon_reward_repository.c src/world/zone_touch_command.c \
	src/world/zone_touch_repository.c src/account/session_audit_command.c \
	src/account/session_audit_repository.c src/economy/coin_transfer_command.c \
	src/player/player_snapshot_codec.c src/economy/collector_command.c \
	src/economy/collector_codec.c src/economy/collector_policy.c \
	src/economy/collector_repository.c src/persistence/corpse_lifecycle_command.c \
	src/persistence/corpse_lifecycle_repository.c \
	src/persistence/critical_command_repository.c \
	"${MYSQL_LIBS[@]}" -lcrypto \
	-o "$ROOT/bin/tests/collector_repository_mysql_harness"
"$ROOT/bin/tests/collector_repository_mysql_harness"
printf 'collector custody, catalog, currency, replay, and lifecycle transactions (%s): ok\n' \
	"$IMAGE"
