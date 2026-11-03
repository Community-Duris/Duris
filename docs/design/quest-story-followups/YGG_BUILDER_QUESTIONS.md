# Yggdrasil Guild Hall builder questions and recommended resolutions

Canonical key: component:ygg; coverage data row 472 / CSV line 473;
candidate dependency row 121 / CSV line 122. These are source-owner questions
for the unregistered WLD/MOB/OBJ/ZON/SHP package. Recommendations preserve
the checked-in sources and do not authorize gameplay edits.

1. **Who owns the canonical Yggdrasil Guild Hall map and editable source?**
   Recommendation: identify the map owner, editor project, and authoritative
   export before changing any manifest entry or file path.
2. **What is the intended zone identity and header for ygg?** Its normalized
   title and ZON header #9 match circlegh, while all thirteen room VNUMs
   #30400–#30412 sit outside the conventional #900–#999 band for header #9.
   Recommendation: retain the header, VNUMs, and both source identities until
   the owner supplies a title crosswalk and confirms the intended namespace;
   do not infer a rename, range correction, or replacement.
3. **Is “Yggdrasil Guild Hall” the intended public title for this component?**
   Recommendation: preserve the raw title and stem pending confirmation.
4. **Are all thirteen rooms one intended package despite the tree rooms
   #30400–#30409 and forest rooms #30410–#30412 forming separate components?**
   Recommendation: preserve the current grouping and graph while the map
   owner confirms the boundary.
5. **What story ties the root camp’s loom and fate imagery to the tree
   branches?** Recommendation: preserve the source text and do not add a
   quest outcome or cosmology without the story owner.
6. **Are Ratatoskr, Urd, Heidrun, and the leaf-colored forest spirits intended
   as specific Norse-myth characters or local variants?** Recommendation:
   retain their current names and descriptions; do not infer additional
   relationships.
7. **Do the faithful branch and autumn arms-and-armor room represent named
   gods, heroes, or a guild history?** Recommendation: keep the displays as
   written until the author identifies the intended references.
8. **Is #30409 intentionally called a crest while explaining that it is only
   another branch?** Recommendation: preserve the description and direction
   until the builder confirms whether the viewpoint is complete.
9. **What is the authoritative text and destination for #30407’s scrambled
   Sparkling Plane description?** Recommendation: do not repair or reinterpret
   the text from the corrupted color-coded source alone.
10. **How does the isolated forest slope and gate group relate to the
    Yggdrasil tree?** Recommendation: leave both graph components separate
    until the story and map owner confirms a connection.
11. **Should #30400 D0 to #30401 enter a Yggdrasil room or active Ard’gral?**
    Both sources define #30401. Recommendation: resolve VNUM ownership before
    treating this as a route; preserve it as an owner-ambiguous source edge.
12. **How should Yggdrasil rooms #30401–#30412 coexist with the twelve
    different active Ard’gral rooms using those VNUMs?** Recommendation:
    establish an explicit map namespace/loader plan before activation, merge,
    or renumbering.
13. **Are the 22 local-number targets intended to stay within Yggdrasil when
    these records are loaded?** Recommendation: do not rely on numeric
    resolution alone; validate every target identity and the two graph
    components with the map owner.
14. **Is #30400 D4 to SurfaceKeeps room #123522 an intentional one-way
    approach?** The checked-in SurfaceKeeps room has no return to Yggdrasil.
    Recommendation: confirm the destination and return intent before using
    or changing this route.
15. **Is #30410 D4 to unregistered Duris3 room #157535 intentional?** That
    exact room has no checked-in return to #30401–#30412.
    Recommendation: confirm both map owners and return intent before
    relying on the boundary.
16. **Do portal object #30400’s Lush Jungles description and stored
    destination #30412 refer to the same intended place?** Recommendation:
    confirm the object field semantics and destination identity before edits.
17. **Do portal object #30401’s realm-of-Light description and stored
    destination #30407 refer to the same intended place?** Recommendation:
    confirm the field semantics and destination identity before edits.
18. **Is the active Ard’gral mask of nightmares the intended competing
    owner for Yggdrasil object #30401?** Recommendation: preserve both source
    records; do not assign the portal identity by VNUM alone.
19. **Is Yggdrasil mobile #30401, Urd, meant to share a number with active
    Ard’gral’s nightmare essence?** Recommendation: preserve the full
    prototypes and resolve ownership before activating or importing either.
20. **Are the other five mobile prototypes and their stats intended to match
    the current six mobile reset identities?** Recommendation: keep each MOB
    prototype paired with its source reset until the mobile owner confirms.
21. **Are the nine M commands intentional, including the three repeated Norn
    rows and two repeated green-spirit rows?** Recommendation: retain every
    command and limit until the builder confirms reset semantics.
22. **Are all five Player Castles object imports intended at their current
    rooms and limits?** The ZON references #11055 twice, #11105 twice, and
    #11013 once. Recommendation: preserve the imports and confirm them with
    both area owners.
23. **Is the #30400 shop, with Ratatoskr as keeper at #30408, intended to sell
    all eleven listed products?** Recommendation: preserve its stock and
    keeper/room values until the shop owner confirms the catalog.
24. **Which object prototype does product #53 mean?** It has four current
    owners. What should product #6089 resolve to? No current OBJ prototype
    was found. Recommendation: do not invent a replacement; ask the stock
    owner to identify the intended items.
25. **Is the stockless #30412 shop record, assigned to the red forest spirit
    in #30410, intentional?** Recommendation: retain the record and confirm
    its purpose with the shop and mobile owners.
26. **Which shop data source should production load?** The checkout lacks
    areas/world.shp, and full-mode fallback concatenates areas/shp/*.shp.
    Recommendation: establish the authoritative shop build/export path and
    keeper VNUM ownership before relying on this fallback.
27. **How should the source-only Yggdrasil portal records coexist with
    Ard’gral’s compiled #30401–#30464 spell range and object-pool bindings?**
    Recommendation: preserve the active Ard’gral hooks and require an
    owner-reviewed namespace plan before code or area changes.
28. **Is a formal quest or authored story file intended?** No same-stem QST,
    native Q/QA contract, or story sidecar exists. Recommendation: keep the
    current absence explicit; do not invent a quest objective or write a
    canonical story sidecar without the owner.

Keep all five files **RETAIN IN PLACE** outside areas/AREA and areas/qst/AREA.
Do not activate, merge, renumber, regenerate, move, or archive until map and
editor ownership, the title crosswalk, Ard’gral VNUM/code conflicts, route
targets and returns, Player Castles imports, portals, resets, shop fallback,
deployment, runtime, and persisted identity are resolved. See the
[Yggdrasil audit](YGG_AUDIT_REVIEW.md).
