# Dark Forest repository-wide source audit

Checkpoint 204 reconciles active dark_forest against its environmental story, reset-aware WLD graph, all-source inbound approaches, typed prototype consumers, compiled bindings, title, and tracked history. This is a static source audit; it does not inspect a live server, player state, database, editor, or persisted world state. The [canonical dossier](../zone-stories/DARK_FOREST.md) summarizes the authored story and owner questions.

## Source identity and authored story

The active contributor is listed in areas/AREA with range note *750-751 (Arthorn). Its .zon header is #750, reset mode 2, and title Dark Forest. It has 171 rooms (#75001–#75171), 34 mobiles (#75001–#75034), 15 objects (#75001–#75015), and 354 authored WLD exits. There is no local .qst, .shp, or story sidecar. The separate QST/AREA index includes dark_forest, but that index is not a production quest source ([active list](../../../areas/AREA#L269), [zone](../../../areas/zon/dark_forest.zon#L1), [QST index](../../../areas/QST/AREA#L240)).

The descriptions cover several connected forest communities and mysteries: unnatural circular burns and an ever-burning pit share symbols; an isolated oak forms a circle of sunlight; Urd trails lead past an old tree log and a corpse-covered tree; Ogrillon huts surround an old battlefield and a hidden fortress whose elaborate rooms predate its occupants; a Spriggan village reaches a river whose local legend credits Cu’L Torik with ending a war against Orcs called the Black Skulls; and an Ettercap territory fills the woods with webs and prey. A chasm contains a fallen island of forest, and a precarious bridge crosses the roaring river. No local quest file or sidecar assigns these strands a formal completion sequence. The river legend is an in-world account, not verified external chronology.

The river rooms #75125, #75127, and #75128 use sector value 7 (WATER_NOSWIM). Bridge rooms #75165–#75167 use room flag 8192, defined as SINGLE_FILE. The local WLD contains no F fall-chance or C current records. The loader reads those fields separately; the bridge text warns of an unstable crossing, but the source does not quantify fall chance or current ([WATER_NOSWIM sector](../../../src/core/defines.h#L465) and [SINGLE_FILE room flag](../../../src/core/defines.h#L407), [loader](../../../src/world/db.c#L1395), [river room](../../../areas/wld/dark_forest.wld#L2710), [bridge room](../../../areas/wld/dark_forest.wld#L3597)).

## Active WLD graph and reset-state reach

All 354 authored exits resolve in the active world: 352 local exits and two outbound exits. The 171 rooms form one local weak component, and all-exits-available directed reach from the active arrival room #75171 covers all 171 rooms. There are no negative or unresolved local targets.

| Direction | Active WLD edge | Paired edge or status |
| --- | --- | --- |
| Inbound, one-way | Surface #605089 west to dark_forest #75171 | No direct reverse edge to #605089; the area entrance room has two other Surface exits. |
| Inbound and paired | Surface #633650 west to #75171 | dark_forest #75171 east to Surface #633650. |
| Inbound and paired | Surface #634049 north to #75171 | dark_forest #75171 south to Surface #634049. |

The active arrival and outbound edges are grouped at dark_forest #75171. Local exits are at [#75171](../../../areas/wld/dark_forest.wld#L3723); active Surface inbound records are at [#605089](../../../areas/wld/surface.wld#L2206278), [#633650](../../../areas/wld/surface.wld#L2806127), and [#634049](../../../areas/wld/surface.wld#L2814486). The exact outbound records are in cross-area exit register rows [865](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L865) and [866](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L866); the Surface inbound records are rows [1587](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1587), [1660](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1660), and [1661](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1661).

### Source-only inactive Surf approach

An all-95-file scan found one non-overlapping source-only approach in inactive surf.wld. It is not active ingress.

| Inactive source | Exit to active dark_forest | Evidence |
| --- | --- | --- |
| surf.wld #208177 | North to #75171 | [Room and exit](../../../areas/wld/surf.wld#L171641), [target](../../../areas/wld/surf.wld#L171648) |

Preserve it on Surf candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). It raises the curated source-only totals by one to 3,453 overall and 217 for Surf; ud_map2 remains 17. This record does not change the active WLD graph or establish travel through the inactive map.

The .zon has 12 unconditional D resets, all setting paired room doors to closed and none to locked or secret:

| Closed door pair | Reset records |
| --- | --- |
| #75018 south / #75019 north | [zone lines 8 and 12](../../../areas/zon/dark_forest.zon#L8) |
| #75029 east / #75036 west, through the ancient log | [lines 16 and 20](../../../areas/zon/dark_forest.zon#L16) |
| #75032 east / #75033 west, forest paths | [lines 24 and 28](../../../areas/zon/dark_forest.zon#L24) |
| #75039 east / #75048 west, smoke-filled trail | [lines 32 and 36](../../../areas/zon/dark_forest.zon#L32) |
| #75063 east / #75066 west, burnt clearing | [lines 40 and 44](../../../areas/zon/dark_forest.zon#L40) |
| #75092 south / #75093 north, fortress entrance | [lines 48 and 52](../../../areas/zon/dark_forest.zon#L48) |

After those resets, 340 of 352 local directed exits are already open, non-secret, and non-blocked. The strict reset-open graph has four weak components sized 147, 16, 5, and 3. Directed reach from the active arrival #75171 covers 147/171 rooms before opening any reset-closed doors. The six pairs are closed but unlocked in the reset commands, so this strict subset does not establish inaccessibility. It does not model player commands, combat, code callbacks, water or bridge hazards, or safe return.

## Typed inbound and outbound dependencies

Local reset commands use only this zone’s room, mobile, and object prototypes; every local mobile and object definition appears in a reset. A bounded source scan found no outside typed references to a local prototype or room:

| Consumer scan | Result |
| --- | --- |
| Active and unregistered ZON reset roles | No outside reset targets a local room, mobile, or object. |
| QST numeric references | No external QST line matches a local room, mobile, or object VNUM. |
| SHP product entries | No external shop product targets a local object. |
| Story-sidecar numeric VNUMs | No match among 232 current story sidecars. |
| External MOB/OBJ definitions | No duplicate definition of a local mobile or object VNUM. |
| External type-25 portals and type-29 switches | No value target points to a local room. |

There is one important compiled binding. specs.assign.c assigns the archer routine to mobile #75013, an experienced Ogrillon soldier reset in rooms #75081, #75088, and #75094. The routine’s source table uses local archer rooms #75081, #75088, and #75094 with local target rooms #75076, #75075, #75087, #75084, #75093, and #75095 ([assignment](../../../src/specs/specs.assign.c#L326), [routine and room table](../../../src/specs/specs.monster_attacks.c#L226), [reset placements](../../../areas/zon/dark_forest.zon#L111)). A nearby comment calls the binding group darkfall_forest; active Darkfall Forest is a separate zone. Treat the comment’s relationship as unresolved rather than as evidence of a replacement.

## Title, tracked history, and archive disposition

After color-stripping and normalization, the .zon title Dark Forest is unique among all 442 current zone sources. The similar title Darkfall Forest belongs to active zone darkfall (#953); Surface rooms with “dark forest” in their descriptions are geographic text, not a replacement claim. The code comment darkfall_forest is attached to an exact binding for dark_forest mobile #75013, so it remains a naming question.

Followed WLD, MOB, OBJ, and ZON history retains the dark_forest stem to the 2009 area-tree path move. Later same-stem changes include zone data updates and broad field or line-ending normalization. No alternate-stem predecessor, retirement, or whole-zone replacement is established. The title and history findings do not resolve the historical QST/AREA entry without a local QST.

Keep active dark_forest DO NOT MOVE. Preserve its three active Surface arrivals, two reciprocal exits, local VNUMs, and compiled archer behavior. Keep Surf candidate row 97 RETAIN IN PLACE with its one source-only approach. No separate attic candidate row or source move is proposed.

## Builder follow-up

1. Confirm whether the circular burn, matching symbols, ever-burning pit, and sunlight ring around the oak are related.
2. Confirm the intended relationship, if any, among Gar-Thurin’s earlier builders, the Ogrillon encampment, and the Cu’L Torik / Black Skulls legend.
3. Confirm the WATER_NOSWIM assignment for the three river rooms and whether current fields should remain absent.
4. Confirm whether the swinging bridge should remain SINGLE_FILE with no authored fall chance.
5. Confirm that the archer callback is intended for dark_forest mobile #75013 and clarify the darkfall_forest source comment.
6. Confirm why QST/AREA lists dark_forest without a local QST file.
7. Standardize Spriggan/Spriggen and Ogrillon/Orgillon spellings and copy-edit the room and prototype prose.

This audit records static source evidence. It does not qualify actual travel, archer combat, river crossing, bridge safety, or return routes.
