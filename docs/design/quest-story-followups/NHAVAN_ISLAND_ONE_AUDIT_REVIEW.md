# Nhavan Island: graph, dependencies, history, and retention

Checkpoint 313 · active area [`nhavan_island_one`](../../../areas/AREA#L105), zone 203 · source title in [`nhavan_island_one.zon`](../../../areas/zon/nhavan_island_one.zon#L1) · baseline source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`.

This supplement adds repository-wide graph, inactive-WLD, typed-dependency, title/history, and candidate-retention findings to the [story dossier](../zone-stories/NHAVAN_ISLAND_ONE.md). All conclusions are from source declarations and static code inspection; the server and gameplay were not run.

## Directed WLD graph and active routes

The local WLD contains 23 rooms and 53 authored exits. Fifty-two resolve to rooms inside #20300–#20322; the sole positive unresolved target is east from #20301 to #148782. The resolved local graph forms one directed component of all 23 rooms: from #20300 all 23 are reachable and all 23 can return. The six `D` reset rows in [`nhavan_island_one.zon`](../../../areas/zon/nhavan_island_one.zon#L8) close three reciprocal gate pairs: #20311 east ↔ #20312 west, #20316 south ↔ #20320 north, and #20320 east ↔ #20321 west. Reach/return counts describe authored local exits and do not test open-door state in a running game. The coverage census records no fall or current markers.

| Source route | Target | Current classification |
| --- | --- | --- |
| #20301 east | #148782 | Positive unresolved exit. The target occurs only as `The Ocean` in inactive [`Duris3.wld`](../../../areas/wld/Duris3.wld#L1024423); full-world loading prunes it. The exact row is in the [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L47). |
| Other active-area WLDs | #20300–#20322 | No active inbound exit record was found. There is no active cross-zone boundary or reciprocal pair for this area. |

The all-95 inactive-WLD sweep parsed 527,791 room records and 2,080,773 direction records with no parse issues. It found no inactive exit into #20300–#20322 and no inactive room using a VNUM in that range. No source-only approach is added to the migration register. The active exit to #148782 is already one of the 26 active WLD references listed for the existing [`Duris3` candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36); its presence is the reason that row must retain this reference until an owner selects a loaded destination or explicitly retires the edge.

## Typed dependency closure

The local source inventory has 23 rooms, seven mobile prototypes, nine object prototypes, no local QST contract/giver, and one physical shop record. The QST manifest includes zone 203, but the corresponding `.qst` file is absent. There is no local story sidecar. The shop source [`nhavan_island_one.shp`](../../../areas/shp/nhavan_island_one.shp#L1) defines keeper #20303 and products #20302–#20305. Since `areas/world.shp` is absent, [`open_shop_stream`](../../../src/economy/shop.c#L3266) falls back to concatenating the files under `areas/shp/`; the per-area record is therefore in the inspected fallback input.

Reset orders in [`nhavan_island_one.zon`](../../../areas/zon/nhavan_island_one.zon#L34) use only local prototypes: villagers with shivs, Pantu with a boning blade and the four fish products, the puck, wisps, the witch doctor with mask and rattle plus a scarlet gourd, and the livestock. No local object is a type-25 room teleporter or another typed room destination. The complete external exact-ID scan across QST, SHP, story, ZON, MOB, and OBJ declarations found no typed reference into the island, no external QST/SHP/story consumer, and no duplicate external mobile/object prototype. Numeric matches in a few outside OBJ files were in cost fields, not VNUM-typed fields. No direct VNUM destination or callback binding for a local prototype was found. One active Menden fisherman callback does carry a narrative reference: mobile #88805 is assigned `menden_fisherman`, whose ambient chatter includes one case about bringing turtle legs back from Nhavan Island. The line does not inspect or deliver object #20304 and does not encode a route; see [`specs.assign.c`](../../../src/specs/specs.assign.c#L1088) and [`specs.menden.c`](../../../src/specs/specs.menden.c#L237). This closes source-declared dependencies; it does not establish the absence of every possible runtime-generated or non-VNUM route.

## Title, history, and replacement finding

After removing color codes, `Nhavan Island` appears once among the 442 current `.zon` files. Other current island titles include Moonshae Island and two `CBL Guildhall Monster Island` copies (`cbl_outpost2`, `Monsteri`). The latter pair is already traced to each other and active CBL Hall rooms #29820–#29824 in [checkpoint 199](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-199-cbl-hall-and-monster-island-copies); their #1000–#1004 rooms and reset content have no matching Nhavan identity. Followed WLD, MOB, OBJ, ZON, and SHP histories remain on the `nhavan_island_one` stem. The tracked history includes the 2009 area-tree move and later format/maintenance changes; the mobile file also has a 2026 maintenance entry. No alternate-stem predecessor or explicit whole-zone replacement was found in the reviewed history or replacement index. The inactive `Duris3` room named `The Ocean` is a map target, not evidence that it owns or replaces Nhavan Island.

## Safe retention plan

Keep `nhavan_island_one` **DO NOT MOVE**. It is an un-commented contributor in `areas/AREA`, and its local story, shop, reset population, and room graph are current source. No attic candidate row is warranted for the active island. Keep `Duris3` **RETAIN IN PLACE**. Its candidate row remains at 26 active exit references; this checkpoint adds no inbound source-only approach and does not change the curated source-only total of 4,247. Preserve the #20301 east → #148782 reference as an unresolved builder decision. It is not an active sea route and is not proof of an active route into the island.

## Twenty direct builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| NHV-01 | Which active-world route is intended to bring players to the strip at #20300? | Name a loaded source room and reciprocal destination before changing the WLD; no active inbound route is currently authored. |
| NHV-02 | Should #20301 east continue to target #148782, or point to a current loaded sea room? | Keep the current target recorded as unresolved until the intended destination is selected; do not revive `Duris3` by assumption. |
| NHV-03 | If players should reach the sea from the island, where should the return route land and how should it be entered? | Specify both directions and their arrival rooms together, then review them as a separate travel change. |
| NHV-04 | Are the decorated canoes and partly submerged craft intended to provide travel, or are they scenery? | Treat them as scenery under current source; any boat trip needs an explicit loaded route and safe return. |
| NHV-05 | Is the uncrossable glass slope the final volcano boundary, or is a higher area/event intended? | Keep the climb blocked unless a destination, access condition, and return are designed. |
| NHV-06 | Menden's active fisherman #88805 has an ambient line about bringing turtle legs back from Nhavan Island. Is this intended as cross-zone lore or only flavor? | Preserve the line as flavor: it has no item check, delivery, or route; do not infer a boat connection or objective from it. |
| NHV-07 | Is Urtu the captain who brought the Endu to this island? | Keep the epitaph's claim; add a connection to the ship only if the builder confirms it. |
| NHV-08 | What does maintaining the wreck require, and what does the promised plenty mean? | Decide whether this remains background belief or receives a specific repeatable player action and outcome. |
| NHV-09 | Which god does the village idol represent, if any? | Do not name a deity from its appearance alone; add a name only with owner confirmation. |
| NHV-10 | Is the witch doctor an active religious guide, a quest giver, or only a reset NPC? | Preserve current role as environmental character until a concrete interaction is authored. |
| NHV-11 | Are the mask, rattle, and gourd meant to work together in a ceremony? | Keep their reset placement as props/equipment; a ritual needs an explicit action and visible result. |
| NHV-12 | What do the gourd's cross, figure, and shield-bearing figure signify? | Confirm whether the signs are a clue, belief, or decorative detail before building a puzzle around them. |
| NHV-13 | What causes the evil fog and haunted-woods manifestations? | Leave the cause unstated unless a source-supported story or encounter is chosen. |
| NHV-14 | Does the sign warn about a real effect or only set tone? | Keep it as a warning in current source; if it denotes a hazard, state its trigger and escape route. |
| NHV-15 | Should the fog make navigation mechanically disorienting? | Current resolved exits form a returnable loop; add no movement randomness without a clear map and return plan. |
| NHV-16 | Is the fiendish puck a native threat, a fog creature, or unrelated? | Preserve the current hostile reset without inventing a faction relationship. |
| NHV-17 | Are the will-o'-the-wisps meant to lead, mislead, or simply threaten travelers? | Keep their current placement; confirm an intended route clue before tying their movement to travel. |
| NHV-18 | Should the idol, village, and pen gates reset closed, and are all three meant to be passable by opening them? | Preserve the six reset rows and verify intended door behavior before any lock/key change. |
| NHV-19 | Are Pantu's four local products, sale terms, and hours the intended service for visitors? | Keep the shop data and correct the inventory to one shop; make economy changes separately from story interpretation. |
| NHV-20 | Should the island stay environmental lore, or should ship upkeep or the fog become a native quest, and what encounter level is intended? | If no objective is desired, document the hooks as lore. If one is, specify an attributable action, accepted proof, reward, caps/replay rules, and encounter range before implementation. |

## Scope and validation

This checkpoint changes documentation and the two inventory/review rows only. It does not change area, code, quest, route, or live world source. The local shop count was corrected from zero to one after confirming the `.shp` block and the loader fallback. The authored local graph and unresolved-target counts match the existing census. No builds, tests, database work, migrations, server boot, or gameplay were run. Local links, CSV structure, question count, restricted-directory status, and whitespace are checked after the documentation update.
