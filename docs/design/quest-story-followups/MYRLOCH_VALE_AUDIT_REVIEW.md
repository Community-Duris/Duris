# Myrloch Vale — Checkpoint 107 full-area audit

This supplement reconciles the comprehensive dossier with the active area manifest, static WLD graph, all inactive WLD files, typed quest/story/reset references, direct numeric bindings, active title inventory, and followed source history. It does not qualify played travel, current reset stock, ordinary-player safety, or a wider campaign relationship. The existing [Myrloch Vale dossier](../zone-stories/MYRLOCH_VALE.md) retains the detailed quest and builder-question record.

## Active source and static connectivity

`areas/AREA` lists `myrloch_vale` at line 133 as an active full-area contributor. Its ZON title is “Myrloch Vale,” zone 264, with physical WLD rooms #26401–#26578 (178 rooms). The complete active WLD census found 373 authored exits and 373 loader-resolving targets: 372 local and one outbound. There are no active-room VNUM collisions for Myrloch Vale.

The local undirected projection has two weak components, sized 165 and 13. Directed authored WLD traversal from #26401 reaches #26401–#26413 only (13/178); the other 165-room component spans #26414–#26578. This is static exit-graph evidence only. It does not simulate item TOUCH/ENTER teleports, reset stock, door state, mobile movement, or other runtime travel already discussed in dossier follow-ups.

The single active boundary is reciprocal with Moonshae Island: Moonshae #26311 west enters Myrloch #26401, and Myrloch #26401 east returns to Moonshae #26311. See active [inbound exit row 408](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L408), [outbound exit row 409](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L409), [inbound pair row 290](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L290), and [outbound pair row 312](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L312). The coverage register records one active inbound exit from `moonshae` and one outbound exit to it.

## Inactive WLD sweep

All 95 inactive WLD files parsed without issues: 527,791 rooms and 2,080,773 exits, with no inactive source-room VNUM overlap against Myrloch. The scan found zero inactive-WLD exits into Myrloch Vale. This checkpoint adds no Surf edge and does not change the reviewed Surf subset (133) or curated source-only route total (1,520).

## Typed dependencies and effective numeric references

The only external active QST item command found is `areas/qst/caves_skelenak.qst:165`, `G I 26438`: Caves of Mt. Skelenak Q155 accepts Myrloch's exact flaming shield #26438 and returns marble eye #4029 to giver #4038 in room #4153. The shield is defined in `areas/obj/myrloch_vale.obj:487`. The active Caves story sidecar independently records the exact typed `item_vnums: [26438]` and the matching completion key in `areas/story/caves_skelenak.story.json` under `monk-warlord-shield`. This is the existing outside lead already recorded as ZSQ-MYRLOCH-13; it does not establish a campaign link to the elder's head return or a personal kill.

Two Myrloch zone reset rows import objects defined by other sources: object #358 from `areas/obj/heavens.obj` (“stone rune-covered”), and object #55455 from `areas/obj/wh.obj` (“memory of Myrloch Vale”). Their local reset rows are `areas/zon/myrloch_vale.zon:212–213`. The dossier identifies the rune stone as an `epic_stone` touch and the memory as quest-tagged/`_noquest_`; preserve both object owners and their separate roles.

Direct C/C++ bindings include mobile #26402 to `unblock_on_death` (`src/specs/specs.assign.c:1253`), objects #26413–#26416 to `item_switch` (lines 2034–2037), and room #26566 to `inn` (line 2513). These are local effective behavior bindings already covered in the dossier, not additional cross-zone quest contracts.

The bounded typed scans found no external SHP product, no outside active ZON reset-role reference to Myrloch room/mobile/object prototypes, no external mobile/object prototype duplicate, and no target among 865 external type-25 declarations. No other outside active QST item or exact sidecar VNUM was found. The all-sidecar scan parsed 231 external story files without errors; its four broad numeric-token hits are all the exact #26438 shield/completion reference in `caves_skelenak.story.json`, including the typed object entry.

## Title, source history, and disposition

The normalized active ZON title “Myrloch Vale” is unique among 351 non-empty active titles. The only lexically similar titles found were “The Chasm of the Misty Vale” (`mist_chasm`) and “Towering Glacier of Dayedenvale” (`glacier`); neither title establishes shared identity or replacement.

Followed WLD/ZON paths show the 2009 repository-wide `areas/` move and same-path edits in 2013 and 2016, including the 2016 `unblock_on_death` integration. Later ZON adjustments include the 2026 ambient/zone update. The 2026-10-06 story-sidecar commit adds journals and world-action mapping. The reviewed active manifest and tracked paths show no explicit predecessor, alternate-stem rename, or replacement claim. Moonshae is a separate active area connected by the reciprocal boundary above.

Myrloch Vale is an active `areas/AREA` contributor and remains **DO NOT MOVE**. Preserve the Moonshae boundary and the Caves shield/QST dependency with their separate source owners. Do not add Myrloch to the attic candidate set. No inactive WLD approach or dependency finding supports a move.

## Builder follow-up for the dossier

The static directed graph from #26401 reaches only 13 of 178 rooms, while a 165-room weak component exists outside that set and no inactive WLD file supplies an approach. Confirm the intended access path into that component, including which TOUCH/ENTER or other dynamic routes are expected to provide access and safe return. Keep that answer distinct from current stock and door state. Separately preserve the Caves shield exchange as an outside typed dependency unless its builder confirms an explicit campaign relationship.

## Source references

- [Active manifest line 133](../../../areas/AREA#L133), [Myrloch ZON title/header](../../../areas/zon/myrloch_vale.zon#L1), and [local imported-object resets](../../../areas/zon/myrloch_vale.zon#L212).
- [Myrloch entrance and outbound target](../../../areas/wld/myrloch_vale.wld#L1), [Moonshae return room](../../../areas/wld/moonshae.wld#L2298), and [Moonshae return target](../../../areas/wld/moonshae.wld#L2315).
- [Caves QST shield command](../../../areas/qst/caves_skelenak.qst#L165), [Myrloch flaming shield object](../../../areas/obj/myrloch_vale.obj#L487), and [Caves story-sidecar item reference](../../../areas/story/caves_skelenak.story.json#L273).
- [Imported rune stone](../../../areas/obj/heavens.obj#L3709), [imported memory object](../../../areas/obj/wh.obj#L5359), and [direct local bindings](../../../src/specs/specs.assign.c#L1253) (also [switch bindings](../../../src/specs/specs.assign.c#L2034) and [inn binding](../../../src/specs/specs.assign.c#L2513)).

Evidence files used for this reconciliation: active and inactive WLD scans, typed QST/ZON/SHP/sidecar scans, direct C/C++ binding scan, title inventory, and `git log --follow` for the Myrloch WLD/ZON paths. No server build, regression suite, gameplay session, database query, migration, or operational script was run for this documentation checkpoint.


## Checkpoint 300: all-442 title and full followed history

A normalized scan after removing color codes from all 442 current .zon titles finds one exact owner for “Myrloch Vale”: active myrloch_vale. “The Chasm of the Misty Vale” (mist_chasm) and “Towering Glacier of Dayedenvale” (glacier) are distinct titles and contributors. The exact title is recorded in [myrloch_vale.zon](../../../areas/zon/myrloch_vale.zon#L2).

Followed tracked paths were checked for the current WLD, MOB, OBJ, ZON, QST, and SHP sources. WLD and QST histories reach the 2009 areas-tree move and same-stem edits through 2016; MOB, OBJ, and ZON histories remain on the same stem through later revisions, including the 2026 area updates; SHP history reaches the 2009 move and 2013 update. The schema-three story sidecar was added October 6, 2026. No alternate-stem predecessor, tracked title rename, or explicit whole-zone replacement claim was found in these bounded histories. The CP107 static graph, all-95 inactive-WLD sweep, and typed dependency closure remain unchanged: one reciprocal Moonshae boundary, zero inactive-WLD approaches, the outside Caves shield exchange, imports #358/#55455, and local compiled bindings. No migration clearance follows.

## Checkpoint 300: twenty direct builder questions and recommendations

These questions use the established source findings in the [comprehensive dossier](../zone-stories/MYRLOCH_VALE.md) and the [CP107 graph and typed review](MYRLOCH_VALE_AUDIT_REVIEW.md). Recommendations keep native returns independent and preserve source ownership until builders select a documented change.

1. **What route should take players from #26401–#26413 into the separate 165-room weak component?** Recommendation: confirm each intended TOUCH/ENTER or other dynamic route and its safe return before changing the static map.
2. **Should the reciprocal Moonshae boundary remain Myrloch’s sole ordinary WLD entrance?** Recommendation: preserve #26311 west ↔ #26401 east and its active owner unless a specific new boundary is authorized.
3. **Are the six stocked teleport objects and the waterfall ENTER object the intended dynamic access routes?** Recommendation: qualify each selected object, command, destination, arrival, and return before promising route access.
4. **Should the advertised hidden stairway #26437 be stocked, retired, or left as unstocked lore?** Recommendation: choose a separately scoped repair or wording change; retain the current ordinary backtracking route in the analysis.
5. **Should the four accepted item returns remain independent recipes?** Recommendation: preserve the old key, old bracelet, old dagger, and elder’s head as four exact receipts without inventing a required chain.
6. **Should the old dagger return remain the only master exchange that departs?** Recommendation: keep the current exact departure rule unless the area owner defines a different actor lifecycle.
7. **Does the elder’s sacrifice farewell mean the head GIVE is the full accepted outcome?** Recommendation: keep the native key exchange as its own receipt; specify a separate altar action only if an actual ritual is intended.
8. **Are the actual addressed dialogue topics limited to the reviewed six aliases?** Recommendation: expose quest/quests/head/warlord/fire/chest only and keep ambient echoes and qc_unblock control text out of player topic evidence.
9. **Should transferred or supplied hidden proofs remain acceptable without personal SEARCH or recovery?** Recommendation: preserve exact item acceptance while recording SEARCH, first recovery, custody, and accepted return as separate events.
10. **Is the red flame’s flaming key #26409 intended as an alternate brass-door route?** Recommendation: preserve this source and key path separately from the old-key exchange and its receipt.
11. **Are the six key prototypes’ break and NORENT values intentional?** Recommendation: verify admitted unlock, destruction, rent, and recovery behavior before changing item values or durability policy.
12. **Should the four hidden PULL switches keep their present room targets and staged access order?** Recommendation: preserve each selected root-to-edge mapping and do not make every switch a prerequisite for the elder’s head exchange.
13. **Should the shared item_switch handler fail safely when a reciprocal exit is absent or mismatched?** Recommendation: review a generic handler fix separately and preserve the four currently correct Myrloch effects.
14. **Is the asymmetric middle route through #26539/#26545 intended to reach the warlord after the first two switches?** Recommendation: confirm whether the farther switches are optional before changing topology or requiring all four.
15. **Should opening the warlord barrier credit a specific killer or qualified participants?** Recommendation: keep the current death-triggered wall opening distinct from actor-attributed victory and head acceptance until attribution is explicitly designed.
16. **Are floor drops after the warlord’s special death the intended recovery path without a corpse?** Recommendation: preserve inherited floor-loot custody and lineage; do not add a speculative second corpse.
17. **Should the elder’s locked, pickproof chest and its two child items remain separate from accepting the head?** Recommendation: require actual key/open/child retrieval evidence for any future chest outcome.
18. **Is Caves of Mt. Skelenak’s shield exchange meant to join Myrloch’s head story?** Recommendation: retain the exact outside dependency as an independent quest unless both owners confirm an explicit campaign endpoint.
19. **Should imported rune stone #358 and Memory of Myrloch Vale #55455 keep their separate owners and functions?** Recommendation: preserve the epic_stone touch path and quest-tagged/_noquest_ souvenir semantics without treating either as an extra native QST receipt.
20. **Should reset mode 1 continue withholding proof, switch, teleport, container, and loot allocation until durable reset-generation ownership exists?** Recommendation: keep active READY accounting and renewal policy separate from daily metadata or an already-recorded receipt.
