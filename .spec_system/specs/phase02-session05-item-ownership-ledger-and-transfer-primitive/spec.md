# Session Specification

**Session ID**: `phase02-session05-item-ownership-ledger-and-transfer-primitive`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `a3ad397f`

## Objectives

1. Replace process-local item identity allocation with database-reserved, non-overlapping
   ranges before world objects load.
2. Add a typed owner taxonomy, one authoritative current-owner row per durable item,
   generic owner revisions, immutable ownership ledger, baseline, and quarantine schema.
3. Add a fixed, bounded transfer command that atomically validates and moves a declared
   complete item subtree while committing inbox result and outbox notification.
4. Reconcile unambiguous legacy player, corpse, locker, account-locker, and room custody;
   quarantine duplicate identities instead of choosing a winner.
5. Prove player, custody, subtree, creation, destruction, stale, conflict, replay, and
   restart behavior with synthetic adapters only; live movement remains Session 06 scope.

## Design Boundary

An owner is `(type, id, context_id)`, where type is player, container, room, corpse,
locker, auction, system, or destruction. A bounded command declares every item in one
rooted durable subtree, the expected source owner, the destination owner, and the
expected item and owner revisions. `item_current_owner.root_item_uid` makes completeness
an indexed equality check; every declared row is then locked by ascending item UID.

Creation moves a previously absent identity from system creation into custody.
Destruction is terminal custody, never row deletion. Generic snapshots retain item
payload compatibility but do not write current-owner, owner-revision, or ledger tables.

## Success Criteria

- [x] Database-reserved item UID ranges cannot overlap across processes or Redis state.
- [x] Every accepted item has one current owner row and one stable nonzero identity.
- [x] Transfer, all item revisions, both owner revisions, ledger, inbox, and outbox commit once.
- [x] Declared subtree mismatch, stale state, missing/duplicate identity, and overflow fail closed.
- [x] Baseline imports only unambiguous custody and records every conflict in quarantine.
- [x] Synthetic adapters prove creation, player/custody transfer, subtree, and destruction.
- [x] Focused, guarded schema, reconciliation, format, build, security, and full tests pass.
