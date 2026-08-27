# Code Review

## Findings Repaired

1. **High - 200-player wave exceeded the keyed PID admission bound.** The former
   128-PID worker and pipeline limits could reject a simultaneous 200-player checkpoint
   wave before the byte limit was reached. Both limits are now 256 and the load gate
   asserts 200 admitted identities.
2. **High - operator world validity trusted an obsolete independent Redis flag.** A flag
   could report valid while the payload was missing, stale, or corrupt. Status now uses
   `redis_has_world_state()`, which validates the pointed generation.
3. **High - the immortal `redis clear dirty force` message claimed to discard pending
   player saves but only deleted obsolete Redis keys.** The control and Redis key
   lifecycle were removed; the real local queue remains fail-closed and non-clearable.
4. **Medium - retired persistence child and JSON code remained compiled as dead code.**
   The state, polling, termination, synchronous fallback, serializer, and parser were
   deleted rather than hidden behind unused annotations.
5. **Low - documentation still described fork children and Redis player dirty authority.**
   Architecture, database, configuration, testing, runbook, codebase, and README text
   now match the revisioned/journaled player and immutable world pipelines.

## Residual Boundary

The hostname lookup child and generic SIGCHLD support remain because they are not
persistence paths. The Phase 01 gate explicitly scopes its no-fork assertion to
`redis.c` and persistence worker modules.

No unresolved blocking findings remain.
