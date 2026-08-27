# Session Specification

**Session ID**: `phase02-session04-account-bank-and-wallet-transactions`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `7fe860c2`

## Objectives

1. Add explicit wallet and account-bank revisions, opening baselines, and an immutable
   operation-keyed denomination-vector ledger with guarded verification/reconciliation.
2. Extend the critical repository with one typed command that locks player then account
   state canonically and commits wallet, bank, revisions, ledger, inbox result, and
   success outbox atomically.
3. Make ATM deposit-all, denomination deposit/withdrawal, aggregate bank payment/change,
   refunds, pickups, and audited rewards use stable commands and exact game-thread ACKs.
4. Publish committed wallet state to the initiating player and committed bank state to
   every online same-account/same-racewar alternate.
5. Remove transaction-owned wallet and bank fields from independent checkpoint and
   legacy flat-file authority without weakening new-character/load initialization.

## Design Boundary

The command identifies one player PID and one account-bank identity, carries signed
four-denomination wallet and bank vectors, typed reason/source metadata, and expected
wallet/bank revisions. The vectors may transfer equal denomination counts between wallet
and bank or apply a one-sided reward/refund, but every resulting denomination must remain
nonnegative and within the schema/application bound.

The game thread owns operation creation, bounded typed continuation state, and publication.
Workers receive only immutable values, acquire player then account-bank rows in canonical
order, and return exact resulting vectors/revisions. No cached character balance is an SQL
write authority after cutover.

## Success Criteria

- [x] Wallet/bank vectors, revisions, ledger, inbox result, and success outbox commit once.
- [x] Duplicate, ambiguous, stale, insufficient, invalid, and overflow paths are exact.
- [x] Deposit-all is one indivisible four-denomination command.
- [x] Initiating wallet and every matching online alternate bank cache publish only ACK data.
- [x] Audited refunds, pickups, rewards, and aggregate bank-payment change use the boundary.
- [x] Player checkpoints and flat-file replay cannot overwrite transactional currency.
- [x] Opening baselines plus ledger deltas reconcile to every materialized balance.
- [x] Focused, schema, format, build, security, and full validation gates pass.
