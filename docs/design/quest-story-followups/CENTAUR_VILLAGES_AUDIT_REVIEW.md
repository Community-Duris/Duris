# Centaur Villages typed-dependency audit

Checkpoint 780, reviewed October 12, 2026. This static review closes the typed-reference inventory for active Centaur Villages and its registered attic candidates. It records prototype ownership, item-specific room fields, and compiled VNUM uses; it does not qualify loading, switch operation, item creation in play, or route safety. No native gameplay or area source was changed.

## Active source inventory and owned references

`centaur_zone` is an active full-area contributor with an indexed QST file and a structured story sidecar. It has 100 rooms (#93300–#93399), 29 mobiles (#93300–#93328), 31 objects (#93300–#93330), five local QST giver sections, seven native contracts, and no local SHP file. No same-type room, mobile, or object VNUM overlaps another current source file.

All 193 typed ZON reset rows reference local prototypes: 70 M and seven F mobile rows, plus 21 O, one E, 23 G, and 71 P object rows. No external or unresolved ZON prototype reference was found. All five QST giver headers resolve to local mobiles (#93301, #93302, #93308, #93309, #93310). Its 17 G/R I item commands reference seven local objects: #93310, #93311 (4 rows), #93312, #93313 (8 rows), #93314, #93317, and #93330. The structured sidecar has 22 `giver_vnum`, eight `mob_vnum`, and eight `item_vnums` roles; all 38 resolve to local prototypes.

No active or registered-candidate ZON reference, external QST giver/item command/QC room coordinate, SHP product, or story-sidecar role references a Centaur mobile, object, or room. No active or candidate shop product or inbound typed object-room field targets this area. There is no outgoing foreign reference or unresolved prototype number in the audited reset, QST, or sidecar fields.

## ITEM_SWITCH room fields

Centaur owns five type-29 `ITEM_SWITCH` objects. In this format, `value[0]` is the trigger command, `value[1]` is the room containing the affected blocked exit, `value[2]` is its direction, and `value[3]` records whether the switch object moves. All five `value[1]` room VNUMs are local:

| Switch object | Trigger command | Affected local room | Direction | Object moves |
| --- | ---: | ---: | ---: | ---: |
| #93305 vines | 270 | #93377 | 0 | 1 |
| #93306 vines | 270 | #93375 | 2 | 1 |
| #93307 vines | 270 | #93363 | 0 | 1 |
| #93308 vines | 270 | #93365 | 2 | 1 |
| #93323 jagged rock | 270 | #93326 | 1 | 0 |

The data and engine contract are recorded in [centaur_zone.obj](../../../areas/obj/centaur_zone.obj#L77), [`ITEM_SWITCH`](../../../src/core/defines.h#L144), and [`item_switch`](../../../src/specs/specs.object.c#L302). The repeated 270 is a command ID; it is not a room destination and does not reference source-only candidate `dalvik.wld` room #270. A current active/candidate object scan found no external ITEM_SWITCH `value[1]` field or ITEM_TELEPORT destination into Centaur, and Centaur defines no type-25 ITEM_TELEPORT object.

## Compiled asset reference

The only exact local object/mobile VNUM match in maintained C/C++ source is [event_conjure_water](../../../src/classes/innates.c#L3487), which reads object #93300 and places the freshwater spring pool in the caster's room with a timed decay. No other exact VNUM reference or special-procedure assignment to a Centaur prototype was found in the maintained-source scan. This code-owned asset reference does not create a separate area or candidate dependency.

## Disposition

Keep active Centaur Villages DO NOT MOVE. The existing dossier already asks the owner whether the four vine gates and rock switch should provide the configured reciprocal routes; this closure adds no duplicate builder question. Preserve all local reset, QST, sidecar, and switch fields. No candidate row or candidate plan is changed, no WLD exit total changes, and no live route or item-use claim is made. Loading, switch interaction, timing, player safety, editor/tool export, persisted references, and installed play remain separate qualification work.
