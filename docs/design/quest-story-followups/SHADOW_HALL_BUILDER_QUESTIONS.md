# shadow_hall builder questions and recommended resolutions

Checkpoints 529 and 933, source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74.
These questions concern the unlisted Shadow Law Guildhall, its three-room
map, judged-soul reset, Player Castles object imports, external ZON uses,
code helper roles, and same-number Underworld3 identities. They do not
authorize activation, renumbering, editing, moving, merging, or archiving.

## Explicit builder questions and recommended resolutions

1. **Who owns the shadow_hall WLD, MOB, empty OBJ, and ZON files?**
   Recommended resolution: identify the authoring owner and whether these
   files are edited directly, generated, or supplied by deployment tooling.
2. **Is “Shadow Law Guildhall” the canonical title?** Recommended
   resolution: record the owner-approved title and aliases without deriving
   them from the filename or VNUM band.
3. **Why is shadow_hall absent from areas/AREA and areas/qst/AREA?**
   Recommended resolution: confirm current loader intent with the area and
   deployment owners before making membership changes.
4. **Who owns room VNUMs #30100–#30102, which listed Underworld3 defines
   differently?** Recommended resolution: document the active Underworld3
   identities and preserve the Shadow Law source as a separate candidate.
5. **Are the same numbers in unlisted cabal_hall and nah separate maps?**
   Recommended resolution: have each map owner record its identity; do not
   infer a replacement relationship from shared numbers or filenames.
6. **Does Underworld3's `*300-303` range intentionally contain different
   #30100–#30102 room prototypes?** Recommended resolution: ask the manifest
   and world-data owners to confirm the range and intended namespace.
7. **Should the D4 outlet from Courtyard of Law #30102 target Duris3
   #151052 “Thick Forest”?** Recommended resolution: confirm the target map
   and intended endpoint with both area owners before retaining or changing
   the route.
8. **Is the #151052 outlet intentionally one-way?** Recommended resolution:
   document the intended return journey; no reverse exit to the Shadow Law
   rooms appears in the current Duris3 source.
9. **Is the three-room Walk/Nexus/Courtyard graph complete with five exits?**
   Recommended resolution: have the mapper confirm the four local links and
   the external outlet before adding any connector.
10. **Should four copies of MOB #30100 be reset in Courtyard #30102?**
    Recommended resolution: confirm the encounter's population and room
    with the area owner, preserving the four authored commands meanwhile.
11. **Does judged-soul MOB #30100 have any intended active placement?**
    Recommended resolution: identify its current prototype and placement
    owner because no active same-number MOB definition exists.
12. **Is Player Castles OBJ #11110, the bulletin board, intended in room
    #30101?** Recommended resolution: confirm this cross-area import and
    keep it distinct from the local room identity.
13. **Is Player Castles OBJ #11060, the magic mouth, intended in room
    #30102?** Recommended resolution: confirm the object and placement
    with the Player Castles and Shadow Law owners.
14. **Should the empty shadow_hall OBJ file remain part of the source set?**
    Recommended resolution: preserve the tracked file until its editor and
    export role is documented; do not infer it is disposable from zero
    records.
15. **Do the unlisted cabal_hall.shp and nah.shp room bindings belong to
    their own source maps?** Recommended resolution: keep those typed shop
    dependencies source-local unless their owners establish another target.
16. **Do Underworld3's eight exits to these room numbers remain internal to
    its active map?** Recommended resolution: preserve them as
    Underworld3-owned edges; numeric overlap does not make them Shadow Law
    ingress.
17. **Should Underworld3's water-weird reset in room #30101 remain there?**
    Recommended resolution: confirm the active Underworld3 room identity
    and keep the reset attached to that map's own VNUM.
18. **Does the Shadow Law setting have a separate quest or story source?**
    Recommended resolution: ask the story owner for any QST, sidecar, or
    editor narrative that is not present beside these files.
19. **Did the 2013 `stuff` update or 2009 area-tree move retire or replace
    shadow_hall?** Recommended resolution: recover a direct owner decision;
    those path events alone do not establish retirement.
20. **Does editor, saved-state, or deployment data preserve room and object
    references to this guildhall?** Recommended resolution: recover that
    provenance and persisted-reference closure before any path proposal.

21. **Are Player Castles objects #11060 and #11110 intended in the other
    `bastard` and `sky_gh` ZON rows, and how should code's mobile-helper
    references to #11110 be interpreted?** Recommended resolution: preserve
    the typed OBJ imports and #11060 `magic_mouth` binding; ask the code owner
    to confirm the #11110 helper MOB role because no same-number MOB
    prototype is checked in and the helper list does not establish an OBJ/MOB
    identity.

See the [shadow_hall static audit](SHADOW_HALL_AUDIT_REVIEW.md#checkpoint-933-player-castles-imports-have-separate-code-consumers).
