# The Crypts of Rays

This source dossier covers active contributor graves: 25 room prototypes (#19500–#19524), 23 mobile prototypes (#19500–#19522), and 16 object prototypes (#19500–#19515). The inventory is in [coverage row 114](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L114), and the static exits, generated-instance code, dependencies, and history are detailed in the [Graves audit supplement](../quest-story-followups/GRAVES_AUDIT_REVIEW.md).

## A loaded template area

Both area rosters list graves under the note “random template zone” ([full area list](../../../areas/AREA#L98), [QST area list](../../../areas/qst/AREA#L91)). The ZON header names it “The Crypts of Rays,” sets reset mode 0, and contains no reset commands ([zone source](../../../areas/zon/graves.zon#L1)). A matching graves.qst file, SHP source, and story sidecar are absent. The QST roster entry therefore does not supply a local quest.

None of the 25 static room records has a D exit. Their single-room components are template records, rather than proof that a normal player can enter 25 disconnected live rooms. The random-zone generator can copy selected room descriptions into generated rooms and create exits for those copies; its current code path and limits are reviewed below.

## Four environmental themes in the room text

### Mass graves, #19500–#19505

Six rooms depict bodies buried with broken weapons and shields, rancid flesh, maggots, bones, claw and hand marks, and runes. Several descriptions suggest people were buried alive or tried to climb out. Another frames the dead as an army killed by dark minions. No named battle, army, or event is identified in the local text ([room templates](../../../areas/wld/graves.wld#L1)).

### Mirrors, glass, and time, #19506–#19515

The first mirror room feels watched. The following room shows a reflection shaped by the visitor's deeds, and some visitors end their lives after seeing it. Another mirror shows carried objects but no person or worn equipment. A narrow glass shaft is cramped to crawl through; the next room is caved in around an orc's feet. In a broken mirror room, dried blood and body parts lie among claw-marked walls. The hallway of frozen images shows warriors, knights, and mages trapped in glass with expressions of terror.

The later mirrors give the sequence a time motif: one small mirror lies in an eroded chamber, aged mirrors show a much older reflection, and three mirrors are labeled Past, Present, and Future. Their images change among war, oceans, and nature. A corpse that appears to have died of old age lies before the three mirrors. The records provide no cause, actor, or resolution for these effects ([mirror templates](../../../areas/wld/graves.wld#L61), [later mirror templates](../../../areas/wld/graves.wld#L132)).

### Stone catacombs, #19516–#19522

Seven rooms move from dry, dust-covered caskets and half-burnt torches into a scarab nest and a defiled casket chamber. A large domed room holds worn statues of past royalty, demons, and goddesses. An aged hallway shows a crude hieroglyph of a large eye beside a brittle corpse; a dried fountain has become another scarab nest. The source does not name the kingdom, royal line, warning, or cult represented by the images ([catacomb templates](../../../areas/wld/graves.wld#L162)).

### Dwarven tunnels, #19523–#19524

The final two descriptions mention a polished shaft with steps leading upward to the surface and tunnels attributed to dwarven masonry. They contain no actual WLD exits and do not identify a dwarven settlement or destination ([tunnel templates](../../../areas/wld/graves.wld#L260)).

## Mobile and object prototypes

The 23 mobile descriptions range from maggots and two sizes of centipede through fiends, skeletal entities, zombies, vampire minions, shadow creatures, souls, and a zombie cultist. Two priest descriptions refer to the ancient Rays; one carries a disturbing body-marking image using that name. The final prototype, #19522 Zeretornaz, is described as a huge winged minion of infernal darkness ([mobile prototypes](../../../areas/mob/graves.mob#L1), [Rays references](../../../areas/mob/graves.mob#L222), [Zeretornaz](../../../areas/mob/graves.mob#L326)). The local ZON does not place any of these 23 mobiles, and the random-zone code creates generic random mobs instead of reading these mobile VNUMs.

Objects #19500–#19506 are seven potion descriptions: dusty, damp, stone, strange, glowing, weird, and pulsating. The generator's theme loot table uses only #19500–#19505; #19506 is outside that table. Active reset commands in Alatorin and Verspin also import #19500–#19505, as detailed in the audit supplement.

The remaining prototypes provide a mysterious tombstone with a digging clue, silver treasure chest, bone key, warning sign, glass shard, skeletal corpse, unused fountain, scarab nest, and heavy stone slab with an entrance/exit clue. The generator reads the tombstone, glass shard, or slab as theme entrance markers, and reads the chest, key, and warning sign for generated instances. Their special procedures are bound in source. The extra descriptions are clues, not local quest outcomes ([objects](../../../areas/obj/graves.obj#L1), [entrance tombstone](../../../areas/obj/graves.obj#L85), [chest and key](../../../areas/obj/graves.obj#L101), [slab](../../../areas/obj/graves.obj#L172)).

## Shared random-zone story and access code

The source assigns room templates #19500–#19505 to theme 0, mirror rooms #19506–#19515 to theme 1, and catacomb rooms #19516–#19522 to theme 2. The row for theme 2 is mislabeled “glass theme” in its C comment. Theme 3 contains only tunnel rooms #19523–#19524. The explicit staff command calls create_zone with a theme from 0 through 2, so that route cannot select theme 3. The boot generator would also choose only themes 0 through 2, and its custom RANDOM_AREA input is empty ([template table](../../../src/world/random.zone.c#L82), [staff command](../../../src/item/randobj.c#L235), [empty random-area input](../../../areas/RANDOM_AREA)).

When create_zone runs, it clones selected WLD templates into dynamic room numbers beginning at 100000, connects them with generated exits, and joins the first generated room to a chosen map room through a secret, closed down trapdoor. It places the theme's entrance marker in that map room. The bound object procedure accepts digging/search interaction; on a successful dig it searches for a secret exit. This describes a potential generated route. Source inspection does not establish that an instance was created or is currently reachable.

A shared sigil story may also be added to a generated zone. The generator places a sigil in the final generated room, while a generic random-quest mobile asks for help recovering a friend's sigil. The sigil's displayed name is drawn from a randomly selected zone. When the matching sigil is returned, the shared procedure consumes it, awards random-zone epic points, and gives stones, materials, and random equipment. This code path is separate from a local Graves quest and does not explain the title's “Rays” ([sigil creation](../../../src/world/random.zone.c#L579), [sigil builder](../../../src/world/random.zone.c#L1327), [generic quest dialogue and outcome](../../../src/world/random.zone.c#L1146)).

The current compile-time setting defines RANDOM_ZONES as 1, while the body of create_randoms is guarded by #ifndef RANDOM_ZONES. Consequently the automatic boot-generation body is excluded by the checked-in definition. The trusted randobj zone command remains an in-tree caller. Runtime invocation has not been verified. There is also a source-level selection defect to resolve before relying on this path: the template array has ten columns, but selection calls number(0, 10), and number uses an inclusive upper bound. That permits index 10, outside the array's valid 0–9 range ([setting](../../../src/core/defines.h#L1396), [compile guard](../../../src/item/randomeq.c#L494), [selection](../../../src/world/random.zone.c#L204), [inclusive number function](../../../src/core/random.c#L127)).

## Builder questions and recommended resolutions

| Question for the area owner | Source evidence | Recommended resolution |
| --- | --- | --- |
| Are these room records intended as random-zone templates, or as a directly visited static zone? | Both area lists call graves a random template zone; ZON mode 0 has no commands; static WLD rooms have no exits; generator code clones selected rooms into dynamic IDs. | Preserve them as template source unless the owner identifies a separate active arrival path. Keep the dynamic route conditional on a confirmed generator invocation. |
| Should the generator selection upper bound be 9, and should the boot guard be corrected? | The array dimension is ten, the selection requests inclusive 0–10, and the current RANDOM_ZONES definition excludes the automatic-generation body. | Have the code owner align the index range and boot guard with intended behavior before depending on generated content; retain this as an implementation question until that work is reviewed. |
| What are the “Rays” named by the title and priest descriptions? | “Rays” appears in the zone title and two priest descriptions, but the local room and mobile text supplies no identity or history. | Preserve the name as an unresolved lore clue. Add an explanation only if the area owner confirms the intended group, god, or event. |
| Do the mirrors tell one intended story, or provide separate horror scenes? | The records move from moral reflection and trapped images to age and Past/Present/Future, with no source-authored actor or outcome. | Keep the existing thematic sequence and environmental ambiguity. Add a task or ending only when its author, trigger, and outcome are specified. |
| Should the dwarven tunnel templates connect to another area? | Rooms #19523–#19524 describe an upward route but have no exits; theme 3 has no in-tree caller. | Treat the text as unconnected template description for now. If this theme is intended for play, first identify the destination, return path, and actual creation hook. |
| Are the shared sigil and imported potions part of a Graves-specific quest? | The sigil flow is implemented by shared random-zone code; Alatorin and Verspin reset the potion prototypes; no local graves.qst exists. | Keep those dependencies attributed to their shared generator or importing areas. Preserve the prototypes and do not invent a Graves-specific quest from the item placements. |

The recommended source disposition is to keep active graves in place, preserve all template prototypes and imported potion VNUMs, and resolve the random-zone code questions before treating generated Graves content as ordinarily available.

## Source boundary

The room and mobile prose creates a horror setting with mass burial, reflective glass, ancient catacombs, scarabs, and dwarven tunnels. The source does not explain the Rays, name an event behind the graves, place local mobiles through resets, or record a local player objective and outcome. The generated sigil flow belongs to the shared random-zone system. This dossier distinguishes those clues and source behaviors without adding a native quest or changing gameplay.
