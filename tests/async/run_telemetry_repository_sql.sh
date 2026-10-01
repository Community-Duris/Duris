#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE="${TELEMETRY_REPOSITORY_DB_IMAGE:-mariadb:11.4}"
case "$IMAGE" in
    mysql:8.4|mariadb:11.4) ;;
    *) printf 'unsupported telemetry SQL fixture image: %s\n' "$IMAGE" >&2; exit 2 ;;
esac

token=$(tr -d '-' < /proc/sys/kernel/random/uuid)
name="duris-telemetry-${token:0:16}"
password="telemetry-${token}"
engine=${IMAGE%%:*}
database="duris_telemetry_test_${engine}_${token:0:20}"
if [[ "$IMAGE" == mariadb:* ]]; then
    password_name=MARIADB_ROOT_PASSWORD
    host_name=MARIADB_ROOT_HOST
    container_client=mariadb
else
    password_name=MYSQL_ROOT_PASSWORD
    host_name=MYSQL_ROOT_HOST
    container_client=mysql
fi
proxy_pid=
ready_file=
cleanup() {
    if [[ -n "$ready_file" ]]; then rm -f -- "$ready_file"; fi
    if [[ -n "$proxy_pid" ]]; then kill "$proxy_pid" >/dev/null 2>&1 || true; fi
    docker rm -f "$name" >/dev/null 2>&1 || true
}
trap cleanup EXIT

docker run -d --name "$name" \
    -e "$password_name=$password" -e "$host_name=%" \
    -p 127.0.0.1::3306 "$IMAGE" >/dev/null

ready=0
for _ in $(seq 1 120); do
    if docker exec -e MYSQL_PWD="$password" "$name" \
        "$container_client" --protocol=tcp -h127.0.0.1 -uroot -N -B -e 'SELECT 1' >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
if [[ "$ready" != 1 ]]; then
    printf 'telemetry SQL fixture did not become ready: %s\n' "$IMAGE" >&2
    docker logs "$name" >&2 || true
    exit 1
fi

binding=$(docker port "$name" 3306/tcp)
port=${binding##*:}
if [[ ! "$port" =~ ^[0-9]+$ ]]; then
    printf 'could not resolve telemetry SQL fixture port: %s\n' "$binding" >&2
    exit 1
fi

fixture_host=127.0.0.1
if ! MYSQL_PWD="$password" mysql --protocol=tcp -h"$fixture_host" -P"$port" \
    -uroot -N -B -e 'SELECT 1' >/dev/null 2>&1; then
    ready_file=$(mktemp)
    rm -f "$ready_file"
    python3 "$ROOT/tests/async/loopback_tcp_proxy.py" \
        host.docker.internal "$port" "$ready_file" &
    proxy_pid=$!
    for _ in $(seq 1 50); do
        [[ -s "$ready_file" ]] && break
        kill -0 "$proxy_pid" >/dev/null 2>&1 || break
        sleep 0.1
    done
    [[ -s "$ready_file" ]] || {
        printf 'telemetry SQL loopback proxy did not become ready\n' >&2
        exit 1
    }
    read -r port < "$ready_file"
    rm -f "$ready_file"
    ready_file=
fi
if ! MYSQL_PWD="$password" mysql --protocol=tcp -h"$fixture_host" -P"$port" \
    -uroot -N -B -e 'SELECT 1' >/dev/null 2>&1; then
    printf 'telemetry SQL fixture is not reachable through the published port\n' >&2
    exit 1
fi

TELEMETRY_REPOSITORY_DISPOSABLE=1 \
TELEMETRY_REPOSITORY_DB_IMAGE="$IMAGE" \
TELEMETRY_REPOSITORY_HOST="$fixture_host" \
TELEMETRY_REPOSITORY_PORT="$port" \
TELEMETRY_REPOSITORY_USER=root \
TELEMETRY_REPOSITORY_PASSWORD="$password" \
TELEMETRY_REPOSITORY_DATABASE="$database" \
python3 "$ROOT/tests/async/test_telemetry_repository.py" --sql-fixture
