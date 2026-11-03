# The Mountain of Peril Peaks: repository-wide audit review

Checkpoint 309 follows [coverage row 207](ZONE_COVERAGE_REGISTER.csv#L207). This supplement extends the [comprehensive source story map](../zone-stories/THE_MOUNTAIN_OF_PERIL_PEAKS.md) and [source evidence index](../../reference/zone-story-audits/nexus.md) with the directed WLD graph, inactive-map sweep, bounded typed references, all-442 title/history result, and twenty direct builder questions. These are static repository findings; they do not qualify live doors, reset execution, account readiness, current inventory, combat outcomes, safe travel, or player-specific history.

## Active boundaries and directed WLD graph

The active map has 196 rooms and 380 authored exit declarations. The source-resolved census has 379 exits; the remaining declaration is room #57553 D8 to negative target -1, excluded by the loader ([source](../../../areas/wld/nexus.wld#L1292), [unresolved register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L252)). The local undirected graph has five weak components of 188, 4, 2, 1, and 1 rooms.

Two boundaries reciprocate with Mountain Tracks:

| Peril Peaks exit | Mountain Tracks exit | Register evidence |
| --- | --- | --- |
| #57536 down to #21144 | #21144 up to #57536 | [Peril Peaks](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L809), [Mountain Tracks](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L355) |
| #57637 west to #21015 | #21015 east to #57637 | [Peril Peaks](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L810), [Mountain Tracks](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L353) |

Both boundary rooms belong to the 188-room weak component. Restricting paths to
Peril Peaks WLD edges, #57536 reaches 66 rooms; 95 rooms can return to #57536,
and 66 are reachable in both directions. From #57637, 37 rooms are reachable,
52 can return, and 37 are in both sets. The other four weak components have no
WLD path to either boundary. These counts exclude teleport objects, Mountain
Tracks transit, doors, environmental effects, wandering, and runtime
eligibility. The source map also includes falling properties and a current
record, so graph membership alone is not a safe route claim.

## Inactive WLD source sweep

The targeted sweep parsed all 95 WLD files whose stems are outside the active
area manifest. It found no duplicate room VNUM in #57500–#57695 and no exit from
an inactive source room into Peril Peaks. No source-only approach or candidate
row is added. The two Mountain Tracks edges above are active reciprocal exits,
not attic ingress.

## Typed dependencies and ownership

The local area has ten item-only deliveries from eight QST giver records,
eleven addressed M families, 297 reset commands, and two actual shop blocks.
The ten native outcomes, exact accepted items, optional history, local source
caps, keys, portals, and shared travel behavior remain mapped in the source
dossier. Coverage row 207 had a zero/blank SHP inventory despite the source
file; this checkpoint corrects it to #57537–#57560, count two. The keepers are
#57537 and #57560.

Targeted exact-number scans of other current source files found no external QST
item command, SHP product, structured story-sidecar VNUM, or ZON reset room or
prototype reference into the Nexus ranges. No other mobile or object source
redefines a local VNUM. No foreign type-25 object targets a Peril Peaks room.
The only three exact-number appearances in other object records are numeric
value fields in the ice ball (#34560, [prototype](../../../areas/obj/4horse.obj#L956)),
vortex axe (#32600, [prototype](../../../areas/obj/eth2.obj#L1)), and seashell
bra (#67307, [prototype](../../../areas/obj/sea.obj#L61)); they are not teleport
destinations.

The token scan of compiled sources found one exact Nexus object literal:
#57576, the local prismatic halo, appears in the CHAOS enhanced-illusionist kit
table ([prototype](../../../areas/obj/nexus.obj#L1091), [kit row](../../../src/account/chaos_eq_data.h#L503)). This is a shared equipment reference, not a native quest outcome or personal encounter record. The bounded source scan does not establish editor/tooling consumers, external persisted references, or live operation.

## Checkpoint 309: all-442 title and followed history

After color stripping, “The Mountain of Peril Peaks” is unique among all 442
current `.zon` titles. The followed WLD/MOB/OBJ/ZON/QST/SHP histories remain on
the `nexus` stem, with their tracked paths reaching the 2009 area-tree move.
WLD, MOB, OBJ, ZON, and SHP have later same-stem edits through 2026-05-05;
QST history reaches 2013-11-13. The revision-one story sidecar was added on
2026-10-04 (`d286a793d`). This bounded history finds no alternate-stem
predecessor or explicit whole-zone replacement. Prior all-442 review already
records that Peril Peaks, the Mountain of the Banished, and Mountain Tracts
have distinct exact titles. See the [title and history log](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-309-peril-peaks-all-442-title-and-history),
[title source](../../../areas/zon/nexus.zon#L2), and [coverage row 207](ZONE_COVERAGE_REGISTER.csv#L207).

The matching source-key family does not identify one shared zone. Separate
unregistered candidates `nexus56` (exact title “Nexus”) and `Nexus_1` (exact
title “Nexus_1”) have their own low-VNUM source histories and candidate audits;
neither audit finds a successor or whole-zone replacement. The active
`nexus_ic`, `nexus_uc`, `nexus_island`, and `nexus_roper` entries have distinct
Reliquary titles and separate current roster keys. Keep the two candidates
retained under their existing identities, and leave any command alias or
ownership decision open rather than treating the shared “nexus” wording as
replacement evidence ([Nexus56 audit](NEXUS56_AUDIT_REVIEW.md), [Nexus_1 audit](NEXUS_1_AUDIT_REVIEW.md), [candidate rows](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79-L80)).

## Retention disposition

Keep active `nexus` DO NOT MOVE. Preserve the two reciprocal Mountain Tracks
boundaries, five WLD components, the unresolved negative D8 record, local
delivery identities and source caps, the persistent rune medallion, distinct
emerald controls, and the existing four local teleport commands. Coverage row
207 now lists the two real shops. The inactive-WLD sweep adds no approach or
attic candidate; no route or migration clearance is added. The comprehensive
source dossier remains the narrative record.

## Checkpoint 309: twenty direct builder questions and recommendations

1. **Should the ten native deliveries remain ten distinct story outcomes rather than one completion flag?** Recommendation: retain each exact accepted input, reward, and receipt as a separate outcome unless an owner defines a new campaign endpoint.
2. **Should receiving a matching item from another player remain sufficient for a delivery?** Recommendation: keep the current exact item proof and do not imply that the recipient personally killed or first recovered its producer.
3. **Should the Foreman's silver bag and Roxon's stud remain a linked but optional two-step errand?** Recommendation: preserve the two separate contracts and allow the supplied stud to satisfy Roxon's accepted input without requiring earlier receipt history.
4. **Should the tribal barbarian and Gooran chain continue to accept the delivered sasquach parchment without a prior trophy receipt?** Recommendation: retain the authored item proof while describing the earlier exchange as useful history rather than mandatory progress.
5. **Which reward should Gooran give: the promised Bel'Rok ring or the current prayer book #57511?** Recommendation: have the builder settle the intended reward and align dialogue and accepted outcome in one named content change.
6. **Should the silver bag retain its split `fu ll` aliases when clearer phrases are added?** Recommendation: add usable bag aliases while preserving existing commands so old player habits continue to work.
7. **Should the stud prototype #57523 accept `stud` as a command alias in addition to `earring`?** Recommendation: add the clearer alias without removing the existing one.
8. **Should the kraken tentacle #57566 keep its `tentacly` alias, or should a clearer form be added?** Recommendation: retain the existing alias and add a builder-approved synonym rather than changing required-item identity.
9. **Is the snow-bear fur supply intentionally capped at two while three snow-bear placements are declared?** Recommendation: preserve the global cap until the owner confirms whether contention is intended, and avoid a guaranteed daily-stock claim.
10. **Should other materials keep their current shared reset caps and source placements?** Recommendation: review each source and cap as authored; qualify depletion and renewal before promising a repeatable hunt.
11. **Should hunting dialogue remain separate from evidence of a personal kill?** Recommendation: keep dialogue, reset stock, and accepted item receipts as separate facts unless a new committed kill record is designed.
12. **Should the companion investigation remain an unfinished question after the injured traveler receives the medallion?** Recommendation: preserve the current endpoint and design a separate accepted search/report/rescue result before promising a conclusion.
13. **Should rune medallion #57504 remain a persistent key for the reciprocal white-washed gates?** Recommendation: keep its key identity and distinguish unlocking from opening; do not make the gates free or consumed by the receipt without owner review.
14. **How should the two ghost-held emerald controls become available to the player?** Recommendation: qualify the exact holder-death and item-exposure path, preserve the distinct emerald identities and target exits, and do not describe a carried NPC gem as directly touchable.
15. **Should the two emeralds continue to control separate courtyard and tower exits?** Recommendation: preserve the exact #57558/#57567 control-to-exit mapping and qualify both actions independently.
16. **Should small iron key #57556 remain unused while the tower exits it is named beside are already open non-doors?** Recommendation: remove misleading key references or approve a separately designed lock; do not silently activate a new gate.
17. **Should the unused cave portal prototypes, `MREPORT` exit, and tower plaque become supported content?** Recommendation: let the owner choose which are intended, then add an explicit placement and verified command/return path before presenting any as usable clues or travel.
18. **Should the four active travel objects retain their exact `LOOK`, `TOUCH`, and `ENTER` commands and destinations?** Recommendation: document each object and command separately, and qualify arrival and return before calling any route safe or guaranteed.
19. **Should the two existing SHP blocks remain the only shops despite weapon-store prose and the weapon master?** Recommendation: keep row 207's corrected two-shop inventory at #57537 and #57560 unless a real third SHP record is deliberately authored.
20. **Which maze, fall, current, and combat hazards should player guidance call out before describing a complete trip?** Recommendation: retain the documented environmental boundaries and confirm accepted movement, arrival, interruption, and return behavior before promising a safe traversal.
