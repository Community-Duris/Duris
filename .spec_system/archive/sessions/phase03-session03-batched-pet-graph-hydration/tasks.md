# Task Checklist

**Session ID**: `phase03-session03-batched-pet-graph-hydration`
**Total Tasks**: 10
**Work Window**: Complete the bounded pet recovery read, staged aggregate, and
non-destructive publication boundary inside the existing consistent player load.
**Created**: 2026-08-27

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[S0303]` session ref;
`TNNN` task ID.

---

## Setup (1 task)

- [x] T001 [S0303] Reconcile pet checkpoint capture/write, load/delete, NPC/follower,
  item ownership, descriptor publication, reconnect, copyover, and schema contracts and
  record exact aggregate/recovery rules (`src/player_snapshot_capture.c`,
  `src/player_snapshot_repository.c`, `src/sql_player.c`, `src/nanny.c`,
  `migrations/bootstrap_multithread_safe.sql`).

---

## Foundation (2 tasks)

- [x] T002 [S0303] Extend pointer-free load DTOs with aligned pet/item identities and
  explicit pet, item, graph, metadata, query, byte, and operation bounds with exhaustive
  external-input error mapping (`src/player_load_repository.h`,
  `src/player_load_repository.c`).
- [x] T003 [S0303] Refactor item graph materialization to stage unpublished roots and
  ownership entries for player or pet characters without independent global ownership
  publication, with cleanup on every exit (`src/player_load_items.h`,
  `src/player_load_items.c`).

---

## Implementation (4 tasks)

- [x] T004 [S0303] Add three deterministic set-based pet, pet-item/current-owner, and
  pet-metadata reads inside the existing snapshot, with bounded rows/bytes, validated
  numeric/text input, deterministic ordering, combined player/pet custody bijection,
  deadlines, and explicit failure outcomes (`src/player_load_repository.c`).
- [x] T005 [S0303] Implement indexed pet validation, staged NPC/item construction,
  follower commit, exact cleanup, and post-entry room placement without external I/O or
  partial world mutation (`src/player_load_pets.h`, `src/player_load_pets.c`).
- [x] T006 [S0303] Stage player items and every pet graph, hydrate one combined
  ownership batch, then commit the character family; on any failure compensate all
  staged character/object/follower state (`src/player_load_materialize.c`,
  `src/player_load_items.c`, `src/player_load_pets.c`).
- [x] T007 [S0303] Integrate the module and fresh-login placement, explicitly exclude
  SQL pets from copyover, remove the legacy per-pet/delete-on-read login call, and
  preserve rent/crash messaging and non-SQL item behavior (`src/Makefile`,
  `src/copyover.c`, `src/nanny.c`).

---

## Testing (3 tasks)

- [x] T008 [S0303] [P] Add focused runtime/source regressions for zero/many pets,
  256-plus items, nested/equipped graphs, combined ownership, malformed/bounded input,
  staged cleanup, follower/room publication, retained recovery, copyover, stale and
  duplicate completion, query count, and linear operations
  (`tests/async/test_player_load_pets.py`, `tests/async/test_player_load_items.py`,
  `tests/async/test_player_load_pipeline.py`).
- [x] T009 [S0303] Extend the guarded local-development MySQL harness with
  connection-local pet/item/metadata/ownership fixtures for valid, empty, mismatch,
  cross-pet, bound, and retryable recovery cases, without persistent or production
  writes (`tests/async/player_load_repository_mysql_harness.cpp`,
  `tests/async/run_player_load_repository_mysql.sh`).
- [x] T010 [S0303] Run focused pet/item/pipeline/database regressions, changed-line
  format, warning-as-error C++20 build, ASCII/LF, security/privacy and behavioral
  checks, `git diff --check`, and `make test-all`; record exact evidence and the
  `creview` handoff (`.spec_system/specs/phase03-session03-batched-pet-graph-hydration/implementation-notes.md`).

---

## Completion Checklist

- [x] All tasks marked `[x]`.
- [x] All tests and checks passing.
- [x] All files ASCII-encoded with LF line endings.
- [x] `implementation-notes.md` updated.
- [x] Ready for `creview` (next step in the implement -> creview -> validate sequence).

---

## Next Steps

Run the `implement` workflow step.
