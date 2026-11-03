# The Town of Moregeeth: repository-wide boundary and dependency supplement

Status: bounded static supplement to the comprehensive
[Moregeeth source map](../zone-stories/THE_TOWN_OF_MOREGEETH.md). It reconciles
active and source-only WLD edges, direct external references, the active title,
and the tracked WLD/ZON history. It does not qualify runtime availability or
player outcomes.

## Active and source-only WLD boundaries

The active area is `goblinht`, zone 700, with physical rooms 70000–70352. The
comprehensive dossier records five active cross-zone exit declarations:

| Active boundary | Evidence |
| --- | --- |
| Trukt 1655 south ↔ Moregeeth outpost 70198 north | [Trukt exit](../../../areas/wld/truk.wld#L1048), [Moregeeth exit](../../../areas/wld/goblinht.wld#L5356) |
| Surface 626940 west ↔ Moregeeth cave entrance 70327 east | [Surface exit](../../../areas/wld/surface.wld#L2665208), [Moregeeth exit](../../../areas/wld/goblinht.wld#L8478) |
| Heavens room 35 south → Moregeeth inn 70175 | [Heavens exit](../../../areas/wld/heavens.wld#L426) |

A stateful parse of the 95 current unregistered WLD sources read 2,080,773
authored D0–D9 exits with zero parse issues. One source-only `surf.wld` exit
targets an active Moregeeth room:

| Unregistered source room | Direction and active target | Evidence |
| --- | --- | --- |
| 274726 | West (`D3`) to Moregeeth room 70198 | [exit declaration](../../../areas/wld/surf.wld#L1568090) |

This record is in an unregistered source file, not the active WLD input. It
does not add an active-world entrance or change the 1,350-exit active census.
It brings the curated source-only record count from 383 to 384.

## Direct external typed references

The comprehensive dossier's bounded scan covers all 713 active portal
prototypes, foreign resets, and native recipe inputs/rewards involving local
prototype kinds. It found no foreign portal to a Moregeeth room, no foreign
reset loading a local mobile or object, and no foreign recipe consuming or
producing a local item. A scan of the active-area SHP sources found no foreign
shop consumer of a local prototype. The unregistered `myranth1.shp`, however,
contains five source-only shop records whose typed keeper and room fields point
into Moregeeth:

| Source-only shop | Local keeper prototype | Local room | Evidence |
| --- | --- | --- | --- |
| `myranth1.shp` shop 70101 | Mobile 70101 | Room 70101 | [keeper](../../../areas/shp/myranth1.shp#L25), [room](../../../areas/shp/myranth1.shp#L27), [mobile](../../../areas/mob/goblinht.mob#L1526) |
| `myranth1.shp` shop 70102 | Mobile 70102 | Room 70107 | [keeper](../../../areas/shp/myranth1.shp#L63), [room](../../../areas/shp/myranth1.shp#L65), [mobile](../../../areas/mob/goblinht.mob#L1542) |
| `myranth1.shp` shop 70103 | Mobile 70103 | Room 70108 | [keeper](../../../areas/shp/myranth1.shp#L108), [room](../../../areas/shp/myranth1.shp#L110), [mobile](../../../areas/mob/goblinht.mob#L1556) |
| `myranth1.shp` shop 70104 | Mobile 70104 | Room 70111 | [keeper](../../../areas/shp/myranth1.shp#L147), [room](../../../areas/shp/myranth1.shp#L149), [mobile](../../../areas/mob/goblinht.mob#L1571) |
| `myranth1.shp` shop 70105 | Mobile 70105 | Room 70110 | [keeper](../../../areas/shp/myranth1.shp#L185), [room](../../../areas/shp/myranth1.shp#L187), [mobile](../../../areas/mob/goblinht.mob#L1586) |

The active [`areas/AREA`](../../../areas/AREA#L261) manifest lists `goblinht`
for Moregeeth and has no `myranth1` entry. Normal SHP aggregation reads `AREA`
and selects the corresponding per-area files
([aggregator](../../../areas/src/shp/make_shp.c#L6),
[manifest open](../../../areas/src/shp/make_shp.c#L26)). The source-level shop
loader also has a fallback: if `areas/world.shp` cannot be opened, it globs
every `areas/shp/*.shp`, which includes this unregistered file
([fallback](../../../src/economy/shop.c#L3272)). Therefore the source-only shop
records are excluded by normal manifest aggregation, but cannot be treated as
unconditionally inert. No runtime path was exercised.

The typed C/H scan found room/mobile bindings rather than direct references to
Moregeeth object prototypes: the goblin-town home and race-room table
([home](../../../src/core/constant.c#L1740),
[room table](../../../src/core/constant.c#L1982)); the justice mobile 70187 and
jail room 70044 ([justice table](../../../src/combat/justice.c#L100)); the
Moregeeth alias entry at room 70296 ([chaos map](../../../src/combat/chaos.c#L34));
and the local world-quest and inn assignments for mobile 70029 and room 70175
([special assignments](../../../src/specs/specs.assign.c#L764),
[inn assignment](../../../src/specs/specs.assign.c#L2558)). These are typed
room/mobile uses, not item consumers. Same-numbered values in a quest or shop
file were counted only when their field type matches a local prototype.

## Active title and tracked history boundary

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“The Town of Moregeeth” owner: `goblinht`. Both current indexes list
`goblinht *700-704` ([full-area entry](../../../areas/AREA#L261),
[QST entry](../../../areas/qst/AREA#L234)); the title is in
[goblinht.zon](../../../areas/zon/goblinht.zon#L2). The nearby “The Goblin
Camp” (`goblincamp`) and “Malch'Hor Ganl the Goblin City” (`malch`) titles
belong to separate current owners and ranges *114 and *236-237. Their shared
goblin theme does not establish an alternate Moregeeth identity or replacement
([Camp title](../../../areas/zon/goblincamp.zon#L2),
[City title](../../../areas/zon/malch.zon#L2)).

Followed WLD and ZON history remains on the `goblinht` stem from the 2009
area-tree move. The 2014 Quartermaster transfer to `siege` is a specific
mobile relocation; Moregeeth's title, key, and zone range persist. Later edits
remain on the same paths, including the 2026 letter-desk keyhole correction.
The story sidecar was added on 2026-10-04. No alternate-stem predecessor,
retirement, or whole-zone replacement was found. Keep active `goblinht`
**DO NOT MOVE**. The semantic title comparison resolves the prior pending
note; the town's open builder questions remain unchanged.

[Moregeeth title](../../../areas/zon/goblinht.zon#L2) ·
[full-area entry](../../../areas/AREA#L261) ·
[QST entry](../../../areas/qst/AREA#L234) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

## Audit boundary

These are static source and manifest findings. The unregistered `surf.wld`
edge is not an active entrance. Source-only shop records and code-defined
references do not prove live availability, safe travel, player custody, sales,
or quest settlement. Editor/tooling, persisted-player, and indirect or dynamic
consumers remain outside this bounded closure.

## Checkpoint 513: myranth1 QST item typing and Moregeeth shop records

The existing myranth1 SHP supplement identifies five source-only shops whose
keeper/mobile and in_room fields point into Moregeeth. A separate
myranth1.qst file uses item commands for #70107 and #70108, but those VNUMs
are active goblinht mobile prototypes—a warg and warg cub—and neither has a
current object prototype. The QST’s other item #70157 and all 19 distinct
SHP product VNUMs also lack current object definitions. Do not treat these
typed references as live object bindings. The quest/story detail and bounded
definition scans are in the [myranth1 audit](MYRANTH1_AUDIT_REVIEW.md).

## Checkpoint 813: current typed-role closure

The local `goblinht.zon` has 455 typed prototype operands: 322 mobile and 133
object operands. Those include 14 imported object operands (13 distinct
VNUMs), from `verz1` (#3097) and `heavens` (#188, #189, #363, #368, #369,
#421, #835, #500, #54, #73, #77, and #412); #500 is repeated, and #54/#73/#77
also have source-only definitions under other stems. Active AREA ownership
resolves these imports to their registered owners. The `desk-picks` optional
sidecar item #412 is the active Heavens lockpick object
([definition](../../../areas/obj/heavens.obj#L4327)); the same object is
imported by the town's reset file. All 109 local mobiles and 96 local objects
are accounted for by the typed source inventory; 108 mobiles and 88 objects
are directly reset-used.

The 11 native Q/QA contracts have 27 item fields, all resolving to local
objects. The active story sidecar has 56 typed references: 21 contact mobiles,
19 quest givers, and 16 items. Fifteen sidecar item values are local; the
remaining optional desk-picks value is the Heavens object above. The
source-only `myranth1` records retain their separate interpretation: its QST
item-typed #70107/#70108 are mobile VNUMs rather than object definitions, and
#70157 plus the 19 distinct source-only shop products have no current object
definition. Its five shop records refer to Moregeeth keepers #70101-#70105 and
rooms #70101, #70107, #70108, #70111, and #70110. Normal AREA aggregation
omits this component; the shop loader's conditional all-SHP fallback means
these records are not declared unconditionally inert. See the
[Myranth1 audit](MYRANTH1_AUDIT_REVIEW.md#typed-item-references-and-missing-prototypes)
and [CP513 details](#checkpoint-513-myranth1-qst-item-typing-and-moregeeth-shop-records).

No outside active or source-only ZON reset role, active QST/SHP consumer,
external prototype definition, typed sidecar role, or external WLD key field
was found for Moregeeth prototypes or rooms. Eight local type-25 routes target
local rooms; there are no local type-29 routes and no external type-25/type-29
target into the town. The 12 local WLD key fields use local objects #70057 or
#70019. Static C/H matches remain room/mobile bindings: chaos alias room
#70296, justice mobile #70187 and room #70044, the #70000 home and guild-room
table entries, world-quest mobile #70029, and inn room #70175. The #70316
`actoth.c` match is comment text, not a binding. The mapped code references
are detailed above in the typed scan.

The active WLD census stays at 1,350/1,350 resolved exits, three inbound and
two outbound cross-area records; the source-only `surf.wld` approach remains
outside the active graph. Runtime/editor, persisted-player, and indirect or
dynamic closure remain pending. No live behavior or safety conclusion follows
from these static records.
