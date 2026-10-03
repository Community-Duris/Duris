#!/usr/bin/env bash
# Runs only against a disposable database container and never reads .env.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
NAME="duris-corpse-lifecycle-repository-$$-$RANDOM"
PASSWORD="corpse-lifecycle-repository-$$-$RANDOM"
IMAGE="${CORPSE_LIFECYCLE_REPOSITORY_DB_IMAGE:-mariadb:10.11}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then
	PASSWORD_ENV=MARIADB_ROOT_PASSWORD
else
	PASSWORD_ENV=MYSQL_ROOT_PASSWORD
fi
source "$ROOT/tests/async/_sql_fixture_network.sh"
sql_fixture_network
docker run -d --name "$NAME" "${SQL_FIXTURE_NETWORK[@]}" \
	-e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" "${SQL_FIXTURE_SERVER[@]}" >/dev/null
mapping="$(sql_fixture_mapping "$NAME")"
export ENVIRONMENT=test DB_HOST="${CORPSE_LIFECYCLE_REPOSITORY_DB_HOST:-127.0.0.1}" \
	DB_PORT="${mapping##*:}"
export DB_USER=root DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
export DB_NAME=corpse_lifecycle_repository_test
export CORPSE_LIFECYCLE_TEST_DB_NAME="$DB_NAME"
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
python3 scripts/migration_runner.py adopt --kind fresh_bootstrap
python3 scripts/migration_runner.py run
python3 scripts/migration_runner.py run
bash migrations/verify_runtime_compatibility.sh

mkdir -p "$ROOT/bin/tests"
SQL_DISPATCH_SOURCES_TEXT="$(python3 tests/async/_sql_dispatch_sources.py)"
read -r -a SQL_DISPATCH_SOURCES <<< "$SQL_DISPATCH_SOURCES_TEXT"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -ffunction-sections -fdata-sections -Wl,--gc-sections -Wall -Wextra -Wpedantic -Werror -pthread \
    "${SQL_DISPATCH_SOURCES[@]}" \
	-DCORPSE_LIFECYCLE_REPOSITORY_TRACE_SQL -Isrc \
	"${MYSQL_CFLAGS[@]}" tests/async/corpse_lifecycle_repository_mysql_harness.cpp \
	src/persistence/critical_command.c src/world/epic_command.c \
	src/economy/currency_command.c \
	src/combat/combat_outcome_command.c \
      "${MYSQL_LIBS[@]}" -lcrypto \
	-o "$ROOT/bin/tests/corpse_lifecycle_repository_mysql_harness"
"$ROOT/bin/tests/corpse_lifecycle_repository_mysql_harness"
printf 'corpse lifecycle authority, materialization, collector, currency, artifact, replay, and rollback transactions (%s): ok\n' \
	"$IMAGE"
