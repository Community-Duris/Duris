#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# Commission an isolated database explicitly. Do not source the server's .env,
# apply migrations, or change any permanent table in the selected database.
: "${DB_HOST:?set the isolated test database host}"
: "${DB_USER:?set the isolated test database user}"
: "${AUCTION_TEST_DB_NAME:?set a commissioned duris_pr717_ database}"
[[ "$AUCTION_TEST_DB_NAME" == duris_pr717_* ]] || {
    echo 'refusing refund test: expected a commissioned duris_pr717_ database' >&2
    exit 1
}
BIN_ROOT="${BIN_ROOT:-$ROOT/bin}"
mkdir -p "$BIN_ROOT/tests"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/auction_remove_refund_mysql_harness.cpp \
    src/economy/auction_repository.c src/economy/auction_command.c \
    src/economy/currency_command.c src/persistence/critical_command.c \
    "${MYSQL_LIBS[@]}" -lcrypto -o "$BIN_ROOT/tests/auction_remove_refund_mysql_harness"
"$BIN_ROOT/tests/auction_remove_refund_mysql_harness"
