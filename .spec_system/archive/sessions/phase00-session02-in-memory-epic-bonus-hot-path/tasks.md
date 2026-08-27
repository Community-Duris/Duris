# Task Checklist

**Session ID**: `phase00-session02-in-memory-epic-bonus-hot-path`  
**Total Tasks**: 18  
**Work Window**: Complete bounded lifecycle from login hydration through selection, award, expiry, all active reads, and local runtime proof.  
**Created**: 2026-08-27

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup (2 tasks)

- [x] T001 [S0002] Re-run deterministic analysis, confirm Session 01 completion and Session 02 selection, record the planning HEAD, and inventory every `get_epic_bonus()` caller plus canonical player-load and award paths (`.spec_system/scripts/analyze-project.sh`, `src/epic_bonus.c`, `src/sql_player.c`, `src/epic.c`)
- [x] T002 [S0002] Confirm shipped rolling-window and cap properties, existing `epic_bonus`/`epic_gain` schema/indexes, date predicate semantics, and local-only database mode without printing credentials or executing migrations (`lib/duris.properties`, `migrations/bootstrap_multithread_safe.sql`, `.env`)

---

## Foundation (3 tasks)

- [x] T003 [S0002] Define trivial fixed-capacity hydration states, daily expiry buckets, supported window bounds, saturating totals, and pure transition APIs with exhaustive status handling (`src/epic_bonus_state.h`)
- [x] T004 [S0002] Implement reset, unavailable, validated publish, same-expiry coalescing, award addition, local expiry, next-expiry maintenance, cap math, and clock-boundary behavior without allocation or external I/O (`src/epic_bonus_state.c`)
- [x] T005 [S0002] Embed the zero-safe state in `pc_only_data` and add the new translation unit to the existing C++20 build (`src/structs.h`, `src/Makefile`)

---

## Implementation (7 tasks)

- [x] T006 [S0002] Add one deterministic grouped hydration query for selection plus positive non-bottle rolling gains, validate all rows into temporary bounded buckets, and publish ready-none or unavailable state atomically on the game thread with explicit failure mapping (`src/epic_bonus.c`, `src/epic_bonus.h`)
- [x] T007 [S0002] Wire hydration after principal player status/PID load in the canonical database login path, preserving explicit nonfatal component behavior while preventing partial or lazy cache state (`src/sql_player.c`, `src/sql_player.h`)
- [x] T008 [S0002] Replace `get_epic_bonus_data()` and `get_epic_bonus()` database reads with player-owned selection and modifier snapshots that expire due buckets locally and return zero for NPC, uninitialized, unavailable, invalid-type, and mismatched-type cases (`src/epic_bonus.c`)
- [x] T009 [S0002] Gate bonus-selection success copy and cache reset on successful persistence, retain prior state on failure, validate the requested type, and avoid duplicate publication while the write is unresolved (`src/epic_bonus.c`)
- [x] T010 [S0002] Record the final positive non-bottle award into the rolling state after the existing persistence publication call, preserving calculation order so the selected epic modifier applies before the new contribution (`src/epic.c`, `src/epic_bonus.c`, `src/epic_bonus.h`)
- [x] T011 [S0002] Audit every regeneration, XP, shop, cargo, help, status, and award caller for the pure in-memory contract and remove any redundant data lookup without changing gameplay-visible modifier formulas (`src/limits.c`, `src/shop.c`, `src/ships/ship_shop.c`, `src/actinf.c`, `src/epic.c`)
- [x] T012 [S0002] Document the active-player memory authority, bounded grouped hydration, day-based expiry, explicit unavailable behavior, supported configuration bound, and unchanged Phase 02 durability boundary (`docs/DATABASE.md`)

---

## Testing (6 tasks)

- [x] T013 [S0002] [P] Build a standalone C++20 runtime harness covering ready-empty, unavailable, validated hydration, selection reset, bucket coalescing, multiple and exact expiries, caps, invalid inputs, saturation, and clock rollback (`tests/async/test_epic_bonus_state.py`)
- [x] T014 [S0002] [P] Add source-contract coverage that inventories every active caller, rejects database/Redis/filesystem/allocation/locking work in the read body, and proves load, selection-success, award-order, and bottle-exclusion integration (`tests/async/test_epic_bonus_hot_path.py`)
- [x] T015 [S0002] Run the new tests plus nearest epic, regeneration, XP, SQL observability, and player-load source/runtime regressions (`tests/async/test_epic_bonus_state.py`, `tests/async/test_epic_bonus_hot_path.py`, `tests/async/test_epic_save_guards.py`, `tests/async/test_persistence_observability.py`, `tests/async/test_sql_persistence_paths.py`)
- [x] T016 [S0002] Format touched C/C++ lines, verify changed-line formatting, and compile with the repository C++20 warning profile (`scripts/format.sh`, `src/Makefile`)
- [x] T017 [S0002] Confirm local mode without printing secrets, start the game on a non-production port, authenticate with the configured account, exercise bonus help and regeneration, and compare persistence query snapshots across ordinary reads (`scripts/start_mud.sh`, `.env`)
- [x] T018 [S0002] Run `make test-all`, `git diff --check`, and a byte-level ASCII/LF scan of every created or modified text file (`Makefile`, `.spec_system/specs/phase00-session02-in-memory-epic-bonus-hot-path/spec.md`, `.spec_system/specs/phase00-session02-in-memory-epic-bonus-hot-path/tasks.md`)

---

## Completion Checklist

- [x] All tasks marked `[x]`
- [x] All tests and checks passing
- [x] All files ASCII-encoded with LF line endings
- [x] implementation-notes.md updated
- [x] Ready for `creview` (next step in the implement -> creview -> validate sequence)

---

## Next Steps

Run the `implement` workflow step.
