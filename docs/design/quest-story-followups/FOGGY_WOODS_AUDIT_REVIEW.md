# The Fog Enshrouded Wood repository-wide source audit

Checkpoint 211 maps active `foggy_woods` against its local WLD, MOB, OBJ, and ZON sources; active and unregistered WLD boundaries; followed file history; and typed references elsewhere in the repository. This is a static source audit. It does not inspect a live server, persisted state, player access, database, or actual gameplay. The [source dossier](../zone-stories/FOGGY_WOODS.md) summarizes the setting and builder questions.

## Source identity and authored content

The active area list calls the contributor `foggy_woods` with note `*900-902`; its zone header is #900, titled “The Fog Enshrouded Wood,” and its last room is #90220. The local WLD has 213 rooms (#90000–#90220), the MOB has 32 prototypes (#90000–#90031), and the OBJ has 33 (#90000–#90032). The QST area index lists the area, but there is no local QST, SHP, or story sidecar. See the [area list](../../../areas/AREA#L318), [zone header](../../../areas/zon/foggy_woods.zon#L1), [QST area index](../../../areas/qst/AREA#L283), and [coverage row 100](ZONE_COVERAGE_REGISTER.csv#L100).

The WLD’s authored sequence moves from a cold, animal-haunted forest through an abandoned logging camp and into deeper fog, a trapper’s hut, giant territory, and a cave network. The camp rooms describe wrecked wagons and barracks, slashed bunks, human remains, and a dead dog chained at a cabin. A wolf claws at a new grave. The source does not identify the attacker, cause, or chronology. The main forest also includes the ashen fog, giant tracks, a windfall breach in the log wall, and a trapper who is reset into a hut described as occupied ([camp](../../../areas/wld/foggy_woods.wld#L238), [grave](../../../areas/wld/foggy_woods.wld#L1051), [ashen fog](../../../areas/wld/foggy_woods.wld#L3183), [trapper’s hut](../../../areas/wld/foggy_woods.wld#L4293)).

Fog giants occupy several caverns around a warm, underground pond. The pond’s high opening points toward the snowy mountains; its west exit is the reciprocal Underworld3 boundary. Rat tunnels lead to the Pathway of Spears and the Beastmaster’s grove, described with rune-marked stones, ash spears, human heads, incense, and skulls. The Beastmaster is reset with a rune book and ashen spear and guarded by a spirit wolf. This establishes rune magic as a theme, but no local QST, native Q/QA contract, or sidecar defines a quest sequence ([pond](../../../areas/wld/foggy_woods.wld#L5552), [rat tunnels](../../../areas/wld/foggy_woods.wld#L5597), [Pathway of Spears](../../../areas/wld/foggy_woods.wld#L5674), [Beastmaster mobile](../../../areas/mob/foggy_woods.mob#L78), [Beastmaster reset](../../../areas/zon/foggy_woods.zon#L192)).

The local ZON contains 136 non-`S` commands: 20 D, 18 O, 6 P, 76 M, 7 E, and 9 G. All 32 mobile prototypes are used by reset commands; 32 of 33 object prototypes are used. Object #90025, the water-logged rune note, has no local reset reference. The store prose says goods are for sale and mobile #90022 is called Ulon the shopkeeper, but there is no local SHP file to define a shop inventory ([resets](../../../areas/zon/foggy_woods.zon#L8), [Ulon](../../../areas/mob/foggy_woods.mob#L329), [unplaced scroll](../../../areas/obj/foggy_woods.obj#L268)).

## Active WLD boundaries and unresolved exits

The local WLD records 607 exits: 592 local, six outbound to active areas, one positive unresolved target, and eight negative `-1` targets. Of the 607, 598 resolve against the loader’s active room inventory. The local exit graph is one weak component of 213 rooms. There are six inbound active edges, six outbound active edges, and six reciprocal pairs: five with Surface and one with Underworld3. The [cross-area pair summary rows 145–146](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L145) and [exit-register rows 423 and 1108–1111](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L423) retain the exact pair records.

| Active neighbor | Entry targets in `foggy_woods` | Local returns |
| --- | --- | --- |
| Surface | #90000 (two arrivals), #90162, #90185 (two arrivals) | #90000 east/south, #90162 west, #90185 north/east |
| Underworld3 | #90212 | #90212 west |

Room #90046 also exits west to #143836, present only in unregistered `Duris3.wld`. The normal active world list omits that destination and the registered unresolved-exit inventory says the loader prunes the edge. This is already represented by the Duris3 candidate row; keep that row unchanged and retain its source-only classification. See [the unresolved edge](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L266), [Duris3 room #143836](../../../areas/wld/Duris3.wld#L920557), and [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

The eight negative targets are #90057 south, #90058 north, #90095 down, #90105 west, #90183 south, #90193 south, and #90219 north and south. They are retained in the [negative/unresolved exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L267). A full scan of all 95 unregistered WLD files found no room-number overlap or source-only approach into #90000–#90220; the active Duris3 edge above is an outbound edge from `foggy_woods`, not inactive ingress.

## Door states and static reach

The zone has 20 D commands across ten local door pairs. Their reset-state values are 1 (nine commands: closed), 2 (four: closed and locked), 5 (five: closed and secret), 6 (one: closed, locked, and secret), and 14 (one: closed, locked, secret, and blocked). Door reset bit handling is in [db.c](../../../src/world/db.c#L7857). The general-store doorway and cave passages matter to the local route: the secret stair links #90013 with #90030/#90031; the cave pair #90207 south / #90208 north carries states 14 and 6; and the secret downward door from #90208 leads to the rat tunnels.

The local key and switch records are linked by exact source values. The zone equips object #90024 on shaman #90014 in #90205; both sides of the #90207/#90208 door pair name that key. Switch object #90010 is placed in #90210. Its ITEM_SWITCH values select CMD_PUSH, room #90207, south, and clearing the blocked bit; the object’s switch handler documents that operation in [specs.object.c](../../../src/specs/specs.object.c#L302), and CMD_PUSH is declared in [interp.h](../../../src/cmd/interp.h#L305). Clearing that bit does not open, unlock, or reveal the door.

With all exits treated as available, each of the six active inbound targets reaches the local 213-room component. Applying unconditional D resets and treating closed, locked, secret, or blocked exits as unavailable leaves strict static directed reach at 193/213 from every active arrival. The same 20 rooms are excluded: #90013, #90018–#90020, #90022, #90030–#90031, #90081–#90084, #90161, and #90208, #90214–#90220. This calculation does not model a character opening or picking doors, revealing secret exits, pushing the switch, or using non-WLD mechanics. No local fall-chance or current fields are authored.

## Typed dependencies and source-only references

Exact role-aware scans found no external duplicate definition of a local MOB or OBJ VNUM, no external WLD door key using a local object, and no outside ITEM_TELEPORT/ITEM_SWITCH target into local rooms. The exact authored outside references are:

- Active `icecrag.qst` requires two copies of object #90017, the local bottle of cheap white wine; its story sidecar repeats those item inputs ([QST](../../../areas/qst/icecrag.qst#L320), [object](../../../areas/obj/foggy_woods.obj#L190), [sidecar](../../../areas/story/icecrag.story.json#L507)).
- Active `alatorin.qst` grants object #90018, the local silver-studded white dragon scalemail, as an armor-service result. The Alatorin story sidecar records the same output ([QST](../../../areas/qst/alatorin.qst#L5724), [object](../../../areas/obj/foggy_woods.obj#L198), [sidecar](../../../areas/story/alatorin.story.json#L2959)).
- Active `verspin.zon` resets local object #90026, a stash of gold coins, into room #28174 ([reset](../../../areas/zon/verspin.zon#L228), [object](../../../areas/obj/foggy_woods.obj#L294)).
- Unregistered `nexus56.zon` contains inventory resets for local objects #90000 and #90016, and unregistered `nexus56.shp` lists the same VNUMs as shop products. `nexus56` is absent from `areas/AREA`; these are source-only references, not active full-area consumers ([resets](../../../areas/zon/nexus56.zon#L1377), [shop products](../../../areas/shp/nexus56.shp#L500), [local rune book](../../../areas/obj/foggy_woods.obj#L1), [local ale](../../../areas/obj/foggy_woods.obj#L182)).

Other raw numeric intersections were checked by field type: for example, several QST `R E 90000` entries mean experience, not object VNUMs. The local QST/AREA listing remains without a local QST file. Ulon’s shop text therefore has no local SHP inventory behind it. Object #90025 has no local reset or external typed item consumer.

## Title, source history, and disposition

After color stripping and normalization, “The Fog Enshrouded Wood” is unique among 442 current `.zon` titles. Local WLD room #90046 uses the nearby phrase “The Fog Enshrouded Forest” for an outbound route, not a separate contributor. Followed WLD, MOB, OBJ, and ZON histories stay on the `foggy_woods` stem from the 2009 area-tree move through later same-stem edits; their latest file history runs through 2013 (WLD), 2015 (MOB), and 2017 (OBJ and ZON). No alternate-stem predecessor, retirement, or whole-zone replacement is established.

Keep active `foggy_woods` **DO NOT MOVE**. Preserve its active Surface and Underworld3 boundaries, reset states, rune and key objects, and external item consumers. Keep Duris3 candidate row [36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36) **RETAIN IN PLACE**; do not add another candidate row for the active-to-Duris3 outbound. No curated source-only count changes. See the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-211-fog-enshrouded-wood-title-and-source-history), [coverage row 100](ZONE_COVERAGE_REGISTER.csv#L100), and [source dossier](../zone-stories/FOGGY_WOODS.md).

## Builder follow-up

1. Identify the logging-camp attack and Ulon’s survival, if these are meant to form an explained event.
2. Confirm whether Ulon should have an actual shop inventory.
3. Decide whether the unplaced water-logged rune note is intentional.
4. Confirm whether the Duris3-only target #143836 is meant to have an active destination.
5. Confirm that Icecrag’s wine input, Alatorin’s scalemail output, Verspin’s coin placement, and the source-only Nexus56 references should continue using these exact object VNUMs.
6. Qualify player access through the closed, locked, secret, and blocked routes; static graph reach does not test actual movement.

## Checkpoint 802: typed census and compiled bindings

The local zone still has 136 non-S commands and 122 typed mobile/object
operands: 76 M mobile slots, seven E, 18 O, nine G, and twelve object slots
across six P commands. All typed prototypes and explicit reset-room operands
resolve locally. All 32 mobile prototypes and 32 of 33 object prototypes are
reset-used; object #90025 has no local reset or outside typed item consumer.
No outside active or source-only reset points to a Foggy prototype or room,
apart from the source-only Nexus56 object imports listed below.
[Local reset source](../../../areas/zon/foggy_woods.zon#L8),
[unplaced note](../../../areas/obj/foggy_woods.obj#L268).

Active consumers are Icecrag's two `G I` inputs for local wine #90017,
Alatorin's `R I` armor-service output #90018, and Verspin's `O` reset of
local gold #90026 into room #28174. Icecrag's story sidecar records #90017 in
both `item_vnums` and `completion_key`; Alatorin's completion keys record
#90018 as the result. There is no outside QST giver header, active SHP keeper
or product reference, or external WLD door-key field for a Foggy object.
[Icecrag inputs](../../../areas/qst/icecrag.qst#L320),
[Icecrag sidecar](../../../areas/story/icecrag.story.json#L507),
[Alatorin output](../../../areas/qst/alatorin.qst#L5724),
[Alatorin sidecar](../../../areas/story/alatorin.story.json#L2959),
[Verspin reset](../../../areas/zon/verspin.zon#L228).

Unregistered `nexus56.zon` has G resets for Foggy objects #90000 and #90016;
unregistered `nexus56.shp` lists those same objects as products. Nexus56 is
absent from `areas/AREA`, so these remain source-only references. No same-type
duplicate prototype was found. Foggy has no local QST, SHP, or story sidecar.
[Nexus56 resets](../../../areas/zon/nexus56.zon#L1377),
[Nexus56 shop products](../../../areas/shp/nexus56.shp#L500).

The local objects contain no type-25 portal and one type-29 switch, #90010,
which names local room #90207. A full scan of all 872 type-25 and 380 type-29
declarations found no external target into a Foggy room. The existing
switch's local exit configuration and the six door pairs remain as described
above; no item target adds an authored WLD edge.
[Switch prototype](../../../areas/obj/foggy_woods.obj#L125),
[switch implementation](../../../src/specs/specs.object.c#L334).

Compiled source binds object #90004 to `fw_ruby_monocle` and object #90010 to
`item_switch`; rooms #90107, #90112, and #90114 use `fw_warning_room`. The
monocle code checks local room numbers #90124–#90142 and, when its zone has
just reset, relocates the object to a random room in that range. The switch
handler can clear the blocked bit on the configured #90207 south exit. These
behaviors do not add WLD exits. Other numeric matches in the exact-number
source scan are constants, map offsets, or comments, not typed Foggy VNUMs.
[monocle object binding](../../../src/specs/specs.assign.c#L2065),
[switch object binding](../../../src/specs/specs.assign.c#L2066),
[room #90107 binding](../../../src/specs/specs.assign.c#L2553),
[room #90112 binding](../../../src/specs/specs.assign.c#L2554),
[room #90114 binding](../../../src/specs/specs.assign.c#L2555),
[monocle room-range check](../../../src/specs/specs.foggy_woods.c#L32),
[random room selection](../../../src/specs/specs.foggy_woods.c#L35),
[room callback](../../../src/specs/specs.room.c#L1346),
[switch handler](../../../src/specs/specs.object.c#L389).

Keep active `foggy_woods` DO NOT MOVE and preserve the listed active item
consumers, source-only Nexus56 references, local callbacks, switch/key data,
and existing Duris3 classification. No area source, WLD total, candidate
disposition, or owner question changed. Live reset state, route access, shop
availability, and persisted-player behavior remain unqualified.
