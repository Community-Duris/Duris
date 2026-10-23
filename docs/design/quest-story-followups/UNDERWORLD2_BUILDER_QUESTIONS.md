# Underworld2 builder questions and recommended resolutions

Canonical key: `component:underworld2`; coverage row 466 / CSV line 467 and
candidate dependency row 115 / CSV line 116. These questions document owner
decisions needed before activation, migration, or archive. The recommendations
preserve current source evidence; they are not gameplay changes.

1. **Who owns the canonical underworld2 map and its current source/editor project?**
   Recommendation: record the responsible builder and authoritative editable
   project before considering any source move.
2. **Is “Underdark of Duris” the intended public title for stem `underworld2`?**
   Recommendation: preserve the checked-in title until the map owner confirms
   an alias or title change.
3. **Is ZON #415402 the intended identity and `599999` the intended top VNUM?**
   The highest room is #440657. Recommendation: leave the declared range intact
   until its reservation and generator meaning are documented.
4. **Is the gap between the declared zone top and highest room intentional?**
   Recommendation: treat it as range metadata, not missing content, pending
   the original builder’s explanation.
5. **Why are all 8,256 room long descriptions empty?**
   Recommendation: confirm whether descriptions were never authored or live
   in an external editor/export source before reconstructing prose.
6. **Do the 16 room-title groups provide the complete intended narrative?**
   Recommendation: treat them as environmental labels only until the owner
   supplies a story brief or approved prose.
7. **Do the four large title groups describe one continuous dried-lava and
   infernal environment?** Recommendation: preserve their current labels and
   avoid inventing hazards, factions, or events from names alone.
8. **Should the “Bridge made of Bone and Flesh” titles signal a route or simply
   scenery?** Recommendation: do not infer bridge mechanics or a quest objective
   without a builder-defined use.
9. **Is “PITS OF CEREBRUS” an intentional in-world spelling beside active Pits
   of Cerberus?** Recommendation: preserve the raw title and ask before any
   editorial correction.
10. **Are the eleven singleton destination titles intended as named entrances
    or exit-side landmarks?** Recommendation: document each room’s purpose
    without changing its graph until the owner confirms directionality.
11. **Is #415402 the intended graph seed or starting room?** Raw directed
    traversal reaches 8,245 rooms from it. Recommendation: retain the current
    seed-based metric as a source observation, not an admission guarantee.
12. **Should the eleven singleton SCC rooms remain unreachable from #415402
    while still routing back toward it?** Recommendation: preserve this
    one-way topology pending an explicit access design.
13. **Are the 32,592 local directed exits intended to form a single weak
    component?** Recommendation: do not join, prune, or reverse edges solely
    to simplify graph metrics.
14. **Are all exit flags and key fields intentionally zero?** Recommendation:
    do not add doors, locks, keys, or hidden exits from environmental prose.
15. **Are the absence of fall and current fields intentional in a dried-lava
    environment?** Recommendation: treat the source as having no such declared
    mechanics; any new hazards need a separate design.
16. **Does ZON reset mode 0 with no reset commands intentionally leave the map
    without local prototype resets?** Recommendation: preserve it as authored
    and do not borrow resets from another Underworld source.
17. **Were MOB, OBJ, QST, SHP, and story-sidecar files intentionally omitted?**
    Recommendation: confirm whether those assets are external or never existed
    before creating same-stem files.
18. **Is there a separate quest, NPC, object, or story contract that belongs to
    this map?** Recommendation: require its owner and source location before
    linking or packaging it.
19. **Are all eleven exits to active destinations intentional connections?**
    Recommendation: keep them source-only and preserve every tuple until each
    destination owner confirms route intent.
20. **Should #417516 north to Cloister #67100 remain one-way?** Recommendation:
    request an explicit return-path decision; do not assume a reciprocal exit.
21. **Should #417529 north to Fort Boyard #38100 remain one-way?** This route
    is already in destination evidence. Recommendation: keep its candidate
    source identity and avoid counting it twice.
22. **Should #421086 west to Lab #82350 remain one-way?** Recommendation:
    preserve the boundary declaration while the Lab and map owners review it.
23. **Should #421439 east to Githzerai Stronghold #44400 remain one-way?**
    Recommendation: preserve the source route and require an owner-approved
    return design if one is intended.
24. **Should #422044 north to Nakral’s Crypt #14300 remain one-way?**
    Recommendation: retain the route with the map and confirm its intended
    source-side and destination-side access.
25. **Should #425645 west to Sevenoaks #38600 remain one-way?** Recommendation:
    preserve the authored direction; do not infer active ingress.
26. **Should #427709 east to Pits of Cerberus #22118 remain one-way?**
    Recommendation: preserve the source-only edge already recorded in the Pits
    review and avoid a duplicate count.
27. **Should #429194 south to Tezcat #98900 remain one-way?** Recommendation:
    keep it attached to the source map until both area owners confirm intent.
28. **Should #431340 west to Dragonnia #6803 remain one-way?** Recommendation:
    retain the existing source-only boundary with no assumed return.
29. **Should #436465 east to Forgotten Mansion #3500 and #437928 south to
    Temple of the Earth #43500 remain one-way?** Recommendation: retain both
    independent routes and resolve each with its destination owner.
30. **Do map-generator inputs, generic code-range checks, deployment settings,
    or persisted room records reference underworld2 outside this checkout?**
    Recommendation: have the map/runtime owners check those systems before
    activation, regeneration, migration, or archive.

These questions do not authorize activation, route repair, source generation,
or movement. Keep underworld2 **RETAIN IN PLACE** while ownership and any
external editor, runtime, deployment, and persisted-identity dependencies are
resolved. See the [audit review](UNDERWORLD2_AUDIT_REVIEW.md).
