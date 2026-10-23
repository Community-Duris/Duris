# Cyric Tunnels repository-wide source audit

Checkpoint 203 reconciles active cyrictunnel against its environmental story, reset-aware WLD graph, all-source inbound approaches, typed prototype consumers, compiled bindings, title, and tracked history. This is a static source audit; it does not inspect a live server, player state, database, editor, or persisted world state. The [canonical dossier](../zone-stories/CYRIC_TUNNELS.md) summarizes the authored story and owner questions.

## Source identity and authored story

The active contributor is listed in areas/AREA with range note *545. Its .zon header is #545, reset mode 2, and title The Tunnels of Cyric the Destroyer. It has 82 rooms (#54500–#54581), 16 mobiles (#54500–#54515), 10 objects (#54500–#54509), and 195 WLD exits. There is no local .qst, .shp, or story sidecar. The separate QST/AREA index includes cyrictunnel, but the project inventory distinguishes that historical index from actual production quest sources ([active list](../../../areas/AREA#L232), [zone](../../../areas/zon/cyrictunnel.zon#L1), [QST index](../../../areas/QST/AREA#L212)).

The room and prototype text traces a route from the named Cyric mountain region through a cave-in, black tunnels, a lake and flow of lava, and a subterranean cult devoted to Cyric. The local statue's object description attributes a cataclysm and the later return of the burned realm to Cyric's legend. The necromancer's mobile text says he sacrifices children and lost adventurers and raises their bodies; the high priest, followers, and sacramental dagger reinforce that ritual. No local quest contract or story sidecar turns this environmental account into a structured quest chain.

Fourteen lava-lake and lava-flow rooms use sector value 11 (SECT_FIREPLANE); the source defines SECT_LAVA as 39. The movement handler invokes firesector() on entering FIREPLANE, LAVA, or Liquid Mith sector rooms, so the distinction does not mean those rooms lack a coded hazard. Confirm that FIREPLANE is the intended sector for this lake and flow ([sector definitions](../../../src/core/defines.h#L469), [movement effect](../../../src/world/handler.c#L1542), [example room #54515](../../../areas/wld/cyrictunnel.wld#L283)).

## Active WLD graph and reset-state reach

All 195 authored exits resolve in the active world: 190 local exits and five outbound exits. The 82 rooms form one local weak component, and all-exits-available directed reach from each boundary arrival covers all 82 rooms. No local exit is negative or unresolved. The five inbound exits are the paired return edges shown below.

| Direction | Active WLD edge | Paired edge |
| --- | --- | --- |
| Surface pair | cyrictunnel #54500 up to Surface #581796 | Surface #581796 down to #54500 |
| Underdark pair | cyrictunnel #54529 down to Underdark #757626 | Underdark #757626 up to #54529 |
| Underdark pair | cyrictunnel #54549 up to Underdark #743256 | Underdark #743256 down to #54549 |
| Underdark pair | cyrictunnel #54554 up to Underdark #746483 | Underdark #746483 down to #54554 |
| Connector Zones pair | cyrictunnel #54561 up to Connector Zones #53632 | Connector Zones #53632 down to #54561 |

See the [cyrictunnel exits](../../../areas/wld/cyrictunnel.wld#L15), [Surface return](../../../areas/wld/surface.wld#L1717076), [Underdark returns](../../../areas/wld/underdark.wld#L72789), and [Connector Zones return](../../../areas/wld/connectorzones.wld#L558). The global exact records are in the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L732).

The .zon has 12 unconditional D resets: two blocked; one closed, locked, and blocked; one closed and locked; four closed; and four closed and secret. This leaves 178 of the 190 local directed exits already open, non-secret, and non-blocked after reset. The strict reset-open graph has seven weak components of 40, 19, 8, 7, 6, 1, and 1 rooms. Before switch, key, or secret-door interactions, directed reach is 7/82 from Surface arrival #54500, 40/82 from each of the three Underdark arrivals, and 1/82 from Connector Zones arrival #54561; their union covers 48 rooms.

The blocked cave-in is not a passive barrier in the source. Objects #54500 and #54501 are reset onto rooms #54506 and #54507; both are ITEM_SWITCH objects configured for CMD_PUSH on the corresponding blocked sides. The shared handler clears EX_BLOCKED on that exit and its reverse. Modeling that switch effect makes directed reach from #54500 47/82 while the remaining closed, locked, and secret doors still constrain the graph. The statue #54502 is another push switch for the blocked down exit from #54519. That exit remains closed and locked until key #54504 is used. The runed key is reset onto the necromancer, and the diorite key #54503 matches the locked #54535 south door and is given to the sentry at that threshold ([reset block](../../../areas/zon/cyrictunnel.zon#L8), [object prototypes](../../../areas/obj/cyrictunnel.obj#L1), [item_switch](../../../src/specs/specs.object.c#L302), [automatic ITEM_SWITCH binding](../../../src/world/db.c#L5281), [door reset handler](../../../src/world/db.c#L7857)).

The strictly-open counts omit those source-backed interactions. They do not simulate player commands, secret search, door/key behavior, reset timing, firesector damage, or return safety and do not establish inaccessibility.

## Source-only inactive Surf approaches

A scan of all 95 WLD files outside areas/AREA found five non-overlapping surf.wld approaches into cyrictunnel. They remain source-only records, not active ingress.

| Inactive source | Exit to active cyrictunnel | Evidence |
| --- | --- | --- |
| surf.wld #265588 | Down to #54500 | [room](../../../areas/wld/surf.wld#L1376276), [exit](../../../areas/wld/surf.wld#L1376296), [target](../../../areas/wld/surf.wld#L1376299) |
| surf.wld #267393 | West to #54529 | [room](../../../areas/wld/surf.wld#L1414169), [exit](../../../areas/wld/surf.wld#L1414185), [target](../../../areas/wld/surf.wld#L1414188) |
| surf.wld #270997 | West to #54529 | [room](../../../areas/wld/surf.wld#L1489817), [exit](../../../areas/wld/surf.wld#L1489833), [target](../../../areas/wld/surf.wld#L1489836) |
| surf.wld #271893 | West to #54554 | [room](../../../areas/wld/surf.wld#L1508621), [exit](../../../areas/wld/surf.wld#L1508637), [target](../../../areas/wld/surf.wld#L1508640) |
| surf.wld #273391 | North to #54549 | [room](../../../areas/wld/surf.wld#L1540055), [exit](../../../areas/wld/surf.wld#L1540059), [target](../../../areas/wld/surf.wld#L1540062) |

The 95-file scan found no cyrictunnel source-room VNUM overlap. Preserve all five records on Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). They add five to the curated source-only evidence, bringing the current total to 3,452 overall, 216 for Surf, and 17 for ud_map2. They do not change the active WLD graph or establish travel through the inactive map.

## Typed inbound and outbound dependencies

The local reset table uses only cyrictunnel mobile, object, and room prototypes. In particular, it places the three functional switch objects, both keys, and the necromancer/sentry that carry those keys. A source-level scan of current external consumers found no exact typed references in other files:

| Consumer scan | Result |
| --- | --- |
| Active and unregistered ZON reset roles | No outside reset targets a local room, mobile, or object. |
| QST item commands | No outside QST item command targets a local object or mobile. |
| SHP product entries | No outside shop product targets a local object. |
| Story-sidecar numeric VNUMs | No match among 232 current story sidecars. |
| External MOB/OBJ definitions | No duplicate definition of a local mobile or object VNUM. |
| External type-25 portals and type-29 switches | No value target points to a local room. |
| Exact source references in C/C++ | No local room, mobile, object VNUM or cyrictunnel name match. The generic ITEM_SWITCH binding still applies by object type. |

Mobile #54515 is defined locally but has no local ZON placement, and no external typed reset role was found for it. Its relationship to reset-placed imp #54513 is unresolved. The nearby Surface story sidecar uses the geographic phrase Rugged Foothills of Cyric the Destroyer for separate recipes, but no structured cyrictunnel prototype or room VNUM dependency was found.

## Title, tracked history, and archive disposition

The normalized .zon title The Tunnels of Cyric the Destroyer is unique among all 442 current zone sources. Similar names such as Black Tunnels, Tunnels of Payang, and The Twisting Tunnels of the Durian Underdark identify separate titles. Surface's Mountains of Cyric room names describe the surrounding region and share the active Surface boundary; they do not identify a replacement zone.

Followed WLD, MOB, OBJ, and ZON history retains the cyrictunnel stem to the 2009 area-tree path move. Later same-stem edits update mobile fields, worshiper spelling, zone serialization and quest-item flags, plus an ANSI correction in 2026. No alternate-stem predecessor, retirement, or whole-zone replacement is established.

Keep active cyrictunnel DO NOT MOVE. Preserve its five reciprocal active boundaries, switch/key puzzle, fire-sector effects, and current VNUMs. Keep Surf candidate row 97 RETAIN IN PLACE with all five source-only approaches. No separate attic candidate row or source move is proposed.

## Builder follow-up

1. Confirm whether the QST/AREA cyrictunnel row is intentionally retained without a local QST file.
2. Confirm the cave-in switches, the sentry's key, and the necromancer's runed key form the intended access sequence.
3. Confirm whether the lava rooms should use FIREPLANE sector value 11 or the separate LAVA sector value 39.
4. Confirm whether the unused imp mobile #54515 is deliberate.
5. Reconcile the Cryic/Cyric spelling and copy-edit the room and prototype prose.
6. Confirm the legacy Surf approaches' intended map role before any separate Surf move proposal.

The audit records static source evidence. It does not qualify actual travel, key acquisition, secret discovery, combat, fire hazard severity, or safe return.
