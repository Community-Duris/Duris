# Plan 3: item supply, custody, and provenance

Start from add-double-entry HEAD 49af585c4. The existing item-transfer
accounting owner and references are the starting point, not a claim of complete
item coverage. Work against isolated UID fixtures while the epoch is inactive.
This plan owns ordinary item and world lifecycle routes; coupled priced
transactions belong to [Plan 4](04_COMPOUND_DOMAINS.md). See
[R4](../REMAINING_REQUIREMENTS.md).

## Result

For every admitted durable item UID, an operator can reconstruct why it
entered the economy, each owner/container/equipment transition, and why it
left. A retry or restoration cannot create a second live copy. Realized trade
prices are linked by Plan 4; no appraisal balance is invented.

## Starting files and first checks

Inspect src/item/item_movement_transaction.c,
src/economy/item_transfer_accounting.c,
src/item/item_transfer_repository.c,
src/persistence/economic_sql_item_transfer_transaction.c, and
src/flatfile/flatfile_item_repository.c. Start with
python3 tests/async/test_item_transfer_accounting.py and
python3 tests/async/test_universal_item_transfer_accounting.py.
Run tests/async/test_economic_accounting_item_reference_mysql.py only
against its disposable SQL fixture.

## Work

1. Audit all item allocation, source admission, movement, saved-item load,
   world reset, pet, spell, consume, extraction, and cleanup call paths.
   Separate real issuance/destruction from temporary objects and projections.
   Classify remaining lexical hits by reachable writer, nonwriter, or
   unsupported route; use the shared inventory format in Plan 5.
2. Extend typed source-event identity for quest/world generation, zone resets,
   mob loot generation, spell creation (e.g. wind blade, flame blade, minor creation),
   and item consumption/destruction (potions, scrolls, bandages, broken keys).
   Dedupe a logical issuance even when retried with another command ID. Keep UID
   lifetime and source lineage through copyover, save, reconnect, and restored snapshots.
3. Complete root/child/event references for all eligible ordinary transfers:
   player, room, container, equipment, locker, pet, corpse handoff, and
   same-owner topology changes. Support PC-to-NPC quest turn-ins without refusing
   durable ownership; consume submitted items under `item_transfer_reason::quest_turnin`.
   Support spell-driven drops (e.g. `remove curse` dropping cursed items to room floor).
   The before-state, event order, exact native ownership ledger row, and final graph
   must agree. Preserve historical root/parent data on destroyed children.
4. Integrate saved-item handoff and retirement with source epoch/root,
   payload digest, receipt, acknowledgement, and safe cleanup ordering.
   Missing payload, conflicting ownership, cyclic topology, or uncertain
   restoration retains evidence and refuses materialization. Do not turn
   extract_obj or template allocation into a universal supply writer.
5. Port each accepted route and refusal to flatfile after SQL qualification.
   Keep the existing item authority/journal; do not add a second mutable
   ownership catalog.

## Independent acceptance

- SQL and flatfile fixtures assert one current owner, one ordered legacy event
  per change, one exact accounting item reference per event, source claim for
  first admission or retirement when policy requires it, and exact-ID replay.
- Player journeys cover get/drop/put/equip, same-owner moves, nested containers,
  pet handoff, sourced world/quest/spell creation, spell component consumption,
  destruction, save/reconnect/restart, and UID-preserving restoration.
- Faults cover two simultaneous claimants, stale revision, missing payload,
  duplicate source with a new command ID, cyclic placement, interrupted commit,
  and unknown origin. Confirm no live publication before committed evidence.
- Focused item regressions and both server builds pass. Item-only operations
  may have no coin legs; money-valued pile create/destruction must use Plan 2's
  balanced money route or refuse in an active epoch.

## Boundary and handoff

Plan 4 consumes the item event/reference adapter for commercial, crafting, and
death composites. Plan 5 independently checks provenance and live UID totals.
This plan is complete when its own item routes and refusals pass on both
backends, without waiting for those composite domains to activate.
