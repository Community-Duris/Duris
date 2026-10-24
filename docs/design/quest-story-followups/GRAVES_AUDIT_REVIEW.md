# Graves audit review

This supplement records the source-only audit of active contributor graves. It covers template interpretation, static and generator-defined connectivity, external typed dependencies, title and file history, and attic disposition. It does not qualify installed state or live gameplay.

## Source inventory

The full area roster lists graves at zone 195 as a “random template zone.” The QST roster also lists the key, but areas/qst/graves.qst is absent. The WLD, MOB, OBJ, and ZON files contain 25 room prototypes (#19500–#19524), 23 mobiles (#19500–#19522), and 16 objects (#19500–#19515). The ZON title is “The Crypts of Rays,” its reset mode is 0, and it has no non-S reset command. There is no local SHP file, story sidecar, or native Q/QA contract. See [coverage row 114](ZONE_COVERAGE_REGISTER.csv#L114) and the [source dossier](../zone-stories/THE_CRYPTS_OF_RAYS.md).

The local room text groups into mass graves, mirror and time imagery, old stone catacombs, and two dwarven tunnel descriptions. The mobile set is entirely unplaced by the local ZON. The object set includes seven potion descriptions, procedural entrance markers, a chest/key/warning sign, and decorative remains. No local QST objective, reward, or story outcome is authored.

## Static and generated connectivity

All 25 static WLD records have zero exits. The authored graph therefore has 25 weak singleton components with a largest component of one room; there are no active WLD inbounds or outbounds, negative targets, unresolved targets, falls, or currents. A scan of all 95 inactive WLD sources found no room-number overlap and no source-only exit targeting #19500–#19524. These static counts describe template records.

The in-tree generator maps three themes to the Graves room prototypes:

| Theme | Room source VNUMs | Source comment |
| ---: | --- | --- |
| 0 | #19500–#19505 | grave |
| 1 | #19506–#19515 | glass |
| 2 | #19516–#19522 | glass, though the prose is catacombs |
| 3 | #19523–#19524 | tunnel |

The generated route is built by create_zone. It clones templates into dynamic VNUMs beginning at 100000, creates randomized internal exits, and connects the first generated room to a chosen map room through a secret, closed down trapdoor. For theme 0, 1, or 2 it places object #19507, #19511, or #19515 as the map-room marker. Those objects are bound to random_tomb, random_glass, and random_slab; their successful digging interaction invokes room search, which can reveal the generated secret exit ([theme/source map](../../../src/world/random.zone.c#L82), [room creation](../../../src/world/random.zone.c#L135), [marker placement](../../../src/world/random.zone.c#L246), [procedure assignment](../../../src/specs/specs.assign.c#L1772), [procedures](../../../src/specs/specs.graves.c#L16), [secret-exit search](../../../src/cmd/actobj.c#L9893)).

The checked-in boot configuration defines RANDOM_ZONES as 1, but create_randoms wraps its automatic generation body in #ifndef RANDOM_ZONES. The normal boot call therefore reaches an empty function body under the current define. The custom areas/RANDOM_AREA file is empty. A trusted randobj zone command still directly calls create_zone for a random theme from 0 through 2, so manual creation is source-supported; no invocation or generated live instance was inspected. Theme 3's tunnel templates have no in-tree caller. Source selection also calls number(0, 10) for a ten-column array; number's upper bound is inclusive, so the call permits an out-of-range column 10. This is a generator reliability question for the code owner, not a change made by this audit ([macro](../../../src/core/defines.h#L1396), [boot generation block](../../../src/item/randomeq.c#L494), [manual call](../../../src/item/randobj.c#L235), [array access](../../../src/world/random.zone.c#L204), [number semantics](../../../src/core/random.c#L127)).

The shared random-zone code can put a random-themed loot object in an optional chest and places a generated sigil in the final room. A generic random-quest mobile asks the player to recover a friend's sigil. The sigil uses a randomly selected zone name and is stored with the generated zone number; returning the matching sigil consumes it, awards random-zone epic points, and yields stones, materials, and random equipment. These are shared random-zone behaviors rather than a Graves QST. The generator uses generic randomized mobiles instead of Graves mobile VNUMs ([chest and theme loot](../../../src/world/random.zone.c#L395), [sigil placement](../../../src/world/random.zone.c#L579), [sigil creation](../../../src/world/random.zone.c#L1327), [quest handler and reward](../../../src/world/random.zone.c#L1146), [quest mobile assignment](../../../src/specs/specs.assign.c#L1026)).

The generated graph has not been instantiated for this review. Reachability, return, door state, repeated entry, and safety in a live instance remain unqualified.

## Inbound and outbound dependency closure

No active or inactive WLD exit targets a Graves room, and there is no Graves WLD boundary. No external ZON reset-role reference to a Graves room or mobile was found. Fifteen active external ZON G resets do import Graves potion objects #19500–#19505:

| Importing zone and recipient | Reset references | Graves object prototypes |
| --- | --- | --- |
| Alatorin: three young channelers, K'tharz, Babdrabble, and three shield dwarf summoners | 10 G commands | #19501 twice; #19502 three times; #19503 once; #19504 once; #19505 three times ([resets](../../../areas/zon/alatorin.zon#L2092), [later resets](../../../areas/zon/alatorin.zon#L2265), [Babdrabble](../../../areas/zon/alatorin.zon#L2571), [summoners](../../../areas/zon/alatorin.zon#L2709)) |
| Verspin: four mages in training and head sorceress Vespus | 5 G commands | #19500, #19501, #19502, #19503, and #19504 once each ([resets](../../../areas/zon/verspin.zon#L452)) |

The source code also names the Graves prototypes for template copying, theme loot, chest/key/sign creation, and entrance-procedure assignment. No external QST item command, SHP product, story-sidecar item_vnums reference, duplicate MOB/OBJ prototype, or external ITEM_TELEPORT target to #19500–#19524 was found. The scan covered 872 external type-25 declarations. A loose numeric match of 19500 in the Oasis eyepiece OBJ data is its listed cost field, not a VNUM target.

## Title and followed history

After color-code removal and normalization, “The Crypts of Rays” is unique among 442 current ZON titles. Followed same-stem history remains on graves: WLD through 2013-11-13; MOB and ZON through 2015-10-13; OBJ through 2013-11-13, with 2012 and 2009 source-tree history also present in the tracked files. The 2009 source-tree move appears throughout the file history. No alternate-stem predecessor, retirement, or explicit replacement is established. The absent QST, SHP, and story-sidecar files have no followed local file history.

## Builder questions and recommended resolutions

The [source dossier's decision table](../zone-stories/THE_CRYPTS_OF_RAYS.md#builder-questions-and-recommended-resolutions) records owner questions about the template's intended use, generator index and boot guard, the Rays, mirror imagery, dormant tunnel theme, shared sigil, and imported potions. The recommended disposition preserves the environmental text and all currently consumed prototypes while keeping unresolved quest and route decisions visible.

## Attic disposition

Keep active graves **DO NOT MOVE**. It is a full area contributor, not an attic candidate. Moving the source out of the active roster would remove room templates and objects addressed by hard-coded random-zone code, the three assigned entrance procedures, and 15 active Alatorin/Verspin resets. Preserve the template files and their identifiers in place. No Graves candidate row, source-only WLD approach, or migration clearance is added; existing candidate histories and their counts stay unchanged.

This review covers checked-in source only. It does not establish whether staff have invoked randobj zone, whether a generated instance exists, or whether it is currently reachable. No build, test, server, gameplay session, database operation, migration, or operational script was run for this documentation audit.
