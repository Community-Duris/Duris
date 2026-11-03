# sky_gh builder questions and recommended resolutions

Checkpoint 531, source snapshot `c500ac51b8cd26540943257e7ca4e9807263ef74`.
These questions concern the unlisted Hall of the Clouds source, Harrow's
same-number room and mobile identities, the Duris3 outlet, Player Castles
imports, and the shop's possible full-mode SHP fallback. They do not
authorize activating, changing, moving, merging, or archiving any source.

## Explicit builder questions and recommended resolutions

1. **Who owns the five `sky_gh` files and can decide their source status?**
   Recommended resolution: identify the area, shop, and deployment owners
   before proposing any change.
2. **Is “The Hall of the Clouds” the canonical title, and is “Hall of the
   Skies” an alias?** Recommended resolution: have the source owner record
   the approved title and aliases instead of inferring them from WLD prose.
3. **Why is `sky_gh` absent from both active area manifests?** Recommended
   resolution: confirm intended loader and QST status with manifest and
   deployment owners before changing either list.
4. **Who owns WLD #29400–#29404, which active Harrow defines as different
   Fountain Square/garden rooms?** Recommended resolution: document both map
   identities and keep Harrow's active definitions distinct until the
   respective owners resolve the collision.
5. **How do the shared room numbers in `dark.wld` and `infernal_gh.wld`
   relate to the Hall of the Skies?** Recommended resolution: ask each map
   owner for source provenance; matching VNUMs do not prove a common map or
   replacement.
6. **Should the D4 up exit from #29400 target unlisted Duris3 room #153760,
   “Mountains”?** Recommended resolution: confirm endpoint and map ownership
   with both builders before changing the route.
7. **Is the outlet intentionally one-way?** Recommended resolution:
   document the intended return route; #153760's current Duris3 exits lead
   to other Duris3 rooms rather than #29400.
8. **Is the five-room graph complete with eight local exits and one
   outlet?** Recommended resolution: have the mapper confirm each authored
   link and preserve the current source graph while that review is open.
9. **Should golem #29402 be the only golem reset in room #29400?**
   Recommended resolution: confirm the intended encounter and whether
   #29400/#29401 golems are meant to be unplaced by this ZON.
10. **Are ZON resets #29402/#29403 meant to resolve to Harrow prototypes
    under current numeric ownership?** Recommended resolution: reconcile
    their identities with Harrow before any manifest or prototype change.
11. **Are Player Castles objects #11013 and #11110 intended in candidate
    room #29403?** Recommended resolution: preserve both typed imports and
    confirm their placements with both area owners.
12. **Does object #11013 retain its Player Castles bonding-altar behavior
    in this room?** Recommended resolution: ask the Player Castles owner to
    confirm the external object behavior independently of the local map.
13. **Are all 14 `G` rows and SHP products intended to stay in the merchant's
    stock list?** Recommended resolution: compare the ZON and SHP imports
    with the shop owner and retain both exact source lists meanwhile.
14. **How should missing OBJ #6089 and the alternate definitions of #53,
    #108, and #109 be handled?** Recommended resolution: resolve each
    product's typed owner and intended current prototype; do not guess from
    a matching number.
15. **Were bags #491–#494 intentionally added to this unlisted shop in the
    September 2026 update?** Recommended resolution: confirm with the
    gathering-bag and area owners; recent edits do not answer manifest
    membership.
16. **Is candidate room #29403, “Inn of the Hall of the Skies,” meant to
    share a VNUM with Harrow's Fountain Square?** Recommended resolution:
    resolve the room identity with both builders and preserve Harrow's
    current `inn` code binding pending that decision.
17. **Should the #29403 merchant be the active Harrow cucumber if its SHP
    record is read by fallback?** Recommended resolution: check the intended
    keeper prototype and loader path with the shop and deployment owners.
18. **Should shop `in_room` #29404 resolve to Harrow's Fountain Square
    rather than the Hall's Armory?** Recommended resolution: confirm the
    shop's room identity before treating the source SHP as operational.
19. **Should full-mode shop loading continue to use the individual-file
    fallback while `areas/world.shp` is absent?** Recommended resolution:
    resolve generated shop inputs and fallback expectations; static source
    inspection does not prove a shop booted.
20. **Does editor, export, saved-state, or deployment data preserve this
    five-file component or its VNUM references?** Recommended resolution:
    recover that provenance and retain all five files IN PLACE until the
    owner records a separate path decision.
21. **Does Harrow story `mob_vnum: 29404` remain a Harrow mobile contact
    while the same sidecar's #29404 reward stays object-typed and the
    `sky_gh` #29404 WLD/SHP values stay room-typed?** Recommended
    resolution: preserve the field-specific Harrow and sky_gh identities;
    do not reinterpret the story contact, reward, or shop location from the
    shared numeric value.

See the [sky_gh static audit](SKY_GH_AUDIT_REVIEW.md).
