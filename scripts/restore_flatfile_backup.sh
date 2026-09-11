#!/usr/bin/env bash
set -euo pipefail
# The old raw-copy interface is closed. Restore now creates a fresh candidate
# below the policy's isolated restore root.
if [[ $# -ne 2 ]]; then
  echo "usage: BACKUP_POLICY_FILE=<policy> $0 <generation-id> <current-tombstone-evidence>" >&2
  exit 2
fi
exec python3 "$(dirname "$0")/persistence_backup.py" \
  --policy "$BACKUP_POLICY_FILE" \
  restore --generation "$1" --tombstones "$2"
