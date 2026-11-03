# The Reliquary Nexus of Aggression: comprehensive zone-story mapping

This dossier maps the story expressed by active source stem nexus_ic. It is source review, not proof of installed gameplay. The [repository-wide Nexus audit supplement](../quest-story-followups/NEXUS_IC_AUDIT_REVIEW.md) records boundaries, inactive-map references, typed dependency closure, title/history, and builder questions.

## Source inventory and present delivery

The active area owns 100 WLD rooms, #130100–#130199, zone 1301; 21 mobile prototypes, #130101–#130121; 11 object prototypes, #130104–#130120; and one ZON reset source. It has no local QST, SHP, or story-sidecar file. The shared [QST area index](../../../areas/qst/AREA#L327) nevertheless lists nexus_ic, so that membership needs owner confirmation. The source contains no native contract, shop, or sidecar-defined outcome.

The ZON header calls the area “The Reliquary Nexus of Aggression.” Its reset body has 64 door commands, 111 mobile placements, eight object placements, and one equipment placement. These counts describe source declarations, not observed reset state or available inventory.

## Entry and local map

The WLD file declares 258 exits: 254 within the area and four to Surface. All 258 targets resolve. The local undirected view has two components, of 76 and 24 rooms. Four reciprocal Surface boundaries meet the 76-room component:

| Nexus room and exit | Reciprocal Surface exit | Source evidence |
| --- | --- | --- |
| #130100 west to #511534 | #511534 east to #130100 | [Nexus exit](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1290), [return](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1320) |
| #130112 north to #511135 | #511135 south to #130112 | [Nexus exit](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1291), [return](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1318) |
| #130124 east to #511536 | #511536 west to #130124 | [Nexus exit](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1292), [return](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1321) |
| #130136 south to #511935 | #511935 north to #130136 | [Nexus exit](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1293), [return](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1323) |

The first room presents a narrow path between ice cliffs, thaw-and-refreeze cracks, and stains described first as rust and then as dried blood. The first 48 rooms develop that icy maze through narrow paths, ice-block passages, rune marks, mirrored crystals, snowy alcoves, and doors whose passages may be obscured or reset. Several descriptions imply the ice or rune is a boundary, but the actual exit and reset declarations determine the authored route.

Rooms #130148–#130179 shift into ice-block halls, icicle tunnels, four Mushroom Forest rooms, and four enclosed Barren Trap rooms. The forests describe warm moss, windblown blue sparks, red-capped mushrooms, and small people moving through doors and windows in the stems. The traps contrast unreachable sky and sheer cliffs with blood, bones, and creatures fighting over remains. The area has no WLD fall or current markers.

Room #130180 explains the unusual transition to a ship: its rope ladder rises into the hollow hull above. Rooms #130181–#130189 describe the lower deck as a fighting tavern where honored dead warriors battle daily and expect to return to life; #130190–#130198 describe the upper deck, Norse visitors, valkyries, banners, mead, and open ocean. The final room, #130199, is “The Nexus”: its center radiates horror and ecstasy, making it hard to distinguish friend from foe. These are explicit environmental stories, not proof of a combat effect, resurrection service, or controller.

## Residents, objects, and reset behavior

The 21 mobile definitions include ice and snow trolls, ice lizards, frost giants, kabouters, pterodactyls, Nidhoggr, a snow ogre, Skadi, gletzglo, an einherjer warrior, and a valkyrie. Multiple descriptions say a creature was taken from its home, trapped, or made to fight; some seek revenge or freedom. Skadi is described as a stationary ocean watcher, while Nidhoggr is characterized as a corpse seeker. Treat those words as source characterization until a builder identifies each intended contact, agency, threat, and lasting outcome.

Eight local objects are fixed type-25 mushrooms whose command value is “stomp.” They point to local forest/trap rooms, outside the WLD exit graph:

| Object | Declared destination |
| --- | --- |
| #130104 | #130155 |
| #130105 | #130163 |
| #130106 | #130171 |
| #130107 | #130179 |
| #130117 | #130154 |
| #130118 | #130170 |
| #130119 | #130178 |
| #130120 | #130162 |

Some extra descriptions call destinations “room 72” or “room 80,” which does not identify the written VNUM. Resolve that wording and the intended landing/return behavior with the builder before treating these objects as safe travel or a story objective.

The other three objects are a frozen troll heart (#130113), a lizard claw (#130114), and an ornate drinking horn (#130116). The first two descriptions connect them to violent encounters. The horn has an unnatural pearlescent glow and unreadable runes; one ZON equipment reset places it on a mobile. Their descriptions alone do not establish a quest, owner, recipient, or reward.

## Story boundaries and future design

The source supports several connected story threads: an icy maze with a strange rune and blood-marked passages; captive beings forced into violence; mushroom portals to isolated forest/trap scenes; an afterlife ship for honored Norse dead; and a central Nexus whose influence is frightening and ecstatic. The 24-room WLD component and fixed mushroom destinations may make those scenes deliberate separate chapters, but static source does not establish the intended order or a playable return path.

No external QST item command, SHP product, story-sidecar VNUM, ZON reset target, duplicate mobile/object prototype, external type-25 target, or compiled numeric prototype reference into #130100–#130199 was found in the bounded scan. The area remains an active full contributor and is **DO NOT MOVE**. Keep the four reciprocal Surface crossings and existing item destinations while the owner decides whether the QST/AREA entry is intentional and which of the environmental stories should receive authored, testable outcomes. No contract, reward, route, or migration change is claimed here.
