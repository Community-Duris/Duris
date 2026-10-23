# Clan Stoutdorf Settlement repository-wide audit supplement

This checkpoint reconciles the [source-comprehensive Clan Stoutdorf dossier](../zone-stories/CLAN_STOUTDORF.md) with active-area graph, inactive-WLD, typed dependency, title, and bounded history passes at source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. It records source dependencies and builder decisions. No native gameplay or area placement changes are part of this audit.

## Active connectivity and map admission

The active `drst` WLD contains 199 rooms and 432 authored exits, with no parse errors. All 432 resolve: 430 remain local and two cross area boundaries. Every room is in one weak component. An authored directed traversal from entrance #34200 reaches all 199 rooms and has authored returns from all 199 when exits are treated as edges; this ignores live door state, source/reset timing, visibility, access requirements, and actor state. The story dossier separately maps those gates and hazards, so these source counts do not prove current playability or safety.

Both active boundary connections are reciprocal:

- Underdark #840288 west to Stoutdorf entrance #34200, with the local east return.
- Cloister #67138 north to Stoutdorf #34351, with the local south return.

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues and found no source-room VNUM overlap. It found one source-only approach: `ud_map2.wld` #1121983 west to Stoutdorf #34200. That source is not loaded through `areas/AREA`, so this is not active ingress. The curated source-only approach total rises from 1,532 to 1,533; the reviewed Surf subset remains 142. Preserve the source record with its map.

## Typed dependencies and scoped negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Active WLD boundaries | The two reciprocal pairs are present in both directions: local #34200 east ↔ Underdark #840288 west, and local #34351 south ↔ Cloister #67138 north ([Stoutdorf WLD](../../../areas/wld/drst.wld#L16), [Underdark](../../../areas/wld/underdark.wld#L252193), [Cloister](../../../areas/wld/cloister.wld#L729)). |
| Inactive map approach | `ud_map2.wld` #1121983 west targets #34200. It is a source-only edge, not an active route ([source WLD](../../../areas/wld/ud_map2.wld#L17769), [candidate record](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114)). |
| Outside exact item use | Alatorin has two active QST `G I 34226` records. Its sidecar identifies #34226 as the King StoutDorf DwarfSlayer offered in each of two alternative five-item returns; the recipients and transaction history remain as mapped in the dossier ([QST records](../../../areas/qst/alatorin.qst#L5486), [first accepted exchange](../../../areas/story/alatorin.story.json#L2809), [second accepted exchange](../../../areas/story/alatorin.story.json#L3853)). |
| Outside reset use of a local object | Alatorin reset line 1679 places local gauntlets #34217 inside display case #83367 at room #83569. The display's locked state and the separate origin of gauntlets are covered by the story dossier ([Alatorin ZON](../../../areas/zon/alatorin.zon#L1679)). |
| Imported objects in local reset stock | `drst.zon` also resets imported objects #358 and #55451 on King StoutDorf; these are outside the local `.obj` prototype range and are not additional local prototypes ([local ZON](../../../areas/zon/drst.zon#L719)). Their epic and memory outcomes stay separate from the tinkerer receipt. |
| Effective compiled binding | Local axe #34226 has the enabled `dwarfslayer` object binding. The dossier records its actual wielder, melee-hit, target, and proc conditions ([assignment](../../../src/specs/specs.assign.c#L1293)). The old board assignments remain commented out in source. |
| Unregistered shop matches | `player_guilds.shp` has raw numeric matches for local object IDs, but it is not an active `areas/AREA` shop contribution and `drst` has no local `.shp`. These hits do not establish active Stoutdorf shop stock. |
| Bounded negatives | The scan found no external duplicate mobile/object definitions, no type-25 portal declaration targeting a local Stoutdorf room, and no local shop. Outside typed item/story references are the Alatorin DwarfSlayer records above; no additional outside exact QST item command, SHP product, or typed sidecar item VNUM was found in the scanned scope. |

The local `drst.story.json` and native QST continue to define the sixteen-item recipe and its current capability boundary. The 18 existing [builder follow-ups](../zone-stories/CLAN_STOUTDORF.md#builder-follow-ups-and-capability-expansion) remain authoritative for material issuance and recovery, gate intent, equipment and potion effects, and durable accounting. The repository-wide dependency pass does not alter those findings or claim a played exchange.

## Title, history, and disposition

After stripping color tokens and normalizing whitespace/case, “Clan Stoutdorf Settlement” has one exact owner among 351 current active zone titles: `drst` ([title source](../../../areas/zon/drst.zon#L2)). Followed `.wld`, `.zon`, `.mob`, `.obj`, and `.qst` histories stay under the same `drst` stem after the 2009 `areas/` relocation. The tracked WLD and QST paths show no alternate stem; the ZON/MOB/OBJ histories include same-stem quest-flag and 2026 ambassador updates. `drst.story.json` was added on 2026-10-06. No alternate-stem rename, explicit predecessor, or replacement claim appears in these bounded paths. Keep active `drst` **DO NOT MOVE**. Keep `ud_map2` **RETAIN IN PLACE** while map ownership and broader typed/tooling closure remain open.

## Builder questions and recommendations

1. Confirm both current reciprocal WLD boundary pairs are intended. Preserve each return; do not infer live open state or safe travel from the authored edges.
2. Preserve `ud_map2.wld` #1121983 west into Stoutdorf as a source-only approach. Keep `ud_map2` in place until its map ownership and dependency closure are resolved.
3. Confirm Alatorin's two exact DwarfSlayer returns and its gauntlet reset as intentional outside dependencies. Keep item identity, recipient departure, source custody, and the Stoutdorf collection receipt distinct.
4. Treat the `player_guilds.shp` token matches as unregistered source hits until the shop record's owner and load path are established; do not count them as active Stoutdorf services.
5. Retain the dossier's 18 source-specific follow-ups for builder-intent decisions. This repository-wide audit supplies no native fix or area move recommendation beyond retaining current `drst` and preserving the recorded source-only approach.

## Checkpoint 790: current static typed closure

A fresh source reconciliation finds 303 active `drst.zon` prototype-reset fields: 259 M,
15 O, 23 E, and six G rows. Three hundred one target local mobiles or objects; the two
imports are G rows 719 and 720 for objects #358 and #55451
([reset](../../../areas/zon/drst.zon#L719)). The local QST has 17 R/G item fields,
including reward #34217; all resolve to local objects
([QST](../../../areas/qst/drst.qst#L42)).
Its current story sidecar has 19 typed VNUM values: nine mobile, two giver, and eight
item values, all local ([sidecar](../../../areas/story/drst.story.json#L24)).

Outside consumers remain specific. Alatorin QST rows
[5486](../../../areas/qst/alatorin.qst#L5486) and
[7145](../../../areas/qst/alatorin.qst#L7145) each use Stoutdorf axe #34226. The sidecar
has matching typed `item_vnums` at
[line 2809](../../../areas/story/alatorin.story.json#L2809)
and [line 3853](../../../areas/story/alatorin.story.json#L3853). Alatorin P row 1679
resets gauntlets #34217 into display case #83367
([reset](../../../areas/zon/alatorin.zon#L1679)).
All current ZON and QST files contain no other reset or QST-item reference to local
Stoutdorf prototypes; no outside QST-giver header targets a local mobile. The current
sidecar scan found no other typed VNUM consumer.

`player_guilds.shp` contains raw #34202 matches at
[product line 45](../../../areas/shp/player_guilds.shp#L45) and
[line 47](../../../areas/shp/player_guilds.shp#L47), but is not registered as an active
shop source. No active external shop product or local Stoutdorf shop is present. The
enabled compiled object binding for #34226 is `dwarfslayer`; the old board assignment for
#34200 remains inside a block comment ([enabled](../../../src/specs/specs.assign.c#L1293),
[commented](../../../src/specs/specs.assign.c#L1866)). No external duplicate mobile/object
prototype was found.

No local item is type 25 or type 29, and no type-25 teleport or type-29 switch room
target in the current object sources points into Stoutdorf's 199 WLD rooms. These results
refresh the bounded negatives in the earlier audit without changing the source-only
`ud_map2.wld` approach or WLD totals. This closes the static typed-source inventory only;
it does not establish live reset stock, runtime command/tool behavior, successful exchange
outcomes, or persisted player inventory/history. No gameplay or database inspection was
performed.

