# Winterhaven builder questions

Canonical key: `area:wh`. Physical coverage row 345. The
[source-comprehensive dossier](WINTERHAVEN.md) and
[bounded audit supplement](../quest-story-followups/WINTERHAVEN_AUDIT_REVIEW.md)
document the native receipts, custom procedures, graph, and typed dependencies.
These questions collect remaining content and ownership decisions without
changing world or server source.

## Explicit builder questions and recommended resolutions

1. **Which of the 221 native Q/QA contracts should be presented as stories?**
   There are 135 request rows, 84 service rows, and two exclusions.
   **Recommend** preserving those categories and existing identities; qualify
   a service separately from a story receipt.
2. **Should the 49 item-plus-coin exchanges remain unavailable until payment
   accounting supports them?** The source set includes 22 tannings, 18 clothing
   recipes, eight silk garments, and one final earring. **Recommend** requiring
   one durable transaction for exact inputs, fee, reward, and receipt before
   exposing mixed-payment availability.
3. **What decay policy should apply to the 21 fresh-animal objects?** Only
   #55500-#55520 have assigned decay replacements; births, ordinary corpses,
   pregnant-puffadder, and some worn placements differ. **Recommend** retaining
   the per-object rules and not deriving a universal timer from duration fields.
4. **How should each of the three dragon-heart lifecycles recover from a
   missing replacement object?** Tiamat, Bahamut, and Dragonnia create separately
   timed hearts, while the reviewed decay failure can leave a retry counter
   below zero. **Recommend** recording each birth, custody, deadline, and
   replacement independently and asking for a retry policy before claiming
   recovery is reliable.
5. **Are four baby-centaur exchanges intended to be the repeatable source of
   four chief keys?** A key breaks after one unlock, and four shaman doors have
   separate locks; the journal's four-baby check is optional. **Recommend** keep
   the optional preparation distinct from an enforced quest prerequisite and
   confirm the intended key renewal path.
6. **Should the snow-ogre shaman chain remain gated by the current painting,
   reset, and one-use-key sequence?** Shaman reset chance is 25% with live cap
   one, and the chief's locket exchange removes him. **Recommend** preserving
   the exact sequence and no-fee Q recipes; qualify availability and replacement
   keys before promising repeatable access.
7. **What is the intended endpoint for the thief sequence's mark of Death?**
   The mark received from three spirits differs from the mark carried by
   Frenbar. **Recommend** keep the two exact items and their rewards separate
   unless the builder confirms a shared identity.
8. **Should activating Aevenyl's book switch count as discovering the hidden
   passage?** Shared switch behavior clears `EX_BLOCKED`, while the exit also
   has `EX_SECRET`. **Recommend** represent switch activation, revealing/finding
   the exit, and entering it as separate states.
9. **Is Chibbleniffle's Winterhaven roof route retired, incomplete, or intended
   to be restored?** The roof and chest exist, but no ordinary incoming exit,
   maintained literal route, or confirmed source for key #55327 was found;
   Evermeet remains a placement lead. **Recommend** decide the route and key
   owner before changing the map, and do not infer a new upward exit.
10. **How should the forty-nine ambassador-memory rewards be qualified when
    placement moves?** Forty-eight memories have active reset sources, while
    Torg's ambassador #55274 lacks a normal M/F reset in the reviewed source.
    **Recommend** keep receipt ownership with Winterhaven, name the actual
    foreign carrier/reset owner, and avoid promising availability from source
    absence or a wandering location.
11. **Which authored acquisition paths should support Sootfoot's distant
    artifacts?** Several exact recipe inputs have no confirmed active static
    producer in the reviewed source paths. **Recommend** preserve the exact
    contracts and require source evidence or an explicit builder decision
    before adding supply claims.
12. **How should repeated names and random artifact outcomes preserve exact
    identity?** The dossier distinguishes same-name mists, marks, colors, and
    outcomes, including two copies required for some recipes. **Recommend** use
    exact VNUM and item-instance lineage; do not collapse same-name objects or
    count one consumed item twice.
13. **Should Lancer's gem-studded-hide gift contract be held until publication
    handles a null creation result?** The reviewed handler retires the input
    before publishing a result that can be null. **Recommend** resolve output
    creation, input retirement, prices, and retry atomically before recording a
    successful gift.
14. **Should the random scroll exchanges be made deterministic per accepted
    input?** Their handlers select by generic keywords, retire inputs, and
    publish random outcomes. **Recommend** verify the exact selected item and
    preserve a recoverable outcome before marking a scroll exchange complete.
15. **Which prose discrepancies are intentional?** Examples include Volo's
    three-cloak dialogue versus two-cloak QA, Lancer's wand/mask wording versus
    egg/blueprint rewards, Davril's named helmet metal, and Incarnate two-drop
    prose versus one created drop. **Recommend** confirm each exact contract
    first and edit only the mismatched text or handler, not valid alternatives.
16. **Which advertised item powers should actually be active?** Some powers are
    commented or unassigned, and the reviewed Living Legend combat-periodic
    branch is unreachable after an earlier return. **Recommend** decide intended
    effects separately from quest completion and preserve receipts that do not
    claim an unexecuted effect.
17. **Which source should own the imported objects used by Winterhaven resets?**
    The bounded scan finds 29 nonlocal object references; #53 and #100 have
    multiple definitions, and #6067 has none in the current object tree.
    **Recommend** confirm exact loader/owner intent and repair only after that
    identity is established.
18. **Are the current one-way exits and separate map components intentional?**
    The open-edge graph reaches 553 rooms from the five active arrivals, and 12
    rooms cannot reach an outbound exit. **Recommend** qualify intended player
    access and safe return for each route before changing exits or claiming full
    traversal.
19. **Should Winterhaven's 219 story entries remain one sidecar?** Revision 2
    is already close to the original 256 KiB limit; the source plan raises the
    sidecar bound to 512 KiB. **Recommend** retain existing IDs and decide
    whether future authored guidance should be divided by district/family while
    preserving stable bindings.
20. **Which support services are meant to remain outside story completion?**
    The dossier records explicit refusals for cleric, pet, and gambling
    accounting, plus separate bank, board, storage, and world-quest paths.
    **Recommend** preserve those boundaries unless an owner specifies a
    separately qualified integration.
