# Village of Split Shield: comprehensive source story map

Reviewed October 10, 2026. Source area `split_shield`, zone 103. This map covers 99 WLD rooms (#10300–#10398), 34 mobile prototypes, 190 object prototypes, and 705 local reset commands. The active WLD routes, native procedures, population descriptions, equipment, and typed cross-area source references are mapped here. No local QST file, story sidecar, or SHP file exists; the QST/AREA listing alone does not create a native contract.

## Evidence and review boundary

Reviewed the complete active [room file](../../../areas/wld/split_shield.wld), [mobile file](../../../areas/mob/split_shield.mob), [object file](../../../areas/obj/split_shield.obj), and [reset file](../../../areas/zon/split_shield.zon). A repository-wide supplement records active and inactive WLD identity, typed references, title/history, and builder questions: [Checkpoint 368 audit](../quest-story-followups/SPLIT_SHIELD_AUDIT_REVIEW.md).

The local zone header uses reset mode 2. Its 705 commands comprise 207 M, 153 E, 241 G, 80 O, and 24 D records; there are no local F commands. Thirty object placements reference active Heavens prototypes rather than the local OBJ file. All source statements below remain static records. Reset eligibility, live stock, movement callbacks, and player access still require installed-world qualification.

## Settlement geography and defense

| Room family | Authored setting |
| --- | --- |
| #10300–#10318 | A worn forest trail, a wilderness campsite at #10310, and a thick thorn patch at #10314–#10316. The final patch room has a down exit to the Chief's Basement. The basement's separate down exit returns to the patch; both faces are authored as down. |
| #10319–#10347 | The fenced perimeter roads, four named gates, patrol intersections, and four watch-tower ladders. The wall and tower descriptions present the settlement as an organized, guarded village. |
| #10348–#10359 | The main north/south village path, city square, and east/west village roads. The square description calls out commerce and social status. |
| #10360–#10365, #10369–#10373 | Commoner and merchant huts, the medicine man's home, and ordinary household rooms. The room descriptions show class differences between a “filthy hut” and larger merchant homes. |
| #10366–#10368, #10384 | The Barreling Brew Tavern and Inn, its connected gambling room below, the card tables, and room descriptions of a crowded low-cost drinking house. The source has no local shop file. |
| #10374–#10383, #10397–#10398 | Guard training rooms, food storage, recruits' and recreation rooms, a planning room, the covert Swift Special Attack Team quarters, parade ground, and officers' recreation room. Their text distinguishes ordinary recruits from an elite force hidden from most villagers. |
| #10385–#10388 | The Chief's War Room, living room, dusty bedroom, and safe-room basement. The War Room connects to the Chief's Basement; several intervening faces are reset closed or locked. |
| #10389–#10396 | Four watch-tower ladder pairs over the village gates. |

The active graph has 213 resolved directions: 209 stay within the area and four leave it. One weak component contains all 99 rooms. Four WLD exits have active reciprocal area boundaries: Split Shield #10300 north / Surface #587095 south; #10327 east / shortc #53200 west; #10334 south / Surface #587895 north; and #10340 west / Surface #587494 east. A fifth active inbound edge comes from White Harbor #55402 up to #10307, with no Split Shield WLD return at that destination. The full tuples and inactive approaches are in the audit supplement.

## Residents, institutions, and native interaction boundary

The 34 mobile prototypes divide into these authored groups:

| VNUMs | Source-described roles |
| --- | --- |
| #10301, #10310–#10318 | Gate guard, patrolling and experienced guards, trainer, huge guard, intelligence-gathering special guard, assigned protector, and elite village guard. |
| #10302 | A shady old man who scans the horizon for someone who never comes. His assigned procedure can block southward travel at certain levels. |
| #10320–#10330 | Common male/female residents, mothers, fathers, older residents, cubs, adults, teenagers, and the aged wise orc. Mobile #10330 has no local reset placement. |
| #10340–#10343 | Small and large dogs, a large warg, and a brown wolf. Their descriptions place scavenging and stray animals among the village streets. |
| #10350–#10352 | The village Chief, his shaman advisor, and a young shaman. The Chief plans to revitalize Split Shield; the shaman combines intelligence with religious leadership. |
| #10353–#10354 | A recurring drunk and a card-playing orc whom the description associates with organized crime. |
| #10360–#10363 | The Swift Special Attack Team warrior, mercenary, thief, and shaman. The room and mobile descriptions frame them as a covert elite unit. |

The village's language of planning, revitalization, family life, criminal influence, and military readiness comes from room and mobile descriptions. It has no local [QST source](../../../areas/qst/AREA) or story-sidecar episodes, so none of those descriptions is a native completion, political outcome, rescue, learned topic, or required campaign stage. The local reset mode and QST/AREA membership do not supply those absent receipts.

Two assigned procedures are the only area-specific literal mobile bindings. In [specs.assign.c](../../../src/specs/specs.assign.c#L928), mobile #10301 receives `gate_guard` and #10302 receives `shady_man`. The implementation in [specs.split_shield.c](../../../src/specs/specs.split_shield.c#L32) blocks an untrusted player's south command from room #10320 when the player's level is below 51. The second procedure blocks south from #10302 when the player is above level 20 and below 57. Both checks return false for periodic events, and trusted characters bypass the blocks. Their message text describes a warning or challenge; neither procedure records quest progress.

## Object and reset inventory

The 190 local object prototypes are mostly equipment and ordinary household or street objects: 70 armor pieces, 25 weapons, 11 shields, five jewelry objects, 51 playing cards, nine potions, five food prototypes, seven type-12 miscellaneous prototypes, four rocks, one purse, a fountain, and one version tag. The playing-card set is a source list, not evidence of a complete 52-card deck: it contains 51 object records, with #10476 absent. The repetitive armor and weapon families appear as NPC equipment in local E resets; the card objects align with the tavern's gambling-room descriptions. The local source has no local shop file even though a room is called an Inn.

The 30 cross-area object reset roles in the local ZON file resolve to active Heavens prototypes: object #386, an oak water barrel, is floor-placed in five rooms; object #398, a wooden torch, is equipped eight times; object #376, a small leather backpack, is given sixteen times; and object #215, a pair of studded leather sleeves, is equipped once. Representative [local reset records](../../../areas/zon/split_shield.zon#L125) point to their typed [Heavens prototypes](../../../areas/obj/heavens.obj#L4036). These items remain Heavens OBJ identities.

An active foreign source also uses local prototypes. In [bs.zon](../../../areas/zon/bs.zon#L1926), the shop-keeper reset chain references local OBJ #10382, well-crafted leather boots, and #10340, padded leather boots. [Shop #74195 in bs.shp](../../../areas/shp/bs.shp#L1082) lists both as products. Object #10340 shares its number with local mobile #10340, the small dog; keep the ZON and SHP references in the object namespace. These lines establish source listings and reset declarations, not live stock or a successful sale.

No local native QST contract consumes an object, no external QST or typed story-sidecar item reference points to a local object, and no fixed active type-25 destination targets a Split Shield room. The town's WLD boundaries and the foreign reset/shop links are separate evidence types.

## Scope and repair boundary

The area is an authored orc settlement with explicit gates, guard ranks, an inner training compound, the Chief's military quarters, a tavern and gambling room, family residences, street animals, and a covert attack team. That source framing is strong environmental context; it does not define a tracked village-liberation or military-campaign endpoint. The area-specific procedures enforce two source-authored level restrictions only. Preserve the 51-card object set, unplaced wise-orc prototype, typed external equipment references, and reset/exit states until an owner makes a separate content decision.

Static inspection does not establish how many guards are present at runtime, whether a given level route is fair, whether an inn transaction exists, or how source items are obtained and renewed. The audit supplement records those builder decisions without changing source or claiming gameplay.
