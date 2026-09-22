#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$ROOT"
IMAGE=${DURIS_TEST_DB_IMAGE:-mariadb:11.4}
NAME="duris-pdr-native-$$"
DB_NAME=duris_issue_331_test
PASSWORD="pdr-native-$$-$RANDOM"
cleanup() {
    docker rm -f "$NAME" >/dev/null 2>&1 || true
}
trap cleanup EXIT HUP INT TERM

if [[ "$IMAGE" == mariadb:* ]]; then
    PASSWORD_ENV=MARIADB_ROOT_PASSWORD
else
    PASSWORD_ENV=MYSQL_ROOT_PASSWORD
fi
if [[ "$IMAGE" == mariadb:* ]]; then
    docker run -d --name "$NAME" -p 127.0.0.1::3306 \
        -e "$PASSWORD_ENV=$PASSWORD" -e MARIADB_ROOT_HOST=% \
        "$IMAGE" --event-scheduler=OFF >/dev/null
else
    docker run -d --name "$NAME" -p 127.0.0.1::3306 \
        -e "$PASSWORD_ENV=$PASSWORD" -e MYSQL_ROOT_HOST=% \
        "$IMAGE" --event-scheduler=OFF >/dev/null
fi
mapping="$(docker port "$NAME" 3306/tcp)"
DB_HOST=127.0.0.1
DB_PORT="${mapping##*:}"
export DB_HOST DB_PORT DB_USER=root DB_PASSWD="$PASSWORD" DB_NAME MYSQL_PWD="$PASSWORD"
if [[ "$IMAGE" == mariadb:* ]]; then
    MYSQL=(docker exec -i "$NAME" mariadb --protocol=tcp -h127.0.0.1 -P3306 -uroot -p"$PASSWORD" --batch --skip-column-names)
    MYSQL_ADMIN=(docker exec "$NAME" mariadb-admin --protocol=tcp -h127.0.0.1 -P3306 -uroot -p"$PASSWORD")
else
    MYSQL=(docker exec -i "$NAME" mysql --protocol=tcp -h127.0.0.1 -P3306 -uroot -p"$PASSWORD" --batch --skip-column-names)
    MYSQL_ADMIN=(docker exec "$NAME" mysqladmin --protocol=tcp -h127.0.0.1 -P3306 -uroot -p"$PASSWORD")
fi
ready=0
for _ in $(seq 1 90); do
    if "${MYSQL_ADMIN[@]}" ping >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
[[ "$ready" == 1 ]]
"${MYSQL[@]}" < tests/async/player_death_restitution_test_schema.sql
"${MYSQL[@]}" "$DB_NAME" < migrations/immutable/0020_player_death_restitution.sql

mkdir -p bin/tests
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" \
    tests/async/player_death_restitution_native_mysql_harness.cpp \
    tests/async/critical_command_repository_link_stubs.c \
    src/persistence/critical_command.c \
    src/world/epic_command.c src/economy/currency_command.c \
    src/item/item_transfer_command.c src/item/item_transfer_repository.c \
    src/economy/auction_command.c src/economy/auction_repository.c \
    src/combat/combat_outcome_command.c src/combat/combat_outcome_repository.c \
    src/guild/artifact_guild_command.c src/guild/artifact_guild_repository.c \
    src/economy/boon_reward_command.c src/economy/boon_reward_repository.c \
    src/world/zone_touch_command.c src/world/zone_touch_repository.c \
    src/account/session_audit_command.c src/account/session_audit_repository.c \
    src/economy/coin_transfer_command.c src/player/player_snapshot_codec.c \
    src/economy/collector_command.c src/economy/collector_codec.c \
    src/economy/collector_policy.c src/economy/collector_repository.c \
    src/persistence/corpse_lifecycle_command.c src/persistence/corpse_lifecycle_repository.c \
    src/persistence/player_death_restitution_command.c \
    src/persistence/player_death_restitution_repository.c \
    src/persistence/economic_accounting_repository.c \
    src/persistence/economic_sql_bank_transaction.c \
    src/economy/economic_currency_adapter.c \
    src/economy/economic_accounting_types.c \
    src/economy/economic_accounting_plan.c \
    src/economy/economic_accounting_intent.c \
    src/persistence/critical_command_repository.c \
    "${MYSQL_LIBS[@]}" -lcrypto -o bin/tests/player_death_restitution_native_mysql_harness
docker cp bin/tests/player_death_restitution_native_mysql_harness "$NAME:/tmp/player_death_restitution_native_mysql_harness"
docker exec "$NAME" env DB_HOST=127.0.0.1 DB_PORT=3306 DB_USER=root DB_PASSWD="$PASSWORD" DB_NAME="$DB_NAME" \
    /tmp/player_death_restitution_native_mysql_harness
printf 'native player death restitution disposable MySQL checks passed\n'
