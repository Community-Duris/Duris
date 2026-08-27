# Session Specification

**Session ID**: `phase03-session04-set-based-pvp-and-epic-task-reads`
**Phase**: 03 - Load Path, Schema, and Retention
**Status**: Not Started
**Created**: 2026-08-27
**Base Commit**: `6ba79b1c946bbb08f4f42e4b569a660440bc890b`
**Work Window**: One coherent gameplay-read boundary: hydrate bounded recent-PvP and
epic-completion state during login, maintain it through committed mutations, and make
heaven-time and epic-task callbacks entirely in-memory with end-to-end equivalence tests.

---

## 1. Session Overview

Session 03 completed bounded player and pet graph hydration, but two gameplay callbacks
still perform synchronous MySQL work. `setHeavenTime` reads the latest 20 victim events
and then queries each event separately, while `epic_random_task_zone` performs a
full-history exclusion and `ORDER BY RAND()` for every new task.

This session loads each player's bounded recent-death timestamps and distinct completed
zone identities set-wise with the existing consistent login transaction. A small fixed
game-thread state keeps those histories current after accepted/committed PvP and epic
operations. A separately bounded task-zone catalog refreshes outside the task callback;
selection then uses uniform in-memory reservoir sampling.

## 2. Objectives

1. Replace the latest-20 PvP N+1 with one joined ordered read and preserve the exact
   base, one-hour, doubling, and extreme-duration heaven-time rules.
2. Load legacy plus Phase 02 epic zone completion identities in one set-based read and
   keep the live set aligned only with durable award outcomes.
3. Build and safely refresh a bounded task-zone catalog outside gameplay callbacks,
   retaining the last valid catalog on refresh failure.
4. Remove MySQL and Redis calls plus `ORDER BY RAND()` from task selection and prove
   uniform, eligible-only behavior and constant query counts.

## 3. Prerequisites

### Required Sessions

- [x] `phase02-session09-pvp-and-combat-outcome-batching` - durable PvP event identity
  and exact committed completion.
- [x] `phase02-session03-epic-ledger-and-balance-transactions` - authoritative epic
  ledger and committed publication.
- [x] `phase03-session01-consistent-player-load-transaction` - bounded read snapshot.
- [x] `phase03-session03-batched-pet-graph-hydration` - complete normal-login cutover.

### Required Tools Or Knowledge

- C++20, existing player-load DTO/materializer, critical-command completion callbacks,
  MySQL temporary-table harness, and legacy heaven/task selection rules.

### Environment Requirements

- Database tests use only the configured local development database with
  connection-local temporary tables. No production migration or operational script.

## 4. Scope

### In Scope (MVP)

- One latest-20 `pkill_info`/`pkill_event` join returning validated event timestamps.
- One distinct union over legacy `epic_gain` and authoritative `epic_ledger` zone
  completions, with explicit row, value, byte, query, and allocation bounds.
- A fixed pointer-free per-player gameplay-read state published during materialization,
  updated provisionally for accepted PvP work with rollback on rejection, and updated
  for zone completion only after a committed epic award.
- A bounded task-zone catalog loaded outside the task callback with deterministic
  ordering, valid world-zone identities, atomic replacement, and last-good retention.
- In-memory heaven-time calculation and uniform eligible task selection without
  database/Redis I/O, null-sensitive SQL exclusion, or random database sort.
- Focused pure-state, source-contract, repository MySQL, callback, boundary, failure,
  selection-distribution, copyover, stale, and duplicate-completion regressions.

### Outside This Work Window

- PvP/epic write transaction redesign and reward changes - completed in Phase 02.
- Epic trophy payout history and broad report caching - separate consumers that do not
  share the task-selection acceptance boundary.
- New indexes - Session 05 measures representative plans and write cost first.
- General maintenance scheduling - Session 06 owns periodic bounded job orchestration.

## 5. Technical Approach

### Architecture

Extend the player-load result with a separate read-state mask rather than adding
read-only history bits to the revisioned save-component mask. The worker returns at most
20 ordered death timestamps and a bounded sorted unique completion set. The game thread
publishes them into fixed storage on `pc_only_data`. Copyover also hydrates this state
because it reconstructs a fresh character and has no serialized live read-state payload.

The task-zone catalog owns a validated sorted vector and swaps only a complete candidate
into service. Initial or refresh failure makes task-zone selection return no zone (the
existing spill-blood fallback) or retains the last valid catalog. Selection scans the
bounded catalog once and uses reservoir sampling over eligible entries, avoiding both
allocation and modulo/index mistakes.

### Design Patterns

- **Snapshot hydration**: External rows cross threads only as bounded pointer-free DTOs.
- **Last-good atomic refresh**: Catalog failures never publish partial eligibility.
- **Provisional mutation with compensation**: Accepted PvP work becomes visible for a
  subsequent death and is removed if the durable completion rejects it.
- **Commit-driven derived state**: Epic completion membership changes only after ACK.

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/gameplay_read_state.h` | Fixed recent-death/completion state and APIs | ~100 |
| `src/gameplay_read_state.c` | Validation, window counting, provisional rollback, membership | ~260 |
| `src/epic_task_catalog.h` | Bounded catalog refresh/publication/selection API | ~70 |
| `src/epic_task_catalog.c` | Last-good MySQL refresh and uniform in-memory selection | ~260 |
| `tests/async/test_set_based_gameplay_reads.py` | Runtime, source, bounds, and distribution regressions | ~500 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `src/player_load_repository.h`, `src/player_load_repository.c` | Add read mask, two set-based reads, DTO bounds, and 22-query contract | ~220 |
| `src/player_load_materialize.c`, `src/structs.h` | Publish fixed gameplay read state without pointers | ~60 |
| `src/fight.c`, `src/combat_outcome_command.h` | Replace heaven SQL and compensate provisional recent-death state | ~100 |
| `src/epic.c`, `src/epic.h` | Replace random SQL and publish committed completion membership | ~80 |
| `src/comm.c`, `src/Makefile`, `src/copyover.c` | Refresh catalog after zone DB bootstrap, link modules, and preserve copyover state | ~30 |
| `tests/async/player_load_repository_mysql_harness.cpp`, `tests/async/test_player_load_pipeline.py` | Add isolated history fixtures and read-mask/query contracts | ~180 |

## 7. Success Criteria

### Functional Requirements

- [x] One latest-20 join supplies heaven-time state; `setHeavenTime` performs no
  external I/O and preserves zero, 3599/3600-second, doubling, and extreme rules.
- [x] Accepted PvP work is counted by a subsequent death and exact rejection removes
  only its provisional timestamp; committed and duplicate completions remain idempotent.
- [x] Completion membership loads from legacy and ledger rows in one set-wise read and
  changes live state only after a committed zone award.
- [x] Task selection performs no database or Redis operation, excludes completed and
  invalid zones, and is uniform within a deterministic tested tolerance.
- [x] Catalog refresh is bounded and atomic; failure retains the prior valid catalog,
  while unavailable/empty/all-complete states use the existing safe fallback.
- [x] Normal login and copyover both require the new read state before a freshly
  reconstructed character can be published.

### Testing Requirements

- [x] Focused pure-state, heaven-time, task-selection, callback, and source tests pass.
- [x] Guarded local MySQL fixtures prove zero/many, latest-20, boundary timestamps,
  legacy/ledger duplicates, malformed/bound, and exact 22-query behavior.
- [x] Existing player-load, combat, epic, copyover, and repository-wide tests pass.

### Non-Functional Requirements

- [x] Gameplay callbacks issue zero database, Redis, filesystem, or worker operations.
- [x] Per-death work is O(20); selection is O(Z) with bounded catalog/state storage and
  no callback allocation.
- [x] Logs contain stable query sites and aggregate outcomes only, never player identity
  or row values.

### Quality Gates

- [x] All deliverables are ASCII with Unix LF endings.
- [x] Changed C/C++ lines pass `.clang-format`.
- [x] `make -C src` and `make test-all` pass warning-clean.

## 8. Implementation Notes

### Working Assumptions

- Completion identity is zone number, because both legacy `epic_gain.type_id` and Phase
  02 `epic_ledger.reason_id` use it for `EPIC_ZONE`/`zone_award`.
- Task-zone catalog data remains database-owned; boot refresh follows `update_zone_db`,
  and Session 06 may later schedule the already-exposed bounded refresh.
- Completion time is sufficient for the provisional recent-death timestamp because the
  old write used database `NOW()` immediately after heaven calculation; rejection is
  compensated and restart reloads the durable truth.

### Conflict Resolutions

- The save-component mask describes revisioned checkpoint writes, so read-only history
  gets a separate load-read mask rather than extending `PLAYER_CHECKPOINT_COMPONENT_ALL`.
- `get_epic_zone_trophy` shares tables but not the task-selection result shape or
  bounded-history semantics; it remains outside this session rather than silently
  changing trophy payouts.
- No index is added from local fixtures; Session 05 owns clone-backed plan evidence.

### Key Considerations

- Failure of required per-player history rejects normal login rather than converting
  missing completion data into a duplicate-task opportunity.
- Catalog failure uses no-zone fallback and never replaces a valid prior catalog.
- Exact request/result alignment and stale completion guards remain mandatory.

### Potential Challenges

- Async PvP rejection after provisional publication: carry one exact timestamp through
  the pending payload and remove one matching entry on rejection.
- Uniform selection without allocation: validate reservoir sampling with deterministic
  repeated draws and inclusive RNG boundaries.
- Legacy/ledger overlap: SQL and materialization both enforce sorted uniqueness.

### Relevant Considerations

- [P00] **External I/O can stop the simulation**: Both scoped callbacks lose direct DB
  access; catalog refresh remains outside gameplay.
- [P00] **Do not tune from tiny local plans**: Session 04 proves query shape only;
  Session 05 owns indexes.
- [P00] **Trace code before trusting prose**: Tests inventory exact callback bodies and
  query sites after cutover.

### Behavioral Quality Focus

Checklist active: Yes
Top behavioral risks for this session:

- Missing history must fail closed rather than permit duplicate eligible zones.
- Provisional recent-death state must compensate rejection without removing other rows.
- Refresh and selection must never expose partial, stale-invalid, or unbounded state.

## 9. Testing Strategy

### Unit Tests

- Pure fixed-state tests for count windows, exact boundaries, saturation, duplicates,
  provisional add/remove, completion membership, catalog swap, and reservoir sampling.

### Integration Tests

- Compile real modules with controlled callbacks and extend the guarded temporary-table
  MySQL harness for the two history reads and 22-query snapshot.

### Runtime Verification

- Source contracts prove no `qry`, Redis, `ORDER BY RAND()`, or worker submission in
  `setHeavenTime` and `epic_random_task_zone`; build and complete regression gate pass.

### Edge Cases

- Zero and 20-plus deaths, 3599/3600 seconds, duplicate timestamps, overflow-safe
  doubling, no catalog, empty catalog, all complete, duplicate legacy/ledger zones,
  invalid world zone, allocation failure, rejected/stale/duplicate completion, and
  copyover re-entry.

## 10. Dependencies

### Other Sessions

- Depends on: Phase 02 Sessions 03 and 09; Phase 03 Sessions 01-03.
- Depended by: Phase 03 Sessions 05, 13, and 14.

## Next Steps

Run the `implement` workflow step to begin implementation.
