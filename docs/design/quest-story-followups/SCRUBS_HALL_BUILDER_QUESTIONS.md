# scrubs_hall builder questions and recommended resolutions

Checkpoints 528 and 932, source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74.
These questions concern the unlisted Scrubs Hall map, its typed resets and
shop, overlapping room numbers, and the recent gathering-bag source edits.
They do not authorize activation, renumbering, loader changes, deletion,
moving, merging, or archiving.

## Explicit builder questions and recommended resolutions

1. **Who owns the scrubs_hall WLD, MOB, empty OBJ, ZON, and SHP files and
   their export process?** Recommended resolution: identify the authoring
   owner and whether each file is edited directly, generated, or supplied
   by deployment tooling.
2. **Is “Ye' Old ScRubS Pirate Galleon” the canonical title?** Recommended
   resolution: record the owner-approved title and aliases without deriving
   them from the filename or zone number.
3. **Why is scrubs_hall absent from areas/AREA and areas/qst/AREA when its
   ZON and SHP files were changed in 2026?** Recommended resolution: ask
   the area and deployment owners to confirm the intended loader inputs;
   recent source edits alone do not establish runtime membership.
4. **Who owns WLD #29100–#29109, which active Torg defines differently?**
   Recommended resolution: resolve the active Torg identities and the
   candidate Scrubs map as separate source records before changing either
   manifest or VNUM set.
5. **Are the different same-number maps in afterlife_gh, solstice_gh,
   void_gh, and wlust_gh intentional separate uses?** Recommended
   resolution: have those map owners document their namespaces and avoid
   treating unlisted files as proof of replacement.
6. **Do Torg's 13 WLD edges and 32 ZON room references to #29100–#29109
   belong to its listed map?** Recommended resolution: preserve them as
   Torg-owned numeric references unless its map owner confirms otherwise;
   they do not identify active Scrubs ingress.
7. **Should Scrubs #29100 D4 continue to target Duris3 #156119 “The Ocean”?**
   Recommended resolution: confirm the intended destination and map owner
   before keeping or redirecting the one-way outlet.
8. **Is the one-way route to #156119 intentional without a return path?**
   Recommended resolution: document the expected travel loop; do not add a
   return exit without the relevant mapper's direction.
9. **Is the ten-room, one-component ship and tavern topology complete?**
   Recommended resolution: have the mapper confirm the nineteen authored
   exits and identify any intended connection that is not present in source.
10. **Should four MOB #29100 golems be reset together in room #29100?**
    Recommended resolution: confirm the gate encounter and retain the
    current reset multiplicity until the area owner reviews it.
11. **Are MOB #29101 and #29102 intended to remain without ZON resets in
    this source?** Recommended resolution: identify any editor or external
    placement owner before adding or removing reset rows.
12. **Is MOB #29103, the tired trader, the keeper intended for SHP #29103?**
    Recommended resolution: confirm the mobile prototype and shop pairing;
    no same-number active MOB prototype exists in the current manifest.
13. **Should the shop be in room #29106 when active Torg owns that VNUM as
    the Shop of Gornogo?** Recommended resolution: ask both area and shop
    owners to verify room identity and in_room before loading this catalog.
14. **Is the code binding `inn` to room #29103 meant for the Scrubs map?**
    Recommended resolution: trace its author-approved room identity because
    current numeric lookup assigns the special to Torg's escape tunnel.
15. **Can the full-mode shop fallback safely include scrubs_hall.shp while
    no active keeper MOB #29103 exists?** Recommended resolution: have the
    shop and loader owners resolve the missing-keeper path before relying on
    the glob fallback; do not infer successful loading from source presence.
16. **Should the twelve ZON G imports and matching SHP stock rows be kept
    together?** Recommended resolution: preserve their exact shared item
    list while the shop and item owners check prototype availability.
17. **What is product #6089 meant to be?** Recommended resolution: confirm
    whether it is an external/deployment-only object or a stale VNUM; no
    current OBJ prototype was found.
18. **Which same-number object identity is intended for product #53?**
    Recommended resolution: confirm the Heavens rope coil against unlisted
    Dalvik, hostel, and mavulsk definitions before relying on numeric lookup.
19. **Do the 2026 gathering-bag additions confirm that scrubs_hall is one of
    the maintained guild-hall shops?** Recommended resolution: ask the
    commit author and deployment owner to record intended membership and
    catalog scope; the commit does not establish that the map is loaded.
20. **Does an editor export, saved state, or deployment record document a
    predecessor, successor, or retirement?** Recommended resolution: recover
    that provenance and persisted-reference closure before proposing any
    activation, renumbering, move, merge, or archive.

21. **Where is OBJ #6089 supplied for the checked-in ZON resets and SHP stock
    rows across 31 ZON files and 33 SHP files?** Recommended resolution:
    identify the canonical object record or generated/deployment owner, then
    qualify active, unlisted, and fallback consumers together; do not treat
    active WLD room #6089 as an item prototype.

See the [scrubs_hall static audit](SCRUBS_HALL_AUDIT_REVIEW.md#checkpoint-932-product-6089-is-a-shared-unresolved-source-reference).
