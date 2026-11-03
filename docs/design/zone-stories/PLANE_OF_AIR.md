# Plane of Air: comprehensive source story map

Reviewed October 10, 2026. Active source area `plane_air_one`, zone 244.
Its WLD defines 125 rooms, its MOB seven prototypes, its OBJ eight prototypes,
and its ZON contains the reset set. There is no local QST file or story
sidecar. The full source, graph, typed references, title/history review, and
twenty builder decisions are reconciled in the
[Checkpoint 321 audit review](../quest-story-followups/PLANE_OF_AIR_AUDIT_REVIEW.md).

## Evidence boundary

Reviewed the complete [room map](../../../areas/wld/plane_air_one.wld),
[mobile prototypes](../../../areas/mob/plane_air_one.mob),
[objects](../../../areas/obj/plane_air_one.obj), and
[resets](../../../areas/zon/plane_air_one.zon). The active
[area manifest](../../../areas/AREA) loads the area. No
`plane_air_one.qst` or `plane_air_one.story.json` exists.

The route and behavior findings also read the shared
[plane-travel code](../../../src/magic/spells.c#L295),
[random room selector](../../../src/magic/spell_travel.c#L747),
[planar mobile procedure](../../../src/specs/specs.elemental_bosses.c#L55),
[object procedure bindings](../../../src/specs/specs.assign.c#L1976), and
[pool teleport handler](../../../src/specs/specs.teleport_pools.c#L107).
Those static sources identify alternate movement and combat behavior; they do
not establish live resets, player eligibility, safe arrival, or successful
return.

## A windy region with no local quest receipt

The common room prose describes wind, drifting rocks and smoke, absent gravity,
and travel in any direction. The source data does not encode flight or free
three-dimensional movement in those words. Most of the 125 rooms use the Air
Plane sector. Room #24433, titled **A Jet Stream**, instead uses the Fire Plane
sector. The followed WLD history records that value changing from Inside to
Fire Plane in 2009; preserve that exact source fact pending an owner decision.

Two rooms intentionally interrupt the repeated landscape text. Room #24406,
**Edge of a Sphere of Fire**, describes a molten fragment of the Fire Plane
floating in Air, while #24433 describes a chromatic jet and popping rocks.
Room and object VNUMs are separate namespaces: object #24406 is the
**Sphere of Wind**, assigned the `lightning` object procedure. The room's
fire-fragment prose does not identify that item or make the room a Fire Plane
sector.

The local file set has no QST or story sidecar, no item turn-in, and no recorded
achievement. The seven mobile prototypes and their resets give the region
hostile elementals, djinn, jann, vortices, winter wolves, and Yan-C-Bin. Those
encounters can support a future story, but their dialogue and combat source do
not record a completed task.

## Yan-C-Bin and the resident forces

Mobile #24440 is Yan-C-Bin, Prince of the Elemental Plane of Air. The ZON
resets him at room #24450, equips his cloak of mists #24403 and lightning
earring #24402, and gives him the local Sphere of Wind #24406 plus four
objects owned by other areas:

| Object | Current source owner | Source name |
| --- | --- | --- |
| #67207 | `unique.obj` | Staff of Air Conjuration |
| #358 | `heavens.obj` | Rune-covered stone |
| #55200 | `wh.obj` | Essence of Yan-C-Bin |
| #14122 | `realm.obj` | Billowing wind of a forge |

The foreign prototypes are exact typed reset references, not evidence that
each item is currently generated or present on a live Yan-C-Bin episode. The
local reset block and each source owner are linked from the
[audit review](../quest-story-followups/PLANE_OF_AIR_AUDIT_REVIEW.md#exact-typed-dependencies).

The compiled `yancbin_shout` binding makes Yan-C-Bin periodically call
`shout_and_hunt` with the local vortex, air elementals, djinni, jann, and
winter-wolf VNUMs as helpers. That is a combat response, not a quest contract.
The external `surfacemini` story sidecar calls him an “Air planar essence
source” but defines no topics. Heavens' active QST uses his lightning earring
#24402 and cloak #24403 in its own nightmare-keeper chain. Keep those external
contracts attached to their owning areas.

The ZON also contains four `M` records for mobile VNUMs #24401, #24402,
#24404, and #24403. None has a matching local or external mobile prototype;
these values coincide with local object VNUMs. Do not treat the records as
valid spawns or resolve them by crossing object/mobile namespaces.

## Connected routes and the older Smoke pocket

The authored WLD graph connects 125 rooms in one weak component. Its largest
directed strongly connected component has 122 rooms. Rooms #24418, #24450,
and #24518 have no incoming authored WLD edge; each can follow exits into the
122-room component. Yan-C-Bin's reset room #24450 is one of these one-way
staging points. The nine active inbound WLD records all target the large
component, and both outbound WLD exits are reachable from it.

Three planar spells use room #24401 as the Air zone anchor and call
`get_room_in_zone` when selecting a destination. That helper samples rooms
from the loaded zone's real-room range, subject to entry, no-teleport, and
hometown checks. Spell travel therefore has behavior outside the authored
WLD graph. The helper and spell code do not certify that a sampled landing
room is safe or that it has a player-usable return.

The old Smoke connection is a separate object route. The ZON places portal
#24407 at Air room #24405, and its type-25 destination is the Smoke Plane's
fresh-air pocket #139832. The Smoke blue portal #139821 in that pocket points
back to #24405. The Smoke story sidecar explicitly distinguishes this older
portal pair from the later Air Plane area and Plane of Fire. In the current
loader, active Smoke owns #139832; inactive `Duris3.wld` has a same-numbered
room titled **The Ocean**, so preserve the existing map-identity caveat.

Another separate code route is the local grey color pool #24404, reset at
#24422. Its `magic_pool` procedure targets Ethereal room #12451 and has a
200-damage value; non-trusted characters are left at one hit point when that
damage would be lethal. This object route is not an Air WLD exit or proof of
a return path.

## Story boundaries

The Air Plane can safely be described as a dangerous elemental region with
distinct Fire and Smoke connections, hostile residents, and a powerful
planar prince. Current source does not specify an ordered campaign, a required
boss kill, an Air Plane reward receipt, or a persistent outcome. Treat the
descriptions of flight, gravity, or elemental allegiance as narrative until a
builder defines an accepted action and durable result.

The normalized title **Plane of Air** is unique among all 442 current zone
titles. Active `airp`, **The Heart of the Storm**, is a distinct later
destination with a reciprocal WLD boundary to this area. Followed WLD, MOB,
OBJ, and ZON histories remain on `plane_air_one`; they do not establish an
alternate-stem predecessor or whole-zone replacement. Keep the active area
**DO NOT MOVE** and retain all existing VNUMs until the open builder questions
are resolved.
