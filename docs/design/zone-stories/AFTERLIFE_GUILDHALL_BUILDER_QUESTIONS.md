# The Afterlife Guildhall: builder questions

Canonical key: `component:afterlife_gh`. Physical coverage row 354. The
[story dossier](THE_AFTERLIFE_GUILDHALL.md) and [static audit](../quest-story-followups/AFTERLIFE_GH_AUDIT_REVIEW.md)
record a source-only household story, a lore QST, an editor map, and ten room
VNUMs that overlap active Torg. These recommendations are not builder answers
or live-play qualification.

## Explicit builder questions and recommended resolutions

1. **Which source should own rooms #29100–#29109?** Active `torg.wld` and
   unregistered `afterlife_gh.wld` assign distinct locations to all ten same
   VNUMs. **Recommend** confirm numeric ownership before either source changes
   or moves. [Afterlife/Torg comparison](../quest-story-followups/AFTERLIFE_GH_AUDIT_REVIEW.md#authored-map-and-torg-vnum-overlap)
2. **Are the Afterlife rooms meant to be alternate drafts of Torg rooms or a
   separate map using a colliding range?** Their titles and descriptions
   differ throughout. **Recommend** preserve them as separate identities until
   the owner resolves the shared numbers.
   [Torg overlap audit](../quest-story-followups/TORG_AUDIT_REVIEW.md#inactive-wld-sources-and-the-surf-candidate)
3. **What should the compiled Players Guild `inn` handler serve at #29103?**
   It binds to Torg's escape tunnel under the current full area list, while
   this source calls #29103 the basement entrance. **Recommend** confirm the
   intended room owner before altering code or source numbers.
   [Code assignment](../../../src/specs/specs.assign.c#L2448), [Afterlife room](../../../areas/wld/afterlife_gh.wld#L91),
   [Torg room](../../../areas/wld/torg.wld#L4373)
4. **Should the Afterlife WLD and QST remain outside their full area lists?**
   The source map and Suse dialogue exist but neither `areas/AREA` nor
   `areas/qst/AREA` lists them. **Recommend** keep both index decisions tied to
   confirmed VNUM and quest ownership.
   [Area index](../../../areas/AREA#L140), [QST index](../../../areas/qst/AREA#L130)
5. **Is Suse intended as a lore guide without a quest objective?** Her QST
   contains only message (`M`) responses and no `Q`/`QA` contract.
   **Recommend** preserve it as dialogue unless an owner defines a specific
   state change or reward.
   [Suse QST](../../../areas/qst/afterlife_gh.qst#L1)
6. **Is “a few months ago” still the intended house-building chronology?**
   Suse's line gives relative timing but no fixed date. **Recommend** confirm
   the intended chronology or make it independent of elapsed repository time.
   [House response](../../../areas/qst/afterlife_gh.qst#L9)
7. **Should the house's reincarnation belief describe a game mechanic or only
   members' theology?** The QST states a belief but gives no return procedure.
   **Recommend** preserve the lore as belief until an actual implementation
   and eligibility rule are confirmed.
   [Afterlife response](../../../areas/qst/afterlife_gh.qst#L28)
8. **How should the universal altar relate to members' different gods?** The
   room and dialogue invite all religions but do not define shared rituals.
   **Recommend** leave the altar non-exclusive unless the owner specifies its
   intended ritual use.
   [Prayer room](../../../areas/wld/afterlife_gh.wld#L182)
9. **Is Suse's appearance and greeting the intended way to introduce the
   house?** The sources describe her as half-naked and have several characters
   direct visitors to her. **Recommend** confirm the intended characterization
   and audience before revising that text.
   [Suse mobile](../../../areas/mob/afterlife_gh.mob#L16)
10. **What is the handsome human's guard role beyond directing young women to
    Suse?** His mobile text combines door security with flirtatious direction.
    **Recommend** clarify whether both parts are intentional and preserve any
    access condition separately.
    [Human mobile](../../../areas/mob/afterlife_gh.mob#L33)
11. **Should both golems direct visitors to Suse rather than guard or block
    the entrance?** Their descriptions and ZON placements put them at the
    outside room. **Recommend** keep their current source role open until the
    intended entry behavior is confirmed.
    [Golems](../../../areas/mob/afterlife_gh.mob#L54), [placements](../../../areas/zon/afterlife_gh.zon#L18)
12. **Are four different mobiles intended to reset at the same entrance room?**
    The ZON places the girl, handsome human, and two golems at #29100, while
    the merchant is in #29107. **Recommend** confirm those placements without
    inferring live availability from source counts.
    [Mobile resets](../../../areas/zon/afterlife_gh.zon#L18)
13. **Is the room #29105 chair prepared for a cow part of an established
    character story?** The text ties it to food stains and a successful
    resurrection but names no cow. **Recommend** leave the allusion unexplained
    unless its owner supplies context.
    [Dining room](../../../areas/wld/afterlife_gh.wld#L133)
14. **Are #29120, #29124, and #29126 the intended training-room equipment?**
    The Afterlife reset imports them from candidate `afterlife_op2`.
    **Recommend** preserve that owner relationship and confirm each object's
    intended training use before copying or moving the prototypes.
    [Resets](../../../areas/zon/afterlife_gh.zon#L14), [source objects](../../../areas/obj/afterlife_op2.obj#L1)
15. **Are the four Player Castles objects intentional placements in this
    house?** The ZON imports #11053, #11005, #11103, and #11013.
    **Recommend** keep Player Castles as prototype owner until each placement
    and purpose is confirmed.
    [Imported resets](../../../areas/zon/afterlife_gh.zon#L9), [Player Castles prototypes](../../../areas/obj/player_castles.obj#L41)
16. **What was the intended exterior destination of the upward exit from
    #29100 to #123813?** Its target record in `Duris3.wld` is “Rolling Hills.”
    **Recommend** confirm the target map and return route before treating the
    authored edge as current access.
    [Exit](../../../areas/wld/afterlife_gh.wld#L33), [target](../../../areas/wld/Duris3.wld#L500074)
17. **Should the map editor positions continue to use this unregistered
    `zoneId`?** The JSON maps all ten room VNUMs. **Recommend** retain the file
    with this source until its final VNUM owner is decided.
    [Editor map](../../../areas/map/afterlife_gh.json)
18. **How do the Guildhall and the two “Afterlife GC OP” sources fit together?**
    This ZON imports objects from `afterlife_op` and `afterlife_op2`, each an
    existing candidate with separate room ranges. **Recommend** preserve those
    source owners and avoid assuming one map replaces another.
    [Identity review](../quest-story-followups/DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-441-afterlife-guildhall-and-active-torg-identity)
19. **What does the shop mean by “your people,” and is #6089 valid stock?**
    The refusal supplies no eligibility rule, and no current OBJ declares
    #6089. **Recommend** keep the refusal and item reference documented without
    guessing a faction or prototype owner.
    [Shop](../../../areas/shp/afterlife_gh.shp#L1), [audit](../quest-story-followups/AFTERLIFE_GH_AUDIT_REVIEW.md#typed-reset-qst-shop-and-code-references)
20. **Should the repeated spelling variants and apparent typos receive an
    editorial pass?** The WLD and QST use “AftwerlLife” for the group and also
    contain other grammar and spelling variants. **Recommend** confirm the
    canonical name and separate copy editing from story and VNUM decisions.
    [Guildhall title](../../../areas/zon/afterlife_gh.zon#L2), [QST dialogue](../../../areas/qst/afterlife_gh.qst#L6)

This is a source-backed recommendation review. It changes no area source,
quest dialogue, mobile, reset, shop, or player-facing text.
