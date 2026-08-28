#!/bin/bash
set -euo pipefail

usage() {
    printf 'usage: %s --confirm <host:port/database|unix:/absolute/socket/database>\n' "$0"
}

case "${1:-}" in
    -h|--help)
        usage
        exit 0
        ;;
    --confirm)
        if [[ $# -ne 2 || -z "${2:-}" ]]; then
            usage >&2
            exit 2
        fi
        CONFIRMED_TARGET="$2"
        ;;
    "")
        usage >&2
        exit 2
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
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi
ENVIRONMENT="${ENVIRONMENT:-}" \
REDIS_HOST="${REDIS_HOST:-}" \
REDIS_PORT="${REDIS_PORT:-}" \
REDIS_SOCKET="${REDIS_SOCKET:-}" \
REDIS_DB="${REDIS_DB:-}" \
REDIS_NAMESPACE="${REDIS_NAMESPACE:-}" \
REDIS_USERNAME="${REDIS_USERNAME:-}" \
REDIS_PASSWORD="${REDIS_PASSWORD:-}" \
REDIS_MAINTENANCE_USERNAME="${REDIS_MAINTENANCE_USERNAME:-}" \
REDIS_MAINTENANCE_PASSWORD="${REDIS_MAINTENANCE_PASSWORD:-}" \
REDIS_TLS="${REDIS_TLS:-}" \
REDIS_CA_CERT="${REDIS_CA_CERT:-}" \
REDIS_ALLOWED_TARGETS="${REDIS_ALLOWED_TARGETS:-}" \
REDIS_DESTRUCTIVE_CONFIRM="$CONFIRMED_TARGET" \
    "$SCRIPT_DIR/clear-duris-redis-keys.sh"
