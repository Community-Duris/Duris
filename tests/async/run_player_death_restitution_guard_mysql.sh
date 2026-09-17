#!/usr/bin/env bash
set -euo pipefail

# Real-session exclusion test. It never mutates application tables.
block() {
    printf 'BLOCKED: %s\n' "$1" >&2
    exit 77
}
failure() {
    printf 'FAIL: %s\n' "$1" >&2
    exit 1
}

case "${ENVIRONMENT:-}" in
    test|dev|development|local) ;;
    *) block 'ENVIRONMENT must be test/dev/development/local' ;;
esac

DB_HOST=${DB_HOST:-}
DB_PORT=${DB_PORT:-3306}
DB_USER=${DB_USER:-}
DB_NAME=${DB_NAME:-}
if [[ -z "$DB_HOST" || -z "$DB_USER" || -z "$DB_NAME" ]]; then
    block 'DB_HOST, DB_USER, and DB_NAME are required'
fi
case "${DB_HOST,,} ${DB_NAME,,}" in
    *prod*|*production*|*live*) block 'production/live SQL targets are forbidden' ;;
esac
if [[ -z "${MYSQL_PWD:-}" && -n "${DB_PASSWD:-}" ]]; then
    export MYSQL_PWD="$DB_PASSWD"
fi
if [[ -z "${MYSQL_PWD:-}" ]]; then
    block 'MYSQL_PWD or DB_PASSWD is required'
fi

MYSQL_BIN=${MYSQL_BIN:-}
if [[ -z "$MYSQL_BIN" ]]; then
    MYSQL_BIN=$(command -v mysql || command -v mariadb || true)
elif [[ "$MYSQL_BIN" != */* ]]; then
    MYSQL_BIN=$(command -v "$MYSQL_BIN" || true)
fi
if [[ -z "$MYSQL_BIN" || ! -x "$MYSQL_BIN" ]]; then
    block 'mysql/mariadb client is unavailable'
fi
if [[ ! "$DB_PORT" =~ ^[0-9]+$ ]] || (( DB_PORT < 1 || DB_PORT > 65535 )); then
    block 'DB_PORT must be a valid TCP port'
fi

MYSQL_COMMAND=(
    "$MYSQL_BIN" --protocol=tcp --connect-timeout=10
    -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER"
    -N -B --raw --binary-mode "$DB_NAME"
)
LOCK_EXPR="CONCAT('duris.player.death.restitution.',DATABASE())"
TMP=$(mktemp -d "${TMPDIR:-/tmp}/duris-issue331-guard.XXXXXX")
HOLDER_PID=''
HOLDER_OUT=''
HOLDER_ERR=''
HOLDER_THREAD_ID=''

run_query() {
    "${MYSQL_COMMAND[@]}" -e "$1"
}

assert_value() {
    local sql=$1 expected=$2 actual
    if ! actual=$(run_query "$sql" 2>"$TMP/query.err"); then
        block "SQL target became unreachable: ${TMP}/query.err"
    fi
    actual=${actual//$'\r'/}
    [[ "$actual" == "$expected" ]] || failure "expected [$expected], got [$actual]"
}

wait_for_competing_refusal() {
    local attempt actual owner
    for attempt in $(seq 1 100); do
        if ! kill -0 "$HOLDER_PID" 2>/dev/null; then
            return 1
        fi
        if actual=$(run_query "SELECT GET_LOCK(${LOCK_EXPR},0); SELECT RELEASE_LOCK(${LOCK_EXPR});" 2>"$TMP/competing.err"); then
            actual=${actual//$'\r'/}
            # A failed acquisition returns 0; RELEASE_LOCK also returns 0.
            if [[ "$actual" == $'0\n0' ]]; then
                owner=$(run_query "SELECT IS_USED_LOCK(${LOCK_EXPR})" 2>"$TMP/owner.err" || true)
                if [[ "$owner" =~ ^[0-9]+$ ]] && (( owner > 0 )); then
                    HOLDER_THREAD_ID="$owner"
                    return 0
                fi
                return 1
            fi
        fi
        sleep 0.05
    done
    return 2
}

start_holder() {
    local role=$1 statements
    HOLDER_OUT="$TMP/${role}.out"
    HOLDER_ERR="$TMP/${role}.err"
    HOLDER_THREAD_ID=''
    if [[ "$role" == recovery ]]; then
        statements="SET autocommit=0; START TRANSACTION; SELECT GET_LOCK(${LOCK_EXPR},0); DO SLEEP(120);"
    else
        statements="SELECT GET_LOCK(${LOCK_EXPR},0); DO SLEEP(120);"
    fi
    # This is one real client process and therefore one server-owned session.
    "${MYSQL_COMMAND[@]}" -e "$statements" >"$HOLDER_OUT" 2>"$HOLDER_ERR" &
    HOLDER_PID=$!
}

stop_holder() {
    if [[ -n "$HOLDER_PID" ]]; then
        if [[ "$HOLDER_THREAD_ID" =~ ^[0-9]+$ ]]; then
            current_owner=$(run_query "SELECT IS_USED_LOCK(${LOCK_EXPR})" 2>/dev/null || true)
            if [[ "$current_owner" == "$HOLDER_THREAD_ID" ]]; then
                run_query "KILL CONNECTION $HOLDER_THREAD_ID" >/dev/null 2>"$TMP/kill.err" || true
            fi
        fi
        kill -TERM "$HOLDER_PID" 2>/dev/null || true
        wait "$HOLDER_PID" 2>/dev/null || true
        HOLDER_PID=''
        HOLDER_THREAD_ID=''
    fi
}

require_competing_refusal() {
    local role=$1 status
    if wait_for_competing_refusal; then
        return 0
    fi
    status=$?
    stop_holder
    case "$status" in
        1) block "$role holder exited before owning the SQL lock" ;;
        *) block "$role holder did not make the SQL lock unavailable" ;;
    esac
}

cleanup() {
    stop_holder
    rm -rf "$TMP"
}
trap cleanup EXIT

# Connectivity is a prerequisite, not a mocked positive.
assert_value 'SELECT 1' '1'
assert_value "SELECT GET_LOCK(${LOCK_EXPR},0); SELECT RELEASE_LOCK(${LOCK_EXPR});" $'1\n1'

# A runtime-like session acquires the boundary; a new runtime startup refuses.
start_holder runtime
require_competing_refusal runtime

# Ownership is session-bound: killing the holder must release it.
stop_holder
released=0
for _ in $(seq 1 50); do
    if actual=$(run_query "SELECT GET_LOCK(${LOCK_EXPR},0); SELECT RELEASE_LOCK(${LOCK_EXPR});" 2>"$TMP/release.err"); then
        actual=${actual//$'\r'/}
        if [[ "$actual" == $'1\n1' ]]; then
            released=1
            break
        fi
    fi
    sleep 0.1
done
(( released == 1 )) || failure 'lock was not released after holder loss'

# A recovery-like transaction owns the same boundary; startup still refuses.
start_holder recovery
require_competing_refusal recovery
stop_holder

# The lock is available again only after the recovery session ends.
released=0
for _ in $(seq 1 50); do
    if actual=$(run_query "SELECT GET_LOCK(${LOCK_EXPR},0); SELECT RELEASE_LOCK(${LOCK_EXPR});" 2>"$TMP/release-final.err"); then
        actual=${actual//$'\r'/}
        if [[ "$actual" == $'1\n1' ]]; then
            released=1
            break
        fi
    fi
    sleep 0.1
done
(( released == 1 )) || failure 'lock was not released after recovery holder loss'
printf 'PASS: real DB advisory exclusion blocks competing startup and releases on session loss\n'
