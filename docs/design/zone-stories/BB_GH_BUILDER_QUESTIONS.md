# BB GH: builder questions

Canonical key: component:bb_gh. Physical coverage row 365; attic candidate
row 14. The [story dossier](BB_GH.md) and
[technical audit](../quest-story-followups/BB_GH_AUDIT_REVIEW.md) preserve
the source map, portal placements, typed imports, and collisions. These
questions do not establish live room, mobile, object, or shop availability.

## Explicit builder questions and recommended resolutions

1. **What does “BB” mean, and which guild or clan owns this hall?** The ZON
   title is only “BB GH”; room prose says “the clan” without naming it.
   **Recommend** preserve the abbreviation and authored prose until the
   builder identifies the organization.
2. **Is bb_gh the canonical source stem and title?** “BB GH” is unique among
   current ZON titles, but header #1 is shared. **Recommend** retain the stem
   and raw title separately and avoid treating header #1 as an identity.
3. **Is omission from areas/AREA intentional?** The five WLD/MOB/OBJ/ZON/SHP
   files remain present without full-area membership. **Recommend** confirm
   the intended load set before changing manifests or source paths.
4. **Are bb_gh, vodgh, and venom separate maps or related versions?** Their
   raw titles are “BB GH,” “VoDsGH,” and “Venom GH.” Their WLD room numbers
   overlap across #30300–#30313, and their local mobiles overlap at
   #30300–#30302. **Recommend** keep their source identities separate until
   each owner explains the shared ranges.
5. **Which source owns rooms #30300–#30343?** The #30300–#30313 subset also
   appears in unregistered vodgh and venom.
   **Recommend** resolve numeric map ownership before registering, editing,
   or moving any source.
6. **Are the four non-entrance WLD pockets intended to remain disconnected
   by ordinary exits?** They form separate local WLD components.
   **Recommend** preserve the authored graph and review each portal pair as
   a distinct transport connection.
7. **Should #30300 connect up to ud_map2 #1126542?** ud_map2 has a reciprocal
   numeric down exit, but #30300 is shared by three candidate maps.
   **Recommend** confirm both the intended map identity and whether the
   source-only reciprocal pair should be retained.
8. **Should #30320 connect up to active tharnrifts #111255?** This is the
   only source boundary resolving to the current full-world WLD set.
   **Recommend** confirm the destination and the intended one-way or return
   arrangement before describing it as a usable route.
9. **Are the Duris3 targets #123743 and #140954 intentional?** Both target
   rooms exist only in unregistered Duris3 in this checkout.
   **Recommend** preserve these exits as source references until map
   ownership and any replacement destination are decided.
10. **Do the four reciprocal portal pairs still represent the intended
    layout?** Objects #30302–#30309 target #30307/#30313, #30308/#30323,
    #30303/#30333, and #30306/#30343 in opposite directions.
    **Recommend** check every destination against the chosen room owner
    before changing object values or resets.
11. **Are the type-25 portal values intentional?** Each object uses command
    value 7 and value[2] = -1; the code matches type 25 to CMD_ENTER.
    **Recommend** preserve the values pending a builder/runtime owner check,
    since source compatibility is not a live-route test.
12. **Is the inn callback at numeric room #30303 still intended for this
    hall?** Compiled source binds #30303 to the player rent procedure.
    **Recommend** confirm that code owner agrees on room identity before
    changing the binding or resolving the overlapping maps.
13. **Are the Player Castles objects #11057, #11107, and #11013 intended in
    these reset rooms?** The ZON contains six such imports.
    **Recommend** preserve the exact imports and locations until both area
    owners confirm them.
14. **What should object #6089 resolve to?** It appears as both reset gear
    and shop stock, but no current object prototype was found.
    **Recommend** identify its intended source or replacement before
    changing either reference.
15. **Are all imported G-reset products assigned to the intended creatures?**
    The zone imports Heavens and Verz1 gear, with #53 and #108/#109 also
    duplicated by unregistered object files. **Recommend** retain each
    command and verify owner and equipment intent in the zone editor.
16. **Which shop record owns keeper #30301?** bb_gh.shp and vodgh.shp each
    define a separate shop with that same key, while the keeper mobile number
    is also duplicated. **Recommend** confirm one shop identity and key
    ownership before generating or merging an aggregate shop file.
17. **Is the fourteen-item bb_gh stock list correct?** It includes local
    pouch #30301, active-source products, and missing #6089.
    **Recommend** preserve the listed products as authored dependencies
    until the keeper and object sources are resolved.
18. **Are room #30300 and the repeated room descriptions intentionally
    populated this way?** The prose varies between hall, elemental pockets,
    and repeated shaft text. **Recommend** retain the current text and ask
    the builder to review only the identified room blocks before any rewrite.
19. **Do the fire-lake and shaft descriptions correspond to explicit hazard
    mechanics?** The prose promises danger, but no WLD fall-chance or current
    records were found. **Recommend** review hazards independently rather
    than infer mechanics from the description.
20. **Are editor, persisted-state, or external tool references missing from
    the repository scan?** No local map JSON, QST file, or story sidecar was
    found. **Recommend** check the builder’s editor and saved-state sources
    before any archive or path decision.
