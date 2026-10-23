# The Shaughin Hunting Grounds

Checkpoint 359 adds the lower Shaughin source dossier. Read it with the distinct [upper Shaughin Settlement dossier](SHAUGHIN_SETTLEMENT.md) and the [Shaughin2 audit supplement](../quest-story-followups/SHAUGHIN2_AUDIT_REVIEW.md). The upper settlement has the local QST and story sidecar; Shaughin2 supplies three of its exact lower-hunt trophies, reset providers, two portal prototypes, and two camp workers.

## Authored story

The lower map presents a winding cavern system beneath the smoother tunnels. Its prose describes dry, sticky mud, glowing moss and mushrooms, massive claw marks, growling and scraping sounds, small tunnel creatures, and large animals that have made the maze their hunting ground. The narration says the original maze makers have gone farther into the Underdark. It introduces large bears, panthers, constrictors, deep crawlers, trolls, Shaughin hunters, a deep dwarf, and a cavern cube, but does not identify one authoring event or assign the destroyed mushrooms a cause.

Several narrow paths are lit by glowing mushrooms, while four rooms are explicitly named as shattered-mushroom paths. A campfire at #6686 makes one part of the maze seem less daunting. A distant camp passage, a dark lair, animal lairs, and a waterfall approach mark the maze's edges. Descriptions of an old camp and of Shaughin-made passages establish signs of use; they do not define a local quest, map, safe camp service, or a required progression order.

This is the authored environmental story, not proof of live creature availability, item acquisition, route safety, or a canonical history beyond the text in the source.

## Source inventory and authored reset roles

Active files are [shaughin2.wld](../../../areas/wld/shaughin2.wld), [shaughin2.zon](../../../areas/zon/shaughin2.zon), [shaughin2.mob](../../../areas/mob/shaughin2.mob), and [shaughin2.obj](../../../areas/obj/shaughin2.obj). The zone header is 66, its reset mode is 1, and its 100-room range is #6600-#6699. The sources declare 12 mobile prototypes and 23 objects. There is no local QST, shop, or story sidecar. The name does appear in areas/qst/AREA, so index membership must not be mistaken for a local QST source.

The 70 M, 44 G, 24 O, and two E rows, together with two D reset rows, give 142 command records. Reset stock is conditional on the zone's mode and the live reset system. The map's text and reset declarations do not establish that every creature or item is currently available.

| Authored role | Source declarations |
| --- | --- |
| Large-animal hunting | Giant cavern bears #6600 are paired with tooth #6613 in four lairs (#6603, #6611, #6684, #6692). Cave panthers #6602 are paired with paw #6614, and ancient constrictors #6603 with heart #6615, in the lower hunting rooms #6609, #6621, #6674, and #6677. Grizzlies #6601 share some of those rooms without the special trophy G rows. |
| Other maze inhabitants | Deep crawlers #6604, black trolls #6605, dark hunters #6606, elite hunters #6607, a cavern cube #6608, and an angry deep dwarf #6609 are reset in the shattered-path maze. Their selected G or E rows include a crawler leg, troll blood, cube-coated wand, map, and hunting equipment. |
| Workers and camp | Two Shaughin camp-worker mobile prototypes (#6610 and #6611) are declared; workers also occur in the upper zone's camp reset references. The lower workers' item and reset context do not establish a quest or a service. |
| Scenery | Reset objects include the wooden trapdoor, adventurer skeleton, glowing and shattered mushrooms, carcass, bones, footprints, charred skeleton, skull, campfire remains, and blood spatters. These are separate from the three required trophy prototypes. |
| Equipment and portals | The lower object file also defines hunting sandals #6616, gloves #6617, crawler leg #6618, troll blood #6619, bone wand #6620, map #6621, and skinning knife #6622. Type-25 wooden trapdoor #6600 targets upper room #6504; opening #6601 targets upper room #6535. Both are source declarations, not a promise that travel is safe or available in a live state. |

The trophy prototypes #6613-#6615 are distinct from the upper Shaughin heart #6509, common animal parts, corpse descriptions, or another trophy of similar appearance. The upper [Shaughin QST](../../../areas/qst/shaughin.qst#L40) requests two exact roots per exchange: upper heart #6509 for potion #6510, lower heart #6615 for #6511, bear tooth #6613 for #6512, and panther paw #6614 for #6513. Its [story sidecar](../../../areas/story/shaughin.story.json) models these as four independent quantity-two returns and names lower-provider mobile VNUMs #6600, #6602, and #6603 in provider hints. It does not add a local lower-zone quest, a required order, a personal-kill test, or proof that a trophy has been obtained.

## Typed cross-area uses

Exact active-source scans find these external consumers of Shaughin2-owned prototypes:

- Upper [Shaughin ZON](../../../areas/zon/shaughin.zon#L67) imports roaring campfire object #6604 into room #6543 and opening object #6601 into room #6548. It imports Shaughin camp-worker mobile #6610 twice at #6543 and mobile #6611 at #6546 ([mobile resets](../../../areas/zon/shaughin.zon#L185)). These are separate typed objects and mobiles, not local upper prototypes.
- [Alatorin ZON](../../../areas/zon/alatorin.zon#L1691) resets object #6603 at rooms #83617 and #83944. That Shaughin2-owned object is the remnants of an old campfire; preserve this active importer if source ownership or placement is later reviewed.
- The [Tharnadia1 shop-source file](../../../areas/shp/tharnadia1.shp#L510) lists object #6609 under shop #6111. Shaughin2 defines #6609 as a pitted, charred skeleton scenery object. The declaration is a typed shop-source reference, not proof of a reachable live shop, sale, or useful stock.
- Upper Shaughin's own wooden trapdoor #6500 is reset at #6504 and targets lower room #6600; lower trapdoor #6600 targets back to upper #6504. These are two distinct portal objects with reciprocal room destinations. Upper maelstrom #6501 and imported opening #6601 are separate same-zone portal declarations between #6535 and #6548.

No same-type active WLD, MOB, or OBJ header overlap was found for these local prototypes. External reset/shop references still have to be preserved separately from that uniqueness finding.

## Local graph, entrances, and hazards

The WLD contains 100 rooms and 212 authored exits. All 208 local destinations resolve; there are no local negative or positive unresolved targets, out-of-range rooms, or range-mismatch exits. The authored directed local graph is strongly connected: each of the four boundary rooms can reach and return from all 100 rooms in the raw WLD. This is a static source graph, not a reset-aware, live movement, or safe-travel result.

Four reciprocal active WLD pairs connect the map:

| Pair | Boundary |
| --- | --- |
| Shaughin #6543 south ↔ Shaughin2 #6699 north | Secret-passage prose connects the upper processing camp and lower passage; the pair has no lower-zone D reset. |
| Shaughin #6546 north ↔ Shaughin2 #6686 south | The old campfire is reciprocal with the upper passage. Secret keywords and authored flags need builder/runtime review; there is no D reset on the lower side. |
| Shaughin #6547 down ↔ Shaughin2 #6695 up | Waterfall crossing. The lower room has one C current marker, strength 100, direction north, and is a water/no-swim sector in the source. The upper side is also a water and current crossing. |
| Underdark #790149 north ↔ Shaughin2 #6696 south | A direct active boundary to the Underdark; this is separate from any inactive-map approach. |

In Shaughin2, the only D rows set #6696 west and #6697 east to state 5 (closed/secret, key 0) at 100% chance. They pair the Underdark-side cavern and the hand-carved hunting passage. The authored local WLD exit flags are otherwise zero. Do not infer from the raw maze graph that the secret passage is open after reset, or that discovery, opening, and crossing are the same event.

The source has no fall marker. Its one current is at #6695. The current, waterfall, teleport objects, destination admission, and ability to return are separate route questions. Room descriptions alone cannot establish survival or availability.

## Title, tracked history, and disposition

After stripping color codes and the terminator, trimming whitespace, and folding case, the ZON title is “The Shaughin Hunting Grounds.” It appears once among 442 current ZON title records and once among the 351 active area contributors. The followed WLD and ZON histories stay on shaughin2 through 2016-01-13 and reach the 2009-08-12 area-tree move. MOB history reaches 2015-10-13; OBJ history reaches 2015-07-13. No alternate-stem rename, distinct predecessor, or whole-zone replacement is established by those bounded histories. The upper shaughin settlement is a separate active area, not a replacement. Active same-type header checks found no competing owner for these 100 rooms, 12 mobiles, or 23 objects.

Keep active shaughin2 DO NOT MOVE. Preserve its upper-area trophy, portal, and worker references and its four active boundary pairs. The inactive Surf WLD approach into #6683 is recorded only on the existing Surf candidate; it does not make Surf or Shaughin2 an active ingress candidate. No migration or gameplay change is proposed.
