# Validation

## Focused Gate

- `python3 tests/async/test_epic_transaction_contract.py` - pass; schema, codecs,
  transaction ordering, mutation inventory, checkpoint authority, and ACK publication.
- `tests/async/run_epic_transaction_schema_mysql.sh` - pass; guarded migration, exact
  schema, award, spend, insufficient funds, duplicate replay, ledger, baseline, and
  rejection-without-outbox behavior.
- `migrations/reconcile_epic_balances.sh` - pass; all three discrepancy counts are zero.
- Epic bonus state/hot-path, boot preflight, critical coordinator/transaction,
  publication-copyover drain, player snapshot, and ship save guard tests - pass.
- `python3 scripts/security_source_check.py` - pass.

## Repository Gate

- `./scripts/format.sh --check` - pass.
- `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `make test-all` - pass; build, 187/187 Python regressions, and signal handlers.

The configured development schema retains the additive epic tables and revision column.
No production database, credentials, player/account export, Redis state, or destructive
operation was used.
