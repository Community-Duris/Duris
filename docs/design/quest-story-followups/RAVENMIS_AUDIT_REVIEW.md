# ravenmis source audit

Canonical key: `component:ravenmis`. Physical coverage row 435 / CSV line
436; candidate dependency data row 84 / CSV line 85. Reviewed October 11,
2026, Checkpoint 522, against source snapshot
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
narrative, identity, route, and typed-reference review; it does not establish
that Ravenmis is part of a deployed aggregate or operating world.

## Source identity and narrative

The current same-stem files are [ravenmis.wld](../../../areas/wld/ravenmis.wld#L1),
[ravenmis.mob](../../../areas/mob/ravenmis.mob#L1),
[ravenmis.obj](../../../areas/obj/ravenmis.obj#L1),
[ravenmis.zon](../../../areas/zon/ravenmis.zon#L1), and
[ravenmis.shp](../../../areas/shp/ravenmis.shp#L1): 10 rooms, five mobiles,
two objects, one zone/reset file, and one shop record with 12 product rows.
There is no same-stem QST, map/editor JSON, or story sidecar. The ZON title
is “ravenmist guildhall” (header #453; last VNUM #29309; reset mode 0).
That is the only current exact “ravenmist guildhall” title found. The file
stem/title do not establish a relationship to Ravenloft or another guildhall.

Ravenmis is absent from [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA); current
[RANDOM_AREA](../../../areas/RANDOM_AREA) is empty. The active Newbie package
is listed for zone range 292–293 at [areas/AREA line 141](../../../areas/AREA#L141)
and separately in [qst/AREA line 131](../../../areas/qst/AREA#L131). The
Ravenmis narrative and reset records therefore remain source-only declarations.

The room text describes a fog-filled descent from rainforest into a damp
cavern-and-tunnel guildhall, a warm sleeping cavern and well, a collapsed
tunnel, and a hammered niche. It names “Iatyh” in a wall inscription and has
orange “XANAN SUCKS” graffiti ([Iatyh](../../../areas/wld/ravenmis.wld#L115),
[scribble](../../../areas/wld/ravenmis.wld#L139)). No QST contract or separate
story file supplies a quest context for those details.

## Room graph and the unregistered outlet

The WLD has 19 authored direction records across #29300–#29309. Eighteen
target another room number within that range, forming one weak component of
10 rooms. The remaining exit is [room #29300 up to #125119](../../../areas/wld/ravenmis.wld#L36),
whose current same-number source record is “Grassy Plains” in unlisted
[Duris3.wld #125119](../../../areas/wld/Duris3.wld#L527500). `Duris3` is absent
from the full AREA list, so that positive target is unresolved in the active
source membership; no reverse exit from #125119 returns to #29300. The source
map has no rooms outside its ZON range, no exits from range-mismatched rooms,
and no fall-chance or current records.

All 18 local target VNUMs also exist in the active Newbie WLD, but they resolve
to Newbie’s distinct room records, not to the unlisted Ravenmis map. The full
active-source scan found zero inbound exits to the Ravenmis source identity.
These duplicated numbers do not activate this WLD or add a route to the
active connectivity graph. See the
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-522-ravenmis-guildhall-map-and-unregistered-outlet).

## Same-number identities and bounded history

Every Ravenmis room #29300–#29309 has a different complete room block from
the same-number active Newbie rooms and from the corresponding unlisted
`cbr_hall`, `loc_gh`, and `eskull_hall` rooms. Existing identity reviews
document this collision family and find no identical room records. For
example, Ravenmis #29300 is the foggy descent, while active Newbie #29300 is
the Crypt of the Undead. Do not treat Newbie’s current #29300–#29309 records
or the three unlisted guildhall maps as evidence of a Ravenmis replacement.

The five followed same-path histories reach the 2009 area-tree move and the
2013 “stuff” commit; MOB, OBJ, and ZON also have a 2012 field-update commit.
No explicit predecessor, successor, rename, or retirement decision appears
in this bounded history. The source-only overlap with other guildhall maps
and the title alone do not establish shared ownership. See
[DUPLICATE_AND_REPLACED_ZONES.md Checkpoint 522](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-522-ravenmis-distinct-guildhall-source).

The mobile and object identities collide as well. Ravenmis mobiles #29300–
#29304 are a hermit and four vapor figures; active Newbie defines the same
numbers as a mercenary, assassin, alchemist, bleeding man, and half-elf
([Newbie mobiles](../../../areas/mob/newbie.mob#L1507)). Unlisted `cbr_hall`,
`loc_gh`, and `eskull_hall` also define some of #29300–#29303. Ravenmis
objects #29300/#29301 are a well and an iron ration, while active Newbie
defines a Pain dagger and sign at those numbers
([Newbie objects](../../../areas/obj/newbie.obj#L1376)); other unlisted guildhall
OBJ files also reuse them.

## Reset, shop, and typed dependencies

The ZON contains 22 commands in reset mode 0: five object placements, five
mobile placements, and twelve inventory-give rows. Its object placements
import Player Castles objects #11054, #11104, and #11013 into Ravenmis rooms
#29300, #29302, and #29304; place local object #29300 in #29304; and place
#29311 in #29307 even though Ravenmis OBJ defines only #29300/#29301
([reset commands](../../../areas/zon/ravenmis.zon#L9)). Current Player
Castles prototypes exist at [#11013](../../../areas/obj/player_castles.obj#L121),
[#11054](../../../areas/obj/player_castles.obj#L153), and
[#11104](../../../areas/obj/player_castles.obj#L273). #29311 is not defined
in Ravenmis OBJ; the same number is an active Newbie arrow and an unlisted
Ethdawn portal ([Newbie arrow](../../../areas/obj/newbie.obj#L1533),
[Ethdawn portal](../../../areas/obj/ethdawn_op.obj#L9)). These reset rows
remain source-only and their intended object identities need owner review.

The single SHP record is headed #29300, uses keeper #29300, and stores
`in_room #29309`; its 12 products are #29301, #53, #111, #109, #108, #347,
#364, #412, #6098, #398, #3050, and #377
([SHP product list](../../../areas/shp/ravenmis.shp#L1),
[keeper](../../../areas/shp/ravenmis.shp#L62),
[room](../../../areas/shp/ravenmis.shp#L64)). Only #6098 has no current OBJ
prototype. Ten of the remaining eleven products have current active
prototypes in Heavens (#53, #108, #109, #111, #347, #364,
#377, #398, #412) or Verz1 (#3050). Ravenmis #29301 is its iron-ration
prototype, but active Newbie #29301 is a sign. Keeper #29300 and room #29309
also collide with active Newbie’s mercenary and room records. The exact
source owners and rows are inventoried below:

| Shop product | Current source identity |
| ---: | --- |
| #29301 | Ravenmis iron ration; active Newbie same-number object is a sign. |
| #53 | Active Heavens rope coil; other unlisted files reuse #53. |
| #111, #109, #108 | Active Heavens quartz, obsidian, and firwood totems. |
| #347, #364, #412, #398, #377 | Active Heavens waterskin, ration, lockpicks, torch, and backpack. |
| #6098 | No current OBJ prototype found. |
| #3050 | Active Verz1 vellum scroll. |

The normal shop aggregate path is [areas/world.shp](../../../src/world/db.h#L22),
which is absent in this checkout. If it cannot be opened, the shop loader’s
fallback concatenates all `areas/shp/*.shp`, including this unlisted fragment
([fallback](../../../src/economy/shop.c#L3267)). The parser resolves products
with `real_object`, the keeper with `real_mobile`, and stores the room VNUM
separately ([product](../../../src/economy/shop.c#L3380),
[keeper and room](../../../src/economy/shop.c#L3493)). With Ravenmis absent
from the full AREA manifest, these duplicate numbers can therefore resolve
to active Newbie identities under that conditional path. This is parser
exposure only; it does not prove that the shop is deployed, that its intended
prototypes are loaded, or that trade is accessible.

No direct `ravenmis` string reference was found in C/C++. Numeric code bindings
are to the current active Newbie identities: `specs.assign.c` binds room
#29305 to the inn callback ([line 2447](../../../src/specs/specs.assign.c#L2447)),
and `vnum.mob.h` names mobile #29303/#29304 as Ailvio constants
([lines 13–14](../../../src/world/vnum.mob.h#L13)). The source scan cannot
prove external editor, deployed aggregate, or persisted references.

**Disposition: RETAIN IN PLACE.** Preserve all five Ravenmis files and their
current source identity. Resolve the unregistered #125119 outlet, overlaps
with active Newbie and other guildhall maps, Player Castles imports, missing
#29311 and #6098 identities, shop-loader fallback, editor/build provenance,
and external/persisted consumers before any activation, renumbering, move,
consolidation, or archive decision. No files under `areas/` or `src/` changed.
See [20 builder questions](RAVENMIS_BUILDER_QUESTIONS.md),
[coverage row 435](ZONE_COVERAGE_REGISTER.csv#L436),
[candidate dependency row 84](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L85),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-522-ravenmis-guildhall-map-and-unregistered-outlet),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-522-ravenmis-distinct-guildhall-source),
and [retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-522-ravenmis-retain-source-identities-pending-owner-decisions).

## Checkpoint 926: shared room numbers resolve to Newbie identities

On October 13, 2026, a typed follow-up scanned current WLD exits, ZON reset
fields, SHP keeper/location/product fields, QST item/quester fields, story
sidecars, ITEM_TELEPORT/ITEM_SWITCH destinations, and fixed code bindings
for the Ravenmis room, mobile, and object VNUM sets. This adds source-role
evidence to Checkpoint 522; it does not change Ravenmis's unregistered status
or qualify a deployed aggregate.

All ten Ravenmis room VNUMs #29300–#29309 are also defined by active Newbie
WLD. Nine active WLD exits target numbers in this range, all from
`newbie.wld`; in the current room table they resolve to Newbie rooms, not to
the unlisted Ravenmis records:

| Active source exit | Target |
| --- | ---: |
| [#29224 D1](../../../areas/wld/newbie.wld#L502) | #29304 |
| [#29286 D6](../../../areas/wld/newbie.wld#L1741) | #29306 |
| [#29292 D0](../../../areas/wld/newbie.wld#L1843) | #29300 |
| [#29298 D8](../../../areas/wld/newbie.wld#L1953) | #29301 |
| [#29301 D5](../../../areas/wld/newbie.wld#L1997) | #29302 |
| [#29302 D4](../../../areas/wld/newbie.wld#L2027) | #29301 |
| [#29306 D0](../../../areas/wld/newbie.wld#L2064) | #29307 |
| [#29307 D2](../../../areas/wld/newbie.wld#L2081) | #29306 |
| [#29309 D5](../../../areas/wld/newbie.wld#L2124) | #29306 |

The independent OBJ scan found six ITEM_TELEPORT `value[0]` destinations
into the same room-number range and no ITEM_SWITCH `value[1]` destination
there. All six resolve to active Newbie WLD rooms in the current table:

| Portal source object | Destination | Source membership |
| --- | ---: | --- |
| [Newbie #29299](../../../areas/obj/newbie.obj#L1361) | #29305 | Active Newbie |
| [Newbie #29313](../../../areas/obj/newbie.obj#L1556) | #29308 | Active Newbie |
| [Eskull Hall #29301](../../../areas/obj/eskull_hall.obj#L15) | #29309 | Unlisted |
| [Ethdawn OP #29310](../../../areas/obj/ethdawn_op.obj#L1) | #29307 | Unlisted |
| [Loc GH #29300](../../../areas/obj/loc_gh.obj#L1) | #29309 | Unlisted |
| [Loc GH #29302](../../../areas/obj/loc_gh.obj#L17) | #29307 | Unlisted |

These portal fields do not add WLD exit edges to Ravenmis. In particular,
the four unlisted portal prototypes are not active inbound routes, and the
two Newbie destinations belong to the current active Newbie map. The existing
active-WLD inbound count to Ravenmis source identity remains zero.

Other typed consumers reinforce the same role separation. The manifest-listed
Newbie reset file uses mobile VNUMs #29300–#29304 in six `M` records and
room numbers #29300–#29309 in mobile, door, and object reset fields. Its
mobile and room roles resolve to Newbie's active prototypes. Unlisted
`cbr_hall`, `eskull_hall`, and `loc_gh` reset files reuse some of those
numbers for their own mobiles, objects, and room placements; for example,
`cbr_hall.zon` places its #29303 shopkeeper in its own room #29303
([reset](../../../areas/zon/cbr_hall.zon#L16)), while `loc_gh.zon` places its
own portal objects by object VNUM ([reset](../../../areas/zon/loc_gh.zon#L12)).
Those sources establish neither alias nor Ravenmis activation.

The SHP scan found additional typed collisions. Active
[Newbie shop #29283](../../../areas/shp/newbie.shp#L256) has keeper #29283,
room #29305, and products #29305, #29306, and #29307; these are Newbie's
active room and object roles, not Ravenmis rooms stocked as objects. Three
unlisted guildhall records also intersect Ravenmis roles: [cbr_hall shop
#29303](../../../areas/shp/cbr_hall.shp#L1) uses keeper/room #29303/#29303;
[eskull_hall shop #29303](../../../areas/shp/eskull_hall.shp#L1) uses
#29303/#29302; and [loc_gh shop #29301](../../../areas/shp/loc_gh.shp#L1)
uses #29301/#29308. They remain separate source shop records and mobile/map
identities. The conditional SHP fallback and those same-number fields do not
prove which records reach a deployed aggregate.

Newbie QST and story sources consume these numbers in item/mobile roles.
`newbie.qst` uses quester header [#29304](../../../areas/qst/newbie.qst#L4135),
awards item #29302 at [line 1287](../../../areas/qst/newbie.qst#L1287),
and awards item #29300 at [line 2053](../../../areas/qst/newbie.qst#L2053);
the QST parser treats `R I` as an item reward ([quest parser](../../../src/world/quest.c#L2251)).
Newbie OBJ defines #29302 as the emblem of healing and #29300 as the Pain
dagger ([emblem](../../../areas/obj/newbie.obj#L1412),
[dagger](../../../areas/obj/newbie.obj#L1376)). Its story sidecar separately
uses mobile #29304 and repeats item #29302/#29300 in completion keys
([mobile entry](../../../areas/story/newbie.story.json#L418),
[#29302 reward](../../../areas/story/newbie.story.json#L1443),
[#29300 reward](../../../areas/story/newbie.story.json#L1869)). Active Newbie
mobile #29304 is the half-elf woman; Ravenmis #29304 is a vapor figure. The
same numeric values do not identify the same prototypes: Ravenmis #29300 is
its well object, and #29302 is a Ravenmis room with no same-number Ravenmis
object.

These QST and sidecar records are source declarations, not proof of runtime
quest completion. The QST loader opens `areas/world.qst` and explicitly does
not glob `areas/qst/*.qst` ([loader](../../../src/world/quest.c#L2086));
`areas/world.qst` is absent in this checkout. Fixed code bindings are also
typed to Newbie: `specs.assign.c` binds active room #29305 to the inn callback
([line 2447](../../../src/specs/specs.assign.c#L2447)) and assigns the Ailvio
callbacks to mobile constants #29303/#29304
([assignments](../../../src/specs/specs.assign.c#L206),
[VNUM definitions](../../../src/world/vnum.mob.h#L13)). No direct
`ravenmis` string binding was found.

No new predecessor, successor, retirement decision, or active WLD inbound
was found. Preserve all five Ravenmis files **RETAIN IN PLACE** while the map
owner resolves the #125119 outlet, the six shared-number teleport targets,
Newbie's quest/story mobile and item identities, the sibling guildhall shop
records, and loader/editor/deployment provenance. See
[22 builder questions](RAVENMIS_BUILDER_QUESTIONS.md),
[connectivity checkpoint 926](WORLD_CONNECTIVITY.md#checkpoint-926-ravenmis-shared-room-numbers-remain-newbie-rooms),
[identity checkpoint 926](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-926-ravenmis-shared-numbers-do-not-establish-lineage),
and [retention checkpoint 926](ATTIC_MIGRATION_PLAN.md#checkpoint-926-ravenmis-retain-after-newbie-role-closure).
