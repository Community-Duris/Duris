# player_guilds source audit

Canonical key: component:player_guilds. Physical coverage row 434 / CSV
line 435; candidate dependency row 83 / CSV line 84. Reviewed October 11,
2026, Checkpoint 521, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static narrative,
identity, shop, and typed-reference review. It does not establish that the
source shops are in a deployed aggregate or operating in a live world.

## Source identity and QST membership

The source component contains only
[player_guilds.qst](../../../areas/qst/player_guilds.qst#L1) and
[player_guilds.shp](../../../areas/shp/player_guilds.shp#L1). No same-stem
WLD, MOB, OBJ, ZON, SHP-side story text, or story/map sidecar exists. The
component is absent from [areas/AREA](../../../areas/AREA),
[areas/qst/AREA](../../../areas/qst/AREA), and the empty
[RANDOM_AREA](../../../areas/RANDOM_AREA). This is a cross-area shop catalog,
not a geographic zone with a map title or local population package.

The nine-byte QST contains one header, #35320, immediately followed by S:
there is no M/Q contract or quest text. The mobile exists as Anan in unlisted
Asunwa source ([asunwa.mob #35320](../../../areas/mob/asunwa.mob#L286)).
The QST manifest lists Asunwa at
[qst/AREA line 157](../../../areas/qst/AREA#L157), but does not list
player_guilds. Asunwa’s own [QST](../../../areas/qst/asunwa.qst#L1) uses
quester #35319 for Balgar’s scroll contract, not #35320. The quest loader
opens areas/world.qst and explicitly does not glob areas/qst/*.qst
([quest path](../../../src/world/quest.c#L79),
[no-glob fallback](../../../src/world/quest.c#L2087)); areas/world.qst is
absent in this checkout.

## Shop inventory and room/mobile ownership

The SHP fragment has 13 shop records and 97 product rows covering 37 distinct
object VNUMs. The shop loader resolves product numbers with real_object(),
the keeper with real_mobile(), and records a separate in_room VNUM
([product lookup](../../../src/economy/shop.c#L3380),
[keeper and room fields](../../../src/economy/shop.c#L3493)).

| Shop record | Keeper VNUM and source owner | in_room VNUM and source owner | Stock rows |
| --- | --- | --- | ---: |
| [#34202](../../../areas/shp/player_guilds.shp#L1) | #34202, active Drst | #34202, active Drst ([line 47](../../../areas/shp/player_guilds.shp#L47)) | 7 |
| [#34301](../../../areas/shp/player_guilds.shp#L63) | #34301, no current MOB definition | #34306, active Drst ([line 103](../../../areas/shp/player_guilds.shp#L103)) | 9 |
| [#34402](../../../areas/shp/player_guilds.shp#L119) | #34402, active Longhollow and unlisted Temple2 | #34404, active Longhollow and unlisted Temple2 ([line 161](../../../areas/shp/player_guilds.shp#L161)) | 11 |
| [#35000](../../../areas/shp/player_guilds.shp#L177) | #35000, no current MOB definition | #35008, no current WLD definition ([line 221](../../../areas/shp/player_guilds.shp#L221)) | 5 |
| [#35050](../../../areas/shp/player_guilds.shp#L237) | #35050, no current MOB definition | #35056, no current WLD definition ([line 281](../../../areas/shp/player_guilds.shp#L281)) | 8 |
| [#35101](../../../areas/shp/player_guilds.shp#L297) | #35101, no current MOB definition | #35105, no current WLD definition ([line 344](../../../areas/shp/player_guilds.shp#L344)) | 8 |
| [#35199](../../../areas/shp/player_guilds.shp#L360) | #35199, no current MOB definition | #35207, active Newhaven ([line 423](../../../areas/shp/player_guilds.shp#L423)) | 11 |
| [#35252](../../../areas/shp/player_guilds.shp#L439) | #35252, active Newhaven | #35253, active Newhaven ([line 491](../../../areas/shp/player_guilds.shp#L491)) | 4 |
| [#35300](../../../areas/shp/player_guilds.shp#L507) | #35300, unlisted Asunwa | #35303, unlisted Asunwa ([line 551](../../../areas/shp/player_guilds.shp#L551)) | 5 |
| [#35554](../../../areas/shp/player_guilds.shp#L567) | #35554, no current MOB definition | #35552, active Lava Caves ([line 618](../../../areas/shp/player_guilds.shp#L618)) | 12 |
| [#35703](../../../areas/shp/player_guilds.shp#L634) | #35703, no current MOB definition | #35704, no current WLD definition ([line 678](../../../areas/shp/player_guilds.shp#L678)) | 5 |
| [#35902](../../../areas/shp/player_guilds.shp#L694) | #35902, no current MOB definition | #35903, no current WLD definition ([line 723](../../../areas/shp/player_guilds.shp#L723)) | 5 |
| [#35952](../../../areas/shp/player_guilds.shp#L739) | #35952, no current MOB definition | #35957, no current WLD definition ([line 793](../../../areas/shp/player_guilds.shp#L793)) | 7 |

Nine keeper VNUMs have no current MOB prototype. Six shop location VNUMs
have no WLD definition in the current source tree; one more, #35303, is in
the unlisted Asunwa WLD. The other six locations resolve to active WLD
owners. A shop-location VNUM is not itself an authored WLD connection.

Twenty-one product rows use 13 VNUMs with no prototype in areas/obj:
#3051, #5541, #6089, #8331, #34301, #35002, #35054, #35100, #35250, #35551,
#35702, #35901, and #35909. Several other source products resolve to active
objects from different area owners; for example, nine rows across eight
shops list Kimordril objects #95547, #95540, or #95536
([first](../../../areas/shp/player_guilds.shp#L7),
[remaining Kimordril rows](../../../areas/shp/player_guilds.shp#L745)).
Four more list active Tundra iron rations #13723
([first](../../../areas/shp/player_guilds.shp#L240),
[last](../../../areas/shp/player_guilds.shp#L746);
[prototype](../../../areas/obj/tundra.obj#L324)). Shop #35050 also lists
active Realm object #14112
([line 242](../../../areas/shp/player_guilds.shp#L242)); shop #34402 lists
#34405, whose same-number object definitions belong to active Longhollow
and unlisted Temple2
([line 121](../../../areas/shp/player_guilds.shp#L121),
[Longhollow audit](LONGHOLLOW_AUDIT_REVIEW.md)).

Shop #35300 has a second, distinct source record in unlisted
[ddguild.shp](../../../areas/shp/ddguild.shp#L1). Both records use keeper
#35300 in Asunwa room #35303. The ddguild record stocks only #35301, while
player_guilds stocks #35301 plus four other products and declares different
trade/keeper flags. This is a same-keeper/same-room source overlap, not an
identified replacement relationship. Asunwa’s own shop is a separate record
for keeper #35320 in room #35374
([asunwa.shp](../../../areas/shp/asunwa.shp#L1)). See the
[Asunwa typed-dependency review](ASUNWA_AUDIT_REVIEW.md#typed-dependency-census).

## Conditional loader exposure and source-only boundaries

The normal shop stream is areas/world.shp, which is absent in this checkout.
If that file cannot be opened, open_shop_stream() concatenates every
areas/shp/*.shp fragment and appends end.shp
([SHOP_FILE](../../../src/world/db.h#L22),
[fallback](../../../src/economy/shop.c#L3267)). Thus invoking the shop boot
from this checkout would expose player_guilds.shp to the fallback parser;
this does not prove the file is in a deployed aggregate, its keepers resolve,
or any shop is operating. The missing keeper/product and location records
remain unresolved source dependencies.

The source scan found no direct player_guilds reference in C/C++ and no
WLD route data in this component. The previously documented typed consumers
remain source-only: nine Kimordril product rows, four Tundra ration rows,
the Longhollow/Temple2 #34405 ambiguity, and Lava Caves location #35552.
The latter is a shop room field, not active WLD ingress or active stock.
See the [Kimordril](KIMORDRIL_AUDIT_REVIEW.md),
[Tundra](TUNDRA_AUDIT_REVIEW.md),
[Longhollow](LONGHOLLOW_AUDIT_REVIEW.md),
[Faerie Realm](FAERIE_REALM_AUDIT_REVIEW.md), and
[Underground Lava Caves](UNDERGROUND_LAVA_CAVES_AUDIT_REVIEW.md) audits.

Bounded same-path history shows the SHP and QST paths at the 2009 area-tree
move and 2013 “stuff” commit. No explicit successor or retirement decision
was found. That history does not establish whether the player_guilds
catalog is intended as an addition, a retained older copy, or a source
fragment for another build process.

**Disposition: RETAIN IN PLACE.** Preserve all 13 shop records and their
source rows until the catalog owner resolves the nine missing keepers, 13
missing product VNUMs, six missing WLD locations, active cross-area stock,
same-room #35300 source duplication, aggregate/fallback behavior, and editor
or deployment provenance. No shop activation, source deletion, merge, move,
or archive is cleared by this static review. See
[20 builder questions](PLAYER_GUILDS_BUILDER_QUESTIONS.md),
[coverage row 434](ZONE_COVERAGE_REGISTER.csv#L435),
[candidate dependency row 83](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L84),
[connectivity checkpoint 521](WORLD_CONNECTIVITY.md#checkpoint-521-player-guilds-catalog-no-room-graph),
[identity checkpoint 521](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-521-player-guilds-shared-shop-sources-not-zone-replacement),
and [retention checkpoint 521](ATTIC_MIGRATION_PLAN.md#checkpoint-521-player-guilds-retain-shop-records-pending-owner-closure).

## Checkpoint 925: typed reset and numeric role closure

On October 13, 2026, a follow-up scan of the same source snapshot compared
all 13 shop keeper VNUMs and 37 distinct stock object VNUMs against typed
fields in every current `areas/zon/*.zon` reset source and fixed numeric code
references. The scan confirms that the references below use mobile, object,
room, or shop fields by their actual parser role; number equality alone does
not connect the catalog to a loaded shop.

Four of the 13 keeper VNUMs occur in mobile-prototype fields of 40 ZON `M`
reset rows:

| Keeper VNUM | Current mobile source and reset rows | Player Guilds room field and current reset placement |
| --- | --- | --- |
| #34202 | Active [Drst mobile #34202](../../../areas/mob/drst.mob#L41), a large spider; 25 Drst `M` rows, including [first](../../../areas/zon/drst.zon#L429) and [last](../../../areas/zon/drst.zon#L583). | Shop #34202 names room #34202. No matching #34202 reset targets that room; Drst instead resets mobile #34204 there ([line 426](../../../areas/zon/drst.zon#L426)). |
| #34402 | The VNUM has two source prototypes: active [Longhollow fomorian guard](../../../areas/mob/long.mob#L34) and unlisted [Temple2 demented cultist](../../../areas/mob/temple2.mob#L24). Four reset rows split between [Longhollow](../../../areas/zon/long.zon#L251) and [Temple2](../../../areas/zon/temple2.zon#L370). | Shop #34402 names room #34404. Temple2 resets mobile #34411 in that room ([line 350](../../../areas/zon/temple2.zon#L350)); none of the four #34402 rows targets #34404. |
| #35252 | Active [Newhaven mobile #35252](../../../areas/mob/newhaven.mob#L832), the wraith of a knight commander; one Newhaven reset places it in room #35298 ([line 396](../../../areas/zon/newhaven.zon#L396)). | Shop #35252 names room #35253. No current ZON `M` row places keeper #35252 there. |
| #35300 | Unlisted [Asunwa mobile #35300](../../../areas/mob/asunwa.mob#L1), a massive jaguar-man guard; ten Asunwa reset rows, from [line 45](../../../areas/zon/asunwa.zon#L45) through [line 81](../../../areas/zon/asunwa.zon#L81). | Shops #35300 name room #35303. Asunwa places other mobiles #35301, #35302, #35303, and #35306 there ([lines 59–62](../../../areas/zon/asunwa.zon#L59)); none of the ten #35300 rows targets #35303. |

The other nine keeper VNUMs have no current ZON `M` reset using that number
as its mobile prototype. A separate room-field scan found other mobiles
reset at Player Guilds locations #34306 (Drst), #34404 (Temple2), #35207
(Newhaven), and #35303 (Asunwa), as well as #34202 (Drst); those resets do
not use their shop's declared keeper VNUM. The remaining eight shop room
fields have no current ZON `M` placement. This is source evidence about
prototype and room-field roles, not proof that any shop is deployed or
operating.

Three of the 13 missing product prototypes also appear in direct ZON object
reset fields. Object #3051 has two `G` resets and #8331 has one, all in
[Sylvandawn](../../../areas/zon/sylvandawn.zon#L487); #6089 has 33 `G` reset
rows across 31 zone files, including [Aeon Guildhall](../../../areas/zon/aeon_gh.zon#L22)
and [Ygg](../../../areas/zon/ygg.zon#L25). The source comments label these
items missing from the corresponding `.obj` file. No current reset uses any
of the 13 missing product VNUMs in a `P` container-object field. These are
additional source-only consumers of the same object numbers, not evidence
that `player_guilds.shp` is loaded.

Exact numeric code references also require role separation. `#3051` is a
mobile prototype in manifest-listed [verz1](../../../areas/mob/verz1.mob#L1094)
(an earth elemental); `RANDOM_SAILOR` is only a macro definition for that
number ([rand_items.h](../../../src/item/rand_items.h#L38)), with no other
use found. `#5541` has no current mobile prototype. Its `real_mobile0(5541)`
callback assignment is under `#if 0` ([specs.assign.c](../../../src/specs/specs.assign.c#L667)),
but active code uses `real_room(5541)` ([specs.verzanan.c](../../../src/specs/specs.verzanan.c#L5204))
for room #5541, “Inside the Cathedral,” in active
[Tharnadian Ruin](../../../areas/wld/tharnadian_ruin.wld#L995). Neither code
reference supplies the missing object prototype #5541. Numeric matches for
#35050 and #35952 in `specs.assign.c` are inside a commented-out block
([lines 1860–1864](../../../src/specs/specs.assign.c#L1860)) and are not
active bindings.

No WLD exit, active WLD inbound count, tracked history, or identified
successor changes in this follow-up. Keep the source package **RETAIN IN
PLACE** while the shop owner resolves which mobile identities and intended
placements the keeper fields mean, the three shared product/reset consumers,
the duplicate #35300 catalog row, and the existing loader and deployment
questions. See [21 builder questions](PLAYER_GUILDS_BUILDER_QUESTIONS.md),
[connectivity checkpoint 925](WORLD_CONNECTIVITY.md#checkpoint-925-player-guilds-reset-roles-do-not-add-room-edges),
[identity checkpoint 925](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-925-player-guilds-mobile-roles-do-not-establish-replacement),
and [retention checkpoint 925](ATTIC_MIGRATION_PLAN.md#checkpoint-925-player-guilds-retain-after-reset-role-census).
