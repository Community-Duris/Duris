# Twinkeep shop-source builder questions

These questions retain unresolved builder and shop-owner choices for the two
records in [`twinkeep.shp`](../../../areas/shp/twinkeep.shp#L1). The detailed
source and typed-reference findings are in the
[Twinkeep audit](TWINKEEP_AUDIT_REVIEW.md). Recommendations preserve the
separate source records and avoid implying a live shop until the aggregate and
keeper bindings are verified.

## Explicit builder questions and recommended resolutions

| # | Source-backed question | Conservative recommendation |
| --- | --- | --- |
| 1 | Is `twinkeep` intended to identify an area, a shop family, or only a legacy file stem, given it has no WLD or ZON source? | Preserve the stem as source identity; do not assign a map or story title from the filename. |
| 2 | Should the two shop records remain together in `twinkeep.shp`? | Retain the complete two-record file until its owner confirms the intended catalog. |
| 3 | Should either shop be added to the normal `areas/AREA` aggregate? | Keep both unlisted until an area and shop owner authorize a specific source selection. |
| 4 | What was the intended status of the 2009 source and its 2013 edit? | Preserve the tracked history; neither date alone documents retirement or replacement. |
| 5 | Is roaming shop #6500 intended to belong to active Shaughin keeper #6500? ([keeper](../../../areas/mob/shaughin.mob#L1), [shop](../../../areas/shp/twinkeep.shp#L1)) | Treat the association as conditional because the shop file is not in the normal aggregate. |
| 6 | Should shop #6500 list active object #6500, the type-25 wooden trapdoor to room #6600, as stock? ([object](../../../areas/obj/shaughin.obj#L1)) | Preserve the declared product, but do not claim that it is obtainable or safe to sell. |
| 7 | Is the #6500 roaming flag and zero `in_room` value intended? | Keep the current flags together and have the shop owner confirm the intended roaming behavior. |
| 8 | Which #6500 record should a fallback shop catalog associate with the active keeper when `tharnadia1.shp` has a different record for the same keeper? | Require a valid explicit binding or a reviewed single-source decision; do not choose by file order. |
| 9 | Is Twinkeep #6500's 0.50 buy percentage intentional beside `tharnadia1` #6500's 0.30? | Retain each file's distinct percentage until an owner selects the intended record. |
| 10 | Is non-roaming shop #6519 intended at active Shaughin2 room #6656? ([shop](../../../areas/shp/twinkeep.shp#L38), [room](../../../areas/wld/shaughin2.wld#L1581)) | Keep the typed location reference distinct from any WLD edge or candidate route. |
| 11 | Which mobile, if any, should own keeper VNUM #6519 when no current MOB prototype exists? | Do not infer an active merchant; require a current typed keeper definition before activation. |
| 12 | Should Twinkeep #6519 omit product #3050, which appears in the same-key `tharnadia1` record? | Preserve the exact source lists and resolve the difference with the source owner. |
| 13 | Are active Heavens objects #203 and #204 intended stock for Twinkeep #6519? ([objects](../../../areas/obj/heavens.obj#L2378)) | Keep those numeric imports typed as current OBJ references; do not infer a quest or purchase journey. |
| 14 | What did the #6537 product reference mean, given #6537 is a WLD room but no current OBJ prototype? | Keep it unresolved as an object product; a room with that number does not satisfy the SHP reference. |
| 15 | Is active Tharnadia shop #132650 a deliberate related continuation or only a separate remapped record? | Keep it as related-source evidence; its keeper, room, and stock do not prove replacement. |
| 16 | Which source should win when `twinkeep` and `tharnadia1` provide different full records for #6500 and #6519? | Preserve both histories and ask the owner before merging, remapping, or dropping either record. |
| 17 | What should a non-mini server do when `areas/world.shp` cannot be opened and the generic loader would glob every `.shp` file? | Confirm the deployed aggregate and permitted fallback inputs; do not infer production selection from this checkout. |
| 18 | Is the existing explicit shopkeeper binding sufficient to distinguish the two roaming #6500 records if fallback aggregation includes both? | Verify the persisted binding for keeper #6500 before relying on singleton selection. |
| 19 | Are editor exports or persisted NPC/shop rows still tied to the Twinkeep path or shop keys? | Reconcile those owners before any file move, archive, or activation decision. |
| 20 | Who owns the final retention or archive decision for these shop records? | Keep **RETAIN IN PLACE** until aggregate selection, record identity, and keeper binding are resolved. |
