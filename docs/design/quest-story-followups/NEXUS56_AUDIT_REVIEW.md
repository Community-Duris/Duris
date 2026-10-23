# Nexus56 source audit

Canonical key: `component:nexus56`. Physical coverage row 429 / CSV line 430;
candidate dependency row 78 / CSV line 79. Reviewed October 11, 2026,
Checkpoint 516, against source snapshot
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static narrative,
connectivity, identity, reset, shop, dependency, and bounded-history review.
It does not establish live area loading, reset outcomes, shop operation, or
safe migration.

## Source package, title, and authored setting

The unregistered package contains five same-stem files: [WLD](../../../areas/wld/nexus56.wld#L1),
[MOB](../../../areas/mob/nexus56.mob#L1), [OBJ](../../../areas/obj/nexus56.obj#L1),
[ZON](../../../areas/zon/nexus56.zon#L1), and [SHP](../../../areas/shp/nexus56.shp#L1).
There is no `nexus56.qst` or story/map sidecar. `nexus56` is absent from
[areas/AREA](../../../areas/AREA#L238) and
[areas/qst/AREA](../../../areas/qst/AREA#L216). The exact normalized ZON
title “Nexus” is unique in the 442-title scan. The registered `nexus` stem
is separate: its ZON title is “the Mountain of Peril Peaks” and its manifest
annotation is `*575-580`.

The candidate ZON header is #830, titled “Nexus,” with last VNUM #83825 and
reset mode 2. Its 760-room map is a large city with the Planar Causeway,
Mindflayer’s Way, Bell’s Road, the City offices, embassies, a palace,
market, jail, and residential and service districts. The candidate files
contain 202 mobile and 268 object prototypes; their prose describes city
guards, emissaries, dragons, gith, drow, and illithid groups. The source set
has substantial authored setting content even though it has no native quest
or story sidecar.

## WLD graph and current room-table comparison

The WLD has 760 rooms from #83000 through #83825, with 66 unused numbers in
that interval, and 1,663 direction records: 447 north, 369 east, 447 south,
369 west, 16 up, and 15 down. All target VNUMs exist in this candidate WLD;
the undirected interpretation is one weak component containing all 760
rooms. A source scan found no `F` fall-chance or `C` current directives.
Three directions lack their opposite tuple:

| Source direction | Reverse tuple absent | WLD line |
| --- | --- | ---: |
| #83400 south to #83397 | #83397 north to #83400 | [7581](../../../areas/wld/nexus56.wld#L7581) |
| #83639 north to #83636 | #83636 south to #83639 | [11146](../../../areas/wld/nexus56.wld#L11146) |
| #83804 up to #83802 | #83802 down to #83804 | [13047](../../../areas/wld/nexus56.wld#L13047) |

The active room table owns 666 candidate room numbers: 659 under Alatorin and
7 under Delwyn. No overlapping room title is an exact match. Candidate exits
have 1,454 targets that resolve numerically to those active owners. Most are
map-grid aliases whose source and destination IDs already belong to one active
map. Twenty-three directions begin in Nexus56-only room IDs and target a
current active room owner; these are the distinct source-only approach
records already included in the global connectivity recount (11,987 total,
415 from Surface). Preserve these once, without adding them again.

| Nexus56-only source direction | Active target owner | WLD line |
| --- | --- | ---: |
| #83007 west to #83006 | Delwyn | [164](../../../areas/wld/nexus56.wld#L164) |
| #83015 east to #83381 | Alatorin | [325](../../../areas/wld/nexus56.wld#L325) |
| #83026 up to #83818 | Alatorin | [559](../../../areas/wld/nexus56.wld#L559) |
| #83027 east to #83819 | Alatorin | [578](../../../areas/wld/nexus56.wld#L578) |
| #83029 south to #83819 | Alatorin | [612](../../../areas/wld/nexus56.wld#L612) |
| #83030 west to #83496 | Alatorin | [629](../../../areas/wld/nexus56.wld#L629) |
| #83035 east to #83321 | Alatorin | [735](../../../areas/wld/nexus56.wld#L735) |
| #83045 east to #83283 | Alatorin | [963](../../../areas/wld/nexus56.wld#L963) |
| #83050 east to #83277 | Alatorin | [1071](../../../areas/wld/nexus56.wld#L1071) |
| #83054 east to #83201 | Alatorin | [1162](../../../areas/wld/nexus56.wld#L1162) |
| #83057 east to #83196 | Alatorin | [1228](../../../areas/wld/nexus56.wld#L1228) |
| #83059 east to #83187 | Alatorin | [1277](../../../areas/wld/nexus56.wld#L1277) |
| #83063 up to #83822 | Alatorin | [1363](../../../areas/wld/nexus56.wld#L1363) |
| #83064 down to #83823 | Alatorin | [1386](../../../areas/wld/nexus56.wld#L1386) |
| #83068 west to #83600 | Alatorin | [1460](../../../areas/wld/nexus56.wld#L1460) |
| #83077 west to #83688 | Alatorin | [1638](../../../areas/wld/nexus56.wld#L1638) |
| #83087 south to #83403 | Alatorin | [1842](../../../areas/wld/nexus56.wld#L1842) |
| #83095 south to #83410 | Alatorin | [1995](../../../areas/wld/nexus56.wld#L1995) |
| #83099 north to #83100 | Alatorin | [2078](../../../areas/wld/nexus56.wld#L2078) |
| #83099 south to #83109 | Alatorin | [2082](../../../areas/wld/nexus56.wld#L2082) |
| #83343 east to #83349 | Alatorin | [6700](../../../areas/wld/nexus56.wld#L6700) |
| #83343 south to #83344 | Alatorin | [6704](../../../areas/wld/nexus56.wld#L6704) |
| #83343 west to #83342 | Alatorin | [6708](../../../areas/wld/nexus56.wld#L6708) |

Active WLD files contain 1,516 raw target references into the Nexus56 numeric
room set: 1,446 from Alatorin, 51 from Surface Keeps, 14 from Delwyn, 3 from
Surface, and 2 from WH. Every target number has a current active room owner;
none targets one of the 94 Nexus56-only room IDs. Candidate-specific active WLD inbounds
therefore remain zero. These numeric matches identify present table owners;
they do not place Nexus56 in the active loader list or establish that its
source-only map is traversable.

## Same-type VNUM conflicts and identity history

Nexus56 room IDs overlap active Alatorin on 659 numbers and Delwyn on 7;
none of the room titles match exactly. It also shares 114 directed exit
tuples with Alatorin and none with Delwyn. The candidate’s 202 mobile IDs
include 102 active Alatorin IDs, with zero identical prototype blocks. Its
268 object IDs include 168 active Alatorin IDs, also with zero identical
blocks. The same 168 object numbers appear in unregistered
`alatorin_objects.obj`; none of those blocks is identical to Nexus56, while
76 match active `alatorin.obj` records. The existing
[Alatorin/Nexus56 comparison](DUPLICATE_AND_REPLACED_ZONES.md#L3179) records
this namespace collision. These comparisons establish competing prototype
identities; they do not establish a canonical source or replacement.

Followed Git histories reach the August 2009 area-tree move for all five
paths. `nexus56.wld` also has 2013, 2014, and 2026 edits; MOB has a 2014
spelling edit; OBJ has a 2026 ANSI cleanup; ZON and SHP have 2013 edits. The
reviewed path histories contain no explicit successor, rename, or
whole-area replacement. The exact Nexus title is unique, while the separate
registered `nexus` title names Peril Peaks.

## Resets, imported objects, and shop records

The ZON contains 1,181 commands: 226 door-state `D`, 279 mobile `M`, 76
follower `F`, 4 mount `R`, 127 room-object `O`, 33 container `P`, 143 give
`G`, and 293 equip `E` commands. All M/F/R mobile references and associated
room references, all D room references, and all O room references resolve
within the Nexus56 source sets. The 629 typed object references across O,
P (object and container), G, and E include 65 distinct out-of-package VNUMs.
Sixty have a current active object prototype. Five have no current object
prototype: #2569, #17233, #17234, #17241, and #17243. The four #172xx
values occur as active Khildarak room/mobile numbers, while this ZON uses
them as object IDs. Preserve the rows pending item-owner clarification.

Representative external object imports include Ashrumite #66025 from the
source-only G reset at line 1385; Bastine #7627 from the source-only [G reset
at line 1370](../../../areas/zon/nexus56.zon#L1370); Braddistock zone-13
#1335 from the [G reset at line 1378](../../../areas/zon/nexus56.zon#L1378);
Twin Towers Forest #13554,
#13556, #13558, #13560, #13562, #13564, and #13566; Prison #7356/#7357;
Rift Valley Jungle #80074/#80002; Shady Grove #97521; Jindon #82011;
Kimordril #95510/#95508; Faerie Realm #14029; Arachdrathos #36427–#36430;
Astral Main #19770; Domain #36822; Charing #45037; Cityruin #66211; and
Village of Refugees #98618. These are source-only reset declarations. The
current owner/source links are tracked in the corresponding area supplements
and the [dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).

The SHP contains 19 legacy shop records: #83026–#83043 and #83184, with 95
product rows. Forty-one product rows reference Nexus56 objects; 54 reference
external IDs. Fifty-three of those external products have a current active
object prototype; #2569 has none and also appears in the ZON as a G reset.
Shop #83037 lists Bastine object #7627 in its [product list at line
458](../../../areas/shp/nexus56.shp#L458), matching the ZON reset; preserve
both under the existing RETAIN plan. Shop #83038 also lists active Braddistock
zone-13 object #1335 at [line 501](../../../areas/shp/nexus56.shp#L501),
matching the source-only G reset; preserve that reference too.
Shop #83032 alone lists 15 products. `areas/world.shp` is absent. The source
shop loader conditionally globs individual SHP files when the combined file
cannot be opened, so the code path can encounter this file; no source review
establishes live shop loading or operation. See the
[shop loader](../../../src/economy/shop.c#L3270) and
[Nexus56 SHP](../../../areas/shp/nexus56.shp#L1).

## Typed consumers of the same numeric IDs

Active QST files have 42 giver headers and 451 `G I`/`R I` item rows whose
numbers fall in Nexus56’s mobile/object ranges. Each currently resolves to
an active Alatorin prototype. Active ZON files contain 209 M reset rows into
Nexus56’s mobile-number range and 586 object reset rows into its object-number
range; the source rows are from Alatorin, Surface Keeps, and Vehicles, and
the referenced prototypes currently belong to Alatorin. Active SHP has 15
product rows in the candidate object-number range, all from Alatorin. Source
code also has five distinct Nexus56-range object literals (#83140, #83164,
#83209, #83228, #83235) in the chaos-equipment item arrays in
[chaos_eq_data.h](../../../src/account/chaos_eq_data.h#L73). These are
references to current active object identities sharing the numbers; the
findings do not establish a dependency on the unregistered Nexus56 prototypes.

## Disposition

**RETAIN IN PLACE.** Keep all five candidate files separate while the owner
resolves the 666 current room-number overlaps, 102 mobile and 168 object
prototype collisions, 23 distinct source-only approach directions, three
missing reverse exits, five unresolved ZON object IDs, 19 shops and their
stock identities, reset intent, the absent manifests, and editor/persisted
references. Preserve the 60 currently defined imported objects and the
previously cross-linked source-only reset/shop references. Do not merge,
renumber, activate, move, or archive based on numeric overlap alone. No
migration clearance is implied.

See the [21 builder questions](NEXUS56_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-516-nexus56-one-city-graph-and-23-source-only-approaches),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-516-nexus56-title-and-vnum-conflicts),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-516-nexus56-retained-pending-owner-and-reference-closure),
[candidate dependency row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79),
and [coverage row 429](ZONE_COVERAGE_REGISTER.csv#L430).

## Checkpoint 777: Breale source-only consumers

Candidate Nexus56 ZON G rows 1332, 1336–1338, and 1349 reset active Breale objects #2660, #2662, #2663, #2658, and #2667. Candidate shops #83032 and #83034 list those same five products. Preserve these source-only references under the existing RETAIN plan; they are not active resets or proven operating shop stock. See the [Breale typed-dependency audit](BREALE_AUDIT_REVIEW.md#registered-candidate-only-consumers).

## Bloodstone Checkpoint 778: candidate-only consumers

Candidate Nexus56 ZON line 1365 and SHP shop #83037 line 453 reference active Bloodstone object #74256. Preserve this typed source dependency under the existing RETAIN plan; it is not an active reset or shop product. See the [Bloodstone typed-dependency audit](BS_AUDIT_REVIEW.md#registered-candidate-only-dependencies).

## Checkpoint 785: Delwyn room reset fields in Nexus56

Candidate nexus56.zon contains six D reset-room fields at lines 8, 12, 16,
20, 24, and 28, and seven M placement-room fields at lines 1074, 1076, 1080,
1081, 1082, 1089, and 1090. Their VNUMs #83000–#83006 overlap active
Delwyn room numbers, and nexus56.wld defines the same seven IDs as its own
rooms with different titles. Preserve these candidate-local reset locations
with the existing Nexus56 source identity; they do not establish active
Delwyn resets or new WLD ingress. The separate #83007 west to #83006 WLD
approach remains already counted. See the
[Delwyn typed audit](DELWYN_AUDIT_REVIEW.md#candidate-room-reset-overlap).

## Checkpoint 802: source-only Foggy Woods object references

Unregistered `nexus56.zon` has G inventory resets for active Foggy Woods
objects #90000 and #90016 at [line 1377](../../../areas/zon/nexus56.zon#L1377)
and [line 1387](../../../areas/zon/nexus56.zon#L1387). Unregistered
`nexus56.shp` also lists both as shop products at
[line 500](../../../areas/shp/nexus56.shp#L500) and
[line 547](../../../areas/shp/nexus56.shp#L547). Nexus56 is absent
from `areas/AREA`, and these references do not establish active reset or shop
availability. Preserve Foggy Woods as prototype owner; see the
[Foggy typed audit](FOGGY_WOODS_AUDIT_REVIEW.md#checkpoint-802-typed-census-and-compiled-bindings).

## Checkpoint 924: exact room-role endpoints and shop fallback bindings

The 760 Nexus56 rooms divide into 666 VNUMs with current Alatorin or Delwyn
owners and 94 candidate-only VNUMs. A role-aware pass across current ZON,
SHP, OBJ, story/map JSON, and fixed C/C++ numeric bindings found no external
room-field reference to those 94 candidate-only rooms. The 1,169 active ZON
door/placement fields, 11 active SHP in-room values, and active ITEM_SWITCH
and ITEM_TELEPORT destinations that fall in the Nexus56 room-number range
use IDs already owned by the active map table (11 switch and 21 teleport
destinations). Those numeric matches do not identify the unregistered
Nexus56 rooms. A raw #83343 match in active
[Alatorin SHP](../../../areas/shp/alatorin.shp#L306) is its shop ID and
keeper VNUM; that record's `in_room` is #83681
([field](../../../areas/shp/alatorin.shp#L329)). It is not a room reference
to Nexus56's candidate-only room #83343
([candidate room](../../../areas/wld/nexus56.wld#L6696)). No active WLD
inbound or graph boundary count changes.

Nexus56's [SHP](../../../areas/shp/nexus56.shp#L1) has 19 keeper/location
pairs. Eighteen keeper VNUMs exist only in `nexus56.mob`; the remaining
[#83184 record](../../../areas/shp/nexus56.shp#L764) also names an active
Alatorin mobile, a scarred dark-iron dwarf
([active prototype](../../../areas/mob/alatorin.mob#L1314)) rather than the
candidate's Ctanmir dealer
([candidate prototype](../../../areas/mob/nexus56.mob#L2473)). Eighteen shop
locations resolve by number to active Alatorin rooms with different
Nexus56 room identities. Shop #83184 instead names candidate-only room
#83076, which has no active WLD owner. The source loader conditionally
globs this file only when the combined
[world.shp](../../../src/economy/shop.c#L3270) cannot be opened. The loader
maps each [keeper](../../../src/economy/shop.c#L3493) through
`real_mobile` and reads the [`in_room` VNUM](../../../src/economy/shop.c#L3496);
this does not establish that these shops loaded or operated.

The candidate OBJ parser covered all 268 prototypes. It found one
ITEM_SWITCH destination: candidate
[chair #83148](../../../areas/obj/nexus56.obj#L2241) stores room #83638; the
candidate ZON [resets that chair there](../../../areas/zon/nexus56.zon#L1041).
The candidate WLD room is “The Central Audience Chamber”
([room](../../../areas/wld/nexus56.wld#L11121)), while active Alatorin owns
the same room VNUM as “Atop a High Mountain Rise”
([room](../../../areas/wld/alatorin.wld#L11172)). Active Alatorin OBJ #83148
is a mithril sign
([prototype](../../../areas/obj/alatorin.obj#L677)), not the candidate chair.
The ITEM_SWITCH handler reads `value[1]` as a room VNUM and looks it up with
`real_room` ([handler](../../../src/specs/specs.object.c#L302)); the current
active table therefore identifies Alatorin's room. This is a candidate-
internal switch endpoint under a colliding room number, not a current
Nexus56 route.
The three other candidate teleport-described objects, #83161, #83219, and
#83220, have ITEM_TELEPORT `value[0]` equal to zero; no other candidate
ITEM_TELEPORT or ITEM_SWITCH destination was found.

Fixed code-role matches in the Nexus56 number ranges also remain bound to
active Alatorin records: room #83117 in the
[auction-room registry](../../../src/economy/auction_room_registry.c#L9);
mobile #83187 in the
[Steelgrip VNUM](../../../src/world/vnum.mob.h#L18) and mobile #83414 in the
[rent-a-cleric binding](../../../src/specs/specs.assign.c#L2627); objects
#83140, #83164, #83209, #83228, and #83235 in
[chaos kits](../../../src/account/chaos_eq_data.h#L73), plus #83318 in the
[mining-pick check](../../../src/economy/mining.c#L562) and #83457, #83605,
and #83698 in
[object-proc bindings](../../../src/specs/specs.assign.c#L1297). None names a
candidate-only room, mobile, or object identity. Keep the five-file package
**RETAIN IN PLACE** while the intended shop, room-owner, switch, editor,
loader, deployment, and persisted identities remain unqualified. See the
[21 builder questions](NEXUS56_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-924-nexus56-has-no-typed-inbound-to-candidate-only-rooms),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-924-nexus56-switch-and-shop-vnums-follow-active-alatorin-owners),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-924-nexus56-retained-after-room-role-closure),
[candidate dependency row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), and
[coverage row 429](ZONE_COVERAGE_REGISTER.csv#L430).

## Checkpoint 943: source-only Marigot shop and reset references

Exact typed closure confirms that `nexus56.zon` line 1369 has a `G` reset for active Marigot OBJ #5319, and `nexus56.shp` shop #83037 lists the same Marigot instrument at line 457. Keep both candidate-owned source records with Nexus56 **RETAIN IN PLACE** and preserve Marigot as prototype owner. They are unregistered source references; the SHP fallback path remains conditional and does not establish active stock. This adds no active exit, room ingress, graph edge, or candidate activation. See the [Marigot source review](MARIGOT_STORY_REVIEW.md#checkpoint-943-bounded-typed-and-compiled-code-closure), [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), [coverage row](ZONE_COVERAGE_REGISTER.csv#L430), and [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-943-marigot-shop-and-reset-imports-stay-with-candidates).
