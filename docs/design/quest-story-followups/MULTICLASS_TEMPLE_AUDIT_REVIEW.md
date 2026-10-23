# The Lost Temple of the North audit supplement

**Checkpoint 296 — normalized title, tracked history, active connectivity, typed references, and direct builder questions.** The [source dossier](../zone-stories/MULTICLASS_TEMPLE.md) maps the full local WLD/MOB/OBJ/ZON closure and compiled room behavior. This supplement records bounded cross-area and history evidence without changing areas/ or src/.

## Active title and tracked history

A color-code-stripped scan of all 442 current .zon files finds exactly one owner for “The Lost Temple of the North”: multiclass_temple. The lexical candidate tikitt, “Lost Temple of Tikitzopl,” is a distinct exact title and source stem. The current title scan does not resolve every command alias or prove a historical relationship.

Followed same-stem WLD, MOB, OBJ, ZON, QST, and SHP paths reach the 2009 areas/ tree move. Bounded file history includes 2009 multiclass-area updates and later prototype/reset edits, including the 2015 quest-item flag pass. No alternate-stem predecessor, tracked rename, or explicit whole-zone replacement claim was found in these paths. This is a bounded history result, not proof that no earlier untracked or differently named material existed. Keep the similar tikitt area separate absent an owner decision.

The normalized title scan and tracked histories close the static duplicate/replacement classification: `multiclass_temple` (*529) and `tikitt` (*441–443) have distinct active roster ranges, exact titles, and source stems. No repository evidence shows one whole area replacing the other. Whether either name should be a command alias remains a separate builder/owner question.

The current registration lists the area in both [areas/AREA](../../../areas/AREA#L225) and [areas/qst/AREA](../../../areas/qst/AREA#L206), while no local .qst file or story sidecar is present. Preserve that inventory distinction until the owner decides whether the QST index membership is intentional.

## Active graph and Surface boundaries

The 20 Temple room records have 47/47 resolving authored exits. The local graph has three weak components with 13, 4, and 3 rooms. Five active Surface boundary pairs are reciprocal: #52901/#511126, #52914/#513537, #52917/#513539, #52918/#518740, and #52920/#517940. Temple-side declarations are [exit-register rows 651–655](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L651); reverse Surface declarations appear at rows 1317, 1329–1330, 1348, and 1352.

Rooms #52902 and #52903 have fall chance 100. The #52913 east/#52904 west gate is closed and locked by both zone commands; the WLD key VNUM differs across directions (#52902 east-side, 0 west-side). Object #52901 is configured to respond to touch and teleport to #52918, connecting the isolated three-room tunnel to the main local component through generic item travel. These facts describe authored source and command dispatch, not traversal success, permission, damage, or safe return. No source-only WLD approach or unregistered target overlap was identified for this row; no route or attic candidate is added.

## Typed-reference and code closure

The zone's four M resets load local golem #52902 into #52904 and dragon #52901 into #52913, plus imported mobiles #31402 (ice golem) and #31403 (crystal) into #52909. Those imported definitions live in the active mobs_surf source ([#31402](../../../areas/mob/mobs_surf.mob#L12), [#31403](../../../areas/mob/mobs_surf.mob#L23)).

The local ice key #52902 has a reset/prose mismatch: its prototype says it lies on the ground, while the G reset follows the dragon mobile load. The flux object #52901 declares destination #52918 and command 320 (CMD_TOUCH); its text warns of burning, but the generic item-teleport handler provides destination travel and no local burn-specific handler was found. The handler binding at #52904 is the one direct compiled code reference to this area's VNUMs.

The bounded exact-number sweep across active QST, ZON, SHP, story-sidecar, and code sources found no outside item consumer, shop product, local-prototype reset consumer, or typed sidecar reference. It also found no duplicate foreign definition of the local mobile/object VNUMs. The static closure is documented; indirect tooling, live access, persistence, and actual class-change outcomes remain unqualified. The source dossier contains the room-procedure checks and local links.

## Checkpoint 296 all-442 title and direct builder questions

The questions below are source-backed decisions for the zone builder or owner. Each recommendation preserves the current source behavior until intent is established.

1. **Is this distinct from the similarly named Lost Temple of Tikitzopl?** Recommendation: keep multiclass_temple and tikitt as separate area identities; record any alias only if an owner confirms it.
2. **Is room #52904 still intended to host a multiclass service?** Recommendation: confirm the room's purpose and keep the compiled service described as a class change rather than as a native quest.
3. **Should the service continue to respond only to the multiclass command in #52904?** Recommendation: document that invocation path and retain the room-bound assignment unless a deliberate interface change is approved.
4. **Are the Human/Orc, non-NPC, non-morph, and not-already-multiclass checks intended?** Recommendation: confirm these as the service's eligibility contract and explain them in player-facing help.
5. **Is the default minimum level of 50 still intended?** Recommendation: confirm the configured default and its property override before describing the threshold as fixed.
6. **Is the default epic-experience minimum of 250 still intended?** Recommendation: confirm the configured default and explain whether the threshold is character-wide or otherwise qualified by existing code.
7. **Should the current can_char_multi_to_class eligibility check remain the class allow-list?** Recommendation: document its result as the final class-specific gate and review the allowed set with the owner.
8. **Is it intentional that no local QST, item, coin, or token payment is required?** Recommendation: keep the current no-offering behavior unless the owner defines a separate, explicit contract.
9. **Are the consequences of do_start during the class change acceptable?** Recommendation: review the full reset semantics and disclose the confirmed effects before calling the procedure a simple class add.
10. **Should the change continue to forget spells, refund epic skills, and clear the zone trophy?** Recommendation: confirm each side effect and its recovery behavior with the gameplay owner.
11. **Should the gate remain closed and locked in both directions with different key VNUMs?** Recommendation: align both WLD key fields if the asymmetry is accidental; otherwise document why the reverse side uses key 0.
12. **Should the ice key come from the dragon or lie on the ground?** Recommendation: make the ZON G reset and key description express one intended acquisition method.
13. **Are the local golem at #52904 and dragon at #52913 the intended guards for the trainer, energy sphere, and gate?** Recommendation: confirm placements and roles against the intended challenge progression.
14. **Are the imported ice golem #31402 and crystal #31403 at #52909 intentional?** Recommendation: retain the documented mobs_surf dependency and confirm the imported prototypes remain suitable.
15. **Are the guardians at #52919 meant to remain scenery with no mobile resets?** Recommendation: confirm the statue/rune passage is descriptive only or specify the intended encounter separately.
16. **Are all five reciprocal Surface boundaries intended as player entrances and exits?** Recommendation: confirm the two tunnel pairs and the main shaft route as separate access paths without assuming their safety.
17. **Are the 100-percent fall records at #52902 and #52903 intentional?** Recommendation: verify the resulting landing, damage, and return route before treating the shaft as a safe ordinary entrance.
18. **Should the touch object bridge the main cavern to the separate tunnel at #52918?** Recommendation: confirm the destination as the intended route connection and retain its fixed target only if that path is deliberate.
19. **Should touching the flux sphere burn the player, or only teleport them?** Recommendation: align the warning text with the implemented behavior, or specify an intentional damage rule for a separate future change.
20. **Should multiclass_temple remain listed in areas/qst/AREA without a local QST file?** Recommendation: have the owner retain or remove that index entry after confirming its purpose; do not infer a missing quest from the registration alone.
