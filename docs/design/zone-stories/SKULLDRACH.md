# Skulldrach: source story map

Checkpoint 363 maps physical coverage row 276, area:skulldrach. The active zone is headed #127, titled Skulldrach, and uses reset mode 1. Its source inventory has 252 rooms, 12 mobile prototypes, six object prototypes, and 220 reset commands. The area is a fixed outdoor route through a shadow grove, forest trails, Skulldrach Lake, the Moody River, and foothills along the Alterian range. One mobile carries a shield used by an exact-item request in Bastine. There is no local QST file, Q/QA contract, shop file, or story sidecar.

This is a static source map. It records authored text, data, and typed references; it does not certify live access, reset outcomes, safety, or item acquisition.

## Source inventory

| Source | Reviewed content |
| --- | --- |
| [Area list](../../../areas/AREA#L72) and [zone header](../../../areas/zon/skulldrach.zon#L1) | Active contributor skulldrach, zone number 127, title Skulldrach, range declaration through 12972, reset mode 1. |
| [World file](../../../areas/wld/skulldrach.wld) | All 252 rooms and 691 directional records, including two reciprocal active cross-area pairs and three negative destinations. |
| [Mobile file](../../../areas/mob/skulldrach.mob) | All 12 prototypes: shadow-grove encounters, trail and shore wildlife, a fisherman, a stone golem, and lake creatures. |
| [Object file](../../../areas/obj/skulldrach.obj) | All six prototypes: two fishing vests, a blessed shield, and three rusty armor pieces. |
| [Reset file](../../../areas/zon/skulldrach.zon) | All 215 M, four E, and one G command. Every command has a 100-percent chance value. |
| [Quest-area index](../../../areas/qst/AREA#L65) | Lists skulldrach, although no areas/qst/skulldrach.qst file exists. |
| [Bastine quest](../../../areas/qst/bastine.qst#L50) and [story entry](../../../areas/story/bastine.story.json#L79) | Bastine requests exact object VNUM 12802, the local blessed shield; its hint names the stone golem near Skulldrach Lake. |
| [Repository audit supplement](../quest-story-followups/SKULLDRACH_AUDIT_REVIEW.md) | Static connectivity, inactive approaches, title and path history, external consumers, and migration disposition. |

## The authored landscape

The zone contains two weakly connected WLD components. The larger component has 225 rooms and includes the shadow grove, the crystal bridge, the main forest and shoreline routes, the lake grid, and the two active boundaries to Citadel and Surface. The second component has 27 rooms in the forest and foothills; it has no authored WLD connection to the larger component. See the [connectivity review](../quest-story-followups/SKULLDRACH_AUDIT_REVIEW.md#static-connectivity-and-source-only-approaches) for exact directions and targets.

| Room-title family | Count | Authored place and story |
| --- | ---: | --- |
| Inside a Large Grove of Shadows | 94 | Gray trees and close shadows fill the main grove. The prose says sound and paths are confusing, but the directions are fixed in the WLD. Room 12743 is a small clearing inside this repeated title family. |
| On Skulldrach Lake | 71 | The lake interior has its own dense room grid and the fish, swarmfish, and tentacle-beast encounters. |
| On the Shores of Skulldrach Lake | 43 | The banks follow a thick forest around a lake described as a huge peanut laid northwest to southeast. The Moody River flows west out of the lake. |
| A Forest Trail | 16 | The main wooded path winds toward the shores, with a clearing and an end-of-trail room among the title variants. |
| Vague Trail Across the Hilly Plains | 10 | The detached foothill route runs along the base of the Alterian mountain range. The text describes snow-capped peaks and a view from a small hill. |
| A Dark Trail | 8 | A dense canopy closes above the route. One room, 12859, connects south to the active Surface bridge. |
| A Clearing | 2 | One clearing is on the main forest trail and one lies on the detached trail. |
| Unique room titles | 8 | Entrance to a Dark Shadow Grove, a small clearing in the grove, Before a Crystal Bridge, End of the Forest Trail, An Old Rickety Bridge, A Gully, Top of a Small Hill, and A Small Landing. |

Room 12700 is the entrance to the shadow grove. Its text points north into the grove and describes stairs down to room 12935, the small landing. Room 12796, Before a Crystal Bridge, describes a crystal span over a chasm filled with shadowy mist. Its north exit reaches Citadel room 13000, the drawbridge before the Citadel; the reciprocal exit returns south to 12796. A stone golem is reset at 12796.

The lakeside route crosses the Moody River at room 12839, An Old Rickety Bridge. Its description places the lake to the east and describes the river flowing west out of the lake. Shore rooms on either side describe paths following the banks. Rooms titled On Skulldrach Lake carry the water setting and wildlife spawns; source prose mentions shark-infested water, but the local mobile set is fish, swarmfish, and a tentacle beast.

The detached 27-room component begins at forest-trail room 12847 and runs through a gully, a clearing, the dark-canopy trail, and the vague foothill trail. It ends at the base of a mountain in room 12875. Its only three outward directions target -1: north from 12847, south from 12864, and south from 12875. These are negative destinations in source data, not live exits. Preserve the authored separation until an area owner supplies intended endpoints.

## Mobile ecology and reset placement

| Mobile | Authored identity | M reset lines |
| ---: | --- | ---: |
| 12700 | Wraith, described as awakened and vindictive | 15 |
| 12701 | Swirling mist that moves against the wind | 20 |
| 12702 | Ghoul shedding flesh | 20 |
| 12703 | Darkwolf | 20 |
| 12704 | Stone golem placed to guard the crystal bridge | 1 |
| 12800 | Stork on the wooded trail | 20 |
| 12801 | Solitary fisherman | 1 |
| 12802 | Large dragonfly | 10 |
| 12803 | Swarm of mosquitoes | 5 |
| 12901 | Magical tentacle beast | 8 |
| 12902 | Large fish whose fin shows above water | 15 |
| 12903 | Swarmfish | 80 |

The repeated grove resets distribute wraiths, mist, ghouls, and darkwolves through the shadow rooms. Storks, dragonflies, and mosquitoes occupy the woodland and shore route. The lake grid has repeated groups of swarmfish, with fish and tentacle-beast placements between those groups. The reset data also equips rusty armor on a darkwolf, a rusty helm on a ghoul, a rusty sword on swirling mist, and a fishing vest on the fisherman. One G command gives the blessed shield to the golem after its M command.

The golem's description says its creator first intended it to stop entry to the crystal bridge, then made a truly impassable wall of force and left the golem behind. The authored WLD boundary itself has zero exit flags and zero keys; the zone contains no door-reset command. The source prose therefore does not establish a working gate or force wall.

## Objects and the external request

| Object | Prototype and local reset use |
| ---: | --- |
| 12800 | Fishing vest prototype. No local object reset or bounded typed consumer was found. |
| 12801 | Another fishing vest, equipped on the fisherman in room 12836. |
| 12802 | Blessed shield of defense. The zone gives it to the golem at room 12796. Bastine's exact-item request requires this object VNUM. |
| 12803 | Rusty iron helm, equipped on a ghoul. Its numeric VNUM also belongs to the separate local mosquito mobile prototype; code references to 12803 must be resolved by type. |
| 12804 | Rusty iron armor, equipped on a darkwolf. |
| 12805 | Rusty iron sword, equipped on swirling mist. |

At [Bastine QST line 50](../../../areas/qst/bastine.qst#L50), the command is G I 12802. The matching [story step](../../../areas/story/bastine.story.json#L79) assigns the item to King Mrotha's Adept Warrior request and says that the stone golem near Skulldrach Lake carries it. The Skulldrach reset sequence places shield 12802 on golem 12704 at room 12796. Preserve this typed dependency if the object, mobile, reset, or room is changed.

The local source has no quest text despite its entry in qst/AREA. It has no shop source. Two maintained code paths load mobile 12803 as a mosquito: the wonder-item summon action and the Highway special. The same numeric value is also the object VNUM for a rusty helm, but neither code call loads that object. No local type-25 portal object is present.

## Builder questions and conservative recommendations

1. **Should the zone remain titled Skulldrach while Surface map descriptions also name Skulldrach Lake?** Keep the unique zone title; retain separate ownership for the Surface map cells and the active reciprocal exit at room 632637.
2. **Should the 94 repeated shadow-grove titles remain as written?** Preserve the repeated titles and descriptions until the area owner requests a naming pass; they do not prove a random-path mechanic.
3. **Should the 225-room component remain joined as one authored route?** Preserve its existing exits and both active boundaries; the static graph reaches all 225 rooms from room 12700.
4. **Should the separate 27-room forest and foothill component remain separate?** Keep its current graph distinct until the missing map connections are identified.
5. **What should replace the north exit from room 12847 that targets -1?** Leave the negative target documented and do not guess a neighboring map VNUM.
6. **What should replace the south exit from room 12864 that targets -1?** Preserve the current source record until the path owner identifies its endpoint.
7. **What should replace the south exit from room 12875 that targets -1?** Keep it unresolved; the prose says a mountain blocks the trail to the south.
8. **Should the Crystal Bridge boundary continue to connect to Citadel room 13000?** Preserve the reciprocal pair #12796 north and #13000 south.
9. **Does the golem or wall-of-force prose imply a blocked bridge?** Keep current WLD flags, keys, and resets. No door flag or door reset implements that claim in this source.
10. **Should the Surface connection from the dark trail remain at room 12859?** Preserve the reciprocal pair #12859 south and Surface #632637 north.
11. **Should inactive Surf room 273887 remain a candidate approach?** Keep its north exit to #12859 on the existing Surf candidate row as source-only; it is not active ingress.
12. **Should the old rickety bridge remain a simple river crossing?** Preserve its authored shore route and avoid adding a key, lock, or bridge mechanic based only on its description.
13. **Should the grove prose about disorientation add dynamic maze behavior?** Keep the fixed WLD directions; no random displacement or maze procedure was found.
14. **Should the raw room-flag variations be normalized?** Retain the authored values pending an owner decision. Do not infer an undocumented flag meaning from the prose.
15. **Should lake rooms gain currents or fall behavior?** Preserve the current room and exit data. The reviewed source has no fall or current markers.
16. **Should reset mode 1 and the repeated wildlife populations change?** Keep all 220 reset commands and their authored chance values pending a reset and gameplay review.
17. **Should the golem continue carrying shield 12802?** Preserve the local M/G sequence and the golem's placement at the bridge while the Bastine request depends on this item.
18. **Should Bastine's exact shield request change?** If object 12802 is replaced or renumbered, update the Bastine QST and story entry together.
19. **Should the qst/AREA entry stay without a local QST file?** Keep the discrepancy visible and ask the area owner whether the index entry is intentional; do not invent a local quest.
20. **Should unused vest 12800 or the two vest prototypes be consolidated?** Preserve both identities until the owner confirms whether 12800 is intentionally unplaced and whether 12801 remains the fisherman's equipment.

## Scope limits

The source map does not establish player access, safe return, underwater survival, door behavior in play, reset availability, golem loot persistence, or Bastine quest completion. No test, build, server boot, or gameplay session was run for this documentation-only checkpoint.
