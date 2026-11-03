# `tharnadia1` shop-source builder questions

These questions distinguish the old shop-only source from the current
Tharnadia shop catalog and the separately inventoried `tharold` and
`twinkeep` files. Recommendations preserve the source until its owners resolve
membership and consumer behavior. See
[THARNADIA1_AUDIT_REVIEW.md](THARNADIA1_AUDIT_REVIEW.md).

1. **Source owner:** Who owns `areas/shp/tharnadia1.shp` and its editor/export
   path? **Recommendation:** retain the file until its source and tool owner
   are named.
2. **Manifest:** Why is `tharnadia1` absent from `areas/AREA` while active
   `tharnadia` has its own `tharnadia.shp`? **Recommendation:** preserve the
   current active membership and require history-backed confirmation before
   changing either list.
3. **Identity:** Is `tharnadia1` an earlier shop catalog, a reusable
   template, or a separate source identity? **Recommendation:** treat the
   sixteen remapped records as related-source evidence, not retirement or
   deletion authority.
4. **Remapping:** Were shops #6000–#6117 intentionally remapped to active
   Tharnadia #132500–#132617? **Recommendation:** keep both records and their
   distinct keeper/room VNUMs until the map and shop owners confirm lineage.
5. **Shop #6085:** Why does active #132585 add object #102 to the older stock
   list? **Recommendation:** preserve the current active stock and candidate
   stock separately until the editor history explains the change.
6. **Shop #6117:** Are active #132617's added products #400223/#400224/#55555
   intentional updates to the source record? **Recommendation:** leave the
   active and candidate inventories unchanged and document the intended
   owner.
7. **Tharold copies:** How should the twelve same-number records in
   `tharold.shp` be related to `tharnadia1`? **Recommendation:** preserve
   every record independently; matching keeper/room values do not make the
   differing full records interchangeable.
8. **Twinkeep copies:** Why do candidate #6500/#6519 also occur in
   `twinkeep.shp`? **Recommendation:** keep those two sources separate and
   confirm the keeper, room, and stock owners before any combined-load plan.
9. **Fallback catalog:** Which exact files should load if `areas/world.shp`
   cannot be opened? **Recommendation:** have the shop/build owner define and
   verify the fallback manifest before relying on the all-file glob.
10. **Shop lookup:** Could duplicated keeper-and-room records from
    `tharnadia1` and `tharold` reach `singleton_shop_id()` together?
    **Recommendation:** establish the intended explicit binding and aggregate
    file before changing a source record.
11. **Keepers:** Who owns the fourteen keeper VNUMs without active MOB
    definitions, and are four active matches (#6000–#6002/#6500) intended?
    **Recommendation:** preserve exact typed references and resolve the
    current prototype owner for each shop.
12. **Nomads:** Should shops #6025/#6066 continue to name Nomads rooms #6242/
    #6243 while their keeper mobiles are not active? **Recommendation:**
    confirm both shop bindings with the Nomads and shop owners.
13. **Lizard stock:** Are #6007–#6009 in shop #6000 intended as Lizard's
    active objects? **Recommendation:** retain the source-only product
    dependency on this row and do not call it active shop stock.
14. **Player Castles stock:** Are #11010/#11011 in #6070 intended imports,
    given that active Tharnadia #132570 also lists them?
    **Recommendation:** preserve the exact source-only import and ask both
    area owners before changing it.
15. **Mist room #6358:** Is #6113's `in_room` field intended to refer to Mist
    room #6358? **Recommendation:** keep the room field distinct from product
    VNUMs; do not create an OBJ dependency for the number.
16. **Unresolved stock:** What is the intended status of the 73 product VNUMs
    with no current OBJ definition? **Recommendation:** preserve the authored
    stock list until each missing item is classified as retired, externally
    provided, or a data error.
17. **Shop #6500:** Is object #6500 intended stock for keeper #6500 at room
    #0, given the different current Tharnadia #132650 inventory?
    **Recommendation:** preserve both typed sources and confirm whether this
    record belongs with Shaughin or another merchant.
18. **Authored prose:** Should the merchant text remain without an attached
    zone story or QST? **Recommendation:** preserve the current messages and
    add no quest or environmental story claims from shop prose alone.
19. **Persisted state:** Are any deployed shop bindings or editor databases
    still keyed to these keeper VNUMs? **Recommendation:** check the owning
    persistence/editor workflow before proposing a rename or retirement.
20. **Disposition authority:** Who can confirm that the remapped active
    catalog supersedes any of these old records? **Recommendation:** retain
    the candidate and require a separate owner decision before co-loading,
    renumbering, moving, merging, or archiving it.
