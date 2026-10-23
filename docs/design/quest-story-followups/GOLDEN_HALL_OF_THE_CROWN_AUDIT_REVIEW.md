# Golden Hall of the Crown title and history audit supplement

This supplement records all-442 normalized title comparison and followed source history alongside the [source-comprehensive Golden Hall dossier](../zone-stories/GOLDEN_HALL_OF_THE_CROWN.md). It changes no native area files.

## Title, history, and disposition

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Golden Hall of the Crown” owner: `gold_hal`. The title words “gold” and “crown” do not occur in any other current title line. Both current indexes list `gold_hal *404-406` ([full-area entry](../../../areas/AREA#L193), [QST entry](../../../areas/qst/AREA#L178)); the title is in [gold_hal.zon](../../../areas/zon/gold_hal.zon#L2). This is distinct from the separately titled [Halfling Silver Mine](../../../areas/zon/mining.zon#L2).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the `gold_hal` stem from the 2009 area-tree move through later edits. The story sidecar was added on 2026-10-03. No alternate-stem predecessor, retirement, or whole-zone replacement was found in the reviewed repository history. Keep active `gold_hal` **DO NOT MOVE**. The dossier's story and route owner questions remain separate. See [coverage row 111](ZONE_COVERAGE_REGISTER.csv#L112) and the [duplicate/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-731-golden-hall-of-the-crown-title-and-history).

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has six object reset rows for Golden Hall of the Crown object #40444. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).

## Checkpoint 814: current typed dependency closure

Reviewed October 12, 2026. This is a static source inventory for active
`gold_hal`; it does not establish boot loading, reset admission, stock,
player custody, safe passage, or successful quest completion.

The local ZON contains 534 reset commands and 808 typed prototype/room
operands: 222 mobile operands from M/F, 171 object operands from E/G/O/P
(including both P object roles), and 415 room operands from M/F/O/D. All 91
local mobiles and 95 of 106 local objects are reset-used. The eleven local
objects without a ZON placement are #40424, #40465, #40483, #40492, #40496,
#40497, #40498, #40499, #40500, #40501, and #40502; the comprehensive
dossier maps them to accepted outcomes and the support chain. Four reset
object operands import active Heavens objects: #72 at
[gold_hal.zon:639](../../../areas/zon/gold_hal.zon#L639), and #363/#368/#369
at [lines 749-751](../../../areas/zon/gold_hal.zon#L749). The active owner for
all four is Heavens ([active list](../../../areas/AREA#L2), [#72](../../../areas/obj/heavens.obj#L1000), [#363](../../../areas/obj/heavens.obj#L3783), [#368](../../../areas/obj/heavens.obj#L3836), [#369](../../../areas/obj/heavens.obj#L3850)). #72 also has definitions in unregistered `hostel` and
`mavulsk` ([hostel](../../../areas/obj/hostel.obj#L959), [mavulsk](../../../areas/obj/mavulsk.obj#L1173)); keep those source identities separate.

The 19 local QST giver headers resolve to local mobiles. Its 36 G/R I item
commands (23 G and 13 R) all resolve to local objects. The structured sidecar
has 93 typed references (69 mobile and 24 object); all resolve locally. The
two local shop records are #40428 and #40462 ([first](../../../areas/shp/gold_hal.shp#L1), [second](../../../areas/shp/gold_hal.shp#L42)). Their keepers and rooms are
local; their product lists contain local objects plus Heavens #363/#368/#369
in shop #40428 ([product list](../../../areas/shp/gold_hal.shp#L6)). No same-type foreign MOB or OBJ definition duplicates a
Gold Hall prototype.

Active foreign ZON consumers account for 76 typed reset operands into Gold
Hall: 72 object and four mobile operands. No source-only ZON consumer was
found, and no foreign reset room operand targets a Gold Hall room.

| Active source | Typed references into Gold Hall |
| --- | --- |
| Alatorin | 52 operands: 48 objects (#40404, #40405, #40441, #40442, #40444, #40469, #40474) and four mobiles (#40429, #40467, #40468); see [resets](../../../areas/zon/alatorin.zon#L1464), [more resets](../../../areas/zon/alatorin.zon#L1644), and [mobile placements](../../../areas/zon/alatorin.zon#L2800). |
| Caertannad | Six object resets for #40444; see the [local outbound table](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references) and [source](../../../areas/zon/caertannad.zon#L1164). |
| Malch and Ravenloft2 | One object reset for #40474 from each: [Malch](../../../areas/zon/malch.zon#L350), [Ravenloft2](../../../areas/zon/ravenloft2.zon#L1051). |
| Shady | Three object resets for #40441; see [Shady](../../../areas/zon/shady.zon#L1101). |
| Ship Yards | Four object resets (#40441 twice and #40442 twice); see [first group](../../../areas/zon/shipy.zon#L485) and [second group](../../../areas/zon/shipy.zon#L595). |
| Surface Keeps | Four object resets for #40442; see [Surface Keeps](../../../areas/zon/surfacekeeps.zon#L252). |
| Vehicles | Five object resets for #40444; see [Vehicles](../../../areas/zon/vehicles.zon#L518). |

Other active typed consumers target local purple potion #40469: one Ship Yards
QST `G I` command at [shipy.qst:890](../../../areas/qst/shipy.qst#L890), the
optional item step in [shipy.story.json:1116](../../../areas/story/shipy.story.json#L1116),
and one Alatorin shop product in [alatorin.shp:443](../../../areas/shp/alatorin.shp#L443).
No other active or source-only QST item, shop keeper/room/product, or sidecar
reference into a Gold Hall mobile, room, or object was found.

The local WLD has 42 nonzero key fields; every key is a local object (#40401,
#40413, #40424, #40429, #40505, #40457, #40466, #40479, #40484, #40486, or
#40489; examples across the door groups include [the fortress gate](../../../areas/wld/gold_hal.wld#L31), [the appointment doors](../../../areas/wld/gold_hal.wld#L2063), [the study](../../../areas/wld/gold_hal.wld#L2172), [the goblin cells](../../../areas/wld/gold_hal.wld#L5234), and [the animal cages](../../../areas/wld/gold_hal.wld#L5818)). No external WLD key field points to a Gold Hall object. Local ITEM_TELEPORT objects #40503/#40504 target local rooms #40699/#40698 ([#40503](../../../areas/obj/gold_hal.obj#L1736), [#40504](../../../areas/obj/gold_hal.obj#L1754)). The four local ITEM_SWITCH objects target local rooms #40655, #40455, #40519, and #40563 ([#40420](../../../areas/obj/gold_hal.obj#L370), [#40431](../../../areas/obj/gold_hal.obj#L543), [#40432](../../../areas/obj/gold_hal.obj#L559), [#40436](../../../areas/obj/gold_hal.obj#L626)). No external type-25 destination or type-29 room field targets a Gold Hall room.

The maintained C/H numeric scan found these direct data/code bindings:

| Role | Source |
| --- | --- |
| Chaos equipment object #40425 | [chaos equipment table](../../../src/account/chaos_eq_data.h#L92) and [second table](../../../src/account/chaos_eq_data.h#L344). |
| World-quest mobiles #40466/#40410 | [assignment table](../../../src/specs/specs.assign.c#L767) and [second assignment](../../../src/specs/specs.assign.c#L768). |
| Reliance object #40409 and inn room #40454 | [assignment table](../../../src/specs/specs.assign.c#L1966) and [inn binding](../../../src/specs/specs.assign.c#L2295). |
| Reliance mount mobile #40429 | [mount check](../../../src/specs/specs.jot.c#L650) and [creation](../../../src/specs/specs.jot.c#L685). |

The active graph remains 674/674 resolved exits with three inbound, two
outbound, and two reciprocal cross-area exit records. This checkpoint changes
no WLD totals or candidate relationships. Runtime/editor, persisted-player,
loading, and indirect/dynamic availability remain pending.
