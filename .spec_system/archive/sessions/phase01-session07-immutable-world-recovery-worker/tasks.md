# Task Checklist

**Session ID**: `phase01-session07-immutable-world-recovery-worker`
**Total Tasks**: 18
**Created**: 2026-08-27

## Inventory And Design

- [x] T001 Confirm validated Session 06 base and Phase 00 prerequisites.
- [x] T002 Inventory fork, serializer, restore, floor-delta, event, and shutdown paths.
- [x] T003 Define framed generation, checksum, sequence, completeness, and bounds.
- [x] T004 Define capture stages, worker ownership, atomic publish, and delta boundary.

## Implementation

- [x] T005 Add immutable world generation and health types.
- [x] T006 Add bounded incremental mob and floor-object capture.
- [x] T007 Add bounded door and zone capture and completeness seal.
- [x] T008 Add long-lived bounded publisher worker and typed completions.
- [x] T009 Add Redis atomic generation publication.
- [x] T010 Add exact sequence acknowledgement and floor-delta boundary handling.
- [x] T011 Add schema/checksum/sequence/age/completeness validation.
- [x] T012 Route restore through the validated framed generation.
- [x] T013 Integrate request, pulse, init, shutdown, and process drain.
- [x] T014 Add independent redacted recovery health.

## Tests And Completion

- [x] T015 Add capture, bound, ownership, checksum, and stale-ACK regressions.
- [x] T016 Add outage, delta, shutdown, and restore contracts.
- [x] T017 Run focused, format, build, security, and full validation.
- [x] T018 Complete review, records, version, commit, and publication.
