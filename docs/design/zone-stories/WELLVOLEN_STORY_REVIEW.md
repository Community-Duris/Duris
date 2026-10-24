# Wellvolen: the Wemic village around a desert spring

Canonical key: area:wemic. Physical coverage row 343. The active area is zone
935, titled “Wellvolen,” with 47 rooms (#93501–#93547), 11 mobile prototypes
(#93500–#93510), and 10 object prototypes (#93500–#93509). Its active files are
[WLD](../../../areas/wld/wemic.wld), [MOB](../../../areas/mob/wemic.mob),
[OBJ](../../../areas/obj/wemic.obj), and
[ZON](../../../areas/zon/wemic.zon). The current manifest lists wemic at
[areas/AREA](../../../areas/AREA#L330). The [QST index](../../../areas/qst/AREA#L292)
also lists wemic, but this checkout has no local wemic.qst, SHP file, or story
sidecar. Index membership alone does not establish a local quest.

## The authored village

The entrance at #93501 is a sandy path beside boulders and a spring; it opens
west to the active Surface area and east into the village
([entrance](../../../areas/wld/wemic.wld#L1)). The paths lead among primitive
huts built from stone, jungle wood, desert plants, and snakeskins. The source
describes hot daytime sand and training grounds for young Wemics
([central path](../../../areas/wld/wemic.wld#L46)).

The settlement is presented as a functioning community, with a leader’s hut,
potter, children’s nursery, artist trail, watering hole, shaman’s huts,
conjurers’ homes, and a sacred altar. The watering-hole path describes daily
bathing, water collection, cooking, and conversation; room #93531 describes a
clear spring among rocks, and the ZON resets the local fountain there
([path](../../../areas/wld/wemic.wld#L481),
[spring](../../../areas/wld/wemic.wld#L584),
[fountain reset](../../../areas/zon/wemic.zon#L9)). The nursery is described
as child care while parents work or hunt
([nursery](../../../areas/wld/wemic.wld#L626)). The altar path and altar room
describe night ceremonies, rituals, incense, and sacrificial tables
([ceremonial path](../../../areas/wld/wemic.wld#L640),
[altar](../../../areas/wld/wemic.wld#L695)). The artist trail describes
painted sand, carved huts, and totem poles
([trail](../../../areas/wld/wemic.wld#L711)).

The environment also includes a deliberately harsh feeding ground. Rooms
#93517–#93519 describe bloody sand, slaughtered humans and other creatures,
corpses, and people stacked as food
([feeding pens](../../../areas/wld/wemic.wld#L312)). Several huts contain
remains, animal parts, or ritual materials; room #93547 describes severed
heads and an animal hide, while the conjurers’ rooms list insect parts, bat
guano, books, charts, and beakers
([shaman’s hut](../../../areas/wld/wemic.wld#L859),
[conjurers’ homes](../../../areas/wld/wemic.wld#L827)).

Named mobiles give the settlement a civic and spiritual structure: Mnenar is
the town leader, Llaranar the arch-shaman, Palom the potter, Lona the high
priest, and Valm a conjurer. Other prototypes are children, commoners,
warriors, mercenaries, acolytes, and conjurers. Their descriptions and
placements establish inhabitants and roles; no local QST or SHP defines a
service or contract for them
([mobile prototypes](../../../areas/mob/wemic.mob#L1),
[named reset placements](../../../areas/zon/wemic.zon#L53)).

## Local objects and the Shipyard contract

The ten local objects are a fountain, wicked-looking mace, belt, robes, apron,
sand visor, leggings, sleeves, ring, and hood
([object prototypes](../../../areas/obj/wemic.obj#L1)). The zone file has 76
reset commands: 54 M, 21 E, and one O. Six Wemic warrior resets are each
paired with an E reset that equips object #93501, the wicked-looking mace;
the mace does not appear as an ordinary loose-object reset
([first warrior and equipment pair](../../../areas/zon/wemic.zon#L15),
[last pair](../../../areas/zon/wemic.zon#L51)).

That equipment is consumed by an external Shipyard contract. Vrash asks for
three wicked maces found on Wemic warriors and three tundra-panther teeth. The
QST completion commands name object #93501 three times, and the Shipyard
story sidecar records the three-item requirement and source hint
([request and completion](../../../areas/qst/shipy.qst#L685),
[typed completion](../../../areas/qst/shipy.qst#L695),
[story contract](../../../areas/story/shipy.story.json#L891),
[source-stock review](SHIP_YARDS.md#source-stock-slots-caps-and-competing-consumers)).
Object #93501 and mobile #93501 are different local prototype types, not a
same-type duplicate. The static reset cap and QST text do not establish live
availability, successful recovery, or simultaneous stock.

The active Bipplewizzlille rumor also names Wellvolen among the zones where
Incarnates may appear
([QST rumor](../../../areas/qst/wh.qst#L783)). This is a name-level story
reference, not a numeric Wemic prototype dependency. The active WLD path from
the Incarnate dispersal room into Wemic is recorded with the other approaches
below.

## Directed paths and approaches

All 93 Wemic WLD exits resolve: 92 target local rooms and one leaves Wemic for
Surface. The local open-edge graph is one weak component of 47 rooms. Directed
traversal from both #93501, the Surface boundary, and #93514, the room targeted
from the Incarnate dispersal room, reaches all 47 local rooms. Every local
room can return to either start within the Wemic graph. This is source topology
only; it does not qualify player admission or live movement.

| Direction | Source record | Interpretation |
| --- | --- | --- |
| Surface to Wemic | Surface #572118 east to Wemic #93501 | Active reciprocal boundary; the reverse is Wemic #93501 west to #572118. See [Surface record](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1492) and [Wemic record](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1126). |
| WH to Wemic | WH #55614 south to Wemic #93514 | Active one-way edge. WH room text calls this a zone-dispersal room for mobs and ties them to the Bipplewizzlille potion quest; do not describe this edge as ordinary player ingress without qualification. See [WH record](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L765) and [source room](../../../areas/wld/wh.wld#L12606). |
| Surf to Wemic | Inactive surf.wld #229296 east to #93501 | One source-only approach found in the 95-file inactive-WLD sweep, with no source-room VNUM overlap. It is not active ingress; preserve it under existing [Surf candidate row 97](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). See [source room](../../../areas/wld/surf.wld#L614548). |

The active Wemic boundary totals are two inbound records, one outbound record,
and one reciprocal pair. The inactive Surf approach raises curated source-only
WLD totals from 1,506 to 1,507 overall and from 123 to 124 for Surf. The source
has no falls, currents, negative or unresolved exits, or local D reset
commands. These static checks do not establish ordinary player routes,
dispersal odds, encounter state, or safe travel.

## Review links

The [technical audit](../quest-story-followups/WEMIC_AUDIT_REVIEW.md) records
the typed references, title and history review, and retention decision. The
[twenty builder questions](WELLVOLEN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions)
keep the authored route, cultural setting, typed item identity, and unresolved
runtime behavior with the area owner.
