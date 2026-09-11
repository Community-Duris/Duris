#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ $# -eq 0 ]]; then set -- backup; fi
if [[ ! -v BACKUP_ENV_FILE ]]; then BACKUP_ENV_FILE="$PROJECT_ROOT/.env"; fi
exec python3 "$SCRIPT_DIR/persistence_backup.py" --env-file "$BACKUP_ENV_FILE" "$@"
