# scrubs_hall source-component audit

Canonical key: component:scrubs_hall. Physical coverage row 441 / CSV line
442; candidate dependency data row 90 / CSV line 91. Reviewed October 11,
2026, Checkpoint 528; extended October 13, 2026, Checkpoint 932, against checkout
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source,
manifest, graph, identity, reset, shop, and code-reference review. It does
not establish a booted world table or an available shop.

## Inventory, title, and manifest status

The candidate has ten WLD rooms #29100–#29109 in
[scrubs_hall.wld](../../../areas/wld/scrubs_hall.wld#L1), four MOB
prototypes #29100–#29103 in
[scrubs_hall.mob](../../../areas/mob/scrubs_hall.mob#L1), a tracked but
empty [OBJ file](../../../areas/obj/scrubs_hall.obj), one ZON header with
17 reset commands in [scrubs_hall.zon](../../../areas/zon/scrubs_hall.zon#L1),
and one SHP record #29103 in
[scrubs_hall.shp](../../../areas/shp/scrubs_hall.shp#L1). The raw ZON title
is “Ye' Old ScRubS Pirate Galleon.” There is no same-stem QST or story
sidecar.

The stem is absent from [areas/AREA](../../../areas/AREA),
[areas/qst/AREA](../../../areas/qst/AREA), and the empty current
[RANDOM_AREA](../../../areas/RANDOM_AREA). Active Torg's manifest entry is
`*289-291` ([AREA line 140](../../../areas/AREA#L140)); its WLD defines a
different room at each of #29100–#29109. Generated `areas/world.wld` and
`areas/world.shp` are absent in this checkout.

## Map topology and target resolution

The Scrubs map has 19 exits. Eighteen same-number targets connect all ten
rooms as one weak component; the remaining exit is a one-way D4 from
#29100 to #156119.

| Source room | Authored exits |
| --- | --- |
| #29100, sea cave | D0 → #29101; D4 → #156119 |
| #29101, galleon plank | D2 → #29100; D3 → #29102 |
| #29102, ship deck | D0 → #29104; D1 → #29101; D5 → #29103 |
| #29103, ship corridor | D0 → #29105; D3 → #29106; D4 → #29102 |
| #29104, bow | D2 → #29102 |
| #29105, hall | D1 → #29109; D2 → #29103; D3 → #29107 |
| #29106, Relendian's tavern | D1 → #29103; D2 → #29108 |
| #29107, map room | D1 → #29105 |
| #29108, store room | D0 → #29106 |
| #29109, crew quarters | D3 → #29105 |

Every local target number #29100–#29109 is also defined in listed Torg, so
current numeric lookup resolves the 18 internal Scrubs edges to different
Torg rooms. Target #156119 is “The Ocean” only in unlisted
[Duris3.wld](../../../areas/wld/Duris3.wld#L1178500); no active WLD defines
it, and no exit from that target returns to #29100–#29109. Thirteen active
Torg WLD exit records also target numbers in this range, and Torg's ZON has
32 door-state or mobile-room references to them. Those resolve to Torg's
listed room identities, not to scrubs_hall. Candidate inbound identity
count is zero. There are no negative exits, range mismatches, fall-chance
records, or current records.

## Same-number identities and code binding

Active Torg uses different WLD records for all ten same-number rooms:
for example, Scrubs #29103 is a corridor beside a pirate tavern while Torg
#29103 is a roughly built escape tunnel; Scrubs #29106 is Relendian's tavern
while Torg #29106 is the Shop of Gornogo. Four other unlisted WLD sources,
afterlife_gh, solstice_gh, void_gh, and wlust_gh, also define different
rooms throughout #29100–#29109. Scrubs MOB #29100–#29103 has different
same-type definitions in those unlisted guild sources; none of the four
Scrubs MOB numbers has an active same-type MOB definition. The Scrubs OBJ
file contains no prototype records.

There are 13 active Torg WLD edges into the same-number room range. This is
numeric overlap, not an active entrance to the Scrubs identity. The
code-level room assignment is a separate concern: active
[specs.assign.c line 2448](../../../src/specs/specs.assign.c#L2448) assigns
the `inn` special to numeric room #29103. Current listed lookup identifies
that room as Torg's escape tunnel, while the unlisted Scrubs source uses the
same VNUM for a ship corridor. This static binding does not prove which
builder or runtime location was intended.

## ZON, SHP, and imported-object dependencies

The 17 ZON commands contain four resets of MOB #29100 in room #29100, one
reset of tired trader #29103 in room #29106, and twelve G rows that give
external object prototypes to the trader. MOB #29101 and #29102 have no
same-file ZON reset. The G-row objects match the SHP
stock list: #377, #3050, #398, #6089, #364, #347, #53, #412, and #491–#494.
Eleven have current active OBJ prototypes: ten in Heavens and #3050 in
Verz1. No current OBJ file defines #6089. Product #53 resolves in the
active table to Heavens' rope coil; unlisted Dalvik also defines #53 as a
sapphire bracelet, with other unlisted same-number object records elsewhere.
Preserve the typed source rows and resolve #53 and #6089 with the item and
shop owners.

The September 6, 2026 `feat(kingdom): ... gathering bags` commit added
Heavens OBJ #491–#494, matching SHP product rows, and matching Scrubs ZON G
resets. This is recent source maintenance; it does not add scrubs_hall to
areas/AREA or prove the zone is loaded.

The one SHP record uses keeper MOB #29103 and in_room #29106. There is no
active MOB #29103, and active Torg owns room #29106 as the Shop of Gornogo.
When `areas/world.shp` is absent, full-mode
[shop.c](../../../src/economy/shop.c#L3278) can glob every `areas/shp/*.shp`
file, including this unlisted source. The parser passes keeper #29103 to
`real_mobile`, which returns -1 when the active prototype is missing
([db.c](../../../src/world/db.c#L8587));
[assign_the_shopkeepers](../../../src/economy/shop.c#L3602) then indexes
`mob_index[keeper]` without a local negative-index check. If this fallback
path consumes the file, that is a static invalid-keeper risk, not an
observed boot failure. Its unresolved product #6089 and Torg room binding
also need owner review before the file can be treated as a usable catalog.

## Provenance, story limits, and disposition

The map-room text labels a table “Property of: The Realms Master,” but no
same-stem QST or story sidecar connects that clue to a current story or
route. Followed history includes an initial file revision and area-tree
moves; the WLD path's 2013 `stuff` commit did not document retirement. More
recently, the September 2026 commit changed both scrubs_hall.zon and
scrubs_hall.shp to stock the new Heavens gathering bags. No explicit
predecessor, successor, or retirement decision was found. The remaining
questions are in [the 21-question builder sheet](SCRUBS_HALL_BUILDER_QUESTIONS.md).

Keep the five source files RETAIN IN PLACE, including the empty OBJ file.
Resolve the Torg and other same-number room owners, the `inn` binding, the
unlisted Duris3 outlet, the missing active keeper, and the imported stock
identities with area, shop, item, editor, and deployment owners before any
activation, rename, move, merge, or archive proposal. No runtime or
migration clearance is implied. See the
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-528-scrubs-hall-map-and-shop-fallback),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-528-scrubs_hall-room-mobile-overlaps-and-recent-shop-updates),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-528-scrubs_hall-retain-map-and-shop-source),
[candidate dependency row 90](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L91),
and [coverage row 441](ZONE_COVERAGE_REGISTER.csv#L442).

## Checkpoint 932: product #6089 is a shared unresolved source reference

A fresh exact-value scan across `areas/`, `areas_mini/`, and `src/` found no
OBJ prototype #6089 in the checked-in full or mini OBJ sources and no direct
compiled-code literal. The number is used as a product by 40 SHP rows in 33
source files and as an object in 33 ZON G-reset rows across 31 source files.
Three SHP rows occur in full-manifest members `cbl_hall` and
`player_castles`; four ZON rows occur in `ashrumite`, `cbl_hall`, and
`player_castles` ([player_castles manifest](../../../areas/AREA#L63),
[cbl_hall manifest](../../../areas/AREA#L143),
[ashrumite manifest](../../../areas/AREA#L248),
[cbl_hall stock](../../../areas/shp/cbl_hall.shp#L6),
[Player Castles stock](../../../areas/shp/player_castles.shp#L8),
[Scrubs stock](../../../areas/shp/scrubs_hall.shp#L6),
[active G resets](../../../areas/zon/cbl_hall.zon#L42),
[Ashrumite reset](../../../areas/zon/ashrumite.zon#L235),
[Player Castles reset](../../../areas/zon/player_castles.zon#L17),
[Scrubs reset](../../../areas/zon/scrubs_hall.zon#L18)). Most of these source
files are unlisted; the shared number does not qualify any one record as an
active OBJ prototype.

The active `lizard` area also defines WLD room #6089
([AREA](../../../areas/AREA#L30), [room](../../../areas/wld/lizard.wld#L2467));
that is a different prototype type and cannot resolve an object lookup. The
generated `areas/world.obj` is absent in this checkout, so external or
deployment object inputs remain unverified.

In full mode, [shop.c](../../../src/economy/shop.c#L3278) falls back to
concatenating `areas/shp/*.shp` when the configured aggregate is absent; its
stock parser resolves each product VNUM with `real_object` and stores that
result in the shop product array
([shop.c](../../../src/economy/shop.c#L3380)).
[real_object](../../../src/world/db.c#L8655) returns -1 for an absent object
prototype, and the ZON parser resolves G-row object fields through the same
lookup ([db.c](../../../src/world/db.c#L1649)). This describes source-level
lookup behavior only; no boot or reset execution was observed. Keep all five
Scrubs files RETAIN IN PLACE. Resolve the shared #6089 object owner and
generated/deployment source across all consumers before treating any SHP or
ZON reference as available. Question 21 in
[SCRUBS_HALL_BUILDER_QUESTIONS.md](SCRUBS_HALL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions)
tracks the cross-area dependency.
