# The Mushroom Forest audit supplement

**Checkpoint 297 — all-442 title, active and source-only connectivity, typed references, and direct builder questions.** The [source dossier](../zone-stories/MUSHROOM_FOREST.md) maps all 128 WLD records, 14 local mobiles, seven objects, and 146 zone-reset commands. This supplement records source-backed review without changing areas/ or src/.

## All-442 title and followed history

A normalized, color-code-stripped scan of all 442 current .zon files finds exactly one owner for “The Mushroom Forest”: active mush1. Similar names belong to separate stems: myconid is “Myconid Mushroom Forest” with rooms #2300–#2392, and mushroom_caverns is “the Mushroom Caverns,” zone 241. The exact-title result does not establish a shared story, former name, or owner alias.

Followed mush1 WLD, MOB, OBJ, and ZON paths reach the 2009 areas-tree move. The reviewed history includes 2010 typos, 2012 object/reset-format updates, 2013 area edits, and the 2015 quest-item-flag pass. No alternate-stem predecessor, tracked rename, or explicit whole-zone replacement claim was found in those bounded paths. The current source inventory has no local QST, SHP, or story sidecar file.

## Current map and active boundaries

Mush1 has 128 rooms and 1,001 authored exits: 999 local, one active outbound to UnderDark #856059, and one negative target. The loader resolves 1,000/1,001; #10178 northeast targets -1 and is dropped. The local weak graph is one component. Authored directed travel from #10100 reaches 125/128 when all exits are treated as available, while every room has a directed path back to #10100. The three rooms outside forward traversal are #10224, #10225, and #10227, all named or used as mob-distribution rooms.

The single active boundary is reciprocal: mush1 #10100 east ↔ UnderDark #856059 west ([pair rows 308 and 721](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L308)). The all-95 inactive-WLD sweep found no room overlap and one non-overlapping source-only approach: ud_map2 room #1124761 north to #10100. It is not active ingress and is recorded on existing [ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114). The curated source-only total therefore rises by one, to 3,778 overall, 249 for Surf, 19 for ud_map2, and eight for Underworld2.

No currents or fall records are present. The two reset door commands set the cave entrance closed but unlocked. This source graph does not establish that the exit labeled “Secret Cave” is hidden, that mobs disperse as expected, or that any player route is safe.

## Typed-reference closure and source-only approach

The QST index lists mush1 while no local .qst exists. Bounded exact scans found no external item consumer, shop product, typed ZON reset reference, sidecar item_vnums entry, duplicate foreign prototype, or fixed type-25 target into the forest. No direct compiled room/mobile/object VNUM consumer was identified. The WLD door key fields #5 and #1 have no local prototypes and occur in unrelated object sources; state 1 leaves those doors unlocked.

The source-only ud_map2 approach is distinct from active UnderDark access. It does not add an active exit row or authorize a route. Preserve unregistered ud_map2 in place while its map ownership, editor/tooling, and dependency review remains open.

## Checkpoint 297 all-442 title and direct builder questions

The questions below are based on local source descriptions, resets, and cross-area declarations. Each recommendation keeps the current source as evidence until an owner confirms an intended change.

1. **Is mush1 a separate area from Myconid Mushroom Forest and Mushroom Caverns?** Recommendation: retain all three stems and titles as distinct areas unless the owner confirms a shared history or alias.
2. **Are the 122 identical central-forest descriptions intentional?** Recommendation: keep the repeated setting text unless builders want selected landmark rooms to carry distinct prose.
3. **Is the large river decorative, or should it have water travel or hazard behavior?** Recommendation: document it as scenery until water mechanics are explicitly intended.
4. **Should “easy to get lost” and overgrown paths remain descriptive only?** Recommendation: avoid implying a navigation or lost-state mechanic unless one is separately specified.
5. **Do the three mushroom-stalk landmark placements match the corpse/armor and dragon-remains descriptions?** Recommendation: confirm the exact objects at #10102, #10173, and #10222 before changing their placement.
6. **Should the cave entrance be mechanically secret?** Recommendation: confirm whether to add a secret-exit treatment or keep the present closed, unlocked door and descriptive cave name.
7. **Are key VNUMs 5 and 1 intentional on the reciprocal cave exits?** Recommendation: verify the intended keys and align the WLD fields only after confirming prototypes; current reset state closes but does not lock the doors.
8. **Which dragon does the drow claim to have killed “above”?** Recommendation: identify the intended tale or keep the line as unverified backstory; the local map loads no living dragon.
9. **Is the drow’s magical staff meant to exist as equipment?** Recommendation: reconcile the staff-and-diamond prose with the sword/statue reset group if the staff is a real carried object.
10. **How many undead dragon followers should the reset create, and which items should they carry?** Recommendation: review the two F rows and adjacent E rows as one intended follower group before adjusting them.
11. **Is the Lolth shrine meant to support any actual sacrifice interaction?** Recommendation: keep it as static scenery unless a distinct, explicit ritual mechanic is approved.
12. **Should huge mushroom creature #10113 load into the area?** Recommendation: confirm whether the unreset sentinel is intentionally unused or whether a reset is missing; do not infer a spawn from its prose.
13. **Is missing mobile VNUM #10110 a deliberate retired gap?** Recommendation: preserve the gap unless history or an owner identifies a required prototype.
14. **Are #10224 and #10225 meant to be behind-the-scenes load rooms?** Recommendation: confirm the intended NPC dispersal flow and preserve their one-way relationship to the forest unless player access is desired.
15. **Are 132 M reset rows and their per-command limits intended?** Recommendation: review the two load rooms and caps as population controls; do not treat reset row counts as current stock.
16. **Should room #10227 remain an empty “Disperal Room”?** Recommendation: confirm its maintenance role and correct or retain the title spelling only as an explicit builder choice.
17. **Is the negative northeast exit from #10178 intentional?** Recommendation: replace or remove target -1 only after the intended destination is known; the loader currently drops it.
18. **Is #10100 east to UnderDark #856059 the intended active entrance, with the ud_map2 north approach retained only as source history?** Recommendation: preserve the reciprocal active pair and record ud_map2 as a separate inactive candidate.
19. **Should mush1 stay in areas/qst/AREA with no local QST source?** Recommendation: have the owner retain or remove the index entry after confirming its purpose; do not infer a missing quest from it.
20. **Is this meant to remain an encounter-and-lore zone without a native quest contract?** Recommendation: preserve the current non-quest source behavior unless an owner defines a separate, explicit quest design.
