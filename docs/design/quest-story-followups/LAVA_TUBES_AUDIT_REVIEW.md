# Lava Tubes: route, dependency, and history supplement

This bounded review accompanies the [comprehensive Lava Tubes dossier](../zone-stories/LAVA_TUBES.md). It reconciles active and source-only WLD edges, directed reach, reset door states, exact typed dependencies, compiled references, title identity, and same-stem source history. The review is static and does not establish actual player traversal or service availability.

## Active WLD boundaries and directed reach

Active **lava_tubes**, zone 120, contains 152 rooms and 317 authored exits. Of those exits, 311 target another local room, five target an active outside room, and one targets an unregistered WLD room that the normal loader omits. The local weak graph is one component. The six active arrival rooms below each reach all 152 local rooms in the raw directed WLD graph, and every local room has a directed local-WLD path back to each arrival room.

| Active arrival source | Lava Tubes arrival | Boundary status |
| --- | --- | --- |
| Bloody Plains #74465 east | #12037 | Reciprocal with Lava Tubes #12037 east to #74465 |
| Bloody Plains #74049 north | #12097 | Reciprocal with Lava Tubes #12097 north to #74049 |
| ConnectorZones #53734 south | #12161 | Reciprocal with Lava Tubes #12161 north to #53734 |
| IceCrag #97153 south | #12049 | One-way active arrival |
| MountainTracks #20970 east | #12104 | Reciprocal with Lava Tubes #12104 west to #20970 |
| MountainTracks #20973 up | #12131 | Reciprocal with Lava Tubes #12131 down to #20973 |

The pair records are in [the cross-area summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L244), with the separate [IceCrag one-way pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L199); active exits from Lava Tubes are in [rows 213–217](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L213). Those source paths do not model discovery, safety, or reset behavior.

The ZON file resets ten directions closed across five local pairs: the Ghastly Chamber and Lava Tube (#12051/#12100), Xexos's chamber and cellar (#12052/#12159), the Viewing Gallery and secret passage (#12115/#12150), the shanty and emporium (#12157/#12158), and the shop-to-cellar trapdoor (#12158/#12159). Removing all ten reset-closed directions from the raw WLD graph leaves 148 of 152 rooms forward-reachable from at least one arrival and 148 can return to at least one arrival; the weak components then have sizes 140, 8, 1, 1, 1, 1. This filter describes authored reset rows, not a guarantee about current open doors or traversability. The reset rows are in [lava_tubes.zon](../../../areas/zon/lava_tubes.zon#L8).

Room #12163 has the contributor's sole fall field, value 20; no local room has a current field. The fall number is source data, not a verified damage or travel result. The lava prose at #12163–#12166 describes safe-looking ledges beside a chasm, while #12143 describes an outside opening with no footing. See [the lava rooms](../../../areas/wld/lava_tubes.wld#L2777) and the [fall field](../../../areas/wld/lava_tubes.wld#L2802).

## Unresolved active exit and inactive WLD approaches

The active exit from room #12138 east to #282989 is a positive unresolved target. The loader resolves 316 of 317 authored exits and prunes this target because #282989 is not in the active area list. That VNUM appears in inactive **surf.wld** and **surface2011.wld**; it does not become an active destination by matching those sources. See the [unresolved-exit row](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L23).

The sweep of all 95 other WLD sources found four non-overlapping exits into active Lava Tubes. Each is source-only and is not active ingress.

| Inactive source record | Target in active Lava Tubes |
| --- | --- |
| **surf.wld** #280593 west | #12118 |
| **surf.wld** #282376 south | #12097 |
| **surf.wld** #282989 west | #12138 |
| **surf.wld** #283874 north | #12037 |

The source records are at [#280593](../../../areas/wld/surf.wld#L1691165), [#282376](../../../areas/wld/surf.wld#L1728576), [#282989](../../../areas/wld/surf.wld#L1741445), and [#283874](../../../areas/wld/surf.wld#L1760002). **surface2011.wld** has the matching #282989 room VNUM but no separate source-only approach into Lava Tubes. Preserve all four records on the existing Surf candidate row 97; their discovery raises the curated source-only count by four, to 3,477 overall and 235 for Surf. **ud_map2** remains 18 and Underworld2 remains eight.

## Exact typed and compiled dependencies

The bounded source scan found no external reset consumer of a Lava Tubes room, mobile, or object; no duplicate local mobile or object prototype in another source file; no external SHP product or WLD key field targeting a local object; and no external type-25 portal target into the room range. Two direct imports and several compiled quest links do cross contributor boundaries.

| Source | Typed reference | Owner and source-backed context |
| --- | --- | --- |
| Lava Tubes ZON lines 159 and 163 | Reset gives object #606 and object #397 to Xexos, mobile #12025. | Both prototypes are defined in **heavens.obj**: #606 is a canvas sack and #397 a torch. Preserve those imported object identities ([resets](../../../areas/zon/lava_tubes.zon#L156), [Heavens #397](../../../areas/obj/heavens.obj#L4137), [Heavens #606](../../../areas/obj/heavens.obj#L6370)). |
| White Haven QST and story sidecar | Ohnagra #55127 requests local object #12001, with objects #20949 and #20950; the completion output is #55062. | The native QST text also directs players to Lava Tubes and Xexos's marked automaton. The current mapping is recorded at [the QST contract](../../../areas/qst/wh.qst#L2202) and [story request](../../../areas/story/wh.story.json#L993). |
| Compiled ship and mobile procedures | Objects #12001, #12028, and #12029, local mobile #12027, Xexos #12025 and transformed mobile #12026. | **ships.h** assigns the finished heart, fragment, and core numbers ([macros](../../../src/ships/ships.h#L918)). Object #12028 is bound to **moonstone_fragment**; its periodic handler consumes two fragments and one core and produces #12001 ([binding](../../../src/specs/specs.assign.c#L1278), [handler](../../../src/ships/ship_shop.c#L3524)). Startup seeds a fragment on local mobile #12027 and another on mobile #23240 when present or in locker room #31724 ([seeder](../../../src/ships/ship_shop.c#L3628)). |
| Cyric's Revenge crew loader | Object #12029 is put in the treasure chest created for the ship. | This is compiled placement, separate from Lava Tubes' ZON reset ([loader](../../../src/ships/ship_npc.c#L2026)). The loader has an economic-authority guard, so static source does not establish live availability. |
| Xexos / Agthrodos bindings | Mobile #12025 uses **xexos**; mobile #12026 uses **agthrodos**. | Xexos's combat procedure transfers inventory and equipment to the transformed mobile; the paired procedure covers its return ([bindings](../../../src/specs/specs.assign.c#L950), [procedures](../../../src/specs/specs.lava_tubes.c#L30)). |
| Cellar route bindings | Room #12158 uses **automaton_trapdoor**; mobile #12027 uses **automaton_unblock**; object #12027 uses **automaton_lever**. | The object and mobile have separate prototype namespaces. The room hook closes the paired directions on descent; the lever and mobile procedures clear blocked state under their coded conditions ([room binding](../../../src/specs/specs.assign.c#L2521), [trapdoor](../../../src/specs/specs.room.c#L1256), [mobile and object procedures](../../../src/specs/specs.lava_tubes.c#L242)). |

The same finished moonstone is therefore a compiled ship-crew item and an Ohnagra contract input. The source identifies both consumers but does not define builder intent about which outcome owns a particular recovered heart. Keep both interactions and the object identity unchanged until the item-sharing decision is made.

## Title and bounded source history

After color stripping and title normalization, “Lava Tubes” has no duplicate among the 351 active **.zon** contributors; the broader current **.zon** scan also records no repeated normalized title for this source. The similarly themed **lavcav** contributor is separately titled “The Underground Lava Caves,” uses zone 355 and a different room range, and has its own [source dossier](../zone-stories/THE_UNDERGROUND_LAVA_CAVES.md). Similar cave language does not establish a merge or replacement.

Followed **lava_tubes.wld** history reaches the 2009 move into the **areas/** tree and same-stem maintenance through 2013. The followed ZON history includes the 2010 automaton-crew quest addition, the 2013 area edit, and 2015 quest-item flag maintenance. The bounded history identifies no alternate-stem predecessor, title rename, or whole-zone replacement. The normalized active title appears in [lava_tubes.zon](../../../areas/zon/lava_tubes.zon#L1); **lavcav** remains a distinct active contributor.

## Migration disposition

Keep active **lava_tubes** **DO NOT MOVE** as a current un-commented **areas/AREA** contributor. Preserve the five reciprocal active boundaries and IceCrag's one-way arrival, the unresolved #12138 east edge, all four source-only Surf approaches on Surf's existing **RETAIN IN PLACE** candidate row 97, both Heavens object imports, and the White Haven and compiled moonstone consumers. Do not merge **lavcav** on title or theme resemblance. No new candidate is warranted.

Editor and installed-world closure, persisted-player references, computed routes, live door and mobile behavior, safe traversal, and actual item availability remain outside this bounded static review.

## Builder questions and recommended resolutions

The following questions record source-owned decisions for builders. Recommendations retain the authored source and typed references until the responsible owner answers.

| # | Direct builder question | Source-backed recommended resolution |
| --- | --- | --- |
| 1 | Is this one contributor intentionally meant to span the glacier, Mt. Gandar, forest trails, crater, and volcanic caverns under the title “Lava Tubes”? | Keep the current zone identity and title while builders confirm whether the broad mountain approach is the intended frame. |
| 2 | Is active **lavcav**, “The Underground Lava Caves,” intended to remain a separate cave contributor? | Keep **lava_tubes** and **lavcav** separate; their active source stems, titles, room ranges, and dossiers are distinct. |
| 3 | What lies beyond the huge metal door in the glacier at #12005, and is the lack of a local WLD destination intentional? | Preserve the door as an unanswered clue until its destination and owner are named. |
| 4 | Should the crater at #12022 remain uncertain between volcanic and meteoritic origin? | Preserve the authored ambiguity unless a later story owner supplies a source-backed answer. |
| 5 | Who made the unusually regular tunnels and artificial chambers beneath the mountain, and are they meant to connect to Xexos's work? | Keep the natural-versus-artificial contrast; do not assign a builder or history without confirmation. |
| 6 | Are the carved rock perch at #12049 and altar/gallery rooms #12115–#12116 parts of one ritual or simply adjacent landmarks? | Retain their authored descriptions and reset placements without inventing an altar quest or trigger. |
| 7 | Which narrow passages, including the vertical tube at #12141–#12143, are meant to impose size, climbing, falling, or one-way travel constraints? | Treat the prose as clue text; have builders identify each intended mechanism before adding route rules. |
| 8 | Is the one-way active IceCrag arrival from #97153 to #12049 intended, with onward exit through other world boundaries? | Preserve the authored one-way boundary and document the intended departure path rather than adding a reciprocal edge by assumption. |
| 9 | Where should active room #12138's east exit to #282989 lead, given that the target occurs only in inactive **surf.wld** and **surface2011.wld**? | Keep it loader-unresolved until the source-map owner selects a destination or explicitly restores a source; do not infer active status from the matching VNUM. |
| 10 | Were the four inactive Surf approaches into #12037, #12097, #12118, and #12138 intended as old approaches or possible future active routes? | Keep all four on Surf's RETAIN row as source-only; require a separate map and owner decision before changing active membership. |
| 11 | Why is **lava_tubes** listed in **areas/qst/AREA** without a matching local **lava_tubes.qst**? | Confirm whether the list entry is intentional compatibility or stale inventory; keep Ohnagra's White Haven contract under its own contributor meanwhile. |
| 12 | Does Xexos's account of three moonstone pieces intentionally correspond to two #12028 fragments plus the #12029 core required by code? | Preserve the compiled two-fragment-plus-core recipe and ask the quest owner to reconcile the dialogue with that exact item model. |
| 13 | Is the second fragment's alternate placement on Olhydra #23240 or in locker #31724 intentional when the local automaton always receives one? | Keep the current code path and validate the intended holder, random branch, and player hint before changing availability. |
| 14 | Should object #12001 remain eligible for both Ohnagra's White Haven request and Erzul's automaton-crew exchange? | Preserve both source consumers and the shared VNUM; have both quest owners decide how competing or supplied copies are meant to work. |
| 15 | Is placing core #12029 in Cyric's Revenge's compiled treasure chest the intended pirate recovery step, including its current loader guard? | Keep the foreign crew-loader owner and core placement intact; qualify ship availability separately before promising the step to players. |
| 16 | Is Xexos's fight-triggered change into Agthrodos, with inventory and equipment transfer and a return after combat, the intended encounter outcome? | Retain the paired hooks and clarify expected death, reward, and reset behavior without treating the morph as a QST completion. |
| 17 | Does the cellar trapdoor's automatic close/block behavior, lever object #12027, and automaton mobile #12027 form the intended escape sequence? | Preserve the three typed bindings and paired directions; ask builders to confirm the safe release path and automaton conditions. |
| 18 | Which of the five reset-closed door pairs should the skeleton key in the cellar rack open, and are the other doors meant to stay sealed? | Keep all ten reset directions; map each key relationship explicitly before altering the closures. |
| 19 | Is Xexos's Supply Emporium intended to be descriptive only, and is the hostel's posted ban on fighting, theft, and graffiti enforceable? | Do not add a shop or hostel policy based on prose alone; identify a native service or behavior owner first. |
| 20 | Why does Xexos receive Heavens objects #606 and #397, and should their foreign prototype ownership remain explicit? | Preserve the imported sack and torch as Heavens prototypes; keep the reset references typed and review them with both area owners. |
| 21 | Is Mockvrakar's warning symbol and demonic heart meant to imply a repeatable return or a future quest despite the lack of a local contract? | Preserve the reset and warning text; do not create a resurrection mechanic or reward path from the hint alone. |
| 22 | Is the Crystal Phalanx's #12000 spike drop, whose assigned object procedure consumes a charge when cast, the intended local reward? | Keep the death drop and compiled object hook together; have builders confirm whether the item belongs in this area's story mapping. |
| 23 | What live effect should builders associate with the fall value 20 in #12163 and the safe-looking ledges beside the lava chasm? | Keep the authored fall field and WLD prose; qualify engine damage and safe routes through the actual movement path before describing a guarantee. |
