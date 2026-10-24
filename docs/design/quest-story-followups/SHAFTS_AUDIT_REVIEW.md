# The Underworld Shafts: connectivity, identity, dependencies, and builder questions

Checkpoint 357; physical coverage line 269; source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. Sources: [WLD](../../../areas/wld/shafts.wld#L1), [ZON](../../../areas/zon/shafts.zon#L1), and tracked empty [MOB](../../../areas/mob/shafts.mob) and [OBJ](../../../areas/obj/shafts.obj) files.

## Source-comprehensive story review

The [story dossier](../zone-stories/SHAFTS.md) maps the environmental story across all 43 rooms. No local mobile, object, QST, shop, Q/QA contract, or structured story sidecar exists. The MOB and OBJ files are zero bytes; the ZON has zone header/range metadata and a terminator but no reset commands. Its header says reset mode 2. The QST AREA list includes shafts, which does not create a local quest file or quest contract.

The story is environmental: carved jade with sharp shards, the scattered tools at room 10007, a steep waterfall with a marked current, hardened fungus, and an unexplained burrow. Room 10030 is absent within the declared range. These are source descriptions, not proof of what builders intended or how the live server resolves hazards.

## Active world connectivity

The WLD has 43 rooms and 84 authored exits. Seventy-four resolve to another local room; six are cross-zone outbound exits, for 80 loader-resolved directions. The remaining four are positive unresolved targets in unregistered ud_map0/1/2 sources. There are no negative targets or room range mismatches. All exit flag and key fields are zero.

| Shafts endpoint | Active reciprocal endpoint | Cross-area register |
| --- | --- | --- |
| 10000 down to Underworld 4580 | Underworld 4580 up to 10000 | [row 77](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L77); [row 190](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L190) |
| 10003 west to MountainTracks 20934 | MountainTracks 20934 east to 10003 | [row 191](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L191); [row 345](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L345) |
| 10021 down to Underdark 844848 | Underdark 844848 up to 10021 | [row 192](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L192); [row 1778](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1778) |
| 10033 down to Underworld 4585 | Underworld 4585 up to 10033 | [row 78](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L78); [row 193](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L193) |
| 10035 south to MountainTracks 20921 | MountainTracks 20921 north to 10035 | [row 194](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L194); [row 343](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L343) |
| 10043 up to Ghore 11548 | Ghore 11548 down to 10043 | [row 195](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L195); [row 212](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L212) |

The six local components are also strongly connected in the directed local WLD graph, with sizes 13, 9, 8, 5, 4, and 4. The active inbound roots are 10000, 10003, 10021, 10033, 10035, and 10043; their local reach/return covers the three boundary components, 22 of 43 rooms. The other 21 rooms fall in components 10004-10011, 10012-10020, and 10026-10029, with no active cross-zone ingress. The first two have exits to unregistered maps; those exits do not make the map sources active. The hard-earth component has no active boundary or exact inbound record from the scanned inactive WLD files.

Coverage records 25 F fall markers and eight C current records; the waterfall rooms 10013-10020 carry the literal C values 55 and 5. This is static reachability only. It does not qualify movement through currents, falls, swimming, or safety in live play.

## Inactive-map approaches and unregistered outbound targets

All 95 WLD files outside the active contributor list were scanned. None defines a room VNUM overlapping the Shafts rooms. Ten exact exit directions in three inactive WLD files target active Shafts rooms. They are source evidence, not live ingress:

| Inactive source | Authored exit to Shafts | Source header / D record / target lines |
| --- | --- | --- |
| ud_map0, room 1104328 | down to 10011 | [22517 / 22537 / 22540](../../../areas/wld/ud_map0.wld#L22517) |
| ud_map0, room 1106354 | up to 10020 | [41698 / 41718 / 41721](../../../areas/wld/ud_map0.wld#L41698) |
| ud_map0, room 1106895 | down to 10003 | [43235 / 43255 / 43258](../../../areas/wld/ud_map0.wld#L43235) |
| ud_map1, room 1111610 | down to 10025 | [6070 / 6090 / 6093](../../../areas/wld/ud_map1.wld#L6070) |
| ud_map1, room 1112933 | down to 10035 | [15629 / 15649 / 15652](../../../areas/wld/ud_map1.wld#L15629) |
| ud_map1, room 1116916 | up to 10004 | [35944 / 35964 / 35967](../../../areas/wld/ud_map1.wld#L35944) |
| ud_map1, room 1117763 | up to 10000 | [42080 / 42100 / 42103](../../../areas/wld/ud_map1.wld#L42080) |
| ud_map2, room 1120722 | up to 10021 | [2234 / 2254 / 2257](../../../areas/wld/ud_map2.wld#L2234) |
| ud_map2, room 1122501 | up to 10021 | [22877 / 22897 / 22900](../../../areas/wld/ud_map2.wld#L22877) |
| ud_map2, room 1122559 | up to 10033 | [23490 / 23510 / 23513](../../../areas/wld/ud_map2.wld#23490) |

Four active Shafts exits target rooms in those unregistered source families. The [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L17) lists all four: 10004 down to ud_map1 room 1116916; 10011 up to ud_map0 room 1104328; 10012 south to ud_map2 room 1121446; and 10020 south to ud_map0 room 1106354. Those exact room headers exist in the current source files, but the files are outside areas/AREA and the normal build omits them. The first, second, and fourth have a matching direct exit back to the Shafts room; room 1121446 has no direct exit back to 10012 in the current source scan. Keep this separate from the six active reciprocal world pairs.

The existing candidate rows remain the correct place to preserve these source dependencies: [ud_map0 row 112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112), [ud_map1 row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113), and [ud_map2 row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114). Checkpoint 357 adds ten reviewed source-only approach records; the cumulative source-only total becomes 4,516, with Surf at 269 and Underworld2 at 9 unchanged. The previously reviewed ud_map2 subset grows from 20 to 23.

## Typed dependency closure

No local MOB or OBJ prototype is defined, so there is no local object prototype for an external QST item command, SHP product, zone reset, type-25 object, or structured story-sidecar VNUM to consume. Exact room-range scans found no external QST or SHP references to 10001-10043; QST hits on 10000 are reward currency or experience values. Structured VNUM-key scans of external story JSON found no reference to rooms 10000-10043. The only non-WLD source matches in compiled C/C++ are an incidental source-line citation fight.c:10003 in a studioproc comment and the player-save error constant 10001; neither is a room binding. The six active WLD boundaries and four unregistered ud_map targets are the typed world dependencies.

## Title, source identity, and history

After color-code normalization, the exact title The Underworld Shafts is unique among all 442 current ZON title records and among the 351 active contributors. The only current files with the shafts stem are the WLD, ZON, empty MOB, and empty OBJ listed above; no alternate-stem or replacement zone appears in the tracked source inventory. Followed history for shafts.wld reaches 2013-11-13 and the 2009-08-12 area-tree move. shafts.zon has a 2015-10-13 zone-overhaul edit, followed by the 2013 and 2009 entries. The empty MOB and OBJ stubs have only older baseline history. No evidence identifies a duplicate or successor zone.

## Builder questions and recommended resolution

| ID | Builder question | Source finding | Recommended resolution |
| --- | --- | --- | --- |
| BSQ-SH-01 | Is the dead-end at 10007, with scattered tools, an abandoned worksite clue? | Room 10007 ends the jade branch and describes tools on the floor. | Preserve the dead end and tools while the area owner confirms its story role. |
| BSQ-SH-02 | Is the absence of room 10030 intentional? | The ZON range extends through 10043, but no WLD room 10030 exists. | Confirm the gap against the builder source before adding, renumbering, or repurposing a room. |
| BSQ-SH-03 | Are the jagged shards in rooms 10000-10003 meant to be a hazard? | The descriptions repeat sharp jade shards and flesh-tearing language. | Keep the authored warning; qualify any mechanical damage separately. |
| BSQ-SH-04 | Should the smooth jade passages 10004-10011 be difficult to traverse? | Text says the surface is smooth and climbing nearly impossible; fall markers occur in this section. | Keep the route topology and verify live movement before describing it as safely climbable. |
| BSQ-SH-05 | Is room 10007 meant to be a true endpoint? | Its only local exit returns to 10006; the text calls it a dead end. | Do not add a connector without builder direction. |
| BSQ-SH-06 | Does the waterfall run from room 10020 down through 10013-10019 to 10012? | The room titles and exits create that sequence; descriptions say the water sweeps everything downward. | Preserve the authored direction until current and fall mechanics are qualified. |
| BSQ-SH-07 | Are the C records with values 55 and 5 intended to pull occupants down the waterfall? | All eight current records are on rooms 10013-10020. | Ask the builder to confirm the intended current direction and force; do not infer safety from graph returnability. |
| BSQ-SH-08 | Are the 25 F fall markers intended to match the danger described in the jade, fungus, and earth rooms? | F markers occur in 25 rooms, including the waterfall approach and several smooth tunnels. | Preserve source values until the area owner confirms live fall behavior and safe returns. |
| BSQ-SH-09 | Is the bottom-of-waterfall exit from 10012 south to inactive room 1121446 intended to be one-way? | The target exists in ud_map2, but no direct reverse exit to 10012 was found. | Keep it classified as an unresolved inactive-map reference pending map-owner confirmation. |
| BSQ-SH-10 | Should the three active exits into ud_map0/ud_map1 remain linked to their matching inactive reverse exits? | Rooms 10004, 10011, and 10020 have exact source-only reverse edges in those candidate files. | Preserve both files and exact VNUMs; do not activate or prune either side based on source presence alone. |
| BSQ-SH-11 | Are the ten ud_map0/1/2 approaches intended historical entrances into active Shafts rooms? | Ten inactive directions target eight Shafts rooms; the source files are not in areas/AREA. | Keep them source-only in candidate rows until their owner, provenance, and intended route are known. |
| BSQ-SH-12 | Should the hardened-fungus corridor from 10021-10025 connect into 10036-10043 through 10025? | Room 10025 has an upward exit to 10036, joining the long fungus passage. | Preserve the join unless the area builder identifies a specific route error. |
| BSQ-SH-13 | Is the hard-earth tunnel 10026-10029 intentionally isolated from active world ingress? | It is a local component with no active cross-zone edge or scanned inactive-map approach. | Keep it separate and unresolved; do not invent an entrance or creature reset. |
| BSQ-SH-14 | Are six separate local components intended for this area? | Local component sizes are 13, 9, 8, 5, 4, and 4. | Retain the components and disclose the three without active ingress until a builder supplies route intent. |
| BSQ-SH-15 | Should the six active reciprocal boundary pairs remain separate access points? | Underworld, MountainTracks, Underdark, and Ghore connect at six distinct rooms. | Preserve every exact pair; evaluate any redesign as an explicit world-map change. |
| BSQ-SH-16 | Does room 10003's west exit into MountainTracks fit the cavern description? | The reciprocal MountainTracks room 20934 exits east to 10003. | Keep the reciprocal connection and adjust prose only if the two builders agree on the geography. |
| BSQ-SH-17 | Does room 10021's down exit to Underdark 844848 intentionally meet the fungus passages? | It is a reciprocal active boundary at the start of the hardened-fungus component. | Preserve the boundary and qualify a live return route before calling it safe. |
| BSQ-SH-18 | Should the room-10043 up exit into Ghore remain the endpoint of the fungus tunnels? | Ghore 11548 exits down to 10043 and the pair is reciprocal. | Retain the pair and its distinct area ownership. |
| BSQ-SH-19 | Is an empty reset/population layer intended for an environmental-only zone? | The MOB/OBJ files are empty, ZON has no reset commands, and no local QST, SHP, or sidecar exists. | Keep the registered stubs; add no population or quest contract without an explicit story decision. |
| BSQ-SH-20 | Are the four active exits into unregistered ud_map rooms intended to remain unresolved in normal builds? | Their targets exist in source files outside areas/AREA, and the normal build omits those map sources. | Retain the warnings and candidate links; require a separate owner decision before any source-list or migration change. |

The review is complete for this coverage row. The route, room-gap, and hazard questions remain builder decisions, not authorization to change gameplay. Shafts remains DO NOT MOVE; preserve all six active reciprocal boundaries, the four unresolved active map exits, and the ten separate inactive-map approaches.
