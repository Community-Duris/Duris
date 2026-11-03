# Nhavan Island

Active source `nhavan_island_one`, zone 203, titled `Nhavan Island`. The island is a small Endu settlement built around an ancestral shipwreck and a village witch doctor. Its story is environmental: the Endu maintain their old ship because the witch doctor promised continued plenty, while a warning sign and hostile creatures frame the nearby woods as evil. No local quest file or story sidecar turns these hooks into recorded objectives.

Sources: [world rooms](../../../areas/wld/nhavan_island_one.wld), [mobiles](../../../areas/mob/nhavan_island_one.mob), [objects](../../../areas/obj/nhavan_island_one.obj), [resets](../../../areas/zon/nhavan_island_one.zon), [shop](../../../areas/shp/nhavan_island_one.shp), [full-world membership](../../../areas/AREA#L105), [QST-area membership](../../../areas/qst/AREA#L98), [graph and dependency supplement](../quest-story-followups/NHAVAN_ISLAND_ONE_AUDIT_REVIEW.md).

## Story in the source

The north and south strip of volcanic land leads around the base of a volcano that cannot be climbed because its surface is slick volcanic glass. Erosion has created fields where the villagers grow tubers and blue patu fruit. The village has a dock, livestock pen, cemetery, fishmonger, central idol, and a witch doctor's hut raised on stilts.

The Endu arrived on the island aboard an old ship from a faraway home. Although it is now a wreck, the ship represents the village's continued life and prosperity. The people keep it up because their witch doctor promised abundance for as long as they do. The idol resembles a squat Endu with flame-shaped hair and receives fish, incense, and flowers. At the cemetery, an anchor is inscribed as the resting place of Urtu, a brave captain who brought the people to a good life.

A gate leads from the idol into haunted woods. A sign warns, “BEWARE THE EVIL FOG.” Rooms describe thick, disorienting mist, shadows, shrieks, and silence. The witch doctor's hut contains incense, candles, a painted mat, and trinkets; the reset gives the witch doctor a wooden rattle and teakwood mask and places a scarlet gourd in the room. These details imply ritual and danger, but the source does not say what caused the fog, name the idol's god, explain the gourd's three painted signs, or define a player task that repairs the wreck or resolves the woods.

## Current interactions and services

`areas/qst/AREA` lists zone 203, but there is no `areas/qst/nhavan_island_one.qst` file and the source inventory finds no local QST giver or contract. There is also no `areas/story/nhavan_island_one.story.json`. Treat the ship promise, idol, anchor, gourd, and warning sign as environmental story until a builder assigns an explicit objective and outcome.

Pantu, mobile #20303, is reset at the fishmonger stand in room #20310. The local SHP source has one shop record for him, with prawn #20302, sea bass #20303, turtle legs #20304, and dragonfish fillet #20305 in its offered list. `areas/world.shp` is absent; the shop loader's fallback reads all files under `areas/shp/`, so this per-area record is included by that source path. The coverage register previously recorded zero shops and has been corrected to one, keyed by keeper #20303. This is a source review, not a gameplay run.

Resets place ordinary Endu villagers around the fields, dock, village, pen, and cemetery; the witch doctor in the hut; a fishmonger at the stand; a fiendish puck in the woods; three will-o'-the-wisps across the fog rooms; and vrang and oongawa livestock in the pen. Villagers receive shivs, Pantu a boning blade, and the witch doctor the mask and rattle; the scarlet gourd is also reset in the hut. The island's fish and meat stock is shop inventory, not evidence of a native quest.

## Static navigation

The WLD contains 23 rooms, #20300–#20322, and 53 authored exits. Fifty-two exits stay within the island and resolve. The remaining exit is east from #20301 to #148782. From the strip at #20300, authored resolved edges reach every island room and every room can return to #20300; the resolved local graph is one directed component. Three paired gates reset closed: the idol/woods fence, the village/pen gate, and the pen/cemetery gate. This count treats traversable exits as openable; it does not claim a live traversal or door-state check.

There is no active WLD entrance from another zone and no active WLD exit from the island to another loaded zone. The #20301 east exit is the only outward record. Its text says the Timeless Sea is there, but the target #148782 is titled `The Ocean` in `Duris3.wld`, whose stem is absent from `areas/AREA`. The full-world loader prunes this unresolved target. The local route returns west to #20300, but the sea destination is not an active route. No active WLD, typed QST/SHP/story/reset dependency, or numeric source-code destination into the island was found. Other non-VNUM travel behavior was not exercised. The only compiled cross-zone mention found is Menden's active fisherman #88805: its `menden_fisherman` callback has one case in an 80-value ambient-chatter switch about bringing turtle legs back from Nhavan Island. That line does not check the island's turtle-legs object, award or consume it, or encode a route; see [the callback binding](../../../src/specs/specs.assign.c#L1088) and [the line](../../../src/specs/specs.menden.c#L237).

## Builder decisions

The source supports a coherent island story, but leaves the travel link, the witch doctor's promise, the wreck's upkeep, and the evil fog unresolved. The [audit supplement](../quest-story-followups/NHAVAN_ISLAND_ONE_AUDIT_REVIEW.md#twenty-direct-builder-questions-and-recommended-resolutions) records twenty source-backed questions and conservative recommendations. Keep the active area in place. Preserve the current local graph and shop data until a builder chooses whether these are environmental lore alone or the basis of a separately specified quest or travel change.
