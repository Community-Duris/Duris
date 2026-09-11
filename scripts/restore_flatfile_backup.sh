#!/usr/bin/env bash
set -euo pipefail
# The old raw-copy interface is closed. Restore now creates a fresh candidate
# below the policy's isolated restore root.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [[ -z "${BACKUP_ENV_FILE:-}" ]]; then BACKUP_ENV_FILE="$(dirname "$SCRIPT_DIR")/.env"; fi
if [[ -z "${BACKUP_POLICY_FILE:-}" ]]; then BACKUP_POLICY_FILE=""; fi
if [[ $# -ne 2 ]]; then
  echo "usage: BACKUP_POLICY_FILE=<policy> $0 <generation-id> <current-tombstone-evidence>" >&2
  exit 2
fi
ARGS=(--env-file "$BACKUP_ENV_FILE")
if [[ -n "$BACKUP_POLICY_FILE" ]]; then ARGS+=(--policy "$BACKUP_POLICY_FILE"); fi
exec python3 "$SCRIPT_DIR/persistence_backup.py" "${ARGS[@]}" \
  restore --generation "$1" --tombstones "$2"
