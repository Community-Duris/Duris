# Task Checklist

**Session ID**: `phase02-session04-account-bank-and-wallet-transactions`
**Total Tasks**: 25
**Created**: 2026-08-27

## Inventory And Design

- [x] T001 Reconcile Phase 00 bank-delta and Session 01-03 transaction contracts.
- [x] T002 Inventory account-bank schema/helpers, ATM paths, alt publication, and baselines.
- [x] T003 Inventory wallet checkpoint fields and ADD/SUB/refund/pickup/reward durability routes.
- [x] T004 Define denomination vectors, canonical keys/locks, revisions, result, and reasons.

## Schema And Repository

- [x] T005 Add guarded wallet/bank revision, baseline, ledger, indexes, and bootstrap schema.
- [x] T006 Add exact verifier, guarded baseline, and read-only reconciliation tools.
- [x] T007 Add typed currency command/result codecs with fixed bounds and invariants.
- [x] T008 Add prepared repository dispatch and canonical player/account-bank locking.
- [x] T009 Apply funds/bounds/revision checks and atomic state/ledger/result/outbox commit.
- [x] T010 Add duplicate, ambiguity, deadlock, connection-repair, and restart convergence.

## Game-Thread And Producer Cutover

- [x] T011 Add bounded operation-keyed currency completions and lifecycle/health routing.
- [x] T012 Publish exact wallet results and all matching online alternate bank results.
- [x] T013 Cut denomination ATM deposits and withdrawals to typed commands.
- [x] T014 Make deposit-all one immutable four-denomination operation.
- [x] T015 Cut aggregate bank payment/change and checked bank reward/deposit adapters.
- [x] T016 Cut auction pickup/refund adapters while preserving Session 08 escrow ownership.
- [x] T017 Cut audited wallet rewards/refunds and durability-crossing direct coin mutations.
- [x] T018 Gate overlapping player/account economy operations through coordinator fences.
- [x] T019 Remove transactional currency from generic snapshots, SQL saves, and flat replay.
- [x] T020 Preserve authoritative hydration, new-character baseline, GMCP, and legacy UX.

## Tests And Completion

- [x] T021 Add schema/baseline/reconciliation and repository atomicity MySQL regressions.
- [x] T022 Add deposit-all, withdrawal, duplicate, stale, insufficient, overflow regressions.
- [x] T023 Add two-character alt publication, reconnect, lifecycle, and source inventory tests.
- [x] T024 Run focused, format, build, security, full, and guarded MySQL validation.
- [x] T025 Complete reviews/records, update PRD/state/version, commit, and publish.
