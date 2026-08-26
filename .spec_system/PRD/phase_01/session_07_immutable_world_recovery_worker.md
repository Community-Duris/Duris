# Session 07: Immutable World Recovery Worker

**Session ID**: `phase01-session07-immutable-world-recovery-worker`
**Status**: Not Started
**Work Window**: The complete world recovery generation from bounded main-thread
capture through background serialization and publication to exact completion and floor-
delta acknowledgement.

---

## Objective

Replace forked world serialization with a long-lived worker that receives immutable,
sequence-numbered snapshots, publishes a checksummed recovery generation atomically,
and clears floor deltas only after the matching acknowledgement.

---

## Scope

### In Scope (MVP)

- Define typed immutable world snapshot records for NPCs, floor objects, doors, zones,
  timestamps, schema version, sequence, and completeness metadata.
- Capture world records on the game thread with explicit per-pulse item, byte, and time
  budgets so large worlds do not create a new serialization spike.
- Add a long-lived in-process worker with bounded queue, runtime, retry, stop, restart,
  and completion behavior; the worker may serialize and publish but never traverse live
  world, character, or object graphs.
- Publish snapshot payload, checksum, timestamp, sequence, and validity through an
  atomic temporary-generation swap whose success is verified before ACK.
- Retain and merge floor-drop deltas across capture, worker, Redis, checksum, timeout,
  and stale-completion failure; clear only deltas included in the exact acknowledged
  generation.
- Validate recovery startup against format version, checksum, sequence, age, and
  completeness and expose world-worker and recovery-domain health separately from
  optional cache health.

### Out of Scope

- An external sidecar process or new deployment IPC contract unless post-Phase-00
  evidence makes the in-process worker unsafe.
- Player checkpoint or gameplay-domain journal records.
- Redis report-cache TTL and single-flight redesign beyond recovery-domain separation.
- Final deletion of the disabled world fork, owned by Session 08.

---

## Prerequisites

- [ ] Phase 00 Redis deadlines, child containment, and floor-delta retention are
      validated.
- [ ] Sessions 03 and 04 worker lifecycle, bounded queue, checksum, and ACK patterns are
      available for reuse where their contracts fit.

---

## Deliverables

1. Immutable world snapshot structures, incremental capture coordinator, worker, and
   completion APIs in focused `src/` modules.
2. Atomic Redis recovery-generation publication and validated restore integration in
   `src/redis.c` and related interfaces.
3. Independent world recovery queue, sequence, checksum, age, retry, and degradation
   diagnostics.
4. Focused large-world, stale-ACK, Redis outage, checksum, partial capture, floor-delta,
   worker crash, timeout, shutdown, and restore regressions under `tests/async/`.

---

## Success Criteria

- [ ] No world worker or serializer traverses live `P_char`, `P_obj`, room, door, or zone
      pointers after the main-thread capture step.
- [ ] Main-thread capture obeys explicit per-pulse work and memory bounds and can resume
      without mixing two world generations.
- [ ] A recovery generation is valid only when payload, checksum, timestamp, sequence,
      and completion metadata are published consistently.
- [ ] Floor deltas survive every failed, stale, timed-out, or superseded snapshot and
      clear only for the exact successful generation that includes them.
- [ ] Worker queue, runtime, restart, and shutdown behavior is bounded and observable.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
