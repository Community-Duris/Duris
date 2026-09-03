#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Probe the configured listener rather than only the default port. cycle_mud.sh
# reads the same .env, so without this a checkout that moves the health port
# reports a healthy server as down. Source in a subshell so only this one field
# is taken and the rest of .env stays out of the environment.
if [[ -z "${DURIS_HEALTH_URL:-}" && -f "$root/.env" && ! -L "$root/.env" ]]; then
	DURIS_HEALTH_URL="$(
		set +u
		# shellcheck disable=SC1090,SC1091
		source "$root/.env" >/dev/null 2>&1 || true
		printf '%s' "${DURIS_HEALTH_URL:-}"
	)"
fi

health_url="${DURIS_HEALTH_URL:-http://127.0.0.1:4050/health}"
response="$(curl --fail --silent --show-error --max-time 3 "$health_url")"

python3 -c '
import json
import sys

payload = json.load(sys.stdin)
if payload != {"status": "healthy", "persistence": "ready"}:
    raise SystemExit("unexpected DurisMUD health response")
' <<<"$response"
