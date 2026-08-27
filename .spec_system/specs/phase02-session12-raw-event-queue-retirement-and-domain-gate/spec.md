# Session Specification

**Session ID**: `phase02-session12-raw-event-queue-retirement-and-domain-gate`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `4785a557`

## Objectives

1. Retire active arbitrary-SQL item/scalar/large queue producers and execution/replay.
2. Replace the remaining login audit producer with a bounded typed operation and retain
   historical fallback records only through inspection/quarantine.
3. Prove all Phase 02 authoritative ledgers/current rows reconcile after replay and
   bounded synthetic load without simulation-thread I/O.
4. Publish an explicit domain inventory, crash matrix, capacity gate, and operator
   recovery contract before Phase 03 consumes current-state tables.

## Design Boundary

Only critical-command and revisioned snapshot journals may carry active durable work.
Legacy flat fallback records are evidence, never executable input: the compatibility
tool inventories record classes, hashes the source, and quarantines it without parsing
arbitrary SQL into operations. Historical event tables remain untouched.

The login/logout audit becomes a bounded schema-versioned command. Obsolete epic-gain,
zone-touch, and corpse item audit queue producers are removed because their authoritative
typed domain ledgers now contain the durable evidence. Phase 02 reconciliation composes
the domain-specific read-only tools and fails closed on any mismatch.

## Success Criteria

- [x] No active producer or replay route accepts unrestricted SQL text.
- [x] Legacy fallback records can only be inspected, hashed, and quarantined.
- [x] Remaining durable messages are bounded, typed, versioned, and deduplicated.
- [x] All Phase 02 reconciliation tools report zero mismatches locally.
- [x] Bounded 25/50/100/200-client synthetic load preserves queue/fence limits.
- [x] Focused, MySQL, format, build, security, and full-suite gates pass.
