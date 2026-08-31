#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
set -a
# shellcheck disable=SC1091
source "$ROOT/.env"
set +a

environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || {
    echo 'refusing account-projection test: environment is not development/local/test' >&2
    exit 1
}
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || {
    echo 'refusing account-projection test: database name is not development/local/test' >&2
    exit 1
}

python3 "$ROOT/tests/async/account_character_projection_mysql_harness.py"
