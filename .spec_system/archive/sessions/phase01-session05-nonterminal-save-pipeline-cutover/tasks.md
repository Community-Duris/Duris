# Task Checklist

**Session ID**: `phase01-session05-nonterminal-save-pipeline-cutover`
**Total Tasks**: 18
**Created**: 2026-08-27

## Inventory And Design

- [x] T001 Confirm clean validated Session 04 base and journal/worker boundaries.
- [x] T002 Inventory dirty, autosave, manual, deferred, direct, startup, pulse, and fork routes.
- [x] T003 Define bounded dispatch ownership and fail-closed checkpoint outcomes.
- [x] T004 Define nonterminal compatibility and Phase 02 critical-domain boundaries.

## Coordinator And Cutover

- [x] T005 Implement bounded in-memory dispatch coordinator and health.
- [x] T006 Implement async journal-before-worker submission and retry retention.
- [x] T007 Implement startup replay, worker lifecycle, completion pulse, and shutdown hooks.
- [x] T008 Implement component mark and unchanged-player checkpoint APIs.
- [x] T009 Convert Redis dirty mark/count/flush compatibility to local revision state.
- [x] T010 Disable Redis dirty-save event and fork scheduling.
- [x] T011 Cut over ordinary `writeCharacter` before legacy save side effects.
- [x] T012 Cut over nonterminal `do_save_silent` before host-file and duplicate full save work.
- [x] T013 Preserve locker, terminal, and Phase 02 compatibility paths explicitly.
- [x] T014 Add redacted coordinator diagnostics and operational documentation.

## Tests And Completion

- [x] T015 Add queue/coalescing/unchanged/overload runtime regressions.
- [x] T016 Add call-site inventory, no-I/O hot-path, Redis/fork, and lifecycle contracts.
- [x] T017 Run focused tests, format, warning-as-error build, security scan, and full suite.
- [x] T018 Complete review, validation, records/version, commit, and publication.

## Completion Checklist

- [x] All 18 tasks complete
- [x] No outstanding blocker or unresolved failure
- [x] Ready for `creview`
