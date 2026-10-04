#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
case "${1:-}" in
    '') [[ $# == 0 ]] || exit 2; TEST_SCRIPT=test_telemetry_repository.py ;;
    --incidents) [[ $# == 1 ]] || exit 2; TEST_SCRIPT=test_telemetry_incidents.py ;;
    --observations) [[ $# == 1 ]] || exit 2; TEST_SCRIPT=test_telemetry_observations.py ;;
    --identity) [[ $# == 1 ]] || exit 2; TEST_SCRIPT=test_telemetry_account_identity.py ;;
    --identity-review) [[ $# == 1 ]] || exit 2; TEST_SCRIPT=test_telemetry_identity_history.py ;;
    --identity-publication) [[ $# == 1 ]] || exit 2; TEST_SCRIPT=test_telemetry_identity_publication.py ;;
    --battle-storage) [[ $# == 1 ]] || exit 2; TEST_SCRIPT=test_telemetry_battle_storage.py ;;
    --battle-runtime) [[ $# == 1 ]] || exit 2; TEST_SCRIPT=test_telemetry_battle_runtime_sql.py ;;
    --contribution-storage) [[ $# == 1 ]] || exit 2; TEST_SCRIPT=test_telemetry_battle_contribution_storage.py ;;
    --build-storage) [[ $# == 1 ]] || exit 2; TEST_SCRIPT=test_telemetry_battle_build_storage.py ;;
    *) printf 'usage: run_telemetry_repository_sql.sh [--incidents|--observations|--identity|--identity-review|--identity-publication|--battle-storage|--battle-runtime|--contribution-storage|--build-storage]\n' >&2; exit 2 ;;
esac
IMAGE="${TELEMETRY_REPOSITORY_DB_IMAGE:-mariadb:10.11.14}"
case "${IMAGE%%@*}" in
    mysql:8.0.46|mariadb:10.11.14|mariadb:10.11.19) ;;
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
SQL_FIXTURE_CONTAINER_ID=
cleanup() {
    if [[ -n "$ready_file" ]]; then rm -f -- "$ready_file"; fi
    if [[ -n "$proxy_pid" ]]; then kill "$proxy_pid" >/dev/null 2>&1 || true; fi
    [[ "${SQL_FIXTURE_CONTAINER_ID:-}" =~ ^[0-9a-f]{64}$ ]] && docker rm -f "$SQL_FIXTURE_CONTAINER_ID" >/dev/null 2>&1 || true
}
trap cleanup EXIT

source "$ROOT/tests/async/_sql_fixture_network.sh"
sql_fixture_network
SQL_FIXTURE_CONTAINER_ID=$(docker run -d --name "$name" \
    -e "$password_name=$password" -e "$host_name=%" \
    "${SQL_FIXTURE_NETWORK[@]}" "$IMAGE" "${SQL_FIXTURE_SERVER[@]}")

ready=0
for _ in $(seq 1 120); do
    if docker exec -e MYSQL_PWD="$password" "$name" \
        "$container_client" --protocol=tcp -h127.0.0.1 -P"${SQL_FIXTURE_PRIVATE_PORT:-3306}" -uroot -N -B -e 'SELECT 1' >/dev/null 2>&1; then
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

binding=$(sql_fixture_mapping "$name")
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
python3 "$ROOT/tests/async/$TEST_SCRIPT" --sql-fixture
