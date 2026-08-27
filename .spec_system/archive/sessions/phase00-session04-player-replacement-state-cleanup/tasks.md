# Task Checklist

**Session ID**: `phase00-session04-player-replacement-state-cleanup`
**Total Tasks**: 15
**Created**: 2026-08-27

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session reference.

## Setup And Inventory

- [x] T001 [S0004] Re-run the analyzer, record base `2689ed9d`, confirm Session 04 selection, and verify local/development mode without printing environment values.
- [x] T002 [S0004] Inventory save/load fields, bounds, table keys, zero semantics, helper validation, and direct/nested transaction ownership for all four components.
- [x] T003 [S0004] Confirm isolated database-test strategy and that no schema or migration edit is required.

## Replacement Implementation

- [x] T004 [S0004] Add checked transaction-scoped deletion of prior `player_timers` rows before rebuilding current timers.
- [x] T005 [S0004] Add checked transaction-scoped deletion of prior `player_undead_slots` rows before rebuilding current slots.
- [x] T006 [S0004] Add checked transaction-scoped deletion of prior `player_forged_items` rows before rebuilding current knowledge.
- [x] T007 [S0004] Add checked transaction-scoped deletion of prior `player_granted_cmds` rows before rebuilding current grants.
- [x] T008 [S0004] Preserve zero/empty-set behavior, array bounds, batch overflow handling, and languages/intros behavior.
- [x] T009 [S0004] Verify every delete/build/insert failure frees temporary state, returns false, and reaches the correct direct or outer rollback owner.

## Documentation And Tests

- [x] T010 [S0004] Add source contracts for delete-before-insert ordering, all four table names, empty-set deletion, bounds, and transaction failure propagation.
- [x] T011 [S0004] [P] Add a disposable MySQL regression for initial value, replacement value, complete clear/revoke, and reload-equivalent reads across all four tables.
- [x] T012 [S0004] [P] Add forced post-delete insert failure cases proving rollback preserves the previous durable set for all four tables.
- [x] T013 [S0004] Document the full-replacement and rollback contract in `docs/DATABASE.md`.
- [x] T014 [S0004] Run focused tests, disposable MySQL evidence, nearest persistence regressions, formatting, and `make -C src`.
- [x] T015 [S0004] Run `make test-all`, `git diff --check`, analyzer, and ASCII/LF/final-newline scans across the complete session surface.

## Completion Checklist

- [x] All 15 tasks complete
- [x] No outstanding blocker or unresolved test failure
- [x] Ready for `creview`

## Next Steps

Run the `implement` workflow step.
