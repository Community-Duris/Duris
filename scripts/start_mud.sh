#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 1

# Prefer the supervised local-development service when it is installed.
# This makes repeated invocations idempotent and keeps console output in
# logs/duris-console.log.
if [[ $# -eq 0 ]] && command -v systemctl >/dev/null 2>&1 &&
   systemctl --user cat duris-mud.service >/dev/null 2>&1; then
  SERVICE_ROOT=$(systemctl --user show duris-mud.service \
    --property=WorkingDirectory --value 2>/dev/null || true)
  if [[ -n "$SERVICE_ROOT" && "$(realpath -m "$SERVICE_ROOT")" == "$ROOT" ]]; then
    systemctl --user start duris-mud.service
    echo "DurisMUD service is running."
    exit 0
  fi
fi

nohup ./scripts/cycle_mud.sh "$@" > logs/duris-console.log 2>&1 &
echo "DurisMUD started; console output is in logs/duris-console.log."
