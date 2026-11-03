# Nax audit supplement

**Checkpoint 301 — all-442 title and history, direct typed-reference closure, compiled bindings, and twenty builder questions.** The existing [Nax story review](NAX_STORY_REVIEW.md) maps the 245-room environment, residents, 286 zone commands, eight shops, static graph, and known typed data dependencies. This supplement adds the broader title/history and compiled-reference pass without changing areas/ or src/.

## All-442 title and followed history

A case- and whitespace-normalized scan after color-code removal from all 442 current .zon titles finds one exact owner for “Nax”: active nax ([zone title](../../../areas/zon/nax.zon#L2)). The active QST index lists nax, but no same-stem nax.qst exists; the story sidecar says coverage is complete while its contacts, stories, and exclusions arrays are empty.

Followed current paths stay on the nax stem. WLD history reaches the 2009 areas-tree move and later edits through 2013; MOB through 2014; OBJ through 2013; ZON through 2026 updates; and SHP through 2014. There is no local QST history because the file is absent. The story sidecar was added October 2, 2026. No alternate-stem predecessor, tracked title rename, or explicit whole-zone replacement claim was found in these bounded paths. The area annotation “minotaur hometown -scip” remains a separate ownership/workflow question; it does not establish a replacement.

## Bounded typed references and compiled bindings

The existing source review records these area-data dependencies: local reset rows import Heavens mobile #101 and Heavens objects #108, #109, #111, #143, #144, #338, #361, #363, #368, #369, #429, #431, and #826; they also import Verz1 #3097, Twin Towers Forest #13563, Realm #14029, and Clfhaven #21585. Alatorin has one active G reset for Nax object #37742. These declarations preserve the external prototype owners and their distinct roles.

The exact external scans found no foreign QST item command, SHP product, WLD exit key, foreign duplicate mobile/object prototype, or outside story-sidecar item_vnums entry for a local object. The only typed outside ZON reset target is Alatorin G #37742. An all-object-file scan found 868 external type-25 declarations and none targets a Nax room. These results refine the CP12 source review and remain bounded to declared source references.

A direct numeric scan of C/C++ source found a small set of meaningful Nax references, all classified by their field type:

| Code reference | Typed target and source meaning | Qualification limit |
| --- | --- | --- |
| [specs.assign.c](../../../src/specs/specs.assign.c#L293) and [line 788](../../../src/specs/specs.assign.c#L788) | Local mobile #37764, Syrm the bartender, and local mobile #37718, a halfling slave, both receive the shared world_quest handler. | This is dynamic shared world-quest behavior, not a local QST contract. The current offer pool, database state, actor availability, and played outcomes were not queried. |
| [world_quest handler](../../../src/specs/specs.world_quest.c#L191) | The shared handler resolves addressed ASK commands and uses player world-quest state. | Its presence does not map a Nax-specific story or establish an active offer. |
| [chaos port table](../../../src/combat/chaos.c#L27) and [destination resolution](../../../src/combat/chaos.c#L55) | The alias “nax” resolves to room #37712 as a compiled chaos-port destination. | This dynamic travel declaration does not qualify player eligibility or a route into the detached 74-room cave group. |
| [tradeskill smith list](../../../src/economy/tradeskill.c#L202) and [initialization](../../../src/economy/tradeskill.c#L958) | Mobile #37755 is recognized and bound to the shared smith function. | Actual service and supply behavior were not exercised. |
| [inn assignments](../../../src/specs/specs.assign.c#L2293), [shared-room assignment](../../../src/specs/specs.assign.c#L2329), and [Nax assignment](../../../src/specs/specs.assign.c#L2425) | Rooms #37715 and #37716 are assigned the inn procedure. | These room bindings are separate from the eight SHP shop records. |
| [hometown lookup](../../../src/core/constant.c#L1734) and [NaxVaran room table](../../../src/core/constant.c#L1952) | Nax is a configured hometown; the table contains local starting destinations #37716, #37776, #37944, and #37763. | The table is static race/hometown mapping; admission, arrival, and safety were not exercised. |
| [justice hometown entry](../../../src/combat/justice.c#L82) | Room/mobile VNUM #37701 appears in the Nax guard/home configuration. | justice_send_guards currently returns before dispatching guards, so the table entry does not establish active guard service. |

The compiled scan separates same-number room, mobile, and object identities by actual field type. It found no additional meaningful direct C/C++ VNUM consumer. The current zone’s general description and old source audit remain useful context; code bindings do not fill the empty local story arrays or prove live behavior.

The existing [inactive-WLD review](NAX_STORY_REVIEW.md#inactive-wld-sweep) preserves one source-only Surf approach: room #256920 west to Nax #37800. It remains on the existing [Surf retention record](ATTIC_MIGRATION_PLAN.md#L27) and [connectivity entry](WORLD_CONNECTIVITY.md#L169). No active route, candidate row, or curated source-only count changes in this checkpoint.

## Checkpoint 301: twenty direct builder questions and recommendations

Recommendations preserve current source and prototype ownership until builders choose a documented change.

1. **Does the sidecar’s “coverage: complete” and local-requests invitation match its empty contacts, stories, and exclusions?** Recommendation: have the owner supply the missing mapping or clarify the intended no-local-quest state before describing coverage as complete.
2. **Should Syrm #37764 keep the shared world_quest handler?** Recommendation: confirm the bartender’s global quest, map, and abandon interactions as a separate compiled service; do not count them as a native Nax QST contract.
3. **Should the halfling slave #37718 also keep the shared world_quest handler?** Recommendation: verify this specific binding and intended topic audience before advertising a local request from that character.
4. **Is the compiled “port nax” destination at #37712 an intended access route for ordinary players?** Recommendation: document its actual eligibility and arrival conditions separately from ordinary WLD access.
5. **Are NaxVaran’s configured hometown starting rooms #37716, #37776, #37944, and #37763 still intended?** Recommendation: preserve each typed room destination and qualify race admission and safe arrival before changing the hometown table.
6. **Is the Justice entry for #37701 intended to remain configured while guard dispatch is disabled?** Recommendation: keep hometown configuration distinct from active justice gameplay and do not promise guard service from the table alone.
7. **Are the inn bindings at rooms #37715 and #37716 intentionally separate from the eight shop records?** Recommendation: preserve the typed room assignments and verify the intended services independently.
8. **Should smith mobile #37755 receive the shared tradeskill smith handler?** Recommendation: confirm its role and stock before changing the compiled smith list or describing a live service.
9. **Is Heavens mobile #101 intended to work as the drug dealer in the Brawling Foot?** Recommendation: preserve the imported mobile owner and verify the paired Heavens equipment and consumer path before changing either area’s resets.
10. **Are the imported Heavens objects #108, #109, #111, #143, #144, #338, #361, #363, #368, #369, #429, #431, and #826 intended in Nax reset groups?** Recommendation: keep each exact source owner and review the local placement set as one explicit cross-area decision.
11. **Should Alatorin continue to reset local liquid potion #37742 onto its mobile?** Recommendation: preserve that inbound typed dependency and confirm the intended owner, recovery, and any story relationship before changing the reset.
12. **Is the active reciprocal Surface boundary the intended ordinary city entrance and return?** Recommendation: keep Nax #37800 east ↔ Surface #625238 west as a paired route unless a specific replacement is approved.
13. **Should inactive Surf room #256920’s west approach remain source-only?** Recommendation: retain the existing Surf candidate record and do not present it as active ingress.
14. **How should ordinary players enter the detached 74-room pit, cavern, labyrinth, and tower component?** Recommendation: identify the intended TOUCH/ENTER, compiled, or other dynamic route and its return before changing WLD connectivity.
15. **Should the #37733 down exit to collapsed cavern #37734 be a secret, closed door, or procedurally managed opening?** Recommendation: reconcile the prose with its current key-zero, non-door exit flags and absent D reset only after the owner confirms intended behavior.
16. **Do the eight current rooms and the fall chance at #37794 match the river and rope-bridge danger described in prose?** Recommendation: keep hazard prose distinct from live mechanics and qualify currents, falls, and return before changing values.
17. **Should the river and delta text warn that current may carry a traveler toward the sea?** Recommendation: confirm direction, safety, and recovery intent before adding a return route or weakening the existing warning.
18. **Is the forced-labor and captive setting intended as a central part of Nax’s story?** Recommendation: preserve the authored context and seek owner review before reframing the affected rooms or residents.
19. **Should the minotaur/orc dwelling wording, “cows” address, and repeated spelling errors remain in the intended voice?** Recommendation: collect a focused prose decision before any editorial pass; keep terminology and descriptions internally consistent.
20. **What does the “-scip” area annotation mean for current ownership or editor workflow?** Recommendation: retain it as an unresolved note until its owner explains it; do not treat it as a rename or replacement marker.
