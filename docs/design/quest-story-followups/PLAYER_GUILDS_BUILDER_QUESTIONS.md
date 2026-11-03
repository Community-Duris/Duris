# player_guilds builder questions and recommended resolutions

Checkpoint 521, source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74.
Question 21 and the expanded resolution note for question 7 were added at
Checkpoint 925. These questions concern the unlisted QST/SHP source group
and its cross-area references. They do not authorize loading, deleting,
merging, moving, or archiving shop records.

1. **Who owns the player_guilds shop catalog and its build/import process?**
   Recommended resolution: identify the source owner and determine whether
   this fragment is maintained independently, generated, or retained for a
   deployment step.
2. **Should this QST/SHP-only source group appear in areas/AREA or
   areas/qst/AREA?** Recommended resolution: document the intended manifest
   boundaries; do not infer zone membership from the file stem.
3. **Was the nine-byte #35320/S QST intentionally retained as a no-contract
   header?** Recommended resolution: ask whether it should remain alongside
   Anan’s mobile prototype; it contains no quest contract now.
4. **Should shop #35300 be defined in player_guilds.shp, ddguild.shp, both,
   or neither?** Recommended resolution: resolve the two source records for
   the same keeper #35300 and room #35303; one lists only #35301 while the
   other lists five products and different shop flags.
5. **Does the #35300 shopkeeper/room belong to the unlisted Asunwa package?**
   Recommended resolution: reconcile it with Asunwa’s WLD/MOB/OBJ/ZON and
   separate #35320 Anan shop before treating the record as an Asunwa service.
6. **Are nine shops with no current keeper prototype obsolete, or are their
   mobile files missing?** Recommended resolution: locate the intended keeper
   definitions for #34301, #35000, #35050, #35101, #35199, #35554, #35703,
   #35902, and #35952 before any loader change.
7. **Where should the 21 stock rows with 13 undefined object VNUMs resolve?**
   Recommended resolution: recover the intended object prototypes or remove
   rows only after their item identity and consumers are confirmed. Current
   ZON sources also request #3051 twice, #6089 in 33 resets across 31 zone
   files, and #8331 once despite the missing object prototypes; confirm those
   consumers before changing the shared item identities. #5541 appears in
   code as a room target and in an inactive mobile-callback block, which does
   not define the missing stock object.
8. **Are the six shop locations without current WLD definitions retired or
   missing?** Recommended resolution: resolve #35008, #35056, #35105, #35704,
   #35903, and #35957 against map/editor history before moving any record.
9. **Is shop #34301 meant to list #34301 as an object twice?** Recommended
   resolution: verify whether the duplicated product number is a stock error
   or whether the intended object prototype is missing.
10. **Should shop #34202 at the active Drst room sell Kimordril object
    #95547?** Recommended resolution: ask Drst, Kimordril, and catalog owners
    to confirm the cross-area stock row and the intended catalog source.
11. **Are all nine Kimordril product rows across eight shops intentional?**
    Recommended resolution: preserve the exact rows until Kimordril and the
    Player Guilds owner confirm their source-only use and fallback status.
12. **Should four shops sell Tundra iron rations #13723?** Recommended
    resolution: confirm the four source-only rows with Tundra and catalog
    owners; do not count them as active sales from their presence here.
13. **Is Realm object #14112 intended stock for shop #35050?** Recommended
    resolution: verify the source-only row with the Realm owner and confirm
    which keeper/location the shop was meant to use.
14. **Which #34405 object identity should shop #34402 stock?** Recommended
    resolution: ask Longhollow and Temple2 owners to distinguish the active
    holy-symbol token from the unlisted black cultist hood.
15. **Is room #35552 a valid shop location for missing keeper #35554?**
    Recommended resolution: confirm the Lava Caves owner’s intent and
    separately resolve the keeper prototype; the room field is not a WLD exit.
16. **Are the active Newhaven locations for shops #35199 and #35252
    intentional?** Recommended resolution: resolve their keepers and missing
    product #35250 before considering either record operating stock.
17. **Should areas/world.shp be generated or restored for this checkout?**
    Recommended resolution: confirm the deployment build path; the fallback
    glob parses all areas/shp fragments but does not qualify their records.
18. **Could the fallback select both player_guilds.shp and ddguild.shp?**
    Recommended resolution: document whether both source records are
    intentionally aggregated, especially the shared Asunwa keeper/location.
19. **Do external scripts, editor tools, imports, or saved shopkeeper state
    depend on these keeper, room, or product VNUMs?** Recommended resolution:
    ask the respective owners to inspect deployed/configured data; a static
    source scan cannot prove that state.
20. **What disposition should the shop-catalog owner authorize after these
    mappings are resolved?** Recommended resolution: keep RETAIN IN PLACE and
    treat any later shop activation, repair, consolidation, move, or archive
    as a separate owner decision.
21. **Are shops #34202, #34402, #35252, and #35300 meant to use the mobiles
    that currently have those same VNUMs?** Recommended resolution: reconcile
    the Drst spider, the competing Longhollow/Temple2 #34402 prototypes, the
    Newhaven wraith, and the unlisted Asunwa jaguar-man with the declared shop
    rooms. Current ZON reset rows place those mobiles elsewhere, and no
    matching keeper reset targets its Player Guilds `in_room`; confirm the
    intended keeper identity, room placement, and aggregate source precedence
    before treating these records as staffed shops.
