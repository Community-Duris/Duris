# realms_master builder questions

Canonical key: component:realms_master. Checkpoints 524 and 928; coverage row 437 / CSV line 438; candidate dependency data row 86 / CSV line 87. These questions record source-backed decisions. Keep the SHP file RETAIN IN PLACE until shop, world-data, loader, and deployment owners resolve them.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| RM-01 | Who owns realms_master.shp and the missing keeper, room, editor, and deployment sources? | Record separate shop, mobile, room, editor, and deployment owners before changing loader inputs or files. |
| RM-02 | Is realms_master the intended source identity when there is no same-stem WLD, MOB, OBJ, ZON, or QST? | Preserve the filename and document its intended role as a shop-only catalog until its owner confirms a broader bundle. |
| RM-03 | Are header keys #11108 and #11109 intended durable shop identifiers? | Keep both records intact and record the durable key policy before renumbering or merging catalog entries. |
| RM-04 | Which MOB prototypes should serve as the #11108 and #11109 keepers? | Locate owner-approved keeper records or confirm the missing prototypes are intentional before allowing this file into the full loader input. |
| RM-05 | Which WLD rooms should receive shops #11108 and #11109, currently configured at #11121 and #11124? | Resolve each typed room identity and intended placement before changing either location field. |
| RM-06 | Are the listed drink and food products still the intended stock for these two shops? | Preserve the eight product VNUMs and confirm them against their Verz1 and Heavens object owners. |
| RM-07 | Do empty buy-type lists and the current messages intentionally describe selling-only stores? | Confirm the intended buy/sell behavior with the shop owner before changing trade-type records or messages. |
| RM-08 | Why do the bug strings refer to shop numbers #69 and #68 when the file headers and keepers are #11108 and #11109? | Trace the old strings and update them only after the owner identifies the intended diagnostic identity. |
| RM-09 | Should the full-mode glob fallback include realms_master.shp despite the absence of an areas/shp/AREA manifest and areas/AREA entry? | Decide and document whether fallback globbing is intentional for this file or whether an aggregate-generation rule should govern it. |
| RM-10 | Who generates areas/world.shp for deployment, and should it contain these two records? | Verify the deployment aggregate and its source list; do not infer membership from the fallback alone. |
| RM-11 | What happens when real_mobile cannot resolve keeper #11108 or #11109 and assign_the_shopkeepers later indexes the result? | Have the loader/code owner establish a guarded failure or provide matching MOB data before relying on these records. |
| RM-12 | Are #11121 and #11124 intended to be room VNUMs even though neither has a current WLD definition? | Ask the world-data owner for the intended room records and retain the raw SHP values meanwhile. |
| RM-13 | Do the five external ZON O commands for OBJ #11108/#11109 intentionally load Player Castles bulletin boards? | Preserve the typed OBJ interpretation and verify each source reset with its zone owner; do not treat them as shopkeeper references. |
| RM-14 | Are the Player Castles board objects #11108/#11109 independent of same-number SHP keys and keeper fields? | Maintain typed namespaces in every register and code review, and document the intended relation if one exists. |
| RM-15 | Is the Realms Master ship-room special block intentionally disabled under #if 0? | Keep it disabled until the code and room owners confirm its history and intended current behavior. |
| RM-16 | Are the navagator helper references to the old 111xx mobiles still intended, including #11108 which also appears as a shop/keeper key and Player Castles board OBJ? | Reconcile the helper IDs, the #11101 branch, and available MOB prototypes with the ship-code owner; preserve the mobile role even when the number matches a shop or object. |
| RM-17 | Should realms_master_shout remain declared and defined without a current assignment? | Confirm whether it is retained for external builds, obsolete source, or a missing assignment; make any separate code change only with owner agreement. |
| RM-18 | Does any exported area set provide the absent keepers, rooms, aggregate shop catalog, or alternate source history? | Trace editor and export inputs before treating the checkout inventory as the entire deployment source. |
| RM-19 | Do persisted shop state, player holdings, saved room references, or scripts depend on these numbers? | Require data-owner review before any VNUM change, move, consolidation, or archive action. |
| RM-20 | What evidence would permit activation, migration, merge, rename, or archive? | Require owner-confirmed keeper and room identities, a validated aggregate/fallback plan, guarded loader handling, and editor/deployment/saved-state signoff; retain in place meanwhile. |
| RM-21 | Do the four Player Castles board OBJ numbers #11108, #11109, #11121, and #11124 identify any intended shop, keeper, or room roles in realms_master.shp? | Keep each number type-qualified: resolve the missing MOB #11108/#11109 and WLD #11121/#11124 with the shop/world owners; do not use a matching board object, ZON O reset, or disabled room binding as a substitute. |

See the [static audit](REALMS_MASTER_AUDIT_REVIEW.md#checkpoint-928-shop-keeper-room-and-board-numbers-remain-typed), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-524-realms_master-shop-records-without-a-room-map), [identity/history note](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-524-realms_master-shop-records-and-typed-vnum-collisions), and [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-524-realms_master-retain-shop-file-pending-loader-and-keeper-closure).
