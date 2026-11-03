# Arbre's Forest: source-story map

Coverage row 264 maps active contributor `sforest`, titled “Arbre's Forest,”
with 24 rooms (#97400–#97423). This dossier records the prose, named
characters, reset placements and source-declared routes. It does not establish
live access or a quest outcome. See the [Arbre's Forest audit supplement](../quest-story-followups/SFOREST_AUDIT_REVIEW.md) for full connectivity, typed dependency, title/history and builder review.

## Story shape

The area is a small, damp forest crossed by a winding path. The canopy varies
from thick and rain-soaked to thinner, sunnier stretches. Mud, puddles,
flowers, fungi, pollen, thorny vines, insects, birds and small animals fill
the descriptions. Large boulders and a stream interrupt the trail. The
authored tone is a quiet, fertile woodland, with a few signs of trouble
among the travelers who use it.

Three small settlements or shelters give the map its human story. In a cared
for grove, Arbre, “The Wandering Druid,” keeps company with forest creatures
and tends his home. A second hut stands in a soggy bog beside a fishing boat;
Istbanlor, the hermit, lives there. A lost merchant wanders the path, while
Valnar, called a renegade rogue, hides under the bridge and is avoiding the
authorities. The bridge itself is worn but maintained, crosses a clear trout
stream, and has a “Contribution Box” sign on each bank. These are narrative
leads in room and mobile descriptions: there is no local QST file or custom
dialogue recording a rescue, contribution, or other completion.

## Narrative landmarks

| Thread | Source evidence | Mapping note |
| --- | --- | --- |
| Surface approaches and forest path | [WLD rooms 97400–97404](../../../areas/wld/sforest.wld#L1) | Two reciprocal Surface connections meet the path at opposite ends. The overgrown entry, mud, canopy and animal life frame the area. |
| Arbre's grove | [Room 97407](../../../areas/wld/sforest.wld#L157), [Arbre mobile](../../../areas/mob/sforest.mob#L14), [local resets](../../../areas/zon/sforest.zon#L16) | Arbre is loaded at the grove with a tree-shaped amulet and leaf mask. His prototype describes tending the grove and talking with plants and animals. |
| Bog hut and hermit | [Room 97413](../../../areas/wld/sforest.wld#L254), [Istbanlor mobile](../../../areas/mob/sforest.mob#L71), [hut/reset rows](../../../areas/zon/sforest.zon#L9) | The room has a small boat; Istbanlor and a wooden hut are both reset there. The grove's separate leaf-and-stick hut appears only in prose. |
| Lost travelers and hidden rogue | [Lost merchant mobile](../../../areas/mob/sforest.mob#L43), [under-bridge room 97421](../../../areas/wld/sforest.wld#L420), [Valnar mobile](../../../areas/mob/sforest.mob#L85) | The merchant has no local quest record. Valnar's text and under-bridge placement both describe evasion from authorities. |
| Stream bridge and contribution boxes | [Bridge room 97420](../../../areas/wld/sforest.wld#L399) | Room text names boxes on both banks, but no local object prototype represents them. Their visibility or interaction is unresolved. |

## Native quest and object boundary

[`areas/qst/AREA`](../../../areas/qst/AREA#L309) lists `sforest *974`, but there is no
`areas/qst/sforest.qst`, local SHP file or `areas/story/sforest.story.json`.
The active ZON has reset mode 2 and 23 literal reset rows: 17 mobile loads,
five equipment loads and one wooden-hut object load. None is a native quest
step. Six local object prototypes include the tree amulet, leaf mask, leather
coat, wooden hut, moldy eyepatch and black dagger; their reset placements
associate the amulet/mask with Arbre, coat with the lost merchant, eyepatch
with Istbanlor, and dagger with Valnar. The wooden-hut object is placed in
the bog room, not Arbre's grove.

The two banks of the bridge are described as having contribution boxes, but
the local object list and resets have no box. Likewise, the text says the
thorny vine blocks travel and describes a puddle as difficult to cross, while
the WLD exits themselves carry no nonzero exit flags or local door resets.
These differences are builder questions; prose alone does not create an
interactive object, blocked exit, trap, or quest.

## Static map and disposition

The 24-room WLD has 48 exits, all resolving, unflagged and locally connected.
The directed graph has one 24-room component; from room #97400, all 24 rooms
are reachable and all 24 have a path back. Surface room #559312 enters
`sforest` #97400 and is met by the reciprocal north exit. At the other end,
`sforest` #97423 south enters Surface #560911 and Surface has the reverse
north exit. These source declarations do not qualify gameplay access or
safety.

Two more approaches are present only in inactive `surf.wld`: #208170 south
to #97400 and #209066 east to #97423. They remain source-only records on the
existing Surf candidate row. There are no fall-chance or current-marked rooms,
local flagged exits, D resets, type-25 object destinations into the forest,
or source-backed scripted behaviors found in the bounded scan.

Keep active `sforest` in place. Preserve both reciprocal Surface boundary
pairs and the two distinct inactive Surf routes. No migration clearance or
gameplay repair is proposed.
