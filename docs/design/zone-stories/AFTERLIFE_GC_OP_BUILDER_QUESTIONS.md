# Afterlife GC OP: builder questions

Canonical key: `component:afterlife_op`. Physical coverage row 355. The
[source dossier](AFTERLIFE_GC_OP.md) and [static audit](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md)
document an unregistered three-room portal passage, conflicting room VNUMs,
and teleporter values that do not match their descriptive destinations. These
recommendations do not establish live behavior or owner intent.

## Explicit builder questions and recommended resolutions

1. **Who should own room VNUMs #29110–#29112?** Active `torg.wld`,
   unregistered `afterlife_op.wld`, and `void_gh.wld` give them different
   maps. **Recommend** confirm numeric ownership before any source or path
   moves. [Three-way comparison](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#room-number-and-source-identity-conflicts)
2. **Is Afterlife OP intended to remain separate from `void_gh` despite the
   shared room numbers?** Their titles, layouts, and descriptions differ.
   **Recommend** preserve the maps as separate source identities until an
   owner decides the intended VNUM ranges. [OP and void rooms](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#room-number-and-source-identity-conflicts)
3. **Are `afterlife_op` and `afterlife_op2` distinct passages or versions of
   one zone?** Their `.zon` titles match, but their room ranges do not.
   **Recommend** retain both candidates independently until their authorship
   and shared title are explained. [Title/history](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#room-number-and-source-identity-conflicts)
4. **Where should object #29110 send its user?** Its text says Khomani-Kan,
   while type-25 `value[0]` is #29106, a Guildhall/Torg collision.
   **Recommend** confirm the intended destination VNUM and descriptive name
   together. [Portal record](../../../areas/obj/afterlife_op.obj#L1),
   [target ownership](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#typed-resets-and-portal-targets)
5. **Where should object #29111 send its user?** Its text says Goodie
   Continent, while `value[0]` is #29110, shared by this source, active Torg,
   and `void_gh`. **Recommend** confirm a unique destination before treating
   the description as a route. [Portal record](../../../areas/obj/afterlife_op.obj#L9)
6. **Is `enter <portal>` the intended action for both type-25 objects?** Both
   encode `value[1]=7`, which maps to `CMD_ENTER`. **Recommend** confirm the
   intended access rule while preserving the shared target-owner question.
   [Command ID](../../../src/cmd/interp.h#L42),
   [target handling](../../../src/magic/spell_travel.c#L991)
7. **Is the D4 exit from #29112 meant to reach #157174, “The Ocean”?** The
   source target exists in `Duris3.wld`, which is absent from the full area
   list. **Recommend** confirm the destination and loader ownership before
   changing the exit. [Afterlife exit](../../../areas/wld/afterlife_op.wld#L33),
   [Duris3 target](../../../areas/wld/Duris3.wld#L1200655)
8. **What return path is intended from the Ocean target?** #157174 has no
   direct exit back to #29112. **Recommend** identify the intended return
   route before treating this as a safe source connection.
   [Connectivity audit](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#local-graph-and-outward-boundary)
9. **Should `Duris3` remain the target map if it stays outside `areas/AREA`?**
   It is a separate existing source candidate. **Recommend** preserve the
   relationship and resolve both endpoints in the owner's map decision.
   [Duris3 candidate row 36](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36)
10. **Is the zero-byte `afterlife_op.mob` file intentional?** Its ZON refers
    to four mobile VNUMs but declares none locally. **Recommend** retain the
    file as-is until its intended ownership is confirmed.
    [MOB inventory](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#inventory-and-source-membership)
11. **Which source should own the reset mobiles #29101–#29104?** Matching
    prototypes occur in `afterlife_gh.mob` and other unregistered candidate
    MOB files. **Recommend** confirm the intended source for each numeric ID
    before loading or merging candidates. [Prototype evidence](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#room-number-and-source-identity-conflicts)
12. **Is Player Castles object #11053 intended in the tunnel entrance?** Its
    source description calls it a magic mouth; this ZON places it in #29112.
    **Recommend** preserve Player Castles as the prototype owner and confirm
    its authored use here. [Reset](../../../areas/zon/afterlife_op.zon#L10),
    [prototype](../../../areas/obj/player_castles.obj#L145)
13. **Is object #29111 intentionally imported into Guildhall room #29106?**
    The Guildhall reset consumes this OP-owned object as well as this map's
    room range. **Recommend** preserve that source dependency until the
    portal target and placement are resolved. [Guildhall reset](../../../areas/zon/afterlife_gh.zon#L12)
14. **What relationship should the Guildhall have to these OP maps?** The
    Guildhall imports OP's #29111 object and resets mobile IDs #29101–#29104.
    **Recommend** retain explicit typed ownership records across the separate
    candidates. [Guildhall audit](../quest-story-followups/AFTERLIFE_GH_AUDIT_REVIEW.md#typed-reset-qst-shop-and-code-references)
15. **Should editor coordinates remain attached to `afterlife_op`?** Its JSON
    maps the three room VNUMs. **Recommend** preserve that file and zone ID
    until room ownership is settled. [Editor map](../../../areas/map/afterlife_op.json)
16. **Is this an environment-only portal passage?** It has no local QST or
    shop source and the MOB file is empty. **Recommend** leave quest and shop
    behavior unspecified unless a builder defines new source content.
    [Source inventory](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#inventory-and-source-membership)
17. **Is the spaced “AFTERLIFE” engraving intentional?** It is the passage's
    only explicit link to the wider Afterlife setting. **Recommend** confirm
    canonical lettering before copy editing. [Tunnel room](../../../areas/wld/afterlife_op.wld#L33)
18. **Should the two portal narratives remain attached to these two objects?**
    Their visible destination names disagree with their stored target IDs.
    **Recommend** reconcile item text and type-25 data as one owner decision.
    [Portal values](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#typed-resets-and-portal-targets)
19. **Are there editor or persisted references to any of the competing room
    VNUMs?** The static source review does not inspect those stores.
    **Recommend** include them in any future ownership review before moving
    the candidate. [Retention plan](../quest-story-followups/ATTIC_MIGRATION_PLAN.md#checkpoint-442-afterlife-op-retains-its-source-identity)
20. **May this candidate be moved or merged after the source questions are
    answered?** The current map overlaps active Torg and `void_gh` and has
    unresolved portal targets and imports. **Recommend** keep it **RETAIN IN
    PLACE** until ownership, destination, and external-state closure are
    explicitly resolved. [Candidate row 4](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L4)

This review changes no room, object, reset, portal text, area index, or
player-facing behavior.
