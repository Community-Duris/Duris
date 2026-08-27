# Implementation Notes

- 2026-08-27: Session opened from `89216f75` after Session 06 passed and published.
- Existing locker snapshots are immutable worker inputs but are destructive row-replace
  SQL for public inventory; ownership authority must remain exclusively in the item
  transfer repository.
- Locker restore currently passes a display name to an owner checker that now requires
  numeric typed identity. This is the first fail-closed compatibility repair.
- Synthetic sorting/private chest wrapper objects are UI scaffolding. Their object UID
  must never become `target_parent_item_uid`; locker chest context is the durable parent.
- Public and private live custody now resolve to `(lockers.id, private_chests.id)`. Floor
  and synthetic-wrapper deposits use a zero durable parent; real nested containers keep
  their authoritative item parent topology.
- Snapshot and leave hooks defer while the player has an accepted movement. Copyover
  performs a second locker drain after critical ACK and terminal player-save drains so
  a generation dirtied during either stage cannot escape serialization.
- The additive normalization creates missing public chest identities, repairs legacy
  null contexts, and updates only opening-revision owner/baseline rows. Repeated local
  application reconciled with zero mismatches.
