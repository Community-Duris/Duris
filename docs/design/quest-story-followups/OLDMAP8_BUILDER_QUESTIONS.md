# oldmap8 builder questions and recommended resolutions

Checkpoint 518, source snapshot `c500ac51b8cd26540943257e7ca4e9807263ef74`.
The questions below concern the WLD-only regional grid and its competing
same-number Duris3 map. They do not authorize loading, merging, renumbering,
moving, or archiving either file.

1. **Who owns `oldmap8.wld`, its editor project, and any map generator or
   coordinate metadata?** Recommended resolution: identify the author and
   retain original editor/generator history before editing the WLD.
2. **Was `oldmap8` intended to be loaded, or was it an export, draft, or
   replacement map?** Recommended resolution: locate a dated manifest or
   deployment record; it is absent from both current area manifests and has no
   ZON file.
3. **What is the intended relationship to unregistered `Duris3.wld`, which
   defines the same 10,000 room VNUMs?** Recommended resolution: ask the map
   owner to identify whether these are alternate coordinates, competing
   editions, or unrelated uses of a shared namespace.
4. **Why do the files share 19,759 exact direction/source/target tuples while
   20,159 oldmap8 tuples differ?** Recommended resolution: compare source grid
   transforms and editor history; do not infer a copy from common edges.
5. **Which room map, if either, is authoritative for #180000–#189999?**
   Recommended resolution: require an owner decision before any activation,
   VNUM reassignment, or replacement claim.
6. **Are the 88 one-way directions within oldmap8 intentional terrain gates or
   missing reverse exits?** Recommended resolution: verify each route against
   the original editor map, prioritizing the 32 Darkpeak and 31 Kur Mountain
   cases.
7. **Are the four-direction one-way clusters at Werrun #181362, mount #184184,
   Forestb #187445, foothills #188134, and Bastine Castle #188272 intended?**
   Recommended resolution: review their complete source blocks and neighboring
   map sheets before adding reverses.
8. **Should #181462 north to Werrun #38300 be a one-way approach?**
   Recommended resolution: ask Werrun's owner to confirm the entrance and
   whether a return route belongs in another map.
9. **Are the two Krethik Pass approaches (#182553 east to #20000 and #182654
   north to #20085) both intended?** Recommended resolution: reconcile both
   with Krethik's active entry design and map-editor sources.
10. **Are Mount Banishment #184183 east to #9100 and Yuan Ti2 #187735 west to
    #80600 intentional external entrances?** Recommended resolution: verify
    each with its active destination owner and preserve the source direction
    as a distinct oldmap8 record.
11. **Do Mntcastl #188135 west to #37409 and #188234 north to #37416 belong to
    the same intended connection?** Recommended resolution: confirm both
    destination doors and any return paths with the Mntcastl owner.
12. **Does #188271 east to Bastine #7600 still have an intended counterpart?**
    Recommended resolution: check the Bastine source and route history before
    calling this entrance live.
13. **Are the five south exits to SurfaceKeeps (#120000 and #120009–#120012)
    meant to target the active SurfaceKeeps rooms, Duris3's same-number rooms,
    or both map identities?** Recommended resolution: preserve the exact
    destinations and resolve ownership with both maintainers.
14. **What destinations were intended for the 102 external targets with no
    room definition in another current WLD source?** Recommended resolution:
    locate the missing map sheets or identify retired neighbors; keep all
    source exits until the target owner is known.
15. **Are the 100 targets #199900–#199999 an omitted next map sector?**
    Recommended resolution: inspect neighboring exports and map indexes before
    treating them as intentionally unresolved.
16. **Are the other 300 cross-file target rows to Duris3 meant to be
    cross-map exits?** Recommended resolution: distinguish every source-only
    route from Duris3's own local edges by source file and owner; do not merge
    their graphs on VNUM equality.
17. **Was any reset or population package intentionally stored outside the
    same-stem files?** Recommended resolution: locate the source ZON, MOB, OBJ,
    quest, and shop data; none is present beside oldmap8.wld today.
18. **Do imports, scripts, teleport items, or external map tools use a name or
    coordinates for oldmap8 that the source scan cannot see?** Recommended
    resolution: ask each system owner to check deployment configuration and
    persisted metadata; the current source scan found no direct stem reference
    or active teleport destination in this VNUM range.
19. **Do player or world-persistence records refer to oldmap8 room numbers or
    saved coordinates?** Recommended resolution: check supported storage and
    backup inventories before proposing a path move or archive.
20. **What final disposition should the map owner authorize after provenance
    and route closure?** Recommended resolution: keep RETAIN IN PLACE for now;
    review any later activation, map replacement, merge, renumbering, move, or
    archive as a separate decision.
