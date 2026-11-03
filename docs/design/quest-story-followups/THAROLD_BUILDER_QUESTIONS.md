# `tharold` shop-source builder questions

These questions distinguish `tharold.shp` from active Tharnadia and the
separate `tharnadia1` shop source. Recommendations keep the records intact
until their owners resolve the loader and editor relationship. See
[THAROLD_AUDIT_REVIEW.md](THAROLD_AUDIT_REVIEW.md).

1. **Source owner:** Who owns `areas/shp/tharold.shp` and its editor/export
   path? **Recommendation:** retain the file until the tool and source owner
   are identified.
2. **Manifest:** Why is `tharold` outside `areas/AREA` while `tharnadia` has
   a separate active shop file? **Recommendation:** preserve current area
   membership until the owner confirms its intended source list.
3. **Current relationship:** Were the twelve shops #6000–#6086 migrated into
   active Tharnadia's #132xxx catalog? **Recommendation:** treat the mapped
   records as related evidence; require a source-history decision before
   calling this file retired.
4. **Tharnadia1 relationship:** Why do all twelve shop numbers in
   `tharold.shp` also occur in `tharnadia1.shp` with the same keeper and room
   but different complete records? **Recommendation:** preserve each source
   until their common editor/export history is confirmed.
5. **Lizard stock:** Are active Lizard objects #6007–#6009 intentional
   products of `tharold` shop #6000? **Recommendation:** keep this source-only
   reference attached to the candidate and do not infer active commerce.
6. **Shop #6001:** What is the intended status of candidate products #835 and
   #6132, which are absent from its `tharold` copy? **Recommendation:** retain
   each file's stock list separately until the editor version is known.
7. **Shop #6002:** Is location #6130 intentionally unresolved under current
   area membership? **Recommendation:** preserve the field and confirm its
   room owner before changing it.
8. **Nomads locations:** Should #6025/#6066 remain bound to Nomads rooms
   #6242/#6243 with keeper VNUMs #6025/#6066? **Recommendation:** confirm the
   keeper prototypes and intended service with the Nomads owner.
9. **Keeper prototypes:** Who owns the nine keeper VNUMs without active MOB
   definitions? **Recommendation:** preserve their numbers and resolve each
   intended prototype source before any loader plan.
10. **Room locations:** What are the intended destinations for #6130, #6262,
    and #6267, which have no active room owner? **Recommendation:** keep these
    location fields source-only until their owner identifies them.
11. **Unresolved stock:** What is the source of the 55 product VNUMs with no
    current OBJ definition? **Recommendation:** preserve every listed item
    until it is classified as a removed object, external source, or data error.
12. **Shop #6070:** Why does `tharold` omit Player Castles products #11010/
    #11011 that appear in `tharnadia1` and active Tharnadia #132570?
    **Recommendation:** keep the different shop lists separate and confirm
    which source version owns those products.
13. **Shop #6085:** Is the shorter `tharold` inventory intended to precede
    active Tharnadia's later #102 product addition? **Recommendation:** retain
    the current records and history while the owner confirms the change.
14. **Fallback file set:** Which exact files may load when `areas/world.shp`
    cannot be opened? **Recommendation:** have the shop/build owner define the
    intended fallback catalog before relying on its glob.
15. **Duplicate lookup:** Could all twelve non-roaming `tharold` records and
    `tharnadia1` copies reach `singleton_shop_id()` together?
    **Recommendation:** confirm the selected aggregate and explicit bindings
    before changing either source.
16. **Normal build:** Should source generation continue to use `areas/AREA`
    as the sole shop-file selection list? **Recommendation:** keep generated
    selection unchanged until the build owner resolves the orphan files.
17. **Dialogue:** Should the merchant prose remain without a local zone story
    or QST? **Recommendation:** preserve the authored wording without
    inventing a quest, setting, or player-facing purpose.
18. **Source history:** Is there an owner record linking the 2009 path move,
    2013 `tharold` edit, and active Tharnadia remapping?
    **Recommendation:** require a documented lineage decision before any
    archive or merge proposal.
19. **Persisted shop state:** Are any deployed shop bindings or editor records
    keyed to these keeper VNUMs? **Recommendation:** check the owning
    persistence and editor workflows before a rename or removal.
20. **Disposition authority:** Who can authorize retirement of this source?
    **Recommendation:** retain `tharold.shp` and require a separate owner
    decision before co-loading, merging, moving, renumbering, or archiving it.
