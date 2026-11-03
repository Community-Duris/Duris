# The Genocide Guild Hall source audit

Canonical key: component:genocide_hall. Physical coverage row 398 / CSV line
399; candidate dependency row 48 / CSV line 48. Reviewed October 11, 2026,
Checkpoint 485, against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static source,
connectivity, identity, history, and typed-reference review. It does not
establish runtime loading, travel, shop behavior, editor ownership, persisted
world state, or permission to activate or move these files.

## Source inventory and range ownership

The unregistered component consists of [WLD](../../../areas/wld/genocide_hall.wld#L1),
[MOB](../../../areas/mob/genocide_hall.mob#L1), an empty
[OBJ file](../../../areas/obj/genocide_hall.obj), [ZON](../../../areas/zon/genocide_hall.zon#L1),
and one [SHP record](../../../areas/shp/genocide_hall.shp#L1). WLD has seven
rooms #29500–#29504 and #29510–#29511. MOB has four prototypes #29500–#29503;
OBJ has none. ZON header #295 is titled “Genocide Guild Hall,” ends at #29511,
and has reset mode 0. The SHP record uses keeper #29503 in room #29503 and
lists twelve products. There is no same-stem QST, story sidecar, editor map,
or cap file among the current area files.

Neither full [areas/AREA](../../../areas/AREA#L142) nor
[areas/qst/AREA](../../../areas/qst/AREA#L132) lists genocide_hall. Both
manifests assign the #294–#296 band to active Harrow. The Harrow WLD does not
define these seven #295 room records, so that band assignment is not evidence
that these separate genocide_hall records are loaded or Harrow-owned. The raw
candidate annotation is blank. “Genocide Guild Hall” is unique among the 442
tracked ZON titles; the separate `genocide_gh` candidate is header #2920 with
rooms #29200–#29209.

## Authored map and source-only boundaries

Six rooms contain authored descriptions. Their titles and text describe a
diabolical blood-stained entryway, a sadistic chamber, a dark-skull inn,
Oxsurd’s Oddities, an ancient passageway, and a marble chamber. Room #29504
is titled “Unnamed” and has an empty description. Three golem prototypes are
described as bloodstained; #29503 is Henrik Oxsurd, the muscular merchant
behind the counter. The source does not explain how this place relates to the
separate genocide_gh guildhall.

The WLD contains twelve directions: ten local edges and two D4 edges to other
maps. Its local graph has two weak components, sizes four and three; the
largest is four rooms. Directed traversal reaches four source rooms from
#29500 and three from #29504. No negative targets, fall records, or currents
are present. All twelve targets are unresolved in the current full loader
list because this source is unlisted and its two external target sources are
also unlisted.

| Source exit | Authored target | Current boundary finding |
| --- | --- | --- |
| #29500 D4 | #1124854 in `ud_map2.wld`, “The Endless Rock Filled Cavern Ithilienienzuth” | `ud_map2` is not in areas/AREA; the target record has no direct edge back to #29500. |
| #29510 D4 | #144550 in `Duris3.wld`, “The Ocean” | Duris3 is not in areas/AREA; the target record exits to #144548, not back to #29510. |

The two local components remain disconnected in the authored map. No
currently listed active WLD direction targets #29500–#29511. The same-number
map records in the separate unregistered `aod_gh`, `legion`, and `rf_op`
sources are different records, not active destinations. See the
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-485-genocide-guild-hall-map-and-boundaries).

## Resets, imports, and shop stock

The ZON has 25 reset commands under mode 0: four O imports, nine M
placements, and twelve G placements. It imports Player Castles objects
#11052 into rooms #29500 and #29510 and #11102 into #29501 and #29511. Four
golem placement commands serve the #29500 component, Henrik #29503 is placed
in the shop room, and four more golem commands serve #29510. The twelve G
resets match the one SHP record’s twelve products:

| Product | Current OBJ definition owner(s) |
| --- | --- |
| #412 | heavens |
| #53 | dalvik, heavens, hostel, mavulsk |
| #347 | heavens |
| #364 | heavens |
| #6089 | No current OBJ declaration |
| #398 | heavens |
| #3050 | verz1 |
| #377 | heavens |
| #491–#494 | heavens |

The genocide_hall OBJ file is empty, so no shop product is declared locally.
The intended identity for duplicate #53 definitions is unresolved, and
#6089 has no current OBJ declaration. The 2026 kingdom commit added gathering
bags #491–#494 to both genocide_hall and genocide_gh shop/reset lists. This is
parallel maintenance evidence, not evidence that either source replaces the
other.

The SHP keeper and room are both #29503. The keeper mobile and shop room are
not present in the current full-area WLD/MOB identity set, though the active
Harrow manifest owns the surrounding numeric band. `areas/world.shp` is
absent. In non-mini mode, `open_shop_stream()` falls back to `areas/shp/*.shp`
when that file cannot be opened ([shop loader](../../../src/economy/shop.c#L3276),
[configured path](../../../src/world/db.h#L22)). The loader maps the keeper
through `real_mobile()` ([lookup](../../../src/economy/shop.c#L3492)); if this
fallback includes genocide_hall, its #29503 keeper has no matching current
mobile prototype. `assign_the_shopkeepers()` then indexes `mob_index[keeper]`
without a negative-index guard
([assignment](../../../src/economy/shop.c#L3602)). This is a conditional
static loading risk, not evidence that the fallback or shop runs at runtime.

Compiled code also assigns the inn handler to `real_room0(29502)` in
[specs.assign.c](../../../src/specs/specs.assign.c#L2449). The candidate calls
#29502 “The Dark Skull Inn”; same-number WLD records also exist in aod_gh and
legion, while the current Harrow WLD has no #29502 room record. The numeric
binding does not establish which unregistered source, if any, owns that
handler. Resolve this with the map and code owners.

## Same-number identities, history, and disposition

The genocide_hall WLD shares #29500–#29504 and #29510–#29511 with different
unregistered `aod_gh` and `legion` WLD records. `rf_op` also defines distinct
WLD rooms #29510 and #29511. MOB identities overlap: aod_gh and legion define
same-number mobiles #29500–#29502, and legion also defines #29503. The
candidate OBJ is empty, while aod_gh and legion define their own objects
#29500–#29504 and rf_op defines objects #29510–#29511. These source records
must stay typed by their own stems. The active Harrow area/QST range and the
compiled #29502 inn assignment are separate ownership questions; they do not
make the unlisted genocide_hall files active.

Followed WLD, MOB, OBJ, ZON, and SHP histories remain on the genocide_hall
stem through the 2009 area-tree move and later same-stem maintenance. The
2026 gathering-bag commit updated genocide_hall and genocide_gh in parallel.
No alternate-stem rename or explicit whole-zone replacement appears in the
followed paths. The other maps that reuse #295xx have their own stems and
records; numeric overlap does not establish lineage.

Keep candidate row 48 **RETAIN IN PLACE**. Preserve the seven-room map and
two disconnected components, both source-only D4 edges, four mobile
definitions, four Player Castles imports, empty OBJ file, twelve matching
shop/reset products, and all same-number identities. Resolve Harrow’s
#294–#296 range, map and room ownership, the #29502 inn binding, fallback
shopkeeper/room mapping, stock prototypes, manifest/editor state, and
persisted IDs before any activation, move, rename, archive, or merge proposal.
See the [24 builder questions](GENOCIDE_HALL_BUILDER_QUESTIONS.md),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-485-genocide-guild-hall-identities-and-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-485-genocide-guild-hall-retained-pending-range-and-shop-ownership),
[candidate row 48](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L48), and
[coverage row 398](ZONE_COVERAGE_REGISTER.csv#L399). No source-area files,
code, tests, builds, runtime, server, database, or containers were changed
or used.


## Checkpoint 897: role-aware closure across the overlapping map band

The source tree remains unchanged from snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697` through HEAD `c500ac51b8cd26540943257e7ca4e9807263ef74`; no `areas/` or `src/` change adds or removes a reference. The current area manifest still omits `genocide_hall` and assigns the broad #294-#296 range to Harrow.

A typed scan of all ZON sources found matches only in unregistered `aod_gh.zon`, `legion.zon`, and `rf_op.zon`: 25 role-field matches each in AoD and Legion, and four room operands in RF Outpost. AoD and Legion's M prototype/room operands and O room placements belong to their own same-number mobile/map records; RF Outpost's four operands are room placements into its own #29510 room. The exact outside ZON matches do not establish genocide_hall resets. The candidate OBJ is empty, so no outside typed object reset targets a candidate object. See the [AoD resets](../../../areas/zon/aod_gh.zon#L9), [Legion resets](../../../areas/zon/legion.zon#L9), and [RF Outpost resets](../../../areas/zon/rf_op.zon#L11).

A full WLD destination scan found 28 external direction records to numbers in the candidate room set: thirteen in AoD, twelve in Legion, and three in RF Outpost. All three WLD sources are unregistered and each defines its own overlapping room records. No active WLD source targets #29500-#29511, so the active inbound count remains zero and these authored edges add no current route. The candidate's two D4 boundaries and its local component counts remain as documented at CP485. See [AoD map edge](../../../areas/wld/aod_gh.wld#L14), [Legion map edge](../../../areas/wld/legion.wld#L9), and [RF Outpost room edge](../../../areas/wld/rf_op.wld#L21).

The two external type-25 portal destinations are both in `legion.obj`: object #29501 stores #29503 and object #29502 stores #29502. No external type-29 switch room target enters the candidate set. These object targets are not WLD edges; the destinations remain ambiguous among separate unregistered same-number maps. See [Legion portal #29501](../../../areas/obj/legion.obj#L15) and [Legion portal #29502](../../../areas/obj/legion.obj#L23).

The exact-value scan found no candidate-range value in the 265 QST files or 232 story sidecars. External SHP values occur only in AoD and Legion and have distinct field types: AoD shop #29501 stocks source-local OBJ #29500, uses its own keeper #29501, and room #29503; Legion shop #29500 stocks its own OBJ #29500 and uses keeper #29504, which is outside the candidate's mobile set. The external role overlap with the candidate is AoD's same-number keeper and shop room; it does not establish that either shop loads. The candidate SHP still uses its own #29503 keeper and room, with loader behavior conditional on a valid shop stream. See [AoD keeper](../../../areas/shp/aod_gh.shp#L52), [AoD shop room](../../../areas/shp/aod_gh.shp#L54), [Legion stock](../../../areas/shp/legion.shp#L43), [Legion keeper](../../../areas/shp/legion.shp#L45), and the [conditional loader path](../../../src/economy/shop.c#L3276).

The only exact executable C/C++ room consumer remains the compiled inn assignment `real_room0(29502)` at [specs.assign.c](../../../src/specs/specs.assign.c#L2449). It still overlaps different unregistered AoD, Legion, and genocide_hall room records; the reference does not identify which room receives the handler. No new alternate-stem rename or replacement evidence was found. Keep row 48 **RETAIN IN PLACE**. Preserve the CP485 graph/history assessment and resolve the source-owned external WLD, ZON, portal, shop, handler, and manifest roles before any activation or path proposal. The [24 builder questions](GENOCIDE_HALL_BUILDER_QUESTIONS.md) record the added evidence.
