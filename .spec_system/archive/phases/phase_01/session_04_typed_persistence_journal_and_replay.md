# Session 04: Typed Persistence Journal and Replay

**Session ID**: `phase01-session04-typed-persistence-journal-and-replay`
**Status**: Complete
**Work Window**: The durable handoff boundary for unacknowledged player snapshots,
including append, sync, spill, checkpoint, restart replay, corruption, and quota edges.

---

## Objective

Ensure queued or unacknowledged player revisions survive process and worker failure in
a bounded typed journal whose replay is checksummed, schema-versioned, idempotent, and
ordered by player.

---

## Scope

### In Scope (MVP)

- Define a typed journal record with format version, record length, PID, player
  revision, component mask, payload version, unique identity, and checksum; never store
  unrestricted SQL.
- Append and sync records atomically before they become the only copy of dirty work,
  with explicit file permissions, directory sync, rotation, disk quota, and partial-
  write recovery behavior.
- Checkpoint or compact records only after exact durable acknowledgement, preserving
  newer records and crash safety across rewrite or rename boundaries.
- Replay valid records in per-player revision order, skip already applied revisions
  idempotently, bound boot/recovery work, and quarantine corrupt or unsupported records
  without silently discarding later valid data.
- Integrate journal spill with queue high-water, shutdown, worker failure, and retry
  behavior; expose journal bytes, oldest age, replay, corruption, duplicate, and
  backpressure metrics.
- Define fail-closed behavior for disk full, read-only filesystem, checksum mismatch,
  incompatible version, and ambiguous checkpoint outcomes.

### Out of Scope

- Legacy pfile deletion or complete historical pfile import.
- Production save-trigger cutover.
- Raw event-queue conversion for Phase 02 gameplay domains.
- World recovery snapshot storage.

---

## Prerequisites

- [x] Session 03 keyed worker and exact acknowledgement semantics are validated.
- [x] Journal tests use isolated ignored paths and never inspect or commit runtime
      player records.

---

## Deliverables

1. Typed journal encoder, decoder, checksum, append, sync, checkpoint, rotation, and
   recovery modules under `src/`.
2. Coordinator integration for append-before-handoff, bounded spill, ACK checkpoint,
   replay, duplicate suppression, and overload state.
3. Ignored runtime location and operator diagnostics documented without exposing player
   values.
4. Focused crash-point, truncation, corruption, disk-full, quota, replay-order, and
   duplicate-apply regressions under `tests/async/`.

---

## Success Criteria

- [x] A process kill after any append, enqueue, apply, commit, acknowledgement, or
      checkpoint boundary converges without losing or applying a player revision twice.
- [x] Corrupt, truncated, oversized, or unsupported records are detected and reported;
      no invalid record is executed as SQL or trusted as acknowledged.
- [x] Journal files and directories use explicit safe permissions and remain bounded by
      configured bytes and age with an explicit overload policy.
- [x] Acknowledged records are compacted atomically, while every newer or unacknowledged
      record survives a crash during compaction.
- [x] Replay is bounded, observable, ordered per player, and safe when the database or
      worker remains unavailable.
- [x] Focused regressions, formatting checks, and `make -C src` pass.
