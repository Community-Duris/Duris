# The Tharnadian Causeway: builder questions

Canonical key: `component:aopal`. Physical coverage row 361. The
[source dossier](AOPAL.md) and [technical audit](../quest-story-followups/AOPAL_AUDIT_REVIEW.md)
record the setting, map identity, loader status, and typed dependencies. The
recommendations below do not establish that the candidate is loaded.

## Explicit builder questions and recommended resolutions

1. **Is `aopal` meant to remain outside `areas/AREA` and
   `areas/qst/AREA`?** Both files retain commented #433–#434 entries.
   **Recommend** preserve the current source-only status until the map owner
   confirms the intended loader set and resolves the room identities.
2. **Is “The Tharnadian Causeway” the intended title and alias for the
   `aopal` source?** The comments describe a road connected to Tharnadia, but
   no active WLD edge to Tharnadia was found. **Recommend** keep the current
   title and comments distinct from claims about an active Tharnadia route.
3. **Who owns rooms #43301–#43328?** Active Ship Yards has all 28 numbers
   under different room records. **Recommend** preserve both maps and resolve
   room ownership before co-loading or moving either source.
4. **Are the four non-overlapping Aopal exits into Ship Yards intended?**
   They originate at #43329, #43330, #43334, and #43342 and target active
   Ship Yards numbers. **Recommend** keep them on Aopal candidate row 10 as
   source-only directions; do not infer that their local Aopal targets merge
   with Ship Yards rooms.
5. **Is #43301 west to Shadow Forest #6368 intended?** The source room shares
   its number with a different active Ship Yards room. **Recommend** retain
   the Aopal record under its own map identity and do not treat it as active
   ingress.
6. **Is the isolated #43343 “Spawn Room” meant to lead north to Ancient
   Bridge #85606?** The room has no local incoming WLD edge. **Recommend**
   preserve the direction and confirm the map owner's intended access and
   return behavior.
7. **Which map should own #230412, the east exit from #43323?** `surf.wld`
   defines an Eastern Road and returns west to Aopal; `surface2011.wld`
   defines the same number as a storm-sea room. **Recommend** retain the
   unresolved map identities until the map owner selects the source; neither
   candidate is active.
8. **Are the 43-room component and isolated #43343 intentional?** From the
   causeway root #43301, #43342 and #43343 are not forward reachable.
   **Recommend** preserve the authored graph and ask the builder to confirm
   its intended access, including the separate patrol service.
9. **Should the reset-closed and secret doors remain as authored?** Twelve
   nonzero WLD exit states match twelve 100%-chance ZON D resets, with ten
   state-1 directions and the well pair in state 5. **Recommend** retain the
   matching pairs until the owner confirms door and reset behavior.
10. **Is mobile #6116 intended at causeway room #43301?** No MOB prototype
    #6116 was found in the repository. **Recommend** identify the intended
    mobile or correct the source reference before considering loader changes.
11. **Are both #445 resets intended imports from active Heavens?** Aopal
    places #445 at #43301 and #43322; Heavens defines it as a Tharnadian
    highway patrolman. **Recommend** preserve the exact imports as source-only
    and confirm the shared mobile owner.
12. **Is Samara intended as an environmental haunting or a quest lead?** The
    ghost is reset at the well with a soaking wet white dress, but there is
    no Aopal QST. **Recommend** retain the prose and reset pairing without
    inventing a quest contract.
13. **Are the 37 mobile reset commands and duplicate animal placements
    intentional?** Several rat, merchant, dog, and archer commands repeat.
    **Recommend** keep exact reset counts and ask the builder before altering
    group sizes.
14. **Is the traveling merchant meant to offer a service?** Its description
    says it sells goods, but no Aopal SHP file exists. **Recommend** treat the
    shop wording as authored prose until the owner identifies an actual shop
    source and loader behavior.
15. **Is `patrol_shops` intended for Aopal room #43341?** Compiled code binds
    the handler there; it lists room #43342 and creates a mobile at
    #43343. **Recommend** preserve the code and source map separately until
    the code owner confirms the numeric binding and loaded room identity.
16. **Are the #43314 patrolman and #43315 marshall the intended staging
    choices?** Both are reset in #43342 for the compiled patrol handler.
    **Recommend** retain the staged prototypes and ask the owner to verify
    their hiring and movement behavior in an authorized runtime review.
17. **Are the Captain's mantle and Samara's dress reset relationships
    intentional?** The ZON equips #43305 on captain #43313 and gives #43304
    to Samara #43307. **Recommend** preserve both typed links and their
    authored descriptions.
18. **Should Aopal have any typed QST, SHP, or story-sidecar consumers?**
    Repository scans found no external QST item, shop product, or structured
    sidecar reference for its mobile/object VNUMs. **Recommend** keep this
    negative evidence bounded to current repository sources.
19. **Are the 2012–2015 same-stem file edits the complete history available
    for this area?** Followed tracked paths show no predecessor or whole-zone
    replacement. **Recommend** keep the current separate source identity and
    record any later editor history from the map owner.
20. **What editor, persisted-room, and live-loader evidence is required
    before any future move?** The tracked files cannot answer those questions.
    **Recommend** keep candidate row 10 **RETAIN IN PLACE** until the owner
    closes those records and resolves the loader, route, and prototype
    dependencies above.
