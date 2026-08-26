# Task Checklist

**Session ID**: `phase00-session06-redis-failure-and-recovery-containment`
**Total Tasks**: 16
**Created**: 2026-08-27

---

## Inventory And Design

- [x] T001 Confirm Session 06 selection, clean base `fc469b0f`, and local/development context.
- [x] T002 Inventory all Redis contexts, 52 synchronous commands, dirty state transitions, both child lifecycles, and floor-delta flow.
- [x] T003 Define bounded context, local retry, merge-before-rename, watchdog, and exact floor-ACK invariants.

## Redis And Dirty-State Containment

- [x] T004 Add bounded connect/command helpers and route primary, reconnect, donation, and world-child contexts through them.
- [x] T005 Route synchronous commands through the guarded wrapper with categorical failure handling.
- [x] T006 Preserve configured-enabled/degraded status and local dirty membership across unavailable Redis.
- [x] T007 Remove synchronous SQL fallbacks from dirty mutation and fork failure.
- [x] T008 Recover boot/stale inflight membership and require merge-before-rename without overwriting active members.
- [x] T009 Restore inflight state on every pre-fork and child failure edge.

## Child And Floor-Delta Safety

- [x] T010 Add dirty-save child deadline, kill/reap handling, and exit-status ACK semantics.
- [x] T011 Add world-snapshot child deadline, kill/reap handling, and exit-status ACK semantics.
- [x] T012 Require every world snapshot Redis write to succeed before child success.
- [x] T013 Gate floor-delta clearing on exact snapshot success and retain newer local deltas.

## Tests And Completion

- [x] T014 Add focused Redis-disabled, null-context, timeout, reconnect, fork/child failure, stale-inflight, and floor-ACK contracts.
- [x] T015 Run focused/nearest regressions, formatting, warning-as-error build, and full suite.
- [x] T016 Complete review, repair findings, and validate the session.

## Completion Checklist

- [x] All 16 tasks complete
- [x] No outstanding blocker or unresolved failure
- [x] Ready for `creview`
