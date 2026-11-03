# Checkpoint 367: Vast Hidden Grove audit

Physical coverage line 280, `area:solonar`, is reconciled with the existing [source-comprehensive story map](../zone-stories/VAST_HIDDEN_GROVE.md). That map remains the detailed account of the fifteen native contracts, ingredient quantities, seven shops, scenery travel properties, and shared source defects. This supplement closes static connectivity, typed-dependency, title/history, and builder-question review. It does not qualify installed gameplay or change the documented source behavior.

## Active graph and map identity

The active [Solonar WLD](../../../areas/wld/solonar.wld) contains 100 rooms (#30600–#30699) and 182 authored directions. All 182 targets resolve locally; there are no active WLD directions into or out of the area. The local weak graph has seven components of 33, 26, 20, 10, 5, 3, and 3 rooms. The target WLD parses cleanly. A broad active-source scan found one unrelated malformed block in `northern_wilderness.wld` near line 1232, visibly targeting #12270.

All 95 inactive WLD files parsed 527,791 rooms and 2,080,773 directions without errors. None reuses a Solonar room VNUM or directs an exit into #30600–#30699. No candidate WLD approach is added. Scenery travel properties form separate routes and are not represented by these WLD counts.

The area has five rooms with fall chances and no current markers. Preserve those chance values as source evidence; the high branch values are `chance_fall` settings, not climbing-skill requirements. The existing story map records the seven components, locked and hidden faces, valid local scenery destinations, and Striker's authored open exit.

## Typed dependencies and inventory correction

The all-source same-type scans found no active or source-only WLD, MOB, or OBJ file reusing a Solonar ID. The VNUM ranges are dense but remain type-specific; interpret #306xx by file type and command role.

An active all-world scan of 713 type-25 object prototypes found one foreign fixed destination into Solonar: [Elvish object #35804](../../../areas/obj/elvish.obj#L63), placed at Homestead room #35810 by [elvish.zon line 90](../../../areas/zon/elvish.zon#L90), targets room #30648. Its paired local travel property is [Solonar object #30604](../../../areas/obj/solonar.obj#L58), reset at room #30648 by [solonar.zon line 65](../../../areas/zon/solonar.zon#L65), with destination #35810. This is an item-based route in both directions, separate from WLD connectivity. The source map documents twelve other local scenery travel properties and validates their destination VNUMs.

One active foreign reset references a Solonar mobile: [Alatorin ZON line 4398](../../../areas/zon/alatorin.zon#L4398) loads mobile #30611 at room #83958. The reset has no following filament G command; the repeated cave fisher mobile does not add another local filament source. No active foreign QST item command, typed sidecar item field, or SHP product listing references a Solonar object. The local QST and story sidecar likewise have no typed outbound item reference, and the local reset file contains no outside room, mobile, or object role.

The [local SHP source](../../../areas/shp/solonar.shp#L1) contains seven shop records, #30600–#30606. The coverage inventory previously reported no shops; its range and count are corrected. Shop source existence remains distinct from live buy/sell qualification. The story map documents the current shared accounting restriction.

A direct numeric scan of maintained C/C++ found no literal #30600–#30699 VNUM reference. The reproducible [source index](../../reference/zone-story-audits/solonar.md) separately records `specs.assign.c`'s candidate inn room #30511 inside the derived range #30507–#30699. No active WLD defines #30511; the existing [assignment](../../../src/specs/specs.assign.c#L2450) is unguarded, and a missing room maps through `real_room0` to room-index zero. Keep this as a checked-target repair decision; do not infer a local inn from the derived range.

## Title, same-type identity, and followed history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “Vast Hidden Grove” occurs once among all 442 current ZON titles and once among the 351 active contributors. No active or source-only WLD, MOB, or OBJ file duplicates a Solonar ID in the same type namespace.

`git log --follow` for the WLD, MOB, OBJ, ZON, QST, and SHP paths reaches the 2009-08-12 areas-tree move. Latest tracked source edits are WLD and MOB in 2014, OBJ and ZON in 2015, QST in 2013, and SHP in 2014. The story sidecar's tracked history is from 2026-10-02. No alternate-stem rename, distinct predecessor, or whole-zone replacement appears in the followed paths or current contributor list. Keep active `solonar` DO NOT MOVE.

## Builder questions and conservative recommendations

These twenty questions preserve explicit choices while keeping current source identities, item quantities, and receipts intact.

1. Should active `solonar` remain in place? Keep it DO NOT MOVE; its source map, QST, reset, shop, and scenery records share this contributor identity.
2. Should the candidate `inn` assignment at #30511 become a local inn? Validate a real room and add a missing-room guard and diagnostic first; do not create or select a room from the derived span.
3. Should the seven WLD components remain separate source pockets? Preserve the authored graph and scenery routes until a builder specifies a connector; do not infer that numeric adjacency creates an exit.
4. How should Valin and the miner at #30607 remain findable? Preserve their exact room, cap, and `ACT_STAY_ZONE` facts; reproduce wandering and holding-room episodes before promising a reliable encounter route.
5. Should the five high-branch fall chances remain 35/40/45/50/55? Keep them as fall probabilities and qualify actual hazard outcomes before attaching skill or travel achievements.
6. Should the Point's #30622/#30629 secret, locked door and speech-unlock behavior remain? Preserve both reset faces, key and keywords; do not add an item or key quest without a route decision.
7. Should Striker's #30686 downward exit remain open even though its text describes a vanished wall? Preserve the loaded exit state until the builder chooses between prose clarification and an actual gate.
8. Should the centaur passage #30651 north to #30699 remain discoverable and unlocked? Keep the current negative-key/reset fields; shared speech does not require a password for an unlocked direction.
9. How should the three mixed-fee exchanges settle? Add exact item and identified-wallet allocation, reward preflight, and replay-safe settlement before exposing fee completion.
10. Should fresh reset-source generation remain blocked by active accounting? Keep the guard; complete the durable generation/recovery path before qualifying new stock.
11. How should shared ingredients serve competing recipes? Preserve exact copies and distinct item identities; test source cap, availability, consumption, two-scroll recovery, and replacement instead of inferring supply from receipts.
12. Should the dead mouse remain impossible to take through ordinary pickup? Qualify the documented ITEM_TAKE refusal and choose an explicit recoverable source or transfer before treating it as a reliable request item.
13. Should Valin's Mage Bane exchange keep the small pickaxe requirement? Preserve the exact three-item Q even when the dialogue emphasizes only the diamond and heart.
14. Should the hawk and swallow continue to disappear after their separate requests? Preserve their independent Q receipts and removal commands; decide episode/reset availability before treating the requests as repeatable.
15. Should the Homestead/Solonar statues remain a paired travel route? Preserve Elvish object #35804 to #30648 and local object #30604 back to #35810; track confirmed arrival independently of WLD movement.
16. Should the other twelve Solonar scenery properties remain exactly as authored? Keep each object-and-room-specific destination; qualify destination validation and confirmed movement before awarding any arrival outcome.
17. Should Alatorin continue resetting mobile #30611? Preserve the typed mobile reference and inspect its actual reset ancestry separately; do not count it as a second local filament G source.
18. How should seven local shops be described while buy/sell remains unqualified? Keep the corrected #30600–#30606 source inventory and avoid presenting source presence as live stock or a story achievement.
19. Should current lore about the lost tablet, curse, nymph departure, Striker's wall, and living-heart transformation receive separate terminal effects? Keep those narrative conclusions distinct from native Q completion until accepted effects are specified.
20. What evidence should scenery travel record? Preserve the existing property commands; award only confirmed destination arrival and keep attempted commands, text, possession, and failed movement separate.

## Register updates

Coverage row 280 now records the seven local shop IDs and links this supplement. The WLD and title/history reviews close without adding candidate approaches. Typed closure records the Elvish fixed item route and Alatorin's mobile reset reuse; no foreign QST, sidecar item, shop-product, or local reset-role link was found. The curated source-only WLD total remains 4,849, including 283 Surf records.
