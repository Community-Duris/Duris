# Task Checklist

- [x] T001 [S0301] Inventory account selection, legacy restore, copyover, and component loaders (`src/`).
- [x] T002 [S0301] Define bounded request, result, component, failure, and metric contracts (`src/player_load_repository.h`, `src/player_load_pipeline.h`).
- [x] T003 [S0301] Implement the consistent-snapshot repository on one pooled connection (`src/player_load_repository.c`).
- [x] T004 [S0301] Implement bounded single-worker admission, cancellation, and exact completions (`src/player_load_pipeline.c`).
- [x] T005 [S0301] Implement game-thread snapshot materialization and revision hydration (`src/player_load_materialize.c`).
- [x] T006 [S0301] Integrate account selection, legacy login, copyover, disconnect, shutdown, and diagnostics (`src/account.c`, `src/nanny.c`, `src/copyover.c`, `src/comm.c`, `src/actinf.c`).
- [x] T007 [S0301] Add focused repository, lifecycle, stale, timeout, capacity, and source tests (`tests/async/`).
- [x] T008 [S0301] Run isolated MySQL, format, build, security, and full implementation gates (`tests/async/`, `src/`).
- [x] T009 [S0301] Record final implementation evidence and hand the session to creview (`.spec_system/specs/phase03-session01-consistent-player-load-transaction/`).

## Completion Checklist

- [x] Every task has exact verification evidence in `implementation-notes.md`.
- [x] All session success criteria are implemented.
- [x] No external blocker remains.
