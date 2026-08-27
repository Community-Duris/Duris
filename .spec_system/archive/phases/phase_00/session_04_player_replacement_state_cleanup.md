# Session 04: Player Replacement State Cleanup

**Session ID**: `phase00-session04-player-replacement-state-cleanup`
**Status**: Complete
**Work Window**: The replacement-row portion of one player-save transaction, verified by
save-clear-save-reload behavior for every affected component.

---

## Objective

Prevent removed timers, undead spell slots, forged-item knowledge, and granted commands
from reappearing after relog by making replacement persistence delete obsolete rows in
the same transaction that inserts the current set.

---

## Scope

### In Scope (MVP)

- Add transaction-scoped deletes or precise cleared-entry deletes for
  `player_timers`, `player_undead_slots`, `player_forged_items`, and
  `player_granted_cmds` before current values are inserted.
- Preserve existing language and introduction replacement behavior and nearby legacy
  style.
- Propagate delete or insert failure so the enclosing player save rolls back instead of
  committing a partial replacement set.
- Add regressions that persist a value, clear or revoke it, save again, and reload.

### Out of Scope

- Phase 01 component dirty masks or set-based rewrites of every player subtable.
- Login batching and consistent-snapshot work assigned to Phase 03.
- Schema changes unrelated to replacement-row correctness.

---

## Prerequisites

- [x] Session 03 durable-save failure semantics are validated.
- [x] Database behavior tests target only an isolated development schema.

---

## Deliverables

1. Correct delete-and-replace statements in `src/sql_player.c` within the existing
   player-save transaction.
2. Focused source-contract and isolated database regressions under `tests/async/`.
3. Error propagation that retains the previous durable replacement set on failure.

---

## Success Criteria

- [x] Clearing each affected value removes its durable row and reload does not revive
      it.
- [x] Current non-zero values still round-trip correctly.
- [x] Any delete or insert failure rolls back the player-save transaction.
- [x] No migration or write test is run against production.
- [x] Focused regressions, formatting checks, and `make -C src` pass.

---

## Completion Summary

Timers, undead spell slots, forged-item knowledge, and granted commands now delete
their previous PID-scoped rows inside the active save transaction before inserting the
current non-zero set. Empty sets therefore persist clears and revocations. Checked
delete and insert failures propagate to the direct or enclosing rollback owner.

A disposable MySQL regression proves replacement, complete clear, forced insert
rollback for all four tables, and forced delete failure without reading repository
credentials. The warning-as-error build, 171/171 Python regressions, signal-handler
checks, formatting, and review-surface scans pass.
