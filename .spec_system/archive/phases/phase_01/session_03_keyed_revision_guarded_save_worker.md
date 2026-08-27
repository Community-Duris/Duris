# Session 03: Keyed Revision-Guarded Save Worker

**Session ID**: `phase01-session03-keyed-revision-guarded-save-worker`
**Status**: Complete
**Work Window**: One end-to-end in-memory player job boundary from keyed enqueue and
coalescing through transactional apply to exact main-thread completion.

---

## Objective

Apply immutable player snapshots outside the simulation thread in per-player order,
reject stale revisions transactionally, and clear only the exact components confirmed
by a successful main-thread acknowledgement.

---

## Scope

### In Scope (MVP)

- Add a bounded byte- and age-limited queue keyed by PID with one ordered stream per
  player and parallelism only across independent players.
- Coalesce pending work by retaining the newest snapshot and the union of all
  unacknowledged component masks; define behavior when a job is already inflight.
- Apply typed components through a borrowed MySQL/MariaDB connection in one transaction
  guarded so revision N can commit only when the durable revision is older.
- Return bounded completions containing PID, revision, component mask, apply outcome,
  retry classification, and durable-revision evidence without live pointers.
- Apply completions on the game thread, ignore stale results, retain newer dirty state,
  and reconcile ambiguous commits by revision identity before retry.
- Expose queue bytes/age, capture/apply/ACK latency, revision gap, retry state, worker
  lifecycle, and high-water metrics through the Phase 00 diagnostics surface.

### Out of Scope

- Durable journal append, spill, or restart replay.
- Production trigger cutover or legacy fork removal.
- Phase 02 critical-domain command transactions and outboxes.
- World recovery snapshots.

---

## Prerequisites

- [x] Session 01 revision state and Session 02 immutable snapshot capture are validated.
- [x] Phase 00 connection contracts and redacted observability are available.

---

## Deliverables

1. Player-keyed coordinator, bounded job/result queues, worker lifecycle, and completion
   application in focused `src/` modules.
2. Typed component repositories and revision-guarded transaction integration with the
   existing connection pool.
3. Operator metrics for queue, worker, revision, retry, and end-to-end save health.
4. Focused concurrency, stale-order, ambiguous-commit, pool-failure, and component
   apply regressions under `tests/async/`.

---

## Success Criteria

- [x] Jobs for one PID apply in revision order while independent PIDs can progress in
      parallel within configured bounds.
- [x] Revision N cannot overwrite N or any newer durable revision.
- [x] A stale or failed acknowledgement clears no newer dirty component.
- [x] Coalescing never drops an unacknowledged component and never duplicates a
      snapshot already represented by a newer queued revision.
- [x] Workers traverse no live game objects and hold no game or queue lock across
      database operations.
- [x] Queue capacity, bytes, age, retry, worker execution, and shutdown states are
      bounded, observable, and truthful.
- [x] Focused regressions, formatting checks, and `make -C src` pass.
