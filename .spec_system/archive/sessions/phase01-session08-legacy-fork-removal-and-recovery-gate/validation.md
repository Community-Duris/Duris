# Validation

## Focused Gate

- `python3 tests/async/test_phase01_recovery_gate.py` - pass; 25/50/100/200 waves.
- `python3 tests/async/test_redis_failure_containment.py` - pass.
- `python3 tests/async/test_world_recovery_pipeline.py` - pass.
- `python3 tests/async/test_player_save_worker.py` - pass.
- `python3 tests/async/test_player_save_journal.py` - pass.
- `python3 tests/async/test_terminal_save_safety.py` - pass.

## Repository Gate

The initial full run exposed one stale source-contract expectation for the intentionally
raised pipeline capacity (183/184 passed). The expectation was updated to 256 and the
complete gate was rerun.

- `./scripts/format.sh --check` - pass.
- `git diff --check` - pass.
- `python3 tests/async/test_security_dependency_baseline.py` - pass.
- `make test-all` - pass; build, 184/184 Python regressions, and signal handlers.
