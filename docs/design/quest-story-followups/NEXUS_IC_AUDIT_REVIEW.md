# The Reliquary Nexus of Aggression: repository-wide route, dependency, and history review

This bounded supplement accompanies the [comprehensive source dossier](../zone-stories/NEXUS_IC.md). It reconciles current WLD boundaries, the all-inactive-map sweep, exact typed dependencies, normalized title ownership, followed same-stem history, and retention disposition. Static source evidence does not establish live access, reset state, or gameplay outcomes.

## Active graph and reciprocal Surface boundaries

Active nexus_ic contains 100 rooms, #130100–#130199. Its 258 exit declarations all resolve: 254 are local and four cross to Surface. The local weak components contain 76 and 24 rooms. The four Surface pairs are reciprocal and use rooms #130100 west ↔ #511534 east, #130112 north ↔ #511135 south, #130124 east ↔ #511536 west, and #130136 south ↔ #511935 north ([outbound rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1290), [return rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1318)). All four Nexus boundary rooms lie in the 76-room component. The remaining 24 rooms are separate in WLD topology; a type-25 item destination is not a WLD edge.

## Inactive WLD sweep and Duris3 source-only approaches

The sweep traversed all 95 WLD files outside the active areas/AREA list: 527,791 rooms and 2,080,773 exits. Six extra standalone tilde lines occur after descriptions in inactive waterplane rooms #5–#10; the scanner treated those as empty terminators and continued. No other structural parsing error was reported. This source formatting quirk is unrelated to Nexus IC.

There are 400 inactive exit records whose destination is #130100–#130199, all in unregistered Duris3.wld. Of these, 224 begin at VNUMs already owned by active areas and are excluded from source-only approaches. They consist of 198 records from same-numbered Duris3 map copies #130100–#130199, one from VNUM #130200 owned by active nexus_uc, and 25 from #130400–#130424 owned by active nexus_roper. The same-number Duris3 records have different room titles/descriptions and do not establish replacement or active ingress.

The remaining 176 records begin outside active room ownership:

| Duris3 source rooms | Exits into Nexus IC | Direction and target | Evidence |
| --- | ---: | --- | --- |
| #129800–#129899 | 100 | south to #130100–#130199 in numeric order | [First target line](../../../areas/wld/Duris3.wld#L625816), [last target line](../../../areas/wld/Duris3.wld#L627895) |
| #130099 | 1 | east to #130100 | [Target line](../../../areas/wld/Duris3.wld#L632091) |
| #130425–#130499 | 75 | north to #130125–#130199 in numeric order | [First target line](../../../areas/wld/Duris3.wld#L638933), [last target line](../../../areas/wld/Duris3.wld#L640487) |

These are inactive source-only references, not installed approaches. The apparent Duris3 #130200 west to #130199 endpoint is excluded because active nexus_uc owns VNUM #130200; see its [active boundary record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1294). Preserve the 176 records in the existing Duris3 candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), without changing that row’s count of 26 active exits into Duris3.

## Typed dependency closure

The local file inventory is WLD/MOB/OBJ/ZON only: there is no nexus_ic QST, SHP, or story sidecar. areas/qst/AREA nevertheless includes nexus_ic at [line 327](../../../areas/qst/AREA#L327). The local ZON declares 64 door resets, 111 mobile placements, eight object placements, and one equipment placement. The eight reset objects are the local type-25 mushrooms mapped in the [source dossier](../zone-stories/NEXUS_IC.md#residents-objects-and-reset-behavior).

Bounded external scans found no QST item command, SHP product, story-sidecar VNUM, or ZON reset target in the room/mobile/object ranges; no external mobile or object definition duplicates those local prototype VNUMs; no external type-25 destination points into the rooms; and no compiled C/C++ numeric prototype reference was found. These literal source scans do not qualify editor, administrative, installed, computed, or other runtime references.

## All-442 title and followed history

After color stripping, “The Reliquary Nexus of Aggression” has one exact owner among all 442 current .zon files: active nexus_ic ([title source](../../../areas/zon/nexus_ic.zon#L2)). Followed nexus_ic.wld history begins and ends at the 2009-08-12 area-tree move, commit 642da5400. The MOB and ZON paths follow that move through the 2015-10-13 QITEM flag overhaul, commit f18992b8f; the OBJ path’s latest reviewed edit is 2013-11-13, commit 71d731c5b. There is no local QST, SHP, or story-sidecar path to follow. The reviewed paths and [duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md) establish no alternate-stem predecessor, tracked rename, or explicit whole-zone replacement.

The row immediately after Nexus IC is active nexus_island, whose color-stripped title is “The Reliquary Nexus of the Corsairs.” It is a distinct stem/title and not evidence that Nexus IC was renamed or replaced.

## Retention disposition

Keep active nexus_ic **DO NOT MOVE** as a current un-commented areas/AREA contributor. Preserve the four reciprocal Surface pairs, the separate WLD component, and the eight local mushroom destinations pending owner decisions. Keep Duris3 **RETAIN IN PLACE** with its 176 inactive source-only approaches on candidate row 36. No active route, candidate count, migration, or source change is proposed.

## Checkpoint 310: twenty direct builder questions and recommendations

1. Are all four reciprocal Surface crossings intended as regular Nexus entrances and exits? Recommendation: identify the intended entry points and wayfinding while preserving each reciprocal pair.
2. What do the red arrows on the four boundary descriptions signify? Recommendation: confirm whether they are signs, portal imagery, or pointers to a deeper objective.
3. Is the detached 24-room WLD component intentional? Recommendation: identify its intended entry, exit, and return route without assuming the mushrooms provide safe passage.
4. Are the 64 ZON door resets intended to preserve the maze’s closed/locked puzzle state? Recommendation: name the required doors, keys, and reset-open/closed state room by room.
5. What is the intended role of the luminous shifting rune and blocked ice passages? Recommendation: identify the intended clue and successful player action before adding story credit.
6. Do the mirror-like crystals intentionally confuse navigation? Recommendation: state whether they are atmosphere or part of a solvable route clue.
7. Are the eight type-25 mushrooms intended as a paired travel system? Recommendation: specify intended source, landing, command, and return path for each fixed destination.
8. Several mushroom extra descriptions say “room 72” or “room 80.” Which exact rooms do those labels mean? Recommendation: reconcile them with the VNUM destinations before changing text or behavior.
9. Should players be able to leave each Barren Trap room safely? Recommendation: identify the intended exit, rescue, or consequence for every trap destination.
10. What event, if any, should happen in the central room #130199? Recommendation: decide whether its horror/ecstasy description is atmosphere or a real effect, then name the effect and its bounds.
11. What is the source or purpose of the power gathered at The Nexus? Recommendation: establish the intended antagonist, force, or unresolved mystery before adding a cause.
12. Is the Viking Knarr meant to represent a continuing afterlife rather than a one-time scene? Recommendation: define who qualifies as honored dead and whether return-to-life language describes reset flavor or an actual mechanic.
13. What role should the valkyries have in admitting, guiding, or returning travelers? Recommendation: identify an actual contact and successful outcome before crediting the journey.
14. Why were the ice trolls, giants, and kabouters taken from their homes? Recommendation: clarify captor, agency, and intended escape or reconciliation outcomes.
15. Is Skadi’s stationary ocean-watcher role intentional? Recommendation: specify whether she is an observer, guardian, or interaction point and what evidence supports that role.
16. Is Nidhoggr’s corpse-seeking behavior a deliberate hazard or characterization? Recommendation: name the affected rooms, player consequence, and fair recovery path if it is a hazard.
17. Are the Mushroom Forest and Barren Trap rooms meant to form four repeated pairs? Recommendation: identify the intended order and distinguish each pair’s purpose before generalizing their story.
18. What is the runed drinking horn’s purpose and rightful recipient? Recommendation: decide whether it is scenery, ship equipment, or a reward; do not infer a contract from its reset placement.
19. Are the frozen troll heart and lizard claw intended as evidence, salvage, or only descriptive remains? Recommendation: identify any intended source, recipient, and accepted result before assigning quest meaning.
20. Is the QST/AREA entry intentional despite the absence of a local QST file and story sidecar? Recommendation: confirm whether the index entry should remain and, if a story is desired, define its contract and saved outcome separately from current source behavior.
