# VoDsGH builder questions and recommended resolutions

Canonical key: `component:vodgh`; coverage data row 468 / CSV line 469 and
candidate dependency data row 117 / CSV line 118. These questions document
owner decisions needed before activation, migration, or archive. The
recommendations preserve the current source records and do not authorize
gameplay changes.

1. **Who owns the canonical VoDsGH map, guild identity, and editable source?**
   Recommendation: name the map owner and authoritative editor/export before
   changing manifests or source paths.
2. **Is “VoDsGH” the intended public title for the Veil of Darkness source?**
   Recommendation: preserve the ZON title until its owner confirms any alias
   or display-name change.
3. **Are zone header #303 and top VNUM #30313 the intended range metadata?**
   Recommendation: keep the existing header and range until the reservation
   and generator meaning are documented.
4. **Are all 14 room records meant to belong to one VoDsGH source package?**
   The first ten describe a guildhall; the last four describe a jungle valley.
   Recommendation: retain both clusters and ask the owner to confirm their
   shared source identity.
5. **Should the #30300–#30309 cave and #30310–#30313 jungle clusters remain
   disconnected in the local WLD graph?** Recommendation: preserve the current
   graph; request an explicit connection design if they should join.
6. **Does the text about the dragon’s slaying and new darkness refer to a
   specific authored event or quest?** Recommendation: keep it as room prose
   until the owner supplies a matching story or quest source.
7. **Which source map owns each room number #30300–#30313?** All 14 numbers
   occur in BB GH, and #30300–#30309 also occur in Venom. Recommendation:
   preserve all three map records separately and resolve ownership before
   loading more than one source.
8. **Should the `ud_map2` #1126542 down exit target BB GH, Venom, or VoDsGH
   room #30300?** Recommendation: leave the source-only target unresolved
   until an owner names its intended map.
9. **Is VoDsGH #30300 up to `ud_map2` #1123336 intentionally one-way?**
   Recommendation: preserve the authored edge and ask for an explicit return
   endpoint if one is required.
10. **Is VoDsGH #30310 up to active Magma #142034 intentionally one-way?**
    Recommendation: retain this already-recorded source-only route and obtain
    an owner-approved return design before changing either map.
11. **Are the two up exits at #30300 and #30310 intended as independent
    boundaries?** Recommendation: keep their separate targets and do not infer
    that either boundary links the two local clusters.
12. **Is full ten-room forward and reverse reach from #30300 intentional?**
    Recommendation: preserve the strongly connected cave graph and request a
    builder decision before removing or reversing any exits.
13. **Should #30307–#30309 remain connected to portal room #30305 by reciprocal
    exits?** Recommendation: treat these paired directions as authored
    topology, not an automatic defect.
14. **Are the room descriptions’ darkness, rift, torture, treasure, and jungle
    details descriptive only?** Recommendation: do not infer hazards, reward
    behavior, or quest mechanics without an authored contract.
15. **How should BB GH and VoDsGH retain distinct identities across shared
    room numbers #30300–#30313?** Recommendation: do not merge or renumber the
    maps without approval from both owners.
16. **How should Venom, BB GH, and VoDsGH resolve shared rooms #30300–#30309?**
    Recommendation: keep all ten full WLD records distinct and require an
    explicit loader/range plan before activation.
17. **Which map owns the mobile prototypes #30300–#30302 referenced by the
    VoDsGH resets?** Recommendation: preserve each source’s different mobile
    record and resolve loader identity before changing reset data.
18. **Are VoDsGH golem records #30300/#30302 and #30303/#30304 intentionally
    separate variants?** Recommendation: keep their separate stats and flags;
    do not consolidate by shared name or description.
19. **Are VoDsGH portal objects #30300/#30301 meant to be a paired portal
    between #30309 and #30313?** Recommendation: preserve both target values
    and confirm which map instance each target should resolve to.
20. **Should BB GH objects with the same #30300/#30301 VNUMs remain separate
    from the VoDsGH portals?** Recommendation: preserve both OBJ records and
    require explicit object ownership before combining them.
21. **Does Sunhall object #30202’s type-25 target #30302 refer to VoDsGH, BB
    GH, or Venom?** Recommendation: keep the external portal target ambiguous
    until its owner identifies the intended map.
22. **Are the five Player Castles O resets (#11061, #11111, and #11013) meant
    at the current VoDsGH rooms?** Recommendation: preserve their exact source
    imports and placements until both object and map owners confirm them.
23. **Are all nine M reset commands intended, including repeated golems at
    #30300 and #30310?** Recommendation: preserve the duplicate commands and
    *max* fields until the zone owner confirms reset behavior.
24. **Is mobile #30301 the intended keeper for the shop at room #30306?**
    Recommendation: keep the shop source separate from BB GH’s same-key shop
    and resolve the shared mobile identity before activation.
25. **Who supplies the shop object #6089 referenced in the shop inventory and
    the hermit’s G list?** No checked-in OBJ source defines it. Recommendation:
    keep the reference visible and require its authoritative prototype source
    before closing dependency review.
26. **Which source owns duplicate shop/inventory objects such as #108, #109,
    and #53?** Recommendation: retain the numeric references and confirm the
    actual loaded object prototypes before adjusting stock.
27. **Which room identity should receive the compiled inn binding at #30303?**
    Recommendation: preserve the code reference and resolve its owner among
    the three room records before changing it.
28. **Were QST and story-sidecar files intentionally omitted?** Recommendation:
    confirm whether an external quest/editor source exists before authoring or
    linking new story content.
29. **Is `vodgh` intentionally absent from `areas/AREA` and `areas/qst/AREA`?**
    Recommendation: keep it outside both manifests until its owner, shared
    VNUMs, and dependency closure are resolved.
30. **Do editor exports, deployment copies, runtime state, or persisted player
    records contain another VoDsGH identity?** Recommendation: check those
    sources before any move, renumber, activation, regeneration, or archive.

These are owner decisions, not gameplay fixes. Keep VoDsGH **RETAIN IN PLACE**
until map ownership, the shared #303 range, boundary routes, resets, portals,
shop stock, editor authority, runtime, and persisted identity are resolved.
See the [audit review](VODGH_AUDIT_REVIEW.md).
