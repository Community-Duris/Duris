# Task Checklist

**Session ID**: `phase03-session04-set-based-pvp-and-epic-task-reads`
**Total Tasks**: 11
**Work Window**: Hydrate and maintain the two bounded gameplay histories and task-zone
catalog needed to remove synchronous SQL from heaven-time and task selection.
**Created**: 2026-08-27

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[S0304]` session ref;
`TNNN` task ID.

---

## Setup (1 task)

- [x] T001 [S0304] Reconcile post-Phase-02 PvP event publication, epic award completion,
  login/copyover materialization, zone bootstrap, legacy history, and schema contracts
  (`src/fight.c`, `src/epic.c`, `src/combat_outcome_transaction.c`,
  `src/player_load_repository.c`, `src/comm.c`, `migrations/bootstrap_multithread_safe.sql`).

## Foundation (3 tasks)

- [x] T002 [S0304] Create bounded fixed per-player gameplay read state with validated
  atomic publication, exact recent-window counting, provisional add/remove, completion
  membership, duplicate handling, and no external calls (`src/gameplay_read_state.h`,
  `src/gameplay_read_state.c`, `src/structs.h`).
- [x] T003 [S0304] Extend player-load request/result contracts with a separate read-only
  component mask, latest-20 timestamps, sorted completion identities, explicit bounds,
  mandatory normal-login/copyover hydration, and exhaustive external-input failure mapping
  (`src/player_load_repository.h`, `src/copyover.c`).
- [x] T004 [S0304] Create a bounded task-zone catalog with deterministic validation,
  atomic last-good refresh, unavailable/empty states, uniform allocation-free reservoir
  selection, and explicit refresh failure handling (`src/epic_task_catalog.h`,
  `src/epic_task_catalog.c`).

## Implementation (4 tasks)

- [x] T005 [S0304] Add one ordered latest-20 PvP join and one set-wise legacy/ledger
  completion read to the consistent snapshot with validated timestamps/zone IDs,
  row/byte/deadline ceilings, stable query sites, and exact 22-query accounting
  (`src/player_load_repository.c`).
- [x] T006 [S0304] Publish the required read state with the fresh character and reject
  missing/misaligned state without partial world publication
  (`src/player_load_materialize.c`, `src/gameplay_read_state.c`).
- [x] T007 [S0304] Replace heaven-time SQL with fixed-state calculation and wire accepted
  PvP provisional state plus exact compensation on rejected completion, preserving the
  current-death exclusion and every duration boundary (`src/fight.c`,
  `src/combat_outcome_command.h`).
- [x] T008 [S0304] Refresh the task catalog after zone DB bootstrap, replace callback SQL
  with eligible in-memory selection, and publish zone completion membership only after
  committed epic awards; link both modules (`src/comm.c`, `src/epic.c`, `src/epic.h`,
  `src/Makefile`).

## Testing (3 tasks)

- [x] T009 [S0304] [P] Add compiled pure/runtime and source-contract regressions for
  recent-death boundaries, doubling/extreme rules, provisional compensation, completion
  dedupe, catalog refresh retention, no/all eligibility, distribution, invalid zones,
  callback I/O absence, copyover, stale/duplicate completion, and operation ceilings
  (`tests/async/test_set_based_gameplay_reads.py`).
- [x] T010 [S0304] Extend guarded local-development MySQL fixtures for zero/many and
  latest-20 deaths, exact boundary timestamps, legacy/ledger overlap, malformed/bound
  completion rows, and exact query count without persistent writes
  (`tests/async/player_load_repository_mysql_harness.cpp`,
  `tests/async/run_player_load_repository_mysql.sh`).
- [x] T011 [S0304] Run focused gameplay-read, player-load, combat, epic, copyover, and
  database tests; warning-as-error C++20 build; changed-line format; ASCII/LF, security,
  privacy, BQC, diff-hygiene, and `make test-all` gates; record exact evidence and the
  `creview` handoff (`.spec_system/specs/phase03-session04-set-based-pvp-and-epic-task-reads/implementation-notes.md`).

## Completion Checklist

- [x] All tasks marked `[x]`.
- [x] All tests and checks passing.
- [x] All files ASCII-encoded with LF line endings.
- [x] `implementation-notes.md` updated.
- [x] Ready for `creview`.

## Next Steps

Run the `implement` workflow step.
