# An Ancient Bridge (`ancientb`) source story dossier

This dossier maps the environmental story and visible NPC/item hooks in the
active zone source. It does not add a quest or claim an outcome that the files
do not implement. The [Checkpoint 182 audit supplement](../quest-story-followups/ANCIENTB_AUDIT_REVIEW.md)
records the source graph, cross-area references, title/history review, and
archive disposition.

## Story shape in the authored rooms

### A watched road across the river

The zone presents a worn stone road across open plains and an old bridge. Two
garrisons guard the crossing, gates lead to horse corrals, and the reset file
loads Tharnadian and Woodseer guards alongside travelers, scouts, and mounts
([bridge approach](../../../areas/wld/ancientb.wld#L1),
[eastern garrison](../../../areas/wld/ancientb.wld#L142),
[western garrison](../../../areas/wld/ancientb.wld#L369),
[guard and traveler resets](../../../areas/zon/ancientb.zon#L50)). The source
suggests a maintained, politically watched route, but it does not define an
inspection, toll, faction choice, escort, or travel quest.

The active map sends both bridge ends outward to Surface rooms #572443 and
#572446. Neither Surface room has an active reverse exit into Ancient Bridge;
the source graph therefore describes departure edges, not a current inbound
route ([western end](../../../areas/wld/ancientb.wld#L1),
[eastern end](../../../areas/wld/ancientb.wld#L561)).

### A hidden under-bridge settlement

A concealed down exit leads beneath the span. Repeated room descriptions place
flood debris, flattened trees, many small footprints, and smoke from a campfire
along the dark bank. A crude driftwood hut stands above the waterline, with a
small door into a room that repeats the troll-like smell and bone-strewn
description ([secret descent](../../../areas/wld/ancientb.wld#L176),
[riverbank](../../../areas/wld/ancientb.wld#L585),
[hut exterior](../../../areas/wld/ancientb.wld#L802),
[hut interior](../../../areas/wld/ancientb.wld#L824)). The reset file places a
kobold family across this under-bridge area: a searching father, several
children, and a mother in the hut ([kobold resets](../../../areas/zon/ancientb.zon#L65),
[mother prototype](../../../areas/mob/ancientb.mob#L1)).

### The troll and the lost crystal

The Bridge Troll's home contains humanoid and creature bones. Its room text
first calls a small glowing object a burning light, then describes it as a
“magical babble” that someone lost to the troll. The reset gives local object
#85600 to mobile #85603; the object itself is a small glowing crystal ball
([troll room](../../../areas/wld/ancientb.wld#L657),
[troll and crystal resets](../../../areas/zon/ancientb.zon#L67),
[crystal object](../../../areas/obj/ancientb.obj#L1),
[troll prototype](../../../areas/mob/ancientb.mob#L43)). This is ordinary reset
stock. There is no local QST contract or story-sidecar item reference that
records the object's owner, recovery, or an outcome for the person who lost it.

The kobold mother carries the local dog-like hide boots through an ordinary
G reset ([boots reset](../../../areas/zon/ancientb.zon#L73),
[boots object](../../../areas/obj/ancientb.obj#L16)). The source does not link
the boots to the crystal, a contract, or a recorded reward.

## Builder questions and recommended resolutions

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| Are both Surface departures meant to have an active arrival and return route? | Ancient Bridge has two active one-way exits to Surface and no active inbound WLD edge. Three approaches appear only in inactive `aopal.wld` and `surf.wld` files. | Decide which map owns each route, then author and qualify the intended reciprocal paths before describing the bridge as ordinarily reachable from both sides. Preserve the 2014 anti-cheese access intent when reviewing transit. |
| What is the relationship between the two garrisons and their guards? | The room text describes separate bridge posts; mobile text names Tharnadia and Woodseer, and the resets also import Heavens mobile #445, a Tharnadian highway patrolman. | Confirm the intended jurisdictions and whether the imported patrolman belongs at both bridge ends. Keep the reset dependency explicit if the patrolman remains. |
| Are the kobolds meant to live beneath the bridge alongside the troll? | Footprints, smoke, the hut, family resets, and the troll's nearby bone-filled rooms all occupy the under-bridge route, but no dialogue or outcome explains their relationship. | Decide whether the coexistence is a threat, a hidden family, or atmosphere. If it is meant as a player-facing conflict, define the parties, choices, and lasting consequence before adding a quest. |
| Is the crystal ball only lost property and troll loot? | The troll-room prose calls it a lost “magical babble,” and an ordinary reset equips the troll with the glowing crystal-ball object. No local quest contract or story item reference consumes or awards it. | Clarify the prose or add an owner/recovery outcome only if one is intended. Until then, keep it ordinary stock and do not promise a quest reward or resolution. |
| Do the corrals promise usable mounts or just explain the guards' horses? | The bridge includes two corral rooms and multiple horse/pony prototypes; the reset file loads them there. | Decide whether a player can interact with or borrow a mount. If not, keep the corral as setting and avoid implying an installed rental or escort service. |
| Is the roster note `Bridge Zone` an intended player-facing or command-target alias for “An Ancient Bridge”? | The active key is `ancientb`, its normalized ZON title is unique among 442 current titles, and the area roster carries the separate note `*856 Bridge Zone`. No explicit alias mapping is present in the reviewed local files. | Keep `ancientb` and “An Ancient Bridge” as the source key and display identity. Add another alias only if its exact target and intended scope are confirmed by the owner. |

The active area appears in the quest index but has no local `ancientb.qst`,
`.shp`, or story sidecar. Its source supports a guarded travel corridor and a
small under-bridge settlement, but not a recorded rescue, investigation,
crystal recovery, or mount service. Keep those choices with the area owner and
preserve Ancient Bridge in the active map.
