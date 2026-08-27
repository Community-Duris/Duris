# Task Checklist

**Session ID**: `phase00-session03-save-failure-retry-and-terminal-safety`
**Total Tasks**: 22
**Created**: 2026-08-27

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session reference.

## Setup And Inventory

- [x] T001 [S0003] Re-run deterministic analysis, record the exact base commit, and confirm Session 03 is the next Phase 00 candidate.
- [x] T002 [S0003] Inventory deferred-slot states and every destructive terminal player/locker caller, separating ordinary checkpoint failures from extraction/process-exit gates.
- [x] T003 [S0003] Record the existing save transaction, flat-fallback, event-lifetime, shutdown, copyover, locker, and offline-dummy assumptions without running migrations or destructive runtime actions.

## Deferred Save Foundation

- [x] T004 [S0003] Define bounded retry constants and explicit slot scheduling/retry metadata with saturating counters in `src/actoth.c`.
- [x] T005 [S0003] Centralize event scheduling so initial, failed, and newly coalesced unscheduled work always owns exactly one callback.
- [x] T006 [S0003] Implement capped exponential retry after save failure while retaining the newest type, level-dirty intent, reason, and age metadata.
- [x] T007 [S0003] Convert direct and global flush APIs to truthful boolean results; clear only success, re-arm live failures, and categorize missing characters.
- [x] T008 [S0003] Prevent a successful pending flush plus terminal caller from performing two equivalent full saves while preserving newer terminal rent semantics.

## Fail-Closed Terminal Persistence

- [x] T009 [S0003] Refactor `writeCharacter()` cleanup so every false result restores original equipment, affects, and reachable inventory; terminal extraction occurs only on SQL success.
- [x] T010 [S0003] Report SQL failure, fallback-record success, and fallback-record failure as distinct redacted outcomes without granting fallback durability.
- [x] T011 [S0003] Gate trusted quit, camp, inn rent, heaven/death, combat death, and idle-rent destructive transitions on required save success.
- [x] T012 [S0003] Make link-loss retain and retry failed state while preserving the linkdead character; keep ghost extraction fail closed.
- [x] T013 [S0003] Make copyover validate pending and final character saves before closing descriptors, stopping workers, replacing the process, or taking its fallback exit.
- [x] T014 [S0003] Add a pre-destruction shutdown/reboot character gate that cancels terminal mode and resumes the live server when required persistence fails.
- [x] T015 [S0003] Gate offline artifact dummy nuke/extraction and terminal artifact transition success on save completion.
- [x] T016 [S0003] Keep legacy deferred terminal locker characters and inventory live after save or commit failure; preserve existing async locker fail-closed behavior.

## Documentation And Tests

- [x] T017 [S0003] Document deferred retry/backoff, explicit flush results, terminal durability gates, fallback limits, and operator recovery in `docs/DATABASE.md` and `docs/RUNBOOK.md`.
- [x] T018 [S0003] [P] Add focused deferred retry tests for failure, capped backoff, rescheduling, coalescing, success clear, flush results, capacity, and missing-character behavior.
- [x] T019 [S0003] [P] Add terminal safety tests for failed inventory restoration and every destructive player, artifact, locker, copyover, and shutdown gate.
- [x] T020 [S0003] Update existing deferred-flush, copyover, locker-terminal, status, and nearest save-guard contracts for the new result semantics.
- [x] T021 [S0003] Run focused tests, changed-line formatting, `make -C src`, and safe local runtime smoke evidence where a non-destructive failure seam exists.
- [x] T022 [S0003] Run `make test-all`, `git diff --check`, and ASCII/LF/final-newline scans across the complete session surface.

## Completion Checklist

- [x] All 22 tasks complete
- [x] No outstanding blocker or unresolved test failure
- [x] Ready for `creview`
