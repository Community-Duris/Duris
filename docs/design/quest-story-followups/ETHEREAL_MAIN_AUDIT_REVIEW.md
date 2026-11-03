# Ethereal Main audit review

This supplement records the source-only review of the active `ethereal_main` contributor. It separates authored WLD exits from object and spell travel, and does not qualify current server state or live gameplay.

## Source inventory

The full-world roster lists `ethereal_main` at `*124-125` with zone header #124. The active files are `areas/wld/ethereal_main.wld`, `areas/mob/ethereal_main.mob`, `areas/obj/ethereal_main.obj`, and `areas/zon/ethereal_main.zon`. The WLD contains 150 rooms (#12401-#12550), the MOB contains eight prototypes (#12400-#12460), the OBJ contains 32 prototypes (#12400-#12484), and the ZON contains 90 non-S reset commands: 52 M, 20 E, 13 O, three G, and two F. It has no D reset commands. The zone header sets reset mode 1.

`areas/qst/AREA` also lists the contributor, but `areas/qst/ethereal_main.qst` is absent. There is no local SHP source, story sidecar, or native Q/QA contract. The coverage record is [row 92](ZONE_COVERAGE_REGISTER.csv#L92).

## Narrative evidence

148 of the 150 rooms use the title “The Ethereal Plane.” Their text repeatedly describes weightlessness, grayness, floating debris, and partial impressions of other dimensions. The two exception rooms are #12494, “A Void in Time,” and #12511, “A Pocket of Antimatter.” The former has no light, dust, or movement; the latter describes sparks, dark clouds, solar wind, and distorted time, color, and space.

The reset roster supplies recurring inhabitants: nightmares, shadow daemons, ethereal daemons, smoke mephits, mist mephits, and grey thought-eaters. Menzellon, reset at #12450, is bound to `menzellon_shout`. The handler can call local Ethereal helpers to pursue a target. No local quest script organizes these encounters; the QST/AREA listing alone does not provide a local quest file.

## Authored WLD graph

The 900 source exits all resolve in the active loader inventory. There are 898 local exits and two exits out of the zone. All exit door flags are zero, the local ZON has no D commands, and the source contains no fall-chance or current fields. The local WLD graph is one weak component. From each unique active WLD arrival room, #12544, #12442, and #12480, all 150 rooms are reachable and all 150 have a path back to that arrival in the directed source graph.

| Boundary | Authored exit | Relationship |
| --- | --- | --- |
| Ethereal #12442 to Dream #31301 | Down, [Ethereal exit-register row 219](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L219) | Reciprocal: Dream #31301 has an up exit to #12442 at [row 435](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L435). |
| Ethereal #12480 to Dreggan Woods #41001 | West, [Ethereal exit-register row 220](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L220) | Reciprocal: Dreggan #41001 has an east exit to #12480 at [row 556](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L556). |
| Prison #7486 to Ethereal #12544 | Five exits: north, south, west, up, and down; [register rows 149-153](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L149) | Five one-way source records enter the same Ethereal room; no matching Ethereal WLD exit to Prison was found. |

The zone’s 150-room WLD graph is fully connected from those three active arrival roots at source level. Reset availability, item presence, and a player's actual access were not observed. The cross-area counts remain separate from item transport below. A scan of all 95 unregistered WLD files found no room-number overlap with #12401-#12550 and no source-only exit targeting those rooms.

## Active object routes into Ethereal rooms

The following ten active ZON placements use objects whose source-declared target is an Ethereal room. Type-25 objects use the generic ITEM_TELEPORT command path; pool objects use `magic_pool` or `teleporting_pool` callbacks.

| Source area and object | Active reset placement | Source mechanism | Ethereal target |
| --- | --- | --- | --- |
| Alatorin #83432 ([prototype](../../../areas/obj/alatorin.obj#L5348)) | #83823, [reset line 1750](../../../areas/zon/alatorin.zon#L1750) | ITEM_TELEPORT | #12511 |
| Ardgral #30403 ([prototype](../../../areas/obj/ardgral.obj)) | #30457, [reset line 24](../../../areas/zon/ardgral.zon#L24) | `teleporting_pool` | #12401 |
| Astral Side Areas #19860 ([prototype](../../../areas/obj/astral_main.obj#L570)) | #19842, [reset line 74](../../../areas/zon/astral_areas.zon#L74) | `magic_pool` | #12479 |
| eth2 #32640 ([prototype](../../../areas/obj/eth2.obj)) | #32632, [reset line 129](../../../areas/zon/eth2.zon#L129) | ITEM_TELEPORT | #12494 |
| Negative Plane #26656 ([prototype](../../../areas/obj/negplane.obj)) | #26616, [reset line 162](../../../areas/zon/negplane.zon#L162) | ITEM_TELEPORT | #12494 |
| Air Plane #24404 ([prototype](../../../areas/obj/plane_air_one.obj)) | #24422, [reset line 10](../../../areas/zon/plane_air_one.zon#L10) | `magic_pool` | #12451 |
| Earth Plane #23804 ([prototype](../../../areas/obj/plane_earth_one.obj)) | #23822, [reset line 9](../../../areas/zon/plane_earth_one.zon#L9) | `magic_pool` | #12492 |
| Fire Plane #25404 ([prototype](../../../areas/obj/plane_fire_one.obj)) | #25422, [reset line 9](../../../areas/zon/plane_fire_one.zon#L9) | `magic_pool` | #12459 |
| Water Plane #23204 ([prototype](../../../areas/obj/plane_water_one.obj)) | #23222, [reset line 9](../../../areas/zon/plane_water_one.zon#L9) | `magic_pool` | #12492 |
| Ravenloft2 #59089 ([prototype](../../../areas/obj/ravenloft2.obj)) | #59169, [reset line 928](../../../areas/zon/ravenloft2.zon#L928) | ITEM_TELEPORT | #12494 |

The shared pool code moves a `teleporting_pool` object within its current zone on a periodic random chance; an attempted entry delegates to `magic_pool`, which resolves value 0 as a room target. The Astral pool #19860 carries damage value 400, which its handler applies to non-trusted entrants. These are source-defined possibilities. Reset mode, current object position, restrictions, command availability, live access, and safe return were not checked.

Generic plane-travel spells also use #12401 as the Ethereal plane anchor in `planes_room_num`. Their source path selects a suitable room from the target plane zone, subject to spell and room restrictions. This is code-level travel support, not a measured ingress path.

## Local outbound objects and dormant rift records

| Object and placement | Declared target | Source behavior |
| --- | --- | --- |
| #12410 in #12511, [reset line 17](../../../areas/zon/ethereal_main.zon#L17) | eth2 #32632 | Type 25 with command value 7 (`CMD_ENTER`); the generic handler reads its target value. The object is also bound to `artifact_hide` in [specs.assign.c line 1392](../../../src/specs/specs.assign.c#L1392). |
| #12460 in #12438, [reset line 14](../../../areas/zon/ethereal_main.zon#L14) | Astral Main #19702 | `magic_pool`, [binding line 1915](../../../src/specs/specs.assign.c#L1915). |
| #12470 in #12530, [reset line 18](../../../areas/zon/ethereal_main.zon#L18) | Fire Plane #25410 | `teleporting_pool`, [binding line 1916](../../../src/specs/specs.assign.c#L1916). |
| #12472 in #12421, [reset line 11](../../../areas/zon/ethereal_main.zon#L11) | Earth Plane #23901 | `teleporting_pool`, [binding line 1917](../../../src/specs/specs.assign.c#L1917). |
| #12474 in #12470, [reset line 15](../../../areas/zon/ethereal_main.zon#L15) | Air Plane #24401 | `teleporting_pool`, [binding line 1918](../../../src/specs/specs.assign.c#L1918). |
| #12476 in #12539, [reset line 20](../../../areas/zon/ethereal_main.zon#L20) | Water Plane #23324 | `teleporting_pool`, [binding line 1919](../../../src/specs/specs.assign.c#L1919). |
| Celestia object #45536 in #12401, #12406, and #12431, [resets at lines 9-13](../../../areas/zon/ethereal_main.zon#L9) | Celestia #45500 | `teleporting_pool`, [binding line 1920](../../../src/specs/specs.assign.c#L1920). The imported prototype is in [celestia.obj](../../../areas/obj/celestia.obj#L534). |

The generic ITEM_TELEPORT path checks an object's command value and target, and the command dispatcher calls it during movement handling ([handler](../../../src/magic/spell_travel.c#L932), [dispatcher](../../../src/cmd/interp.c#L2412)). The local #12410 portal's value 7 is `CMD_ENTER`.

Seven additional local rift prototypes, #12462, #12464, #12466, #12478, #12480, #12482, and #12484, have target values and descriptions but no local reset placements. Their `teleporting_pool` bindings are inside a disabled `#if 0` block in [specs.assign.c lines 1921-1929](../../../src/specs/specs.assign.c#L1921). Their target numbers map to Underworld #4577, Northern Wilderness #12310, Moonshae #26205, Caves Skelenak #4082, and three rooms present only in unregistered Duris3.wld (#163372, #141971, #141960). These records do not establish active travel. Local object #12468 stores target #12401 but has no callback binding or reset placement. Astral Main object #19863 is bound to `magic_pool` and targets #12492, but no active ZON reset placement was found for it.

## Reset imports, quest references, and compiled behavior

The local ZON loads object #45536 from Celestia three times. It also gives Menzellon external objects #112 and #359 from Heavens and #55201 from Winterhaven. The referenced prototypes resolve by object role. The E reset on ethereal daemon #12430 at #12404 requests object #18202, but no OBJ prototype for that number exists in active or unregistered object sources. #18202 is a room in active Wolfen.wld, so the number match does not satisfy the object reset ([local reset](../../../areas/zon/ethereal_main.zon#L29), [Wolfen room](../../../areas/wld/wolfen.wld#L5212)).

External active content consumes local prototypes in two typed places:

- Ravenloft's ZON has an F reset for local mobile #12401 at room #58544 ([reset line 1109](../../../areas/zon/ravenloft.zon#L1109)).
- Heavens' active QST uses local object #12401, the cloak of ethereal mists, and #12405, Silencer, as `G I` items at lines 85-86 ([quest source](../../../areas/qst/heavens.qst#L85)).

No active SHP product row or active story sidecar typed VNUM points to a local Ethereal object or mobile. Local object #12401 and mobile #12401 are separate prototypes in separate namespaces; the Ravenloft F row resolves to the mobile and the Heavens QST row resolves to the object.

Compiled behavior adds two source relationships. Menzellon mobile #12400 uses `menzellon_shout`; the helper list names local mobiles #12401, #12410, #12420, #12430, #12440, #12450, and #12460 as pursuers ([binding](../../../src/specs/specs.assign.c#L1217), [handler](../../../src/specs/specs.elemental_bosses.c#L15)). Plane travel uses room #12401 to locate Ethereal Main in `planes_room_num` ([table](../../../src/magic/spells.c#L295)).

## Title, history, and disposition

After color-code removal and normalization, “The Ethereal Plane” is unique among all 442 current ZON titles. Followed same-stem history remains on `ethereal_main`: the WLD and OBJ histories reach 2013-11-13, while the MOB and ZON histories reach 2016-10-31. The source tree move appears in the 2009 history. The MOB changes include the removal of the mount flag from nightmares. No alternate-stem predecessor or explicit replacement is established by the checked-in title and followed histories.

Keep active `ethereal_main` DO NOT MOVE. The inactive-WLD sweep adds no candidate edge or room overlap, so candidate records and curated source-only totals stay unchanged. Coverage now maps 257 of 351 active contributors, leaving 94 unmapped; the detailed story-review set is 50. Case-insensitive PENDING counts are 197 narrative, 381 connectivity, 191 title/history, and 364 typed-dependency rows. Curated source-only totals remain 3,460 overall, 221 for Surf, and 17 for ud_map2; Underworld2 retains eight approaches.

Static source evidence does not establish current reset state, random-pool position, object availability, spell access, safe return, or live player traversal. No build, test, server, gameplay, database, migration, or operational script was run for this documentation audit.
