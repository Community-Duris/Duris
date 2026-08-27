# Code Review: Typed Persistence Journal and Replay

**Reviewed**: 2026-08-27
**Base commit**: `60a591d2`
**Result**: RESOLVED

## Scope

Reviewed the complete Session 04 diff: snapshot codec, record framing, filesystem
boundaries, quota, scanner, quarantine, compaction, replay, worker hooks, health output,
documentation, and regressions.

## Findings

### High - resolved

1. Duplicate identity initially included the unique record ID, so equivalent re-appends
   could not deduplicate. Replay now keys logical identity by PID, revision, components,
   and typed-payload checksum.
2. Journal checkpointing was initially invoked from main-thread completion draining.
   Exact durable ACK checkpointing now occurs in the worker post-commit path before the
   completion is published; the main pulse performs no journal filesystem operation.
3. Invalid frames could be quarantined again on every failed replay. Initialization now
   atomically compacts all successfully quarantined invalid bytes while preserving every
   later valid frame.

### Medium - resolved

1. A truncated tail reported its starting offset incorrectly, weakening recovery
   diagnostics. The scanner now quarantines the exact remaining tail.
2. Quota arithmetic could underflow when a single frame exceeded the total quota. The
   frame-size condition is checked before subtraction.
3. Allocation failures from scan containers, sorting, duplicate identity, or replay
   watermarks could escape the persistence API. They now return fail-closed I/O or
   replay-backpressure outcomes and retain journal records.
4. Decoder review added validation for spell IDs, object depth, parent ordering,
   metadata coherence, and every nested row count before accepting a frame.

## Behavioral Review

- Records contain typed values only—never raw SQL, pointers, or host struct bytes.
- Physical journal bytes and record counts are bounded before decode or application.
- Append sync completes before a successful durable-handoff result is returned.
- Checkpoint removes only the same PID at or below exact durable revision.
- Scanner can recover later valid magic-delimited frames and quarantines invalid bytes.
- Replay is ordered, duplicate-suppressing, idempotent, and stops without deletion on
  retryable or terminal apply outcomes.
- Production trigger cutover remains intentionally outside this session.

## Verification

- Codec/filesystem/corruption/compaction/replay runtime regression: PASS.
- Worker durable-handoff and post-commit checkpoint regression: PASS.
- Warning-as-error build, security scan, formatting, and whitespace checks: PASS.
- Full suite: PASS, 181/181 plus signal-handler checks.

## Conclusion

All findings are resolved. The implementation is ready for validation.
