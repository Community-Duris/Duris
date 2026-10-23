# Guild Rooms shop source audit

Canonical key: component:guild_rooms. Physical coverage row 400 / CSV line
401; candidate dependency row 50 / CSV line 50. Reviewed October 11, 2026,
Checkpoint 487, against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static source,
identity, history, and typed-reference review. It does not establish runtime
shop loading, item behavior, editor ownership, persisted world state, or
permission to activate or move this record.

## Source inventory and manifest ownership

The only candidate file is one [SHP record](../../../areas/shp/guild_rooms.shp#L1),
headed #8850. Its open/close messages call it “The Guild Store.” It lists
products #8555–#8559, keeper #8552, and shop room #8560. There is no
same-stem WLD, MOB, OBJ, ZON, QST, story sidecar, editor map, or cap file.

Neither [areas/AREA](../../../areas/AREA#L48) nor
[areas/qst/AREA](../../../areas/qst/AREA#L42) lists guild_rooms. Both list
grave at #84–#86 and guildhalls at #480–#489
([areas/AREA](../../../areas/AREA#L218),
[qst/AREA](../../../areas/qst/AREA#L199)). The filename and shop message
suggest a guild association, but they do not identify which registered
guildhall map or owner this record belongs to.

## Typed shop references and active Grave identities

The candidate’s keeper and product references overlap active Grave WLD room
numbers by value but not by type. Its room-typed reference does resolve to
an active Grave room:

| SHP role | Candidate value | Current exact identity finding |
| --- | --- | --- |
| Keeper | #8552 | No current MOB prototype is defined at #8552. Active grave.wld defines room #8552. |
| Products | #8555–#8559 | No current OBJ prototypes are defined at these five numbers. Active grave.wld defines rooms #8555–#8559. |
| Shop room | #8560 | Active grave.wld defines room #8560, “A Simple Stone Tomb.” |

The candidate contributes no matching mobile or object files. A separate
compiled guild-transfer path calls real_room(8556)
([specs.mobile.c](../../../src/specs/specs.mobile.c#L263)); #8556 is also an
active Grave room. This code reference does not supply the SHP keeper or its
five product prototypes, and it is not evidence that the Guild Store shop is
intentionally in Grave.

The checkout has no areas/world.shp. In non-mini mode, the shop reader can
fall back to areas/shp/*.shp
([fallback](../../../src/economy/shop.c#L3276)). It maps the keeper through
real_mobile() ([lookup](../../../src/economy/shop.c#L3492)), then
assign_the_shopkeepers() indexes mob_index[keeper] without a negative-index
guard ([assignment](../../../src/economy/shop.c#L3602)). If this fallback
reads guild_rooms.shp while no active #8552 mobile exists, the unresolved
keeper can reach that unchecked index. The five product IDs also lack
current OBJ prototypes. These are conditional static loading and identity
risks; they do not prove runtime behavior.

## Identity, history, and disposition

This source consists of a single shop file; it has no candidate map graph or
reset list. Its keeper and products have no exact current typed mobile/object
definitions, while all six cross-type values (#8552 and #8555–#8559) appear
as active Grave rooms. The separate room-typed #8560 reference also resolves
to an active Grave room. No same-stem map, mobile, object, zone, or quest
record exists to corroborate the shop’s ownership.

Followed guild_rooms SHP history remains on the same stem through the 2009
area-tree move and the later 2013 edit. No alternate-stem rename or explicit
whole-zone replacement was found. The unrelated active guildhalls manifest
range does not establish replacement or ownership.

Keep candidate row 50 **RETAIN IN PLACE**. Preserve the SHP record and its
five product numbers while its keeper, products, Grave room overlap,
guild-transfer code reference, and intended area owner are unresolved.
Resolve the owner, current mobile and object identities, shop room, global
shop-file source, and editor/deployment state before activation, move,
rename, archive, or merge proposals. See the [24 builder questions](GUILD_ROOMS_BUILDER_QUESTIONS.md),
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-487-guild-rooms-shop-references),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-487-guild-rooms-identities-and-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-487-guild-rooms-retained-pending-typed-shop-ownership),
[candidate row 50](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L50), and
[coverage row 400](ZONE_COVERAGE_REGISTER.csv#L401). No source-area files,
code, tests, builds, runtime, server, database, or containers were changed
or used.


## Checkpoint 899: typed shop and active Grave boundary closure

The source remains a single unregistered SHP record; there is no candidate WLD, MOB, OBJ, ZON, QST, or story-sidecar file. A type-aware exact-reference scan confirms that keeper #8552 still has no current MOB definition and products #8555-#8559 still have no OBJ definitions. The only same-typed current room definition among these SHP fields is Grave's active room #8560. The other same-number Grave room records (#8552 and #8555-#8559) remain cross-type values, not keeper mobiles or stock objects.

Outside ZON sources have no mobile reset role for #8552 and no object reset role for #8555-#8559. The two exact room-operand hits to #8560 are active `grave.zon` placements: mobile #8424 and #8401 are reset in Grave room #8560. They remain Grave reset rows, not evidence that `guild_rooms` is a loaded zone. See [Grave reset placements](../../../areas/zon/grave.zon#L209).

The active Grave WLD has two local direction records targeting #8560, from D3 and D0 edges ([D3](../../../areas/wld/grave.wld#L3336), [D0](../../../areas/wld/grave.wld#L3374)). This confirms ordinary Grave map connectivity to its own room; it does not create a WLD graph for the SHP-only candidate. No outside type-25 portal or type-29 switch target points to the candidate's shop room #8560. The two compiled `real_room(8556)` callsites in [specs.mobile.c](../../../src/specs/specs.mobile.c#L263) still point to active Grave room #8556 and remain distinct from both candidate keeper #8552 and shop room #8560.

No other QST or story sidecar contains the candidate's keeper, products, or room VNUMs. Across 711 external parser-shaped N-format SHP records, no stock, keeper, or room field uses those values; no outside SHP raw line contains any of them. This leaves the candidate's #8552 keeper and #8555-#8559 products unresolved. It also leaves the wildcard fallback conditional; no shop-loading or keeper-assignment runtime behavior is established.

No new lineage, rename, replacement, or path evidence was found. Keep candidate row 50 **RETAIN IN PLACE**. Preserve the Guild Store as an unregistered SHP-only source, keep its typed mobile/object references unresolved, and continue to identify #8560 and the #8556 code consumer as active Grave rooms. See the [24 builder questions](GUILD_ROOMS_BUILDER_QUESTIONS.md).
