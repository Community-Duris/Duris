# Void Guildhall builder questions and recommended resolutions

Canonical key: `component:void_gh`; coverage data row 469 / CSV line 470 and
candidate dependency data row 118 / CSV line 119. These source-backed
questions identify owner decisions needed before activation, a source move,
or archive. Recommendations preserve the current data; they do not authorize
gameplay edits.

1. **Who owns the canonical Void guildhall map and its editable source?**
   Recommendation: name the guild/map owner and authoritative editor/export
   before changing files or manifests.
2. **Is “Main Guildhall for The Void” the intended public title for
   `void_gh`?** Recommendation: preserve the ZON title until an owner confirms
   any alias or display-name change.
3. **Why does this map use zone header #295 while its room VNUMs are
   #29100–#29118 and #295 is shared with Genocide Hall and Legion?**
   Recommendation: retain all source numbers until the range owner confirms
   the generator and collision policy.
4. **Are all nineteen room records intended to form one Void source package?**
   The first ten, lower chasm, elven outpost, and deeper-cavern groups are
   separate WLD components. Recommendation: keep them together pending an
   owner-authored map boundary plan.
5. **Are both surface entrances, #29100 and #29116, intentional?**
   Recommendation: retain both declarations and ask whether they represent
   distinct entrances or alternate copies of one entrance.
6. **Should #29100 D4 lead to Duris3 #154553, “Grassy Plains”?**
   Recommendation: preserve the source-only exit and get an owner-approved
   destination and return plan before editing either WLD.
7. **Should #29113 D4 and #29116 D4 both lead to Duris3 #163943, “Rolling
   Hills”?** Recommendation: preserve both source records and confirm whether
   the shared destination is intentional.
8. **Are both Duris3 target rooms meant to return to a Void entrance?**
   The exact targets have no direction back to #29100–#29118 in checked-in WLD
   data. Recommendation: do not invent a return exit; ask the map owners to
   specify one if the route is intended to be traversable both ways.
9. **Are the four disconnected WLD components intentional?**
   Recommendation: retain their local strongly connected graphs and decide
   separately whether item portals should join them.
10. **Should the four type-25 objects provide the only links between these
    WLD components?** Recommendation: confirm the object destinations and
    interaction behavior before inferring player reachability from VNUMs.
11. **Should object #29100 at #29100 target chasm room #29112?** Its text
    mentions transport to “realms of the evil doers,” while #29112 is a light
    portal scene. Recommendation: preserve `value[0]` and resolve the intended
    narrative destination with the builder.
12. **Should object #29101 at #29112 target #29108?** Recommendation: retain
    the source target and confirm whether its light-portal wording and the
    “not time yet” room text are meant to work together.
13. **Should object #29102, placed at #29115 and #29118, target #29107?**
    Room #29107 describes an incomplete portal. Recommendation: preserve the
    placements and target until its intended availability is confirmed.
14. **Should object #29103 at #29107 target outpost room #29115?** Room
    #29115 itself says it leads back to the main guildhall. Recommendation:
    confirm direction and destination before changing either portal record.
15. **Are the two “portal of light” WLD descriptions at #29107 and #29108
    meant to remain unusable?** Recommendation: treat “not time yet” as authored
    prose only unless the builder confirms an implemented lock or event.
16. **Should room #29118's portal “lead deeper” through a separate
    destination?** Its reset object #29102 instead targets #29107.
    Recommendation: preserve both prose and object reference until their
    relationship is clarified.
17. **Is the forest outpost intended as a branch or a route back to the main
    guildhall?** The WLD directions connect #29113–#29115 only, and the
    cross-link is represented by an object target. Recommendation: retain the
    existing WLD graph and confirm the item route separately.
18. **Does every room in #29100–#29118 belong to the Void rather than active
    Torg's competing map identity?** All nineteen numbers are active Torg rooms
    with different records. Recommendation: require explicit owner and loader
    resolution before enabling either candidate data or a route.
19. **How should #29100–#29109 retain distinct identities across Torg,
    Afterlife GH, Scrubs Hall, Solstice GH, Wlust GH, and Void?**
    Recommendation: preserve all full room blocks and do not merge or renumber
    them without the affected map owners.
20. **Should the #29110–#29112 records remain distinct from Afterlife OP and
    active Torg?** Recommendation: keep the source identities separate and
    require a loader/range plan before route or portal changes.
21. **Which source should own mobile VNUMs #29100–#29104 used by Void's ZON?**
    They overlap several unregistered guildhall MOB files with different
    records. Recommendation: preserve each prototype and resolve ownership
    before any reset is loaded.
22. **Are Void mobiles #29102, #29104, and #29105 intentionally unused by
    `void_gh.zon`?** Recommendation: retain the prototypes and ask whether
    another authored reset source is expected.
23. **Are sixteen golem M rows intended, with twelve requests for #29100 and
    four for #29103 across the four entrance rooms?** Recommendation: preserve
    each row and max value until the zone owner confirms reset behavior.
24. **Is Drac #29101 intended to run the shop at #29103 while the WLD calls
    #29102 the inn and Drac's mobile prose describes his past as an
    innkeeper?** Recommendation: preserve the SHP/WLD/MOB records and ask the
    shop and guild owners to reconcile keeper and room identity.
25. **Which loaded rooms should receive code actions bound to raw VNUMs #29103
    and #29116?** The `inn` callback at #29103 and Torg invasion logic at
    `INVASION_ROOM_VNUM` #29116 resolve to active Torg rooms; Void describes
    #29103 as a shop and #29116 as a stairway. Recommendation: keep both code
    bindings unchanged until code and map owners agree on range-safe identity.
26. **Are object prototypes #29100–#29106 distinct from Afterlife GH and
    Solstice GH same-number objects?** Recommendation: preserve all source
    records and resolve loader ownership before changing or activating them.
27. **Are the fallback descriptions for objects #29104–#29106 meant to be
    unreachable?** Their default text warns that the description should not
    appear. Recommendation: preserve flags and examine builder intent before
    changing the fallback or extra descriptions.
28. **Is carvings object #29106 meant to repeat the room's battle text, and do
    the heart-room pillar letters H/A/L/K encode a clue or reflect a typo?**
    Recommendation: keep the current descriptions and letters pending
    builder/editor confirmation.
29. **Are Player Castles objects #11054, #11013, and #11104 meant to be reset
    in the nine current placements?** Recommendation: preserve those exact
    external imports and placements until both source owners confirm them.
30. **Who supplies object #6089 in Drac's G list and shop stock?** No checked-in
    OBJ prototype defines it. Recommendation: keep the reference visible and
    require its authoritative prototype or an explicit removal decision.
31. **Which object source owns duplicate stock VNUM #53?** It occurs in
    multiple OBJ sources, including active Heavens and unlisted sources.
    Recommendation: preserve the numeric reference until the intended loaded
    prototype is confirmed.
32. **Were Heavens objects #491–#494 intentionally added to both the Void
    ZON and shop in September 2026?** Recommendation: keep these recent
    references and ask their maintainer to confirm that they belong in this
    shop.
33. **Should the separate Afterlife GH QST giver #29101 remain unrelated to
    Void mobile #29101 Drac?** Recommendation: retain each source identity and
    do not infer a quest connection from the shared number.
34. **Was the crude, sexually explicit meeting-room E description at #29105
    intentional and appropriate for the current game?** Recommendation: ask
    the content owner for an explicit editorial decision; do not silently
    rewrite the authored text as part of this audit.
35. **Can the normal AREA-based SHP build and missing-world.shp fallback be
    reconciled for this unlisted shop?** The keeper #29101 has no active AREA
    MOB prototype. Recommendation: confirm the supported boot inputs and
    keeper prototype before relying on fallback behavior.
36. **Do editor exports, deployment copies, runtime state, or persisted
    records contain another Void guildhall identity?** Recommendation: check
    those systems with their owners before any source move or archive proposal.

Until owners resolve these questions, keep all five files **RETAIN IN PLACE**
outside both manifests. Do not activate, merge, renumber, regenerate, move,
or archive them based only on shared numbers, incomplete portal prose, or
manifest exclusion.
