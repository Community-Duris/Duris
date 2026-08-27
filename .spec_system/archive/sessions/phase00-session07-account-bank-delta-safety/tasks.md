# Task Checklist

**Session ID**: `phase00-session07-account-bank-delta-safety`
**Total Tasks**: 16
**Created**: 2026-08-27

---

## Inventory And Design

- [x] T001 Confirm Session 07 selection, clean base `b6491289`, and local/development context.
- [x] T002 Inventory the absolute save, denomination helpers, ATM commands, aggregate payments, boons, insurance, and online-account identity.
- [x] T003 Define owned-transaction, committed-result, delta-only, and online-publication invariants.

## SQL And Publication Boundary

- [x] T004 Add a strict committed account-bank balance result type and parsing.
- [x] T005 Make denomination deposits check begin, ensure, update, result query, and commit.
- [x] T006 Make denomination withdrawals guarded and return a post-update committed result.
- [x] T007 Add one checked delta-only transaction for aggregate copper-value bank payments.
- [x] T008 Remove the cached absolute account-bank save API and all normal callers.
- [x] T009 Publish committed results to every playing same-account/same-racewar character.

## Caller Safety

- [x] T010 Make individual and deposit-all ATM paths mutate wallet/cache only after success.
- [x] T011 Make ATM withdrawals distinguish insufficient funds from database failure.
- [x] T012 Route `SUB_BALANCE()` through authoritative aggregate withdrawal and preserve change behavior.
- [x] T013 Route cash boon and online ship-insurance rewards through checked deltas without false success.

## Tests And Completion

- [x] T014 Add focused stale-cache, transaction-failure, insufficient-funds, caller-order, and online-publication contracts.
- [x] T015 Run focused/nearest regressions, formatting, warning-as-error build, and full suite.
- [x] T016 Complete review, repair findings, and validate the session.

## Completion Checklist

- [x] All 16 tasks complete
- [x] No outstanding blocker or unresolved failure
- [x] Ready for `creview`
