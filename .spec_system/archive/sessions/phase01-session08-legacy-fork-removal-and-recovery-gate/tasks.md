# Task Checklist

**Session ID**: `phase01-session08-legacy-fork-removal-and-recovery-gate`
**Total Tasks**: 18
**Created**: 2026-08-27

## Inventory And Design

- [x] T001 Confirm validated Session 07 base and Phase 00 prerequisites.
- [x] T002 Inventory persistence fork, child, dirty-set, sync fallback, and legacy codec paths.
- [x] T003 Define persistence-only removal boundary and retain unrelated DNS child handling.
- [x] T004 Define deterministic fault, ordering, bounded-load, and documentation gates.

## Implementation

- [x] T005 Delete Redis persistence child polling, termination, timeout, and state.
- [x] T006 Delete legacy synchronous and JSON world save/load implementations.
- [x] T007 Remove Redis dirty-player key lifecycle and administrative controls.
- [x] T008 Correct world recovery diagnostics to use validated generation state.
- [x] T009 Add final player/world route and ownership source contracts.
- [x] T010 Add stale, duplicate, ambiguous, outage, crash, journal, and ACK fault gates.
- [x] T011 Add deterministic 25/50/100/200-client worker load gate and metrics.
- [x] T012 Repair issues found by the complete Phase 01 gate.
- [x] T013 Update architecture, database, configuration, testing, and runbook docs.

## Tests And Completion

- [x] T014 Run focused fault, route, ownership, and load tests.
- [x] T015 Run formatting, build, and security checks.
- [x] T016 Run the full repository validation gate.
- [x] T017 Complete review, security, validation, and implementation records.
- [x] T018 Update PRD/state/version, commit, and publish.
