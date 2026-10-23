# realms_master shop-source audit

Canonical key: component:realms_master. Physical coverage row 437 / CSV line 438; candidate dependency data row 86 / CSV line 87. Reviewed October 11, 2026, Checkpoint 524, against source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static shop-source, typed-reference, loader, and code review. It does not establish that either record loads or that a shop can be reached or used.

## Source scope and records

The only current component file is [areas/shp/realms_master.shp](../../../areas/shp/realms_master.shp#L1). It contains two SHP records with header keys #11108 and #11109. There is no same-stem WLD, MOB, OBJ, ZON, QST, or story sidecar, and realms_master is absent from areas/AREA. No ZON title exists to establish a canonical zone title or alias.

| SHP record | Product OBJ VNUMs | Keeper MOB VNUM | in_room WLD VNUM | Source messages |
| --- | --- | --- | --- | --- |
| #11108 | #3000, #3001, #3002, #3003, #3004 | #11108 | #11121 | Sells drinks; reports shop number #69 in its bug string. |
| #11109 | #364, #2812, #2813 | #11109 | #11124 | Sells food; reports shop number #68 in its bug string. |

The eight product VNUMs resolve to current object prototypes: #3000–#3004, #2812, and #2813 are in full-area Verz1, while #364 is in full-area Heavens. They are water, beer, ale, firebreather, local liquor, an iron ration, soup, and ribs. The [full AREA list](../../../areas/AREA#L2) includes Heavens, Verz1, and Player Castles.

No current areas MOB source defines keeper mobiles #11108 or #11109, and no current WLD source defines shop rooms #11121 or #11124. The two matching #11108/#11109 records in [Player Castles OBJ](../../../areas/obj/player_castles.obj#L305) are bulletin-board objects, not keeper mobiles. Thus the shop keeper, room, and object namespaces cannot be inferred from matching numbers.

## Loader path and unresolved keeper risk

The shop loader's [SHOP_FILE](../../../src/world/db.h#L22) is areas/world.shp, which is absent from this checkout; there is no areas/shp/AREA manifest either. In full mode, [open_shop_stream](../../../src/economy/shop.c#L3265) tries that aggregate and then globs every areas/shp/*.shp file, excluding end.shp and appending that terminator last. That fallback includes realms_master.shp even though its stem is absent from areas/AREA. Mini mode uses areas_mini/world.shp instead. This is a conditional source-level input path, not proof that a boot occurred or these records were accepted.

For each record, the parser resolves the keeper field with real_mobile and keeps the in_room number as written; see [shop parsing](../../../src/economy/shop.c#L3489). In the current areas source set the two keeper MOB VNUMs are absent, and [real_mobile](../../../src/world/db.c#L8587) returns -1 when its lookup fails. Later [assign_the_shopkeepers](../../../src/economy/shop.c#L3602) indexes mob_index by that keeper value without a local missing-keeper guard. If full-mode fallback includes these records without matching external MOB data, this is a potential invalid-index path. This static audit did not execute the loader, and does not claim a boot failure or any observed runtime result.

## Cross-source typed references

Five other source ZON O commands use OBJ #11108 or #11109: one #11109 row in [Dark Guildhall](../../../areas/zon/dark.zon#L10), one #11109 row in [Genocide Guildhall](../../../areas/zon/genocide_gh.zon#L10), two #11108 rows in [Circle of Yggdrasil](../../../areas/zon/rf_gh.zon#L11) and line 16, and one #11108 row in [Dark Sun Guild Hall](../../../areas/zon/sunhall.zon#L10). The exact totals are three O rows for #11108 and two for #11109. Their comments say the objects are absent from those local OBJ files. The current Player Castles OBJ prototypes with those numbers are boards, and Player Castles is in areas/AREA. These are typed OBJ reset references, not evidence for the missing SHP keeper MOBs. The source zones' reset rows do not establish that their resets run.

The SHP header keys #11108/#11109 are not duplicated by another current SHP header. Cross-type matches in Player Castles OBJ and the ZON resets are not duplicate SHP records.

## Compiled code references

The source retains ship and “Realms Master” code around these numbers, but the evidence has different status. The old [Realms Master room-binding block](../../../src/specs/specs.assign.c#L2460) is inside #if 0, so its proposed room callbacks are disabled. The current [navagator helper list](../../../src/specs/specs.minizones.c#L430) includes #11108 and other 111xx mobile IDs, and has a branch for mobile #11101; the function is assigned elsewhere to mobiles #5739 and #90590. A [realms_master_shout function](../../../src/specs/specs.mobile.c#L104) is declared and defined, but a source-wide assignment scan found no current binding to it. These references are code/history leads, not proof that a same-stem mob or room source exists.

The #11121 shop location also appears in the disabled room-binding block, while #11124 has no current WLD definition in the areas tree. The shared real_room0 helper would return index 0 for an absent room, as described in [db.c](../../../src/world/db.c#L8401). Preserve this code/data mismatch for the code and world-data owners to resolve; do not count disabled bindings as active behavior.

## History, disposition, and limits

The followed SHP path reaches the 2009 area-tree move and a later “stuff” commit; no explicit predecessor, successor, or retirement decision was found. The two old bug strings referring to shops #69/#68 do not match the SHP record headers or keeper fields and may be copied text. Their original context is unknown.

Keep realms_master.shp RETAIN IN PLACE. Before any separate activation, move, rename, merge, or archive proposal, identify the intended keeper MOBs and shop rooms, determine whether full-mode fallback is expected to include this file, guard or resolve missing-keeper handling, reconcile the stale bug strings, and check editor/deployment provenance and persisted IDs. See the [21 builder questions](REALMS_MASTER_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-524-realms_master-shop-records-without-a-room-map), [identity/history note](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-524-realms_master-shop-records-and-typed-vnum-collisions), and [retention note](ATTIC_MIGRATION_PLAN.md#checkpoint-524-realms_master-retain-shop-file-pending-loader-and-keeper-closure).

## Checkpoint 928: shop, keeper, room, and board numbers remain typed

The two SHP header keys and keeper values are #11108/#11109, and the two `in_room` values are #11121/#11124. All four numbers also identify active Player Castles OBJ bulletin boards ([#11108](../../../areas/obj/player_castles.obj#L305), [#11109](../../../areas/obj/player_castles.obj#L313), [#11121](../../../areas/obj/player_castles.obj#L409), [#11124](../../../areas/obj/player_castles.obj#L433)). The OBJ matches do not provide keeper MOB prototypes or room WLD definitions: current areas sources still have no MOB #11108/#11109 and no WLD rooms #11121/#11124. The shop header, keeper, room, and board-object references remain separate typed roles.

The five other source ZON O commands remain object references to Player Castles boards: #11108 appears three times in `sunhall.zon` and `rf_gh.zon`, while #11109 appears once each in `dark.zon` and `genocide_gh.zon` ([example #11108 row](../../../areas/zon/rf_gh.zon#L11), [#11109 row](../../../areas/zon/dark.zon#L10)). They do not supply either shop's keeper or room. The current exact-value scan found no WLD targets or definitions, QST/story references, other SHP records, or other areas consumers for #11108, #11109, #11121, or #11124.

The source-code scan classifies #11108 in [navagator's helper list](../../../src/specs/specs.minizones.c#L432) and the unassigned [realms_master_shout helper list](../../../src/specs/specs.mobile.c#L106) as a mobile VNUM. `navagator` is assigned to mobile #5739; its proposed #90590 assignment is commented out, and no current MOB #11108 resolves the helper value. The only code references typing #11108 and #11121 as rooms are inside the `#if 0` Realms Master block in [specs.assign.c](../../../src/specs/specs.assign.c#L2461); they are disabled. This adds no active room binding and does not change the missing-keeper result: `real_mobile` can still return -1, with the previously documented unguarded `assign_the_shopkeepers` indexing risk if the conditional full-mode fallback includes the file.

These cross-type matches add no WLD edge, SHP successor, alias, or replacement finding. Keep `realms_master.shp` RETAIN IN PLACE while the shop, code, loader, deployment, and editor owners resolve the actual keeper and room identities and the conditional loader path. This checkpoint adds [RM-21](REALMS_MASTER_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions); no runtime behavior is inferred.
