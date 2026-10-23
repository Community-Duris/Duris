# Treasure Caves: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Treasure Caves
source dossier](../zone-stories/TREASURE_CAVES.md). It reconciles active and
source-only WLD edges, raw directed reach, selected QST and story references,
active reset consumers, compiled references, and title/history. Static source
links do not qualify installed availability, actual traversal, item issuance,
or player outcomes.

## Active WLD boundaries and directed reach

The active area is dungeon, zone 930, with 116 rooms from #93000 through
#93115. Its 236 authored exits all resolve: 234 stay local and two leave the
area. The local map is one weak component of 116 rooms. Authored directed
traversal from #93000 reaches all 116 rooms and all 116 have a directed path
back; no internal exit has flag 8.

Two reciprocal boundaries connect Treasure Caves to adjacent active areas:

| Boundary | Source evidence |
| --- | --- |
| Dungeon room #93115 west ↔ Arachdrathos Wilderness room #36004 east. | [Dungeon exit](../../../areas/wld/dungeon.wld#L2452), [Wilderness exit](../../../areas/wld/arac_wild.wld#L97), [pair summary rows 20 and 126](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L20) |
| Dungeon room #93069 west ↔ Underdark room #810962 east. | [Dungeon exit](../../../areas/wld/dungeon.wld#L1506), [Underdark exit](../../../areas/wld/underdark.wld#L171341), [pair summary rows 127 and 699](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L127) |

These are authored WLD connections. Directed graph results establish a path
through every authored local room; they do not prove that a door, key,
controller, or route is available to a player.

## Inactive WLD sweep

All 95 current WLD sources outside the active areas/AREA list were parsed:
527,791 rooms and 2,080,773 authored exits, with zero parse errors. The sweep
found no source-room VNUM overlap with Treasure Caves and no inactive exit
targeting a Dungeon room. There is no source-only WLD approach to add, so
curated source-only totals remain 3,333 overall, 164 for the reviewed Surf
subset, and 15 for ud_map2.

## Exact typed and structured references

Two QST sources contain exact item-command references to Dungeon objects:

| Source | Reference and source status | Prototype |
| --- | --- | --- |
| Active White Harbor QST | G I #93011; the corresponding structured request is Lancer's exchange for the gem-studded dragon hide. | [QST row](../../../areas/qst/wh.qst#L2626), [object #93011](../../../areas/obj/dungeon.obj#L174), [story contract](../../../areas/story/wh.story.json#L1021) |
| Unregistered Mountain.qst | G I #93001; this QST file is not in areas/AREA or areas/qst/AREA and is not an active production contract. | [QST row](../../../areas/qst/Mountain.qst#L56), [object #93001](../../../areas/obj/dungeon.obj#L19), [candidate row 74](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L74) |

Surface Mini's two Lancer story completion keys also reference object #93011
([first branch](../../../areas/story/surfacemini.story.json#L689),
[second branch](../../../areas/story/surfacemini.story.json#L1537)). These
structured fields mirror the authored exact item reference; they do not show
that the item is currently in stock or has been delivered.

Active Tharnadian Ruin resets load Dungeon mobile #93015 in four rooms:
#5641, #5643, #5652, and #5653
([reset records](../../../areas/zon/tharnadian_ruin.zon#L263),
[local mobile prototype](../../../areas/mob/dungeon.mob#L215)). This is an
active cross-area mobile dependency.

Compiled source contains two type-distinct references. The Githyanki room
array includes room #93001 in [innates.c](../../../src/classes/innates.c#L2008);
this is a room reference despite the same number also appearing in Dungeon's
mobile and object namespaces. The spec assignment binds object #93007,
the short blue sword, to blue_sword_armor
([assignment](../../../src/specs/specs.assign.c#L2219),
[object prototype](../../../areas/obj/dungeon.obj#L116)).

Dungeon's own reset rows, QST, and story material do not reference external
object prototypes in the selected exact scans, and Dungeon has no local SHP
file. The external reference scan found no duplicate mobile or object
prototype definitions and no external type-25 target into Dungeon. The scan
of 231 other story sidecars parsed without errors and found only the #93011
references described above. No external SHP product reference was found.

## Title and bounded history

The normalized .zon title Treasure Caves occurs once among the 442 scanned
.zon files, owned by active source stem dungeon
([title source](../../../areas/zon/dungeon.zon#L2)). Bounded same-stem
history for dungeon.wld and dungeon.zon continues under the dungeon stem
through the 2009 areas/ tree move and later edits; the WLD path includes 2011
and 2013 edits, and the ZON path includes the 2015
quest-item flag pass. The reviewed paths show no alternate-stem rename,
distinct predecessor, or whole-zone replacement. See
[duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active dungeon DO NOT MOVE and preserve both reciprocal WLD boundaries,
the White Harbor exchange, Surface Mini's structured item references, the
Tharnadian mobile resets, and compiled room/object references. Keep the
unregistered Mountain QST source RETAIN IN PLACE with its exact Dungeon
object dependency recorded in the existing candidate row; do not treat it as
active QST input. No inactive-WLD candidate row is added and the candidate
register remains 121 rows. Editor, administrative, installed-data,
persisted-player, indirect-runtime, and actual delivery closure remain
outside this bounded scan. No source, gameplay, migration, or production
change is proposed here.


## Checkpoint 511 supplement: Mountain QST reward identity

The unregistered uppercase [`Mountain.qst`](../../../areas/qst/Mountain.qst#L30)
has one valid current object output reference into Dungeon: `G I #93001`.
Active [Dungeon object #93001](../../../areas/obj/dungeon.obj#L19) is black
leather armor; the QST's Gulik completion text describes an armoury key.
Mountain.qst references seven unique #210xx item identities with no current
object prototypes, including required object #21027. It is excluded from normal QST
aggregation, and runtime quest boot has no source-file glob fallback. Keep the
candidate on its existing dependency row and do not alter Dungeon's item from
this incomplete source; see the [Mountain QST audit](MOUNTAIN_QST_AUDIT_REVIEW.md).

## Checkpoint 791: current typed closure refresh

The active `dungeon.zon` contains 184 prototype-reset commands: 139 M, 34 E, four O,
four P, and three G rows. Each P row names both an object and its container, so the reset
commands contain 188 typed prototype VNUM fields. All targets resolve to one of Dungeon's
36 mobiles or 24 objects; no foreign prototype is imported
([reset source](../../../areas/zon/dungeon.zon#L41)). The local QST has one item field,
G I #93006, and it resolves locally ([QST](../../../areas/qst/dungeon.qst#L18)). The
current story sidecar has 39 typed VNUM values: 36 mobile, two giver, and one item value,
all local ([sidecar](../../../areas/story/dungeon.story.json#L1)).

Active White Harbor QST row 2626 consumes Dungeon object #93011; its sidecar has the
matching typed `item_vnums` value ([QST](../../../areas/qst/wh.qst#L2626),
[object](../../../areas/obj/dungeon.obj#L174),
[sidecar](../../../areas/story/wh.story.json#L1021)). Active Tharnadian Ruin resets
load Dungeon mobile #93015 four times, in rooms #5641, #5643, #5652, and #5653
([resets](../../../areas/zon/tharnadian_ruin.zon#L263),
[mobile](../../../areas/mob/dungeon.mob#L215)). Unregistered `Mountain.qst` row 56 has
G I #93001 for local black leather armor; keep this on existing candidate row 74 and
outside active QST inputs ([QST](../../../areas/qst/Mountain.qst#L56),
[candidate](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L74)). Surface Mini's two #93011
occurrences at lines 689 and 1537 are `completion_key` strings, not typed `item_vnums`
references ([first](../../../areas/story/surfacemini.story.json#L689),
[second](../../../areas/story/surfacemini.story.json#L1537)).

Dungeon has one local type-29 `ITEM_SWITCH`: object #93023 addresses room #93002 and is
reset in that same room ([object field](../../../areas/obj/dungeon.obj#L347),
[reset](../../../areas/zon/dungeon.zon#L41)). The switch's room field is `value[1]`
([handler contract](../../../src/specs/specs.object.c#L302)).
No local type-25 object exists.
The scan of current active and other object files found no external type-25 destination
or type-29 switch room in the 116 Dungeon rooms. These declarations add no authored WLD
edge and do not change the two boundary-pair counts.

The exact compiled references remain type-distinct: room #93001 appears in the Githyanki
room array, while object #93007 has the `blue_sword_armor` assignment
([room array](../../../src/classes/innates.c#L2008),
[object binding](../../../src/specs/specs.assign.c#L2219)). No other external mobile/object
prototype, SHP product, local shop, or external QST-giver header targets a Dungeon
prototype. All current story sidecars parsed; the only outside typed VNUM reference is
White Harbor's #93011 item above.

This refresh closes the static typed-source inventory. It does not establish whether the
unregistered Mountain QST is deployed elsewhere, whether any reset or exchange is
available, or what players have persisted. Keep Mountain's existing source record and
candidate disposition unchanged; no WLD, area-source, gameplay, or owner-question changes
were made.

## Checkpoint 922 supplement: Mountain reward number has multiple roles

Mountain.qst's G I #93001 is typed as an object goal and resolves to active
Dungeon OBJ #93001, black leather armor. The same number is also active
Dungeon MOB #93001, a header in dungeon.qst interpreted as a mobile quester,
and WLD room #93001; the compiled Githyanki array also uses #93001 as a room.
These separate role bindings do not change the Mountain output object's
identity or resolve its armoury-key wording. Preserve the existing object and
candidate dependency reference pending owner confirmation. See the
[Mountain QST role audit](MOUNTAIN_QST_AUDIT_REVIEW.md#checkpoint-922-typed-collision-with-active-mountaintracks-rooms).
