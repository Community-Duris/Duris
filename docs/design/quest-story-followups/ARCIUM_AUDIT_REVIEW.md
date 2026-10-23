# Arcium repository-wide source audit

This supplement reconciles the [Arcium source-comprehensive dossier](../zone-stories/ARCIUM.md) with the active WLD graph, inactive source approaches, cross-area reset and transport references, compiled numeric uses, title ownership, and bounded same-stem history. It uses the pinned source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. It changes no gameplay or area source.

## Active map and boundary routes

The active Arcium WLD contains 115 rooms and 268 authored exits. All 268 targets resolve: 266 are local and two leave Arcium. The local graph is one weak component. From #85700, authored directed traversal reaches and returns from all 115 rooms; no local room VNUM is duplicated in another active WLD. The only duplicate active room VNUM in the repository-wide census remains unrelated #51800 in Hell Two and Hell Three. These are source-graph results; they do not qualify live doors, transport admission, or safe passage.

Both WLD boundaries are reciprocal with Surface:

| Boundary | Authored route | Evidence |
| --- | --- | --- |
| Arcium to Surface | #85700 east to #591773; #85700 south to #592172 | [Arcium-to-Surface pair row 24](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L24); [exact exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1084) |
| Surface to Arcium | #591773 west to #85700; #592172 north to #85700 | [Surface-to-Arcium pair row 463](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L463); [exact exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1556) |

Arcium has a separate active transport link to the Drifting Realm. Its fluffy bed #85725 is reset at #85805 and targets Dream room #31300. Dream objects #31301, the reality light, and #31303, the cliff, both target Arcium room #85805 using distinct LOOK and JUMP actions. These are item-transport destinations, not additional WLD boundaries; actual use and arrival remain separate from the graph.

## Inactive-WLD scan

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits with no parse issues. It found no inactive source-room VNUM overlap with Arcium and one non-overlapping source-only approach: surf.wld room #283370, “Burnt Plains of a Fallen Realm,” has D0 to #85700 ([source record](../../../areas/wld/surf.wld#L1749418)). The source is excluded from the active areas/AREA world, so this is not current ingress.

Record that approach with surf and keep surf RETAIN IN PLACE. The reviewed Surf subset increases from 163 to 164, and curated source-only approaches increase from 3,332 to 3,333 overall. The reviewed ud_map2 subset remains 15. These are source-only counts, not active routes or migration clearance.

## Typed references and compiled numeric contexts

The external QST item-command, SHP product, and STORY scans found no exact reference to Arcium rooms, mobiles, or objects. All 231 external story sidecars parsed without errors and had no exact token hits. The external ZON scan found one active typed dependency: Alatorin reset line 2810 M-resets Arcium mobile #85708 at Alatorin room #83361 ([reset source](../../../areas/zon/alatorin.zon#L2810)). Keep this active cross-area mobile dependency in both areas' closure.

One unregistered reset group also names an Arcium object: brass-old-1.zon line 543 G-resets heart #85716 under Hutzparoon mobile #139179 at room #139128 ([source](../../../areas/zon/brass-old-1.zon#L543)). brass-old-1 is not listed in areas/AREA; its reset is source-only and does not establish active heart stock. Preserve the reference with brass-old-1.

Arcium's own active ZON file imports Dream skull #31319 as a G reset for Kovii at #85811 ([reset source](../../../areas/zon/arcium.zon#L272)). This is Qin's separate green-skull proof from the existing Joji heart exchange. The Arcium bed's local reset is at [ZON line 109](../../../areas/zon/arcium.zon#L109), and Dream's two return targets are [object #31301](../../../areas/obj/dream.obj#L13) and [object #31303](../../../areas/obj/dream.obj#L40). Of 870 external type-25 declarations, these two Dream objects are the only ones targeting Arcium rooms. No external mobile or object prototype duplicates any of Arcium's 51 mobiles or 33 objects.

The compiled numeric scan identified distinct code roles:

- CHAOS equipment data has 15 loadout rows referencing local objects #85721, #85726, #85709, #85714, or #85711. This includes the same Hope shield VNUM #85721 as the native Joji reward ([equipment rows](../../../src/account/chaos_eq_data.h#L268)); kit membership does not prove a Joji receipt.
- Epic skill data lists Kurlon mobile #85724 for shield combat ([row](../../../src/classes/epic_skills.c#L180)).
- The spec assignment gives Mredock mobile #85751 the world_quest callback ([binding](../../../src/specs/specs.assign.c#L760)).

These compiled references are not duplicate prototypes or new local quest commands. Installed behavior and any player-visible result remain separate qualifications.

## Title, history, and disposition

After stripping color codes and normalizing case and whitespace, “Arcium, the Plagued Kingdom” has one owner among 442 scanned .zon titles. Lexical neighbors include Cloud Giant Kingdom, Sea Kingdom, and The Kingdom of Torg; they have distinct titles and source stems. The active title is at [arcium.zon line 2](../../../areas/zon/arcium.zon#L2).

Followed arcium.wld and arcium.zon histories reach the 2009 areas/ relocation and same-stem edits, including the 2015 quest-item-flag update and recent Arcium source updates. The bounded paths show no alternate-stem rename, distinct predecessor, or whole-zone replacement. Keep active arcium DO NOT MOVE. Preserve the active Alatorin reset, Arcium's Dream transport/skull references, and the source-only brass-old-1 reset. Keep brass-old-1 and surf RETAIN IN PLACE; none of these findings clears a path move.

The source-mapping queue is now 196/220 complete, with 24 pending; Priority 197, Future Ceothia (ceofutur), is next.

Validation: Checkpoint 156 documentation validation checked 134 changed/added Markdown files; all 5,096 local link targets and 2,421 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. No tests, build, server, gameplay session, database operation, migration, or operational script was run.
