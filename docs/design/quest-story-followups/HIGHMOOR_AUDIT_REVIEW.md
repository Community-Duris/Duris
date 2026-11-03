# Highmoor: source audit supplement

This supplement records checked-in evidence for active contributor
`highmoor` at source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`.
The [source dossier](../zone-stories/THE_HIGH_MOOR_FOREST.md) summarizes its
environmental stories and builder questions.

## Inventory and narrative sources

`highmoor` appears in `areas/AREA` and `areas/qst/AREA` at `*191-193`
([world roster](../../../areas/AREA#L96),
[QST roster](../../../areas/qst/AREA#L89)). Its zone header is #191, title
“The High Moor Forest,” top VNUM #19357, reset mode 2
([header](../../../areas/zon/highmoor.zon#L1)). The source set contains 258
rooms, 65 mobiles, 62 objects, one QST file, and one ZON file. The WLD, MOB,
OBJ, ZON, and QST sources are present; there is no SHP or story sidecar.

The 258 WLD records contain 66 distinct room-title forms and 134 distinct
description forms after color-code removal. “Lost in the High Moor Forest”
occurs 102 times; “On the Serpentine
River” occurs 35 times. Other clusters cover the Water-Filled Tunnels, the
Serpent’s Bridge, the decaying barge, Yingin’s hut and infirmary, the Ik-Nah
approach, the Osquip gate, Dr. Fester’s chambers, and the Boring Beetle lair
([WLD](../../../areas/wld/highmoor.wld#L21)).

The Blueleaf room links Goldrym’s love of nature to the later evil in the
woods. Object #19101 is reset there and carries carved “G.G.” initials and a
burned eagle-and-ape image ([room](../../../areas/wld/highmoor.wld#L46),
[object](../../../areas/obj/highmoor.obj#L19),
[reset](../../../areas/zon/highmoor.zon#L201)). Two other objects reset in
Yingin’s hut extend this thread. The bark-bound book contains Boriaul’s note,
signed “Second Ranger Knight to the Burgendy Crown” (source spelling), which
blames Goldrym’s experiments for disturbing nature and says Boriaul is traveling to the ruins
to defeat him. The blood-stained papaya journal distrusts Korgakki and
includes a rough map labeling the Serpent Hills, WyrmWood, High Moor Ruins,
Osquip Camp, Grove of Sadness, Bloodleaf clan, and Yingin
([Boriaul’s note](../../../areas/obj/highmoor.obj#L94),
[journal](../../../areas/obj/highmoor.obj#L120),
[book reset](../../../areas/zon/highmoor.zon#L203),
[journal reset](../../../areas/zon/highmoor.zon#L204)). The unholy driftwood-and-bone
shrine holds a lone Blueleaf leaf ([room](../../../areas/wld/highmoor.wld#L721),
[shrine object](../../../areas/obj/highmoor.obj#L188)).

Yingin’s mobile says the missing owl Oaka may have been harmed through
Goldrym’s involvement and places Yingin between the wilderness and nearby
humans ([mobile](../../../areas/mob/highmoor.mob#L266)). Dr. Fester is
described as a Goldrym research scientist pursuing animal hybrids; his QST
responses criticize doctors and clerics who only heal the body
([mobile](../../../areas/mob/highmoor.mob#L693),
[QST](../../../areas/qst/highmoor.qst#L1)). The source does not give those
responses a Q/QA objective or recorded outcome.

Other mobile and room text establishes the researcher Kirus’s fatal attempt
to capture a species, the Bloodleaf lizardmen, Osquip raiders, Ik-Nah
bandits, a damaged bridge with WyrmWood/Cryain Wars lore, and a mobile
boring-beetle colony. The four explicit unfinished room stubs are the
Lizardman settlement (#19233), Osquip camp (#19278), its eastern entrance
(#19289), and the Ik-Nah guild (#19326).

The ZON contains 421 non-S commands: 226 M, 110 E, 48 D, 14 O, 11 F, 6 G,
4 R, and 2 P. It resets Yingin to room #19105, Dr. Fester to #19269, his
deformed son to #19275, and the local animal, river, lizardman, Osquip, and
beetle encounters in their described areas ([ZON](../../../areas/zon/highmoor.zon#L8)).
It places 64 of 65 mobile prototypes and 61 of 62 object prototypes; unused
local definitions are jermlaine gremlin #19115 and rock-encrusted emerald
#19149 ([mobile](../../../areas/mob/highmoor.mob#L251),
[object](../../../areas/obj/highmoor.obj#L811)).
The two sides of the ceiling-tile passage between #19269 and #19275 are
reset closed/secret ([first side](../../../areas/zon/highmoor.zon#L128),
[second side](../../../areas/zon/highmoor.zon#L132)).

The local QST contains only three `M qc_action 50` text responses for giver
#19141; the coverage register records zero native Q/QA contracts. Dr. Fester
is separately listed as an Anatomy epic teacher in compiled code
([QST](../../../areas/qst/highmoor.qst#L1),
[teacher table](../../../src/classes/epic_skills.c#L172)). This is a service
reference, not a quest contract.

## Static connectivity and hazards

The WLD parse has 731 authored exits: 727 positive local targets, one active
Surface target, and three negative targets. All 728 positive targets resolve
to source rooms. There are no positive unresolved targets, range-mismatch
source rooms, or Highmoor room overlaps with other same-role WLD, MOB, OBJ,
or QST sources.

The active reciprocal boundary is Highmoor room #19100 south to Surface room
#526069, paired with Surface room #526069 north to #19100
([Highmoor exit](../../../areas/wld/highmoor.wld#L21),
[Surface room](../../../areas/wld/surface.wld#L547316),
[cross-area rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L310),
[pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L192)). The 95 WLD sources
outside `areas/AREA` add one non-overlapping source-only approach:
Surf room #209922 north to #19100
([Surf source](../../../areas/wld/surf.wld#L208217)). It is not active
ingress. No other inactive source overlaps or targets Highmoor rooms.

The local weak graph has components of 256, 1, and 1 rooms; #19356 and
#19357 are singleton components. Directed traversal from the active Surface
arrival #19100 reaches 252/258 rooms. Authored exits let 255/258 rooms return
to #19100. The six rooms outside the seed’s directed reach are #19352–#19357;
#19354, #19356, and #19357 have no authored path back to the seed. The first
four identify themselves as load/dispersal or boot-result rooms, but their
titles and exits do not establish player access or the reset system’s live
behavior ([WLD endpoints](../../../areas/wld/highmoor.wld#L6666)).

The three negative VNUM records are north, south, and west from room #19327.
Their descriptions say the forest continues endlessly; the loader drops
negative targets before world renumbering
([exit register rows 43–45](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L43)). There
are 48 `C` current records. The two `F` records are 40% fall chances at
waterfall rooms #19271 and #19272, each of which has a downward exit
([first fall](../../../areas/wld/highmoor.wld#L4851),
[second fall](../../../areas/wld/highmoor.wld#L4876),
[loader](../../../src/world/db.c#L1395)). The static census does not simulate
door resets, currents, falls, NPC movement, or player survival.

## Typed dependencies and dynamic travel

The all-source typed scan found the following external links:

| Source | Reference | Classification |
| --- | --- | --- |
| Active Surface WLD | #526069 north to Highmoor #19100 | Reciprocal active WLD route. |
| Source-only Surf WLD | #209922 north to Highmoor #19100 | One inactive approach; retained with Surf, not active ingress. |
| Active Ixarkon ZON and code | Object #96402 is reset in room #96524; `specs.assign.c` binds it to `illithid_teleport_veil`. That procedure’s 25-entry destination table includes Highmoor room #19275. | Code-backed possible arrival in Dr. Fester’s attic, outside the WLD edge census ([reset](../../../areas/zon/ixarkon.zon#L156), [binding](../../../src/specs/specs.assign.c#L1874), [destination table](../../../src/specs/specs.ixarkon.c#L20)). |
| Source-only Brass-old-1 ZON | G-loads Highmoor object #19127 at line 534. #19127 is the fire-hardened copper shield. | Inactive candidate reset dependency, not active stock ([reset](../../../areas/zon/brass-old-1.zon#L534), [object](../../../areas/obj/highmoor.obj#L474)). |
| Compiled epic-teacher table | Mobile #19141 teaches Anatomy. | Active compiled service reference to Dr. Fester ([table](../../../src/classes/epic_skills.c#L172)). |

`specs.ixarkon.c` chooses a destination from the compiled table and resolves
it through `real_room`; #19275 is present in the active Highmoor WLD. The
attic connects to Dr. Fester’s chambers through a closed/secret ceiling-tile
exit. No runtime route or return safety was inspected.

The focused scan found no other external ZON mobile/object reset role, QST
item command, SHP product, story-sidecar VNUM, type-25 portal destination,
same-role prototype definition, or direct C/C++ VNUM reference in the
Highmoor ranges. The two code hits are the Anatomy teacher entry and the
Ixarkon destination above. This static source closure does not cover editor
state, persisted characters/items, indirect construction, or gameplay.

## Title, history, and disposition

After color-code removal, “The High Moor Forest” is unique among all 442
current `.zon` titles. Nearby forest titles belong to separate contributors.
Followed `highmoor` WLD/MOB/OBJ/ZON histories remain on the same source stem
through the tracked 2009 area-tree move and later maintenance (including WLD
edits in 2013 and MOB/OBJ/ZON edits through 2015). No alternate-stem
predecessor, title rename, or explicit whole-area replacement was found;
broader semantic comparison remains open.

Keep active `highmoor` **DO NOT MOVE**. Do not create a Highmoor attic
candidate. Preserve the Surf source-only approach and Brass-old-1 object
dependency in their existing candidate records, and retain the active
Ixarkon code destination until its owner confirms the route.
