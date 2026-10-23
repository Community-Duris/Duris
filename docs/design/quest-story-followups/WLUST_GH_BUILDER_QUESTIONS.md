# Wanderlust Guild Hall builder questions and recommended resolutions

Canonical key: `component:wlust_gh`; coverage data row 471 / CSV line 472;
candidate dependency row 120 / CSV line 121. These are owner questions for
the unregistered WLD/MOB/OBJ/ZON/SHP package. Recommendations preserve the
checked-in sources and do not authorize gameplay edits.

1. **Who owns the canonical Wanderlust Guild Hall map and editable source?**
   Recommendation: identify the area owner and authoritative editor/export
   before changing the manifest or any file path.
2. **Is “Wanderlust Guild Hall” the intended public title and `wlust_gh` the
   intended source identity?** Recommendation: keep the raw title and stem;
   do not infer an alias or link to another map from the word “Wanderlust.”
3. **Are all ten rooms #29100–#29109 one intended guildhall package?**
   Recommendation: preserve the current grouping and room descriptions until
   its builder confirms the package boundary.
4. **Are the paired angel and demon pillars, Judgment archway, and ominous
   presence in #29100 part of a specific faction or ritual?** Recommendation:
   retain the prose as written; do not add quest flags or mechanics based on
   the imagery.
5. **Does the Grand Entry Hall's good-versus-evil tapestry, trophies, rose
   window, and crest represent a particular guild history?** Recommendation:
   ask the story owner before expanding the lore or naming a canonical alias.
6. **Are the winding stairs and the Forge of Judgment intended to connect
   directly as authored?** Recommendation: preserve #29102 D4 and #29103 D5
   and the forge text pending map-owner confirmation.
7. **Is the Titan meant to work the forge in #29103, and are its current
   stats and reset intended?** Recommendation: keep the Titan prototype and
   its single M reset paired until the mobile owner confirms them.
8. **Is #29104 a planning room for a named conflict or story?** Recommendation:
   preserve the maps-and-notes description without inventing a quest objective.
9. **What does the Gateway to Reality in #29105 do, if anything beyond its
   prose?** Recommendation: retain its current text and D5 return; do not add
   teleport behavior without a documented target and owner.
10. **Are the names of the dead on the Gateway rim meant to link the hall to a
    particular cemetery, plane, or quest?** Recommendation: leave the
    reference open until the builder supplies that relationship.
11. **Is the Wasteland's “less grievous” wound description atmospheric or a
    gameplay promise?** Recommendation: preserve the text; it has no matching
    fall, current, or effect field in this WLD.
12. **Are the red and green mist rooms #29108 and #29109 meant to be distinct
    destinations?** Recommendation: keep both rooms and their current
    directions until their role is confirmed.
13. **Should all 18 local-number exits be interpreted as Wlust-local edges?**
    The numbers also identify active Torg and four other unregistered
    guildhall files; six source-only type-25 object declarations target
    #29106–#29109. Recommendation: preserve the source graph and object
    records, but do not treat them as Wlust routes until identity is resolved.
14. **Is D4 from #29100 intended to lead to Duris3 #154553 (“Grassy Plains”)?**
    That Duris3 source room has no checked-in return to #29100–#29109.
    Recommendation: confirm both destination and return intent before
    changing or relying on the boundary.
15. **Is the matching Void #29100 D4 to #154553 a shared design or only a
    shared numeric destination?** Recommendation: coordinate both map owners;
    do not treat the duplicate target as evidence that either guildhall
    replaces the other.
16. **How should Wlust rooms #29100–#29109 coexist with active Torg and the
    unregistered `afterlife_gh`, `scrubs_hall`, `solstice_gh`, and `void_gh`
    room records?** Recommendation: keep the sources separate until a
    namespace and loader-ownership plan covers all six identities.
17. **Are mobile VNUMs #29100 and #29101 owned by this Archangel and Titan?**
    Four other unregistered guildhall MOB files define those numbers too and
    their ZONs reset them in their own rooms. Recommendation: preserve each
    full prototype and source reset; do not resolve ownership by numeric
    match alone.
18. **Are the four separate Archangel M commands, each with current limit 4,
    intended to reset at #29100?** Recommendation: keep every command and
    parameter while the builder confirms its reset semantics.
19. **Are the Player Castles imports intended at their current rooms and
    limits?** #11053 is in #29100, #11103 in #29101, and #11013 in #29105.
    Recommendation: preserve those three external references and confirm
    placement with both hall and Player Castles owners.
20. **Are all fifteen ZON G and SHP products intended for this shop?**
    Recommendation: preserve the current list and compare its intended stock
    with the source-only hall before regenerating shop data.
21. **Which #53 OBJ identity does the shop mean?** Heavens, Dalvik, Hostel,
    and Mavulsk each define that VNUM. Recommendation: retain the reference
    and require the stock owner to identify the intended prototype.
22. **What is the intended prototype for product #6089?** No checked-in OBJ
    definition was found. Recommendation: do not invent a replacement; have
    the shop or object owner resolve the missing reference.
23. **Are the four #491–#494 Heavens gathering bags intended as Wlust stock?**
    They were added to this source's ZON and SHP in the September 2026
    gathering-bag change. Recommendation: preserve the recorded references,
    but do not use that edit as evidence that Wlust is in the active manifest.
24. **Should `wlust_gh.shp` be consumed by the full-mode fallback shop loader?**
    `areas/world.shp` is absent here; the fallback globs all `areas/shp/*.shp`,
    including this file and `void_gh.shp`, which both use keeper #29101.
    Recommendation: reconcile build inputs, the two shop owners, and a valid
    active keeper before relying on the fallback.
25. **Is #29103 intended to be both the shop room and the forge, given that
    the current manifest maps it to Torg's escape tunnel and compiled code
    binds `inn` there?** Recommendation: resolve the room identity and code
    binding with the Torg and Players Guild owners before any activation.
26. **Does Wlust need its own QST or formal story contract?** No same-stem QST
    exists; the unrelated unregistered Afterlife quest begins at giver #29101.
    Recommendation: do not copy or create a quest from the shared number.
27. **Does any editor export, deployment source, running server, or persisted
    player data identify Wlust rooms or prototypes under a different
    namespace?** Recommendation: resolve those external records before
    activation, movement, merge, renumbering, or archive.
28. **Should these five files remain outside `areas/AREA` and `areas/qst/AREA`
    while ownership is unresolved?** Recommendation: retain them in place and
    request a separate owner decision only after the questions above are
    reconciled.
