# Implementation Notes

**Session ID**: `phase02-session03-epic-ledger-and-balance-transactions`
**Started**: 2026-08-27
**Last Updated**: 2026-08-27

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 24 / 24 |
| Estimated Remaining | Complete |
| Blockers | 0 |

## Implementation Summary

- Added guarded `epic_revision`, `epic_balance_baseline`, and `epic_ledger` schema,
  synchronized bootstrap and migration runner wiring, exact metadata verification,
  guarded baseline capture, read-only reconciliation, and fail-closed boot coverage.
- Added fixed typed epic command/result codecs and repository dispatch with prepared
  statements, balance row locking, expected-revision support, funds/overflow checks,
  atomic balance/revision/ledger/inbox/outbox commit, stable duplicate results, and
  ambiguous-commit reconciliation.
- Added bounded operation-keyed game-thread continuations, exact PID lookup, authoritative
  balance publication, offline completion retention, reconnect publication, lifecycle
  drain observation, health counters, and malformed-completion rejection.
- Cut all audited award and spend producers to the typed boundary. Purchase, award,
  level, skill, reset, ship, training, spellbind, ascension, refund, and administrator
  effects now occur only from the matching committed completion.
- Removed epic balance from immutable player snapshot capture/apply and neutralized
  legacy SQL/flat-file overwrite paths while preserving authoritative SQL hydration and
  new-character initialization.

## Verification Evidence

- `python3 tests/async/test_epic_transaction_contract.py`: PASS, 6/6.
- `tests/async/run_epic_transaction_schema_mysql.sh`: PASS against the guarded local
  development database; exact schema and award/spend/rejection/duplicate behavior pass.
- `migrations/reconcile_epic_balances.sh`: PASS with zero missing baselines, balance
  mismatches, or latest-result mismatches.
- Epic bonus, boot preflight, snapshot, coordinator, copyover-drain, ship-save, and
  critical transaction focused regressions: PASS.
- `make -C src`, `./scripts/format.sh --check`, `git diff --check`, and
  `python3 scripts/security_source_check.py`: PASS.
- `make test-all`: PASS; build, 187/187 Python regressions, and signal-handler checks.

## Scope Notes

- Schema and harness work ran only against the configured local development database
  behind environment and database-name guards. No production migration or wipe ran.
- Baselines were captured for existing development players; new player creation inserts
  its baseline in the same transaction as the player row. Harness rows were removed.
- `epic_gain` remains historical evidence. New non-bottle positive awards are combined
  with the ledger only in compatibility reads that still require historical totals.
