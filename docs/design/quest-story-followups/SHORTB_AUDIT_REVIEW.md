# The Undead Fog source audit

Checkpoint 362 completes the repository-wide source review for physical coverage row 274, `area:shortb`. The new [source story map](../zone-stories/THE_UNDEAD_FOG.md) maps its twelve fog rooms, isolated Web of Unlife pocket, portal pair, reset-loaded characters and items, and lone QST text action. This supplement reconciles static connectivity, inactive approaches, title/history, typed callers, and twenty builder questions. It makes no played-access or quest-completion claim.

## Static connectivity and source-only approaches

The active WLD has thirteen rooms and 87 directions: 85 resolve within shortb and two are active Surface exits. Those two pair exactly with Surface returns: #67801 west to #569793 and #67810 east to #569799; Surface exits east to #67801 and west to #67810. All 87 exit flags and keys are zero; no D door resets, fall markers, current markers, negative destinations, or unresolved exits were found.

The raw local weak graph has one twelve-room component and isolated Web room #67809. Each Surface entry room (#67801 and #67810) reaches and can be returned to from all twelve fog rooms, while #67809 is not on the authored WLD graph. The local portal objects target #67812 and #67809 in opposite directions. Treat that as separate type-25 source wiring; it does not convert the static graph to a proved live route.

All 95 inactive WLD sources parsed without errors (527,791 rooms and 2,080,773 directions). No room VNUM overlap was found. Three distinct source-only `surf.wld` directions enter shortb: #282142 east to #67801, #282143 down to #67800, and #282144 west to #67810. They are not active ingress; preserve them on the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). The curated total becomes 4,532, and the reviewed Surf subset becomes 281. No candidate row is added.

## Typed dependencies, title, and bounded history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “The Undead Fog” is unique among all 442 current ZON titles and 351 active contributors. Same-type WLD, MOB, and OBJ header scans found no active or source-only overlap. Followed WLD, OBJ, ZON, and QST paths remain on the `shortb` stem through the 2013-11-13 maintenance history and 2009-08-12 area-tree move; MOB history also includes the 2015-10-13 quest-item flag update. No alternate-stem predecessor or whole-zone replacement is supported by those bounded paths.

The typed scan found no active external ZON reset of a shortb mobile, object, or room; no exact external QST item command; no active story-sidecar field reference; no active SHP product; and no active foreign type-25 object targeting a local room. The local shortb ZON imports no external mobile/object prototype. A direct numeric scan found no maintained `src/` reference to the area's room/mobile/object VNUMs. The two active Surface WLD connections and three inactive Surf approaches are tracked separately from these typed prototype scans. The local `qst/AREA` index lists shortb, but the local QST file contains no Q or QA contract and the area has no story sidecar.

## Builder questions and conservative recommendations

1. **Should “The Undead Fog” remain the zone title while most rooms share the title “Lost in the Grey Fog”?** Keep the unique zone title and existing room names until the area owner requests a more descriptive room-title pass.
2. **Is the repeated dry-fog prose intended to describe one maze with few landmarks?** Preserve every room and exit; do not add random displacement or a new fog effect based only on prose saying that travelers get lost.
3. **Should room #67800 retain raw room flags 33554436 while the other fog rooms use 33554432?** Preserve the source difference until the owner identifies the intended flag and its runtime meaning.
4. **Are the two Surface entry rooms #67801 and #67810 the intended approaches?** Keep both reciprocal WLD pairs and do not imply that either side guarantees safe travel.
5. **Should the twelve-room fog component stay separate from Web room #67809 in the authored WLD graph?** Preserve the current map and distinguish object teleportation from a static passage.
6. **Should portal objects #67800 and #67801 remain the only encoded route between fog room #67812 and Web room #67809?** Keep their opposite type-25 destinations and reset placements together; qualify actual use and return separately.
7. **Are both portal placements intended under reset mode 2?** Preserve the exact placements at #67809 and #67812; do not promise their presence between resets.
8. **Should the Necronomicon remain reset to Srakart rather than freely placed in Web room #67809?** Keep the current carried-object parent and avoid describing a player pickup or drop unless the reset and live behavior are separately confirmed.
9. **Is Srakart intended as the sole visible owner and current seeker of the Necronomicon?** Preserve his authored story, equipment, and room while the owner decides whether another encounter or terminal story state is wanted.
10. **Should the local QST M block remain a vampire moan line only?** Keep the exact `qc_action 45` message; do not treat it as acceptance, achievement, release, or proof of a master quest.
11. **Should the vampire #67804 continue to be described as abandoned and hungry without an encoded master relation?** Keep the prose and current prototype separate from any new rescue or familiar-ownership mechanic.
12. **Do the lost warrior, cleric, and mage souls imply a later player-facing deliverance story?** Preserve their death and summon prose; add a release objective only with a defined actor, action, recipient, and durable outcome.
13. **Are the two turtle prototypes #67805 and #67806 intentionally separate variants with parallel fog stock?** Keep both records and their distinct gendered prose until their owner decides whether the split is meaningful.
14. **Is undead-draining turtle behavior implemented or only described?** Keep the existing description without claiming active drain mechanics; source review found no direct compiled VNUM binding.
15. **What is the intended meaning of the “drained soul” clues on mobiles #67810-#67813?** Preserve the mystery in prose; do not invent a drain source or event.
16. **Is object VNUM #67806 intentionally unused?** Reserve the gap during future edits and require owner direction before adding a prototype at that number.
17. **What behavior is intended by the scrollcase object's `T 516 14 1 60` trigger record?** Preserve the raw trigger and its parent/reset placement until its trigger semantics and intended player effect are confirmed.
18. **Should the cape, wand, scrollcase, and contained scrolls stay attached to Srakart as the current reset sequence specifies?** Preserve the exact `E`, `P`, and `G` parent chain; do not convert those items into ground rewards or guaranteed drops.
19. **Are the three inactive Surf approaches part of the intended larger map?** Preserve the exact source rooms and directions on Surf row 97 as source-only evidence; do not treat them as loaded ingress.
20. **Does this area need a native journal or contract in addition to its existing lore and vampire action text?** Leave the QST index and file as authored until the builder defines an accepted objective, owner, and terminal receipt.

Keep active `shortb` DO NOT MOVE. Preserve the reciprocal Surface boundary, all source-only Surf approaches on existing row 97, isolated Web room identity, reset parentage, and unused object VNUM. No runtime qualification or migration authorization is implied. See [coverage row 274](ZONE_COVERAGE_REGISTER.csv#L274), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-362-the-undead-fog-connectivity-and-inactive-approaches), the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-362-the-undead-fog-title-vnum-identity-and-history), and the [retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-362-undead-fog-in-place-retention).
