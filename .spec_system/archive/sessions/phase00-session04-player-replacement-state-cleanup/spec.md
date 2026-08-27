# Session Specification

**Session ID**: `phase00-session04-player-replacement-state-cleanup`
**Phase**: 00 - Correctness and Immediate Lag Removal
**Status**: Not Started
**Created**: 2026-08-27
**Base Commit**: `2689ed9d17521d8b2a563a645daf858f000e8f5c`
**Work Window**: One transaction-scoped replacement-row correction for player timers, undead spell slots, forged-item knowledge, and granted commands, ending with save-clear-save-reload and rollback evidence for all four components.

---

## 1. Session Overview

`sql_save_player_status()` already deletes languages and introductions before inserting their current non-zero sets. The next four array-backed components only issue `REPLACE` statements for current entries. When a timer, undead slot, forged-item entry, or granted command is removed in memory, no statement removes the old row, so the next login loads it again.

All of these writes already execute inside the status save's transaction, which is nested under the full player-save transaction when appropriate. This session extends the established delete-then-insert pattern and preserves existing rollback ownership.

## 2. Objectives

1. Delete each affected player's previous component set before inserting its current set.
2. Propagate every delete, batch-build, and insert failure to the transaction owner.
3. Prove non-zero round trips, clear/revoke persistence, and rollback preservation for all four tables.
4. Make no schema, migration, production-data, or unrelated player-load change.

## 3. Prerequisites

- [x] Session 03 durable-save failure semantics are validated and published.
- [x] The configured environment is local/development; database write tests will use a new isolated disposable schema/container only.

## 4. Scope

### In Scope

- Add checked `sql_delete_player_subtable()` calls for `player_timers`, `player_undead_slots`, `player_forged_items`, and `player_granted_cmds` immediately before their replacement batches.
- Preserve languages/intros behavior, current bounds, zero filtering, batch limits, and transaction ownership.
- Verify empty current sets still execute the delete and issue no empty insert.
- Verify a failure after delete rolls back the previous durable set when the caller owns the transaction.
- Add focused source contracts and an isolated MySQL replacement/rollback regression.
- Document these four tables as full replacement components.

### Outside This Work Window

- Dirty masks, per-component revisions, immutable save DTOs, or worker acknowledgements from Phase 01.
- Login batching and consistent snapshots from Phase 03.
- Replacement rewrites for any other player subtable.
- Schema/index/migration changes.

## 5. Technical Approach

Use the same checked helper and failure shape as languages and introductions. For each component, delete by numeric PID inside the active transaction, free the batch buffer on failure, roll back only when `sql_save_player_status()` owns the transaction, and return false. When called by `sql_save_player()`, false propagates to its outer rollback. Build and run the existing `REPLACE` batch only when current non-zero values exist.

The source regression will isolate each component block and assert delete-before-build-before-insert ordering, checked failure returns, and unchanged bounds. A disposable MySQL 8 container will create only the four minimal fixture tables, execute value/replacement/clear cycles for all four components, and force an insert failure after deletion to prove rollback retains the previous durable rows. It will never connect to the configured Duris database.

## 6. Deliverables

### Files To Create

| File | Purpose |
|------|---------|
| `tests/async/test_player_replacement_state.py` | Source contracts for all four delete/replace blocks and transaction failure propagation |
| `tests/async/run_player_replacement_state_mysql.sh` | Disposable MySQL round-trip, clear/revoke, and rollback regression |

### Files To Modify

| File | Changes |
|------|---------|
| `src/sql_player.c` | Add checked transaction-scoped deletes before timers, undead slots, forged items, and granted commands |
| `docs/DATABASE.md` | Document full-replacement semantics and transaction rollback guarantee |

## 7. Success Criteria

### Functional

- [ ] Clearing every timer, undead slot, forged-item entry, or granted command removes the durable row.
- [ ] Current non-zero values for every component still round-trip with the same index/value semantics.
- [ ] Empty sets delete stale rows without issuing invalid empty inserts.
- [ ] Delete or insert failure returns false and the transaction owner rolls back the whole replacement.
- [ ] Language and introduction replacement behavior is unchanged.

### Testing And Quality

- [ ] Focused source contracts cover all four tables, ordering, bounds, empty sets, and ownership-aware failure propagation.
- [ ] Isolated MySQL evidence covers value, replacement, clear/revoke, and forced rollback preservation for all four tables.
- [ ] Formatting, C++20 warning-as-error build, nearest persistence regressions, and `make test-all` pass.
- [ ] All created or modified files are ASCII with Unix LF endings and final newlines.

### Non-Functional

- [ ] No schema, migration, dependency, new query input, or production database operation is introduced.
- [ ] Query count increases by exactly one bounded PID delete per affected component during a full status save.
- [ ] Existing transaction nesting and commit ownership remain unchanged.

## 8. Risks And Resolutions

- **Partial replacement risk**: a delete followed by failed insert must not commit empty state. Keep both statements in the existing transaction and return false through the current owner.
- **Empty-set risk**: deletion must run even when no replacement tuple exists; insertion remains conditional on `has_data`.
- **Direct-versus-nested save risk**: preserve `own_txn`; the status function rolls back its own transaction while the full-save caller rolls back its outer transaction.
- **Test isolation risk**: use a uniquely named disposable container/database and trap cleanup; never source repository credentials.

## 9. Testing Strategy

- Run the focused source contract first.
- Run the disposable MySQL script if Docker is available and record an explicit result.
- Run nearest SQL persistence, dirty-bit, commit-failure, and save-path regressions.
- Run formatter, server build, full suite, diff whitespace, and byte-oriented encoding scans.

## 10. Dependencies

- Depends on validated Session 03 failure semantics.
- Enables Phase 01 component revision work by making the current replacement baseline correct.

## Next Steps

Run the `implement` workflow step.
