# Implementation Notes

**Session ID**: `phase02-session04-account-bank-and-wallet-transactions`
**Started**: 2026-08-27
**Last Updated**: 2026-08-27

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 25 / 25 |
| Estimated Remaining | Complete |
| Blockers | 0 |

## Implementation Summary

- Added guarded wallet/account-bank revision columns, opening-baseline tables, an
  immutable operation-keyed denomination-vector ledger, synchronized bootstrap and
  runner wiring, exact verification, guarded baseline capture, reconciliation, and
  fail-closed boot metadata/coverage checks.
- Added a fixed typed currency command/result, canonical player/account keys, prepared
  repository dispatch, player-then-bank row locks, expected-revision and bounds checks,
  atomic state/revision/ledger/inbox/outbox commit, stable rejection results, and
  ambiguous-commit reconciliation through the shared critical repository.
- Added bounded operation-keyed game-thread continuations, exact wallet publication,
  same-account/same-racewar alternate bank publication, reconnect retention, lifecycle
  drain routing, malformed-result rejection, health counters, and operation fences.
- Cut ATM denomination transfers, one-command deposit-all, aggregate bank payments,
  auction money pickup/refund adapters, boon cash, ship insurance, wallet rewards,
  spends, pickups, wagers, guild transfers, and audited direct mutations to the typed
  boundary. Failed credits retain value through auction-pickup staging where possible.
- Removed wallet columns from player snapshot authority, neutralized legacy SQL update
  and flat-file account-bank authority, and preserved authoritative hydration plus
  same-transaction baselines for new players and account banks.

## Verification Evidence

- `python3 tests/async/test_currency_transaction_contract.py`: PASS, 8/8.
- `tests/async/run_currency_transaction_schema_mysql.sh`: PASS against the guarded local
  development database; schema, deposit-all, withdrawal, insufficient funds, stale
  revisions, overflow, duplicates, ledger, outbox, and baseline behavior pass.
- `migrations/reconcile_currency_balances.sh`: PASS with zero missing wallet/bank
  baselines and zero wallet/bank mismatches.
- Account-bank safety, auction persistence, boot preflight, critical coordinator,
  publication/copyover, snapshot, and security focused regressions: PASS.
- `make -C src`, `./scripts/format.sh --check`, `git diff --check`, and
  `make security-check`: PASS.
- `make test-all`: PASS; build, 188/188 Python regressions, and signal-handler checks.

## Scope Notes

- Schema and harness work ran only against the configured local development database
  behind environment and database-name guards. No production migration or wipe ran.
- The already-running user service points at a different workspace, so this session did
  not restart it or claim an in-game smoke test against the new binary.
- Session 08 still owns auction escrow/claim redesign and Session 11 still owns reward
  producer batching; their current adapters now cross this currency boundary.
