#!/usr/bin/env bash
# Synthetic disposable SQL acceptance; never reads the workspace .env.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
if command -v cygpath >/dev/null 2>&1; then
    ROOT=$(cygpath -m "$ROOT")
    export MSYS_NO_PATHCONV=1
fi
DB_IMAGE=${DURIS_REPAIR_DB_IMAGE:-mariadb:10.11}
TOOLS_IMAGE=${DURIS_TEST_TOOLS_IMAGE:-duris-issue-213-tools:latest}
DB="duris-payload-repair-db-$$"
TOOLS="duris-payload-repair-tools-$$"
cleanup() {
    docker rm -fv "$TOOLS" "$DB" >/dev/null 2>&1 || true
}
trap cleanup EXIT HUP INT TERM
if [[ "$DB_IMAGE" == mysql:* ]]; then
    ROOT_PASSWORD_ENV=MYSQL_ROOT_PASSWORD
    DATABASE_ENV=MYSQL_DATABASE
else
    ROOT_PASSWORD_ENV=MARIADB_ROOT_PASSWORD
    DATABASE_ENV=MARIADB_DATABASE
fi
docker run -d --name "$DB" -e "$ROOT_PASSWORD_ENV=synthetic-repair-only" \
    -e "$DATABASE_ENV=duris_payload_repair_test" "$DB_IMAGE" --event-scheduler=OFF >/dev/null
docker run -d --name "$TOOLS" --network "container:$DB" \
    -e ENVIRONMENT=test -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
    -e DB_PASSWD=synthetic-repair-only -e MYSQL_PWD=synthetic-repair-only \
    -e DB_NAME=duris_payload_repair_test -w /workspace "$TOOLS_IMAGE" sleep infinity >/dev/null
# Explicit copies also work with nested Docker daemons and Windows checkouts.
docker exec "$TOOLS" mkdir -p /workspace/tests/async /workspace/migrations/immutable /workspace/bin/tests
docker cp "$ROOT/src" "$TOOLS:/workspace/src"
docker cp "$ROOT/scripts" "$TOOLS:/workspace/scripts"
for file in player_death_restitution_runtime_check.cpp player_death_restitution_fixture.cpp build_player_death_restitution_runtime.sh player_death_restitution_guard_native.cpp player_item_payload_repair_mysql.py; do
    docker cp "$ROOT/tests/async/$file" "$TOOLS:/workspace/tests/async/$file"
done
docker cp "$ROOT/migrations/bootstrap_multithread_safe.sql" "$TOOLS:/workspace/migrations/"
docker cp "$ROOT/migrations/migration_manifest.json" "$TOOLS:/workspace/migrations/"
docker cp "$ROOT/migrations/immutable/." "$TOOLS:/workspace/migrations/immutable/"
for attempt in $(seq 1 60); do
    if docker exec "$TOOLS" mysql --skip-ssl -h127.0.0.1 -uroot -Nse 'SELECT 1' >/dev/null 2>&1; then break; fi
    if [[ "$attempt" == 60 ]]; then echo 'disposable repair database did not become ready' >&2; exit 1; fi
    sleep 1
done
docker exec "$TOOLS" bash tests/async/build_player_death_restitution_runtime.sh /workspace/bin/tests/repair-runtime
docker exec "$TOOLS" python3 tests/async/player_item_payload_repair_mysql.py
printf 'Exact-UID payload repair acceptance passed on %s\n' "$DB_IMAGE"
