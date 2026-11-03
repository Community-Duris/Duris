# The Ruins of Undermountain audit supplement

**Checkpoint 51 — active and source-only connectivity, exact typed and code
references, title, and tracked-path history.** The existing
[comprehensive source map](../zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md)
remains the authority for local quests, rooms, mobiles, objects, resets, hidden
sources, access, and dormant controllers. This supplement records bounded
cross-area evidence without changing native sources.

## Active WLD boundaries

All 1,059 authored exits resolve across 441 Undermountain rooms, which form one
weak component in the static authored-exit graph. The west route from
Undermountain room #92501 south to `bs` room #74045 has a reciprocal north exit
back to #92501 ([outbound ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1121),
[return ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L855)). The passage from
#92518 down to Underworld room #4557 has no direct reciprocal boundary record
([ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1122)). Underworld #4557's up
exit leads to Kvarkpass Connector #54841; that boundary pairs with the down
exit from #54841 ([Underworld source](../../../areas/wld/underworld.wld#L4623),
[outbound ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L73),
[return ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L745)). These records do
not establish door state, actual player reachability, or safe travel and return.

## Unregistered WLD review

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored exits with zero parse issues. None of their 527,791 source rooms
reuses an Undermountain room VNUM, and no unregistered exit target matches an
Undermountain room. No source-only ingress candidate was found. The curated
source-only follow-up total remains 586; the individually reviewed `surf.wld`
subset remains 26.

## Bounded typed and code references

The exact external typed scan found these active references to Undermountain
prototypes:

| Source | Exact reference | Evidence boundary |
| --- | --- | --- |
| IceCrag quest | Two deliveries of object #92048, the local elven winebottle, are required alongside two IceCrag venisons ([QST](../../../areas/qst/icecrag.qst#L322); [story field](../../../areas/story/icecrag.story.json#L527)). | This is a foreign consumer under IceCrag's quest; its receipt and outcome remain owned by IceCrag. |
| Caertannad resets | Twenty `M` commands reference mobile #92051 at room #78581 ([first command](../../../areas/zon/caertannad.zon#L1009); [last command](../../../areas/zon/caertannad.zon#L1028)). | These are foreign mobile resets, not Undermountain room exits. |
| Brass resets and shop | Active Brass `G` reset #92046 and shop record stock #92046 reference the local mithril lockpick prototype ([reset](../../../areas/zon/brass.zon#L560); [shop stock](../../../areas/shp/brass.shp#L73); [object](../../../areas/obj/undermountain.obj#L475)). | The reset and stock are foreign supply sources; their runtime availability was not tested. |
| FirePlane reset | One `O` reset places local object #92121, Flame of the North, in room #88398 ([reset](../../../areas/zon/firep.zon#L105); [object](../../../areas/obj/undermountain.obj#L1258)). | The foreign reset remains owned by FirePlane. |

Two unregistered source groups add source-only references. `brass-old-1.zon`
has a `G` reset for #92046 and `brass-old-1.shp` lists #92046 as shop stock
([reset](../../../areas/zon/brass-old-1.zon#L558),
[stock](../../../areas/shp/brass-old-1.shp#L70)); `nexus56.zon` resets
Undermountain rum barrel #92050 and `nexus56.shp` lists it as sale stock
([reset](../../../areas/zon/nexus56.zon#L1386),
[stock](../../../areas/shp/nexus56.shp#L546),
[object](../../../areas/obj/undermountain.obj#L507)). The shop records are not
in the un-commented `areas/AREA` list. The shop loader opens `areas/world.shp`
and, if that file is unavailable, falls back to `areas/shp/*.shp`
([loader](../../../src/economy/shop.c#L3272),
[fallback](../../../src/economy/shop.c#L3276)); `areas/world.shp` is absent in
this checkout. This is potential fallback parser exposure, not proof that
either source-only shop operates. No runtime was run.

A structured scan of all other 231 story sidecars parsed without errors and
found one exact VNUM field: IceCrag's item list for #92048. Generic text
matches are the same wine in that contract's completion strings, not additional
prototype references. The exact C/C++ literal scan found the already-documented
Avernus staff controller reading mobile #92076 twice
([first branch](../../../src/specs/specs.avernus.c#L97),
[second branch](../../../src/specs/specs.avernus.c#L136)) and a staff test-command
lead reading #92076 ([test command](../../../src/cmd/testcmd.c#L950)). These
references do not establish player arrival in Undermountain or a new local
quest. Outside the area's own assignments and dormant routines already mapped
in the comprehensive dossier, no other direct C/C++ numeric literal matched
an Undermountain room, mobile, or object VNUM.

## Active title and tracked history

Color-code stripping, whitespace collapse, and case folding across all 442
current `.zon` titles yield one “The Ruins of Undermountain” title, owned by
`undermountain` ([manifest entry](../../../areas/AREA#L326),
[zone title](../../../areas/zon/undermountain.zon#L2)). The tracked WLD and ZON
paths reach the August 2009 area-tree relocation. Later path history includes
content and metadata edits, but identifies no distinct predecessor or
replacement. The semantic map-name comparison is recorded in Checkpoint 764;
runtime command-target alias ownership remains a separate review.

## Checkpoint 764: semantic names and map identity

The all-442 title scan finds no second current `.zon` owner for Undermountain.
The separately rostered `underworld` and `underworld3` contributors both title
their maps “The Underworld”; active `underdark` has its own Durian Underdark
title ([roster entries](../../../areas/AREA#L23),
[underworld3](../../../areas/AREA#L145),
[underdark](../../../areas/AREA#L413); titles:
[underworld](../../../areas/zon/underworld.zon#L2),
[underworld3](../../../areas/zon/underworld3.zon#L2),
[underdark](../../../areas/zon/underdark.zon#L2)). `underworld2`, `ud`, and
`ud_map1` have distinct Underworld/Underdark titles but are unregistered
current source components, not full `areas/AREA` contributors
([coverage rows](ZONE_COVERAGE_REGISTER.csv#L461),
[row 464](ZONE_COVERAGE_REGISTER.csv#L464),
[row 467](ZONE_COVERAGE_REGISTER.csv#L467)). None of these sources identifies
another map owner, predecessor, or replacement for `undermountain`.

The map boundary is a separate connectivity fact: Undermountain room #92518
has a down exit to Underworld room #4557, with no direct reverse exit
([Undermountain room](../../../areas/wld/undermountain.wld#L10022),
[Underworld room](../../../areas/wld/underworld.wld#L4603)). This directed edge
connects two separately rostered areas; it does not make either map an alias
or replacement for the other.

Cross-area text references also have distinct roles. IceCrag's quest names an
item rumored to be in Undermountain, and its story declares an Undermountain
crate as a wine source; these are IceCrag's consumer and dependency references
([quest](../../../areas/qst/icecrag.qst#L297),
[story](../../../areas/story/icecrag.story.json#L12)). Lortower mobile prose
mentions Grimlocks in Undermountain as lore
([mobile source](../../../areas/mob/lortower.mob#L2533)). Desert City's and
Nexus's lowercase “undermountain” wording and Mountain Tracks' under-mountain
cavern descriptions name local terrain in those sources, not another current
Undermountain map ([Desert City](../../../areas/story/desert.story.json#L8),
[Nexus](../../../areas/story/nexus.story.json#L6),
[Mountain Tracks](../../../areas/wld/mountaintracks.wld#L369)).

This closes the source-backed semantic map-identity comparison for the active
contributor. Runtime command-target alias and owner disambiguation remain
separate. Keep `undermountain` active and **DO NOT MOVE**; no attic candidate
row is warranted.

## Scope and disposition

Undermountain remains an active `areas/AREA` contributor. Its reciprocal `bs`
pair and the one-way Underworld boundary stay in the active connectivity
ledger; the all-95 WLD scan found no source-room overlap or target match. Exact
external references include IceCrag's wine consumer, active Caertannad, Brass,
and FirePlane resets, the active Brass shop stock, and source-only Brass/Nexus56
reset and shop records. Avernus code can read Undermountain mobile #92076.
Dynamic access, arrival, shop operation, item availability, reward settlement,
and persisted-player outcomes remain unverified. No migration or gameplay
decision follows from this review.

This documentation-only review used source and static registers. It did not
run tests, a build, server/runtime/gameplay, database work, a migration, or an
operational script. The source-comprehensive audit queue is now **91/220
complete, 129 pending**; priority 92, Desolate Under Fire (`desolateinv`), is
next.

## Checkpoint 412: explicit builder-question coverage

Physical coverage row 325 now has exactly twenty numbered, source-linked
builder questions with conservative recommendations in the [Undermountain
dossier](../zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md#explicit-builder-questions-and-recommended-resolutions).
They keep the native Tamsil and Durnan hand-ins distinct from source recovery,
key access, hidden-item visibility, alternate routes, outside consumers, and
dormant controllers. No area or gameplay source changed.

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has 20 mobile reset rows for Undermountain mobile #92051. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).
## Checkpoint 867: current map and typed owners carried forward

CP867 carries forward the Checkpoint 51 cross-area audit and Checkpoint 764 identity review against the unchanged pinned area/code snapshot. Active Undermountain remains 441 rooms and 1,059/1,059 resolving exits in one weak component. Room #92501 south to Brass Fingers #74045 has its reciprocal north return; #92518 down reaches Underworld #4557 without a direct reverse record, and #4557's up exit reaches Kvarkpass Connector #54841. The all-95 inactive-WLD sweep found no room overlap or target match into active Undermountain.

The existing typed closure remains: IceCrag consumes local wine #92048; active Caertannad has 20 reset placements for mobile #92051; active Brass reset/shop sources reference lockpick #92046; FirePlane resets local object #92121; and Avernus code reads mobile #92076. Source-only `brass-old-1` and Nexus56 reset/shop records refer to local #92046/#92050 and stay with their separate candidate plans. No outside duplicate prototype, outside type-25 target, or typed sidecar item reference was found. The local boundary/source map is unchanged, and the separate active Underworld/Underdark identities remain distinct from Undermountain.

The older Checkpoint 51 source-only subtotal of 586 overall / 26 Surf is historical; current cumulative source-only totals remain 11,993 overall / 415 Surf. Keep active `undermountain` **DO NOT MOVE**. Preserve its note/key sources, closed grate and alternate access cues, hidden object flags, rare monster distribution, and foreign supply/consumer owners. Add no candidate or migration clearance. Door state, player access, travel safety/return, Avernus/controller outcomes, and indirect/runtime dependencies remain outside this static review. See the [source dossier](../zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md), [Checkpoint 764 identity review](#checkpoint-764-semantic-names-and-map-identity), and [coverage row 325](ZONE_COVERAGE_REGISTER.csv#L325).
