#!/usr/bin/env bash
# Always creates its own disposable database; never reads the project's .env.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE="${ARTIFACT_MANA_DB_IMAGE:-mariadb:10.11}"
NAME="duris-artifact-mana-test-$$"
PASSWORD="disposable-mana-$$-$RANDOM"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT
if [[ "$IMAGE" == mariadb:* ]]; then SECRET=MARIADB_ROOT_PASSWORD; else SECRET=MYSQL_ROOT_PASSWORD; fi
docker run -d --name "$NAME" -p 127.0.0.1::3306 -e "$SECRET=$PASSWORD" "$IMAGE" >/dev/null
ready=0
for _ in $(seq 1 90); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -uroot -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]]
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -uroot -e "CREATE DATABASE mana_test; CREATE USER 'mana_test'@'%' IDENTIFIED BY '$PASSWORD'; GRANT SELECT,INSERT,UPDATE ON mana_test.* TO 'mana_test'@'%';"
docker cp "$ROOT/migrations/immutable/0014_artifact_mana.sql" "$NAME:/tmp/artifact-mana.sql" >/dev/null
for _ in 1 2; do
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c 'mysql -uroot mana_test < /tmp/artifact-mana.sql'
done
docker cp "$ROOT/migrations/immutable/0014_artifact_mana.sh" "$NAME:/tmp/artifact-mana.sh" >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME=mana_test "$NAME" bash /tmp/artifact-mana.sh
mkdir -p "$ROOT/bin/tests"
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -g -I"$ROOT/src" -I/usr/include/mysql \
    "$ROOT/tests/async/artifact_mana_mysql.cpp" "$ROOT/src/item/artifact_mana_model.c" \
    "$ROOT/src/item/artifact_mana_store.c" "$ROOT/src/flatfile/flatfile_store.c" \
    -lmysqlclient -lcrypto -o "$ROOT/bin/tests/artifact-mana-mysql"
PORT=$(docker port "$NAME" 3306/tcp | sed 's/.*://')
"$ROOT/bin/tests/artifact-mana-mysql" 127.0.0.1 "$PASSWORD" "$PORT"
