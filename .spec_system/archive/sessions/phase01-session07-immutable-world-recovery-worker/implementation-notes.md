# Implementation Notes

**Session ID**: `phase01-session07-immutable-world-recovery-worker`
**Implemented**: 2026-08-27

- Added incremental main-thread capture for NPCs, floor objects, doors, and zone timers
  with record, time, record-size, generation-size, queue, and retry bounds.
- Added a long-lived publisher receiving owned framed bytes only, sealing CRC32,
  timestamp, schema, sequence, counts, payload length, and completeness.
- Published immutable generation keys before an atomic current-pointer and metadata
  swap; sequence continues above the durable Redis current generation after restart.
- Replaced the active world-save fork route and validated restore through the framed
  generation while leaving legacy dead code for Session 08 removal.
- Preserved pre-boundary Redis floor deltas until exact ACK and retained newer local
  deltas throughout capture/publication.
- Added bounded copyover/shutdown drain and aggregate recovery health.

No configured database, production service, credentials, player data, migration, or
destructive Redis operation was used during validation.
