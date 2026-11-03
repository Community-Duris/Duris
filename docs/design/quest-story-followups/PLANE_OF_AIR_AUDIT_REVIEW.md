# Plane of Air: connectivity, dependencies, history, and builder decisions

Reviewed October 10, 2026 for active `plane_air_one`, zone 244. The source
dossier is [PLANE_OF_AIR.md](../zone-stories/PLANE_OF_AIR.md).

## Active graph and raw directed reachability

The 125-room WLD contains 749 exits, all resolving in the active loader: 747
local and two outbound. The undirected view has one weak component, while
directed strongly connected components have sizes 122, 1, 1, and 1. Rooms
#24418, #24450, and #24518 have no incoming local WLD edges and can each
follow exits into the 122-room component. No active inbound WLD target is one
of those three rooms. From every active inbound target, raw directed
traversal reaches the 122-room component; that is a map result, not a
qualification of player access or safe return.

All authored exits have zero door flags and key VNUM zero. The source has no
negative or unresolved WLD targets, fall-chance markers, or currents.

| Active inbound source | Direction and destination | Return evidence |
| --- | --- | --- |
| `prison.wld` #7490 | North, east, south, west, and down to #24439; five records ([register rows 159–163](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L159)) | No reverse WLD exit |
| `rftjngle.wld` #80444, “Elemental Ranger Load Room” | South to #24501 ([row 975](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L975)) | No reverse WLD exit |
| `alatorin.wld` #83346, Air conduit | Down to #24460 ([row 1026](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1026)) | No reverse WLD exit |
| `bahamut.wld` #25700, “Behind the East Wind” | West to #24460 ([row 400](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L400)) | Reciprocal with Air #24460 east ([row 397](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L397)) |
| `airp.wld` #131600, “The Heart of the Storm” | Down to #24422 ([row 1300](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1300)) | Reciprocal with Air #24422 up ([row 396](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L396)) |

These are nine records in total. The two Air outbound records are the
reciprocal east/west Bahamut pair and up/down Airp pair. The five Prison
directions share one source room and destination and should not be described
as five independent entrances without builder review.

Code supplies separate, dynamic entry. `planes_room_num` maps Air to
#24401; `cast_gate`, `cast_nether_gate`, and `cast_plane_shift` call
`get_room_in_zone(24401, ch)` for Air destinations
([spell table and Gate](../../../src/magic/spells.c#L295),
[Nether Gate](../../../src/magic/spells.c#L507),
[Plane Shift](../../../src/magic/spells.c#L683)). The helper samples a room
from the zone's loaded real-room range, retrying for
`can_enter_room`, `ROOM_NO_TELEPORT`, and hometown restrictions
([implementation](../../../src/magic/spell_travel.c#L747)). This transport
does not appear in the WLD cross-area register. Spell access, zone state,
arrival eligibility, hazards, and return remain unqualified.

## Inactive Water Plane approaches and identity overlaps

The all-95 inactive-WLD scan found ten south exits from `waterplane.wld`
into Air. Five start at source room VNUMs that collide with active Limbo or
Heavens rooms; keep these as legacy identity overlaps, not unique approaches.
The other five start at non-overlapping source VNUMs and are curated
source-only approaches:

| Water Plane source | Air target | WLD line | Classification |
| --- | --- | ---: | --- |
| #1 | #24444 | 17 | Source #1 overlaps active Limbo #1 |
| #2 | #24446 | 58 | Unique-source record; source-only |
| #3 | #24505 | 99 | Source #3 overlaps active Heavens #3 |
| #4 | #24525 | 140 | Source #4 overlaps active Heavens #4 |
| #5 | #24498 | 180 | Source #5 overlaps active Heavens #5 |
| #6 | #24499 | 221 | Unique-source record; source-only |
| #7 | #24472 | 262 | Unique-source record; source-only |
| #8 | #24473 | 303 | Unique-source record; source-only |
| #9 | #24471 | 344 | Unique-source record; source-only |
| #10 | #24422 | 385 | Source #10 overlaps active Heavens #10 |

The same candidate has five more unique-source WLD records into active
Heavens: #6 west to Heavens #4 (line 229), #7 east to #10 (line 261), #8
west to #5 (line 311), and #9 east to #4 and west to #5 (lines 343 and 352).
Across its 12 defined room records (#1–#10, #12, and #72), the candidate
overlaps active Limbo/Heavens room VNUMs at #1, #3, #4, #5, #10, and #12.
The source-only classification excludes the five Air-targeting records from
overlapping rooms but keeps the five unique-source Air approaches and five
unique-source Heavens approaches.

Preserve these records on existing Water Plane candidate row
[120](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L120), with
`RETAIN IN PLACE`. They are not active ingress, do not change the
candidate's active-inbound count, and do not authorize adding or moving an
area. The Air and Heavens coverage rows now record five and six unregistered
WLD target references respectively; Heavens' prior two-stem list gains
`waterplane`.

## Exact typed dependencies

There is no local Air QST or story sidecar. Exact static references outside
the four local area files are:

| Reference | Source evidence | Classification |
| --- | --- | --- |
| Heavens QST item requirements #24402 and #24403 | Active [Heavens QST lines 70–71](../../../areas/qst/heavens.qst#L70) | The Lightning Earring and Cloak of Mists are foreign QST inputs; their owning contract remains Heavens. |
| Yan-C-Bin #24440 | [Surfacemini sidecar line 242](../../../areas/story/surfacemini.story.json#L242) | Typed as an Air planar essence source, with no topics or Air quest outcome. |
| Smoke portal route #24405 ↔ #139832 | [Air portal prototype](../../../areas/obj/plane_air_one.obj#L97), [Air reset](../../../areas/zon/plane_air_one.zon#L9), [Smoke portal object](../../../areas/obj/smoke.obj#L209), [Smoke reset](../../../areas/zon/smoke.zon#L25), and [Smoke narrative](../../../areas/story/smoke.story.json#L10) | A reciprocal type-25 object route to the older Smoke fresh-air pocket, not a WLD exit. The inactive Duris3 room #139832 is titled “The Ocean”; active Smoke owns the VNUM in the current loader. |
| Yan-C-Bin reset gear #67207, #358, #55200, #14122 | [Air reset block](../../../areas/zon/plane_air_one.zon#L54); owners [unique](../../../areas/obj/unique.obj#L132), [Heavens](../../../areas/obj/heavens.obj#L3709), [White Harbor](../../../areas/obj/wh.obj#L2620), and [Realm](../../../areas/obj/realm.obj#L1524) | Exact object prototype imports into Yan-C-Bin's reset inventory; not proof of a live inventory episode. |
| Lightning Earring #24402 | [CHAOS equipment profiles](../../../src/account/chaos_eq_data.h#L186) | Appears at slot 21 in 18 static equipment profiles; profile data is not a live inventory record. |
| Color pool #24404 to Ethereal #12451 | [Procedure binding](../../../src/specs/specs.assign.c#L1976), [pool handler](../../../src/specs/specs.teleport_pools.c#L107), and [destination room](../../../areas/wld/ethereal_main.wld#L2101) | Code-driven transport with a 200-damage field and a one-hit-point floor for non-trusted characters when damage would be lethal. |
| Sphere of Wind #24406 | [Procedure binding](../../../src/specs/specs.assign.c#L1977) | The object is assigned `lightning`; the assignment is not a quest receipt. |
| Yan-C-Bin helper call | [Procedure binding](../../../src/specs/specs.assign.c#L1221) and [handler](../../../src/specs/specs.elemental_bosses.c#L55) | Combat assistance from local mobile IDs, not a saved story event. |

No external active ZON reset, active shop product, exact duplicate active
mobile/object prototype, or other external type-25 destination into the Air
room range was found. The four local ZON mobile commands for #24401, #24402,
#24404, and #24403 do not resolve to any current mobile prototype; their
VNUMs match local objects and should stay classified as unresolved reset
inputs until the owner confirms intent.

## Title, followed history, and retention

After removing color codes and normalizing case and spaces, **Plane of Air**
is unique among all 442 current `.zon` titles. The nearby `airp` area,
**The Heart of the Storm**, remains a distinct active area and has a reciprocal
route to this map. Followed WLD, MOB, OBJ, and ZON paths stay on
`plane_air_one`, reaching the 2009 move into the current `areas/` tree and
later same-stem edits. The history includes the 2009 Jet Stream sector
correction, 2012–2013 Smoke/planar changes, and 2013 Sphere of Wind and
Yan-C-Bin updates. It identifies no alternate-stem predecessor, tracked
rename, or whole-zone replacement.

Keep active `plane_air_one` **DO NOT MOVE**. Preserve Air/Heavens VNUM
ownership, existing WLD and type-25 routes, quest item identities, local
resets, and the existing Water Plane candidate disposition. No active route,
gameplay, reset, or migration change is proposed.

## Checkpoint 321: builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| AIR-01 | What accepted player objective, if any, should the Air Plane represent? | Preserve the current encounters and descriptions as environmental story until an objective has a defined action, accepted outcome, and durable receipt. |
| AIR-02 | Should a local story sidecar describe Yan-C-Bin or the elemental residents? | Keep the Surfacemini “Air planar essence source” typed note attached to its owner; define any Air story separately and do not infer quest progression from it. |
| AIR-03 | Does “gravity does not seem to exist” promise flight or unrestricted movement? | Keep the prose as description until movement rules and a safe return path are explicitly chosen. |
| AIR-04 | Is Jet Stream #24433 meant to use Fire Plane terrain? | Preserve the current sector 11 value and its 2009 same-stem history. Have the builder confirm the terrain and associated hazards before any edit. |
| AIR-05 | Is room #24406's Fire Plane fragment intended to intrude into Air? | Keep the room in its Air sector and preserve its prose pending a content decision; do not conflate room #24406 with object #24406. |
| AIR-06 | Are the one-way WLD components at #24418, #24450, and #24518 intentional? | Preserve the directed map until an owner confirms whether each staging point needs a player route or is deliberately one-way. |
| AIR-07 | Should Yan-C-Bin reset in #24450, which has no inbound WLD edge? | Preserve the reset and callback; decide whether mobile roaming or dynamic spell arrival is the intended way to meet him before moving his load room. |
| AIR-08 | Do Gate, Nether Gate, and Plane Shift provide ordinary Air access? | Record them as dynamic code routes with sampled landings; qualify destination safety and return separately from the authored WLD graph. |
| AIR-09 | Are the five directions from Prison #7490 to #24439 intentional? | Keep them as five separate authored records from one source room; confirm whether they are a staging exception before calling them player entrances. |
| AIR-10 | Is Rftjngle's ranger load room #80444 meant to send players south to #24501? | Preserve the one-way record and decide its player-facing role with the Rftjngle owner. |
| AIR-11 | Should Alatorin's conduit down to #24460 be one-way? | Preserve the incoming edge and do not promise return travel until an owner defines it. |
| AIR-12 | Are the reciprocal Air #24460 / Bahamut #25700 exits the intended wind connection? | Keep both authored directions and their separate area identities; review access and danger without altering the pair by inference. |
| AIR-13 | Is the reciprocal Air #24422 / Airp #131600 boundary meant to connect the first and second Air areas? | Keep the route and `airp` title distinct; confirm that access and return match the intended campaign sequence. |
| AIR-14 | Should the Smoke portals at Air #24405 and Smoke #139832 remain paired as the older pocket route? | Preserve the type-25 destination and return object. Keep active Smoke's “Pocket of Fresh Air” as current owner while the inactive Duris3 “Ocean” identity remains unresolved. |
| AIR-15 | Is the 200-damage Air color pool #24404 meant to send visitors to Ethereal #12451? | Preserve the current target and one-hit-point floor; verify intended damage, access, and return as a separate route review. |
| AIR-16 | What should happen to the four unresolved `M` commands for #24401, #24402, #24404, and #24403? | Confirm whether the mobile IDs are stale or missing; remove or correct them only after owner review, and do not substitute same-numbered objects. |
| AIR-17 | Are the four foreign objects in Yan-C-Bin's reset inventory intended as his gear and essence set? | Preserve their exact owner identities and reset records; confirm the story relationship before moving, replacing, or promising their availability. |
| AIR-18 | Should Heavens' nightmare-keeper QST continue to request #24402 and #24403? | Preserve the existing exact item requirements in Heavens; review supply and narrative there rather than creating a duplicate Air reward. |
| AIR-19 | Is Yan-C-Bin's shout-and-hunt helper set the intended local response? | Preserve its current compiled mobile list and treat it as combat behavior; any larger faction or story event needs an explicit design. |
| AIR-20 | How should the inactive Water Plane's ten Air targets and five Heavens targets be resolved? | Keep five unique-source approaches to each active area source-only on candidate row 120; keep the five same-VNUM Air records as identity overlaps and resolve map ownership before any migration decision. |
