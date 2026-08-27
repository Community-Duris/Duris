# Session Specification

**Session ID**: `phase03-session01-consistent-player-load-transaction`
**Phase**: 03 - Load Path, Schema, and Retention
**Status**: In Progress
**Created**: 2026-08-27
**Base Commit**: `286efed4`

## Objectives

1. Replace component-by-component login reads with one worker-owned consistent snapshot.
2. Reuse the bounded Phase 01 `player_snapshot` value model for status, ancillary rows,
   skills, affects, and shapechange data, while loading Phase 02 domain revisions as
   authoritative read-only values.
3. Stage results by exact request identity and publish them only on the game thread.
4. Make missing components, timeout, cancellation, disconnect, stale completion, and
   shutdown fail cleanly without partial character publication.

## Design Boundary

The worker owns one pooled MySQL connection and one repeatable-read, read-only consistent
snapshot. It returns bounded values only: no descriptor, character, world, guild, room,
object, affect-list, or other live pointer crosses the worker boundary. The game thread
materializes a fresh character only after identity, component, revision, row, byte, and
deadline validation succeeds.

Items and pets are explicitly excluded from this session's repository result and remain
for Sessions 02 and 03. Until those sessions land, their existing post-publication load
is a tracked compatibility boundary rather than part of the Session 01 transaction.

## Success Criteria

- [x] Required non-item/non-pet player rows come from one consistent snapshot.
- [x] Required query failure produces no partial publication.
- [x] Requests, results, rows, bytes, and elapsed time are explicitly bounded.
- [x] Duplicate, cancelled, disconnected, stale, and shutdown results are discarded.
- [x] Normal account login database reads run only on the load worker.
- [x] Copyover uses the same worker-owned repository before publication.
- [x] Focused MySQL, source, format, build, security, and full-suite gates pass.
