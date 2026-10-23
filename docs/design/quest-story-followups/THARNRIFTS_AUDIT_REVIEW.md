# Tharnadia Rifts: repository-wide connectivity and dependency audit

**Checkpoint 386; physical coverage row 299.** This audit reconciles the
[source environmental map](../zone-stories/THARNADIA_RIFTS.md) with active WLD
topology, the all-source inactive-WLD sweep, reset imports, static typed
references, tracked history, and bounded code matches. Authored edges and
resets do not establish a live route, current reset episode, actor access,
safety, or a played journey.

## Active WLD graph

The active WLD has 5,783 rooms and 22,252 authored exit records. Of these,
22,225 target local active rooms, 25 target active neighboring zones, and two
positive directions target VNUMs found only in inactive Duris3. The normal
active loader therefore resolves 22,250 directions and prunes those two
targets. The seven weak components contain 1,963, 1,846, 1,124, 845, 3, 1,
and 1 rooms. Every exit flag is zero; there are no door directions, local D
resets, fall fields, or current fields.

The 74 active inbound directions lead to 74 distinct rooms: 73 from
Caertannad and one from Heavens. The Heavens edge is #1204 down to #110000
([cross-area register row 18](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L18)).
Caertannad #78548 north to #119592 is one example marked reciprocal in the
register ([row 922](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L922)). Of the 73
Caertannad arrivals, the other 50 have no reciprocal local WLD direction;
the full register retains each source room, direction, and return flag.

Twenty-five active directions leave the map. Twenty-three go to Caertannad
and have a reciprocal neighboring WLD direction. Two go one way to
SurfaceKeeps #120000: #110099 east
([row 1190](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1190)) and #119900 south
([row 1214](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1214)). No SurfaceKeeps return
edge is recorded.

Raw directed traversal from the 74 arrival targets reaches 5,349/5,783 rooms.
The other 434 are outside that WLD-only traversal. Reverse traversal from the
25 outbound source rooms finds 5,781/5,783 rooms can reach an active exit;
#110000 and #112545 cannot. Room #110000 is itself the Heavens arrival target
but has no local WLD exits. Room #112545, the Fotenak Hellhammer monolith,
has no known active arrival or local WLD exits. These are authored-edge
counts; they do not include hidden crevices, resets, generic systems, live
actor state, or inferred connections.

The two positive unresolved directions are #116412 west to #116411 and
#116438 east to #116439
([unresolved row 319](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L319),
[row 320](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L320)). Both targets occur only
in inactive Duris3 sources and are pruned by the full active-world loader.

## Inactive WLD sources and map identity

The all-95 inactive-WLD scan parsed 2,080,773 exit records without issues. Five
inactive files contain a target into the active Rifts room set. Across them,
23,136 directions target one of the 5,783 active rooms:

| Inactive source | Directions into Rifts rooms | Classification |
| --- | ---: | --- |
| Duris3 | 23,132 | 1,323 begin at inactive source-room VNUMs with no active WLD owner; 21,809 begin at VNUMs also declared by active contributors. |
| bb_gh | 1 | Source #30320 up to #111255; its source VNUM has no active WLD owner ([source](../../../areas/wld/bb_gh.wld#L346)). |
| circlegh | 1 | Source #45417 up to #110072; its source VNUM has no active WLD owner ([source](../../../areas/wld/circlegh.wld#L271)). |
| cabal | 1 | Source #29001 up to #111079 is already owned by an active WLD contributor; excluded from source-only approaches. |
| rf_gh | 1 | Source #45417 up to #111082 is already owned by an active WLD contributor; excluded from source-only approaches. |

The Cabal-specific source context and Torg ownership of room #29001 are documented in the
[Checkpoint 458 Cabal Guildhall audit](CABAL_GUILDHALL_AUDIT_REVIEW.md#static-wld-graph-and-outbound-records).

Duris3 reuses all 5,783 active Tharnrifts room numbers, but all 5,783
normalized room titles differ. Its 1,323 source-only directions come from
1,073 unique source rooms and target 1,040 distinct active rooms; direction
counts are 508 north, 181 east, 453 south, and 181 west. Preserve all 1,323
on the existing [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).
Keep the 21,809 shared-source directions as map identity evidence, not distinct
approaches. The extra bb_gh and circlegh edges raise this checkpoint's
source-only approach increment to 1,325. None is active ingress.

The two active Tharnrifts edges into inactive-only Duris3 targets remain part
of the candidate's existing 26 unresolved active exit records and 14 active
source-area keys; the candidate counts do not change. The curated source-only
WLD total rises from 8,148 to 9,473. The reviewed Surf subset remains 291.
The approach findings are also retained in the [connectivity register](WORLD_CONNECTIVITY.md#checkpoint-386-tharnadia-rifts-directed-reach-and-inactive-approaches)
and [in-place retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-386-tharnadia-rifts-and-duris3-retention).

## Typed resets, story links, and code references

The ZON header is reset mode 0. Its 62 reset commands comprise 39 M and 23 O
placements; no local QST contract is recorded. The MOB and OBJ files are empty
and contain no local prototypes. Every placed mobile or object is imported:

| ZON reset | Source owner | Exact source locations |
| --- | --- | --- |
| Eight O #78457 half-eaten corpses | caertannad.obj | #110726, #111123, #111335, #111717, #111840, #112233, #112316, #112639 |
| One O #78503 hidden crevice | caertannad.obj | #113375; type-25 destination #78848 |
| Five O #78409 skeletal remains | caertannad.obj | #115679, #116065, #116160, #118296, #119486 |
| One O #78455 silverleaf | caertannad.obj | #116025 at 65% |
| Eight O #94720 “bits bones” | minopass.obj | #116757, #116859, #116956, #117258, #117361, #117455, #117659, #117856 |
| Twenty-four M #78467 starving spike boars | caertannad.mob | Three repeated commands at each of the eight corpse sites |
| Fifteen M #78412 angry vultures | caertannad.mob | Three repeated commands at each of the five remains sites |

The source reset commands are listed in
[tharnrifts.zon](../../../areas/zon/tharnrifts.zon#L10). Prototype ownership
stays with Caertannad OBJ #78457 ([line 844](../../../areas/obj/caertannad.obj#L844)),
#78409 ([line 126](../../../areas/obj/caertannad.obj#L126)), #78455
([line 820](../../../areas/obj/caertannad.obj#L820)), and #78503
([line 1505](../../../areas/obj/caertannad.obj#L1505)); Caertannad MOB #78412
([line 183](../../../areas/mob/caertannad.mob#L183)) and #78467
([line 1023](../../../areas/mob/caertannad.mob#L1023)); and Minopass OBJ #94720
([line 318](../../../areas/obj/minopass.obj#L318)). The hidden crevice leads
from the Rifts placement into Caertannad #78848, the secret cave route described
in the [Twin Keeps dossier](../zone-stories/TWIN_KEEPS_OF_DEVASTATED_THARNADIA.md#access-nested-sources-and-physical-evidence).
The #94720 remains are covered by the
[Deep Ravine source audit](DEEP_RAVINE_AUDIT_REVIEW.md#typed-references-and-compiled-numeric-matches).
Caertannad's story sidecar names #78455 as a Tharnadia Rifts silverleaf
source, while noting that the mixed coin-and-item payment for Mungir is
currently unavailable under active accounting
([story entry](../../../areas/story/caertannad.story.json#L557)).

The QST index lists tharnrifts at
[areas/qst/AREA line 324](../../../areas/qst/AREA#L324), but no
areas/qst/tharnrifts.qst or local story sidecar exists. No local MOB/OBJ
prototype exists for an external same-type owner match. The scan of 872
external type-25 object declarations found no destination into the active
Tharnrifts room set. The local #78503 type-25 object is an outbound reference
to Caertannad #78848, not an inbound path from outside.

The compiled source review found these name and numeric-range matches:

| Reference | Classification |
| --- | --- |
| [mining.c](../../../src/economy/mining.c#L56) / [mining_config.c](../../../src/economy/mining_config.c#L28) | An explicit “Tharnadia Rift” mining band spans #110000–#119999. It is a system region range, not an authored WLD route. |
| Baltazo mobile #500129 | The active binding is in specs.assign.c; the mobile is reset at Surface #568439
([ZON reset](../../../areas/zon/surface.zon#L660)). The old “speed wipe entry to Tharn Rifts” comment does not match its current handler, which emits cosmetic text and rejects some attack commands. The death-and-teleport block is commented out ([binding](../../../src/specs/specs.assign.c#L182), [handler](../../../src/specs/specs.zion.c#L70)). |
| tharnrifts_portal / object #500055 | Despite its procedure name, this is the Surface Realm shadowrift reset at #568439
([reset](../../../areas/zon/surface.zon#L32)). Its active handler targets Tharnadian Ruin #5569 or #5583 by race-war side ([binding](../../../src/specs/specs.assign.c#L1277), [handler](../../../src/specs/specs.zion.c#L37)). |
| elfgate generic range | Its handler can randomly select from a broad #110000–#209999 range, but the only listed object bindings (#8112/#8113) are inside a comment that says the objects do not exist ([handler](../../../src/specs/specs.underworld.c#L1691), [inactive assignments](../../../src/specs/specs.assign.c#L1844)). This is not an active Tharnrifts-specific route. |
| Other 11xxxx code matches | Mining, map, ship, and room-range checks use broad numeric bands; no additional fixed destination into a Tharnrifts room was found. |

## Title, history, and disposition

After color-code and terminator removal, whitespace normalization, and case
folding, the [ZON title](../../../areas/zon/tharnrifts.zon#L2) is “Tharnadia
Rifts,” unique among all 442 current ZON titles and used by one of 351 active
contributors. Same-type active WLD comparisons find no duplicate room VNUM.
There are no local mobile or object prototypes to collide with active owners.
The inactive Duris3 title differences are source-map identity evidence, not
an active duplicate or predecessor claim.

Followed WLD/ZON history stays on the tharnrifts stem; the visible shared
history includes the 2009 area-tree path move and the ZON quest-item flag pass.
It establishes no alternate-stem rename, distinct predecessor, or whole-area
replacement. Keep current active tharnrifts **DO NOT MOVE**. Preserve
inactive Duris3 **RETAIN IN PLACE** on candidate row 36. No new candidate or
native source change is warranted by these source references.

## Builder questions and recommended resolutions

| # | Builder question and source evidence | Recommended resolution |
| --- | --- | --- |
| 1 | Every one of the 5,783 room descriptions is blank ([WLD](../../../areas/wld/tharnrifts.wld#L1)). Is the title-only presentation intentional? | Confirm whether the repeated title palette is the full intended ambient text. Add room descriptions only if a builder supplies that intent. |
| 2 | The WLD has 41 normalized title families ([title map](../zone-stories/THARNADIA_RIFTS.md#room-title-map)). Do those families delineate planned regions or only recurring terrain labels? | Preserve titles as labels; document region boundaries only from a builder map or route plan. |
| 3 | Eighteen rooms are titled “Zone Entrance” ([title map](../zone-stories/THARNADIA_RIFTS.md#room-title-map)). Which boundary or intended approach does each title refer to? | Identify them from the intended map before changing labels or treating them as portals. |
| 4 | The local graph has seven weak components. Are the 1,963-, 1,846-, 1,124-, 845-, 3-, 1-, and 1-room components intentional? | Confirm intended map islands and pockets; do not connect components only to improve graph statistics. |
| 5 | Raw WLD traversal from active arrivals reaches 5,349/5,783 rooms. What should account for the other 434? | Identify intended entrances, item routes, and isolated map areas before adding exits. |
| 6 | #110000 is a Heavens arrival with no local WLD exit ([room](../../../areas/wld/tharnrifts.wld#L1), [inbound register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L18)). Is it a deliberate endpoint or a missing return? | Preserve it as an endpoint until the area owner confirms its intended continuation. |
| 7 | #112545 is a disconnected Fotenak Hellhammer monolith ([room](../../../areas/wld/tharnrifts.wld#L40376)). Should any active path or reset lead there? | Confirm whether it is scenery, an admin landmark, or a player destination; make no route inference from its title. |
| 8 | Seventy-three Caertannad arrivals coexist with 23 reciprocal Caertannad returns ([sample inbound row 922](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L922)). Are the other arrival links intended one-way landings? | Review each source pair in the cross-area register and retain the current directions until the owner confirms edits. |
| 9 | #110099 east and #119900 south leave one way for SurfaceKeeps #120000 ([rows 1190](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1190) and [1214](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1214)). Are missing returns intentional? | Confirm SurfaceKeeps access and the intended return route; do not add reciprocal links automatically. |
| 10 | #116412 west to #116411 and #116438 east to #116439 target only inactive Duris3 records. Are these stale exits or intended replacement targets? | Keep them unresolved in source documentation; request a destination decision before changing either edge. |
| 11 | Duris3 reuses every active room VNUM but all paired normalized titles differ. Does it represent a separate map identity? | Preserve both maps under their current owners and keep the identity evidence on candidate row 36. |
| 12 | Duris3 contributes 1,323 source-only directions; bb_gh and circlegh add one each. Should those inactive approaches ever be restored? | Keep the sources inactive and RETAIN IN PLACE; restoration requires a separate owner decision and map validation. |
| 13 | All 62 resets import Caertannad or Minopass prototypes. Should Rifts population and scenery remain foreign-owned? | Preserve current prototype ownership and coordinate future resets with those source owners. |
| 14 | Reset mode is zero even though the [ZON header](../../../areas/zon/tharnrifts.zon#L1) has 62 M/O commands. What reset availability is intended? | Confirm the desired reset lifecycle from runtime and owner intent before editing the mode or promising fresh stock. |
| 15 | [areas/qst/AREA](../../../areas/qst/AREA#L324) lists tharnrifts without a tharnrifts.qst. Is that future-work membership or stale index state? | Have the owner reconcile the index with intended QST content; do not invent a quest file. |
| 16 | #78455 silverleaf is reset at #116025 and named by Mungir's sidecar service ([story entry](../../../areas/story/caertannad.story.json#L557)), whose mixed payment is unavailable. Is the Rifts source still part of the intended service? | Preserve the source and separately qualify service settlement and reset supply before describing availability. |
| 17 | #78503 at #113375 points into Caertannad #78848 ([prototype](../../../areas/obj/caertannad.obj#L1505)). Is this hidden-crevice item route the intended connection to the pirate cave? | Keep the typed destination and owner intact; validate live item presence and access before describing a usable route. |
| 18 | Skeletal remains are _noquest_, and Minopass #94720 bones are documented as scenery ([Deep Ravine audit](DEEP_RAVINE_AUDIT_REVIEW.md#typed-references-and-compiled-numeric-matches)). Are these all meant to remain non-proof scenery? | Keep them out of story accounting unless the builder names exact accepted evidence. |
| 19 | Mining source names a #110000–#119999 region ([configuration](../../../src/economy/mining.c#L56)). Is mining intended throughout this full numeric band? | Preserve the configuration range and ask the system owner to confirm any future room-range changes. |
| 20 | Baltazo and tharnrifts_portal suggest travel, while their current bindings do not enter this map ([Baltazo](../../../src/specs/specs.assign.c#L182), [portal](../../../src/specs/specs.assign.c#L1277)). Are their old labels/comments stale? | Correct naming or comments only after the responsible code owner confirms intent; do not infer a route from the symbol names. |

This is a source audit only. It does not change gameplay, world topology,
prototype ownership, reset mode, QST indexes, or attic disposition, and no
tests, build, server, database, or operational script were run.

## Checkpoint 779: Caertannad typed dependency

Active Tharnadia Rifts has 39 mobile and 15 object reset rows into Caer Tannad, covering #78412/#78467 and #78409/#78455/#78457/#78503. See [Caertannad inbound consumers](CAERTANNAD_AUDIT_REVIEW.md#active-inbound-consumers).

## Checkpoint 841: Current typed ownership and boundary refreshed

The active Rifts reset file still has 62 M/O commands (39 mobile and 23
object placements), all importing prototypes from Caertannad or Minopass. The
latest bounded inbound pass remains Caertannad's 39 mobile and 15 object reset
rows; its other eight object placements are Minopass OBJ #94720 scenery.
Preserve the prototype owners and reset commands as a separate typed layer.
([Rifts reset](../../../areas/zon/tharnrifts.zon#L10),
[Caertannad inbound-consumer inventory](CAERTANNAD_AUDIT_REVIEW.md#active-inbound-consumers),
[Minopass object](../../../areas/obj/minopass.obj#L318))

Caertannad's story sidecar continues to name its local OBJ #78455 silverleaf as
a Rifts source, while its mixed payment is unavailable under active accounting.
The imported Caertannad OBJ #78503 remains placed at Rifts room #113375 and
points to Caertannad #78848; it is an item-defined route into Caertannad, not a
WLD edge. The source scan still finds no local QST file or story sidecar despite
the `qst/AREA` row, and no external type-25 destination into a Rifts room.
([Caertannad story entry](../../../areas/story/caertannad.story.json#L557),
[hidden-crevice object](../../../areas/obj/caertannad.obj#L1505))

The 74 active inbound WLD records, 25 outbound records, two Duris3-only
unresolved targets, and all-95 inactive-source classification remain as
reported under Checkpoint 386. Duris3's same-VNUM rooms have different titles;
retain 1,323 distinct-source approaches on its existing candidate row 36 and
exclude the 21,809 directions from overlapping active-owned source VNUMs.
No prototype move, map activation, or replacement relationship follows from
these reset and room-number relationships.
