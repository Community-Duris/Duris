# Implementation Notes

**Session ID**: `phase01-session06-terminal-drain-and-shutdown-safety`
**Implemented**: 2026-08-27

## Result

- Added fixed-capacity exact-revision terminal fences and typed DB, journal, timeout,
  invalid, and unavailable outcomes.
- Promoted an already complete cumulative revision when possible; otherwise captured
  one newest all-component revision. Journal durability is remembered explicitly.
- Routed the shared destructive-save helper through the coordinator and retained its
  deferred retry slot unless durability succeeded.
- Added monotonic bounded quiesce/drain gates for copyover and shutdown, including the
  dispatcher record already removed from the visible queue.
- Retired new player flat-fallback writes while preserving locker compatibility and all
  existing files.
- Added aggregate terminal, admission, append-in-flight, timeout, and drain health.

## Review Repair

Review found and repaired two high-impact edges: drain initially ignored the journal
append currently in flight, and copyover failures after quiescence could leave the live
server rejecting saves. Review also made existing fully cumulative revisions promotable
without duplicate capture and replaced inferred journal state with an explicit bounded
durable-revision registry.

No migration, production service, credentials, player data, or existing pfile was
accessed or modified.
