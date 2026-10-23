# The Mushroom Caverns audit supplement

**Checkpoint 298 — all-442 title, bounded history, typed-reference closure, source-only approach scan, and direct builder questions.** The [existing source dossier](../zone-stories/MUSHROOM_CAVERNS.md) already maps the 132 WLD records, eight local mobiles, nine objects, 54 resets, and the legacy and shared Underdark quest material. This supplement records the repository-wide reconciliation without changing areas/ or src/.

## All-442 title and followed history

A case- and whitespace-normalized scan after removing color codes from all 442 current .zon titles finds one exact owner for “The Mushroom Caverns”: active mushroom_caverns ([zone title](../../../areas/zon/mushroom_caverns.zon#L2)). The similarly named mush1 title is “The Mushroom Forest”; myconid is “Myconid Mushroom Forest.” These are separate current titles and stems.

Followed same-stem WLD history reaches the 2009 areas/ tree move and the 2010 edit; MOB, OBJ, and ZON histories reach the same 2009 move and include reviewed edits through 2015. The mushroom_caverns story sidecar was added on October 4, 2026. There is no same-stem QST or SHP file. The QST area index still lists mushroom_caverns ([QST/AREA entry](../../../areas/qst/AREA#L116)), while the three giver contracts are declared in the shared mobs_underdark.qst. No alternate-stem predecessor, tracked title rename, or explicit whole-zone replacement claim was found in these bounded paths. The shared QST declarations and current title do not change local-file inventory counts.

## Static map and inactive-WLD approach scan

The current map has 132 rooms and 313/313 loader-resolving authored exits, with two local weak components and a largest component of 118 rooms. Its five inbound and five outbound cross-zone exits have five reverse records. Three rooms have fall chances; no current declarations are present. The source dossier records the individual boundaries and route conditions. These static records do not establish live access, portal selection, safe falling, or return.

The all-95 inactive-WLD sweep found no overlapping room VNUM and no inactive-WLD exit targeting rooms #24101–#24237. No source-only approach or candidate row is added. Active graph counts and curated source-only totals remain unchanged.

## Shared quest ownership and typed-reference closure

The coverage row’s same-stem QST giver and native Q/QA counts remain zero because no mushroom_caverns.qst exists. That inventory fact coexists with three Mushroom-owned givers, #24021–#24023, whose 16 blocks (13 M, two Q, one QA) are stored in [mobs_underdark.qst](../../../areas/qst/mobs_underdark.qst#L1). The [schema-three sidecar](../../../areas/story/mushroom_caverns.story.json#L116) records three named outcomes, ten contacts, and five optional preparation checks. The source dossier traces all owned blocks, exact inputs and rewards, actor declarations and resets, topic aliases, and accounting consequences.

The bounded direct scans found no foreign native QST item consumer or additional producer for the legacy items, no external SHP product for a local object, no external typed ZON reset-role reference to a local room/mobile/object, no outside item_vnums sidecar reference, no duplicate foreign MOB/OBJ prototype, and no direct compiled C/C++ VNUM consumer. All local reset references and ordinary room targets resolve. The scan of 713 active teleport-object prototypes found no foreign fixed destination into the cavern rooms. Dynamic/admin travel and actual encounters remain unqualified. Detailed source findings and their limits remain in the [source dossier](../zone-stories/MUSHROOM_CAVERNS.md#reviewed-evidence).

## Checkpoint 298 all-442 title and direct builder questions

The questions below use authored source and the existing static review. Recommendations preserve current declarations as evidence until the area owner confirms intent.

1. **Should Mushroom Caverns, Mushroom Forest, and Myconid Mushroom Forest remain separate identities?** Recommendation: retain the three active stems and titles unless an owner supplies specific predecessor or shared-ownership evidence.
2. **Why does QST/AREA list mushroom_caverns without a same-stem QST file?** Recommendation: confirm whether the index entry is intentionally retained; keep the shared giver ownership documented separately from local-file counts.
3. **Should the legacy three-outcome amulet sequence remain maintained?** Recommendation: have the owner choose maintenance, retirement, or a named referral before adding sources or actors.
4. **Where should exact goblet #1515 come from, if the first exchange is meant to remain playable?** Recommendation: identify the intended prototype and renewable source; do not substitute the differently typed golden goblet by name alone.
5. **What durable proof type should half #24014 use?** Recommendation: choose the authoritative item kind and compatibility rules before changing the money-typed prototype or accepting it as a quest root.
6. **Should the two legacy halves remain distinct from the modern Underdark halves and seal?** Recommendation: preserve separate identities, source history, and credit unless the owner defines an explicit conversion rule.
7. **Are Ozman #24022 and Kryz #24023 intentionally unplaced?** Recommendation: confirm their maintenance status before adding resets; their native contracts alone do not provide physical encounters.
8. **Are Haz’on’wyz’s five Magma Lake Disp destinations still intended?** Recommendation: select valid, accessible shared-world rooms and verify renewal before changing the existing dispersal family.
9. **Is Bregnar’s equipped bracelet #4660 the complete intended supply path?** Recommendation: verify actor access, death/corpse transfer, and item survival before treating the declaration as reliable availability.
10. **Should Haz’on’wyz provide the automatic class-level teacher guidance?** Recommendation: retain it as a separate shared role and review recipient matching and bounded runestone output independently of quest credit.
11. **How should topic identifiers preserve the apostrophe-bearing Haz’on’wyz and Sa’zarn aliases?** Recommendation: extend stable topic representation without lossy family filtering or unsafe command escaping.
12. **Should supplied item proof continue to waive the earlier theft, house, and recovery history?** Recommendation: keep acceptance aligned with the existing item recipes unless the owner selects an explicit prerequisite contract.
13. **Should the shared QST giver-range rule continue assigning Mushroom credit to these contracts?** Recommendation: document giver ownership separately from physical zone placement and keep those policies aligned if the actors move.
14. **Does the blocked passage between #24144 and #24168 represent a permanent collapse or a clearing puzzle?** Recommendation: keep the closed/blocked reset until a real mechanism and intended route are specified.
15. **Do the paired pools’ destinations and return text describe the intended player journey?** Recommendation: qualify entry, target choice, arrival, and return, then align the “returning … impossible” text with the chosen behavior.
16. **Should the floating/NOSHOW lake and river pools respond to their shared lake and pool aliases?** Recommendation: confirm generic-find precedence and visibility policy before adjusting flags or names.
17. **What survival behavior is intended for the no-ground shaft rooms #24206–#24208?** Recommendation: qualify falling, underwater protection, follower handling, and return as one journey before changing sectors or fall chances.
18. **Should the two young aboleths follow the adult as one encounter group?** Recommendation: confirm the intended F-parenting and sentinel behavior before altering the reset sequence.
19. **Are unplaced Grell #24103 and the empty named-room descriptions deliberate omissions?** Recommendation: identify intended encounter and scenery roles before adding resets or prose.
20. **Is the broken lift and mining-disaster material intended to stay environmental lore?** Recommendation: preserve the current three native outcomes unless the owner specifies a repair mechanism, ore endpoint, and separately testable campaign result.
