# Siege Historical Audit Review

Reviewed October 12, 2026, Checkpoint 563. This review reconstructs the retired
`siege` area from the parent of retirement commit
`7ddf5432f53fdc9b9c578d7503f4611529b544ef`. It does not restore the deleted
files or qualify any current feature, production record, or live route.

## Current identity and historical source inventory

No current `siege.wld`, `.mob`, `.obj`, `.zon`, QST, SHP, or story-sidecar file
exists. `areas/AREA` reserves the identity as
[`*RETIRED 4010 (siege; do not reuse)`](../../../areas/AREA#L407). There is no
`siege` entry in `areas/qst/AREA`.

The immediate pre-retirement source contained:

| Historical file | Source evidence |
| --- | --- |
| WLD | One room, #401000 “The Siege Room”; its room description is empty and it has no authored exits. |
| MOB | Eleven records: ten regional quartermasters and one Tharnadian militia guard (#401001). |
| OBJ | Empty file; zero object prototypes. |
| ZON | Header #4010, title “Siege,” room ceiling #401000, and no reset commands. |
| QST/story/shop | No `siege` QST, story sidecar, or SHP source. `areas/qst/AREA` has no `siege` row. |

The quartermaster mobiles were #401000 (Tharnadia), #401010 (Woodseer),
#401020 (Ashrumite), #401030 (Charing), #401040 (Kimordril), #401050 (Faang),
#401060 (Ghore), #401070 (Moregeeth), #401080 (Khildarak), and #401090 (Shady).
Their short and long descriptions identify them as town quartermasters; the
only other mobile is a militia guard described as watching beyond the walls.
The WLD itself supplies no environmental history or player objective.

## Story and quest classification

The checked-in text supports a small functional staging room for the regional
quartermasters and a guard. It does not establish a player-facing siege story,
a local quest, or an environmental journey. This is consistent with the
retirement commit's “siege acquisition and world wiring” description, but the
system-purpose interpretation is not additional in-world lore. Do not invent a
quest, connect this room to a neighboring zone, or restore a quartermaster from
the historical files as part of the story audit.

## Historical graph and routes

The retired WLD was a one-node graph: #401000 had zero outgoing exits and was
the only room, so there was no within-zone path to test beyond the isolated
room itself. At the pre-retirement snapshot, the exact #401000 scan found no
other WLD target to this room; the current full and inactive WLD files likewise
contain no target to it. The ten zone-reset imports below placed mobiles in
town rooms. They were not WLD entrances. Current static connectivity adds no
route or source-only total, and no dynamic or live access is inferred.

## Former typed consumers and retirement

Before retirement, ten other zone files reset the ten quartermasters:

| Reset source | Mobile | Reset room |
| --- | ---: | ---: |
| `tharnadia.zon` | #401000 | #132573 |
| `woodseer.zon` | #401010 | #16551 |
| `ashrumite.zon` | #401020 | #66001 |
| `charing.zon` | #401030 | #45000 |
| `kimordril.zon` | #401040 | #95553 |
| `faang.zon` | #401050 | #15263 |
| `ghore.zon` | #401060 | #11703 |
| `goblinht.zon` | #401070 | #70296 |
| `khildarak.zon` | #401080 | #17021 |
| `shady.zon` | #401090 | #97628 |

The old `src/combat/siege.c` assigned its `warmaster` procedure to those ten
quartermaster VNUMs. The retirement commit removed all ten reset commands and
deleted the `siege` source files. Follow-up commit
`bf2ef4ab37d46f9aae8a9a81ccfc4eae60c9009a` removed the dormant Siege runtime
and persistence source, including those bindings. An exact scan of current
`areas/` and `src/` finds no remaining #401000, #401001, or #401010–#401090
source reference.

There is one partial lineage worth preserving: 2014 commit `06d31630e` moved
Charing's Quartermaster prototype from #45095 to Siege #401030. Retirement
removed the Charing reset as well. The Charing review already asks whether that
removal was intentional; see [CHARING_AUDIT_REVIEW.md](CHARING_AUDIT_REVIEW.md).
Do not restore or move the Charing NPC based on the old source alone.

The retirement also removed Kzkrkeep shop products #178 and #179. The retired
`siege.obj` was empty, so those acquisition entries were outside this area's
typed prototype inventory. Keep that cleanup separate from the reserved
4010 identity. Current source scans cannot answer whether production, retained
backups, editors, or other external data still record old identifiers.

## Disposition

There are no current Siege component files to move or archive. Keep zone 4010
reserved exactly as the manifest says; do not restore its WLD/MOB/ZON files,
reuse the old IDs, or create a new candidate row. Any future identifier change
requires a separate owner-approved review of external tooling and persisted
records. No production or persisted-data inspection was performed here.

## Builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| SIEGE-01 | Was “The Siege Room” intended only as staging for the quartermaster system, or was player-facing environmental content omitted? | Record no in-world quest or environmental story from the current evidence; add prose only after the area owner supplies an intended player experience. |
| SIEGE-02 | Should zone number 4010 remain retired and unavailable for reuse? | Preserve the current `*RETIRED 4010 (siege; do not reuse)` manifest row. Do not restore or reassign the number. |
| SIEGE-03 | Were the ten town quartermaster resets and `warmaster` bindings fully retired with the Siege feature? | Keep the removed resets and bindings absent. Any future feature work needs its own gameplay and code review. |
| SIEGE-04 | Was Charing's 2014 Quartermaster move from #45095 to #401030 meant to end with the 2026 deletion of the reset? | Preserve the removal and resolve the owner question already listed in the Charing audit; do not restore or relocate the NPC in this record. |
| SIEGE-05 | Do any editor, map, or external source-management projects still associate content with zone 4010 or the #401000–#401090 IDs? | Keep the number reserved until those owners confirm their indexes; tracked-source absence is not proof about private tooling. |
| SIEGE-06 | Do production or retained player/world backups contain old Siege IDs or acquisition records? | Treat custody as unverified. Require the separately approved read-only inventory before any future identifier reuse or data cleanup. |

These answers preserve the recorded retirement and distinguish tracked source
closure from external ownership. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L477),
the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-563-siege-historical-graph-and-retirement),
the [replacement register](DUPLICATE_AND_REPLACED_ZONES.md#siege), and the
[attic plan](ATTIC_MIGRATION_PLAN.md#explicit-no-move-cases).
