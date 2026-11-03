# ud_map2 audit review

Canonical key: component:ud_map2. Coverage data row 464 / CSV line 465;
candidate dependency row 113 / CSV line 114. Reviewed October 12, 2026
(Checkpoint 551) against checkout c500ac51b8cd26540943257e7ca4e9807263ef74.
This is a static source, graph, typed-reference, identity, and followed-history
review. It does not establish live access, route safety, or reset outcomes.

## Source inventory and environmental story

The same-stem files are [ud_map2.wld](../../../areas/wld/ud_map2.wld#L1),
empty [ud_map2.mob](../../../areas/mob/ud_map2.mob), empty
[ud_map2.obj](../../../areas/obj/ud_map2.obj), and
[ud_map2.zon](../../../areas/zon/ud_map2.zon#L1). The ZON header is #10002,
titled “the caverns of the UnderDark,” has top VNUM #1129479, and reset mode 2.
The stem is absent from [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA). No same-stem QST, SHP, map JSON, or
story sidecar exists.

The WLD defines 6,155 unique rooms from #1120304 through #1129479. Of these,
6,153 have empty descriptions; only #1120304 and #1124947 have short entrance
sentences. Sixteen normalized title groups describe phosphorescent fungus,
rocky and island shores, underground lakes, rapids, rivers, lava, a waterfall,
and related cavern terrain. Those labels and two brief descriptions provide
setting, but the checked-in map has no broader authored plot or prose story.

## Authored WLD graph and boundaries

The WLD contains 24,594 exit records: 22,839 target another ud_map2 room,
27 target active WLD rooms, one targets #30300 in unregistered WLD sources,
1,722 positive targets are absent from all current WLD sources, and five
negative targets are -1. The 1,722 absent positive targets cover 984 distinct
VNUMs. There are no rooms or exit sources outside the ZON range, and no
fall-chance or current fields. Eighty unresolved target records reach beyond
ZON top #1129479, covering 79 VNUMs from #1129480 through #1129579. These are
source gaps, not verified routes to omitted or future rooms.

A hypothetical set containing this candidate plus current active WLD sources
resolves 22,866 exits (all 22,839 local exits and 27 active-area exits). The
current areas/AREA loader omits ud_map2; that static count does not activate
these candidate rooms. The local undirected graph has nine weak components: one
of 6,146 rooms, one of two rooms, and seven singletons. Raw directed traversal
from active Shafts target #1121446 reaches 6,142/6,155 rooms; reverse traversal
reaches 6,146/6,155. Four rooms in the large component can reach the root but
are not forward-reachable from it, and nine rooms are outside its component.
This is authored source topology only, not live access or return qualification.

The 27 source-only exits from this unregistered map to active WLDs are:

| ud_map2 source exit | Current target | Evidence |
| --- | --- | --- |
| #1120305 west to Ixarkon #96492 | Ixarkon | [source](../../../areas/wld/ud_map2.wld#L23) · [target](../../../areas/wld/ixarkon.wld#L2261) |
| #1120404 north to Ixarkon #96492 | Ixarkon | [source](../../../areas/wld/ud_map2.wld#L137) · [target](../../../areas/wld/ixarkon.wld#L2261) |
| #1120722 up to Shafts #10021 | Shafts | [source](../../../areas/wld/ud_map2.wld#L2254) · [target](../../../areas/wld/shafts.wld#L359) |
| #1120813 north to Underworld #4469 | Underworld | [source](../../../areas/wld/ud_map2.wld#L2977) · [target](../../../areas/wld/underworld.wld#L2167) |
| #1121983 west to Clan Stoutdorf #34200 | Clan Stoutdorf | [source](../../../areas/wld/ud_map2.wld#L17769) · [target](../../../areas/wld/drst.wld#L1) |
| #1122501 up to Shafts #10021 | Shafts | [source](../../../areas/wld/ud_map2.wld#L22897) · [target](../../../areas/wld/shafts.wld#L359) |
| #1122559 up to Shafts #10033 | Shafts | [source](../../../areas/wld/ud_map2.wld#L23510) · [target](../../../areas/wld/shafts.wld#L544) |
| #1123442 east to Llzazan #36200 | Llzazan | [source](../../../areas/wld/ud_map2.wld#L36648) · [target](../../../areas/wld/llzazan.wld#L1) |
| #1123784 west to Lair of the Purple Worm #42525 | Lair of the Purple Worm | [source](../../../areas/wld/ud_map2.wld#L41924) · [target](../../../areas/wld/pworm.wld#L586) |
| #1123855 east to Valinhav #90800 | Valinhav | [source](../../../areas/wld/ud_map2.wld#L43071) · [target](../../../areas/wld/valinhav.wld#L1) |
| #1123975 east to Lair of the Purple Worm #42542 | Lair of the Purple Worm | [source](../../../areas/wld/ud_map2.wld#L44832) · [target](../../../areas/wld/pworm.wld#L1004) |
| #1124242 down to Myconid #2300 | Myconid | [source](../../../areas/wld/ud_map2.wld#L48876) · [target](../../../areas/wld/myconid.wld#L1) |
| #1124533 down to Flind #3475 | Flind | [source](../../../areas/wld/ud_map2.wld#L53038) · [target](../../../areas/wld/flind.wld#L1790) |
| #1124682 north to Lair of the Purple Worm #42500 | Lair of the Purple Worm | [source](../../../areas/wld/ud_map2.wld#L55300) · [target](../../../areas/wld/pworm.wld#L1) |
| #1124761 north to Mushroom Forest #10100 | Mushroom Forest | [source](../../../areas/wld/ud_map2.wld#L56455) · [target](../../../areas/wld/mush1.wld#L1) |
| #1124844 east to Caverns of the Worms #6900 | Caverns of the Worms | [source](../../../areas/wld/ud_map2.wld#L57614) · [target](../../../areas/wld/worms.wld#L1) |
| #1124854 up to Khildarak #17000 | Khildarak | [source](../../../areas/wld/ud_map2.wld#L57773) · [target](../../../areas/wld/khildarak.wld#L1) |
| #1124947 north to Clawed Caverns #80700 | Clawed Caverns | [source](../../../areas/wld/ud_map2.wld#L59106) · [target](../../../areas/wld/clwcvrn.wld#L1) |
| #1124947 down to CBL Hall #29703 | CBL Hall | [source](../../../areas/wld/ud_map2.wld#L59122) · [target](../../../areas/wld/cbl_hall.wld#L74) |
| #1125006 down to Gibber #8700 | Gibber | [source](../../../areas/wld/ud_map2.wld#L59882) · [target](../../../areas/wld/gibber.wld#L1) |
| #1125389 down to Valdrak #53100 | Valdrak | [source](../../../areas/wld/ud_map2.wld#L66333) · [target](../../../areas/wld/valdrak.wld#L1) |
| #1125718 south to Mazzolin #21300 | Mazzolin | [source](../../../areas/wld/ud_map2.wld#L70840) · [target](../../../areas/wld/mazzolin.wld#L1) |
| #1126541 up to Pragtog #10649 | Pragtog | [source](../../../areas/wld/ud_map2.wld#L80513) · [target](../../../areas/wld/pragtog.wld#L879) |
| #1126563 north to Truk #1600 | Truk | [source](../../../areas/wld/ud_map2.wld#L80799) · [target](../../../areas/wld/truk.wld#L1) |
| #1127382 down to Domain of Lost Souls #36800 | Domain of Lost Souls | [source](../../../areas/wld/ud_map2.wld#L94634) · [target](../../../areas/wld/dlsc.wld#L1) |
| #1127503 up to Underworld #4608 | Underworld | [source](../../../areas/wld/ud_map2.wld#L96465) · [target](../../../areas/wld/underworld.wld#L5999) |
| #1128575 west to BogenTok #18700 | BogenTok | [source](../../../areas/wld/ud_map2.wld#L114790) · [target](../../../areas/wld/bogentok.wld#L1) |

Twenty-three approaches were already individually recorded; the four newly
reconciled edges are the approaches to Llzazan, Valinhav, Clawed Caverns, and
Truk. All 27 remain source-only, not active ingress, because ud_map2 is absent
from areas/AREA. They raise the curated inactive-WLD total from 11,989 to
11,993 overall / 415 from Surf, and the reviewed ud_map2 subset from 23 to 27.
Each receiving coverage row records its inactive-WLD inbound count; Valinhav
also retains the separately sourced ud_map1 approach.

One current active WLD record targets this candidate:

| Active source exit | Candidate room | Evidence |
| --- | --- | --- |
| Shafts #10012 south to #1121446 | #1121446 | [Shafts source](../../../areas/wld/shafts.wld#L213) · [candidate room](../../../areas/wld/ud_map2.wld#L10911) · [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L19) |

ud_map2 room #1121446 has no authored return exit to Shafts #10012. Five
additional inbound records come from WLD sources absent from areas/AREA:

| Unregistered WLD source exit | Candidate room | Evidence |
| --- | --- | --- |
| bastard.wld #30200 up | #1120721 | [source](../../../areas/wld/bastard.wld#L18) · [candidate room](../../../areas/wld/ud_map2.wld#L2213) |
| bb_gh.wld #30300 up | #1126542 | [source](../../../areas/wld/bb_gh.wld#L14) · [candidate room](../../../areas/wld/ud_map2.wld#L80518) |
| genocide_hall.wld #29500 up | #1124854 | [source](../../../areas/wld/genocide_hall.wld#L15) · [candidate room](../../../areas/wld/ud_map2.wld#L57753) |
| venom.wld #30300 up | #1124260 | [source](../../../areas/wld/venom.wld#L13) · [candidate room](../../../areas/wld/ud_map2.wld#L49196) |
| vodgh.wld #30300 up | #1123336 | [source](../../../areas/wld/vodgh.wld#L16) · [candidate room](../../../areas/wld/ud_map2.wld#L34897) |

The ud_map2 exit from #1126542 down to #30300 is textually reciprocal to
bb_gh.wld #30300 up, but VNUM #30300 is also defined in venom.wld and
vodgh.wld, whose exits target different ud_map2 rooms. Treat that number's
owner and exact route identity as ambiguous until the source owners reconcile
it; do not merge those three WLD rooms by numeric equality.

## Typed reset consumer and bounded reference closure

The [ZON file](../../../areas/zon/ud_map2.zon#L1) contains 268 commands, all
M placements into 96 local rooms (172 placements repeat a room already used
by another command). The 16 requested mobile VNUMs have these command counts:

| Mobile VNUM | Commands | Current prototype status |
| ---: | ---: | --- |
| #4403 | 10 | Active Underworld definition |
| #4404 | 22 | Active Underworld definition |
| #4405 | 17 | Active Underworld definition |
| #4406 | 2 | Active Underworld definition |
| #4410 | 2 | Active Underworld definition |
| #4420 | 28 | Active Underworld definition |
| #4430 | 19 | Active Underworld definition |
| #4450 | 23 | Active Underworld definition |
| #4480 | 2 | Active Underworld definition |
| #4490 | 28 | Active Underworld definition |
| #4510 | 5 | Active Underworld definition |
| #4511 | 5 | Active Underworld definition |
| #4520 | 25 | Active Underworld definition |
| #4530 | 65 | Active Underworld definition |
| #4696 | 14 | Active Underworld definition |
| #44230 | 1 | No current mobile definition |

The first 267 commands request prototypes in active
[Underworld mobile data](../../../areas/mob/underworld.mob#L41). The remaining
command, [ZON line 97](../../../areas/zon/ud_map2.zon#L97), requests missing
mobile #44230 at room #1121556. The same number identifies a room in active
[Tikitt WLD data](../../../areas/wld/tikitt.wld#L2942); it is a room/mobile
namespace collision, not a current mobile prototype definition. These are
source reset instructions in an unregistered zone, not evidence that any
mobile currently resets or spawns.

A bounded exact-token scan found no external ZON room-role reset targeting a
ud_map2 room. No current QST (265 scanned files), SHP (180), story JSON (232),
OBJ (442), or maintained src file contains a room-VNUM reference to this map;
no external object type-25 destination targets it. One numeric token match in
[dungeon.mob line 25](../../../areas/mob/dungeon.mob#L25) is ordinary mobile
data loaded into mob->specials.affected_by, not a room operand
([loader](../../../src/world/db.c#L2664)). The same-stem MOB/OBJ files are
empty. Editor projects, map-generator inputs, deployment configuration, and
persisted room identities were not inspected.

## Title, history, and disposition

After color and whitespace normalization, “the caverns of the underdark” is
unique among 442 current ZON titles (425 normalized title groups). The
ud_map0, ud_map1, and ud_map2 titles are distinct, and their room-number sets
are pairwise disjoint. No ud_map2 room VNUM overlaps any other current WLD
source. Followed WLD/ZON history reaches the August 2009 area-tree move and a
2013 same-stem update; empty MOB/OBJ histories additionally reach 2005/2006
and the same move. No explicit predecessor, successor, alternate-stem rename,
or replacement was found. Similar Underworld/Underdark names, source-only
routes, and shared mobile prototypes do not establish map replacement.

Keep ud_map2 **RETAIN IN PLACE** outside both manifests. Preserve the active
Shafts reference, five inbound references from unregistered WLDs, all 27
source-only approaches, the ambiguous #30300 identity, and all 268 typed ZON
commands—including the one missing mobile prototype—until ownership, editor
and generator roles, deployment, runtime, and persisted-room identity are
resolved. Do not activate, merge, regenerate, move, or archive this component
on the basis of static source counts. See the [25 builder questions](UD_MAP2_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-551-ud_map2-source-graph-and-boundaries),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-551-ud_map2-title-and-room-identity),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-551-ud_map2-retained-with-source-only-routes-and-resets),
[candidate dependency row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114),
and [coverage row 464](ZONE_COVERAGE_REGISTER.csv#L465).
