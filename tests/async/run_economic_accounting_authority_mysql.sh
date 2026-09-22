#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# Uses only the explicit disposable schema prepared by the schema wrapper.
[[ "${ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA:-}" == 1 && "${DB_HOST:-}" == 127.0.0.1 && -z "${DB_SOCKET:-}" ]]
[[ "${DB_NAME:-}" =~ ^economic_schema_test_[A-Za-z0-9_]+$ ]]
mkdir -p bin/tests/economic-sql-authority
TEMP="$(mktemp -d bin/tests/economic-sql-authority/run-XXXXXXXX)"
trap 'rm -rf -- "$TEMP"' EXIT HUP INT TERM
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer -fno-pie -no-pie -Isrc "${MYSQL_CFLAGS[@]}" \
    tests/async/economic_accounting_authority_mysql_harness.cpp \
    src/persistence/economic_accounting_repository.c src/economy/economic_accounting_types.c \
    src/persistence/critical_command.c src/item/item_transfer_command.c \
    "${MYSQL_LIBS[@]}" -lcrypto -o "$TEMP/authority"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 "$TEMP/authority"

# The client-free server must not fabricate a SQL authority snapshot.
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -D__NO_MYSQL__ -Isrc/no_mysql -Isrc \
    tests/async/economic_accounting_authority_flatfile_test.cpp \
    src/persistence/economic_accounting_repository.c -o "$TEMP/flatfile"
"$TEMP/flatfile"
