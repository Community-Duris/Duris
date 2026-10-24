# The Reliquary Nexus of the Corsairs: repository-wide route, dependency, and history review

This bounded supplement accompanies the [comprehensive source dossier](../zone-stories/NEXUS_ISLAND.md). It reconciles active WLD reachability and hazards, the inactive-map sweep, typed dependencies, normalized title ownership, followed same-stem history, and retention. Static source evidence does not establish actual access, fall outcomes, resets, or gameplay credit.

## Active WLD graph and Surface boundary

Active nexus_island has 100 rooms, #130300–#130399, and 277 authored exit records. The loader resolves 275; two D5 exits have destination -1 and are dropped. The remaining 274 resolving exits are local and one leads out to Surface. The sole boundary pair is reciprocal: #130301 south to Surface #519153 and #519153 north to #130301 ([outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1295), [inbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1355)).

All 100 rooms form one weak component. In raw directed traversal from inbound room #130301, all 100 rooms are reachable and all 100 have a path back. This graph pass ignores door state and fall consequences. There are 39 fall markers: five at 30, twelve at 40, seven at 50, five at 60, two at 80, five at 90, and three at 100. No WLD current marker is present. The two unresolved exits are #130342 down and #130348 down, both targeting -1 ([source records](../../../areas/wld/nexus_island.wld#L1114), [second source record](../../../areas/wld/nexus_island.wld#L1264)).

## Inactive WLD sweep and Duris3 source-only references

The all-95 inactive-WLD sweep traversed 527,791 rooms and 2,080,773 exits. Six extra standalone tilde lines occur after descriptions in unrelated inactive waterplane rooms #5–#10; the scanner treated them as empty terminators and continued. No other structural parsing error was reported.

There are 400 inactive exit records targeting #130300–#130399, all in unregistered Duris3.wld. A total of 279 begin at VNUMs already owned by active areas and are excluded from source-only approaches: 198 records from same-numbered Duris3 map rooms #130300–#130399, 80 from active nexus_ic VNUMs #130100–#130179, and one from active nexus_roper VNUM #130400. The same-numbered Duris3 records do not prove duplicate ownership or replacement.

The remaining 121 records begin outside active room ownership:

| Duris3 source rooms | Exits into Nexus Island | Direction and target | Evidence |
| --- | ---: | --- | --- |
| #130600–#130699 | 100 | north to #130300–#130399 in numeric order | [First target line](../../../areas/wld/Duris3.wld#L642608), [last target line](../../../areas/wld/Duris3.wld#L644687) |
| #130599 | 1 | east to #130300 | [Target line](../../../areas/wld/Duris3.wld#L642591) |
| #130080–#130099 | 20 | south to #130380–#130399 in numeric order | [First target line](../../../areas/wld/Duris3.wld#L631696), [last target line](../../../areas/wld/Duris3.wld#L632095) |

These records are inactive source-only references, not installed approaches. Preserve them on existing Duris3 candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), without changing its 26 active exit-target count.

## Typed dependency closure

The local inventory is WLD/MOB/OBJ/ZON only. No nexus_island QST, SHP, or story sidecar exists; areas/qst/AREA still lists the area at [line 329](../../../areas/qst/AREA#L329). The local ZON has 54 mobile placements, two follower commands, four door resets, six equipment placements of local dagger #130302, and no O object-placement command.

Bounded exact numeric scans found no external QST item command, SHP product, story-sidecar VNUM, or ZON reset reference to the room/mobile/object ranges; no external MOB/OBJ prototype duplicates the local VNUMs; no external type-25 object points into the room range; and no compiled C/C++ numeric prototype reference was found. These scans do not qualify editor, administrative, installed, computed, or other runtime references.

## All-442 title and followed history

After stripping color codes, “The Reliquary Nexus of the Corsairs” belongs uniquely to active nexus_island among all 442 current .zon files ([title source](../../../areas/zon/nexus_island.zon#L2)). Followed nexus_island.wld history begins and ends at the 2009-08-12 areas-tree move, commit 642da5400. The MOB and ZON paths continue through the 2015-10-13 quest-item flag overhaul, commit f18992b8f; OBJ’s latest reviewed change is 2013-11-13, commit 71d731c5b. There is no local QST, SHP, or story-sidecar path to follow. The reviewed same-stem history and [duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md) identify no alternate-stem predecessor, tracked rename, or explicit whole-zone replacement.

The next register row is active nexus_uc, titled “The Reliquary Nexus of The Dead.” This is a separate title and source stem, not evidence that Nexus Island was renamed or replaced.

## Retention disposition

Keep active nexus_island **DO NOT MOVE** as a current un-commented areas/AREA contributor. Preserve the reciprocal Surface boundary, raw 100-room reach/return, four door resets, both unresolved down exits for builder review, and all 39 fall markers. Keep Duris3 **RETAIN IN PLACE** with its 121 inactive source-only references on candidate row 36. No active route, candidate count, migration, or source change is proposed.

## Checkpoint 311: twenty direct builder questions and recommendations

1. Is #130301 the intended island entry from Surface? Recommendation: confirm its role as the regular arrival and return point while preserving the reciprocal pair.
2. Should the local story open at #130300 or #130301? Recommendation: clarify the beach-to-jungle sequence without changing the current traversable route.
3. Which account of the king-bound treasure is canon? Recommendation: decide whether the castaways stole it, whether the corsairs only believe that, or whether the dispute is intentionally unresolved.
4. What is the treasure and what proof would settle its custody? Recommendation: identify an exact object/source and an accepted successful result before adding a hand-in.
5. What do the magical walls around both settlements do? Recommendation: name their boundaries, admission rules, and lasting purpose before treating the prose as a working shield.
6. Is the Chief meant to negotiate, command, or fight every outsider? Recommendation: define an addressed contact and fair admission separately from the current defensive description.
7. Is Bella meant to capture or kill all non-corsairs? Recommendation: clarify intended prisoner outcomes and recovery before making her a mandatory gate.
8. Are the hunting cat and trained dog intended to remain with their reset followers? Recommendation: confirm that the two follower resets reflect lasting loyalty and state how their movement should work.
9. Are the toothy fish hazards, food sources, or background wildlife? Recommendation: specify affected water rooms and any fair warning or recovery if they create a lethal encounter.
10. How should players pass the nesting giant birds? Recommendation: decide whether the nests can be avoided, investigated, or safely crossed without requiring an unstated kill.
11. What do the jungle paths, blocked clearings, and glimpsed rowhouses mean? Recommendation: identify the intended settlement route and mark blocked paths that are deliberate.
12. What force twisted the normally timid lemurs? Recommendation: confirm its connection to the central stone and define whether the intended outcome is study, rescue, removal, or continued danger.
13. Who or what are the beetlemen, and why do they occupy the tunnel? Recommendation: define their relation to the crater and whether they are defenders, victims, or independent residents.
14. What is the “correct method” for opening the magical wooden slat door at #130383/#130384? Recommendation: reconcile the prose with the closed, apparently unkeyed reset and name the intended discovery.
15. Why is dagger #130302 equipped in both hands on three giant lemurs? Recommendation: confirm its combat purpose and whether players should encounter or recover it; do not infer a quest from equipment placement.
16. What does the floor carving in #130398 depict, and how does it relate to the stone in #130399? Recommendation: state whether they are one mechanism or separate signs before connecting their story credit.
17. Is the #130398/#130399 closed/secret trapdoor the intended route to the Nexus? Recommendation: specify how players discover it and how they can return after entry.
18. What happens when one of the 39 fall markers triggers? Recommendation: define destination, damage/death policy, protection expectations, and recovery for each hazard band before requiring survival.
19. Are the #130342 and #130348 D5 exits to -1 unfinished paths into the jungle or intentional dead ends? Recommendation: confirm destination and reciprocal return before editing either exit.
20. Is the QST/AREA entry intentional despite the absence of a local QST, SHP, and story sidecar? Recommendation: confirm whether the index entry should remain and define any future contract and saved outcome separately from current source behavior.
