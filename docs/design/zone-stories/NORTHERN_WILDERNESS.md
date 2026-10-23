# Northern Wilderness

Active source `northern_wilderness`, zone 122, is titled `Northern Wilderness` and spans rooms #12200–#12390 (149 room records). Its 21 mobile prototypes and three objects support a broad wilderness map with a tournament valley, quarry, chasm, and cultivated river valley. The story is environmental; the local QST file and story sidecar are absent, even though `areas/qst/AREA` lists the stem.

Sources: [world rooms](../../../areas/wld/northern_wilderness.wld), [mobiles](../../../areas/mob/northern_wilderness.mob), [objects](../../../areas/obj/northern_wilderness.obj), [resets](../../../areas/zon/northern_wilderness.zon), [full-world membership](../../../areas/AREA#L69), [QST-area membership](../../../areas/qst/AREA#L63), [active routes and dependency review](../quest-story-followups/NORTHERN_WILDERNESS_AUDIT_REVIEW.md).

## Story in the source

The northern hills overlook the Calimport valley beneath the Talbert and Eagle mountain ranges. Pine paths, rocky slopes, a trench and an old wooden mountain door establish a wilderness border. The source describes the door but does not give it a WLD exit. A red tree on the eastern trail carries the area's clearest lore: Elenar writes to Moralon to seek an answer inside the mountain and hide a clue among golden flecks.

That inscription is an upstream clue in the separate Caves of Mt. Skelenak story. Caves room #4067 contains Moralon's golden-flecked writing; its text points toward a Calim desert oasis. The active Caves QST has a `moralon` topic for Goortok's tests, while the Caves story sidecar keeps the three offerings and the lore answer as separate outcomes. The Northern Wilderness tree is not itself a QST trigger, does not complete an offering, and does not prove a current route to the cave or oasis. The current source-based connection is described in the [Caves of Mt. Skelenak dossier](CAVES_OF_MT_SKELENAK.md#lore-topics-and-narrated-outcomes).

An active High Moor osquip hunter adds regional context: it says lizardmen have led to the slow destruction of the osquip race in the northern wilderness. The local Northern Wilderness mobile list has no osquip or lizardman prototype, so this outside line supplies lore without a local encounter or route.

Other landmarks give the area several distinct moods. The Valley of Tournaments is arranged around combatants, spectators, and a sign advertising fights; the source defines no tournament rules or player reward. The quarry has a boarded and chained shaft, and a gravel-hidden space houses a raving lunatic. A nearby overgrown path passes a life-sized statue whose face and one arm are missing. In the chasm, inconsistent wind, narrow ledges, a blackened oval opening, and two fall markers suggest danger, but they do not define a complete encounter or safe route. Farther south, a separate river valley contains primitive huts, a cultivated field, and soot-marked mining tunnels.

## Current interactions and services

There is no `areas/qst/northern_wilderness.qst`, no local QST giver or contract, no local SHP file, and no `areas/story/northern_wilderness.story.json`. The three local objects are a sharp rock, a moss patch, and the Valley of Tournaments sign. Resets put the moss near the damaged statue, the sign near the tournament valley, and the sharp rock on the hidden lunatic. The reset population covers ordinary wildlife and the tournament combatants; the local squirt-bug prototype #12200 is not reset by this area's ZON file.

Mobile #12210 is a small bat. Ten active reset commands in `troll_caves.zon` also name that prototype at room #96981. The reset loader resolves `M` commands against the global mobile index, so this is a cross-area prototype dependency, not a route from Troll Caves into the Northern Wilderness. Preserve it until the owner confirms whether the reuse is intentional.

The active Centaur Villages story sidecar describes Northern Wilderness room #12269 as a northward entrance to pass #93300. It says the pass's southern exit returns to Surface room #519684. This matches the authored WLD boundary, but there is no return exit from the pass into #12269. The path is a one-way outward route from this area in current WLD source.

## Static navigation

The WLD has 348 authored exits, all loader-resolved: 347 remain in the area and one goes north from #12269 to Centaur room #93300. The local graph has three weak components of 112, 26, and 11 rooms. From #12200, 101 rooms in the largest component are reachable and 112 can reach #12200. The 11-room chasm pocket (#12271–#12310) can exit south from #12271 into the main component at #12272, but no authored local path leads from that component into the pocket. The quarry rooms (#12256–#12266) and river-valley rooms (#12350–#12390) form separate internally reachable components; each can be fully traversed and returned to from #12256 and #12350 respectively.

There are 21 local exit records without an opposite-direction counterpart. All exit records have raw state 0, the local ZON contains no door-reset commands, and two rooms (#12238 and #12239) declare a fall chance of 10. Static reachability does not qualify fall outcomes, arrival safety, or a practical return. At the Centaur pass, #93300 also has an active southward return to Surface #519684, reciprocated by Surface; that boundary does not return to #12269. No active WLD exit enters the Northern Wilderness, and the all-95 inactive-WLD sweep found no inactive room overlap or exit into its 149 room IDs.

Two `E reference` extra descriptions contain text that looks like `D0` exit data: #12272 mentions #12270, and #12310 describes an entrance to an underground city at #6803. Room #6803 currently exists in active Dragonnia as “Outside the City of Dragonnia,” but no active WLD edge connects it to #12310. The WLD loader reads both values as extra-description text; neither creates an exit. Treat the alleged links as prose until a builder confirms an actual destination and route.

## Builder decisions

The source supports a linked mountain-lore lead, a remote tournament valley, and several isolated environmental subareas. It does not define the chasm's intended entry, a complete return-safe mountain route, tournament rules, or a local objective. The [audit supplement](../quest-story-followups/NORTHERN_WILDERNESS_AUDIT_REVIEW.md#checkpoint-314-twenty-direct-builder-questions-and-recommendations) records twenty source-backed questions and conservative recommendations. Keep the active area and its imported bat prototype in place while the owner resolves those decisions.
