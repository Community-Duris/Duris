#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
set -a
# shellcheck disable=SC1091
source "$ROOT/.env"
set +a
: "${DB_HOST:?}"
: "${DB_USER:?}"
: "${DB_PASSWD:?}"
: "${DB_PORT:=3306}"

environment_value="${ENVIRONMENT:-${APP_ENV:-${DURIS_ENV:-${NODE_ENV:-}}}}"
[[ "${environment_value,,}" =~ (dev|local|test) ]] || {
    echo 'refusing critical schema test: environment is not explicitly development/local/test' >&2
    exit 1
}
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || {
    echo 'refusing critical schema test: configured database name is not development/local/test' >&2
    exit 1
}
CRITICAL_TEST_DB_NAME="$DB_NAME"
export CRITICAL_TEST_DB_NAME
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -N -B)

"${MYSQL[@]}" "$CRITICAL_TEST_DB_NAME" < "$ROOT/migrations/critical_command_inbox_outbox.sql"
"${MYSQL[@]}" "$CRITICAL_TEST_DB_NAME" < "$ROOT/migrations/critical_command_inbox_outbox.sql"
DB_NAME="$CRITICAL_TEST_DB_NAME" "$ROOT/migrations/verify_critical_command_schema.sh"

mkdir -p "$ROOT/bin/tests"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/critical_command_mysql_harness.cpp \
	 src/critical_command.c src/epic_command.c src/currency_command.c \
	 src/item_transfer_command.c src/item_transfer_repository.c src/auction_command.c \
	 src/auction_repository.c src/combat_outcome_command.c src/combat_outcome_repository.c \
	 src/artifact_guild_command.c src/artifact_guild_repository.c \
	 src/boon_reward_command.c src/boon_reward_repository.c \
	 src/zone_touch_command.c src/zone_touch_repository.c \
	 src/session_audit_command.c src/session_audit_repository.c \
	 src/critical_command_repository.c \
    "${MYSQL_LIBS[@]}" -lcrypto \
    -o "$ROOT/bin/tests/critical_command_mysql_harness"
"$ROOT/bin/tests/critical_command_mysql_harness"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/critical_outbox_mysql_harness.cpp \
    src/critical_outbox.c "${MYSQL_LIBS[@]}" \
    -o "$ROOT/bin/tests/critical_outbox_mysql_harness"
"$ROOT/bin/tests/critical_outbox_mysql_harness"
printf 'critical command and outbox isolated MySQL transactional checks passed\n'
