# Session Specification

**Session ID**: `phase00-session06-redis-failure-and-recovery-containment`
**Phase**: 00 - Correctness and Immediate Lag Removal
**Status**: Not Started
**Created**: 2026-08-27
**Base Commit**: `fc469b0f`
**Work Window**: One Redis outage/recovery boundary covering command deadlines, dirty membership, temporary child supervision, and exact floor-delta acknowledgment.

---

## 1. Session Overview

Redis connections use unbounded connect calls, most commands rely on context-specific behavior, dirty mutation falls back to synchronous SQL, stale inflight membership can be overwritten, and both fork children can run forever. World events also clear floor deltas as soon as a snapshot child starts rather than after that exact child succeeds.

## 2. Objectives

1. Bound every Redis connect and synchronous command used by the server.
2. Preserve dirty intent locally and in Redis without synchronous mutation-path saves.
3. Merge stale/current inflight membership before every new flush and after every child failure.
4. Enforce deadlines and exit-status acknowledgment for dirty-save and world-snapshot children.
5. Clear floor deltas only after the matching world child succeeds, retaining newer deltas.

## 3. Technical Approach

- Route all contexts through a bounded connect helper and all synchronous commands through a guarded wrapper that normalizes null, timeout/context, and error replies.
- Keep configured Redis enabled during transient outages, expose availability separately, and retain up to the existing 512-player operational bound in a local retry set until `SADD` succeeds.
- Make inflight restoration return status, use `SUNIONSTORE`, and require restoration before `RENAME` so stale membership cannot be overwritten.
- Track child start times, poll with `waitpid(WNOHANG)`, terminate overdue children, reap them, and drive acknowledgment exclusively from zero exit status.
- Hold floor batches locally while a world child or its remote-delta acknowledgment is pending. On exact success, clear the prior remote delta set, then flush newer local deltas and start the next snapshot.

## 4. Deliverables

| File | Change |
|------|--------|
| `src/redis.c`, `src/redis.h` | Bounded helpers, retry state, watchdogs, exact acknowledgments |
| `tests/async/test_redis_failure_containment.py` | Source contracts for all scoped outage/recovery edges |
| Existing Redis tests | Preserve and extend prior retry and event-loop contracts |

## 5. Success Criteria

- [ ] Every Redis context has bounded connect/read/write behavior and every command is guarded.
- [ ] Dirty mutation never invokes synchronous SQL and survives null context, timeout, reconnect, fork, and child failure.
- [ ] Stale inflight state is merged before rename and after every failed child outcome.
- [ ] Both child types have enforced deadlines and status-based acknowledgment.
- [ ] Floor deltas clear only after exact snapshot success and newer deltas remain pending.
- [ ] Focused tests, formatting, build, and full suite pass.

## Next Steps

Run the `implement` workflow step.
