# Session 05: Item Ownership Ledger and Transfer Primitive

**Session ID**: `phase02-session05-item-ownership-ledger-and-transfer-primitive`
**Status**: Complete
**Work Window**: One authoritative ownership boundary spanning durable item identity,
owner taxonomy, current-owner state, subtree transfer, affected inventory revisions,
immutable ledger, outbox, baseline reconciliation, and a synthetic transfer adapter.

---

## Objective

Create the atomic transfer primitive that gives each transferable durable item one
authoritative current owner and advances all affected inventory state exactly once.

---

## Scope

### In Scope (MVP)

- Inventory implemented item UID allocation, player/locker/corpse/auction persistence,
  container semantics, Phase 01 inventory revisions, and legacy item-event coverage.
- Define bounded typed owner identities for player, container, room/floor, corpse,
  locker/chest, auction custody, system creation, and destruction without embedding
  display names as authority.
- Require stable nonzero collision-free IDs for durable transferable items and define
  creation, legacy missing-ID, duplicate-ID, split/merge, subtree, and destruction rules.
- Add authoritative current-owner and immutable operation-keyed ownership-ledger schema,
  owner/revision indexes, and compatibility references to retained legacy events.
- Implement a transaction that validates expected owner and item revision, locks item
  and affected inventory rows canonically, transfers the complete declared subtree,
  advances all affected revisions, and writes ledger, inbox result, and outbox atomically.
- Reconcile unambiguous existing player, corpse, locker, auction, and floor custody into
  baseline owner rows; report and quarantine conflicts instead of selecting by timestamp.
- Prove the primitive with synthetic player-to-player, custody, subtree, creation, and
  destruction adapters before live route cutover.

### Out of Scope

- Live get, drop, give, trade, corpse, locker, or auction command cutover.
- Phase 03 batched login ownership reads.
- Deletion or reinterpretation of legacy `persistence_item_events` history.

---

## Prerequisites

- [x] Sessions 01 and 02 are validated.
- [x] Phase 01 inventory revision and immutable snapshot contracts are available.
- [x] Baseline scans and schema writes use only isolated non-production databases.

---

## Deliverables

1. Item identity, owner, transfer, subtree, result, and reconciliation contracts in
   focused `src/` modules.
2. Current-owner and ownership-ledger migration, bootstrap synchronization, indexes, and
   schema verification under `migrations/`.
3. Typed ownership transaction repository integrated with inbox, outbox, revisions, and
   Phase 01 journal/completion routing.
4. Focused synthetic transfer, duplicate-ID, stale-owner, subtree, conflict, crash,
   replay, and baseline reconciliation regressions under `tests/async/`.

---

## Success Criteria

- [x] Every accepted durable item has one stable nonzero identity and at most one
      authoritative current-owner row.
- [x] One transfer commits expected-owner validation, current owner, immutable ledger,
      all affected inventory revisions, inbox result, and outbox rows atomically.
- [x] Duplicate or ambiguous replay cannot duplicate, lose, resurrect, or move an item
      twice.
- [x] A container transfer includes its declared durable subtree or changes nothing.
- [x] Stale expected owner, missing identity, duplicate identity, or revision conflict
      fails closed without selecting an owner from incomplete legacy events.
- [x] Baseline reconciliation preserves legacy history and quarantines every ambiguous
      custody case for explicit repair.
- [x] Generic inventory checkpoints cannot overwrite transaction-owned current-owner
      state or regress an affected inventory revision.
- [x] Focused regressions, isolated schema tests, formatting checks, and `make -C src`
      pass.
