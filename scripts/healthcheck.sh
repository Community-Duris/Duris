#!/usr/bin/env bash
set -euo pipefail

health_url="${DURIS_HEALTH_URL:-http://127.0.0.1:4050/health}"
response="$(curl --fail --silent --show-error --max-time 3 "$health_url")"

python3 -c '
import json
import sys

payload = json.load(sys.stdin)
if payload != {"status": "healthy", "database": "ready"}:
    raise SystemExit("unexpected DurisMUD health response")
' <<<"$response"
