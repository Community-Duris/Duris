# Alatorin object bank: builder questions

Canonical key: component:alatorin_objects. Physical coverage row 357. The
[source dossier](ALATORIN_OBJECTS.md) and [static audit](../quest-story-followups/ALATORIN_OBJECTS_AUDIT_REVIEW.md)
document a historical OBJ-only loading stub, 408 IDs that all overlap active
Alatorin objects, a separate Nexus56 conflict, and item references in active
zone, quest, shop, story, and code sources. These recommendations do not
establish live loading or owner intent.

## Explicit builder questions and recommended resolutions

1. **What is the current purpose of alatorin_objects.obj?** The file has no
   WLD or ZON companion and is absent from the full area list.
   **Recommend** confirm whether it remains a saved-object compatibility bank
   or serves another owner-approved role. [History](../quest-story-followups/ALATORIN_OBJECTS_AUDIT_REVIEW.md#historical-loader-status-and-object-identity)
2. **Should this file remain available for persisted player objects?** The
   2010 AREA stub says existing players' Alatorin objects were causing
   crashes when their prototypes were unavailable. **Recommend** check
   saved-object records before removing or relocating the source.
3. **Which source should own the 408 overlapping IDs #83100–#83507?**
   Active alatorin.obj and this file define every ID in the bank.
   **Recommend** decide ownership per prototype before consolidating files.
4. **Which of the 204 differing prototypes should remain canonical?** Half
   the bank's records differ from active alatorin.obj in type, flags, values,
   statistics, affects, or text. **Recommend** review those as item-level
   decisions rather than treating same VNUM as proof of an identical object.
5. **Should the 168 Nexus56 overlaps remain a separate object set?**
   Nexus56.obj defines different objects at each overlapping ID and
   nexus56.zon uses 167 of those IDs. **Recommend** resolve Nexus56's status
   and numeric range alongside Alatorin's. [Candidate row 79](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79)
6. **Should Alatorin's 1,021 object reset/equipment/container commands keep
   using the active alatorin.obj variants?** They span 347 VNUMs from this
   bank. **Recommend** retain the current active source mapping until
   prototype ownership is decided.
7. **Should the other active zones keep their object references?**
   dream, dreggan_wood, ravenloft, ravenloft2, surfacekeeps, and vehicles
   contain 42 O/G/E/P records using these IDs. **Recommend** preserve those
   typed uses while resolving ownership.
8. **Which object versions should the active quest files consume?**
   alatorin.qst contains 602 R-I/G-I records across 118 IDs; cloister,
   surfacekeeps, and vehicles also reference this numeric range.
   **Recommend** reconcile quest item VNUMs with the selected prototype set.
9. **Should Alatorin and Surface Keeps shops continue selling these items?**
   Their SHP product lists contain 39 entries across 34 distinct IDs.
   **Recommend** confirm item availability and preserve typed shop references
   until the product owners are settled.
10. **Should the Alatorin, Cloister, and Surface Keeps story-sidecar item
    fields remain aligned with the current prototypes?** They contain
    structured item_vnums and completion-key references.
    **Recommend** retain those accepted-story contracts during any cleanup.
11. **What should object #83457 do through the miners_helmet callback?**
    Compiled code binds this VNUM to that object callback.
    **Recommend** preserve the prototype and callback relationship until the
    intended item owner confirms a replacement. [Binding](../../../src/specs/specs.assign.c#L1297)
12. **Should active object #83301 continue to lead to Astral Main #19721?**
    It is reset in Alatorin room #83487 and targets the Great Worm Hole.
    **Recommend** confirm the destination and return route before describing
    it as safe travel.
13. **Which #83434 destination is intended?** The active prototype targets
    #83188, Atop the Mystic's Tower; this bank's version targets #83831, an
    Ancient Lava Tube. The active Alatorin ZON resets it in room #83823.
    **Recommend** reconcile both records and the intended route together.
14. **Should the other type-25 targets and triggers remain as authored?**
    Sixteen use enter; the rest respond to sit, cardinal directions, touch,
    rub, or pray. **Recommend** confirm the action and destination for each
    portal, statue, throne, altar, rift, or tear before changing its values.
15. **Do the Alatorin target rooms with Nexus56 duplicates have a single
    intended owner?** Several portal targets share room VNUMs with different
    unlisted Nexus56 rooms. **Recommend** settle the room ranges and loader
    status before treating the target titles as runtime results.
16. **Should Taark Shanat, the Wyrmskull Throne, Breuldin, Dumathoin, and
    Shanat military artifacts remain part of the item lore?** Their text
    carries the bank's strongest narrative. **Recommend** preserve authored
    prose until an owner decides how this item set fits the world.
17. **Should the Jedd Burningbraids tome, Mithril gear, keys, and ruined
    statue objects keep their current descriptions?** These texts connect
    craft, military identity, and Alatorin history. **Recommend** keep the
    narrative evidence intact during any stat or prototype reconciliation.
18. **What should happen to the exact-copy and divergent records as source
    history changes?** 204 match active Alatorin after line-ending
    normalization and 204 differ. **Recommend** record a per-VNUM owner
    decision rather than inferring supersession from age alone.
19. **Are saved objects, editor aliases, or other external references tied
    to this old source path?** The static audit does not inspect those
    stores. **Recommend** include them in any future migration review.
20. **May this object bank be moved, merged, or removed after the questions
    are resolved?** Current item ownership, persisted IDs, and the #83434
    target conflict remain owner decisions. **Recommend** keep the source
    **RETAIN IN PLACE** until all required references and prototype versions
    have been reviewed. [Candidate row 6](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6)

This review changes no object, quest, reset, shop product, area index, or
player-facing behavior.
