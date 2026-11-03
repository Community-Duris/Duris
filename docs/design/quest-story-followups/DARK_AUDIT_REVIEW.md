# Guildhall of the Dark: map, shop, and identity audit

Reviewed October 11, 2026, Checkpoint 467, against source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static review of the unregistered `dark` component. It does not establish that the component is loaded or that its shop, exits, or resets operate at runtime.

## Source scope and story

The candidate consists of [`dark.wld`](../../../areas/wld/dark.wld), [`dark.mob`](../../../areas/mob/dark.mob), [`dark.obj`](../../../areas/obj/dark.obj), [`dark.zon`](../../../areas/zon/dark.zon), and [`dark.shp`](../../../areas/shp/dark.shp). It is absent from both the full [`areas/AREA`](../../../areas/AREA) and [`areas/qst/AREA`](../../../areas/qst/AREA) lists and has no QST file or story sidecar. Its raw AREA annotation is blank. The ZON title is “guildhall of the dark,” header #294, last VNUM #29409.

The ten WLD rooms describe a temple of “Eternal Tenaciousness,” its grand foyer, and a stainless-steel maze. Three mobile prototypes include two temple sentries and an elderly priest; two OBJ prototypes define a waterfall basin and a clerics-for-offense pamphlet. The shop source contains coarse, sexualized language in one response message; retain it for owner/editorial review rather than silently rewriting it during migration work.

The ZON has five M resets (three repeats of mobile #29400, plus #29402 and #29401), nine O resets, and twelve G resets. Its one local O placement is waterfall #29400 in room #29404. Eight O records import objects outside `dark.obj`: Harrow #29410, #29412–#29415 and Player Castles #11013, #11059, #11109. The G resets equip mobile #29401 with the same twelve VNUMs listed in its SHP record.

## Static WLD graph and Harrow identity overlap

| Measure | Result |
| --- | --- |
| Rooms / authored directions | 10 / 37 |
| Candidate-local targets | 36 |
| Targets matching active Harrow room VNUMs | 36; source and target identities remain ambiguous |
| External target | #29400 up → inactive Duris3 #156726, “Grassy Plains” |
| Local weak components / largest | 1 / 10 rooms |
| Plain directed reach from #29400 | 10/10 candidate rooms |
| Positive unresolved / negative targets | 1 / 0 |
| Active WLD inbound to the Dark map identity | 0 |
| Fall / current fields | 0 / 0 |

All ten `dark.wld` rooms #29400–#29409 reuse the same-number active Harrow rooms, and all ten room blocks differ from Harrow’s. The three candidate MOBs #29400–#29402 and two OBJ records #29400–#29401 likewise collide with Harrow prototypes but have different complete records. All 36 local directions target VNUMs also owned by Harrow. They are candidate-local edges in this file and were already recorded as numeric identity matches in the [Harrow audit](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches); they do not make the Dark rooms active. Harrow’s own active Surface boundaries at room #29400 belong to Harrow and are not Dark ingress.

The sole external direction is from Dark temple room #29400 up to Duris3 #156726. That target belongs to unregistered `Duris3.wld` and has no reverse direction to #29400. Neither endpoint is in the active loader set. The Dark file adds no active approach or curated source-only route; preserve the current global inactive-WLD totals. The graph traversal ignores door state, live conditions, and safety.

## Conditional SHP fallback and exact numeric consumers

`dark.shp` contains one record headed #29401. It lists twelve products: #29401, #364, #385, #377, #3050, #398, #53, #108, #109, #111, #6089, and #412. Its keeper VNUM is #29401 and its shop room is #29408. Both names collide with active Harrow identities: active Harrow #29401 is a gardener mobile and Harrow #29408 is Fountain Square; the candidate defines an elderly priest and maze at those numbers. Product #29401 resolves to the active Harrow large bag rather than the candidate pamphlet if mapped through the current active object table. The other current numeric owners are nine Heavens objects and Verz1 #3050; no active object prototype #6089 was found.

There is no `areas/world.shp` in this checkout. The non-mini [`open_shop_stream`](../../../src/economy/shop.c#L3267) falls back to globbing `areas/shp/*.shp`; that path can encounter `dark.shp` despite Dark’s absence from `areas/AREA`. The separate unregistered [`infernal_gh.shp`](../../../areas/shp/infernal_gh.shp#L1) also declares shop record #29401. The current code maps products through `real_object` ([`shop.c`](../../../src/economy/shop.c#L3380)), keeper through `real_mobile` ([`shop.c`](../../../src/economy/shop.c#L3493)), and room through `real_room` ([`shop.c`](../../../src/economy/shop.c#L3092)). These are conditional source-code paths; no shop loader or game session was run.

The five non-Player-Castles object resets in `dark.zon` (#29410, #29412–#29415) resolve by VNUM to active Harrow objects. Those same VNUMs also define portals in unregistered `darkop.obj`, so the intended relationship among `dark`, `darkop`, and active Harrow remains unresolved; see the [Dark Outpost audit](DARKOP_AUDIT_REVIEW.md#candidate-map-and-cross-area-identity). The three Player Castles imports resolve to active Player Castles objects. The repeated shop key, keeper/room ownership mismatch, #6089 without an active prototype, low-VNUM Harrow collisions, and sexualized message all need owner review. See the [30 builder questions](DARK_BUILDER_QUESTIONS.md) and [Harrow’s typed-reference findings](HARROW_AUDIT_REVIEW.md#bounded-typed-and-code-references).

## Title, history, and retention

After color-token and whitespace normalization, “Guildhall of the Dark” is unique among 442 current ZON titles. A separate unregistered `dark_gh` component has the similar title “the Dark Guildhall” and a different #29200–#29242 range; similarity alone does not establish aliasing, merger, or replacement.

Followed histories for the five `dark` paths reach the 2009 area-tree move. Later visible edits include 2012 mobile/object field updates, 2013 source maintenance, and the 2015 ZON quest-item flag pass. No alternate-stem rename, predecessor, or explicit whole-zone replacement is visible in those paths. This bounded history does not establish earlier upstream lineage.

Keep candidate row 30 **RETAIN IN PLACE**. Resolve the Harrow room/mobile/object collisions; the unregistered Duris3 exit; the five Harrow and three Player Castles object imports; the SHP wildcard and duplicate #29401 record with `infernal_gh`; the Harrow keeper/room/product mappings; the missing active #6089 product; and editor, deployment, content-review, and persisted-data ownership before any separate move or activation proposal. See the [30 builder questions](DARK_BUILDER_QUESTIONS.md), [coverage row 381](ZONE_COVERAGE_REGISTER.csv#L381), [candidate row 30](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L30), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-467-guildhall-of-the-dark), [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-467-dark-map-shop-identity-and-history), and [migration plan](ATTIC_MIGRATION_PLAN.md#checkpoint-467-dark-retained-pending-owner-review).
