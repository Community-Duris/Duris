# The Patrol Guard Headquarters

Active source stem **patrols**, zone 305, contains seven “Guard Load Room” records (#30500–#30506), 21 mobile prototypes (#30500–#30520), and seven armor objects (#30500–#30506). It appears in both the full-world and QST area manifests. The source set has WLD, MOB, OBJ, and ZON files, but no local QST, SHP, or story sidecar.

The content reads as a staging set for seven hometown guard forces: Tharnadia, Ugta, Ashrumite, Woodseer, Kimordril, Charing, and Marigot. Each faction has common soldiers, elite soldiers, and a captain, with one distinctive piece of equipment placed on its captain. Room prose is empty, the WLD titles call the rooms load rooms, and the mobile text supplies the only environmental story: these guards patrol their countryside and defend their homes from unspecified forces of evil.

Sources: [world rooms and exits](../../../areas/wld/patrols.wld#L1), [mobile prototypes](../../../areas/mob/patrols.mob#L1), [equipment prototypes](../../../areas/obj/patrols.obj#L1), [population and equipment resets](../../../areas/zon/patrols.zon#L1), [full-world membership](../../../areas/AREA#L147), [QST-area membership](../../../areas/qst/AREA#L136), [connectivity, identity, and dependency review](../quest-story-followups/PATROLS_AUDIT_REVIEW.md).

## Guard stations

| Room | Source title and faction | Raw WLD destinations |
| --- | --- | --- |
| #30500 | Elven Guard Load Room; Charing | North #207259, east #210872, south #215651, west #209935 |
| #30501 | Barbarian Guard Load Room; Ugta | North #209138, east #212448, south #214228, west #211218 |
| #30502 | Tharnadian Guard Load Room | North #225323, east #229235, west #228913, up #235526 |
| #30503 | Halfling Guard Load Room; Woodseer | North #225662, east #228673, west #228948, up #233166 |
| #30504 | Gnomish Guard Load Room; Ashrumite | North #218475, east #222076, west #223248, up #224758 |
| #30505 | Dwarven Guard Load Room; Kimordril | North #213662, east #231981, south #219052, west #222042 |
| #30506 | Centaur Guard Load Room; Marigot | North #221022, east #219847, south #224041, west #224618 |

The seven WLD room descriptions are blank. Each room has three or four exits, all with zero flags and key VNUM, no exit text, and targets outside this seven-room set. Every target is pruned by the current active loader. The target VNUMs appear only in excluded source maps, and in several cases the candidate maps assign the same number to different terrain or cave descriptions. See the [audit review](../quest-story-followups/PATROLS_AUDIT_REVIEW.md#active-graph-unresolved-targets-and-source-only-approaches) before interpreting any destination as a live road.

## The seven guard forces

The reset file has 70 M commands and seven E commands. Each station receives six common soldiers, three elite soldiers, and one captain; the E command equips that captain. These records describe a repeated population pattern rather than a narrated encounter or quest.

| Hometown force | Common / elite / captain prototypes | Captain’s reset equipment |
| --- | --- | --- |
| Charing elves, room #30500 | #30505 / #30512 / #30520 | #30501, etched moorwood bracelet |
| Ugta barbarians, room #30501 | #30501 / #30508 / #30515 | #30500, thick caribou-hide cowl |
| Tharnadian humans, room #30502 | #30500 / #30507 / #30514 | #30502, plumed blacksteel helm |
| Woodseer halflings, room #30503 | #30503 / #30509 / #30517 | #30503, ebonwood shoulder guards |
| Ashrumite gnomes, room #30504 | #30502 / #30510 / #30516 | #30504, gem-studded eyepiece |
| Kimordril dwarves, room #30505 | #30504 / #30511 / #30518 | #30505, mithril boots |
| Marigot centaurs, room #30506 | #30506 / #30513 / #30519 | #30506, leather harness |

The common guards say they patrol the countryside and oppose “the forces of evil.” The captains are described as the most elite countryside guards, watching for trouble so they can “protect and serve the lesser people.” No named enemy, threat, order, battle, or completed objective is supplied. The prose says little about any of the seven hometowns beyond the origin named on each guard.

The equipment gives each captain a different cultural marker, but the source does not say these objects are quest items, gifts, uniforms, or rewards. Several descriptions contain spelling or wording errors, including “etchede,” “bracele,” and “embroided.” Keep the quoted source identities intact until an owner confirms both the intended equipment and copy edits.

## Story boundary

Patrols has no local quest contract, giver, or story sidecar despite its QST manifest entry. Its story is limited to the guard roster, the towns named in mobile text, the repeated defensive duty, and the seven lost map destinations. The area does not say whether the load rooms are intended to be physical locations, internal spawning points, or links to an older map.

The current audit also found seven different room definitions and four different mobile definitions in unregistered Fists of Fury Guildhall source files using some of the same VNUMs. That separate candidate is not established as a predecessor or replacement. Preserve the two identities and see the [typed dependency and retention review](../quest-story-followups/PATROLS_AUDIT_REVIEW.md#external-identity-collisions-and-typed-dependencies).
