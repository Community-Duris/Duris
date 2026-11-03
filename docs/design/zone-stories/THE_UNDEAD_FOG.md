# The Undead Fog: comprehensive source story map

Source area `shortb`, zone 678, is a small fog maze wrapped around a separate Web of Unlife pocket. Its source files are [shortb.wld](../../../areas/wld/shortb.wld), [shortb.mob](../../../areas/mob/shortb.mob), [shortb.obj](../../../areas/obj/shortb.obj), [shortb.zon](../../../areas/zon/shortb.zon), and [shortb.qst](../../../areas/qst/shortb.qst). The dossier maps current room prose, reset-loaded characters and objects, the two active Surface exits, and the local portal pair. No live access, character encounter, portal use, safety, or quest completion is asserted.

## The grey fog and Web of Unlife

The area has thirteen rooms (#67800-#67812). Twelve carry almost identical “Lost in the Grey Fog” prose describing a dry, sight-reducing fog, black charred ground, shadows, and a strong sense of being lost. Room #67812 is titled “Lighter Fog”; its extra prose describes a faint purple portal hovering above the ground. The separate room #67809, “The Web of Unlife,” describes an unmaterial, seemingly endless place awakening to mortals seeking necromantic power.

The source WLD records 87 directions: 85 local directions and two reciprocal Surface connections. From #67801, west reaches Surface #569793; Surface returns east to #67801. From #67810, east reaches Surface #569799; Surface returns west to #67810. The twelve fog rooms form one connected local component, while Web room #67809 has no authored WLD exit. This raw map records confusing loops, not a runtime random-transport or safe-return rule.

Two local type-25 portal objects create a separate object-target route: object #67800 targets fog room #67812 and resets in Web room #67809; object #67801 targets Web room #67809 and resets in fog room #67812. These are source bindings for the portal pair, not authored WLD edges. Current reset state, use admission, and successful return remain unqualified.

## Srakart, the tome, and lost souls

Srakart the Malicious, mobile #67800, resets in Web room #67809. His room/mobile/object prose says that he was once Joahim Eisenberg, studied necromancy, raised and armed undead servants, discovered the long-forgotten *Necronomicon*, and left on an extraplanar search for more power. Black tome object #67802 has the short name “the tome of death ‘Necronomicon’” and is reset to Srakart. The same reset chain equips a flowing ebony cape, a red dried-blood wand, and a black cylinder containing scrolls. The source establishes the starting stock and prose, not a player-facing book-recovery transaction or a live drop.

The fog contains separate lost souls: warrior, cleric, and mage variants (#67801-#67803 and #67807-#67809). Their descriptions say they recently died and have not yet reached their gods. A lost vampire, #67804, is a former servant abandoned by its master; the lone local QST block attaches a `qc_action 45` moaning line to this mobile. That line is not a Q/QA achievement or documented accepted quest contract.

Two giant turtle prototypes, #67805 and #67806, differ in gendered prose and say they feed on undead in the fog. Drained, crippled soul mobiles #67810-#67813 ask who or what has drained the dead of their vital energy. The descriptions present possible setting and antagonist leads; they do not implement an explicit drain event, rescue, summon, allegiance, or campaign state.

## Stock and source boundary

The eight local object prototypes are two portals (#67800-#67801), the Necronomicon (#67802), a black scrollcase/cylinder (#67803), ebony cape (#67804), multicolored scroll (#67805), green flashing scroll (#67807), and red wand (#67808). Object VNUM #67806 is unused in this source; keep that gap until its owner determines whether it is reserved. The scrollcase contains a source `T 516 14 1 60` trigger record; preserve the tuple and do not infer its effect from the prose.

Reset mode is 2. The ZON has 26 commands: 17 mobile resets, two portal object placements, two equipment placements, four container placements, and one carried tome. There is no shop, native Q or QA contract, or story sidecar. The area is listed in `areas/qst/AREA`, and its local `shortb.qst` consists of the one vampire `M qc_action` text block. The three observed Surf directions into the fog are source-only and belong on the existing Surf candidate row, as recorded in the audit supplement.

The room census, both reciprocal Surface pairs, full reset placement, all fourteen mobile roles, all eight object definitions, and the raw room prose are documented in the active source files linked above. No source-only title match or same-type prototype collision is used to infer a replacement zone.
