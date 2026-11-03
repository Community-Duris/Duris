# Plane of Water: connectivity, dependencies, history, and builder decisions

Reviewed October 10, 2026 for active `plane_water_one`, zone 232. The source
dossier is [PLANE_OF_WATER.md](../zone-stories/PLANE_OF_WATER.md).

## Active graph and directed reachability

The WLD contains 750 exits and all resolve in the active loader: 749 are
internal and one leaves the area. Its weak graph is one 125-room component,
and its raw directed graph is one 125-room strongly connected component. The
union of active WLD arrival targets reaches all 125 rooms. No room is missing
a local inbound edge. This is source-graph reachability only; door/reset
state and live access were not tested.

All authored exit flags and key VNUMs are zero. There are no negative or
unresolved targets, fall-chance markers, or currents.

| Active inbound source | Direction and destination | Return evidence |
| --- | --- | --- |
| `prison.wld` #7479 | East, south, west, up, and down to #23255 ([rows 139–143](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L139)) | Five records from one source room; no reverse WLD edge |
| `rftjngle.wld` #80444 | East to #23250 ([row 974](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L974)) | No reverse WLD edge |
| `alatorin.wld` #83345 | Down to #23260 ([row 1025](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1025)) | No reverse WLD edge |
| `seakngdm.wld` #31500 | Up to #23281 ([row 436](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L436)) | Reciprocal with Plane Water #23281 down to #31500 ([row 380](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L380)) |

These are eight inbound records from four source rooms. The only outbound
record is #23281 down to Sea Kingdom #31500; that same-endpoint boundary is
reciprocal. Preserve Prison's five directions as one source-room group, and
keep the three one-way arrivals distinct from the Sea Kingdom pair.

## Code and object routes

These routes sit outside the WLD edge count. They do not establish current
player access, safe arrival, or return.

| Route | Source evidence | Classification |
| --- | --- | --- |
| Gate, Nether Gate, and Plane Shift | Water anchor #23201 in the planar destination array ([spells.c](../../../src/magic/spells.c#L295)); `get_room_in_zone` ([spell_travel.c](../../../src/magic/spell_travel.c#L747)) samples the loaded zone and filters entry, no-teleport, and hometown rooms | Dynamic zone destination; exact landing and return are not fixed by the anchor |
| Plane Water pool #23204 → Ethereal #12492 | Object values ([prototype](../../../areas/obj/plane_water_one.obj#L59)); reset at #23222 ([reset](../../../areas/zon/plane_water_one.zon#L9)); `magic_pool` assignment and handler ([assignment](../../../src/specs/specs.assign.c#L1986), [handler](../../../src/specs/specs.teleport_pools.c#L107)) | Declared route with 200 damage; non-trusted users are left at one hit point if damage would be lethal. No WLD return edge is declared. |
| Negative Plane lake → Water #23233 | Active type-25 object #26658 ([prototype](../../../areas/obj/negplane.obj#L918)) reset at #26723 ([reset](../../../areas/zon/negplane.zon#L168)) | Declared object arrival at the Jet Stream; no Water WLD return edge |
| Ardgral and Ethereal pools → Water #23324 | Ardgral #30407 ([prototype](../../../areas/obj/ardgral.obj#L53), [reset](../../../areas/zon/ardgral.zon#L13), [assignment](../../../src/specs/specs.assign.c#L1938)); Ethereal #12476 ([prototype](../../../areas/obj/ethereal_main.obj#L399), [resets](../../../areas/zon/ethereal_main.zon#L20), [assignment](../../../src/specs/specs.assign.c#L1919)) | Two active type-12 `teleporting_pool` arrivals, not WLD edges |
| Orb of the Sea #23206 → random Water room | Local prototype and reset on Olhydra ([object](../../../areas/obj/plane_water_one.obj#L71), [reset](../../../areas/zon/plane_water_one.zon#L55)); callback assignment ([specs.assign.c](../../../src/specs/specs.assign.c#L1987)); `orb_of_the_sea` ([handler](../../../src/specs/specs.underworld.c#L621)) selects the `WATERP_VNUM_BEGIN`–`WATERP_VNUM_END` range ([utils.h](../../../src/core/utils.h#L1077)) at [line 734](../../../src/specs/specs.underworld.c#L734) | On melee-hit/get-hit events, an eligible victim can be moved to a random loaded Water room at a combined 1-in-90 chance. This is a conditional code route with no declared return path. |

## Inactive-WLD approaches and attic candidates

The all-95 inactive-WLD scan parsed every file without parser issues and
found no inactive source-room VNUM overlap or exit targeting #23201–#23325.
There is no source-only approach into this active map, so curated source-only
counts and candidate active-inbound counts remain unchanged.

Keep the existing `waterplane` candidate row 120 with its previously recorded
approaches into Plane of Air and Heavens. Those old-map records are not active
Water Plane ingress and do not establish that `waterplane` is a replacement
for `plane_water_one`. No candidate row is added or cleared. The detailed
source-story, graph, identity, and dependency findings are in the
[Waterplane candidate audit](WATERPLANE_AUDIT_REVIEW.md).

## Typed dependencies and external consumers

| Reference | Source evidence | Classification |
| --- | --- | --- |
| Heavens fixed items #23202 and #23203 | [Heavens QST lines 94–104](../../../areas/qst/heavens.qst#L94); local prototypes are the black pearl earring and manta-ray cloak ([earring](../../../areas/obj/plane_water_one.obj#L26), [cloak](../../../areas/obj/plane_water_one.obj#L42)) | Active QST item requirements. The nearby singular Poseidon-necklace prose does not name these items; preserve the IDs and ask the Heavens owner to reconcile the story-to-item relationship. |
| Olhydra #23240 | [Surfacemini contact](../../../areas/story/surfacemini.story.json#L256) | Water-planar essence contact without a topic or local quest outcome. |
| Olhydra shout and helper mobiles | `olhydra_shout` assignment ([specs.assign.c](../../../src/specs/specs.assign.c#L1220)) and [helper list](../../../src/specs/specs.elemental_bosses.c#L43) | Compiled callback on #23240; the helper list names Water mobiles #23200, #23210, #23215, #23220, #23230, and #23250. Preserve the VNUMs and callback. |
| Olhydra #23240 and Automatons moonstone fragment | [fragment seeding](../../../src/ships/ship_shop.c#L3628), called during ship initialization ([ship_base.c](../../../src/ships/ship_base.c#L585)) | Startup code may place a fragment on Olhydra or in locker #31724, and loads another fragment on automaton #12027. This is a typed ship-quest consumer, not a local Water QST. |
| Water objects #23202 and #23203 | [compiled object assignment](../../../src/specs/specs.assign.c#L1403) | Both use `artifact_invisible`; preserve the object bindings alongside the fixed Heavens requirements. |
| Water #23204 and #23206 | [object assignments](../../../src/specs/specs.assign.c#L1986) | #23204 uses `magic_pool`; #23206 uses `orb_of_the_sea`. These bindings implement the separate routes above. |

The bounded exact scan found no external duplicate mobile/object definition,
outside active or source-only ZON reset of a local Water prototype, other
QST item command, or SHP product. The active external object routes are the
two typed pool destinations, the Negative Plane type-25 lake, and the Orb of
the Sea code effect already listed above. Preserve the current prototype
owners and consumers; static records do not prove live access or item custody.

## Title, followed history, and retention

After color stripping and whitespace normalization, “Plane of Water” is
unique among 442 current `.zon` titles and 351 active contributors. Followed
WLD, MOB, OBJ, and ZON histories remain on `plane_water_one` from the 2009
move into the current `areas/` tree through later same-stem edits. The MOB,
WLD, and ZON histories include 2012–2015 edits; the OBJ history includes a
2015 Orb of the Sea proc/flag change and a September 2026 gear-stat change.
No alternate-stem rename or whole-zone replacement is established. The
separate inactive `waterplane` file set remains a candidate with its own
identity conflicts and source-only Air/Heavens edges. Sea Kingdom is a
distinct active area whose reciprocal WLD pair is route evidence, not
replacement evidence.

Keep active `plane_water_one` **DO NOT MOVE**. Preserve the Sea Kingdom pair,
the one-way Prison/Rftjngle/Alatorin arrivals, Heavens' #23202/#23203
requirements, Olhydra's callback and ship-quest use, the two incoming pool
routes, the Negative Plane lake, and the damaging Ethereal pool. No route,
reset, gameplay, or migration change is proposed.

## Checkpoint 324: builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| WATER-01 | What player objective, if any, should the local Water map represent? | Preserve the environmental map and encounters until a local action, outcome, and durable receipt are defined. |
| WATER-02 | Should the described shifting currents have an explicit movement mechanic? | Keep the current prose and zero current fields until the area owner confirms whether text or behavior is intended to change. |
| WATER-03 | Is the lightning sphere at #23206 intended to be a Para-Elemental Plane of Lightning fragment inside Water? | Preserve its description and sector pending a decision from the Water and Lightning owners. |
| WATER-04 | Is #23233's Jet Stream the intended arrival point for Negative Plane lake #26658? | Keep both records and verify endpoint, access, and return with both owners before editing either side. |
| WATER-05 | Are Prison #7479's five directions to #23255 intentional alternatives from one room? | Preserve all five and review them as one source-room group. |
| WATER-06 | Should Rftjngle #80444 east to #23250 remain a one-way arrival? | Keep the authored edge and leave return travel unpromised pending owner review. |
| WATER-07 | Should Alatorin #83345 down to #23260 remain one-way? | Preserve the conduit and request route intent before adding a return edge. |
| WATER-08 | Is the reciprocal Water #23281 / Sea Kingdom #31500 pair the intended ordinary connection between the areas? | Preserve both same-endpoint exits and confirm their campaign/access role separately. |
| WATER-09 | Do the eight active arrival records and fully strongly connected raw map reflect the intended player access? | Keep all WLD edges and validate live access, reset state, and safety only through a separate approved gameplay review. |
| WATER-10 | Is Olhydra meant to reset in #23250 with her current earring, cloak, and Orb of the Sea? | Preserve the reset and verify intended loot, item custody, and encounter availability with the Water owner. |
| WATER-11 | Why does the Heavens QST's singular Poseidon-necklace text require a black pearl earring and manta-ray cloak? | Preserve fixed IDs #23202/#23203 and ask the Heavens owner to reconcile the nearby prose with the requirements. |
| WATER-12 | How are Heavens players meant to obtain both required objects from Olhydra or another source? | Preserve the reset association and identify an intended acquisition path before changing either QST or item ownership. |
| WATER-13 | Should objects #23202 and #23203 keep their `artifact_invisible` callback while they are fixed Heavens requirements? | Retain the bindings until their behavior and QST presentation are reviewed together. |
| WATER-14 | Is the Orb of the Sea's 1-in-90 hit-triggered relocation of an eligible victim to a random Water room intended? | Preserve the callback and range; review eligibility, safe arrival, and return as one effect decision. |
| WATER-15 | The 2015 history describes an incompatible-class slip chance, while current code tests it during hit events; is that behavior intended? | Preserve current callback and gear stats until the item owner confirms trigger timing and class policy. |
| WATER-16 | Is Water pool #23204's Ethereal destination #12492 and 200-damage field intended? | Keep the target and damage behavior; review access, the one-hit-point floor, and return together. |
| WATER-17 | Are Ardgral #30407 and Ethereal #12476 intended to arrive at the far map room #23324? | Preserve both active pool consumers and review their access and return with the respective owners. |
| WATER-18 | Should Gate, Nether Gate, and Plane Shift continue to use #23201 as a sampled Water anchor? | Preserve the anchor and helper checks; document landing and return separately if spell behavior changes. |
| WATER-19 | Should Olhydra remain a possible boot-time holder for the Automatons moonstone fragment? | Preserve the ship seeding path and its locker fallback until the Automatons owner confirms holder and recovery intent. |
| WATER-20 | Is unregistered `waterplane` a separate legacy map rather than a predecessor to the current 232-range contributor? | Keep it on candidate row 120 with its existing Air/Heavens source-only edges; do not move or merge either source without owner evidence. |
