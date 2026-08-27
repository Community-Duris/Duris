# Session Specification

**Session ID**: `phase01-session08-legacy-fork-removal-and-recovery-gate`
**Phase**: 01 - Replace Forked Full Saves
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `15fa583e`

## Objectives

1. Delete the retired player/world child-process persistence implementation and legacy
   JSON recovery codec.
2. Remove Redis dirty-player authority while retaining the revisioned local checkpoint
   coordinator and accurate operator controls.
3. Prove revision ordering, journal recovery, bounded queue behavior, exact world ACKs,
   and thread isolation with deterministic 25/50/100/200-client workloads.
4. Bring architecture, database, configuration, testing, and operational documentation
   in line with the final Phase 01 implementation.

## Design Boundary

The DNS lookup child and general SIGCHLD support are outside this persistence session.
Persistence-specific `fork`, `waitpid`, timeout, child connection, synchronous snapshot,
legacy JSON world-state, and Redis dirty-set authority must be absent. Player saves flow
through immutable capture, a private journal append dispatcher, the keyed worker, and
exact completion. World recovery flows through bounded capture and immutable generation
publication. Workers may receive owned data only.

Load verification is deterministic and dependency-free: it drives the production worker
and revision-state code with 25, 50, 100, and 200 logical clients, records queue/latency
metrics, and asserts the published limits. It is a non-production readiness gate, not a
claim about network or production database capacity.

## Success Criteria

- [x] Retired persistence child and legacy world JSON code are deleted.
- [x] Redis dirty-player keys and misleading destructive controls are retired.
- [x] Player/world source inventories enforce immutable-worker and exact-ACK boundaries.
- [x] Fault and 25/50/100/200 logical-client gates pass within explicit bounds.
- [x] Operator diagnostics and documentation describe the implemented topology.
- [x] Focused, format, build, security, and full validation pass.
