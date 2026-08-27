#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: "${ENVIRONMENT:=development}"
export ENVIRONMENT
exec python3 "$ROOT/scripts/migration_runner.py" adopt --kind verified_legacy_adoption
