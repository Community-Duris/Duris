# Patrols: connectivity, story, dependencies, and retention review

This bounded source review accompanies the [Patrol Guard Headquarters dossier](../zone-stories/PATROLS.md). It reconciles the seven load-room graph, all 28 unresolved exits, the all-95 inactive-WLD sweep, guard and equipment resets, compiled room bindings, QST/story coverage, VNUM collisions with the existing Fists of Fury Guildhall candidate, normalized title, and followed file history. It does not establish installed runtime behavior or a played route.

## Active graph, unresolved targets, and source-only approaches

The active source has seven rooms and 28 authored exit records. None resolves to another Patrols room or any current active WLD room. All seven rooms are singleton local components; from any room the raw local graph reaches and can return to that room alone. There are no active cross-area boundary records, inbound active exits, negative targets, fall-chance fields, or currents.

The 28 records have zero exit flags and key VNUMs, blank exit descriptions, and targets outside the Patrols range. Each is positive-unresolved in the full active loader. They are recorded together in [unresolved-exit register lines 151–178](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L151). Every target VNUM appears in inactive surf and surface2011 WLD sources; three also appear in underdark2011. No target should be treated as a live route or as uniquely owned from its number alone.

| Load room | Raw exits |
| --- | --- |
| #30500, Charing | north #207259; east #210872; south #215651; west #209935 |
| #30501, Ugta | north #209138; east #212448; south #214228; west #211218 |
| #30502, Tharnadia | north #225323; east #229235; west #228913; up #235526 |
| #30503, Woodseer | north #225662; east #228673; west #228948; up #233166 |
| #30504, Ashrumite | north #218475; east #222076; west #223248; up #224758 |
| #30505, Kimordril | north #213662; east #231981; south #219052; west #222042 |
| #30506, Marigot | north #221022; east #219847; south #224041; west #224618 |

The inactive-map room identities disagree. For example, #209935 is “The Great Sylvan Woods of Mielikki” in surf, “Mountains” in surface2011, and “Ageless Mountains of The Jade Empire” in underdark2011. #218475 is a Silverbeard trail, a grassy flatland, or a rock-filled cavern across those same candidates. #222076 is a forest, a desolate hillside, or a low-ceiling cavern. Many surface2011 matches use the title “The Azure Waters of a Stormy Sea” even where surf has a different landscape. Preserve active references on the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [Surface2011 row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98), and [UnderDark2011 row 115](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115); their active reference counts do not change.

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. It found seven room-VNUM collisions, all in inactive fists_gh.wld, and 14 source exits into Patrols VNUMs. Twelve exits originate in source rooms #30500–#30506, whose identities collide with Patrols. Two originate in non-overlapping Fists of Fury rooms: #30507 east to #30506 and #30509 up to #30503. Preserve those two records on the existing [Fists of Fury candidate row 42](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42) as source-only numeric approaches with unresolved destination identity; they are not active ingress. The curated source-only total rises by two to 4,251. The Surf subset remains 252, ud_map2 remains 19, and Underworld2 remains eight.

## Reset population and story boundary

The ZON has 70 M commands and seven E commands: each load room receives six common soldiers, three elite soldiers, and one captain, followed by one captain equipment placement. Across the 21 mobile prototypes, the seven source-named forces are Tharnadian, Ugta barbarian, Ashrumite gnome, Woodseer halfling, Kimordril dwarf, Charing elf, and Marigot centaur.

All local room descriptions are blank. The mobile text says the guards patrol their hometown countryside and oppose unspecified “forces of evil”; captains watch for trouble and offer to protect “the lesser people.” There is no named campaign, enemy, order, quest outcome, or specific relationship among the seven forces. The equipment is paired one per captain, but the data supplies no quest use or stated reward purpose.

Patrols appears in both areas/AREA and areas/qst/AREA, but has no local QST file, SHP file, or story sidecar. A numeric scan found no external exact QST item command, active SHP product, or story-sidecar VNUM reference to Patrols objects or mobiles. The QST-area entry alone does not prove that a local contract exists.

## External identity collisions and typed dependencies

The unregistered Fists of Fury Guildhall component uses the same zone number 305 and defines rooms #30500–#30506 with different descriptions and layouts from active Patrols. Its rooms include a Guard room, corridor, store, conference room, inn, board room, and portal room ([Fists of Fury WLD](../../../areas/wld/fists_gh.wld#L1); [Fists of Fury ZON](../../../areas/zon/fists_gh.zon#L1)). It also defines different mobiles #30500–#30503 as golems and a merchant, which collide with Patrols’ Tharnadian, Ugta, gnome, and halfling soldiers ([Fists of Fury mobiles](../../../areas/mob/fists_gh.mob#L1)).

The unregistered Fists of Fury ZON resets those colliding mobiles into its overlapping rooms, and puts foreign object #11013 in its #30504 inn. Its SHP record uses mobile #30503 as shopkeeper and room #30502 as shop room ([SHP keeper](../../../areas/shp/fists_gh.shp#L49), [SHP room](../../../areas/shp/fists_gh.shp#L51), [shop field parsing](../../../src/economy/shop.c#L3489)). These references belong to the existing unregistered candidate, not active Patrols resets or shops. Fists of Fury is absent from both current area manifests. Its identity is not proven to be an earlier or later Patrols version.

Compiled source has two exact room-number assignments under the “Players Guild” section: active code binds room #30504 to the inn function and also names #30511 ([room bindings](../../../src/specs/specs.assign.c#L2445)). In active Patrols WLD, #30504 is the Gnomish Guard Load Room; no current WLD file defines room #30511. Treat this as a compiled VNUM ownership issue for the builder to reconcile, not proof of a usable inn or a live route.

The road-ranger event code contains generic mobile movement behavior, but the bounded exact-VNUM source scan found no direct binding of Patrols’ 21 mobile IDs to that procedure. Do not infer that these guards roam from “patrol” prose alone.

No duplicate Patrols mobile or object prototype exists in another active area. The only exact WLD and mobile definition collisions found are the excluded Fists of Fury sources described above. The active and source-only ZON, WLD, shop, and compiled references are recorded without merging the two identities.

## Title, history, and retention

After color stripping and case folding, “The Patrol Guard Headquarters” is unique among all 442 current ZON titles. Followed WLD, MOB, OBJ, and ZON history stays on the patrols stem through the 2009 move into the current areas tree and later same-stem format and data updates. No alternate-stem rename, active predecessor, or whole-zone replacement was established. The distinct Fists of Fury Guildhall source shares numeric space but does not establish a replacement relationship.

Keep active patrols **DO NOT MOVE**. Preserve all seven rooms, 21 mobile prototypes, seven equipment objects, the 28 unresolved WLD references, the compiled room bindings, and the current VNUM ownership while builders resolve their intended semantics. Keep Surf, Surface2011, UnderDark2011, and Fists of Fury on their existing RETAIN IN PLACE candidate rows; add no candidate row and do not change any candidate active-reference count.

## Checkpoint 318: twenty builder questions and recommendations

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PAT-01 | Is “The Patrol Guard Headquarters” the canonical name, and should “Guard Map Mobs” remain only an area-list label? | Retain the current ZON title and area-list text until the owner confirms any public alias. |
| PAT-02 | Are these seven rooms a physical headquarters, hidden mob staging rooms, or intended travel anchors? | Preserve the seven source identities; do not invent a physical layout or purpose before the area owner identifies it. |
| PAT-03 | Should the blank room descriptions and current sector values remain as loader rooms? | Keep them unchanged until the owner specifies whether players should reach and see these rooms. |
| PAT-04 | Is Patrols intentionally listed in areas/qst/AREA despite having no local QST file? | Preserve the manifest entry while its purpose is checked; do not infer a local quest. |
| PAT-05 | Are the seven hometown assignments—Charing, Ugta, Tharnadia, Woodseer, Ashrumite, Kimordril, and Marigot—correct for these guard units? | Retain the exact names and prototype mappings until each area owner confirms the affiliation. |
| PAT-06 | Is the repeated six-common, three-elite, one-captain reset pattern intentional for every force? | Preserve all 70 M rows and confirm reset caps and intended live counts before changing any repetitions. |
| PAT-07 | Should one captain in each faction wear its matching regional object? | Preserve the seven E placements and item IDs until the captain-to-equipment pairing is confirmed. |
| PAT-08 | Are the seven equipment names, descriptions, and attributes canonical, including the current spellings? | Keep the prototype identities and stats; make copy edits only after item owners approve the wording. |
| PAT-09 | Which enemy, threat, or campaign do the guards mean by “forces of evil”? | Keep that line as source lore; add a named conflict only with an owner-supplied objective and outcome. |
| PAT-10 | Is “the lesser people” the intended voice for the captains’ duty? | Retain the text until the writer confirms its perspective and tone. |
| PAT-11 | Are the 28 exits from the guard load rooms meant to connect to outdoor patrol routes? | Keep the authored records while the map owner identifies their intended function; they do not resolve in the current loader. |
| PAT-12 | Which of surf or surface2011 owns the intended destinations for the 28 exit VNUMs? | Preserve both existing candidates and every unresolved target until room identity is reconciled. |
| PAT-13 | For #209935, #218475, and #222076, should the outdoor Surf/Surface2011 identities or the UnderDark2011 cave and mountain identities apply? | Keep all three candidate identities distinct; do not redirect those exits based on VNUM alone. |
| PAT-14 | Should the unresolved exits be redirected to current active rooms or removed as obsolete map links? | Make no WLD edit until the source-area owner confirms a destination or retirement decision for each record. |
| PAT-15 | Is the current lack of active ingress intentional for all seven load rooms? | Preserve current loader ownership while the owner decides whether guards should be accessible from active world paths. |
| PAT-16 | Should the guards roam, or are “patrol” and “watches” descriptive flavor? | Keep the text-only interpretation; add or bind roaming behavior only after a movement design is approved. |
| PAT-17 | Is the compiled inn binding to active room #30504 intentional when that room is the Gnomish Guard Load Room? | Preserve the source binding while its owning room is identified; do not advertise a live inn from the assignment alone. |
| PAT-18 | What room, if any, should the compiled #30511 inn assignment identify? | Leave the VNUM unresolved until the code owner names the intended room; no current WLD source defines it. |
| PAT-19 | Is the unregistered Fists of Fury Guildhall source a separate retired area, an older design, or another use of the same VNUMs? | Preserve it on existing candidate row 42; do not infer that it was replaced by Patrols. |
| PAT-20 | Should Fists of Fury’s #30507 and #30509 exits be treated as approaches to Patrols, or as old Guildhall links into its own conflicting rooms? | Retain the two source-only records and the VNUM conflict separately until the Fists of Fury owner resolves their destination identity. |
