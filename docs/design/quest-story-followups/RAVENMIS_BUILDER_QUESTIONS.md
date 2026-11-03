# ravenmis builder questions and recommended resolutions

Checkpoint 522, source snapshot
`c500ac51b8cd26540943257e7ca4e9807263ef74`. These questions cover the
unlisted five-file Ravenmist Guildhall source group and its colliding VNUMs.
Questions 21–22 were added at Checkpoint 926 after a typed scan of external
Newbie, guildhall, quest, shop, reset, and portal references. These questions
do not authorize loading, renumbering, deleting, moving, or archiving the
source group.

1. **Who owns the Ravenmis files and their editor/build/import process?**
   Recommended resolution: identify the map owner and the source that creates
   any deployment aggregate before changing manifest membership.
2. **Should this WLD/MOB/OBJ/ZON/SHP group be listed in areas/AREA?**
   Recommended resolution: have the owner document intended activation and
   loader membership; it is currently absent.
3. **Should Ravenmis have a QST/AREA entry or a story/map sidecar?**
   Recommended resolution: confirm whether the five current source files are
   intentionally complete despite having no QST, map JSON, or story sidecar.
4. **Is “ravenmist guildhall” the canonical title, and what guild or builder
   does it identify?** Recommended resolution: record the owner-confirmed
   name and any documented relation to other guildhalls or Ravenloft.
5. **Is the up exit from Ravenmis #29300 to unlisted Duris3 room #125119
   intended?** Recommended resolution: confirm both endpoint owners and the
   intended route/reverse route before either source is activated.
6. **Who owns room VNUMs #29300–#29309 across Ravenmis, active Newbie, and
   cbr_hall/loc_gh/eskull_hall?** Recommended resolution: keep the four map
   identities separate unless their editor lineage establishes otherwise.
7. **Are all ten Ravenmis room descriptions and exits still current?**
   Recommended resolution: ask the map owner to confirm the rainforest fog,
   cavern branches, cave-in, niche, Iatyh inscription, and Xanan graffiti.
8. **Should the raw D9/D6 pair between rooms #29307 and #29308 be retained?**
   Recommended resolution: have the editor owner confirm those raw direction
   codes and intended navigation before normalizing them.
9. **Which source owns mobile #29300: the Ravenmis hermit or active Newbie’s
   mercenary?** Recommended resolution: document separate identities or
   approve a single-owner VNUM plan; do not infer that one replaces the other.
10. **Are Ravenmis vapor figures #29301–#29304 intended to share active
    Newbie and other guildhall mobile numbers?** Recommended resolution:
    resolve all prototype owners. Active Newbie uses #29300–#29304 for a
    mercenary, assassin, alchemist, bleeding man, and half-elf; compiled
    bindings assign #29303/#29304 to Ailvio callbacks. Unlisted cbr_hall,
    eskull_hall, and loc_gh resets reuse some of the same mobile IDs for
    golems, residents, and a shopkeeper. Confirm the intended owners and
    aggregate precedence before any activation or renumber.
11. **Are Ravenmis objects #29300/#29301 intended to share numbers with
    Newbie’s Pain dagger and sign and the unlisted guildhall portal/backpack
    objects?** Recommended resolution: confirm the well and iron-ration
    identities against each OBJ owner and the aggregate’s chosen VNUM owner.
12. **Are Player Castles objects #11054, #11104, and #11013 intentional
    Ravenmis room decorations?** Recommended resolution: confirm each source-
    only reset placement with both map owners.
13. **What object is reset as #29311 into Ravenmis room #29307?**
    Recommended resolution: resolve the missing local prototype against the
    active Newbie arrow and unlisted Ethdawn portal before changing the row.
14. **Is #29301 intended as the shop’s iron-ration stock item?**
    Recommended resolution: confirm the source identity and how shop loading
    should distinguish it from active Newbie’s same-number sign.
15. **Where is the intended #6098 shop/reset item prototype?**
    Recommended resolution: recover and identify the OBJ source or confirm
    that the SHP and ZON rows should remain unresolved.
16. **Are the Heavens products (#53, #108, #109, #111, #347, #364, #377,
    #398, #412) and Verz1 #3050 intentional Ravenmis stock?** Recommended
    resolution: confirm every cross-area product row with its current owner.
17. **Should shop record #29300 use keeper #29300 in room #29309?**
    Recommended resolution: resolve the intended hermit and room identities
    against Newbie’s same-number prototypes. Newbie SHP #29283 is a separate
    source record in room #29305 with products #29305, #29306, and #29307;
    cbr_hall, eskull_hall, and loc_gh also have unlisted shop records using
    Ravenmis-range keeper or room numbers. Confirm aggregate precedence before
    treating any of these rows as the Ravenmis shop.
18. **Should `ravenmis.shp` enter the fallback shop stream when
    areas/world.shp is missing?** Recommended resolution: document the intended
    aggregate and fallback policy; current parser exposure is conditional.
19. **Do editor files, external import scripts, deployed aggregates, or
    persisted references depend on these room/mobile/object/shop IDs?**
    Recommended resolution: ask the map and operations owners to inspect
    those stores; a source scan cannot establish that state.
20. **What disposition should the owner authorize after these decisions?**
    Recommended resolution: retain the five files in place until identity,
    route, typed dependencies, loader, and external-state questions are closed;
    make any later activation, renumber, move, or archive a separate decision.
21. **Should Newbie QST and story entries keep #29300/#29302 as item rewards
    and #29304 as a mobile?** Recommended resolution: confirm the active
    Newbie Pain dagger #29300, healing emblem #29302, and half-elf #29304
    identities with both area owners. Ravenmis uses #29300 for a well, #29302
    for a room, and #29304 for a vapor figure. Also confirm the aggregate path:
    the loader reads `areas/world.qst`, which is absent in this checkout.
22. **Do the six ITEM_TELEPORT objects that target #29305/#29307/#29308/#29309
    intentionally lead to the active Newbie map?** Recommended resolution:
    confirm both Newbie portal objects and the four unlisted Eskull Hall,
    Ethdawn OP, and Loc GH portal sources with their owners. All destinations
    resolve to Newbie rooms in the current WLD table; they do not establish
    inbound routes to the unlisted Ravenmis map.
