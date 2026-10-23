# Astral placeholder audit

Canonical key: `component:astral`. Physical coverage row 362; attic candidate
row 11. This is a one-room, source-only component absent from both current
`areas/AREA` manifests. The tracked `areas/map/astral.json` is coordinate
metadata, not authored story or proof of runtime map ownership.

## Source inventory and narrative

[`astral.wld`](../../../areas/wld/astral.wld#L1) defines only room #1, titled
“Unnamed,” with a blank description and no exits. The corresponding
[`astral.zon`](../../../areas/zon/astral.zon#L1) has zone header #1, title
“The Horrible Tracts of the Unnamed,” last VNUM 1, reset mode 2, and no reset
commands. The MOB and OBJ files are empty. No `astral` QST, SHP, or structured
story sidecar exists, and the current source has no native Q/QA contract.

[`astral.json`](../../../areas/map/astral.json) contains `zoneId: "astral"`
and one coordinate for room #1 at `(0, 0)`, with a `lastModified` value of
2025-11-30. Its shape records map-editor coordinates only; it does not state
that this WLD is loaded or identify who owns its numeric room. The file first
appears in tracked history in commit `b57048496` (2025-12-28, “pfile migration tool”).

There is no quest or environmental story content to summarize from the WLD,
MOB, OBJ, or ZON records. The ZON title alone does not establish a setting,
plot, or relationship to the other files named Astral.

## Loader and room identity

Neither `areas/AREA` nor `areas/qst/AREA` lists the `astral` stem. They list
three different current sources: `astral_main` (zone 197), `astral_areas`
(zone 198), and `astral_tiamat` (zone 199). Their active membership does not
register this one-room `astral.wld` component.

Room VNUM #1 is explicitly reserved as `ROOM_LIMBO_VNUM` in
[`utils.h`](../../../src/core/utils.h#L637). Active [`limbo.wld`](../../../areas/wld/limbo.wld#L8)
is the sole active WLD owner of #1 and contains a different room record with
a full Limbo description. Repository code uses `real_room(1)` for generic
Limbo destinations in Celestia, magic, and prison flows
([Celestia](../../../src/specs/specs.celestia.c#L100),
[magic](../../../src/magic/smagic.c#L1944),
[prison](../../../src/specs/specs.prison.c#L90)); artifact handling also uses
`ROOM_LIMBO_VNUM` ([artifact code](../../../src/guild/artifact.c#L1291)).
Those numeric references do not identify Astral.

The repository currently has twelve WLD sources defining room #1:
`astral`, `dalvik`, `hostel`, `limbo`, `Magetower`, `mavulsk`, `Mentiri`,
`mtncastl`, `Nexus_1`, `theurgist`, `ud-inside`, and `waterplane`. Only `limbo`
is in the active world list. Astral's entire WLD room block is byte-identical
to the #1 placeholder blocks in `mtncastl.wld` and `ud-inside.wld`; this is a
content match, not proof the three stems share one area identity.

The static source scan finds 23 WLD directions to numeric #1: nine from active
`desert.wld` room #49790 and fourteen from eight other unlisted maps. The
Desert directions resolve by current active ownership to Limbo
([room record](../../../areas/wld/desert.wld#L16945)). The other fourteen
directions likewise cannot be attributed to Astral merely because their
target number is 1. Candidate-specific active ingress to Astral remains zero.

## Static graph and typed dependencies

| Measure | Source result |
| --- | ---: |
| WLD rooms | 1 |
| Authored directions | 0 |
| Weak components | 1, containing room #1 |
| Rooms with fall chance / current | 0 / 0 |
| ZON reset commands | 0 |
| MOB / OBJ / QST / SHP records | 0 / 0 / 0 / 0 |

No local mobile or object prototype exists to receive an external reset,
quest-item, or shop-product dependency. Astral has no reset command or local
WLD boundary. A bounded scan also found no story sidecar. Numeric room #1
appears in other unlisted source maps' own reset records, including the door
reset in `Magetower.zon` and door/mobile resets in `mavulsk.zon`; each source
has its own WLD room #1, so these records do not identify Astral as their
target. Current source code and WLD ownership instead reserve #1 for Limbo.

This is a repository-bounded source review. It does not establish editor
project ownership beyond the tracked coordinate file, persisted room
identity, runtime loader contents, live reachability, or safety of an archive
or path move.

## Title and tracked history

ZON header #1 occurs in 30 current files. The exact normalized title
“The Horrible Tracts of the Unnamed” occurs in five: `astral`, `eoo_op2`,
`mtncastl`, `theurgist`, and `ud-inside`. The other four matches are not
evidence that one file replaced another. Separately, the WLD #1 placeholder
matches `mtncastl` and `ud-inside`; the five title matches and three exact WLD
matches are different comparisons.

Followed same-stem history shows the WLD path at the 2011 area-tree commit;
MOB and OBJ paths reach initial 2005 revisions, a 2006 directory move, and
the 2011 area-tree commit; the ZON path reaches the 2009 directory move, the
2011 area-tree commit, and a 2015 quest-item-flag edit. These histories show
no explicit whole-zone replacement in the tracked paths. They cannot rule out
external editor aliases or untracked history.

## Safe disposition

Keep candidate row 11 **RETAIN IN PLACE**. Preserve the empty room, zone
header, and map-coordinate JSON as distinct source records until the map
owner establishes the component's intended purpose and who owns room #1.
Do not treat the active Astral Main/Areas/Tiamat manifests, generic Limbo
references, a duplicated title, or an identical placeholder block as evidence
to delete or move this source. Resolve the editor project, persisted-state,
and live-loader questions before any separate owner decision. See the
[story dossier](../zone-stories/ASTRAL.md),
[builder questions](../zone-stories/ASTRAL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-449-astral-placeholder-has-no-candidate-specific-route),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-449-astral-placeholder-and-room-1-identities),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-449-astral-retained-in-place),
[candidate row 11](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L11), and
[coverage row 362](ZONE_COVERAGE_REGISTER.csv#L362).

## Checkpoint 512 supplement: separate mtncastl placeholder

Unregistered mtncastl also defines room #1 with the same byte-identical
placeholder block as Astral and ud-inside, and its ZON file shares the exact
title “The Horrible Tracts of the Unnamed” with four other current files. The
active room owner is Limbo. These matches do not establish shared area or
editor identity; see the [mtncastl audit](MTNCASTL_AUDIT_REVIEW.md) and
[candidate dependency line 75](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L75).