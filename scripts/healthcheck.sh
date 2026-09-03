#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Sourcing runs whatever the file contains, so refuse a .env anyone else can
# write, the same owner and 0600-equivalent mode cycle_mud.sh requires. Unlike
# cycle_mud.sh this warns and carries on with the default rather than exiting:
# this script is the container HEALTHCHECK, and failing it over a permission bit
# would report a healthy server as down and cycle the container.
env_health_url_safe() {
	local env_file="$1" mode owner
	[[ -f "$env_file" && ! -L "$env_file" ]] || return 1
	mode="$(stat -c '%a' "$env_file")" || return 1
	owner="$(stat -c '%u' "$env_file")" || return 1
	if (( (8#$mode & 0177) != 0 )) || [[ "$owner" != "$(id -u)" ]]; then
		printf '%s\n' "Ignoring unsafe $env_file metadata; run: chmod 600 $env_file" >&2
		return 1
	fi
	return 0
}

# Probe the configured listener rather than only the default port. cycle_mud.sh
# reads the same .env, so without this a checkout that moves the health port
# reports a healthy server as down. Source in a subshell so only this one field
# is taken and the rest of .env stays out of the environment.
if [[ -z "${DURIS_HEALTH_URL:-}" ]] && env_health_url_safe "$root/.env"; then
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
