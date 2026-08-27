# Session Specification

**Session ID**: `phase02-session06-live-item-movement-and-corpse-cutover`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `cab7e556`

## Objectives

1. Hydrate live durable items from `item_current_owner` and retain exact item and owner
   revisions in a pointer-free runtime registry.
2. Stage player get, drop, put, give/trade, and corpse-loot movement without changing
   visible custody until the Session 05 transfer command acknowledges.
3. Publish a captured movement only when the actor, item subtree, source, destination,
   room/corpse generation, and command result still match.
4. Make current-owner rows authoritative for corpse restore and floor recovery; retain
   Redis hints and legacy events as observability only.
5. Preserve same-owner container rearrangement as an inventory checkpoint mutation.

## Design Boundary

Persistent loaders may read authoritative ownership while reconstructing objects. The
simulation thread never queries SQL for movement: it captures a bounded UID subtree and
submits exact hydrated revisions to the critical coordinator. Pending entries contain
only scalar identities and copied message metadata. Completion publication re-resolves
live objects and characters and validates their unchanged source topology before calling
legacy pointer mutation helpers.

Items without authoritative ownership state fail closed at audited player-facing
cross-owner routes. Creation and destruction remain explicit ownership commands rather
than implicit snapshot side effects.

## Success Criteria

- [x] Audited cross-owner commands submit one operation ID and publish after ACK only.
- [x] Same-owner rearrangement records topology without changing the typed owner.
- [x] Rejection, timeout, overload, disconnect, and stale topology preserve custody.
- [x] Concurrent attempts for an item subtree can produce at most one transition.
- [x] Corpse restore and floor recovery reject ownership conflicts.
- [x] Workers retain no live game pointers.
- [x] Focused tests, formatting, build, security, and full tests pass.
