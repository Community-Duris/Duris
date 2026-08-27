# Validation

## Focused Gate

- `python3 tests/async/test_critical_command_coordinator.py` - pass; secure identity,
  canonical codec, journal replay/corruption, exact checkpoint, duplicate attachment,
  acceptance-ordered multi-key execution, unrelated concurrency, stale completion,
  ambiguous retry, replay identity, capacity, completion delivery, and lifecycle.
- `python3 scripts/security_source_check.py` - pass.

## Repository Gate

- `./scripts/format.sh --check` - pass.
- Direct `clang-format --dry-run --Werror` for all new C/C++ files - pass.
- `make -C src` - pass.
- `git diff --check` - pass.
- `make test-all` - pass; build, 185/185 Python regressions, and signal handlers.

No configured database, migration, Redis mutation, or production runtime was used.
