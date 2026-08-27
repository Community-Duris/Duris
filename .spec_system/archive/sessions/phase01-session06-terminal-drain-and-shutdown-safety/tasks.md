# Task Checklist

**Session ID**: `phase01-session06-terminal-drain-and-shutdown-safety`
**Total Tasks**: 18
**Created**: 2026-08-27

## Inventory And Design

- [x] T001 Confirm validated Session 05 cutover base.
- [x] T002 Inventory terminal, extraction, copyover, shutdown, and fallback routes.
- [x] T003 Define exact terminal fence, deadline, and allowed durability outcomes.
- [x] T004 Define quiescence/drain ownership and failure semantics.

## Terminal And Drain

- [x] T005 Implement fixed-capacity terminal fence state and health.
- [x] T006 Implement one-revision promotion/capture and duplicate avoidance.
- [x] T007 Record exact journal durability and database acknowledgement.
- [x] T008 Implement bounded terminal wait and typed result.
- [x] T009 Gate mutation and stale completion release while fenced.
- [x] T010 Route the shared terminal helper to the coordinator.
- [x] T011 Preserve all extraction callers' fail-closed behavior.
- [x] T012 Implement coordinator quiesce and bounded drain.
- [x] T013 Integrate copyover and shutdown drain gates.
- [x] T014 Disable new player flat fallback writes and document inventory-only legacy files.

## Tests And Completion

- [x] T015 Add terminal fence/outcome/deadline regressions.
- [x] T016 Add caller, drain, copyover, shutdown, and fallback contracts.
- [x] T017 Run focused, format, build, security, and full-suite validation.
- [x] T018 Complete review, records, version, commit, and publication.

## Completion Checklist

- [x] All 18 tasks complete
- [x] No unresolved failure
- [x] Ready for `creview`
