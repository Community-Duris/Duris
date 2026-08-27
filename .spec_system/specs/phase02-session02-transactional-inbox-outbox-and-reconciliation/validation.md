# Validation

## Focused Gate

- `python3 tests/async/test_critical_transaction_contract.py` - pass; migration,
  repository, hashing, transaction order, duplicate, ambiguity, retry, outbox bounds,
  lifecycle, thread initialization, diagnostics, and redaction source/runtime contracts.
- `tests/async/run_critical_command_schema_mysql.sh` - pass; guarded development schema
  applied twice, 4 tables/34 columns/8 indexes/2 foreign keys verified, repository
  duplicate/mismatch/concurrency/overflow checks passed, and outbox delivery/retry/
  dead-letter/restart/reconciliation checks passed.
- `python3 tests/async/test_critical_command_coordinator.py` - pass after lifecycle
  ordering contract was updated for the outbox stage.
- `python3 scripts/security_source_check.py` - pass.

## Repository Gate

- `./scripts/format.sh --check` - pass.
- `git diff --check` - pass.
- `make -C src` - pass with the C++20 warning-as-error profile.
- `make test-all` - pass; build, 186/186 Python regressions, and signal handlers.

The configured local development schema retains the four additive tables. Harness data
was cleaned; no production, Redis, credential, player-data, or destructive operation was
used.
