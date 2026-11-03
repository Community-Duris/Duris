# rf_gh builder questions and recommended resolutions

Checkpoints 526 and 930, source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74.
These questions concern the unlisted five-file Rotting Flesh guildhall source
set and its collisions with listed Torg. They do not authorize activation,
renumbering, deletion, merging, moving, or archiving.

## Explicit builder questions and recommended resolutions

1. **Who owns the rf_gh WLD, MOB, OBJ, ZON, and SHP files and their export
   process?** Recommended resolution: identify the authoring owner and
   determine whether the files are independent, generated, or supplied by a
   deployment-only step.
2. **Why do rf_gh rooms #28900–#28912 reuse every same-type room VNUM in
   listed Torg?** Recommended resolution: ask both map owners to establish
   intended identity and namespace history before changing any manifest.
3. **Why do all seven rf_gh mobile IDs #28900–#28906 also identify different
   Torg mobiles?** Recommended resolution: preserve both source definitions
   until the mobile owner confirms which IDs may enter a common table.
4. **Why do rf_gh objects #28900–#28904 collide with different Torg
   objects?** Recommended resolution: resolve each object identity,
   especially #28902, before loading either per-stem OBJ source.
5. **Should rf_gh be listed in areas/AREA or areas/qst/AREA?** Recommended
   resolution: keep current membership unchanged until the WLD/MOB/OBJ
   collisions and all loader inputs have a documented owner decision.
6. **Is “The GuildHall of Circle of Yggdrasil” the canonical title or only a
   historical zone header?** Recommended resolution: ask the area owner to
   record a canonical title and aliases without deriving replacement from
   the file stem.
7. **Was the three-room cathedral fragment intentionally reused in rf_op as
   a separate map component?** Recommended resolution: have both map owners
   confirm the reuse and intended relationship; preserve each source until
   they do.
8. **Should the D4 exit from #28900 target listed Tharnrifts #111082, the
   duplicate Duris3 #111082, or another destination?** Recommended
   resolution: confirm target identity with the map owners and retain the
   current one-way source edge until resolved.
9. **Why does #111082 also identify “The Ocean” in unlisted Duris3.wld?**
   Recommended resolution: establish which historical file owns the target
   number and whether the Duris3 copy is obsolete or separately maintained.
10. **Should the D4 outlet from cathedral room #28910 to Duris3 #154153 be
    retained, redirected, or removed?** Recommended resolution: locate the
    owner-approved destination and return-path plan before changing the exit.
11. **Are the two external outlets intentionally one-way?** Recommended
    resolution: ask the mapper to document expected travel in both
    directions; current target sources contain no return exit to rf_gh.
12. **Was the reciprocal surface-to-Torg route through #28900 ever intended
    to reach the rf_gh Organ Pit that reuses this VNUM?** Recommended
    resolution: treat the current surface/Torg pair as Torg-owned under the
    active manifest; let both map owners resolve the collision before
    proposing any route to rf_gh.
13. **Does unlisted surf.wld's #277211-to-#28900 edge belong to an obsolete
    map or a separate deployment?** Recommended resolution: identify its
    loader and map owner before including it in connectivity claims.
14. **Are the five source O resets to Player Castles #11058, #11013, and
    #11108 intentional?** Recommended resolution: preserve both #11058 rows
    and both #11108 rows exactly while the two area owners confirm ownership.
15. **Which #53 object should the G reset and shop use: active Heavens “rope
    coil hemp” or unlisted Dalvik “dazzling sapphire bracelet”?** Recommended
    resolution: confirm the intended typed object before resolving the
    source-only row through an active object table.
16. **Is local object #28902, the Rotting Flesh sigil, meant to be in shop
    #28904's product list?** Recommended resolution: confirm the exact
    product identity against Torg OBJ #28902, the hand axe, before loading
    the shop fragment.
17. **Should shop #28904's keeper #28904 remain paired with rf_gh's vampire
    prototype?** Recommended resolution: resolve it with the mobile and shop
    owners because the listed same-number Torg mobile is a duergar miner.
18. **Is shop #28904's in_room #28909 intended to be the candidate Bone Hut?**
    Recommended resolution: confirm room identity because listed Torg owns
    #28909 as a corridor and shop lookup also uses its room VNUM.
19. **Should the full-mode shop fallback read rf_gh.shp while
    areas/world.shp is absent?** Recommended resolution: document the
    intended shop catalog inputs and deployment mode before changing the
    fallback or the source file; this checkout did not boot it.
20. **Does any editor export, generated aggregate, saved state, or deployment
    data document a retirement or renumbering decision?** Recommended
    resolution: recover that provenance and confirm persisted references
    before proposing activation, renumbering, move, merge, or archive work.

21. **Are ITEM_TELEPORT objects #28903 and #28904 intended as reciprocal
    destinations between the #28905 and #28912 rooms in the two disconnected
    WLD groups?** Recommended resolution: preserve the type-qualified object,
    placement, and room values while the map/object owners confirm the portal
    pair and loader/reset contract; do not count the object values as WLD
    exits or resolve the numbers to Torg without an explicit identity decision.

See the [rf_gh static audit](RF_GH_AUDIT_REVIEW.md#checkpoint-930-reciprocal-portal-objects-cross-the-two-wld-groups).
