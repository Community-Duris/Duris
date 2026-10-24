# Mentiri source and story audit

Canonical key: component:Mentiri. Physical coverage row 421 / CSV line 422;
candidate dependency row 70 / CSV line 71. Reviewed October 11, 2026,
Checkpoint 508, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source inventory,
narrative, WLD graph, typed-reference, identity, and bounded-history review.
It does not establish live availability, travel, combat, or contract outcomes.

## Inventory and authored environmental story

Mentiri is an unregistered source component made up of
[WLD](../../../areas/wld/Mentiri.wld#L1),
[MOB](../../../areas/mob/Mentiri.mob#L1),
[OBJ](../../../areas/obj/Mentiri.obj#L1), and
[ZON](../../../areas/zon/Mentiri.zon#L1) files. The zone header is #1, its
title is “Mentiri, the Prison of Dis,” its last room VNUM is #72, and reset
mode is 2. It defines 72 rooms (#1–#72), 30 mobiles (#1–#30), and 20 objects
(#1–#20). Neither [areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) selects Mentiri. The same-stem QST,
SHP, and story sidecar files are absent; the current areas/RANDOM_AREA file
is empty. There is no evidence that the component is part of the normal
world loader list.

A separate [map JSON](../../../areas/map/Mentiri.json#L1) records positions
for all 72 rooms and has a lastModified value of 2025-11-30. Keep it with the
source identity until the editor or map owner confirms whether it is a
canonical project file or an export.

The room titles establish an infernal prison and fortress setting. The first
rooms descend from a wicked passage into Dis, through iron-spiked caves and
forged-iron halls, toward Mentiri’s gates. Later named areas include the
hallway and hall of judgement, throne, guards’ barracks, an armory of the
Sundered Flame Battalion, an iron-bound door, and a molten slag pit/pot
complex. WLD room description bodies are empty. Thirty-nine room titles are
literal “Unnamed”: #12 and #35–#72. The remaining titles give landmarks but
little detail about events, access rules, or residents.

Mobile text adds a small number of story anchors: an unnamed
devil roster, a shade, demons, and abishai; Murmur is titled
Commander of the Sundered Flame at #18; Vesper guards the gate at #19;
Solarious is a captured lost archon chained at #20; #21 is an elite pit-fiend
guardian; and #22 is a three-headed hell hound. Most other mobile descriptions
are the same generic lonely-mob placeholder, including Murmur’s. The source
therefore establishes a setting and a few named figures, but it does not
explain why Solarious was captured, what authority Murmur’s title carries, or
how a visitor is meant to interact with the story.

All 20 object records repeat generic unnamed names and descriptions. The
seven ZON records are mobile placements only; they place #20 once, #19 once,
#22 twice, and #21 three times. No object placement, native QST contract, or
shop stock is authored in this same-stem package. Do not treat the source
setting as proof of a missing quest or shop; ask the owner whether this is an
intentional environmental story, incomplete work, or a source set kept for
another purpose.

## WLD graph and current numeric lookup

The source WLD has 160 direction records, no malformed exit tuples, no
repeated direction number in a room, and at least one direction from every
room. Every target is positive and lies in Mentiri’s own #1–#72 room set; no
WLD exit leaves that band. The local graph has one weak component containing
all 72 rooms, and raw directed traversal from #1 reaches all 72. No F fall
chance or current field was found. These are authored WLD relationships,
not a live travel test.

Comparing target numbers with the current full-world room table gives 106
numeric matches and 54 positive targets without a current room definition.
The matched VNUMs are #1, #3–#5, #10–#46, #66, and #69. The current active
owner of #1 is Limbo; the other matched numbers are owned by Heavens. Every
complete Mentiri room block at these active-overlap IDs differs from the
active record. Thus the 106 lookups do not load or identify Mentiri’s local
target rooms. For example, Mentiri #2 points west to its own room #1, “Above
a Wicked … into the Depths of Dis,” while the active #1 definition belongs
to Limbo.

Seventy current active WLD directions from Heavens, Desert, and Thetis
numerically target VNUMs in Mentiri’s #1–#72 band. Those numbers currently
resolve to active Limbo or Heavens room records, so they are not identifiable
Mentiri inbounds. The candidate register records zero active inbounds to
Mentiri. No candidate WLD direction targets an external room, and no
candidate-specific reverse boundary was found. Keep the low-number matches
separate from the source-only Mentiri graph.

## Resets, source identity, and history

The seven M rows in the source-only ZON reference only local mobile and room
VNUMs. The reset file has no O/G/E/P item placements or other typed outbound
room imports. No same-stem QST or SHP is present. A bounded exact-name search
found no external Mentiri, Solarious, Vesper, or Sundered Flame code binding;
low numeric references elsewhere cannot identify Mentiri’s inactive
prototypes because the same-number prototypes have other source owners.

Every Mentiri WLD room VNUM has another same-type definition somewhere in the
current areas source set: 72 IDs with 389 same-number source-block pairs,
including active owners for 43 IDs. Every mobile VNUM also appears in another
MOB source: 30 IDs with 132 pairs, including active owners for 29. Every
object VNUM appears in another OBJ source: 20 IDs with 103 pairs, including
active owners for all 20. All complete blocks differ from the active
same-number Limbo/Heavens owner. No same-type pair proves a shared canonical
identity or replacement.

The separate minimal-mode [areas_mini fixture](../../../README.md#L309)
reuses 27 Mentiri room IDs, 24 mobile IDs, and all 20 object IDs in
mini.wld, mini.mob, and mini.obj. Those complete blocks also differ from the
Mentiri records. The miniature dataset is a separate source configuration,
not evidence that Mentiri is selected by the full areas/AREA list.

Followed WLD, MOB, and OBJ histories first show their current Mentiri paths
being tracked in 2011, with same-stem edits through 2013 and an OBJ quest-item
flag pass in 2015. The ZON history reaches the 2005 initial revision and
same-stem edits through 2011. These histories establish continuity of the
tracked paths only; they do not identify an original editor copy, owner, or
runtime deployment. No alternate-stem rename, retirement decision, distinct
predecessor, or whole-zone replacement was found. The exact source title is
“Mentiri, the Prison of Dis”; the separate “Hell, Dis” title and named Hell
planes do not establish a replacement relationship.

**Disposition: RETAIN IN PLACE.** Preserve the four source files and the
72-position map JSON outside the full and quest area lists. Resolve the
component’s intended status, blank/placeholder records, active and
areas_mini same-number identities, entry route, reset scope, and editor or
deployment copies with the responsible owners before activation, renumbering,
archive, merge, or path move. No migration clearance is implied. See the
[22 builder questions](MENTIRI_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-508-mentiri-low-vnums-remain-distinct-from-active-owners),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-508-mentiri-shared-low-vnums-and-no-replacement-evidence),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-508-mentiri-retained-pending-owner-and-tool-closure),
[candidate dependency row 70](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L71), and
[coverage row 421](ZONE_COVERAGE_REGISTER.csv#L422).

### Checkpoint 919: Mentiri exact cross-source role closure

The CP508 source inventory and candidate WLD graph remain unchanged: 72
rooms, 160 local directions, one weak component, and directed reach from #1
to all 72 rooms. An all-source WLD target scan found 939 other-file target
records in the #1–#72 number band. The 70 active records all resolve to
current active room definitions in Limbo or Heavens. Of 869 unlisted records,
868 target a VNUM also defined in their own source WLD; the one non-local row
is [Surf #201670 D5 → #36](../../../areas/wld/surf.wld), which resolves to
active Heavens #36. These numeric overlaps add no Mentiri inbound edge; the
candidate WLD has no outbound edge and the active inbound count remains zero.

The active ZON pass found two mobile-role matches in Mentiri’s #1–#30 band.
One is local [Heavens M #17](../../../areas/zon/heavens.zon#L285), with the
matching [Heavens mobile #17](../../../areas/mob/heavens.mob); the other is
[Heavens ZON line 286](../../../areas/zon/heavens.zon#L286), M #2, which has no
Heavens MOB #2 definition and resolves by number to [active Limbo #2](../../../areas/mob/limbo.mob#L12). Mentiri
#2 and the separately audited Mavulsk #2 are different unlisted prototypes.
Seventy-two active ZON room-role fields fall in the Mentiri room band, and all
use rooms in their own same-stem active WLDs. No active reset targets an
unlisted Mentiri room.

Twenty active ZON primary object arguments fall in Mentiri’s #1–#20 OBJ
range. Eighteen are cross-source references to #5 from [Alatorin](../../../areas/zon/alatorin.zon),
[Harrow](../../../areas/zon/harrow.zon), [Player Castles](../../../areas/zon/player_castles.zon),
and [Tharnadia](../../../areas/zon/tharnadia.zon); one references #17 from
[Nyneth3](../../../areas/zon/nyneth3.zon); one Heavens reset references its
own #10. The current active object identities are [Limbo #5](../../../areas/obj/limbo.obj),
[Heavens #17](../../../areas/obj/heavens.obj), and Heavens #10. The Mentiri prototypes are
separate unlisted records. The active reset scan found no P-container field
in #1–#20. Mentiri’s own seven ZON commands remain mobile placements only and
do not import objects or external room VNUMs.

QST files contain nine giver-key matches in #1–#30: one active Heavens #17
with its same-stem Heavens MOB, plus eight unlisted keys in Dalvik, Hostel,
and Mavulsk. Mentiri has no same-stem QST. Active QST item commands contain
34 `G I` references in #1–#20: #5 from [Alatorin](../../../areas/qst/alatorin.qst)
and [Tharnadia](../../../areas/qst/tharnadia.qst), #8 from
[SurfaceKeeps](../../../areas/qst/surfacekeeps.qst), and #18/#19 from Alatorin.
Those numbers resolve to active
Limbo or Heavens object identities; the QST rows do not identify Mentiri’s
generic placeholder objects. A structural scan of 232 story JSON files found
14 `item_vnums` references in three active sidecars: [Alatorin](../../../areas/story/alatorin.story.json)
12, [SurfaceKeeps](../../../areas/story/surfacekeeps.story.json) 1, and
[Tharnadia](../../../areas/story/tharnadia.story.json) 1. They use the same
#5, #8, #18, and #19 numbers; no
Mentiri-range room, mobile, or giver role field matched.

The parser-valid N-format SHP scan found three product references in the
Mentiri object range, all #5 in [Alatorin](../../../areas/shp/alatorin.shp),
[Harrow](../../../areas/shp/harrow.shp), and
[Player Castles](../../../areas/shp/player_castles.shp). Each uses active
Limbo object #5. No keeper VNUM falls in #1–#30; the sole room role match is
[Heavens shop room #23](../../../areas/shp/heavens.shp). Mentiri has no same-stem SHP, and these
values do not establish an armory, keeper, or stock identity for Mentiri.

The maintained
[ITEM_TELEPORT handler](../../../src/magic/spell_travel.c#L991) treats
`value[0]` as a room VNUM and resolves it with `real_room()`. Sixty-four
external type-25 object values fall in Mentiri’s #1–#72 band; 60 are in active
source objects. Six active values have no current WLD destination and target
three numbers also defined by other unlisted WLDs:

| Target | Active source object fields |
| --- | --- |
| #2 | [githzer.obj](../../../areas/obj/githzer.obj) #44488, #44493, #44573 |
| #9 | [crypt.obj](../../../areas/obj/crypt.obj) #14547 |
| #50 | [fields_between.obj](../../../areas/obj/fields_between.obj) #71002; [halfcut.obj](../../../areas/obj/halfcut.obj) #27035 |

The other 54 active-source values currently resolve by number in the active
WLD table. None of these object fields identifies Mentiri as its intended
destination or changes WLD connectivity.

A bounded source-code scan found 25 literal `real_room`, `real_room0`,
`real_mobile`, or `real_object` calls in Mentiri’s low-number bands, across 18
files, and no exact Mentiri, Solarious, Vesper, or Sundered Flame code-name
hit. The superficially similar [prison callback](../../../src/specs/specs.prison.c#L90)
sends its temporary character to current Limbo room #1; [the underworld item handler](../../../src/specs/specs.underworld.c#L2048)
uses current Heavens objects #18/#19 for sword and staff effects. The
[room assignments](../../../src/specs/specs.assign.c#L2269) to #17/#42 and
[Heavens callback](../../../src/specs/specs.heavens.c#L4950) at #44 likewise
refer to active Heavens rooms. None binds the unlisted Mentiri source.

No rename, predecessor, retirement, or whole-zone replacement evidence was
added. **Disposition remains RETAIN IN PLACE.** Preserve the four source
files and 72-position map JSON, the distinct active Limbo/Heavens identities,
and the unresolved #2/#9/#50 ITEM_TELEPORT destinations. Keep the separate
areas_mini data and do not infer a route, shop, or contract from low-number
matches. See [MT-15 and MT-17](MENTIRI_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-919-mentiri-source-only-targets-do-not-add-active-ingress),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-919-mentiri-cross-source-roles-do-not-establish-lineage),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-919-mentiri-cross-source-roles-remain-under-retention),
[coverage row 421](ZONE_COVERAGE_REGISTER.csv#L422), and
[dependency row 70](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L71).
