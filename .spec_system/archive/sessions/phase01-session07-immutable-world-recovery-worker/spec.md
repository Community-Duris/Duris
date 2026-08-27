# Session Specification

**Session ID**: `phase01-session07-immutable-world-recovery-worker`
**Phase**: 01 - Replace Forked Full Saves
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `a16731bb`

## Objectives

1. Incrementally capture one sequence-numbered generation on the game thread.
2. Transfer only bounded owned bytes to a long-lived publisher worker.
3. Atomically publish payload and validation metadata to the recovery domain.
4. Accept only exact completion and preserve post-boundary floor deltas.
5. Validate schema, checksum, completeness, sequence, and age before restore.

## Design Boundary

The capture coordinator owns its sequence and stage across pulses; overlapping capture
requests coalesce. Live pointers are used only by the game-thread capture pulse and are
never retained by the worker. The worker receives one self-contained framed byte
generation. Redis report caches remain unrelated to recovery validity.

Floor deltas already flushed before capture form the generation boundary. While capture
or publication is active, later deltas remain in the local retry buffers. Exact publish
ACK clears only the pre-boundary Redis delta set; later local deltas are then eligible
for their normal flush.

## Success Criteria

- [x] Capture work and retained bytes have explicit per-pulse and generation bounds.
- [x] Worker code receives owned bytes and cannot traverse live world pointers.
- [x] Atomic publication preserves the previous valid generation on failure.
- [x] Restore rejects malformed, partial, corrupt, stale, or regressed generations.
- [x] Exact sequence ACK alone advances recovery health and floor-delta boundary.
- [x] Focused and full validation pass.
