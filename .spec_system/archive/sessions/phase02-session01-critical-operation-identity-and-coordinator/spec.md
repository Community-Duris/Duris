# Session Specification

**Session ID**: `phase02-session01-critical-operation-identity-and-coordinator`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `e8e8b872`

## Objectives

1. Define a bounded immutable critical-command envelope and durable 128-bit operation
   identity generated once before acceptance.
2. Persist every accepted command independently in a checksummed typed journal.
3. Serialize conflicting entity-key sets deterministically while allowing unrelated
   commands to execute concurrently.
4. Retain the same operation ID through retries, replay, attachment, and exact typed
   completion.
5. Expose bounded redacted queue, journal, retry, age, and fence health.

## Design Boundary

Critical commands are non-coalescing. Their operation ID is gameplay identity, unlike
the process-local observability IDs and player snapshot revisions inspected during
planning. Entity keys are normalized into a sorted unique set before admission.
Accepted commands cannot be cancelled or replaced; repeated submission with the exact
same envelope attaches to the existing operation, while a mismatched duplicate fails
closed.

The journal append is the bounded acceptance boundary. After it succeeds, coordinator
workers receive owned bytes only and main-thread pulse/completion work performs no
database, Redis, or filesystem I/O. Database inbox/outbox behavior remains Session 02.

## Success Criteria

- [x] Operation IDs are collision-resistant, stable, serializable, and never regenerated on retry/replay.
- [x] Critical records are independently journaled and never coalesced.
- [x] Multi-key conflicts serialize without deadlock and independent commands progress concurrently.
- [x] Exact operation/attempt completion alone releases fences and publishes terminal state.
- [x] Bounds and redacted diagnostics fail closed under overload or malformed input.
- [x] Focused, format, build, security, and full validation pass.
