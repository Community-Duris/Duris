# The Lair of the Swamp Troll King

**Source key:** `area:swamp_two` · zone 261 · rooms #26100–#26134<br>
**Normalized `.zon` title:** “The Lair of the Swamp Troll King”

This is the troll lair reached south from the last trail room in Shadow Swamp.
Its source story descends from a quicksand opening, through a featureless maze
of tunnels, to a guarded chamber and the king’s room. The room text also places
a sealed cavern inside that route network, though its authored directions leave
it connected. Keep that prose and movement discrepancy visible for an owner
decision; this map records source data, not a live gameplay result.

## Source inventory and area boundary

The active [`areas/AREA`](../../../areas/AREA#L131) contributor is zone 261.
Its 35 rooms occupy #26100–#26134. The separate historical quest index also
lists `swamp_two` at [`areas/qst/AREA`](../../../areas/qst/AREA#L123), but there
is no `areas/qst/swamp_two.qst`, story sidecar, or local SHP file. No native Q
contract or local shop is present. The local [MOB](../../../areas/mob/swamp_two.mob#L1)
and [OBJ](../../../areas/obj/swamp_two.obj#L1) files each contain one “nothing”
stub prototype, #26100; neither is placed by the local reset table.

The final Shadow Swamp trail room, #26019, warns “Quicksand Beware” and has a
south exit into #26100. The lair’s north exit returns to #26019, so the authored
area boundary is reciprocal. See [Shadow Swamp](SHADOW_SWAMP.md) and its
[boundary record](../../../areas/wld/swamp_one.wld#L331).

## Place and environmental story

| Rooms | Source text and movement | Reading for builders |
| --- | --- | --- |
| #26100 | “Quicksand” describes soft rising ground, bones, recent death, and something moving. North returns to Swamp One #26019; down enters #26101 through the keyed sand door. Mog is reset here. | Keep the warning, the uncertain movement impression, and Mog’s reset as separate facts. The prose does not establish a quicksand movement rule. [Room](../../../areas/wld/swamp_two.wld#L1) · [reset](../../../areas/zon/swamp_two.zon#L34) |
| #26101 | “Beneath the Sand” is a large air pocket called Mog’s sanctuary. Its prose says it feels safe “now that Mog is gone,” while the ZON places Mog immediately outside at #26100. Down enters the keyed tunnel maze. | The wording is static; no dynamic text update tied to Mog’s presence was found. Preserve the discrepancy for review rather than treating either source as runtime state. [Room](../../../areas/wld/swamp_two.wld#L18) |
| #26102–#26131 | Thirty rooms share the title “A Large Dirty Tunnel” and nearly identical prose: the place is a maze where rooms and directions all look the same. The exit network branches and loops through these rooms. | This is the lair’s maze-like middle. Repeated text establishes deliberate sameness at source level, while the graph still has distinct routes. Do not flatten or renumber it based on repeated descriptions. [Maze start](../../../areas/wld/swamp_two.wld#L36) · [maze end](../../../areas/wld/swamp_two.wld#L613) |
| #26106, #26119, #26130–#26131 | #26106 has a north direction to #26134. #26119 has an east direction to that same cavern. Room #26130 has only an east exit to #26116, despite multiple maze directions leading into #26130. Room #26131 leads onward to the chamber at #26132. | These exceptions shape the maze topology. #26130’s one-way funnel and #26134’s apparent sealed-room routes merit owner confirmation. [#26106](../../../areas/wld/swamp_two.wld#L118) · [#26119](../../../areas/wld/swamp_two.wld#L359) · [#26130](../../../areas/wld/swamp_two.wld#L601) · [#26131](../../../areas/wld/swamp_two.wld#L613) |
| #26132–#26133 | “A Large Chamber” describes recent activity and an ornate east door. “The King’s Chambers” contains a throne and freshly gnawed bones. Guards and the troll king are reset here; the bent key is placed with a guard. | The room sequence makes the troll king the endpoint of the maze. Preserve the guard, key, door, and king as separate reset roles; the source does not establish that a player can acquire the key. [Rooms](../../../areas/wld/swamp_two.wld#L632) · [resets](../../../areas/zon/swamp_two.zon#L54) |
| #26134 | “A Large Cavern” says rocks are falling, all exits are covered, and the room is completely sealed. Yet the WLD has inbound directions from #26106 and #26119 and a northwest direction back to #26102. | The static text and authored route do not agree. The northwest exit uses WLD state 4 and has no matching D reset. The loader masks the WLD state to its low two bits, so this is not authored as a door. Ask the builder whether the text, directions, or a separately implemented collapse effect is intended. [Room](../../../areas/wld/swamp_two.wld#L661) · [direction definition](../../../src/core/defines.h#L435) · [loader](../../../src/world/db.c#L1552) |

## Boundary and static graph

The [active WLD](../../../areas/wld/swamp_two.wld) has 35 rooms and 85 authored
directions. All 85 targets resolve: 84 directions remain inside the area and
one leaves from #26100 north to Swamp One #26019. Swamp One #26019 returns
south to #26100. The two boundary records form one reciprocal pair. The
all-inactive-WLD scan found no source-only direction into these rooms and no
room-VNUM overlap that would add a competing map identity.

The local directed graph is one weak component of 35 rooms. With locks ignored,
raw traversal from #26100 reaches all 35 rooms, and each room has a directed
path to the outward boundary. This graph calculation does not establish access
through locked doors, successful movement through quicksand, collapse timing,
or safe return. No negative or unresolved target, fall-chance marker, or current
marker was found in this WLD. The exact active boundary is listed in
[`WORLD_CROSS_AREA_EXIT_REGISTER.csv`](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L402).

## Doors, keys, and reset roles

The WLD declares three keyed, two-way door pairs. The ZON resets set each side
to state 2 (closed and locked). WLD state 3 loads a door as pickproof before
zone resets; state 2 loads a door with the legacy pickable bit. These source
states differ across the first two gates and the king’s chamber gate. They do
not prove that a key spawns, can be taken, or opens the intended door in a live
game. The [loader](../../../src/world/db.c#L1567), [reset handler](../../../src/world/db.c#L7857),
and [door flags](../../../src/core/defines.h#L442) describe the source
interpretation.

| Paired exits | Key prototype and source owner | WLD state | ZON reset |
| --- | --- | ---: | ---: |
| #26100 down ↔ #26101 up | #26005, Swamp One’s dingy key worn by Phyla | 3 | 2 on both sides |
| #26101 down ↔ #26102 up | #26007, Swamp One’s dirty key placed with the corpse in hidden room #26036 | 3 | 2 on both sides |
| #26132 east ↔ #26133 west | #26011, Swamp One’s bent key placed with a guard at #26132 | 2 | 2 on both sides |

The active [reset table](../../../areas/zon/swamp_two.zon#L8) has six D door
resets, 26 M mobile resets, and seven G object resets. All 26 M commands import
Swamp One mobile prototypes #26003–#26006 and #26011. Nineteen commands reset
swamp trolls across the maze; some rooms have separate repeated reset records.
The remaining commands place Mog, swamp troll guards, the king, and elite
guards. The numeric reset caps and duplicate records are authored configuration,
not a measured live population.

The G commands place Swamp One object #26011 with a guard in #26132, then place
these objects on the troll king in #26133: #67238 (Unique’s sleeves of life),
#26012 (platemail of life), #26014 (Nightbringer), #26013 (glowing crystal
sphere), #55185 (White Harbor’s “memory of the Swamp”), and #359 (Heavens’
rune-covered stone). These are typed object references; their owning source
areas remain unchanged. The full command list is in the [ZON](../../../areas/zon/swamp_two.zon#L34).

## Typed dependencies and source identity

| Dependency | Source evidence | Disposition |
| --- | --- | --- |
| Swamp One mobiles #26003–#26006 and #26011 | Twenty-six M resets in [swamp_two.zon](../../../areas/zon/swamp_two.zon#L34) reference Swamp One prototypes; the source definitions are in [swamp_one.mob](../../../areas/mob/swamp_one.mob#L26). | Preserve Swamp One as prototype owner and Swamp Two as reset owner. Do not infer guaranteed spawn or add duplicate placements. |
| Swamp One objects #26011–#26014 | Four G resets import the bent key, platemail, Nightbringer, and sphere from [swamp_one.obj](../../../areas/obj/swamp_one.obj#L138); Caves has one `G I 26013` exchange and a sidecar completion signature giving #26013 and receiving #4031 ([QST](../../../areas/qst/caves_skelenak.qst#L203), [story](../../../areas/story/caves_skelenak.story.json#L423)). | Preserve the key, handler, and sphere identities with Swamp One; Caves owns its optional exchange and Swamp Two owns the king reset. |
| Unique object #67238 | The troll king’s [G reset](../../../areas/zon/swamp_two.zon#L59) references the active prototype in [unique.obj](../../../areas/obj/unique.obj#L554). | Preserve Unique as the prototype owner and Swamp Two as a reset consumer. |
| Heavens object #359 | The king reset references [heavens.obj](../../../areas/obj/heavens.obj#L3725); Smoke also resets #359 on Ehkahk and its sidecar calls it the imported epic stone ([ZON](../../../areas/zon/smoke.zon#L76), [story](../../../areas/story/smoke.story.json#L21)). Maintained code binds #359 to `epic_stone` ([assignment](../../../src/specs/specs.assign.c#L1604)). | Preserve Heavens as prototype owner and each area's reset role; this shared item use is not another prototype owner. Tracked ZON history records the troll-king reset moving from the ground in 2010. |
| White Harbor object #55185 | The king reset references the “memory of the Swamp” in [wh.obj](../../../areas/obj/wh.obj#L2390). White Harbor has one `G I 55185` QST operand and a matching Ambassador story request ([QST](../../../areas/qst/wh.qst#L3149), [story](../../../areas/story/wh.story.json#L1043)). White Harbor also defines a **mobile** #55185 barkeeper ([MOB](../../../areas/mob/wh.mob#L2574)); its `wh.shp` record uses the MOB as keeper, while the producing list excludes OBJ #55185 ([SHP](../../../areas/shp/wh.shp#L491), [loader](../../../src/economy/shop.c#L3374) and [keeper field](../../../src/economy/shop.c#L3489)). | Preserve the object-type dependency and White Harbor contract with their existing owners; do not mistake the same-number MOB/shopkeeper for OBJ stock. |

The bounded scan found no same-type external room, mobile, or object collision;
no external ZON, QST, sidecar, or SHP product reference to the local #26100
stubs; no local shop or SHP product; no type-25 portal into a local room; and
no maintained C++ exact-integer reference to these room, mobile, or object
IDs. These are source-scan results, not proof about untracked content or live
runtime-created items.

After color-code removal, terminator trimming, whitespace and case
normalization, “The Lair of the Swamp Troll King” is unique among 442 current
`.zon` titles and belongs to one of 351 active contributors. Followed WLD and
OBJ histories remain on `swamp_two` through 2013-11-13; MOB and ZON histories
remain on that stem through 2015-10-13, and tracked history reaches the
2009-08-12 area-tree move. No alternate-stem rename, explicit predecessor, or
whole-zone replacement was established in those paths. The absent local QST,
SHP, and story sidecar have no tracked history to follow. Broader command-target
alias ownership remains a separate question. See the [audit supplement](../quest-story-followups/SWAMP_TWO_AUDIT_REVIEW.md).

**Disposition: DO NOT MOVE.** Preserve active area membership, the reciprocal
Swamp One boundary, all maze routes and door/key references, Swamp Two’s typed
reset ownership, and the incoming Unique, Heavens, and White Harbor object
dependencies. This source dossier does not qualify gameplay behavior or clear
any migration.
