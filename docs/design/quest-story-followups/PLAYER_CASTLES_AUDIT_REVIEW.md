# Player Castles: connectivity, dependencies, history, and builder decisions

Reviewed October 10, 2026 for active `player_castles`, zone 110. The source
dossier is [PLAYER_CASTLES.md](../zone-stories/PLAYER_CASTLES.md).

## Active graph and directed reachability

The WLD has three rooms and three authored exits. Two exits resolve within
the loaded area and connect #11000 and #11001 in both directions. The third
is #11000 D2/south to #-1, a negative target that does not resolve to a loaded
room. The authored door flag and key values on the two resolving exits are
zero. Room #11005 has no WLD exits. The raw graph therefore has two weak
components of sizes two and one; #11000 and #11001 reach one another, while
#11005 has no graph route. No active WLD boundary enters or leaves this area.
These are source-graph facts; they do not establish live availability or
player access.

The all-95 inactive-WLD sweep parsed without issues and found no source-room
VNUM overlap with #11000–#11005 and no inactive exit into those rooms. No
source-only WLD approach is added, and active-inbound candidate counts and
curated source-only WLD totals remain unchanged.

| WLD record | Source evidence | Classification |
| --- | --- | --- |
| #11000 D0/north → #11001; #11001 D2/south → #11000 | [Player Castles WLD](../../../areas/wld/player_castles.wld#L1) | Reciprocal two-room component. |
| #11000 D2/south → #-1 | [Player Castles WLD](../../../areas/wld/player_castles.wld#L12) | Authored negative destination; the source does not explain its intended handling. |
| Room #11005 | [Player Castles WLD](../../../areas/wld/player_castles.wld#L28) | Isolated shop-room prototype with no WLD exits. |

## Code and object routes

These bindings sit outside the WLD edge count. They show code paths and
prototype ownership, not that a route or service is currently available.

| Binding | Source evidence | Classification |
| --- | --- | --- |
| #11007 and #11008 house/guild doors | [Door prototypes](../../../areas/obj/player_castles.obj#L57), [`cast_teleport` handling](../../../src/magic/spell_travel.c#L1115), and [`teleport_to`](../../../src/magic/spell_travel.c#L714) | Code recognizes outside and inside door objects and passes a destination room to the teleport helper. The local prototypes do not establish a live destination. |
| #11008 guild window | [Assignment](../../../src/specs/specs.assign.c#L1854) and [handler](../../../src/specs/specs.player_castles.c#L17) | A `look window` callback temporarily visits the room in the object value and returns the character to the original room. Local reset data does not prove that the value is initialized for a live hall. |
| #11013 guild home | [Assignment](../../../src/specs/specs.assign.c#L1855) and [handler](../../../src/specs/specs.player_castles.c#L41) | Saying “home” sets the character’s birthplace to the current room; it is not a WLD exit. |
| #11010 lock and #11011 key templates | [Prototype constants](../../../src/world/vnum.obj.h#L102), [lock command](../../../src/cmd/actnew.c#L3108), and [key use](../../../src/cmd/actmove.c#L3032) | Shared construction objects used by the lock and key commands; active Tharnadia stock lists both. |
| #11002 and #11051–#11065 magic mouths | [Assignments](../../../src/specs/specs.assign.c#L1624) | Compiled `magic_mouth` object callbacks for the numbered prototypes. |
| #11002–#11004 golem mobiles | [Mobile prototypes](../../../areas/mob/player_castles.mob#L23) and [lure checks](../../../src/cmd/actoff.c#L2562) | `actoff.c` compares each mobile’s `virtual_number` and exempts these IDs from its lure handling. `actmove.c` also has a “guild golems” check, but it compares `index_data.number`, which the struct defines as the existing-unit count; do not treat those comparisons as a typed VNUM binding without owner review. |
| #11005 merchant mobile | [Merchant reset and SHP](../../../areas/zon/player_castles.zon#L11), [shopkeeper exception](../../../src/economy/shop.c#L2407) | Shop code exempts keeper VNUM #11005 from the roaming-keeper cash limit. This source reference does not prove the local shop is active. |

The broad numeric scan also found #11102–#11114 in ship-helper arrays. Those
arrays are typed as mobile VNUMs, while Player Castles defines those numbers
as board objects; no mobile prototype for those IDs is present in the source
tree. The `realms_master_shout` helper procedure has no assignment in the
current spec-assignment source. These are not object dependencies. The room
checks for #11008 in `specs.guards.c` and the ship-room assignments for
#11101–#11123 in `specs.assign.c` are separate room-namespace references; the
ship assignments are inside `#if 0`.

## Typed dependencies and external consumers

| Reference | Source evidence | Classification |
| --- | --- | --- |
| Active CBL Hall reset rows | [CBL Hall ZON lines 11–31](../../../areas/zon/cbl_hall.zon#L11) | Six Player Castles object prototypes (#11013, #11054, #11055, #11062, #11104, and #11105) are reset into active CBL Hall rooms by 16 records. The “not in this .obj file” comments describe the split prototype owner; they do not make these records source-only. |
| Active Heavens board reset | [Heavens ZON](../../../areas/zon/heavens.zon#L188) and [board prototype](../../../areas/obj/player_castles.obj#L393) | Heavens resets Player Council board #11118 in room #1201. |
| Active Surface sailor reset | [Surface ZON](../../../areas/zon/surface.zon#L1668) and [mobile prototype](../../../areas/mob/player_castles.mob#L79) | Surface resets mobile #11006 in room #635260. |
| Active Tharnadia shop products | [Tharnadia SHP](../../../areas/shp/tharnadia.shp#L365) | The shop lists Player Castles lock and key templates #11010 and #11011. |
| Local merchant’s imported shop stock | [Player Castles ZON](../../../areas/zon/player_castles.zon#L11) and [SHP](../../../areas/shp/player_castles.shp#L1) | Twelve object VNUMs appear in the keeper load and shop list. Eleven have a prototype in an active object file; #6089 has no checked-in `.obj` prototype. Current active owners include Minizones #5725, Heavens #364/#412/#53/#203/#204/#399, Verz1 #3050, Kimordril #95540/#95547, and Limbo #5. Inactive `dalvik`, `hostel`, and `mavulsk` sources also define #53 or #5; they do not change the current active owners. |
| Source-only ZON reset imports | [Existing attic candidate rows](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv) | Forty-one unregistered ZON files contain 174 object reset records for Player Castles templates. Each exact source line and VNUM is recorded on that file’s existing candidate row. `surf.zon` separately resets mobile #11006 in room #258421. These references are not active resets. |
| Source-only shop products | [Existing attic candidate rows](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv) | `realms_master.shp` lists board objects #11121 and #11124; `tharnadia1.shp` repeats the #11010/#11011 products also present in active `tharnadia.shp`. |
| Exact external QST/story contracts | Bounded exact-head QST scan and numeric story-sidecar scan | No outside QST item command or outside story-sidecar numeric reference to a Player Castles prototype was found. |
| Prototype and portal ownership | External MOB/OBJ definition scan; 869 external type-25 declarations | No outside duplicate mobile/object definition of a Player Castles prototype and no external type-25 target into its three room VNUMs were found. |

Preserve the distinction between active imports and the source-only candidate
references. The candidate rows remain RETAIN; no candidate is created or
cleared by this review.

## Title, followed history, and retention

After color-code removal and whitespace normalization, “The Adventurers
Guildhalls” occurs once among the 442 current `.zon` titles and is unique in
the active set. Followed WLD, MOB, OBJ, ZON, and SHP histories remain on the
`player_castles` stem from the 2009 area-tree move. Later same-stem edits are
recorded through 2013 for WLD/OBJ, 2015 for ZON, 2014 for SHP, and 2026 for
MOB. No alternate-stem rename or whole-zone replacement is established by
these paths. Active `guildhalls` has a separately documented `castle_rooms`
predecessor and several copied object prototypes; that does not establish
that Player Castles was superseded. See [Checkpoint 223](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-223-guildhalls-title-and-source-history).

Keep active `player_castles` **DO NOT MOVE**. Preserve its object and mobile
VNUMs, callback assignments, local shop references, active CBL Hall/Heavens/
Surface/Tharnadia consumers, and the source-only reset and shop records on
their existing candidate rows. No route, reset, gameplay, or migration change
is proposed.

## Checkpoint 325: builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PC-01 | Is `player_castles` intended as a shared guildhall support catalog, a playable area, or both? | Preserve the contributor and describe its ownership and intended player-facing scope before adding story or routes. |
| PC-02 | What should the foyer at #11000 tell an arriving player about this place? | Keep the authored foyer text until the owner defines whether the room is a real entrance, a test room, or a template. |
| PC-03 | Is #11001, “Fafhrd’s Castle of Testing,” intended to remain a live room or only a builder fixture? | Preserve its VNUM and builder text until its purpose and visibility are confirmed. |
| PC-04 | What is the intended meaning of #11000 D2/south to #-1? | Confirm the engine handling and intended destination with the area owner before changing or removing the negative target. |
| PC-05 | Should the isolated shop room #11005 have an ordinary WLD entrance or return route? | Keep its current graph as authored and request an access decision before adding edges. |
| PC-06 | Are the reset mode 0 header and the local door, merchant, and inventory reset records meant to be used together? | Preserve all source records; qualify reset-mode behavior separately before revising the zone file. |
| PC-07 | Is the wooden door #11008 in #11001 intended to provide a guild window, a door, or both? | Keep its local assignment and reset; confirm its configured value and intended use before editing either source. |
| PC-08 | Should #11007 and #11008 remain the shared outside/inside house and guild door types recognized by `spell_travel.c`? | Retain both object VNUMs and their separate teleport flags until the door owner reviews dynamic destination setup. |
| PC-09 | Is shimmering portal #11001 intended for local use even though the local ZON does not reset it? | Preserve the prototype and its spell handling until its creator confirms whether it is a template or active item. |
| PC-10 | Which of the ten mobile prototypes should be placed by this zone, a dynamic system, or another area? | Keep the mobile definitions and exact VNUM ownership; document placement separately from prototype existence. |
| PC-11 | Are golems #11002–#11004 intended to resist lure and movement effects? | Preserve the explicit `actoff.c` mobile-VNUM checks and ask the code owner to resolve the `actmove.c` count-field comparison separately. |
| PC-12 | Do the magic-mouth objects #11002 and #11051–#11065 represent the numbered guildhall speech slots implied by their values? | Keep all 16 callback bindings and numeric values until the message mapping is confirmed. |
| PC-13 | Is bonding altar #11013 the intended guildhome object for guildhall rooms that reset it? | Preserve its handler and external reset VNUMs; confirm the “home” behavior and placement with the guildhall owner. |
| PC-14 | Which of the 30 board prototypes #11101–#11130 are still assigned to halls, and are #11118/#11130 the intended Player Council boards? | Retain the full object range and current Heavens/Cbl Hall references until the board map is confirmed. |
| PC-15 | Are CBL Hall’s 16 resets of Player Castles prototypes #11013, #11054, #11055, #11062, #11104, and #11105 intended at their current rooms? | Preserve the active cross-area resets and review their placement with both area owners. |
| PC-16 | Is Heavens room #1201 the intended placement for Player Council meeting board #11118? | Keep the active Heavens reset and board owner intact until the council-board owner confirms placement. |
| PC-17 | Is Surface room #635260 the intended location for the Dalvik mobile #11006, and should the old `surf.zon` placement remain source-only? | Preserve the active reset and the separate `surf` candidate reference; do not treat the old file as active stock. |
| PC-18 | Should active Tharnadia continue selling lock/key templates #11010/#11011 for the construction commands that use them? | Preserve the stock products and template constants until their crafting and shop owners confirm availability. |
| PC-19 | Should the local merchant keep its twelve listed products when #6089 has no checked-in object prototype, and should guild keeper #11005 keep its special cash handling? | Preserve the SHP/ZON records and accounting exemption; ask the shop owner to resolve #6089 and intended keeper behavior before a stock or code change. |
| PC-20 | Should the 41 unregistered guildhall ZON importers, `surf.zon`, `realms_master.shp`, and `tharnadia1.shp` remain attached to their current attic candidates? | Keep all existing rows RETAIN, preserve each typed reference, and require full source/tooling/persisted-reference review before any separate migration decision. |

## Checkpoint 514 supplement: Nah reset imports

Unregistered nah.zon has five O reset rows importing Player Castles objects:
#11061 twice, #11013 once, and #11111 twice, placed in Nah rooms #30100,
#30105, #30110, and #30112. These are candidate reset references, not active
Player Castles consumers. Preserve them on the Nah source while the room
owner and intended reset package are resolved; see the
[NAH audit](NAH_AUDIT_REVIEW.md) and
[typed dependency review](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers).