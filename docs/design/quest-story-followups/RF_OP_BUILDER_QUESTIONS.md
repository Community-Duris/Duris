# rf_op builder questions and recommended resolutions

Checkpoints 527 and 931, source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74.
These questions concern the unlisted four-file Tharnadian Outpost source,
its reused rf_gh cathedral fragment, portal destinations, and separate
same-number WLD/MOB/OBJ roles. They do not authorize loading, renumbering,
editing, moving, merging, or archiving.

## Explicit builder questions and recommended resolutions

1. **Who owns rf_op and the process that exports its WLD, MOB, OBJ, and ZON
   files?** Recommended resolution: identify the authoring owner and whether
   these files are independent, generated, or deployment supplied.
2. **Is “The Tharnadian Outpost of Rotting Flesh” the canonical title?**
   Recommended resolution: record owner-approved titles and aliases without
   deriving them from the filename or numeric band.
3. **Was the cathedral fragment intentionally copied from rf_gh?**
   Recommended resolution: have both map owners confirm separate placements
   and maintainers; preserve both sources until they do.
4. **Should the changed portal destinations be preserved as the only
   differences in the reused fragment?** Recommended resolution: compare the
   two portal intents individually rather than treating the file pair as an
   exact whole-area replacement.
5. **Who owns WLD #29510–#29512, which are also used by aod_gh,
   genocide_hall, and legion?** Recommended resolution: document source
   ownership for each same-type room ID before activating any candidate.
6. **Does active Harrow's *294-296 manifest band reserve these VNUMs, even
   though the listed WLD has no #29510–#29512 records?** Recommended
   resolution: ask the world-data owner to explain the band boundary; do not
   treat it as proof that an unlisted map is loaded.
7. **Where should ITEM_TELEPORT object #29510 send its user?** Recommended
   resolution: identify the intended #29505 room with the portal and map
   owners before changing value[0].
8. **Is #29505 meant to be the AOD GH Inn of Nightmares or the Legion
   torchlit castle walkway?** Recommended resolution: choose an owner-approved
   destination; both definitions are unlisted and have different identities.
9. **Is room #29505 missing from rf_op, or is the portal meant to cross into
   another separately loaded component?** Recommended resolution: recover the
   authored destination map and deployment manifest before adding or
   redirecting a route.
10. **Is OBJ #29511 meant to teleport to rf_op altar #29512?** Recommended
    resolution: confirm that destination and placement because the current
    ZON does not place this second portal.
11. **Should OBJ #29510 continue to be reset at altar room #29512?**
    Recommended resolution: ask the map owner whether this location is
    intentional and preserve the current row until confirmed.
12. **Should the D4 exit from room #29510 continue to Duris3 #154153?**
    Recommended resolution: confirm the ocean destination with its world
    owner before retaining or redirecting the one-way exit.
13. **Is the #154153 outlet intentionally one-way?** Recommended resolution:
    document expected return travel; the current Duris3 source has no exit
    back to rf_op.
14. **Are the four cherubim reset placements meant to match rf_gh's three
    first-angel and one second-angel placements?** Recommended resolution:
    confirm whether they are parallel encounter populations or a copied
    reset block before tuning either source.
15. **Why do all four mobiles spawn in gate room #29510 while the altar holds
    the single portal object?** Recommended resolution: ask the author to
    confirm the intended population and scene flow.
16. **Does the Father Benali name refer to active Surface MOB #500209?**
    Recommended resolution: ask the story owner to confirm the character
    relationship; a shared name alone does not establish a route.
17. **How does the Tharnadian setting connect to current Surface geography?**
    Recommended resolution: document any intended story or travel link;
    this WLD has no Surface exit and no same-stem QST.
18. **Should this ZON with reset mode 2 and five local commands remain outside
    areas/AREA?** Recommended resolution: resolve intended manifest and
    generated-aggregate inputs without changing loader membership first.
19. **Which editor export, saved state, or deployment manifest records the
    #29505/#154153 destinations and reused fragment?** Recommended resolution:
    recover that provenance and saved references before a route or VNUM edit.
20. **Has an owner explicitly retired or replaced any part of rf_op?**
    Recommended resolution: the checkout history only shows the area-tree
    move; require owner and editor evidence before any activation, move,
    merge, rename, or archive proposal.

21. **Does Legion OBJ #29500's type-25 destination #29512 refer to Legion's
    own room #29512 rather than rf_op's same-number altar?** Recommended
    resolution: preserve the reference as Legion source data because Legion
    defines its own #29512 WLD room and is also absent from the active
    manifest; do not count it as active inbound travel to rf_op.

See the [rf_op static audit](RF_OP_AUDIT_REVIEW.md#checkpoint-931-same-number-records-keep-separate-typed-roles).
