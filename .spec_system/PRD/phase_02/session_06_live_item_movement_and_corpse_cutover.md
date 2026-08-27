# Session 06: Live Item Movement and Corpse Cutover

**Session ID**: `phase02-session06-live-item-movement-and-corpse-cutover`
**Status**: Complete
**Work Window**: The complete live movement boundary for player, container, floor,
trade, corpse creation/restoration, and corpse loot routes through ownership ACK and
world/player revision publication.

---

## Objective

Route every live cross-owner item movement through the Session 05 transfer primitive so
the game never publishes, destroys, or restores custody that the database did not
commit.

---

## Scope

### In Scope (MVP)

- Inventory post-Phase-01 object movement entry points including get, drop, put, give,
  trade/exchange, floor recovery, corpse creation, corpse restore, and corpse loot.
- Distinguish same-owner inventory/equipment/container rearrangement from critical
  cross-owner movement and mark the correct Phase 01 component or ownership command.
- Capture immutable item/subtree and source/target identities before mutation, reserve
  all affected keys, and stage or fence live objects until exact transfer completion.
- Integrate room/floor sequence state and corpse durable identity with player inventory
  revisions, ownership ledger, and outbox in the command transaction.
- Publish object movement and player messages only after durable ACK; on rejection,
  timeout, overload, or journal failure preserve the prior visible custody and retryability.
- Replace corpse-only relative item-event timing and floor-pickup hints as ownership
  authority while retaining compatible audit and world-recovery signals.
- Cover containers, quantity/split items, simultaneous looters, reconnect, death, and
  stale world/corpse generation edges.

### Out of Scope

- Locker custody, owned by Session 07.
- Auction custody and settlement, owned by Session 08.
- Phase 03 login ownership batching or historical retention.

---

## Prerequisites

- [x] Session 05 transfer primitive and baseline reconciliation are validated.
- [x] Phase 01 player and world revision/ACK paths are authoritative.
- [x] Phase 00 terminal death and recovery failures retain live state safely.

---

## Deliverables

1. Ownership command integration across audited live movement and trade paths in
   `src/actobj.c` and related modules.
2. Atomic corpse creation, restore, and loot custody integration in `src/files.c`,
   `src/sql_player.c`, and related recovery code.
3. Floor/world sequence and ownership coordination without Redis or event-timing
   authority.
4. Focused movement, container, trade, death, restore, simultaneous-loot, crash,
   replay, and failure-retention regressions under `tests/async/`.

---

## Success Criteria

- [x] Every audited cross-owner live movement supplies one operation ID and reaches the
      authoritative transfer primitive.
- [x] Same-owner rearrangement advances only the intended inventory component/revision
      and does not create a false ownership transfer.
- [x] A failed, stale, timed-out, or overloaded command leaves the item and subtree in
      their prior visible and durable custody.
- [x] Simultaneous pickup, trade, or corpse-loot attempts can produce at most one
      successful owner transition.
- [x] Corpse creation and restore cannot duplicate an item already owned elsewhere.
- [x] Floor recovery hints and legacy item events no longer decide current ownership.
- [x] Workers traverse no live object, player, corpse, room, or container pointers.
- [x] Focused regressions, formatting checks, and `make -C src` pass.
