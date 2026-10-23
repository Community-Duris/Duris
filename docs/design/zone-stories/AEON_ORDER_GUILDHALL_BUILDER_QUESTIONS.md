# Aeon Order Guildhall: builder questions

Canonical key: `component:aeon_gh`. Physical coverage row 353. The
[static audit](../quest-story-followups/AEON_GH_AUDIT_REVIEW.md) records its
unregistered source status, source-only map, imported objects, and shared
VNUMs with the separate `genesis` candidate. These are recommendations for
the source owner; they are not builder answers or live-play qualification.

## Explicit builder questions and recommended resolutions

1. **Which source, if either, is intended to occupy reserved zone #299?**
   `aeon_gh` and `genesis` both use #299 and rooms #29900–#29909, but have
   distinct prose, mobiles, resets, and shops. **Recommend** keep both source
   candidates in place until the owner identifies the intended range owner.
   [Area annotation](../../../areas/AREA#L144), [Aeon candidate](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L2),
   [Genesis candidate](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L46)
2. **Are these two maps meant as separate guilds, alternate drafts, or a
   retired and replacement pair?** Current source titles and descriptions do
   not settle their relationship. **Recommend** preserve them as distinct
   sources until their shared VNUM ownership is confirmed.
   [Identity review](../quest-story-followups/DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-440-aeon-order-guildhall-and-genesis-vnum-collision)
3. **Which room should the compiled Players Guild `inn` handler bind at
   #29903?** Aeon calls it a cozy niche; Genesis calls it a sanctuary.
   **Recommend** confirm the intended room before changing either source or
   code assignment. [Code binding](../../../src/specs/specs.assign.c#L2452),
   [Aeon room](../../../areas/wld/aeon_gh.wld#L62), [Genesis room](../../../areas/wld/genesis.wld#L63)
4. **Are mobile #29900 and #29901 supposed to mean different actors in each
   map?** Aeon defines a mist golem and merchant while Genesis defines
   gargoyles at those numbers. **Recommend** settle source ownership before
   either map is added to the loader.
   [Aeon mobiles](../../../areas/mob/aeon_gh.mob#L1), [Genesis mobiles](../../../areas/mob/genesis.mob#L1)
5. **Should both ZON headers continue to claim zone #299?** The full area list
   currently contains only `*OPEN 299`. **Recommend** reserve the range for one
   confirmed source owner and record that choice in the area-list process.
   [Aeon ZON](../../../areas/zon/aeon_gh.zon#L1), [Genesis ZON](../../../areas/zon/genesis.zon#L1)
6. **What was the intended destination of Aeon's D4 exit from #29900 to
   #153244?** The target exists in `Duris3.wld` as “The Ocean,” but no active
   route is established here. **Recommend** confirm the intended map and return
   path before treating the exit as usable.
   [Aeon exit](../../../areas/wld/aeon_gh.wld#L12), [target room](../../../areas/wld/Duris3.wld#L1118125)
7. **Was the circular indentation in #29906 meant to receive a portal?** The
   room text says it was prepared for one, but no local portal reset or WLD
   destination is present. **Recommend** clarify whether the indentation is
   historical description, unfinished design, or a future encounter.
   [Portal room](../../../areas/wld/aeon_gh.wld#L127), [ZON resets](../../../areas/zon/aeon_gh.zon#L13)
8. **Do the authored exits match the room descriptions?** Room #29901 names
   north, west, and east exits plus a downward staircase; its records point
   east, south, west, and down. **Recommend** reconcile the prose and exit
   directions after the map owner is settled.
   [Aeon room #29901](../../../areas/wld/aeon_gh.wld#L22)
9. **Should four massive mist golems reset in the entrance room?** The mobile
   text describes a motionless but dangerous guard, and the ZON repeats its
   reset four times. **Recommend** confirm the intended count and entrance
   encounter without changing the current source before owner review.
   [Mobile](../../../areas/mob/aeon_gh.mob#L1), [resets](../../../areas/zon/aeon_gh.zon#L13)
10. **What role does the Aeon Order play in the guildhall?** The merchant's
    name supplies a faction but the room descriptions do not explain its
    members or purpose. **Recommend** add only the relationship the owner can
    confirm from the intended setting.
    [Merchant](../../../areas/mob/aeon_gh.mob#L17)
11. **Who are “your people” in the shop refusal?** The refusal text does not
    identify a race, guild, or other eligibility rule. **Recommend** preserve
    the line and document the intended buyer condition before attributing one.
    [Shop record](../../../areas/shp/aeon_gh.shp#L1)
12. **Is object #6089 intended stock and equipment?** The Aeon ZON and SHP
    refer to it, but no current OBJ file declares #6089. **Recommend** confirm
    its owner or missing-source status before changing the references.
    [Reset](../../../areas/zon/aeon_gh.zon#L22), [shop](../../../areas/shp/aeon_gh.shp#L1)
13. **Which #53, #108, and #109 definitions should resolve for Aeon stock?**
    Those VNUMs have multiple current OBJ definitions. **Recommend** identify
    the intended prototypes and loader ownership before relying on their names
    or behavior.
    [Audit ownership scan](../quest-story-followups/AEON_GH_AUDIT_REVIEW.md#typed-reset-shop-and-code-references)
14. **Are the three imported Player Castles objects deliberate in these
    rooms?** Aeon places #11057, #11107, and #11013 from that source, and
    Genesis also imports #11013. **Recommend** preserve these cross-area
    references until their distinct intended placements are confirmed.
    [Aeon resets](../../../areas/zon/aeon_gh.zon#L9), [Player Castles objects](../../../areas/obj/player_castles.obj#L121),
    [Genesis reset](../../../areas/zon/genesis.zon#L11)
15. **Should the merchant receive all fifteen `G` objects on reset?** The
    ZON equipment set matches the shop's fifteen products. **Recommend** keep
    the current set intact until imported item identity and shop behavior are
    verified by the owning builder.
    [Resets](../../../areas/zon/aeon_gh.zon#L19), [shop](../../../areas/shp/aeon_gh.shp#L1)
16. **Should one of the conflicting maps receive a new room and mobile range?**
    Remapping either source could also affect the compiled #29903 inn binding
    and other references. **Recommend** decide ownership and reference closure
    before renumbering either map.
    [Static audit](../quest-story-followups/AEON_GH_AUDIT_REVIEW.md#vnum-collision-title-and-history)
17. **Was a local quest file or story sidecar ever intended?** Neither exists
    for the current Aeon component. **Recommend** keep that absence explicit
    until a builder identifies a missing quest or authored sidecar.
    [Source inventory](../quest-story-followups/AEON_GH_AUDIT_REVIEW.md#source-inventory-and-area-list-status)
18. **What is the intended purpose of the small bluestone storage room and
    larger cavernous storeroom?** Their descriptions show storage but assign
    no quest or service. **Recommend** preserve them as descriptive spaces
    unless a builder confirms a player interaction.
    [Small storage room](../../../areas/wld/aeon_gh.wld#L140), [large storeroom](../../../areas/wld/aeon_gh.wld#L179)
19. **Should the source prose receive an editorial pass after identity is
    settled?** The descriptions contain apparent spelling and grammar errors.
    **Recommend** keep any later wording changes separate from unresolved map,
    faction, and portal decisions.
    [Aeon descriptions](../../../areas/wld/aeon_gh.wld#L1)
20. **Should Aeon or Genesis ever be added to the full area list?** Both are
    current source sets, yet neither is a full `areas/AREA` member.
    **Recommend** do not activate either until the range, handler, imported
    item identities, and migration/editor dependencies have an owner-approved
    resolution. [Area list](../../../areas/AREA#L144),
    [retention review](../quest-story-followups/ATTIC_MIGRATION_PLAN.md#checkpoint-440-aeon-and-genesis-retain-their-current-sources)

This is a source-backed recommendation review. It changes no area source,
reset, shop behavior, portal, or player-facing text.
