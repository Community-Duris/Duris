# Venom GH builder questions and recommended resolutions

Canonical key: `component:venom`; coverage row 467 / CSV line 468 and
candidate dependency row 116 / CSV line 117. These questions document owner
decisions needed before activation, migration, or archive. Recommendations
preserve the checked-in records and do not authorize gameplay changes.

1. **Who owns the canonical Venom GH map and its editable source?**
   Recommendation: name the guild/map owner and authoritative editor project
   before changing list membership or source paths.
2. **Is “Venom GH” the intended public title and guild identity?**
   Recommendation: keep the checked-in title until the Venom owner confirms an
   alias or title correction.
3. **Is zone header #303 with top #30309 the intended range metadata?**
   Recommendation: preserve the current header and range pending confirmation.
4. **How should the three WLD maps `bb_gh`, `venom`, and `vodgh` be identified
   as separate maps despite sharing rooms #30300–#30309?** Recommendation:
   retain each source family and do not infer ownership from numeric equality.
5. **Are all ten same-numbered room records across those files intentionally
   different?** Recommendation: preserve the three authored descriptions and
   resolve each VNUM’s owner before loading more than one source.
6. **Which source map should ud_map2 #1126542 down to #30300 enter?**
   Recommendation: leave it an ambiguous source-only target until a map owner
   names the intended destination.
7. **Should Venom #30300 up to ud_map2 #1124260 remain one-way?**
   Recommendation: preserve the authored exit and ask whether the separate
   #1126542 inbound declaration is related or independent.
8. **Are the two ud_map2 boundary records intended to connect to different
   Venom/BB GH/VoDs GH locations?** Recommendation: require an explicit
   endpoint pairing; the current VNUMs do not form a reciprocal pair.
9. **Is the four-room forward reach from Venom #30300 intentional?**
   Recommendation: retain the graph as authored; do not add links to the
   #30303–#30305 branch without approval.
10. **Should the Merchant’s Hovel and Elder’s Hall cycle remain unreachable
    from #30300 in raw directed traversal?** Recommendation: treat the one-way
    relationship as a builder decision, not an automatic graph defect.
11. **Which map’s room #30303 should receive the compiled inn callback?**
    [`specs.assign.c`](../../../src/specs/specs.assign.c#L2455) binds the raw
    number while three room records share it. Recommendation: resolve the
    owner and intended service before changing the binding.
12. **Are Venom’s two Player Castles object resets intended in rooms #30300
    and #30301?** Recommendation: preserve object #11054 and #11104 imports
    until both object and guildhall owners confirm placement.
13. **Do the “magic mouth” and “board” descriptions fit the intended Venom
    rooms?** Recommendation: keep the imported prototypes distinct from
    Venom-local objects, which are absent.
14. **Are the four M reset commands for bronze golems intended at #30300?**
    Recommendation: preserve the two repeated #30300 rows and the #30301/#30302
    rows until the owner clarifies whether the duplicate command is deliberate.
15. **Which of the three same-numbered #30300–#30302 mobile sets owns Venom’s
    resets?** Recommendation: preserve Venom, BB GH, and VoDs GH prototype
    records separately and resolve loader ownership first.
16. **Are the different stats and flags on the three bronze-golem sets
    intentional?** Recommendation: do not merge or normalize their records by
    name alone.
17. **Should Venom have a local OBJ source or remain empty?** Recommendation:
    keep `venom.obj` empty unless the map owner supplies a source-backed item
    design.
18. **Is a Venom SHP file intentionally absent?** Recommendation: do not
    attach BB GH or VoDs GH’s #30301 shop to Venom from numeric matches alone.
19. **Should BB GH shop #30301 at room #30302 and VoDs GH shop #30301 at
    #30306 remain separate?** Recommendation: confirm each keeper and location
    owner before any shop aggregate or activation decision.
20. **Does Sunhall object #30202’s type-25 destination #30302 target Venom,
    BB GH, or VoDs GH?** Recommendation: keep the unregistered portal target
    unresolved until its author identifies the map owner.
21. **Do BB GH and VoDs GH portal objects target their own local maps?**
    Recommendation: preserve their paired source records and do not treat the
    same target VNUMs as confirmed Venom entry.
22. **Are the #30300–#30309 titles and descriptions the full intended Venom
    story?** Recommendation: use the checked-in cave and guildhall prose as
    evidence; request a separate story brief before expanding it.
23. **Should the “negative energy,” shifting images, and bright treasury text
    remain descriptive only?** Recommendation: do not infer damage, blindness,
    treasure, or magical mechanics from prose alone.
24. **Are the training grounds and clan hierarchy meant to define any actual
    practice, rank, or combat contract?** Recommendation: keep them as room
    narrative absent an authored quest or trainer contract.
25. **Is reset mode 0 intentional with the current O and M command list?**
    Recommendation: preserve all source reset rows and ask the zone author
    what the header means in this source.
26. **Should any active WLD map have an entrance to Venom?** No active WLD
    inbound direction was found. Recommendation: do not create access until
    an area owner specifies an entry and safe return.
27. **Are there intended links from Venom to areas other than ud_map2?**
    Recommendation: preserve the single authored boundary and request explicit
    endpoints for any additional connection.
28. **Should the shared #303 VNUM family remain out of both manifests until
    identities are settled?** Recommendation: keep Venom RETAIN IN PLACE and
    avoid loading competing room/mobile records together.
29. **Does any map-editor, generator, source-control branch, or deployment copy
    define a preferred Venom version?** Recommendation: compare the editable
    owner source and deployed artifact before any regeneration or migration.
30. **Do runtime or persisted player/object records refer to Venom rooms or
    prototypes?** Recommendation: check those external states before proposing
    activation, renumbering, movement, or archive.

These are conservative owner decisions, not gameplay fixes. Keep Venom
**RETAIN IN PLACE** while map identity, #303 ownership, typed imports, shop and
portal references, runtime configuration, and persisted identity are
unresolved. See the [audit review](VENOM_AUDIT_REVIEW.md).
