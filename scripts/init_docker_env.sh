#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCKER_ENV_FILE="$PROJECT_ROOT/.env.docker"

if [[ -e "$DOCKER_ENV_FILE" || -L "$DOCKER_ENV_FILE" ]]; then
  echo "Refusing to overwrite existing Docker configuration: $DOCKER_ENV_FILE" >&2
  exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 is required to generate Docker database credentials" >&2
  exit 1
fi

database_password="$(python3 -c 'import secrets; print(secrets.token_hex(32))')"
root_password="$(python3 -c 'import secrets; print(secrets.token_hex(32))')"
temporary_file="$(mktemp "$PROJECT_ROOT/.env.docker.tmp.XXXXXX")"
cleanup() {
  rm -f -- "$temporary_file"
}
trap cleanup EXIT HUP INT TERM
chmod 0600 "$temporary_file"

{
  printf 'DURIS_DB_PASSWORD=%s\n' "$database_password"
  printf 'DURIS_DB_ROOT_PASSWORD=%s\n' "$root_password"
  printf 'DURIS_BUILD_JOBS=2\n'
  printf 'DURIS_DOCKER_BIND_ADDRESS=127.0.0.1\n'
  printf 'DURIS_GAME_HOST_PORT=4000\n'
  printf 'DURIS_TLS_HOST_PORT=4001\n'
  printf 'DURIS_WEB_HOST_PORT=4050\n'
} > "$temporary_file"

mv -T -- "$temporary_file" "$DOCKER_ENV_FILE"
temporary_file=""
echo "Created owner-readable Docker configuration: $DOCKER_ENV_FILE"
