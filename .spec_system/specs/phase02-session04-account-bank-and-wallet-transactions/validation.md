# Validation

## Focused Gate

- `python3 tests/async/test_currency_transaction_contract.py` - pass; schema, codecs,
  repository ordering, ATM atomicity, producer inventory, checkpoint authority,
  alternate publication, fences, and lifecycle routing.
- `tests/async/run_currency_transaction_schema_mysql.sh` - pass; guarded migration,
  exact schema, deposit-all, withdrawal, insufficient funds, stale revision, overflow,
  duplicate replay, ledger/outbox, baseline, and rejection-without-outbox behavior.
- `migrations/reconcile_currency_balances.sh` - pass; all four discrepancy counts are
  zero.
- Account-bank delta safety, auction persistence, boot preflight, critical transaction
  and coordinator, player snapshot, copyover-drain, and security tests - pass.
- `make security-check` - pass.

## Repository Gate

- `./scripts/format.sh --check` - pass.
- `git diff --check` - pass.
- `make -C src` - pass under the C++20 warning-as-error profile.
- `make test-all` - pass; build, 188/188 Python regressions, and signal handlers.

The configured development schema retains the additive currency tables and revision
columns. No production database, credential, player/account export, Redis state, wipe,
or destructive migration was used. The installed user service was already running from
a separate workspace and was left untouched, so no new-binary live login is claimed.
