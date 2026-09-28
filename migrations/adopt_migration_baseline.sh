#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: "${ENVIRONMENT:=development}"
export ENVIRONMENT

# The legacy upgrade establishes the verified baseline before immutable changes.
# Adoption is idempotent for an existing matching baseline, but refuses stale
# fingerprints. Never treat an arbitrary migration failure as a reason to adopt
# again: a partially applied immutable DDL step needs operator recovery.
python3 "$ROOT/scripts/migration_runner.py" adopt --kind verified_legacy_adoption
python3 "$ROOT/scripts/migration_runner.py" run
# Opening character baselines are created only after the legacy schema exists.
# Imported characters must still pass the full verifier before first load.
"$ROOT/migrations/verify_runtime_compatibility.sh" --schema-only
echo "immutable migration head verified; character baselines still require full runtime verification"
