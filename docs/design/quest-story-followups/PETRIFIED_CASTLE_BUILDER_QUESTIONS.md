# Petrified Castle builder questions and recommended resolutions

Checkpoint 519, source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74.
These questions concern the unlisted six-file castle package. They do not
authorize activation, quest/shop changes, renumbering, moving, or archiving.

1. **Who owns the WLD, MOB, OBJ, ZON, QST, and SHP files and their editor
   project?** Recommended resolution: identify the builder and preserve the
   source project, exports, and last-known deployment before editing.
2. **Is “Tower of Testing” an intentional alias, a testing-only label, or an
   unrelated earlier title for the castle in the WLD/QST?** Recommended
   resolution: ask the author to reconcile the unique ZON title with the
   castle and kingdom prose.
3. **Was this package intended to be playable, a test zone, or an
   unshipped story draft?** Recommended resolution: locate an owner-approved
   release or editor record; current AREA manifests do not list it.
4. **Should zone #938 reset through the current full-world import path?**
   Recommended resolution: confirm the reset and room range #93700–#93886 in
   the source manifest/editor before adding it to any loader list.
5. **Was the QST intended to load from a separate aggregate?** Recommended
   resolution: determine how the file reached areas/world.qst in any
   deployment; the current loader does not glob qst/*.qst and qst/AREA does
   not list this package.
6. **Are the six plain one-way directions missing reverse links or intentional
   shortcuts?** Recommended resolution: verify #93702 down, #93732 west,
   #93761 east, #93798 east, #93809 west, and #93860 south against the
   original editor map.
7. **What is the intended destination for #93724 south → -1?** Recommended
   resolution: check whether this is a deliberate boundary/placeholder,
   malformed legacy exit, or missing destination before altering it.
8. **Should Guards lounge #93723 be isolated from the 186-room castle graph?**
   Recommended resolution: ask whether the no-exit room was intentionally
   retained as unused scenery; it has no ZON reset.
9. **Can the 70 rooms titled “Unnamed” be matched to intended names in the
   editor?** Recommended resolution: restore labels only from authored
   source or builder confirmation, not from neighboring prose guesses.
10. **Is the 60% WLD fall field in room #93702 intended to remain?**
    Recommended resolution: confirm the hazard and target behavior
    independently of the ZON F follower resets.
11. **Are all 40 D reset states intentional for their WLD door directions?**
    Recommended resolution: review the state bits and door descriptions in
    the editor; do not compare the numeric encodings as if they were the
    same field.
12. **Why is mobile #93718 unused by the ZON and missing a short description?**
    Recommended resolution: identify whether it is reserved, abandoned, or
    incomplete before adding a reset or removing the prototype.
13. **Is the regal sculpture #93705 the king who speaks in the QST?**
    Recommended resolution: confirm the quester’s identity and whether the
    statue is meant to communicate despite being described as stone.
14. **Should the quest ask for the Medusa head instead of keys #93730?**
    Recommended resolution: compare the QST narrative with its R I 93730
    and G I 93731 clauses and obtain an owner-approved item exchange.
15. **Where should glowing keys #93730 come from?** Recommended resolution:
    locate the intended reset, hidden placement, or giver; the object has no
    local ZON reset or shop stock entry.
16. **Is Medusa supposed to carry a head that the quest also gives as its
    reward?** Recommended resolution: verify the ZON G command at room #93861
    and the quest reward together, including whether the item can be obtained
    twice.
17. **Should the single shop #93704 be part of the loaded shop catalog?**
    Recommended resolution: confirm the keeper, seven products, and aggregate
    import path; the source loader’s fallback glob alone does not prove
    deployment behavior.
18. **Is object #93733 intentionally unreferenced?** Recommended resolution:
    check it against builder notes and editor placements before treating it
    as dead content.
19. **Do external imports, scripts, teleport items, editor saves, or
    persistence records refer to rooms #93700–#93886 or the package name?**
    Recommended resolution: ask the respective system owners to check their
    deployed/configured data; the current static active-source scan found no
    typed inbound reference or direct C/C++ stem literal.
20. **What disposition should be authorized after those answers are
    documented?** Recommended resolution: keep RETAIN IN PLACE for now and
    treat any later activation, repair, merge, renumbering, move, or archive
    as a separate owner decision.
