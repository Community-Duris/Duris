# Implementation Notes

**Session ID**: `phase00-session07-account-bank-delta-safety`
**Started**: 2026-08-27
**Last Updated**: 2026-08-27

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 16 / 16 |
| Estimated Remaining | Complete |
| Blockers | 0 |

## Implementation Summary

- Removed `sql_save_account_bank()` and every normal cached absolute shared-bank write.
- Added strict cache-representable four-denomination results and deterministic no-MySQL failure outputs.
- Made denomination deposits and guarded withdrawals own short transactions that check begin, ensure, update, affected rows, result read, and commit.
- Added an authoritative locked aggregate withdrawal that preserves legacy denomination consumption and wallet change while applying one arithmetic delta vector.
- Added utility publication to every playing character on the same case-insensitive account and racewar side, including switched players' originals.
- Changed ATM, training, locker, cash-boon, and online ship-insurance paths to mutate or report only after checked bank success; failed insurance retains its existing durable pickup fallback.
- Added source contracts and an ephemeral MySQL 8 regression for stale caches, insufficiency, forced failure, aggregate change, and concurrent deltas.

## Verification Evidence

- Focused account-bank source contract: PASS.
- Ephemeral isolated MySQL 8 delta regression: PASS.
- Nearest SQL, locker, ship, persistence-status, and commit-failure regressions: PASS.
- `./scripts/format.sh --check`: PASS.
- `make -C src`: PASS with the C++20 warning-as-error profile.
- `make test-all`: PASS; 174/174 Python regressions plus signal-handler checks.
- `git diff --check` and UTF-8 session-file scan: PASS.

## Review Repair

Review made aggregate denomination rounding use widened arithmetic, distinguished a missing or malformed row from genuine insufficient funds after a zero-row guarded update, updated original PCs behind switched descriptors, initialized no-MySQL output objects deterministically, and compensated an unpaid private-chest creation by deleting the new chest before reporting failure.

## Scope Notes

- No migration, schema, dependency, configured database, production system, credential, or player/account data changed.
- The isolated database test used a disposable container and test-only schema.
- Bank and carried-wallet durability remain separate temporary boundaries; Phase 02 adds operation identity, atomic wallet/bank transactions, ledger, outbox, and reconciliation.
