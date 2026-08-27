# Session 06: Redis Failure and Recovery Containment

**Session ID**: `phase00-session06-redis-failure-and-recovery-containment`
**Status**: Complete
**Work Window**: One Redis outage and recovery domain spanning connection deadlines,
dirty-player preservation, temporary child supervision, and world floor-delta ACKs.

---

## Objective

Make Redis failure bounded and non-destructive: no unguarded or indefinitely blocking
commands, no synchronous full-player-save fallback in mutation callers, no abandoned
dirty inflight set, no unbounded temporary fork child, and no floor-delta deletion
before the matching world snapshot succeeds.

---

## Scope

### In Scope (MVP)

- Use bounded connect and command deadlines for the primary, reconnect, donation, and
  world-recovery Redis contexts.
- Guard every relevant context before a Redis command and classify null, timeout, error
  reply, and reconnect outcomes consistently.
- Remove synchronous SQL player-save fallbacks from `mark_player_dirty()` and fork
  failure paths while retaining retryable dirty state and truthful degraded status.
- Recover or merge `mud:dirty_players:flushing` during boot and after every child
  failure without overwriting newer dirty membership.
- Add explicit deadlines, exit-status handling, and retry-state restoration for both
  temporary player-save and world-snapshot children.
- Retain floor-drop deltas until the exact world-snapshot child succeeds; preserve them
  after timeout, crash, Redis error reply, or invalid completion.
- Add focused outage and source-contract regressions for Redis-disabled, null-context,
  timeout, fork-failure, child-failure, and stale-inflight cases.

### Out of Scope

- The Phase 01 replacement of both fork paths with immutable long-lived workers or a
  separately started sidecar.
- The Phase 01 typed, checksummed local journal and revision-aware ACK protocol.
- Redis report-cache TTL and single-flight redesign beyond changes required for bounded
  failure behavior.

---

## Prerequisites

- [x] Session 01 diagnostics expose Redis failures without bound values.
- [x] Session 03 establishes the fail-closed save behavior reused by dirty-state
      recovery edges.

---

## Deliverables

1. Bounded Redis connection and command helpers in `src/redis.c` and shared interfaces.
2. Non-blocking mutation failure behavior and recoverable dirty-set state.
3. Temporary child deadlines, exit handling, and exact floor-delta retention rules.
4. Focused regressions under `tests/async/` for every specified failure edge.

---

## Success Criteria

- [x] No Redis connection or command can block the simulation thread without a bounded
      deadline.
- [x] No dirty-player mutation failure invokes a synchronous full player save.
- [x] Existing and newly produced dirty inflight membership is merged without loss on
      boot, timeout, crash, or child failure.
- [x] Player-save and world-snapshot children have enforced runtime bounds and their
      exit status controls success reporting.
- [x] Floor deltas are cleared only after the matching world snapshot has completed
      successfully.
- [x] Focused regressions, formatting checks, and `make -C src` pass.

---

## Completion Summary

Completed on 2026-08-27. Redis connections and commands now have explicit deadlines,
dirty membership survives degraded operation and every child failure edge, temporary
children are supervised by deadline and exact exit status, and floor deltas remain
pending until the corresponding world snapshot is acknowledged. Validation passed
with 173/173 regressions plus signal-handler checks.
