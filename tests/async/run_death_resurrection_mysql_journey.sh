#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# Disposable loopback-published database; never source the checkout's .env.
NAME="duris-death-resurrection-$$-$RANDOM"
PASSWORD="death-resurrection-$$-$RANDOM"
IMAGE="${DEATH_RESURRECTION_DB_IMAGE:-mariadb:10.11}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

docker run -d --name "$NAME" -p 127.0.0.1::3306 \
    -e MARIADB_ROOT_PASSWORD="$PASSWORD" -e MARIADB_ROOT_HOST='%' "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
export TEST_DB_HOST=127.0.0.1 TEST_DB_PORT="${mapping##*:}"
export TEST_DB_USER=root TEST_DB_PASSWORD="$PASSWORD" TEST_DB_DISPOSABLE=1
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then
    MYSQL_SSL=(--ssl-mode=PREFERRED)
else
    MYSQL_SSL=(--skip-ssl)
fi
printf 'disposable MariaDB endpoint: %s:%s\n' "$TEST_DB_HOST" "$TEST_DB_PORT"

ready=0
for _ in $(seq 1 90); do
    if mysql "${MYSQL_SSL[@]}" --protocol=tcp --connect-timeout=2 -h "$TEST_DB_HOST" \
        -P "$TEST_DB_PORT" -u "$TEST_DB_USER" -p"$TEST_DB_PASSWORD" \
        -e 'SELECT 1' >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
if [[ "$ready" != 1 ]]; then
    echo 'disposable MariaDB did not accept bounded Docker-host SQL connections' >&2
    docker logs "$NAME" >&2 || true
    exit 1
fi
printf 'disposable MariaDB ready\n'
python3 tests/async/test_death_resurrection_mysql_journey.py
