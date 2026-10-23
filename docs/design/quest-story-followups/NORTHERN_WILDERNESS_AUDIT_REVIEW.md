# Northern Wilderness: graph, dependencies, history, and retention

Checkpoint 314 · active area [`northern_wilderness`](../../../areas/AREA), zone 122 · title in [`northern_wilderness.zon`](../../../areas/zon/northern_wilderness.zon#L1) · source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`.

This supplement adds directed navigation, cross-area routes, inactive-WLD approaches, typed dependencies, title/history, and retention findings to the [source dossier](../zone-stories/NORTHERN_WILDERNESS.md). It records authored data and static loader behavior. The server and gameplay were not run, so reset execution, hazards, and safe return are not qualified.

## Directed WLD graph and active routes

The local WLD has 149 room records and 348 exits. All 348 targets resolve: 347 are local and one is north from #12269 to Centaur room #93300. The resolved local graph has weak components of 112, 26, and 11 rooms. From #12200, 101 rooms in the 112-room component are reachable, while all 112 can reach #12200. The 11-room pocket #12271–#12310 is its own strongly connected component, with a single bridge south from #12271 to #12272; that bridge points out of the pocket, so no local authored route leads into it from the main component. The quarry #12256–#12266 and the river valley #12350–#12390 form separate components of 11 and 26 rooms, each fully reachable and returnable from its lowest-numbered listed entrance (#12256 and #12350 respectively).

Twenty-one local exit records lack an opposite-direction record. Every exit's raw state is zero, the local reset file has no `D` door commands, and no current marker was found. Rooms #12238 and #12239 each declare a fall chance of 10. These are source values, not a live fall probability, survival check, or return guarantee.

| Boundary or source reference | Target | Current classification |
| --- | --- | --- |
| #12269 north | Centaur #93300, [`Pass Into the Mountains`](../../../areas/wld/centaur_zone.wld#L1) | One-way active cross-area exit. The exact record is [cross-area register row 218](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L218); no active WLD exit returns to #12269. |
| Centaur #93300 south | Surface #519684 | This separate boundary has a reciprocal Surface north exit ([Centaur row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1125), [Surface row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1359)). It returns to Surface, not to Northern Wilderness. |
| Other active WLDs | #12200–#12390 | No active inbound edge was found. |
| All 95 inactive WLDs | #12200–#12390 | No inactive room ID overlap and no inactive exit into any of the 149 actual room IDs. |

The inactive-WLD pass parsed 527,791 room records and 2,080,773 direction records without a structural issue. No Duris3, Surf, or other candidate receives a new approach from this pass. The active route from #12269 is represented in the existing active cross-area register, not the attic candidate register.

Two room extra descriptions use the keyword `reference` and contain text resembling exit fields. At #12272 the body includes `D0 0 0 12270`; at #12310 it says an underground city is at #6803 and includes `0 0 6803`. Room #6803 currently exists in active Dragonnia as “Outside the City of Dragonnia” ([room source](../../../areas/wld/dragonnia.wld#L35)), but it has no WLD edge to #12310. In [`db.c`](../../../src/world/db.c#L1382), top-level `D` records are parsed as exits while `E` records consume two tilde-terminated strings. These values therefore remain extra-description text and do not add a #12270 or #6803 edge.

## Story continuity and typed dependencies

The red-tree inscription at room #12243 gives Moralon a clue from Elenar: look inside the mountain, where the answer will be hidden among golden flecks. Active Caves room #4067 contains the corresponding golden-flecked writing; its text says Moralon found the answer and intends to hide the king's name at a rumored ancient oasis in the Calim desert ([Caves source](../../../areas/wld/caves_skelenak.wld#L1563)). The active Caves QST includes Goortok's `moralon` exchange and later item tests ([QST source](../../../areas/qst/caves_skelenak.qst#L3)); the [Caves story sidecar](../../../areas/story/caves_skelenak.story.json#L8) distinguishes those accepted offerings from hearing lore or learning the monolith's answer. No current oasis continuation was verified. The Northern Wilderness inscription is a story lead, not a local QST contract, item check, or completion event.

The active Centaur story sidecar describes #12269 as a route into #93300 and continues from that pass to Surface #519684, through a mountain path and fall-prone incline rooms ([orientation](../../../areas/story/centaur_zone.story.json#L11)). This is a narrative map consistent with the active WLD records, not a reciprocal return into the Northern Wilderness. A High Moor mobile separately mentions the destruction of the osquip race in the northern wilderness ([source text](../../../areas/mob/highmoor.mob#L551)); it supplies regional lore but no typed route or objective. Connectorzones prose also uses the phrase “northern wilderness” for regional geography and does not declare an exit into this area's room IDs.

The typed scan used the actual local room, mobile, and object prototype sets, keeping numeric domains separate:

- No outside QST item command or SHP keeper/product points to these prototypes. The local QST manifest has an entry, but the local `.qst` file is absent; there is no local SHP file or story sidecar.
- The only active external ZON prototype reference is ten `M` commands in [`troll_caves.zon`](../../../areas/zon/troll_caves.zon#L277) for mobile #12210 at room #96981. The local definition is a small bat ([mobile source](../../../areas/mob/northern_wilderness.mob#L136)); the comment in Troll Caves says the type is not in that area's `.mob` file. `renum_zone_table` resolves `M` arguments through `real_mobile` and disables a command only when resolution fails ([loader](../../../src/world/db.c#L1634)). Preserve this source-level cross-area reset dependency; actual reset outcomes were not run.
- No external mobile or object prototype duplicates the local mobile/object IDs. Across 442 object files, the scan found no type-25 destination into any of the local room IDs. No direct compiled numeric VNUM use was found; the only matching decimal substring in C/C++ was part of the unrelated `HASH_BLOOM_SIGNATURE` hexadecimal constant in `uthash.h`.
- A broad exact-number search of outside story sidecars found the Centaur route sentence for #12269. The Caves link is expressed through names and matching source prose rather than a typed room reference.

These results close the reviewed source declarations. They do not rule out manually supplied routes, runtime-generated transitions, or non-VNUM mechanisms that were not exercised.

## Title, history, and replacement finding

After color stripping, `Northern Wilderness` appears once among all 442 current `.zon` titles. Nearby names such as `Northern Lakes and Settlements`, `Alterian Region - Wilderness`, and `Arachdrathos Wilderness` are distinct titles and stems. Followed WLD, MOB, OBJ, and ZON history remains on `northern_wilderness`. The reviewed history includes the 2009 move into the current `areas/` tree, source-format maintenance, a 2013 content pass, and a 2015 mobile/reset quest-item flag pass. No alternate-stem predecessor, tracked rename, or explicit whole-zone replacement was found. The linked Caves lore and Centaur boundary are independent stories/routes, not replacement evidence.

## Safe retention plan

Keep active `northern_wilderness` **DO NOT MOVE**. It is an un-commented contributor in `areas/AREA`, owns the room graph and local prototypes, and supplies small-bat mobile #12210 to ten active Troll Caves reset commands. Preserve the one-way #12269 north exit and record the lack of an incoming route as a builder decision; do not infer that the separate Centaur-to-Surface return brings players back here. Keep the chasm prose references distinct from actual edges. No candidate row or source-only approach is added. The existing Duris3 row remains at 26 active exit references, and curated source-only totals remain 4,247 overall, 250 Surf, 19 ud_map2, and eight Underworld2.

## Checkpoint 314: twenty direct builder questions and recommendations

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| NW-01 | Is the #12271–#12310 chasm pocket meant to be reached only through a one-way descent or entry outside this area's WLD? | Keep its one-way exit into the main area as authored until the intended entry and a safe return or rescue path are specified. |
| NW-02 | Is the `D0 0 0 12270` text in #12272's `reference` extra description only a note, or was a WLD exit intended? | Treat it as prose; add an exit only after confirming the destination and return direction. |
| NW-03 | Is #12310's `reference` description meant to point into active Dragonnia room #6803, “Outside the City of Dragonnia,” or is that number only a lore note? | Keep the areas unconnected until the owner confirms the story link and specifies a traversable route and return; current WLD source declares none. |
| NW-04 | Are the fall chances of 10 at #12238 and #12239 intentional, and are their destinations #12300 and #12299 safe landings? | Preserve the source values pending a live hazard and return review; document protection and escape before making a travel guarantee. |
| NW-05 | Should the trench's old wooden door into Talbert Mountain remain scenery, or should it connect to an active room? | Keep it non-traversable until a loaded destination, access condition, and return are named. |
| NW-06 | Are the boarded shaft at #12260 and gravel-hidden lunatic at #12262 intended to form a quarry secret or encounter? | Preserve their present reset and prose; define any digging, key, trigger, or reward before adding a mechanic. |
| NW-07 | Does the damaged statue on the overgrown path belong to a named history or faction? | Leave its identity unexplained unless a builder can supply a source-backed connection. |
| NW-08 | Is Elenar's tree inscription meant to point players from Northern Wilderness into the existing Caves of Mt. Skelenak Moralon story? | Maintain it as an upstream clue; do not make reading it complete or unlock a Caves offering without an explicit event. |
| NW-09 | Does the oasis named by Moralon's Caves writing still have an intended current room or active area? | Identify the exact active destination before extending the clue chain; the prior source review did not verify one. |
| NW-10 | Should finding the mountain clue change Goortok's QST state, or does his `moralon` topic remain independent of it? | Preserve Goortok's declared offering and lore outcomes separately unless an explicit native prerequisite is designed. |
| NW-11 | Should #12269 north continue as a one-way exit to Centaur #93300, or should #93300 gain a return to #12269? | Keep the current edge pending an owner decision; any change should define both boundary directions and arrival rooms. |
| NW-12 | Is the Centaur-to-Surface path intended as a reliable way out for someone entering from Northern Wilderness? | Review the full route and fall-prone incline hazards; the Surface return does not return to this area. |
| NW-13 | Is the Valley of Tournaments intended for player participation or only as an environmental battle setting? | If players participate, define entry, rules, level range, refusal/escape, outcome, and reward before authoring a quest. |
| NW-14 | Are the reset tournament fighters meant to attack one another, challenge the player, or simply populate the valley? | Confirm combat behavior and spawn expectations from the builder; do not infer a playable bracket from the room text. |
| NW-15 | Is Troll Caves' use of Northern Wilderness mobile #12210 (small bat) intentional? | Preserve the imported prototype reference and document its owner; replace the reset only if a different source mob is chosen. |
| NW-16 | Are the ten identical Troll Caves `M` commands for #12210 at #96981 deliberate population resets? | Confirm their intended cap and placement before consolidating or changing the source lines. |
| NW-17 | Should the local squirt-bug prototype #12200 remain unused by this area's resets? | Treat it as an unused definition until its intended placement is confirmed; do not add a spawn based only on prototype presence. |
| NW-18 | Are the sharp rock, moss patch, and tournament sign scenery, or should any have an explicit player use? | Keep their reset placements as authored; define a specific action and result before attaching mechanics. |
| NW-19 | Should the separate #12350–#12390 river and farm valley stay independently reachable, or connect to the northern trail network? | Preserve the separate component until a connecting route and safe return are selected. |
| NW-20 | Should Northern Wilderness remain a no-QST environmental-story area, or should a local player objective be authored? | If one is desired, specify action, proof, reward, level band, repeat/limit policy, and safe access first; otherwise document the current hooks as lore. |

## Scope and validation

Checkpoint 314 changes documentation and the Northern Wilderness coverage row only. It does not change WLD, MOB, OBJ, ZON, QST, code, routes, or operational state. The source-only graph, inactive-WLD, title/history, and typed-reference scans were read-only. No build, test, database operation, migration, server boot, gameplay session, or operational script was run. Documentation links, CSV shape, the 20-question count, whitespace, and restricted source-directory status are checked after the register update.
