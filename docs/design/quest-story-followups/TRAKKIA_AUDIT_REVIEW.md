# The Trakkia Mountains repository-wide audit supplement

This supplement reconciles active and inactive connectivity, typed dependencies, exact title ownership, and bounded source history for the [source-comprehensive Trakkia dossier](../zone-stories/TRAKKIA_MOUNTAINS.md). The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed.

## Active graph and Surface boundaries

The active `trakkia` WLD has 166 rooms and 387/387 resolving authored exits: 385 local and two outbound. Its five weak components contain 109, 28, 16, 12, and 1 rooms. From entry #57000, raw directed traversal reaches 72/166 and reverse traversal finds 106/166 rooms with a directed path back to #57000 when the four source-flagged exits are unavailable. If all four are assumed available, forward reach rises to 104/166 while reverse reach remains 106/166. The blocked records are #57001 down→#57067, #57119 south→#57103, #57134 west→#57136, and #57136 east→#57134 ([first](../../../areas/wld/trakkia.wld#L41), [second](../../../areas/wld/trakkia.wld#L2750), [third](../../../areas/wld/trakkia.wld#L3030), [fourth](../../../areas/wld/trakkia.wld#L3066)). These raw scenarios do not model switch access, doors, keys, visible portals, actor admission, or survival.

Both outbound edges from #57000 are reciprocated by active Surface records: east→#629225 and south→#629624, with one reverse record for each ([Trakkia exits](../../../areas/wld/trakkia.wld#L11), [#629624 exit](../../../areas/wld/trakkia.wld#L15), [Surface returns](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L613), [Trakkia pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L666)). The WLD has four fall fields and no current fields. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L310) and [dossier access scenarios](../zone-stories/TRAKKIA_MOUNTAINS.md).

The dossier's scenario counts depend on declared switches, key states, object portals, visibility, stock, admission, and hazard survival. The ordinary root traversal reaches only 72 rooms, and even the all-blocked-edges-open scenario reaches 104; neither figure establishes the larger conditional scenario or a played return. Keep the five weak components and the five staging rooms distinct from a complete player route.

## Inactive-WLD approach and cumulative map evidence

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues, and found no room-VNUM overlap with active Trakkia. It found one non-overlapping source-only approach: inactive `surf.wld` #246406 west→#57000 ([source room](../../../areas/wld/surf.wld#L973710), [exit](../../../areas/wld/surf.wld#L973726)). This is not active ingress. At Checkpoint 132 it raised curated totals from 2,610 to 2,611 overall and from 152 to 153 for Surf; the approach is already included in the current 11,993 overall / 415 Surf totals. Preserve the Surf record and keep `surf` / `surface2011` **RETAIN** pending map ownership and remaining dependency/tool closure; keep active `trakkia` **DO NOT MOVE**.

## Typed dependencies and bounded negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Trakkia's two Q24 reward prototypes | The addressed Gauce return outputs external object #55323 and vial #32019 (`R I` commands at [QST lines 29–30](../../../areas/qst/trakkia.qst#L29)); their definitions belong to `wh.obj` ([aura](../../../areas/obj/wh.obj#L3992)) and `avernus.obj` ([vial](../../../areas/obj/avernus.obj#L227)). Preserve these as two outputs of the local QST; they are not Trakkia inputs or proof of later use. The dossier maps the aura's separate external recipe consumers. |
| Local reset imports | Active `trakkia.zon` resets imported epic stone #359 and memory #55436 in #57097 ([resets](../../../areas/zon/trakkia.zon#L115), [memory reset](../../../areas/zon/trakkia.zon#L121)); their definitions are in `heavens.obj` ([stone](../../../areas/obj/heavens.obj#L3725)) and `wh.obj` ([memory](../../../areas/obj/wh.obj#L5169)). The stone's compiled epic behavior is separately mapped in the dossier; the memory's QUESTITEM flags do not create a selected story handler. |
| External reset references to Trakkia prototypes | Active `alatorin.zon` resets local potion #57003 ([active reset](../../../areas/zon/alatorin.zon#L3799)). Inactive/unregistered `brass-old-1.zon` has a source-only mobile reset for local mobile #57060 ([source-only reset](../../../areas/zon/brass-old-1.zon#L668)); this is not live placement. No outside exact QST item consumer, external shop product, or duplicate external mobile/object prototype was found. |
| Local shop and compiled service binding | `trakkia.shp` declares one local witch shop with four local products #57001/#57002/#57003/#57009 ([shop header and stock](../../../areas/shp/trakkia.shp#L1)); it is not an external dependency. The compiled `pet_shops` room function is assigned at #57068 ([binding](../../../src/specs/specs.assign.c#L2594)); the dossier separately records active-accounting purchase/rental refusal. |
| Portal and sidecar boundaries | The selected area contains seven local type-25 object portals as documented in the dossier. The scan of 865 outside active type-25 declarations found no target into Trakkia. The external sidecar scan found no typed room/mobile/object VNUM reference to the area, and the bounded numeric scan produced no external story token hit. |

The reset and QST references are static source declarations. They do not establish current reset stock, a personal source kill, a completed soul return, future hireling availability, a successful shop/pet transaction, or durable epic publication. Keep imported objects, external quest reward prototypes, the active Alatorin reset, and the unregistered Brass source-only command separated by ownership and active status.

## Title, history, and disposition

After stripping color tokens and normalizing whitespace and case, “The Trakkia Mountains” has one exact owner among 351 active `.zon` titles: `trakkia` ([title source](../../../areas/zon/trakkia.zon#L2)). Followed WLD history includes same-stem changes through 2013; followed MOB/OBJ/ZON history records same-stem switch/Tharn access updates in 2010, 2012–2014 maintenance, 2016 chest changes, and 2026 ambassador/equipment changes. The story sidecar was added on 2026-10-06 in commit `287c9070e`. No alternate-stem rename, explicit predecessor, or whole-zone replacement appears in these bounded paths. Keep active `trakkia` **DO NOT MOVE** and preserve the inactive Surf approach as map evidence. No source placement changed.

## Builder recommendations

1. Preserve the two reciprocal Surface exits and four source-flagged blocked records. The root and all-edge traversals do not replace the dossier's separate switch/key/portal scenarios or qualify entry, survival, and return.
2. Retain the Surf #246406 approach as inactive source evidence. Its Checkpoint 132 cumulative snapshot was 2,611 overall / 153 Surf; it is already included in the current 11,993 overall / 415 Surf total. Do not count it as active access or infer that the legacy map replaces Trakkia.
3. Keep Q24 reward objects #55323/#32019 distinct from local inputs, and preserve local reset imports #359/#55436 with their original source identities. The Aura's independent foreign recipes and the epic node require their own outcomes.
4. Preserve the active Alatorin reset for #57003, the source-only Brass reset for #57060, the local four-product witch shop, and the #57068 compiled pet-shop binding as separate typed edges with their current active/source-only status.
5. Keep the exact four-root grangle requirement and separate soul, ancestral ring, lost-flock, and reward receipts. Narrative outcomes, raw traversal, current inventory, and authored resets do not qualify stock, personal source work, or durable completion.

Checkpoint 397 records 20 explicit builder questions and conservative
recommendations in the [source dossier](../zone-stories/TRAKKIA_MOUNTAINS.md#explicit-builder-questions-and-recommended-resolutions).
The questions retain the current QST predicates and separate future story,
availability, ownership, service, and played-qualification work. No world or
gameplay source changed.

## Checkpoint 852: Current graph, source-only approach, and typed closure carried forward

The reviewed `areas/` and `src/` content remains identical to pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`; this checkpoint carries the Checkpoint 132 source findings and Checkpoint 397 builder review forward without claiming a second independent parse. Trakkia remains a 166-room WLD with 387/387 resolved exits, two reciprocal Surface boundary pairs, five weak components, and four source-flagged blocked exits. Inactive Surf #246406 west to #57000 remains source-only, is already included in the current cumulative total of 11,993 overall / 415 Surf, and adds no active ingress or new candidate. The older 2,611 / 153 figure is the Checkpoint 132 snapshot.

The recorded typed closure remains unchanged: Q24 outputs foreign objects #55323/#32019; local resets import Heavens #359 and Winterhaven #55436; active Alatorin resets local OBJ #57003; unregistered `brass-old-1.zon` contains a source-only reset of local MOB #57060. The local four-product shop and compiled `pet_shops` binding at #57068 remain separate configured dependencies. Seven local item portals and the conditional switch/key/hazard routes remain distinct from WLD edges. The twenty builder questions in Checkpoint 397 remain source review, not gameplay qualification.

Keep active `trakkia` **DO NOT MOVE**. Preserve inactive Surf #246406 on existing candidate row 97 and keep `surf` / `surface2011` **RETAIN IN PLACE** pending ownership and tooling closure. No source activation, relocation, or migration clearance follows. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L310), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-852-trakkia-graph-and-surf-approach-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
