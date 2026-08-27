#!/bin/bash
set -e

case "${1:-}" in
    "") ;;
    -h|--help)
        printf 'usage: %s\n' "$0"
        exit 0
        ;;
    *)
        printf 'unknown argument: %s\n' "$1" >&2
        exit 2
        ;;
esac

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
ENV_FILE="$PROJECT_ROOT/.env"
if [[ -L "$ENV_FILE" ]]; then
    printf 'unsafe environment file: symbolic links are not allowed\n' >&2
    exit 2
elif [[ -e "$ENV_FILE" ]]; then
    if [[ ! -f "$ENV_FILE" || "$(stat -c '%u' "$ENV_FILE")" != "$(id -u)" ||
          $((8#$(stat -c '%a' "$ENV_FILE") & 0177)) -ne 0 ]]; then
        printf 'unsafe environment file: require an owner-only regular file\n' >&2
        exit 2
    fi
    # shellcheck disable=SC1091
    source "$ENV_FILE"
fi
: "${REDIS_HOST:?REDIS_HOST is required}"
: "${REDIS_PORT:?REDIS_PORT is required}"

echo "clearing redis cache..."
redis-cli -h "$REDIS_HOST" -p "$REDIS_PORT" FLUSHDB
echo "done."
