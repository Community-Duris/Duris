# Yggdrasil Guild Hall Builder Questions

Checkpoint 464 source-backed questions for the unregistered `circlegh` component. Recommendations retain the existing files while the room map, portal destinations, Player Castles imports, shop, and shared `circle_op` blocks are reviewed by their owners.

| ID | Question | Recommended resolution |
| --- | --- | --- |
| CG-01 | Who owns the `circlegh` WLD, MOB, OBJ, ZON, SHP, and external editor data? | Identify the map, shop, source-control, and tooling owners before any file or path change. |
| CG-02 | Is “Yggdrasil Guild Hall” the canonical title for this component? | Confirm the official display name and any title alias with the owner before recording one. |
| CG-03 | How does `circlegh` relate to `circle_op`, whose #45410–#45412 rooms and #45410–#45413/#45410–#45411 prototypes are exact copies? | Require an owner-approved room/mobile/object map; do not infer which source is canonical or replaceable. |
| CG-04 | Are `ygg` (#30400–#30412) and `rf_gh` (#28900–#28912) related to this guildhall? | Record only documented lineage and retain each source’s current numeric identity until then. |
| CG-05 | Should the ZON and WLD room field stay 9 for rooms in the #454 VNUM/100 band? | Confirm the legacy numbering and editor convention before changing the raw fields. |
| CG-06 | Is ZON last VNUM #45417 correct when mobile #45418 is defined and reset in #45417? | Confirm how the max-VNUM field is intended to cover mobiles and preserve it until the builder responds. |
| CG-07 | Are the missing WLD room records #45413 and #45414 intentional gaps? | Confirm the intended room map and range with the area owner without creating placeholder rooms. |
| CG-08 | Is the D4/up destination -1 from room #45400 intentional? | Ask the builder to identify its intended boundary or special meaning; retain the raw destination meanwhile. |
| CG-09 | Should the #45400–#45409, #45410–#45412, and #45415–#45417 map sections remain three separate components? | Document each section’s role and connections before adding or redirecting any exit. |
| CG-10 | Is #45410 D4/up to Duris3 #154462 “The Ocean” intentional? | Confirm both source and destination owners and preserve the one-way edge until the boundary is decided. |
| CG-11 | What return route should exist from Duris3 #154462, if any? | Document a reciprocal route only if both map owners approve it. |
| CG-12 | Is #45417 D4/up to active Tharnrifts #110072 “The Sea of Twin Worlds” intentional? | Confirm that existing source-only approach with both map owners and retain it in the global inventory. |
| CG-13 | Should Tharnrifts #110072 have a return route to #45417? | Record an owner-approved direction and destination; do not infer a return from the existing outbound edge. |
| CG-14 | Are the shared #45410–#45412 WLD blocks independently editable in both `circlegh` and `circle_op`? | Preserve both exact blocks until a canonical text source or edit workflow is documented. |
| CG-15 | Are the shared golem mobiles #45410–#45413 intended to remain in both MOB files? | Confirm prototype ownership and reset behavior before changing either copy. |
| CG-16 | Are objects #45410 and #45411 meant to be shared by both sources? | Confirm their area ownership and every ZON placement before any deduplication. |
| CG-17 | Do the three Norns and Heidrun belong in camp room #45400? | Confirm their current placements and intended narrative function with the builder. |
| CG-18 | Should squirrel mobile #45400 reset in giant squirrel’s nest #45408? | Preserve the ZON placement while its relationship to the shop room is resolved. |
| CG-19 | Should the SHP keeper and shop room both remain #45400 when the ZON places the keeper in #45408? | Resolve the keeper, room, and non-roaming setting together before treating this as a working shop. |
| CG-20 | Are the ten SHP product VNUMs #364, #377, #398, #3050, #412, #347, #108, #109, #111, and #53 still intended? | Confirm the inventory with the shop owner; preserve the existing Heavens and Verz1 object references meanwhile. |
| CG-21 | Are the ten G commands meant to equip the squirrel with the same objects listed as shop products? | Confirm whether the duplicate inventory is intentional equipment, stock setup, or legacy reset data. |
| CG-22 | Should the non-mini SHP fallback include `circlegh.shp` while its keeper and room are outside the active full list? | Have the shop-loader and area owners define an explicit inclusion and identity contract before any loader change. |
| CG-23 | Should the seven Player Castles imports #11052, #11013, and #11102 remain in their current rooms? | Preserve the exact imports until Player Castles and Circle owners confirm each object’s placement. |
| CG-24 | Is object #45410’s destination #45407 intended, and is its placement in #45412 correct? | Confirm the root portal’s destination and source room as a pair. |
| CG-25 | Does object #45411’s destination #45412 match its “Goodie Cont” description? | Reconcile the target identity and prose with the object owner before editing either value. |
| CG-26 | Does object #45415 correctly send travelers to #45407 from its reset location in #45415? | Confirm both the placement and destination with the map builder; retain current records until then. |
| CG-27 | Does object #45416’s “Ice Crag” description match its stored target #45415 in this WLD? | Confirm the intended Ice Crag route and its room identity before correcting prose or destination. |
| CG-28 | Were all thirteen mobile resets and their quantities intentionally set as authored? | Confirm the Norn, Heidrun, squirrel, golem, and cold-climate guardian populations with the builder. |
| CG-29 | Is the story complete without a same-stem QST or story sidecar? | Have the area owner document whether the Norse destiny story, shop, portals, and tree guardians need another contract. |
| CG-30 | Which script, editor, dynamic route, and persisted room/mobile/object references depend on `circlegh`? | Obtain a complete reference map and owner approval before any migration, archive, or loader-membership proposal. |
