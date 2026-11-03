# Afterlife GC OP2: builder questions

Canonical key: component:afterlife_op2. Physical coverage row 356. The
[source dossier](AFTERLIFE_GC_OP2.md) and [static audit](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md)
document three maps, nine room VNUMs that also belong to active Torg, two
D4 exits into unlisted Duris3, and seven teleport objects with stored targets
that do not verify their descriptive destinations. These recommendations do
not establish live behavior or owner intent.

## Explicit builder questions and recommended resolutions

1. **Who should own room VNUMs #29120–#29128?** Active torg.wld and
   afterlife_op2.wld define all nine IDs with different titles and content.
   **Recommend** confirm numeric ownership before changing either source or
   any path. [Room ownership](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md#active-torg-room-ownership)
2. **Are afterlife_op and afterlife_op2 separate passages or versions of
   one zone?** They share the normalized ZON title and header #1 but use
   different room ranges. **Recommend** retain separate candidate identities
   until authorship and title relationship are confirmed.
   [Identity and history](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md#identity-and-source-history)
3. **Should the three disconnected OP2 maps remain separate story areas?**
   The WLD has an icy cavern, foggy forest, and salt circle with no local
   links among them. **Recommend** preserve the three authored components
   until a builder confirms intended connections. [Source graph](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md#three-map-story-and-source-graph)
4. **Should active Torg routes that use #29120–#29128 keep those numbers?**
   Torg has fourteen direction records targeting IDs also declared by OP2.
   **Recommend** resolve active numeric ownership before treating any Torg
   path as OP2 ingress. [Torg ownership](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md#active-torg-room-ownership)
5. **Are the D4s from #29120 and #29123 intended to reach Duris3 rooms
   #172709 and #163943?** Their source titles are “The Ocean” and “Rolling
   Hills.” **Recommend** confirm both destinations and map ownership before
   changing either exit. [Authored boundaries](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md#three-map-story-and-source-graph)
6. **What return routes should exist from each Duris3 target?** Neither
   target has a direct exit to its OP2 source room, and Duris3 is not listed
   in the full area index. **Recommend** establish loaded endpoints and safe
   return paths before treating these as player routes.
7. **Which source should own mobile VNUMs #29101–#29104 used at all three
   entrances?** OP2's MOB file is empty and multiple candidate MOB files
   declare overlapping IDs. **Recommend** confirm each prototype owner
   rather than inferring it from a ZON reset. [Reset sources](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md#typed-resets-and-cross-candidate-dependencies)
8. **Do the reset mobiles match the “four stoic frost golems” in #29120?**
   The ZON repeats the same four IDs in the forest and salt-circle entrances
   too. **Recommend** resolve the source prototypes and preserve the prose
   versus reset discrepancy for the owner.
9. **Are the six Player Castles object resets for #11053 and #11103 intended
    in the first two rooms of each map?** These imports appear in all three
    pairs of entrances. **Recommend** preserve their source ownership and confirm
   each use before changing resets.
10. **Should the local portal objects #29122, #29123, and #29125 remain
    reset in their current rooms?** Their ZON placements put one portal at
    each map's endpoint. **Recommend** confirm placement and reset limits
    with the owner.
11. **Should Guildhall room #29109 continue to import OP2 objects #29120,
    #29124, and #29126?** This is a reverse source dependency from
    afterlife_gh.zon. **Recommend** preserve the imports until Guildhall and
    OP2 ownership is resolved. [Guildhall resets](../../../areas/zon/afterlife_gh.zon#L14)
12. **Where should object #29120 send its user?** Its description names
    Tundra while value[0] is #29122, shared by OP2 and Torg.
    **Recommend** confirm the target VNUM and text together.
    [Object record](../../../areas/obj/afterlife_op2.obj#L1)
13. **Where should object #29121 send its user?** Its description names the
    Goodie Continent while value[0] is #29120, also owned by active Torg;
    no current placement was found for this object. **Recommend** confirm
    destination and placement together. [Object record](../../../areas/obj/afterlife_op2.obj#L9)
14. **Where should objects #29122, #29123, and #29125 send their users?**
    Each names Khomani-Khan but stores #29109, which is Guildhall's Room of
    Exercises and active Torg's Small Barracks in the City.
    **Recommend** confirm the intended common or separate destination IDs.
    [Target audit](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md#portal-target-values)
15. **Should object #29124 target #29125 when its text names Evermeet?**
    #29125 is OP2's stone-portal room and an active Torg room.
    **Recommend** resolve both target ownership and description before
    considering it a verified portal route.
16. **Should object #29126 target #29128 when its description says “Undead
    Cont.”?** #29128 is both an OP2 endpoint and an active Torg room.
    **Recommend** confirm destination text and VNUM together.
17. **Is enter the intended action for all seven type-25 objects?** Each
    stores value[1]=7, which maps to CMD_ENTER.
    **Recommend** confirm intended access and travel behavior without
    assuming runtime destination resolution. [Command and handler](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md#portal-target-values)
18. **Should the editor coordinates remain attached to afterlife_op2?**
    Its map JSON stores all nine positions, while room IDs also belong to
    Torg. **Recommend** preserve the separate map file and zoneId until
    ownership is settled. [Editor map](../../../areas/map/afterlife_op2.json)
19. **Should a quest or shop layer be added, and are there external references
    to preserve?** No local QST/SHP or exact current-source QST reference was
    found; editor and persisted stores were not inspected. **Recommend**
    leave those behaviors unspecified until the owner reviews external state.
20. **May this candidate be moved or merged after the source questions are
    answered?** Room ownership, Duris3 boundaries, portal destinations,
    reset-mobile ownership, and editor/persisted/runtime closure remain open.
    **Recommend** keep it **RETAIN IN PLACE** until those decisions are
    explicit. [Candidate row 5](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L5)

This review changes no room, object, reset, portal text, area index, or
player-facing behavior.
