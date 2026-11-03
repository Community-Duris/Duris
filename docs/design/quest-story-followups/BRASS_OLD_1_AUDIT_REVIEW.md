# Brass Old 1 source audit

Checkpoint 454, reviewed October 11, 2026. Candidate row 17. The source is
unregistered in the current full-area and QST manifests; this review is a
static source, history, and loader-path assessment. It does not establish
runtime loading, shop operation, player-data identity, or editor ownership.

## Scope and source identity

The complete source family is [WLD](../../../areas/wld/brass-old-1.wld),
[MOB](../../../areas/mob/brass-old-1.mob),
[OBJ](../../../areas/obj/brass-old-1.obj),
[ZON](../../../areas/zon/brass-old-1.zon),
[QST](../../../areas/qst/brass-old-1.qst), and
[SHP](../../../areas/shp/brass-old-1.shp). Its ZON header is #252, its raw
title is “Plane of Fire, Brass,” and its reset-mode field is 1. All six paths
are absent from `areas/AREA` and `areas/qst/AREA`. There is no
`brass-old-1.story.json` or local map sidecar. The current
[active Brass dossier](../zone-stories/PLANE_OF_FIRE_BRASS.md) and
[`brass.story.json`](../../../areas/story/brass.story.json) describe the
registered successor.

The normalized ZON title occurs in four of the 442 current ZON files:
`brass`, `brass-old-1`, `brass-old-2`, and `brass-old-3`. The matching title
alone does not identify those three archived components as one source copy.
For `brass-old-1`, the commit history and a broad VNUM translation establish
the relationship to active `brass`.

## Relationship to active Brass

The 2012 commit `e71c1369d41cd2aa2bf31997c4b01707ba899434` says New Brass was
added and old Brass disconnected. The 2025 commit
`0e925fa992ecb648b260efa159bd016fa88f9136` changes the `areas/AREA` entry
from `New_Brass_2012 *1390-1393` to `brass *1390-1393`, keeps the six
New_Brass_2012 source files under the `brass-old-1` name, and updates the
active `brass` sources. The followed histories show the old path as the
retained, inactive source family.

Subtracting 100 from local old-source VNUMs gives a strong crosswalk to
active Brass. The match is broad but not a byte-for-byte replacement:

| Source family | Archived source | Active `brass` | Offset comparison |
| --- | ---: | ---: | --- |
| WLD | 357 rooms, 821 exits | 357 rooms, 821 exits | 356 room IDs map after −100; 277 mapped titles match; 633 exit records match after translating local room references |
| MOB | 144 mobiles | 147 mobiles | All 144 IDs map; 142 keyword fields and 143 short descriptions match |
| OBJ | 150 objects | 170 objects | All 150 IDs map; 148 keyword fields and 149 short descriptions match |
| QST | 7 giver blocks | 7 giver blocks | All 7 giver IDs map; 5 blocks match after local VNUM translation |
| SHP | 18 shop records | 18 shop records | All 18 shop keys map; product lists match after translating local object VNUMs and preserving external VNUMs |
| ZON | 786 commands | 779 commands | 585 command records match after translating local VNUMs |

The differences are real and should remain documented. Old room #139348 maps
to active-range #139248, which has no current Brass room; active Brass adds
#139357 without an old counterpart. Among the mapped WLD rooms, 79 titles
differ, and 188 old exit records differ from the current map. The old
`#139183` QST block has `R C 3000000` at [line 26](../../../areas/qst/brass-old-1.qst#L26),
while active giver #139083 uses `R C 1000000`. The old `#139224` block also
has a source-only `R I 55271` at [line 163](../../../areas/qst/brass-old-1.qst#L163);
the active mapped block has no such line. Preserve that Winterhaven object
reference as archival evidence, not as an active Brass requirement or reward.

The seven QST blocks align one-to-one with active givers after subtracting
100: #139172→#139072, #139183→#139083, #139210→#139110,
#139219→#139119, #139221→#139121, #139224→#139124, and #139232→#139132.
The active dossier is the story record for the registered source; the two
legacy differences above stay with this archived-source review.

## Static WLD graph and boundary

The old WLD has 357 rooms, #139100–#139456, and 821 authored directions:
820 local and one boundary. Its local undirected graph is one 357-room
component. Directed traversal from #139100 reaches 354 rooms; #139265,
#139266, and #139347 are not reached. No fall-chance or current fields occur
in this WLD.

The historical `*1390-1393` AREA span corresponds to #139000–#139399; 57 old
rooms (#139400–#139456) and 131 exits from those rooms are outside that span.
After the −100 crosswalk they fall within the active Brass span. This is
consistent with the version relationship above, not an active range repair.

The sole boundary is [old room #139100, D2, south, to #25455](../../../areas/wld/brass-old-1.wld#L14).
It maps to active Brass room #139000, whose south exit reaches the same
Plane of Fire room; active Fire supplies the reciprocal north exit to
#139000. This is the predecessor form of the current boundary, not a new
approach. Active WLD sources contain 598 exit records targeting VNUMs also
present in the old file, but all 598 target same-number rooms owned by active
`brass`; none targets one of the 100 old-only numeric room IDs. Candidate
inbound remains zero, and this source does not add to the curated inactive
approach total of 11,987 overall / 415 from Surf. The earlier Plane of Fire
classification is corrected in the
[Plane of Fire audit](PLANE_OF_FIRE_AUDIT_REVIEW.md#inactive-wld-approaches-and-candidate-records).

## Typed references

The inactive [ZON](../../../areas/zon/brass-old-1.zon) has 66 reset commands
whose mobile/object VNUMs resolve to current active prototypes outside the
old source's own MOB/OBJ catalogs. They span 57 VNUMs and 37 current area
owners. These are source-only references, not active resets:

| Current owner | ZON line: command #VNUM |
| --- | --- |
| alatorin | 544:G #83469; 921:G #83179 |
| arcium | 543:G #85716 |
| ashrumite | 895:G #66048; 896:G #66045 |
| church | 924:G #87859 |
| cosmic | 673:M #76073; 825:G #76064 |
| crakkaro | 542:G #87011 |
| earth | 761, 764, 765, 767, 960:M #43540 |
| ethereal_main | 536:G #12426 |
| evkeep | 529:G #44876 |
| firep | 812:G #88322 |
| githzer | 559:G #44504 |
| guildhalls | 458:O #48012 |
| harrow | 887:G #29479 |
| heavens | 463:O #60; 553:G #97; 554:G #96; 555:G #77; 556:G #73; 557:G #54; 828:G #1240 |
| highmoor | 534:G #19127 |
| icecrag | 823:G #97006 |
| jotun | 813:G #96071; 814:G #96076 |
| juiblex | 889:G #87568 |
| mansion | 521:G #3532; 522:G #3524 |
| maze_are | 886:G #94009 |
| minopass | 762, 763, 766, 957, 959:M #94740 |
| mntcastl | 535:G #37144; 551:G #37142 |
| plane_fire_one | 471, 472:M #25400 |
| ravenloft | 897:G #58331 |
| realm | 831:G #14030 |
| sea | 832:G #67359 |
| snogres | 815:G #87725 |
| suntmpl | 520:G #82416 |
| surfacekeeps | 549:G #120047 |
| tharnadia | 829:G #132509; 923:G #132632 |
| torg | 821:G #28983; 822:G #28984 |
| torrhan | 824:G #66637 |
| trakkia | 668:M #57060 |
| truk | 545:G #1604 |
| undermountain | 558:G #92046 |
| unique | 466:P #67244; 1040:G #67245 |
| wh | 816–820:G #55554–#55550; 830:G #55150 |

The source-only QST item command at line 163 names active Winterhaven object
#55271. Two archived SHP records have product lists: shop #139177 has seven
products from active Undermountain, Githzer, and Heavens; shop #139226 has
five products from active Heavens, Tharnadia, Winterhaven, Realm, and Sea,
plus one old-local object. Shop #139181 has nine old-local product VNUMs.
None of these archival QST/ZON/SHP records proves current stock or active
quest behavior.

`src/specs/specs.assign.c` has a numeric reference to active object #139138
([line 1392](../../../src/specs/specs.assign.c#L1392)); old OBJ also defines
#139138 with a different description. That code reference resolves the
current active numeric owner and does not load `brass-old-1`. No source path
binding for `brass-old-1` or `New_Brass_2012` was found in `src/`.

## Full-mode shop-loader path and risk

This checkout has no `areas/world.shp`. In non-mini mode,
[`open_shop_stream`](../../../src/economy/shop.c#L3267) falls back to
`areas/shp/*.shp`, skipping only `end.shp`, and appends the end records.
Therefore the 18 `brass-old-1.shp` records enter full-mode shop parsing even
though this source is outside `areas/AREA`. The separate mini-mode shop path
uses `areas_mini/world.shp`.

Of the 18 archived keeper VNUMs, 17 have no current active mobile prototype.
The remaining numeric key, #139141, resolves to an active Brass mobile with
a different short description; no active ZON M command resets any of these
18 old keeper numbers. `boot_the_shops` converts the stored keeper through
[`real_mobile`](../../../src/economy/shop.c#L3492), and
[`real_mobile`](../../../src/world/db.c#L8587) returns −1 for a missing
prototype. `assign_the_shopkeepers` then indexes `mob_index[keeper]` without
a negative-index guard ([line 3627](../../../src/economy/shop.c#L3627)).
This is a static full-mode boot-safety risk; no crash or runtime outcome was
observed because no build or server start was performed.

## Disposition

Keep candidate `brass-old-1` **RETAIN IN PLACE**. Git history establishes it
as an archived predecessor to active `brass`; it does not clear the old
files for deletion or path movement. Resolve the full-mode SHP fallback and
keeper VNUMs, confirm the two QST differences and 66 source-only ZON imports,
and preserve editor, persisted-object, and map identity evidence before any
separate owner decision. See [builder questions](BRASS_OLD_1_BUILDER_QUESTIONS.md),
[coverage row 368](ZONE_COVERAGE_REGISTER.csv#L368), and
[candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17).


## Checkpoint 782: source-only Cosmic reset references

The archived `brass-old-1.zon` contains two typed reset rows to active Cosmic
prototypes: M #76073 and G #76064 ([lines 673 and 825](../../../areas/zon/brass-old-1.zon#L673)).
Keep candidate row 16 (CSV line 17) **RETAIN IN PLACE** and preserve the
source-only imports with the existing archive. They do not activate Brass Old
1 or change the active Cosmic ownership. See the
[Cosmic typed audit](COSMIC_AUDIT_REVIEW.md#active-and-candidate-inbound-dependencies).

## Checkpoint 784: Crakkaro candidate import

The source-only brass-old-1.zon has one G reset for active Crakkaro object
#87011 (stone collar) at [line 542](../../../areas/zon/brass-old-1.zon#L542).
Preserve this candidate-only reference with existing candidate row 16 and its
**RETAIN IN PLACE** disposition. It does not establish an active consumer or
authorize a path move. See the
[Crakkaro typed audit](CRAKKARO_AUDIT_REVIEW.md#active-and-candidate-inbound-consumers).
