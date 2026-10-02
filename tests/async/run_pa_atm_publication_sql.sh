#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
python3 "$ROOT/tests/async/pa_accounting_batch_artifact.py" --component S06-ATM

RESOURCE_ROOT="${DURIS_ACCOUNTING_RESOURCE_ROOT:-}"
HEAVY_LOCK="${DURIS_ACCOUNTING_DOCKER_HEAVY_LOCK:-}"
if [[ -z "$RESOURCE_ROOT" || "$RESOURCE_ROOT" != /* || -z "$HEAVY_LOCK" || "$HEAVY_LOCK" != /* ]]; then
    printf 'Set DURIS_ACCOUNTING_RESOURCE_ROOT and DURIS_ACCOUNTING_DOCKER_HEAVY_LOCK to the assigned absolute lock paths.\n' >&2
    exit 2
fi
for lock in "$RESOURCE_ROOT/test-slot-1.lock" "$HEAVY_LOCK"; do
    if [[ ! -f "$lock" || -L "$lock" ]]; then
        printf 'Required assigned resource lock is absent or unsafe: %s\n' "$lock" >&2
        exit 2
    fi
done

exec 9>>"$RESOURCE_ROOT/test-slot-1.lock"
flock -w 900 -x 9
exec 8>>"$HEAVY_LOCK"
flock -w 900 -s 8
export DURIS_PARALLEL_DB_TEST_SLOT=1

NAME="duris-atm-publication-${BASHPID}-${RANDOM}"
PASSWORD="atm-${BASHPID}-${RANDOM}-${RANDOM}"
DB_NAME="atm_pub_test_${BASHPID}_${RANDOM}"
IMAGE="${ATM_PUBLICATION_DB_IMAGE:-mariadb:10.11}"
PROXY_PID=""
PORT_FILE=""
export MARIADB_ROOT_PASSWORD="$PASSWORD"
cleanup() {
    if [[ -n "$PROXY_PID" ]]; then
        kill "$PROXY_PID" >/dev/null 2>&1 || true
        wait "$PROXY_PID" >/dev/null 2>&1 || true
    fi
    if [[ -n "$PORT_FILE" ]]; then rm -f "$PORT_FILE"; fi
    docker rm -fv "$NAME" >/dev/null 2>&1 || true
    if docker container inspect "$NAME" >/dev/null 2>&1; then
        printf 'ATM SQL disposable container remains after cleanup: %s\n' "$NAME" >&2
        return 1
    fi
}
trap cleanup EXIT HUP INT TERM
if docker container inspect "$NAME" >/dev/null 2>&1; then
    printf 'Refusing to reuse existing ATM SQL fixture container: %s\n' "$NAME" >&2
    exit 1
fi

if [[ "$IMAGE" == mariadb:* ]]; then PASSWORD_ENV=MARIADB_ROOT_PASSWORD; else PASSWORD_ENV=MYSQL_ROOT_PASSWORD; fi
export "$PASSWORD_ENV=$PASSWORD"
docker run -d --name "$NAME" --cpus=2 --memory=2g --memory-swap=2g \
    -p 127.0.0.1::3306 -e "$PASSWORD_ENV" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
if [[ "$mapping" != 127.0.0.1:* ]]; then
    printf 'ATM SQL fixture published outside loopback: %s\n' "$mapping" >&2
    exit 1
fi
published_port="${mapping##*:}"
export ENVIRONMENT=test DB_USER=root DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
ready=0
for candidate in "127.0.0.1:$published_port" "host.docker.internal:$published_port"; do
    TARGET_HOST="${candidate%:*}"
    TARGET_PORT="${candidate##*:}"
    MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp --connect-timeout=3 -h "$TARGET_HOST" -P "$TARGET_PORT" -u "$DB_USER" -N -B)
    for _ in $(seq 1 10); do
        if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break 2; fi
        sleep 1
    done
done
if [[ "$ready" != 1 ]]; then
    printf 'Disposable ATM SQL fixture was unreachable through loopback and the Docker host gateway; container log follows.\n' >&2
    docker logs "$NAME" >&2 || true
    exit 1
fi
if [[ "$TARGET_HOST" != 127.0.0.1 ]]; then
    PORT_FILE="$(mktemp)"
    python3 -c '
import socket, sys, threading
upstream = (sys.argv[1], int(sys.argv[2]))
server = socket.socket()
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("127.0.0.1", 0))
server.listen(32)
with open(sys.argv[3], "w", encoding="ascii") as ready:
    ready.write(str(server.getsockname()[1]))
    ready.flush()
def bridge(client):
    try:
        remote = socket.create_connection(upstream, timeout=5)
        def copy(source, destination):
            try:
                while True:
                    data = source.recv(65536)
                    if not data:
                        break
                    destination.sendall(data)
            except OSError:
                pass
            try:
                destination.shutdown(socket.SHUT_WR)
            except OSError:
                pass
        threading.Thread(target=copy, args=(client, remote), daemon=True).start()
        copy(remote, client)
        remote.close()
    except OSError:
        pass
    finally:
        client.close()
while True:
    connection, _ = server.accept()
    threading.Thread(target=bridge, args=(connection,), daemon=True).start()
' "$TARGET_HOST" "$TARGET_PORT" "$PORT_FILE" >/dev/null 2>&1 &
    PROXY_PID=$!
    for _ in $(seq 1 50); do
        [[ -s "$PORT_FILE" ]] && break
        sleep 0.1
    done
    if [[ ! -s "$PORT_FILE" ]]; then
        printf 'Loopback-only ATM fixture forwarder failed to bind.\n' >&2
        exit 1
    fi
    DB_PORT="$(<"$PORT_FILE")"
    rm -f "$PORT_FILE"
    PORT_FILE=""
    TARGET_HOST=127.0.0.1
    MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp --connect-timeout=3 -h "$TARGET_HOST" -P "$DB_PORT" -u "$DB_USER" -N -B)
else
    DB_PORT="$TARGET_PORT"
fi
DB_HOST=127.0.0.1
export DB_NAME DB_HOST DB_PORT ATM_PUBLICATION_DISPOSABLE_SCHEMA=1
ready=0
for _ in $(seq 1 10); do
    if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
if [[ "$ready" != 1 ]]; then
    printf 'Loopback-only ATM SQL fixture forwarder failed health check.\n' >&2
    exit 1
fi

"${MYSQL[@]}" -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/bootstrap_multithread_safe.sql"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/critical_command_inbox_outbox.sql"
DB_NAME="$DB_NAME" "$ROOT/migrations/verify_critical_command_schema.sh"
for migration in "$ROOT"/migrations/immutable/*.sql; do
    "${MYSQL[@]}" "$DB_NAME" < "$migration"
done
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/currency_ledger.sql"
DB_NAME="$DB_NAME" "$ROOT/migrations/verify_currency_ledger_schema.sh"

python3 "$ROOT/tests/async/test_pa_atm_publication_sql.py" --execute
cleanup
trap - EXIT HUP INT TERM
printf 'ATM SQL fixture cleanup verified absent: %s\n' "$NAME"
