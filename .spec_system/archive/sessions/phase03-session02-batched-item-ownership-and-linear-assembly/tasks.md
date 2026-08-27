# Task Checklist

**Session ID**: `phase03-session02-batched-item-ownership-and-linear-assembly`
**Total Tasks**: 10
**Work Window**: Complete the authoritative player-inventory read and linear
all-or-nothing materialization boundary inside the Session 01 load workflow.
**Created**: 2026-08-27

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[S0302]` session ref;
`TNNN` task ID.

---

## Setup (1 task)

- [x] T001 [S0302] Reconcile active player ownership, owner revision, serialized item,
  affect, description, equipment, container, save, and login contracts and record the
  exact field/bijection rules (`src/sql_player.c`, `src/item_ownership_runtime.c`,
  `src/player_snapshot.h`, `migrations/bootstrap_multithread_safe.sql`).

---

## Foundation (2 tasks)

- [x] T002 [S0302] Extend the pointer-free player-load DTO and explicit query, item,
  metadata, graph, byte, and operation bounds with exhaustive custody states and no
  silent truncation (`src/player_load_repository.h`).
- [x] T003 [S0302] [P] Add the game-thread inventory materializer interface and staged
  cleanup/metrics contract for every acquired object and allocation (`src/player_load_items.h`,
  `src/player_load_items.c`).

---

## Implementation (4 tasks)

- [x] T004 [S0302] Implement three deterministic set-based snapshot queries for the
  exact player item/current-owner/owner-revision bijection, affects, and descriptions,
  with escaped/validated input, hard deadlines, bounded results, deterministic order,
  and explicit failure mapping (`src/player_load_repository.c`).
- [x] T005 [S0302] Implement pre-allocation graph and metadata validation plus O(N)
  indexed prototype construction, string/affect/description application, deepest-first
  container linking/weight calculation, equipment placement, ownership hydration, and
  failure cleanup without partial mutation (`src/player_load_items.c`,
  `src/item_ownership_runtime.c`, `src/item_ownership_runtime.h`).
- [x] T006 [S0302] Require equipment/inventory components during fresh-character
  materialization and discard every staged object on snapshot, authorization,
  allocation, prototype, graph, or ownership-runtime failure (`src/player_load_materialize.c`,
  `src/player_load_repository.h`).
- [x] T007 [S0302] Integrate the new module into the build and remove only the normal
  `rtype == 0` post-publication SQL item reload, preserving rent/crash branches and
  revalidation on every login (`src/Makefile`, `src/nanny.c`).

---

## Testing (3 tasks)

- [x] T008 [S0302] [P] Add focused synthetic runtime and source regressions for empty,
  flat, nested, equipped, large, mismatch, duplicate, cycle, missing-parent, invalid
  slot/prototype, over-limit, allocation-failure, cleanup, constant-query, and linear
  operation-count behavior (`tests/async/test_player_load_items.py`,
  `tests/async/test_player_load_pipeline.py`).
- [x] T009 [S0302] Extend the guarded local-development MySQL harness with exact item,
  current-owner, owner-revision, affect, description, mismatch, and bounds fixtures;
  clean every fixture and never target production (`tests/async/player_load_repository_mysql_harness.cpp`,
  `tests/async/run_player_load_repository_mysql.sh`).
- [x] T010 [S0302] Run focused item/pipeline/database regressions, changed-line format,
  C++20 build, security/privacy and behavioral spot-checks, `git diff --check`, and
  `make test-all`; record exact evidence and the `creview` handoff
  (`.spec_system/specs/phase03-session02-batched-item-ownership-and-linear-assembly/implementation-notes.md`).

---

## Completion Checklist

- [x] All tasks marked `[x]`.
- [x] All tests and checks passing.
- [x] All files ASCII-encoded with LF line endings.
- [x] `implementation-notes.md` updated.
- [x] Ready for `creview` (next step in the implement -> creview -> validate sequence).

---

## Next Steps

Run the `creview` workflow step.
