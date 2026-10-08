# Required room-floor reset producer: implementation handoff - 2026-10-07

The original day-one requirement includes zone-reset spawns with zero refusals.
Current active accounting skips the original `O` room-floor object command before
construction (`src/world/db.c:6919-6977`). Its original body at 7316-7385 retains
prototype limits, force-repop, incumbent/non-take checks, artifacts, load checks,
RNG decisions and room publication. Those gameplay decisions must remain.

This is required producer work, independent of the cache correspondence reader.
Root owns its shared admission, supply-policy and durable room contracts. A
bounded producer owner can own `zone_reset_item_owner.{h,c}` and the original
`O` branch once those narrow contracts are ready. Existing private staged object
construction in `world/db.h` can be lent to that owner through a narrow friend;
the original constructor and once-only publication steps must remain intact.

The existing player creation API is insufficient: `queue_creation_grant()` in
`item/item_movement_transaction.c:1582` rejects absent/NPC players, and
`sourced_item_creation()` in `economy/item_transfer_accounting.c:150` requires
a positive player PID and excludes coin piles. A fabricated player cannot supply
reset authority. `item_lifecycle_source()` at 278 binds lineage/UID or a logical
integer; it does not freeze the real reset invocation and command slot.

The existing native-mobile reset path establishes actual invocation, operation,
slot, allocator identity and zone/room VNUMs at
`world/quest_mobile_native_birth.c:664-680`. Reuse that existing reset-generation
boundary for an actorless admitted world-generation operation. Freeze actual
decisions and output UIDs before mutation; commit supply/custody, source claim,
receipt and room literal in the same root, then use retained original publication.

Modern room persistence currently handles admitted `player_drop` at
`item/item_transfer_repository.c:2433,2793,2831`. The new creation requires exact
same-root room literal retention and cold recovery through the existing
`sql_room_item_payload` owner. Prototype coin objects additionally require their
actual denomination issuance and UID-held pile in that root. Artifact and hook
effects require their existing original owners; removing the refusal alone,
changing RNG order or reporting skipped required objects is not completion.

Extend original controls in
`tests/async/native_birth_accounting/alchemist_actual_reset.cpp` and
`run_actual_reset.py`, plus the original room SQL recovery journey. Their current
inactive and modeled/component evidence does not qualify active reset issuance.
Active SQL/flat parity, actual constructor/publication, retries and cold restart
remain original major-plan gates. No producer source or gate is changed by this
handoff; current inactive accounting and the declined spell-path decision remain.

Fresh opening still needs a separate witnessed lifetime for originless NPC cash.
The existing persisted holdings reader and later typed-birth wallet locker do
not adopt those NPCs. Runtime IDs or VNUMs cannot substitute for durable lifetimes;
unknown historical origin must remain explicit in reconciliation.
