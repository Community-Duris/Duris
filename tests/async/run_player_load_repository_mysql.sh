#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ENV_FILE="${PLAYER_LOAD_ENV_FILE:-$ROOT/.env}"
set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
case "${environment_name,,}" in
dev|development|local|test) ;;
*) echo 'refusing player-load test: environment is not development/local/test' >&2; exit 1 ;;
esac
db_port="${DB_PORT:-3306}"
if ! [[ "$db_port" =~ ^[0-9]+$ ]] || (( 10#$db_port < 1 || 10#$db_port > 65535 )); then
    echo 'refusing player-load test: database port is invalid' >&2
    exit 1
fi
[[ "$DB_HOST" == 127.0.0.1 || "$DB_HOST" == localhost || "$DB_HOST" == ::1 ]] || {
    echo 'refusing player-load test: database host is not loopback' >&2
    exit 1
}
case ",${DB_ALLOWED_TARGETS:-}," in
*,"$DB_HOST/$DB_NAME",*) ;;
*) echo 'refusing player-load test: database target is not allow-listed' >&2; exit 1 ;;
esac
case "${DB_NAME,,}" in
*prod*|duris) echo 'refusing player-load test: database name is production-like' >&2; exit 1 ;;
esac
mkdir -p "$ROOT/bin/tests"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/player_load_repository_mysql_harness.cpp \
    src/player/player_load_repository.c src/player/player_load_topology.c src/persistence/persistence_observability.c \
    "${MYSQL_LIBS[@]}" -o "$ROOT/bin/tests/player_load_repository_mysql_harness"
"$ROOT/bin/tests/player_load_repository_mysql_harness"
