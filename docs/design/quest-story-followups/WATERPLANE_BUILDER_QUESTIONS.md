# Water Plane builder questions and recommended resolutions

Canonical key: `component:waterplane`; coverage data row 470 / CSV line 471;
candidate dependency row 119 / CSV line 120. These questions document owner
decisions needed before activation, source movement, or archive. The
recommendations preserve current files and do not authorize gameplay edits.

1. **Who owns the canonical Water Plane map and editable source?**
   Recommendation: name the map owner and authoritative editor/export before
   changing manifests or paths.
2. **Is “Water Plane” the intended public title for `waterplane`?** The active
   `plane_water_one` title is “Plane of Water.” Recommendation: keep both
   source titles and identities separate until an owner supplies a title or
   map crosswalk.
3. **Are zone header #1, top VNUM #72, and reset mode 2 intentional for this
   package?** Recommendation: preserve the current metadata until its range
   and reset meaning are documented.
4. **Are all twelve room records (#1–#10, #12, and #72) intended to form one
   source package?** Recommendation: retain the present file grouping while
   the owner confirms the isolated technical rooms.
5. **Are the repeated “An airy gravity well” titles and nearly repeated
   descriptions intentional?** Recommendation: preserve them as authored
   variation and correct no prose without the builder's direction.
6. **Does the prose about an unseen force pulling north describe a mechanic or
   only atmosphere?** The source has no explicit current or gravity field.
   Recommendation: do not add a mechanic based on narrative text alone.
7. **Should the Air curtain be reached south by D2 when the same prose says
   the force pulls north?** Recommendation: retain the declared direction and
   request an owner-approved route design before changing it.
8. **Does the red fog block sight only, or was it meant to block passage?**
   The WLD declares exits through it. Recommendation: preserve both prose and
   exits until the intended visibility and movement behavior is clarified.
9. **Are the colored curtain's separate natural-fog and magical-wall
   explanations intentional?** Recommendation: treat the extra descriptions
   as source text, not proof of conflicting implemented effects.
10. **Should room #12 remain a disconnected, undescribed “Elemental Plane of
    Water” room?** Recommendation: keep it isolated until the builder supplies
    its role and any desired exits.
11. **Is room #72 meant only as the mobile-generation room?** It has no exits
    or long description and holds all three M resets. Recommendation: preserve
    the room and reset placements; do not infer player access or spawn outcome.
12. **Are all three M commands at #72 intended, including their current
    counts and reset parameters?** Recommendation: retain the records while
    confirming spawn limits, reset rules, and whether the technical room is
    deliberately isolated.
13. **Who owns mobile VNUMs #1–#3 if this candidate is ever loaded?** Active
    Limbo and six other unregistered MOB sources have distinct same-number
    prototypes. Recommendation: require explicit namespace/loader ownership
    before activation; do not overwrite or consolidate by VNUM.
14. **Do the five unique-source D2 exits from #2 and #6–#9 intentionally
    lead into active Air rooms?** Recommendation: preserve these previously
    counted source-only records and obtain a return plan before changing them.
15. **Do the five Air exits from source IDs #1, #3, #4, #5, and #10 belong to
    Waterplane, despite those source IDs belonging to active Limbo/Heavens?**
    Recommendation: keep them classified as identity-overlap declarations,
    not Waterplane entrances.
16. **Are the five exits from #6–#9 to Heavens #4, #5, and #10 intended as
    cross-map routes even though those target numbers are local Waterplane
    IDs?** Recommendation: preserve the raw records and resolve destination
    ownership before treating them as local traversal or active boundaries.
17. **Should any Air or Heavens target have a return route into Waterplane's
    unique rooms #2 and #6–#9?** No active return is present. Recommendation:
    do not invent one; ask the map owners for endpoints if reciprocal access
    is intended.
18. **How should Waterplane retain distinct room identities across the other
    legacy WLD files sharing #1–#10, #12, or #72?** Recommendation: keep all
    differing room blocks and require an explicit map/loader plan before
    activating or renumbering any of them.
19. **Are the 87 directions from other unregistered WLD files to #2, #6–#9,
    and #72 intended to remain local to their own same-number rooms?** Each
    source WLD defines its target locally. Recommendation: preserve those
    records as source-local loops unless their own builders confirm another
    destination.
20. **Should Waterplane #3 D3 to numeric #1 connect the Heavens and Limbo
    identities, or is it only a legacy local edge?** Recommendation: retain
    the record as ambiguous; do not treat it as a Waterplane-to-Limbo route.
21. **Was the zero-byte `waterplane.obj` file intentionally kept?**
    Recommendation: preserve the tracked empty file and do not invent object
    prototypes or destinations.
22. **Were local QST, SHP, story-sidecar, and editor-map sources intentionally
    omitted?** Recommendation: ask the owner for any external authoritative
    copies before authoring replacements or considering archive.
23. **Is the candidate intentionally absent from both `areas/AREA` and
    `areas/qst/AREA`?** Recommendation: keep it outside both manifests while
    identity, reset, route, and dependency questions remain open.
24. **Do deployment copies, editor exports, live runtime state, or persisted
    player records refer to another Waterplane identity?** Recommendation:
    resolve those sources before any activation, merge, source move, or
    archive decision; the repository audit does not establish those external
    facts.
