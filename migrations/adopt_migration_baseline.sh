#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: "${ENVIRONMENT:=development}"
export ENVIRONMENT

# The legacy upgrade is deliberately re-runnable.  A database that already has
# a valid immutable head contains post-baseline tables, so attempting to adopt
# the pre-migration fingerprint again would reject an otherwise healthy schema.
if python3 "$ROOT/scripts/migration_runner.py" run >/dev/null 2>&1 && \
   "$ROOT/migrations/verify_runtime_compatibility.sh" >/dev/null 2>&1; then
    echo "existing immutable migration head verified"
    exit 0
fi

exec python3 "$ROOT/scripts/migration_runner.py" adopt --kind verified_legacy_adoption
