# Task Checklist

**Session ID**: `phase02-session03-epic-ledger-and-balance-transactions`
**Total Tasks**: 24
**Created**: 2026-08-27

## Inventory And Design

- [x] T001 Reconcile Session 01/02 command, inbox, outbox, result, and lifecycle contracts.
- [x] T002 Inventory epic schema, bonus state, checkpoint authority, and direct mutations.
- [x] T003 Group award and spend producers by staged side-effect/revalidation boundary.
- [x] T004 Define baseline, ledger, balance revision, typed command/result, and reconciliation semantics.

## Schema And Domain Foundation

- [x] T005 Add guarded epic ledger/revision migration, bootstrap synchronization, and verifier.
- [x] T006 Add guarded local-development baseline and discrepancy tooling.
- [x] T007 Add canonical epic award/spend payload and exact result codecs.
- [x] T008 Add epic transaction repository and critical command-type dispatch.
- [x] T009 Add balance/revision locking, funds/overflow checks, ledger, result, and outbox atomically.
- [x] T010 Add duplicate, ambiguous, deadlock, and restart convergence.

## Game-Thread Cutover

- [x] T011 Add bounded operation-keyed epic pending continuations and exact completion routing.
- [x] T012 Cut `gain_epic`, bottles, rewards, refunds, bonus publication, tasks, and level eligibility.
- [x] T013 Cut epic store, tradeskill reset, alchemist, specialization, and information purchases.
- [x] T014 Cut epic skill purchase/reset and nexus-stone purchase routes.
- [x] T015 Cut free-level, stone-level, ascend/descend/remort, and reset routes.
- [x] T016 Cut ship hull/upgrade and remaining audited spend routes.
- [x] T017 Cut administrator set/reset paths with explicit typed reasons and audit behavior.
- [x] T018 Remove epic balance from player checkpoint capture/apply and legacy direct save paths.
- [x] T019 Add lifecycle, disconnect/reconnect, fence, retained-effect, health, and redacted alert contracts.

## Tests And Completion

- [x] T020 Add schema/baseline/reconciliation and repository atomicity regressions.
- [x] T021 Add award/spend/insufficient/duplicate/ambiguity/bonus/completion regressions.
- [x] T022 Add exhaustive source inventory and checkpoint-authority regression.
- [x] T023 Run focused, format, build, security, full, and guarded MySQL validation.
- [x] T024 Complete reviews/records, update PRD/state/version, commit, and publish.
