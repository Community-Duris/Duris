#!/bin/bash
set -euo pipefail

: "${ENVIRONMENT:?ENVIRONMENT is required}"
: "${REDIS_HOST:?REDIS_HOST is required}"
: "${REDIS_PORT:?REDIS_PORT is required}"
: "${REDIS_DB:?REDIS_DB is required}"
: "${REDIS_NAMESPACE:?REDIS_NAMESPACE is required}"
: "${REDIS_TLS:?REDIS_TLS is required}"
: "${REDIS_ALLOWED_TARGETS:?REDIS_ALLOWED_TARGETS is required}"
: "${REDIS_DESTRUCTIVE_CONFIRM:?REDIS_DESTRUCTIVE_CONFIRM is required}"

if [[ "$ENVIRONMENT" != "local" ]]; then
    printf 'refusing Redis deletion: ENVIRONMENT must be local\n' >&2
    exit 2
fi
if [[ ! "$REDIS_NAMESPACE" =~ ^duris:local:[a-z0-9]([a-z0-9_-]{0,30}[a-z0-9])?$ ]]; then
    printf 'refusing Redis deletion: REDIS_NAMESPACE must match duris:local:<deployment>\n' >&2
    exit 2
fi
if [[ ! "$REDIS_PORT" =~ ^[0-9]+$ ]] || ((REDIS_PORT < 1 || REDIS_PORT > 65535)); then
    printf 'refusing Redis deletion: invalid REDIS_PORT\n' >&2
    exit 2
fi
if [[ ! "$REDIS_DB" =~ ^[0-9]+$ ]] || ((REDIS_DB > 255)); then
    printf 'refusing Redis deletion: REDIS_DB must be an integer from 0 through 255\n' >&2
    exit 2
fi
if [[ "$REDIS_TLS" != "TRUE" && "$REDIS_TLS" != "FALSE" ]]; then
    printf 'refusing Redis deletion: REDIS_TLS must be TRUE or FALSE\n' >&2
    exit 2
fi
case "$REDIS_HOST" in
    127.0.0.1|localhost|::1) ;;
    *)
        if [[ "$REDIS_TLS" != "TRUE" ]]; then
            printf 'refusing Redis deletion: non-loopback Redis requires REDIS_TLS=TRUE\n' >&2
            exit 2
        fi
        ;;
esac

TARGET="$REDIS_HOST:$REDIS_PORT/$REDIS_DB"
case ",$REDIS_ALLOWED_TARGETS," in
    *,"$TARGET",*) ;;
    *)
        printf 'refusing Redis deletion: %s is not in REDIS_ALLOWED_TARGETS\n' "$TARGET" >&2
        exit 2
        ;;
esac
if [[ "$REDIS_DESTRUCTIVE_CONFIRM" != "$TARGET" ]]; then
    printf 'refusing Redis deletion: confirmation must exactly equal %s\n' "$TARGET" >&2
    exit 2
fi
if ! command -v redis-cli >/dev/null 2>&1; then
    printf 'refusing Redis deletion: redis-cli is required\n' >&2
    exit 2
fi

REDIS_CLI=(redis-cli --raw -h "$REDIS_HOST" -p "$REDIS_PORT" -n "$REDIS_DB")
if [[ -n "${REDIS_USERNAME:-}" ]]; then
    REDIS_CLI+=(--user "$REDIS_USERNAME")
fi
if [[ -n "${REDIS_PASSWORD:-}" ]]; then
    REDISCLI_AUTH="$REDIS_PASSWORD"
    export REDISCLI_AUTH
fi
if [[ "$REDIS_TLS" == "TRUE" ]]; then
    if [[ -z "${REDIS_CA_CERT:-}" || ! -f "$REDIS_CA_CERT" || ! -r "$REDIS_CA_CERT" ]]; then
        printf 'refusing Redis deletion: readable REDIS_CA_CERT is required for TLS\n' >&2
        exit 2
    fi
    REDIS_CLI+=(--tls --cacert "$REDIS_CA_CERT")
fi
if [[ "$("${REDIS_CLI[@]}" PING)" != "PONG" ]]; then
    printf 'refusing Redis deletion: PING failed for %s\n' "$TARGET" >&2
    exit 1
fi

DELETE_BATCH='local r=redis.call("SCAN",ARGV[1],"MATCH",ARGV[2],"COUNT",256); local n=#r[2]; for i=1,n,128 do redis.call("DEL",unpack(r[2],i,math.min(i+127,n))) end; return {r[1],n}'
COUNT_BATCH='local r=redis.call("SCAN",ARGV[1],"MATCH",ARGV[2],"COUNT",256); return {r[1],#r[2]}'
PATTERNS=("$REDIS_NAMESPACE:*" 'mud:*' 'ship:snapshot:*')

scan_batches() {
    local script="$1"
    local pattern="$2"
    local cursor=0
    local total=0
    local reply next count
    local -a fields
    while :; do
        if ! reply="$("${REDIS_CLI[@]}" EVAL "$script" 0 "$cursor" "$pattern")"; then
            printf 'Redis scan failed for pattern %s\n' "$pattern" >&2
            return 1
        fi
        mapfile -t fields <<< "$reply"
        if [[ ${#fields[@]} -ne 2 ]]; then
            printf 'unexpected Redis scan reply for pattern %s\n' "$pattern" >&2
            return 1
        fi
        next="${fields[0]}"
        count="${fields[1]}"
        if [[ ! "$next" =~ ^[0-9]+$ || ! "$count" =~ ^[0-9]+$ ]]; then
            printf 'unexpected Redis scan reply for pattern %s\n' "$pattern" >&2
            return 1
        fi
        total=$((total + count))
        cursor="$next"
        [[ "$cursor" == 0 ]] && break
    done
    printf '%d\n' "$total"
}

deleted=0
for pattern in "${PATTERNS[@]}"; do
    count="$(scan_batches "$DELETE_BATCH" "$pattern")"
    deleted=$((deleted + count))
done

remaining=0
for pattern in "${PATTERNS[@]}"; do
    count="$(scan_batches "$COUNT_BATCH" "$pattern")"
    remaining=$((remaining + count))
done
if ((remaining != 0)); then
    printf 'Redis deletion postflight failed: %d Duris keys remain\n' "$remaining" >&2
    exit 1
fi

printf 'deleted %d Duris Redis keys from %s; postflight clean\n' "$deleted" "$TARGET"
