#!/bin/bash
set -e

cd "$(dirname "$0")/.." || exit 1

# Prefer the supervised local-development service when it is installed.
# This makes repeated invocations idempotent and keeps console output in
# logs/duris-console.log.
if command -v systemctl >/dev/null 2>&1 &&
   systemctl --user cat duris-mud.service >/dev/null 2>&1; then
  systemctl --user start duris-mud.service
  echo "DurisMUD service is running."
  exit 0
fi

nohup ./scripts/cycle_mud.sh > logs/duris-console.log 2>&1 &
echo "DurisMUD started; console output is in logs/duris-console.log."
