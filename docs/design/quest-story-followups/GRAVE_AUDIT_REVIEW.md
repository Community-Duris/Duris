# Grave audit review

This supplement records the source-only review of active contributor `grave`. It maps the cemetery narrative, reset placements, authored WLD graph, typed source dependencies, inactive-WLD approaches, and followed file history. It does not qualify installed state or live gameplay.

## Source inventory

The active area roster lists Grave at *84–86 under the title “The Old Graveyard.” Its WLD, MOB, OBJ, and ZON files contain 204 rooms (#8400–#8603), 28 mobiles (#8400–#8427), and 15 objects (#8400–#8414). The ZON uses reset mode 2 and has 189 non-S commands: 154 M, 18 D, nine E, three O, three G, and two P.

The QST roster also lists Grave, but `areas/qst/grave.qst` is absent. There is no local SHP record, story sidecar, or native Q/QA contract. The current coverage entry is [row 113](ZONE_COVERAGE_REGISTER.csv#L113).

## Narrative and reset placements

The cemetery’s room text describes fog, dark paths, graves opened from within, a pried-open crypt, a beating coffin, and an open casket with an inscription asking for rest. Its central family story is the Fryntoks: the children and Emily are reanimated, while Raydyrn returns as a vampire after the family’s murder by ogres and his own death in Faang. The petrified oak, magical garden, growing auras in the largest tomb, and necromancer’s laboratory reinforce the cemetery’s supernatural setting. See the [source dossier](../zone-stories/THE_OLD_GRAVEYARD.md).

The resets place an old desk in the caretaker’s shed and iron key #8401 inside it. Marble key #8405 is reset in room #8462; coffin #8402 is reset at #8518 with medallion #8403 inside. The family’s undead return in rooms #8525–#8529, with Raydyrn #8412 at #8529 wielding Ogrebane #8406. The ancient necromancer #8427, wearing cloak #8414, is placed in laboratory #8601 with a wight and wraith ([key, desk, and coffin resets](../../../areas/zon/grave.zon#L81), [Fryntok and Ogrebane resets](../../../areas/zon/grave.zon#L177), [laboratory resets](../../../areas/zon/grave.zon#L246)).

The source binds object #8406 to the `ogrebane` special procedure. It checks for a wielded hit against an ogre and has a one-in-fifteen chance to cast Bigby’s Clenched Fist ([binding](../../../src/specs/specs.assign.c#L1288), [procedure](../../../src/specs/specs.jot.c#L382)). This connects Raydyrn’s written revenge to a functional item record. The absent local QST file leaves no authored player objective or outcome.

## Authored graph, boundaries, and door resets

All 480 exits in active `grave.wld` resolve: 478 stay inside Grave and two leave it. Exit #8400 south reaches Surface #593389; exit #8427 east reaches Surface #592593. Each has a reciprocal Surface exit back to Grave. These are the two active inbound and two outbound cross-area records ([Grave exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L178), [Surface reverse exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1558), [Grave→Surface pairs](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L164), [Surface→Grave pairs](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L509)).

The local directed WLD graph is one weak component containing all 204 rooms. With authored exits available, reach and return from both arrival rooms, #8400 and #8427, cover 204/204. The ZON contains 18 D resets: six at state 1, eleven at state 2, and one at state 5. WLD key fields identify iron key #8401 on the paired doors #8454/#8542 and #8457/#8551; marble key #8405 on #8464/#8500 and #8490/#8491; and wooden key #8413 on #8571/#8601. The concealed #8487/#8490 passage has no key. Treating all reset-door exits and their opposite sides as unavailable in a static graph leaves 3/204 reachable and returnable from #8400 and 149/204 from #8427. This is a source-state model, not a claim about loaded door state. No falls or currents are authored.

A compiled route also references this area: the registered `shift_prime` innate has a nine-room destination list for Phantoms on the Astral Plane, including Grave rooms #8486 and #8430 ([registration and destination list](../../../src/classes/innates.c#L159), [route logic](../../../src/classes/innates.c#L2005)). This is a conditional code route and does not appear as a WLD boundary record.

Other numeric room references occur in special-procedure definitions: `shadow_demon` targets #8450, `tiaka_ghoul` targets #8556, `mystra_dragon` targets #8512, `hunt_cat` targets #8426, and `mailed_fist_guardian` checks #8574. Repository symbol search finds only declarations and definitions, with no assignment in `specs.assign.c`; these procedures are not established active routes. The associated `Mask` values are guild-insignia object VNUM checks, not Grave room references. The `akh_elamshin` definition sends text to room #8531 but does not move a character, and no assignment to it was found.

## Source-only inactive-WLD approaches

The all-95 inactive-WLD sweep found no room-number overlap with Grave and four source-only approaches. They are not active ingress:

| Source-only exit | Target | Source anchors |
| --- | --- | --- |
| `surf.wld` #275541 west | Grave #8427 | [room](../../../areas/wld/surf.wld#L1585169), [exit](../../../areas/wld/surf.wld#L1585185) |
| `surf.wld` #276137 east | Grave #8400 | [room](../../../areas/wld/surf.wld#L1597657), [exit](../../../areas/wld/surf.wld#L1597665) |
| `underdark2011.wld` #250432 south | Grave #8400 | [room](../../../areas/wld/underdark2011.wld#L188560), [exit](../../../areas/wld/underdark2011.wld#L188572) |
| `underdark2011.wld` #250731 east | Grave #8400 | [room](../../../areas/wld/underdark2011.wld#L189925), [exit](../../../areas/wld/underdark2011.wld#L189933) |

Preserve the two Surf approaches with existing candidate row 97 and the two UnderDark 2011 approaches with candidate row 115 ([Surf](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [UnderDark 2011](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115)).

## Typed references and source closure

No external typed ZON reset-role reference to a Grave mobile, object, or room was found. No external QST item command, SHP product, story-sidecar `item_vnums` reference, or duplicate MOB/OBJ prototype uses a local Grave object or mobile. All 872 external type-25 object declarations scanned have no destination in rooms #8400–#8603.

The Grave reset commands use local Grave prototypes. The compiled `shift_prime` route and the locally bound Ogrebane procedure are recorded above. Guild-insignia IDs, zone-owner range bounds, refusal-code arguments, and an opaque checksum fragment also matched the numeric range; their types and call sites do not make them Grave room dependencies.

## Title, history, and disposition

After color-code removal and normalization, “The Old Graveyard” is unique among 442 current ZON titles. Followed history remains on `grave`: WLD history reaches 2013-11-13, MOB and ZON history reach 2015-10-13, and OBJ history reaches 2016-03-05. The 2009 source-tree move appears in each followed history. No alternate-stem predecessor, retirement, or explicit replacement is established by the checked-in titles and histories.

Keep active `grave` **DO NOT MOVE**. Preserve its two reciprocal Surface connections, keyed crypts, conditional Phantom code route, and the four inactive-WLD approaches. Add the approaches to the existing Surf and `underdark2011` candidate records; do not add a Grave candidate row or treat these records as migration clearance. The reviewed Surf subset is now 225 and curated source-only total is 3,467; `ud_map2` remains 18 approaches and Underworld2 remains eight.

This review covers checked-in source only. It does not confirm current NPC or item placement, loaded door state, whether the code paths are reachable in a running game, or safe traversal. No test, build, server, gameplay session, database operation, migration, or operational script was run for this documentation audit.
