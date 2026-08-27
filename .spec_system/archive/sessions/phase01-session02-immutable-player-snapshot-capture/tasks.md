# Task Checklist

**Session ID**: `phase01-session02-immutable-player-snapshot-capture`
**Total Tasks**: 16
**Created**: 2026-08-27

## Inventory And Design

- [x] T001 Confirm clean Session 01 base `7d3b7996` and validated revision contract.
- [x] T002 Inventory current status, replacement, affect, item, pet, shape, trophy, and recipe save semantics.
- [x] T003 Define pointer-free DTO rows, local relationship indices, and Phase 02 boundaries.
- [x] T004 Define byte/row/object/depth/string limits and failure classes.

## DTO And Capture

- [x] T005 Implement immutable metadata and status/replacement DTO values.
- [x] T006 Capture skills, affects, languages, introductions, timers, slots, forged items, and grants.
- [x] T007 Capture equipment/inventory/container trees with local parent indices and item subrows.
- [x] T008 Capture crash-save pets and nested pet items without live pointers.
- [x] T009 Capture shapes, trophies, and explicit recipe compatibility state.
- [x] T010 Add size accounting, graph-cycle/depth checks, allocation classification, and atomic publication.

## Tests And Completion

- [x] T011 Add compile-time/source proof that DTOs contain no engine pointers.
- [x] T012 Add runtime metadata, bounds, empty-set, nesting, and mutation-isolation tests.
- [x] T013 Add equivalence contracts against current component fields and filters.
- [x] T014 Prove capture has no active queue/SQL/save-route cutover.
- [x] T015 Run focused tests, formatting, warning-as-error build, and full suite.
- [x] T016 Complete review, validation, records/version, commit, and publication.

## Completion Checklist

- [x] All 16 tasks complete
- [x] No outstanding blocker or unresolved failure
- [x] Ready for `creview`
