# The Reliquary Nexus of The Dead: repository-wide route, dependency, and history review

This supplement accompanies the [comprehensive source dossier](../zone-stories/NEXUS_UC.md). It reconciles the declared WLD graph, reset object routes, code-driven entrances, inactive-map references, typed dependencies, normalized title ownership, followed history, and retention. Static evidence does not certify live reset state, route safety, or player outcomes.

## Active WLD graph and Surface boundary

Active `nexus_uc` contains 85 rooms, #130200–#130284, and 177 authored exits. All 177 targets resolve: 176 lead to rooms in the area and one goes north from #130200 to Surface #577404. The Surface return goes south from #577404 to #130200 ([outbound register row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1294), [return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1505)). There are no negative targets, positive unresolved targets, fall markers, or currents.

The WLD-only local graph has eight weak components with sizes 39, 10, 10, 10, 4, 4, 4, and 4. Raw directed traversal from Surface entry #130200 reaches four rooms, and four rooms have a path back to it. The seven paired object-route connections documented in the dossier add fourteen directed transitions. Including those ZON-reset `ITEM_TELEPORT` destinations makes all 85 rooms reachable from and returnable to #130200 in the source-level graph. This conditional topology ignores live reset availability, item interaction, and door state. The local ZON declares 22 door resets, including key-gated rite, courtyard, tower, and final-Nexus passages.

## Code-driven entrances outside the WLD register

The WLD register captures only the reciprocal Surface boundary. A separate special proc on object #468 directly sends users to #130200. That object is reset in Heavens #23, Tharnadia #132573, Kimordril #95553, Khildarak #17021, and Shady #97628. The proc accepts `enter`, limits non-trusted characters to levels 10–30, and is assigned in `specs.assign.c` ([proc](../../../src/specs/specs.heavens.c#L1519), [assignment](../../../src/specs/specs.assign.c#L1652), [Heavens reset](../../../areas/zon/heavens.zon#L145), [Tharnadia reset](../../../areas/zon/tharnadia.zon#L515), [Kimordril reset](../../../areas/zon/kimordril.zon#L170), [Khildarak reset](../../../areas/zon/khildarak.zon#L2127), [Shady reset](../../../areas/zon/shady.zon#L369)).

Local object #130200 is a `bathe` teleport to #130202 and has value 30 after the 2016 level pass. `specs.assign.c` binds it to `obj_tp_no_high_levels`; the object procedure blocks non-trusted users above the object’s limit before the generic teleport handler proceeds ([local object assignment](../../../src/specs/specs.assign.c#L1696), [gate procedure](../../../src/specs/specs.nexus_uc.c#L15), [item-teleport handler](../../../src/magic/spell_travel.c#L932)). These code references are real dependencies and explain why the extra entrances do not appear as WLD exit records.

## Inactive WLD sweep and Duris3 source-only references

The all-95 inactive-WLD scan traversed 527,791 room records and 2,080,773 D0–D9 direction records with no structural parse issue. It found 340 records targeting #130200–#130284, all in unregistered `Duris3.wld`. Of these, 169 begin at active-owned source VNUMs and are excluded from source-only approaches: 168 east/west records begin at same-numbered `nexus_uc` rooms, and one east record begins at #130199, owned by `nexus_ic` ([#130199 record](../../../areas/wld/Duris3.wld#L634188)).

The other 171 references begin outside active room ownership:

| Duris3 source rooms | Count | Direction and target | Evidence |
| --- | ---: | --- | --- |
| #129900–#129984 | 85 | south to #130200–#130284 in numeric order | [First](../../../areas/wld/Duris3.wld#L627913), [last](../../../areas/wld/Duris3.wld#L629677) |
| #130285 | 1 | west to #130284 | [Record](../../../areas/wld/Duris3.wld#L636002) |
| #130500–#130584 | 85 | north to #130200–#130284 in numeric order | [First](../../../areas/wld/Duris3.wld#L640505), [last](../../../areas/wld/Duris3.wld#L642269) |

These inactive references are not installed approaches. Preserve them on the existing Duris3 candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), leaving its 26 active-reference count unchanged.

## Typed dependency closure

The local inventory is WLD/MOB/OBJ/ZON only. No `nexus_uc.qst`, `.shp`, or story sidecar exists; the separate [QST/AREA index](../../../areas/qst/AREA#L328) lists the area without adding a local contract. Bounded exact scans found no external QST item command, SHP product, structured story `item_vnums` reference, ZON reset-role target into #130200–#130284, duplicate mobile/object prototype, or external type-25 destination into the room range.

Two compiled references were found and classified: `specs.heavens.c` sends the shared object #468 directly to #130200; `specs.assign.c` binds local object #130200 to the level-limit procedure. The associated shared-object proc and five reset placements are documented above. No additional external exact numeric token was found in the bounded QST, SHP, story-sidecar, ZON, MOB, OBJ, or C/C++ scans. These source checks do not qualify administrative, editor, installed-build, or computed runtime references.

## All-442 title and followed history

After color stripping and normalization, “The Reliquary Nexus of The Dead” is unique among all 442 current `.zon` files ([title source](../../../areas/zon/nexus_uc.zon#L2)). The separate “Nexus” source stems have their own titles; in particular, Aggression, Corsairs, Roper Den, and The Dead remain distinct current areas.

Followed `nexus_uc.wld` history contains only the 2009-08-12 move into the `areas/` tree (commit `642da5400`). MOB, OBJ, and ZON share a 2016-09-22 content update (commit `1866f24c0`) whose message records lower-level replacement mobs, increased Nexus experience payout, new portals from Tharnadia, Kimordril, Khildarak, and Shady, and a level-30 cap on the first local teleporter. MOB and ZON also include the 2015 quest-item flag pass; OBJ has a 2016-08-15 cleanup. These paths otherwise show same-stem edits. No alternate-stem predecessor, tracked rename, or explicit whole-zone replacement appears in the bounded history or duplicate/replacement review.

## Retention disposition

Keep active `nexus_uc` **DO NOT MOVE** as a current un-commented `areas/AREA` contributor. Preserve the reciprocal Surface crossing, all seven component-connecting object-route pairs, five external #468 placement sites, the local #130200 level cap, and the current key/reset structure until the builder resolves the open questions. Keep Duris3 **RETAIN IN PLACE** with its 171 source-only references on existing candidate row 36. No active route, source, candidate count, or migration disposition changes.

## Checkpoint 312: twenty direct builder questions and recommendations

1. Is Surface #577404 the intended ordinary arrival at #130200? Recommendation: confirm it as the visible returnable entrance and distinguish it from the level-gated portal proc.
2. Should the #468 rift sites in Heavens, Tharnadia, Kimordril, Khildarak, and Shady share one access policy? Recommendation: confirm all five placements and their common landing room before changing any reset.
3. Is the level-10-to-30 range on #468 still intended, including trusted bypass? Recommendation: record the accepted range and exception once for all five sites.
4. Is the maximum level of 30 on the blood-filled tub #130200 intentional? Recommendation: reconcile its 2016 value with the shared rift gate and keep both limits aligned with the zone’s intended audience.
5. What happened in the massacred antechamber and ruined living quarter? Recommendation: identify the victims and cause from canon before attaching a contract or allegiance.
6. Who is Thotenthomanp, and why does the Lord of Decay occupy that chamber? Recommendation: define his relationship to the massacre, blood tub, and Land of the Dead before expanding his role.
7. Is bathing in the tub meant to transport a player into the dead field? Recommendation: retain the authored command only if the destination and return mirror are clearly discoverable and safe.
8. What does the “rite of passage” key open, and why does Thotenthomanp carry it? Recommendation: confirm that it belongs to the #130219/#130220 secret door and state the intended acquisition path.
9. Is the key path through the rite door fair when the key holder and door are both in the dead-field route? Recommendation: confirm room order, door direction, and recovery if a key is lost.
10. Are the Anubian soldiers meant to be lethal gatekeepers or guardians? Recommendation: state their allegiance and ensure the #130215 and #130216 keys match the intended gate sequence.
11. What is the choking acrid dust and how should travelers navigate it? Recommendation: define its hazard, warning, and recovery before treating the field text as a playable effect.
12. What do the two masses of negative energy represent? Recommendation: confirm the enter-activated #130217↔#130226 route, its lore, and its return behavior.
13. Why does a lush tropical courtyard grow inside the limestone enclosure? Recommendation: connect its source and purpose to the dead field and tower, or mark the contrast as deliberate.
14. Which white gates require the #130215 key, and how does a player reach them from the soldier at #130219? Recommendation: reconcile each lock, reset, and reciprocal passage against the actual key locations.
15. What do the tower’s murals and unreadable glyphs teach? Recommendation: name the wars, artifacts, or warlords that matter to the route before using them as clues.
16. What is Osiris’s role beyond Keeper of the Land of the Dead? Recommendation: decide whether his chanting and #130217 key mark him as a guide, gatekeeper, or opponent.
17. What does the final Nexus do when its magic taints a visitor? Recommendation: specify whether the effect is descriptive, temporary, or durable and identify any intended recovery.
18. Is the closed return door from #130284 meant to require the Nexus key? Recommendation: confirm a reliable route back through the throne chamber and account for its reset state.
19. Why are fire and earth elemental pockets inside this death-themed area? Recommendation: establish their relation to the Nexus; preserve the paired `touch`, `scream`, `stare`, and `worship` routes only with explicit destination cues.
20. Is the QST/AREA listing intentional without a local QST, SHP, or sidecar? Recommendation: confirm whether `nexus_uc` remains an environmental story or receives a separately designed contract and saved result.
