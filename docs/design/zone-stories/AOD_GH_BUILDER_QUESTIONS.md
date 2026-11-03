# AoD GH: builder questions

Canonical key: `component:aod_gh`. Physical coverage row 360. The
[source dossier](AOD_GH.md) and [static audit](../quest-story-followups/AOD_GH_AUDIT_REVIEW.md)
record the setting, disconnected WLD components, duplicate #295 identities,
and typed reset/shop/code dependencies. The recommendations below do not
establish that the source is loaded.

## Explicit builder questions and recommended resolutions

1. **Is AoD GH intended to remain outside `areas/AREA` and
   `areas/qst/AREA`?** Its WLD, MOB, OBJ, ZON, and SHP files are present, but
   neither manifest lists it. **Recommend** preserve its source-only status
   until the loader owner identifies the intended file set and resolves
   same-number map/prototype ownership.
2. **Are zone numbers #296 for AoD and #295 for Genocide Hall, Legion, and
   Void Guild Hall all intentional?** The titles are distinct, but three
   independent maps share #295. **Recommend** retain the authored headers and
   ask the world owner to confirm reset-zone identity before any loader edit.
3. **Which source owns rooms #29500–#29504 and #29510–#29512?** AoD overlaps
   different records from Genocide Hall, Legion, and, at #29510–#29512,
   `rf_op`. **Recommend** preserve every source map and resolve each identity
   before making a shared-room or entrance claim.
4. **Which source owns the other shared Legion rooms #29505–#29509 and
   #29520–#29522?** Their AoD and Legion descriptions differ. **Recommend**
   treat each as a separate source room until the owner resolves the VNUM
   namespace.
5. **Which mobile definitions should own #29500–#29502?** AoD's Bone Golems
   and Valrok share numbers with Genocide Hall and Legion mobiles.
   **Recommend** resolve the mobile namespace before co-loading or moving
   either zone; preserve each file's local reset links.
6. **Who owns object #29500 and the overlapping #29501–#29503 prototypes?**
   AoD #29500 is an obsidian badge, while Legion #29500 is a portal; both
   sources also define different records at #29501–#29503. **Recommend** keep
   AoD's badge/shop/reset references distinct from Legion's objects.
7. **Is the `inn` callback intended for AoD #29502?** Compiled source calls
   `real_room0(29502)` beneath a Players Guild comment, while AoD calls that
   room the Inner Sanctum and Legion has a different room there.
   **Recommend** ask the code/area owner to identify the intended loaded room
   before changing that binding.
8. **Which source map should resolve AoD's down exit from #29520 to #210208?**
   Both `surf.wld` and `surface2011.wld` define #210208 differently.
   **Recommend** keep the target source-only and choose a map owner before
   describing the route as active or moving either map.
9. **Is #141442 in `duris3.wld` the intended target from #29500?** The source
   map is not listed in the full area manifest, and no reverse WLD path was
   found. **Recommend** preserve the down exit with its Duris3 target identity
   until map ownership and return behavior are confirmed.
10. **Are the three WLD components intentional, and should #29516 and #29518
    remain outside directed reach from #29514?** Both rooms are outside the
    eight-room forward-reachable set in the citadel group. **Recommend** ask
    the builder to confirm the authored one-way/dead-end layout before
    changing exits.
11. **Are all four type-25 destination values intentional?** AoD objects
    #29501–#29504 store #29513, #29509, #29519, and #29507; only #29502 is
    reset in the ZON. **Recommend** confirm object placement, callback, and
    target room ownership before representing these values as working travel.
12. **What is the intended object for shop/reset reference #6089?** The AoD
    SHP stock and ZON G reset both name it, but no OBJ file defines it.
    **Recommend** identify the intended prototype or correct the source
    references before loading the shop or resets.
13. **Are the other fifteen shop products intended for Valrok?** The 16-item
    SHP list exactly matches the 16 G resets; #53, #108, and #109 have more
    than one source definition. **Recommend** retain the stock/reset pairing
    and confirm which source owns each duplicated prototype before loading.
14. **Are the seven Player Castles object resets intentional imports?** AoD
    ZON names #11061, #11111, #11013, #11062, and #11112; those prototypes
    exist in the active Player Castles source. **Recommend** preserve all
    seven exact source commands and verify owner intent before changing their
    rooms or prototypes.
15. **Should the 2026 gathering-bag resets remain on Valrok?** The ZON added
    #491–#494, which active Heavens OBJ defines as gathering containers.
    **Recommend** retain the four current G commands with their Heavens
    prototype identity and confirm the intended shop/restock behavior.
16. **Does AoD need a local quest contract or only environmental guild lore?**
    There is no AoD QST file, giver, or structured story sidecar.
    **Recommend** preserve the current prose-driven story unless its builder
    supplies a concrete quest design.
17. **Which saved/editor/runtime records must be checked before a future
    migration decision?** Tracked files do not describe external map-editor
    aliases, persisted instances, or live loader selection. **Recommend** keep
    the candidate **RETAIN IN PLACE** until the responsible owner closes those
    records and resolves the source identities above.
