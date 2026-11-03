# The Black Pearl audit supplement

Checkpoint 774 completes the static typed-dependency inventory for the active
Black Pearl area. It confirms the sole foreign native-quest item reference,
checks reset, quest, shop, sidecar, duplicate-prototype, and portal consumers,
and classifies the old zone-ship room constant as dormant code. This source
review does not claim that the campaign givers or ship are currently
accessible.

## Scope and source connectivity

The active Black Pearl contributor owns 84 rooms (#142200–#142283), 63
mobiles, 98 objects, and 31 native Q/QA contracts. It has no local SHP file.
The static WLD census records 175 exit declarations, 174 resolved exits, two
weak components with a largest component of 83 rooms, no active cross-area
boundary, and one positive unresolved/unregistered target. Keep those map
findings with the [coverage row](ZONE_COVERAGE_REGISTER.csv#L32) and
[WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md); this supplement does not
change them. The comprehensive story and access review is in
[The Black Pearl dossier](../zone-stories/THE_BLACK_PEARL.md).

## Foreign quest item and owner

The only native item edge leaving Black Pearl's prototype set
is QST line 384, which rewards Moonshae object #26201, the shimmering key.
The story sidecar repeats #26201 as an item VNUM for the same Abal access
exchange. Prototype #26201 is defined in active Moonshae OBJ, not in
Black Pearl. The dossier records that the key opens Moonshae's king's secret
door, a different target from Black Pearl's heavy chest. Moonshae's own
[typed-dependency review](MOONSHAE_ISLAND_AUDIT_REVIEW.md#typed-inbound-and-outbound-dependencies)
also records the Black Pearl QST and sidecar as inbound consumers. Preserve
Moonshae's item ownership and do not treat the reward declaration as proof the
Moonshae route, recipient, or later door is available.

| Source | Exact record | Target |
| --- | --- | --- |
| Black Pearl QST | [R I #26201 at line 384](../../../areas/qst/blackpearl.qst#L384) | Moonshae shimmering key #26201 |
| Black Pearl story sidecar | Structured item_vnums at [line 1069](../../../areas/story/blackpearl.story.json#L1069); completion key repeats the output at line 999 | Same Moonshae object; one dependency, not another owner |
| Prototype source | [moonshae.obj #26201](../../../areas/obj/moonshae.obj#L9) | Current object owner |

The remaining Black Pearl QST item inputs and outputs resolve to local
prototypes. No foreign active or source-only QST item command consumes a
Black Pearl object, and no outside story sidecar item_vnums field targets one.
No other external story-sidecar giver/mobile reference into the area's MOB
set was found.

## Reset, shop, and portal references

Every Black Pearl ZON reset-role mobile and object reference resolves to its
local source file; no outside prototype import is declared. No active or
unregistered external ZON reset targets a Black Pearl mobile or object.
Black Pearl has no local SHP record, and the active plus unregistered SHP scan
found no product row for a Black Pearl object. No current object of type 25
or 29 points into rooms #142200–#142283, and no such portal object is owned by
Black Pearl. Exact same-type scans found no other current mobile or object
prototype owner for the local IDs.

## Dormant zone-ship reference

[ship_npc.h line 64](../../../src/ships/ship_npc.h#L64) retains the constant
ZONE_SHIP_ZONE_ENTRANCE = 142201, which names an interior Black Pearl room.
The implementation explains that the zone ship is disabled: the loader call,
interior-linking code, and ship-specific cases are commented out, and
load_zone_ship has no active definition ([ship_npc.c](../../../src/ships/ship_npc.c#L2055),
[ship_base.c](../../../src/ships/ship_base.c#L584), and
[ship_base.c](../../../src/ships/ship_base.c#L1674)). Treat the constant and
commented reference as retained design history, not a running travel edge.
The existing [area-owner question](../zone-stories/THE_BLACK_PEARL.md#direct-area-owner-questions-and-conservative-recommendations)
already asks whether to restore the dreadnought or choose static access.

## Disposition

Keep Black Pearl **DO NOT MOVE** as an active areas/AREA contributor. Preserve
Moonshae ownership of object #26201 and the disabled-ship evidence. This
dependency pass adds no attic candidate row and changes no native gameplay.
The campaign's absent ordinary placement and access questions remain in the
story dossier; source references alone do not qualify item availability,
placement, ship operation, or persisted-player state.
