# Wellvolen repository-wide audit review

Checkpoint 430; physical coverage row 343, area:wemic. This supplement
completes the source-backed story, directed graph, typed-reference, title,
bounded-history, active-area disposition, and builder-question reviews. It
accompanies the [source dossier](../zone-stories/WELLVOLEN_STORY_REVIEW.md)
and [builder questions](../zone-stories/WELLVOLEN_BUILDER_QUESTIONS.md). No
area source or gameplay file was changed.

## Active source and authored purpose

The active manifest lists wemic at [areas/AREA](../../../areas/AREA#L330).
Zone 935 is titled Wellvolen and contains 47 rooms, 11 mobiles, and 10
objects. WLD, MOB, OBJ, and ZON files are present. The [QST index](../../../areas/qst/AREA#L292)
lists wemic, but there is no local Wemic QST file, SHP file, or story sidecar.
The area uses reset mode 2 and has 76 ZON commands: 54 M, 21 E, and one O.

The room and mobile prose describe a Wemic settlement around a spring, with
primitive huts, hot sands, training grounds, a leader, craftspeople, children,
spiritual roles, a nursery, a watering hole, an altar, and a painted artists’
trail. The feeding pens and several huts contain explicit remains and
sacrificial imagery. This is environmental story; it does not define a local
contract, shop, or reward.

## Active WLD graph, directed reach, and approaches

The parser resolves all 93 authored Wemic exits: 92 are local and one leaves
for active Surface. There are no negative or unresolved targets or
range-mismatch rooms. The local graph is one weak component of 47 rooms.
Under open-edge local traversal, both #93501 and #93514 reach every Wemic
room, and every room can return to either start within the local graph.

The active cross-zone records are the reciprocal Surface pair
Surface #572118 east to Wemic #93501 and Wemic #93501 west to Surface #572118,
plus a one-way WH #55614 south exit to Wemic #93514. The WH source room is
explicitly a zone-dispersal room for mobs associated with the Bipplewizzlille
potion quest. The active Bipplewizzlille QST also names Wellvolen among zones
Incarnates may visit. These facts support a mob-distribution relationship but
do not establish ordinary player ingress, actual appearance, or encounter
odds. There is no Wemic return edge to WH. See the
[Surface-to-Wemic record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1492),
[Wemic-to-Surface record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1126),
[WH-to-Wemic record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L765),
[WH source room](../../../areas/wld/wh.wld#L12606), and
[WH quest rumor](../../../areas/qst/wh.qst#L783).

The full scan of 95 inactive WLD files found one additional source-only
approach: inactive surf.wld room #229296 east to Wemic #93501. No room-number
overlap was found. It is not active ingress; preserve it under existing
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Curated
source-only WLD totals rise to 1,507 overall and 124 for Surf. See the
[inactive source room](../../../areas/wld/surf.wld#L614548).

No Wemic falls, current fields, or D reset commands were found. The source
graph does not settle player admission, WH dispersal semantics for players,
live reset or movement behavior, or safe travel.

## Typed dependencies and external consumers

All Wemic M, E, and O resets resolve to local prototypes. One O reset places
fountain #93500 in watering-hole room #93531. Six M placements create Wemic
warrior mobile #93501 and six corresponding E resets equip wicked mace
object #93501 at village locations
([first equipment pair](../../../areas/zon/wemic.zon#L15),
[mace object](../../../areas/obj/wemic.obj#L9),
[warrior mobile](../../../areas/mob/wemic.mob#L14)). The equal VNUM is a
cross-type local identity, not a same-type duplicate.

The exact external numeric scan found the Shipyard QST’s three item
requirements for object #93501 and matching story-sidecar contract references
([QST](../../../areas/qst/shipy.qst#L697),
[story sidecar](../../../areas/story/shipy.story.json#L895)). The authored
recipe also asks for three exact tundra-panther teeth; see the
[Shipyard source-stock review](../zone-stories/SHIP_YARDS.md#source-stock-slots-caps-and-competing-consumers).
The local static cap of six equipped maces is not a live acquisition guarantee.

No other active external ZON reset, external SHP product, same-type mobile or
object definition, type-25 portal target, or C/C++ direct VNUM reference into
the Wemic room/mobile/object sets was found. The WH QST’s place-name mention
is narrative context, not a numeric prototype reference. The inactive Surf
approach remains a separate source-only map dependency.

## Title, history, and owner boundary

After color-code removal, the active ZON title Wellvolen is unique among the
351 active area titles. The followed WLD, MOB, OBJ, and ZON histories stay on
the wemic stem from the tracked 2009 areas-tree relocation through later
same-stem edits. No alternate-stem predecessor, title rename, or whole-zone
replacement was identified. The command-target alias/owner-disambiguation
field remains separate from the bounded title/history result.

The Surface boundary, WH mob-dispersal link, Shipyard item use, and WH QST
rumor connect independently owned content. They do not transfer area
ownership or establish a predecessor/replacement relationship. Keep Wemic,
Surface, WH, Shipyards, and the inactive Surf map source as separate owners.

## Safe disposition

Keep active wemic **DO NOT MOVE** as a current full-area contributor. Preserve
the reciprocal Surface boundary, the one-way WH dispersal reference, the local
Wemic prototypes and equipment resets, and Shipyard’s typed use of object
#93501. Keep inactive surf.wld **RETAIN IN PLACE** under existing candidate
row 97 and preserve its source-only #229296 east edge to #93501. The curated
source-only totals rise from 1,506 to 1,507 overall and from 123 to 124 for
Surf. No Wemic-specific attic candidate or migration clearance is warranted.

The static story, topology, exact typed-reference, title/history, and
builder-question reviews are complete. Ordinary player access, live
dispersal, reset availability, and safe-return behavior remain unqualified.
No tests, builds, server, gameplay, database, migration, or operational
commands were run.
