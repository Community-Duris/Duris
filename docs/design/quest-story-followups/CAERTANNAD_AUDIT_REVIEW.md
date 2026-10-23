# Caer Tannad typed-dependency audit

Checkpoint 779, reviewed October 12, 2026. This static source review closes the typed-reference inventory for active Caer Tannad and its registered attic candidates. It records current source declarations and exact prototype ownership; it does not qualify loading, reset admission, portal use, player access, or durable availability. No native gameplay or area source was changed.

## Active source inventory and ownership

`caertannad` is an active full-area contributor with an indexed QST file and a story sidecar. The current source contains 451 rooms (#78400–#78850), 123 mobiles (#78400–#78522), 121 objects (#78400–#78520), 32 local QST giver sections, and 30 contracts. There is no local SHP file and no same-type room, mobile, or object VNUM overlap with another current source file.

All 105 local QST G/R I item commands target local Caer Tannad objects, and all 32 local giver headers resolve to local mobiles. The structured story sidecar has 76 `giver_vnum`, 42 `item_vnums`, and 24 `mob_vnum` references. All but two typed references resolve to local prototypes: `giver_vnum` #5506 and #5528 point to active Tharnadian Ruin mobiles. No inbound active or candidate sidecar reference to a Caer Tannad prototype was found.

## Outbound active reset references

Caer Tannad has 78 ZON reset rows that reference prototypes outside its local MOB/OBJ set, across 13 distinct active source owners. The owner/type breakdown is:

| Prototype owner | Reset rows | Referenced VNUMs |
| --- | ---: | --- |
| Connectorzones | 13 | Mobiles #53610/#53639 (12); object #53607 (1) |
| Undermountain mobile | 20 | #92051 |
| Underworld mobile | 10 | #4530 |
| Shady mobile | 4 | #97550 |
| Citadel object | 1 | #13036 |
| Golden Hall of the Crown object | 6 | #40444 |
| Heavens objects | 6 | #358, #359, #360 |
| Herders object | 4 | #94381 |
| Labyrinth of No Return object | 4 | #5019 |
| Deep Ravine of Passage (`minopass`) object | 5 | #94720 |
| Faerie Realm (`realm`) object | 1 | #14004 |
| Ship Yards object | 3 | #43125 |
| White Harbor object | 1 | #55377 |

The four mobile resets for #97550 resolve through the active `shady` source. Registered candidate `shady2` also defines mobile #97550; preserve that separate source identity and do not attribute the active resets to the candidate. Reset declarations do not establish that a prototype loads or appears in play.

## Active inbound consumers

Active reset references into Caer Tannad are:

| Active source | ZON rows | Local prototype VNUMs and roles |
| --- | ---: | --- |
| Alatorin | 6 | M #78438 (2); O #78414 and #78484 (4) |
| Elemental Groves | 2 | O #78455 |
| Surface Keeps | 12 | M #78438 (6); O #78414 (6) |
| Tharnadia Rifts | 54 | M #78412 and #78467 (39); O #78409, #78455, #78457, and #78503 (15) |

Alatorin is the only active inbound QST consumer found: it has 36 G/R I commands over seven local Caer Tannad objects: #78414 (25), #78415 (1), #78419 (2), and #78442–#78445 (two each). Tharnadian Ruin has no QST item import into Caer Tannad; the two cross-area references in the Caer Tannad story sidecar point outward to its mobiles #5506 and #5528. No external QST giver or QC room reference into Caer Tannad was found. No active shop product targets a Caer Tannad object, and no external ITEM_TELEPORT destination or ITEM_SWITCH value[1] room field targets a Caer Tannad room.

No registered candidate ZON, QST, SHP, or story-sidecar inbound reference to a Caer Tannad prototype was found.

## Portal and compiled references

Caer Tannad defines ten type-25 ITEM_TELEPORT objects: nine target local rooms and #78510 targets active Dracopit room #15907. Object #78519 is type 29, ITEM_SWITCH. Its values are command ID 340, local affected room #78781, direction 0, and the switch-movement flag 1; type-29 value[0] is a command, not a room VNUM. The source contract is documented by [`ITEM_SWITCH`](../../../src/core/defines.h#L144) and [`item_switch`](../../../src/specs/specs.object.c#L302); the local object fields are [caertannad.obj #78519](../../../areas/obj/caertannad.obj#L1752) at [lines 1757–1758](../../../areas/obj/caertannad.obj#L1757). No candidate Dalvik room dependency exists here, and the switch field is not a WLD or teleport edge.

Direct maintained C/C++ VNUM references are all local: [chaos equipment data](../../../src/account/chaos_eq_data.h#L100) names objects #78417, #78415, #78511, #78430, #78467, and #78423; [random-equipment data](../../../src/item/randomeq.c#L146) names object #78439; the [assignment table](../../../src/specs/specs.assign.c#L2213) binds mobile #78476; and [Caertannad specs](../../../src/specs/specs.caertannad.c#L29) name helper mobiles #78478 and #78480. These code declarations do not alter area ownership.

## Disposition

Keep active Caer Tannad DO NOT MOVE. Preserve its 19 existing story decisions; local switch #78519 names command ID 340 and affected room #78781. Candidate Shady2 remains separate from active Shady. This checkpoint changes documentation and typed-coverage fields only. It adds no candidate, changes no WLD exit totals, and makes no live gameplay claim. Editor/tool/export identity, persisted references, loading, teleport and switch operation, and installed play remain outside this static review.
