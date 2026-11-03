# Checkpoint 368: Village of Split Shield audit

Physical coverage line 281, `area:split_shield`, is mapped in the new [source-comprehensive story map](../zone-stories/VILLAGE_OF_SPLIT_SHIELD.md). This supplement closes the area's active/inactive graph identity, typed dependencies, title/history, and twenty source-backed builder recommendations. Static source review does not establish a played route or live reset/shop availability.

## Active and inactive WLD identity

The active [Split Shield WLD](../../../areas/wld/split_shield.wld) has 99 rooms (#10300–#10398), 213 authored exits, and no parse issues. All 213 exits resolve to active WLD rooms: 209 local directions and four exits from Split Shield to other active contributors. The local weak graph is one component of 99 rooms. There are five active inbound directions, four outbound directions, and four reciprocal pairs:

- Split Shield #10300 north to Surface #587095; Surface #587095 south to #10300.
- Split Shield #10327 east to shortc #53200; shortc #53200 west to #10327.
- Split Shield #10334 south to Surface #587895; Surface #587895 north to #10334.
- Split Shield #10340 west to Surface #587494; Surface #587494 east to #10340.
- White Harbor #55402 up to #10307 is the additional active inbound direction and has no Split Shield WLD return at that destination.

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without errors. No inactive file reuses a Split Shield room VNUM. Two source-only Surf directions enter the village: [Surf #206049 south to #10300](../../../areas/wld/surf.wld#L126993) and [Surf #206649 north to #10334](../../../areas/wld/surf.wld#L139561). Neither source room has an active WLD owner. Both room VNUMs also appear in other inactive WLDs with different titles: #206049 in surface2011; #206649 in surface2011 and underdark2011. Preserve the exact `surf.wld` edge and file identity on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); these are not active ingress. The reviewed source-only total rises by two to 4,851, of which Surf is 285.

The target has no unresolved WLD exit, fall marker, or current marker. All 24 local ZON D commands target declared WLD directions: four reset open, four closed, and sixteen closed/locked. Nine reciprocal pairs reset both faces with no state disagreement. These authored resets do not establish live door state. The three active source routes, the White Harbor inbound, and the two Surf approaches remain distinct from local level checks and local reset state.

## Typed dependencies, source census, and procedures

The current source inventory contains 34 local MOB definitions and 190 OBJ definitions, with no same-type WLD, MOB, or OBJ overlap against any active or source-only area file. Thirty local ZON object roles use active Heavens prototypes: five floor O placements of object #386 (oak water barrel), sixteen G placements of #376 (small leather backpack), eight E placements of #398 (wooden torch), and one E placement of #215 (studded leather sleeves). Representative references are [local ZON lines 125](../../../areas/zon/split_shield.zon#L125), [204](../../../areas/zon/split_shield.zon#L204), [205](../../../areas/zon/split_shield.zon#L205), and [699](../../../areas/zon/split_shield.zon#L699); the prototypes are in the active [Heavens OBJ file](../../../areas/obj/heavens.obj).

Active foreign `bs.zon` references two Split Shield objects: #10382 and #10340 at [lines 1926–1927](../../../areas/zon/bs.zon#L1926). Active [Black Shroud shop #74195](../../../areas/shp/bs.shp#L1082) lists the same objects at lines 1082–1083. #10340 is a local small-dog mobile and padded-leather-boots object under distinct namespaces. No active external QST item command or typed story-sidecar item reference points to a local object. The all-713-active-type-25 scan found no fixed destination in Split Shield. There is no local QST file, story sidecar, or SHP file; coverage's QST/AREA membership remains a list status, not a native contract. The two assigned mobile procedures are the area-specific direct C/C++ references; a bounded literal scan's other numeric matches are Unicode/scalar values, not additional local prototype bindings.

The [two procedure assignments](../../../src/specs/specs.assign.c#L928) lead to exact southward level checks in [specs.split_shield.c](../../../src/specs/specs.split_shield.c#L32): mobile #10301 at #10320 blocks an untrusted player below level 51; mobile #10302 at #10302 blocks an untrusted player above level 20 and below 57. Both restrictions apply to south commands. Keep those callback facts separate from WLD reachability and resets.

A reset-set comparison and census show 705 local ZON commands: M207, E153, G241, O80, and D24. Thirty-three of 34 local mobile prototypes have reset placements; aged wise orc #10330 is unplaced. The local object inventory has 51 playing-card prototypes, with #10476 absent. These are exact source facts, not installed-world behavior.

## Title and followed history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “Village of Split Shield” occurs once among all 442 current ZON titles and once among the 351 active contributors. Active and source-only WLD, MOB, and OBJ scans found no same-type VNUM overlap. Followed WLD, MOB, OBJ, and ZON histories reach the 2009-08-12 areas-tree move. Latest tracked edits are WLD 2013, MOB 2026-08-26, OBJ 2016, and ZON 2015. No alternate-stem rename, distinct predecessor, or whole-zone replacement appears in the followed paths or current active list. Keep active `split_shield` DO NOT MOVE.

## Builder questions and conservative recommendations

These twenty questions keep route, settlement, and population decisions visible without inventing native quests or completions.

1. Should active `split_shield` remain in place? Keep it DO NOT MOVE; current WLD, MOB, OBJ, ZON, and procedure bindings share this contributor identity.
2. Should the four reciprocal Surface/shortc WLD pairs remain the town's active borders? Preserve all four exact pairs until an owner names a changed route.
3. Should White Harbor's one-way up direction to #10307 remain? Preserve it as active ingress with no inferred Split Shield return; decide on a return only as a separate route change.
4. Should Surf's two inactive approaches be kept as source candidates? Retain the exact `surf.wld` source identity on Surf row 97; do not publish either direction as active ingress.
5. Should the reused Surf source VNUMs be treated as unique by number? Keep the titles and inactive file owners attached; surface2011 and underdark2011 reuse those IDs for different rooms.
6. Is gate_guard's below-level-51 south block at #10320 intended for all arriving players below that threshold? Preserve the exact bound mobile and scope; qualify fairness against the other area entries before altering it.
7. Is shady_man's level 21–56 south block at #10302 intended to overlap the gate guard's range? Keep the two checks separately named; do not collapse their distinct thresholds into one zone-level access claim.
8. Should trusted-character bypass and periodic-call no-op behavior remain unchanged? Preserve those callback boundaries while any future movement policy is reviewed.
9. Should the thorn patch route keep only its authored west exits and the two same-direction down declarations between #10316 and #10388? Keep the exact WLD records pending an explicit return-direction decision.
10. Should the Chief's War Room and basement doors retain their reset states and keywords? Preserve the 24 local D records as written; do not infer a key, reveal, or accessible shortcut from room prose.
11. Should the training compound's closed/locked transitions remain a guarded military area? Keep the exact WLD/D pairing and decide any access or key change as a complete route.
12. Should the Chief's planning and Swift Special Attack Team text remain environmental lore? Leave it outside counted campaign progress until native topics, admitted work, ownership, and lasting outcomes are authored.
13. Should aged wise orc #10330 remain unplaced? Keep the prototype separate from the reset population; add a placement only after the intended room, role, and cap are decided.
14. Is the 51-card object set an intentional incomplete deck or a missing queen of diamonds (#10476)? Preserve current object IDs and choose explicitly before filling that gap.
15. Should Heavens objects #215, #376, #386, and #398 remain on the local reset chains? Keep their Heavens object namespace, typed reset commands, and occurrence counts; decide stock ownership separately.
16. Should Black Shroud continue listing and resetting Split Shield objects #10382 and #10340? Preserve both source references and shop #74195 context; qualify transaction and reset behavior before calling them available stock.
17. Should the Barreling Brew Inn become a source-backed shop? Keep the local shop count at zero until a shop record and owner-defined transactions are present.
18. Should the #10340 dog and #10340 boots ever share story evidence? Preserve mobile/object types as separate prototypes across all resets, products, and descriptions.
19. Should the village's family, crime, religious, and military details become native progression? Keep them as authored descriptions unless builders define exact contacts, accepted conditions, branches, and durable outcomes.
20. What route evidence should count if the player meets a level check or uses an external approach? Record the actual allowed crossing/arrival and chosen origin; a warning, blocked command, or nearby room description does not establish completion.

## Register and candidate updates

Coverage row 281 now links the new story map and this audit supplement. Existing Surf candidate row 97 receives the two `surf.wld` approaches with both inactive VNUM-collision notes; no new candidate row is created. The new source map raises the detailed story set by one. No game data or procedure implementation changed.

## Bloodstone Checkpoint 778: object imports

Active Bloodstone has two ZON reset rows and two SHP product rows using Split Shield objects #10340 and #10382. Preserve the current prototypes; see the [Bloodstone typed-dependency audit](BS_AUDIT_REVIEW.md#outbound-shop-products).
