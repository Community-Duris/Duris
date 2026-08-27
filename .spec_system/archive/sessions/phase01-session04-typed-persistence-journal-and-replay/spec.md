# Session Specification

**Session ID**: `phase01-session04-typed-persistence-journal-and-replay`
**Phase**: 01 - Replace Forked Full Saves
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `60a591d2`
**Work Window**: Durable typed player-snapshot handoff and bounded restart replay; no production trigger cutover.

---

## 1. Session Overview

The in-memory worker can apply snapshots exactly but process loss can still discard
unacknowledged values. This session adds an append/sync-before-handoff journal, exact
ACK checkpointing, and idempotent bounded replay using the Session 03 repository.

## 2. Objectives

1. Encode every snapshot value in a versioned endian-stable format with CRC32 framing.
2. Append under 0700/0600 permissions, enforce quota, sync data, and sync directory metadata.
3. Scan past checksummed corrupt records when framing is trustworthy and quarantine invalid bytes.
4. Compact acknowledged revisions by fsynced temporary rewrite, atomic rename, and directory sync.
5. Replay newest cumulative records per PID in revision order with duplicate suppression and bounded work.
6. Integrate durable append/ACK hooks with the keyed worker and expose redacted journal health.

## 3. Scope

### In Scope

- Snapshot binary codec, record framing, checksum, file lifecycle, replay and health.
- Isolated temporary-directory crash/corruption/quota/permission/order tests.
- Optional worker hooks; production startup and triggers remain disabled.

### Outside This Work Window

- Legacy pfile import/deletion, world snapshots, Phase 02 event journals, and trigger cutover.

## 4. Technical Approach

Records have fixed magic/version/length/identity metadata followed by a typed payload
and CRC32. Decoders validate every count/string/length against Session 02 bounds before
allocation. The journal mutex never spans database apply. Append writes one complete
frame with retrying writes and fdatasync. Checkpoint writes retained frames to a new
0600 file, syncs it, renames it over the journal, and syncs the 0700 parent directory.
Replay scans bounded bytes/records, coalesces newest per PID, sorts by PID/revision,
and treats equal/newer durable evidence as idempotent success.

## 5. Success Criteria

- [x] Full snapshot round trips without raw pointers, SQL, host-layout structs, or unbounded fields.
- [x] Append is durable before optional worker handoff and quota/permission failures fail closed.
- [x] Truncation, checksum mismatch, unsupported version, and oversized length are classified/quarantined.
- [x] Exact durable ACK compaction retains every newer and other-PID record across crash boundaries.
- [x] Replay is bounded, duplicate-suppressing, per-PID ordered, idempotent, and retry-safe.
- [x] Journal health reports only bytes/age/counts/outcomes/backpressure metadata.
- [x] Focused tests, formatting, warning-as-error build, security scan, and full suite pass.

## Next Steps

Continue with Phase 01 Session 05 nonterminal save pipeline cutover.
