# Cabal Guildhall Audit Review

Reviewed October 11, 2026, Checkpoint 458. This is a static source, WLD graph, followed-history,
shop-loader, and typed-reference review of the unregistered cabal component. It does not establish
live shop operation, player access, portal behavior, source ownership, or permission to move files.

## Source scope and identity

The current source family contains [WLD](../../../areas/wld/cabal.wld),
[MOB](../../../areas/mob/cabal.mob), [OBJ](../../../areas/obj/cabal.obj),
[ZON](../../../areas/zon/cabal.zon), and [SHP](../../../areas/shp/cabal.shp). The ZON header is #1,
titled “Cabal Guildhall,” with last VNUM #29012 and reset mode 0. The files define 13 rooms
#29000–#29012, three mobiles #29000–#29002, four objects #29000–#29003, and one SHP record keyed
#29001. There is no same-stem QST or story sidecar.

No cabal row appears in the current [full area manifest](../../../areas/AREA) or
[QST manifest](../../../areas/qst/AREA). The active area list instead includes Torg at *289–291
([line 140](../../../areas/AREA#L140)). The WLD room fields and ZON header use zone 1 even though
these VNUMs are in zone 290. The source-list and zone-number findings do not settle ownership.

## Environmental story

The source depicts a Cabal Citadel and a linked outpost. The cavern Great Hall has a speaking
platform and routes to a sleeping area, equipment room, and antechamber
([Great Hall](../../../areas/wld/cabal.wld#L1)). The entrance text describes a stone citadel and
red marble pillars ([entrance](../../../areas/wld/cabal.wld#L30)); the treasure room presents
weapons, armor, and staves as guild equipment awaiting use
([treasure room](../../../areas/wld/cabal.wld#L63)). The antechamber opens into several rooms
described as shimmering portals
([antechamber](../../../areas/wld/cabal.wld#L79),
[portal rooms](../../../areas/wld/cabal.wld#L106)).

Room #29009 is described as a quiet Cabal meditation room with healing items, study books, and a
guild message board ([room](../../../areas/wld/cabal.wld#L173)). A separate three-room outpost
cluster is described as a busy destination, with blood and dirt in its small chamber and a guarded
entrance hidden below ground ([outpost](../../../areas/wld/cabal.wld#L186),
[small chamber](../../../areas/wld/cabal.wld#L199),
[entrance](../../../areas/wld/cabal.wld#L218)). No native quest file defines a task or completion
contract for this story.

The MOB file gives the group two Cabal guardians and Odif Yltsaeb, described as a shopkeeper whose
behavior is “backwards” ([mobiles](../../../areas/mob/cabal.mob#L1)). The OBJ file contains an
emblem, a can of spam, and two type-25 portal objects. Their descriptions name Goodie Continent and
Ice Crag, while their encoded room destinations are #29010 and #29007 respectively
([objects](../../../areas/obj/cabal.obj#L1)). The ZON places object #29002 in room #29007 and
object #29003 in room #29010 ([portal placements](../../../areas/zon/cabal.zon#L12)). These values
could connect the two separate WLD components through item commands, but they do not add WLD
directions or prove the advertised destination, return route, or live operation. The source's
generic ITEM_TELEPORT code checks the command value and looks up a room target
([handler](../../../src/magic/spell_travel.c#L932),
[target lookup](../../../src/magic/spell_travel.c#L991)).

## Active Torg ownership of the same room and mobile VNUMs

All 13 Cabal room VNUMs #29000–#29012 are also active Torg room VNUMs. Cabal's three mobile VNUMs
#29000–#29002 likewise overlap active Torg mobiles with different descriptions and purposes:

| VNUM | Cabal identity | Active Torg identity |
| --- | --- | --- |
| #29000 | Great Hall; Cabal guardian; Cabal emblem | Outer compound corner; Tranug |
| #29001 | Citadel entrance; Odif shopkeeper; spam can | Outer-compound hallway; drunk hobgoblin prison guard |
| #29002 | Cabal sleeping area; Cabal guardian; Goodie Continent portal | Torg visitor waiting area; starving goblin |
| #29003 | Cabal treasure room; Ice Crag portal | Torg inner-compound guard doors |
| #29004–#29012 | Antechamber, portal rooms, meditation room, and outpost | Different rooms along Torg's compound and cavern routes |

The active ownership is visible in [Torg's WLD](../../../areas/wld/torg.wld#L2074),
[Torg's mobiles](../../../areas/mob/torg.mob#L1647), and its
[active reset file](../../../areas/zon/torg.zon#L865). Active Torg currently owns the same room
and mobile identities; its WLD's same-numbered directions therefore do not establish access to
Cabal's alternate source map. In contrast, Cabal object VNUMs #29000–#29003 have no current
same-type object definitions outside the unregistered cabal.obj file.

The SHP location is a second identity collision. Cabal's shop record uses keeper VNUM #29001 and
room #29003 ([shop record](../../../areas/shp/cabal.shp#L1)). The current active Torg catalog maps
those numbers to the drunk guard and the inner-compound doors, while Cabal's Odif and treasure room
are not in the active area list. In this checkout areas/world.shp is absent. In non-mini mode
the [shop loader](../../../src/economy/shop.c#L3267) falls back to globbing areas/shp/*.shp
([fallback](../../../src/economy/shop.c#L3278)); while parsing a shop it converts the keeper VNUM
with real_mobile ([lookup](../../../src/economy/shop.c#L3493)). Thus the full-mode fallback can
read this unlisted Cabal record and resolve its keeper number to active Torg's #29001. This is a
static loader-path conflict, not evidence that a shop is currently operating or that a server
startup fails. The separate mini-mode shop aggregate is areas_mini/world.shp.

## Static WLD graph and outbound records

The Cabal WLD has 13 rooms and 24 authored directions: 22 local and two cross-zone. The local graph
has two weak components, #29000–#29009 (10 rooms) and #29010–#29012 (3 rooms). Directed traversal
from #29000 reaches the first ten rooms and not the outpost component. The two type-25 object
destinations described above could bridge those components, but the graph census excludes item
commands. All 13 WLD zone fields say 1 although every VNUM divides into zone 290; no negative exit,
fall-chance field, or current field was found. The loader's
[recalc_zone_numbers function](../../../src/world/db.c#L1691) derives a loaded zone-table number
from its lowest room VNUM; it does not prove Cabal is loaded.

| Cabal source exit | Target | Static classification |
| --- | --- | --- |
| #29001 D4 up | #111079 | Target is active Tharn Rifts and is also defined by unregistered Duris3. Cabal source room #29001 is already owned numerically by active Torg, so Tharn Rifts excludes this from eligible source-only approaches. |
| #29012 D4 up | Duris3 #156242 | Source-only exit to an unregistered WLD record; no reciprocal exit to #29012 was found. |

The #111079 active target is [Tharn Rifts “A Rocky Hillside”](../../../areas/wld/tharnrifts.wld#L13009);
unregistered Duris3 defines that same VNUM with different “Ocean” text
([Duris3 copy](../../../areas/wld/Duris3.wld#L232660)). The latter file also defines #156242 as an
Ocean room ([target](../../../areas/wld/Duris3.wld#L1181083)). Neither outbound record is active
ingress into Cabal. The source has no candidate-specific active inbound exit. The first edge is
already excluded from source-only approach totals because its source room VNUM belongs to active
Torg; the second is inactive-to-inactive. Eligible inactive-WLD totals remain 11,987 overall /
415 from Surf. See the [Tharn Rifts audit](THARNRIFTS_AUDIT_REVIEW.md#inactive-wld-sources-and-map-identity)
for the first edge's existing classification.

## Reset, shop-stock, and other typed dependencies

The Cabal ZON contains eight O resets, nine M resets, and 13 G equipment resets. Six O commands
import Player Castles objects #11005, #11062, #11013, and #11112 at rooms #29000, #29001, #29002,
#29009, #29011, and #29012 ([imports](../../../areas/zon/cabal.zon#L9)); keep these as source-only
reset references to the active Player Castles prototypes, not Cabal-owned objects. The G commands
also name local emblem #29000 and spam can #29001 plus object VNUMs #364, #385, #377, #3050, #398,
#53, #108, #109, #111, #6089, and #412 ([equipment list](../../../areas/zon/cabal.zon#L23)).

Cabal's single shop record stocks 13 item VNUMs: local #29000 and #29001, plus #364, #385, #377,
#3050, #398, #53, #108, #109, #111, #6089, and #412
([stock list](../../../areas/shp/cabal.shp#L1)). The current active object sources include
#364, #385, #377, #398, #53, #108, #109, #111, and #412 in Heavens and #3050 in Verz1. Cabal's
local #29000/#29001 items are not in the active object catalog, and #6089 has no current object
prototype; the active CBL Hall audit documents the shared missing #6089 prototype and its old
Tharnadia counterpart ([CBL dependency review](CBL_HALL_AUDIT_REVIEW.md#typed-inbound-and-outbound-dependencies)).
No exact #29000–#29012 C/C++ VNUM reference was found in the bounded source scan.

## Followed history and similarly titled sources

Followed Git paths show cabal.wld at the same stem from the 2009 areas-tree move; cabal.mob,
cabal.obj, and cabal.zon also have same-stem changes in 2012 and 2013. cabal.shp has only the
tracked 2009 tree move. No alternate-stem rename or explicit replacement decision was identified
in these paths.

The unregistered cabal_hall component has the near-identical ZON title “The Cabal Guildhall,” but
different VNUMs #30100–#30109 and different rooms, mobiles, and shopkeeper. Its ZON and SHP paths
were edited in commit ed94862f0 in September 2026, so keep that current source identity distinct
while its owner explains the relationship. Its title and similar shop products do not establish
that it replaced cabal ([other source WLD](../../../areas/wld/cabal_hall.wld#L1),
[other source ZON](../../../areas/zon/cabal_hall.zon#L1)); its distinct room ownership, content, and shop-loader findings are in the [Checkpoint 459 Cabal Hall audit](CABAL_HALL_AUDIT_REVIEW.md#title-history-and-neighboring-guildhall-sources). The active cbl_hall is a separate
registered “Guildhall of Clan BloodLust” at *297–298; its review does not establish Cabal ownership
([active CBL audit](CBL_HALL_AUDIT_REVIEW.md#title-history-and-duplicate-content-review)).

## Safe disposition

Keep cabal **RETAIN IN PLACE**. Resolve the room/mobile collision with active Torg, the two
unmatched guildhall identities, the Tharn Rifts and Duris3 boundary targets, and the full-mode SHP
fallback before proposing a separate path change. Confirm whether Cabal's shop should exist and,
if so, identify its intended keeper, room, and valid product prototypes. Preserve the Player
Castles reset imports as their own typed references. No manifest, area, shop, gameplay, or code
change is authorized by this audit. The full owner-question list is in the
[Cabal Guildhall builder questions](CABAL_GUILDHALL_BUILDER_QUESTIONS.md).
