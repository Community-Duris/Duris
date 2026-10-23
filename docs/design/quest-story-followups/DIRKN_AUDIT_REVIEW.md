# Dirk’nspire: boundary and typed-reference supplement

Status: repository-wide audit supplement to the existing
[Dirk’nspire dossier](../zone-stories/DIRKN.md). The dossier already maps the
native story, source files, selected code-defined arrivals, and foreign
Alatorin contracts. This supplement checks source-only WLD inbounds, current
active typed references, and the title/history boundary. It does not qualify
runtime access or gameplay.

## Active boundary and source-only scan

The active WLD census records 177 authored and resolving exits, one local weak
component, and one reciprocal boundary pair with Valinhav. The exact active
records are:

| Source record | Direction and target | Evidence |
| --- | --- | --- |
| Valinhav room 90810 | North (`D0`) to Dirk’nspire room 96800 | [Valinhav exit target](../../../areas/wld/valinhav.wld#L207) |
| Dirk’nspire room 96800 | South (`D2`) to Valinhav room 90810 | [Dirk’nspire exit target](../../../areas/wld/dirkn.wld#L18) |

A scan of all 95 current unregistered WLD files parsed 2,080,773 authored
D0–D9 exit records and found no target in Dirk’nspire’s active room range,
96800–96885. This adds no record to the curated source-only inbound list; its
count remains 362. The active reciprocal pair is already in the cross-area
exit register and does not by itself qualify ordinary access or a safe route.

## Active typed references

The exact non-Dirk’nspire source scan found these active external prototype
references:

| Consumer or producer | Reference | Evidence |
| --- | --- | --- |
| Alatorin Ragmor contract | Accepts Foe Hammer object 96835 as one of five items | [QST item reference](../../../areas/qst/alatorin.qst#L5485) |
| Alatorin Wikzor contract | Independently accepts Foe Hammer object 96835 as one of five items | [QST item reference](../../../areas/qst/alatorin.qst#L7144) |
| Alatorin rubble reset | Places loose-change object 96820 as a child of rubble 83330 in room 83544 | [reset placement](../../../areas/zon/alatorin.zon#L1655) |

The Alatorin story sidecar repeats the two contract definitions as guidance;
it is not a third native consumer. Aside from these listed contexts, no other
current external MOB, OBJ, ZON, QST, SHP, or story sources matched the local
numeric prototype ranges. The existing dossier separately maps three code-defined
arrival functions targeting room 96803, the explicit veil binding in Ixarkon,
and the full active type-25 object-portal scan, which found no local
Dirk’nspire destination. Those selected code paths and source declarations do
not establish a successful runtime arrival or item-source guarantee.

## Title and replacement-history boundary

The normalized active title “Dirk’nspire Stronghold” has one owner: area
`dirkn`, zone 968. The tracked WLD/ZON paths reach a 2009 area-directory move.
Later retained changes are limited to WLD spelling corrections, a 2013 file-
mode change, and a 2015 zone editor-header update. None names a distinct
predecessor or says another area replaces Dirk’nspire. Broader semantic
comparison remains pending.

[Active title](../../../areas/zon/dirkn.zon#L2) ·
[active-area entry](../../../areas/AREA#L346) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md) ·
[2011 WLD spelling correction](https://github.com/Community-Duris/Duris/commit/677a9f877901d558d6d4c78c0b5cbd796843ef03) ·
[2013 file-mode change](https://github.com/Community-Duris/Duris/commit/71d731c5b7a89e5352a86098bceae73817e42a80) ·
[2014 WLD spelling correction](https://github.com/Community-Duris/Duris/commit/4c1f865cb3b477c4e5a87d26a30052d09c4c38fe) ·
[2015 zone editor-header update](https://github.com/Community-Duris/Duris/commit/f18992b8f23f4e9b6fb179991ae991a97166aca0)

## Audit boundary

These are static source findings. The existing dossier’s travel, reset
admission, shop and item issuance, combat effects, and foreign contract
settlement still require their stated live qualification. Editor/tooling,
persisted-player references, and indirect or dynamic consumers remain outside
this bounded numeric-reference closure.

## Checkpoint 153 current repository-wide reconciliation

The active Dirk’nspire map has 86 rooms and 177/177 resolving exits (176 local and one outbound) in one weak component. Authored directed traversal from root #96800 reaches and returns from all 86 rooms. The sole active boundary remains reciprocal: Dirk’nspire #96800 south to Valinhav #90810 and Valinhav #90810 north to #96800 (pair rows [116](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L116) and [765](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L765)). One source room has a fall-chance field; the static declaration does not establish a live fall or its outcome.

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues. It found no source-room VNUM overlap and no exit target into #96800–#96885. No source-only approach is added. The reviewed source-only total remains 3,328 overall, with 15 `ud_map2` records and 159 Surf records.

Typed context remains narrow and explicit. Active Alatorin QST contracts at [5485](../../../areas/qst/alatorin.qst#L5485) and [7144](../../../areas/qst/alatorin.qst#L7144) both accept Dirk’nspire Foe Hammer #96835. Active Alatorin reset [line 1655](../../../areas/zon/alatorin.zon#L1655) places Dirk’nspire object #96820 as rubble’s child at room #83544. The Alatorin story sidecar has two typed `item_vnums` fields for #96835 at [line 2808](../../../areas/story/alatorin.story.json#L2808) and [line 3852](../../../areas/story/alatorin.story.json#L3852), mirroring those two contracts rather than adding another native consumer. Its remaining #96835 string hits are completion keys and step identifiers. The full scan found no other exact external QST, SHP, STORY, or ZON prototype references, local outbound prototype reference, duplicate external mobile/object definition, or external type-25 target.

The direct C/C++ numeric scan found room #96803 in three existing code locations: [prime shift destinations](../../../src/classes/innates.c#L2009), [psionic destinations](../../../src/classes/psionics.c#L615), and [Ixarkon special](../../../src/specs/specs.ixarkon.c#L21). These are room-context references already bounded by the dossier; they are distinct from object/mobile namespaces. All 231 external story sidecars parsed without errors. The only external raw numeric-token hits were the ten #96835 tokens in Alatorin’s sidecar, including the two typed item fields above.

The normalized active title “Dirk’nspire Stronghold” is unique among 442 scanned `.zon` files; the corrected lexical scan found no other owner. The bounded WLD/ZON history and existing same-stem supplement show no predecessor or replacement. Keep active `dirkn` **DO NOT MOVE**. The queue is now 194/220 complete, with 26 pending; Priority 196, Arcium (`arcium`), is next.

See [coverage row 75](ZONE_COVERAGE_REGISTER.csv#L75), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and the existing [dossier](../zone-stories/DIRKN.md).

Validation: Checkpoint 153 documentation validation checked 128 changed/added Markdown files; all 4,957 local link targets and 2,306 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.
