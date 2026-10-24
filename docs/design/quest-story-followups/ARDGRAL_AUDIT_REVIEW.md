# Ard'gral repository-wide audit review

This supplement records the story, graph, title/history, and typed-reference
review for active area `ardgral`. It accompanies the
[Ard'gral source story dossier](../zone-stories/ARDGRAL.md). Findings describe
repository source; they do not establish current runtime availability or
qualify actual player traversal. No area or native gameplay source was
changed.

## Active source and story inventory

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`, zone 304 is
titled `Ard'gral, plane of dreams` and contains 64 rooms (#30401–#30464), one
mobile (#30401), and 14 objects (#30401–#30414). Active WLD, MOB, OBJ, and
ZON files are present. The stem is listed in
[`areas/qst/AREA`](../../../areas/qst/AREA#L135), but there is
no local QST file or native Q/QA contract, no SHP file, and no story sidecar.
The [ZON header](../../../areas/zon/ardgral.zon#L1) sets reset mode 2. Its reset records place 18 objects, one
nightmare essence mobile, and its mask equip.

Every room repeats the same interplanar dream-mist prose; the only local
character speaks of an inescapable nightmare, and the local mask reinforces
that image. There is no local authored quest or recorded player outcome. See
the [story dossier](../zone-stories/ARDGRAL.md) for the authored narrative
and builder questions.

## Active WLD graph

All 256/256 authored exits resolve internally. Each room has four directional
exits, and the links form a wraparound 8-by-8 grid: the static weak graph has
one component of 64 rooms. From #30401, raw directed traversal over authored
exits reaches all 64 rooms and every room has a return path. No exit is
`EX_BLOCKED`, and the active WLD graph has no inbound or outbound area
boundary. These are static exit results; they do not qualify travel callbacks,
spell conditions, or in-game arrival safety.

## Typed travel and cross-area dependencies

### Fixed rifts and random gateway

The ZON file resets six copies of object #30402 and one copy of each rift
object #30403–#30414. The compiled bindings associate #30402 with
`teleporting_map_pool` and #30403–#30414 with `teleporting_pool`
([bindings](../../../src/specs/specs.assign.c#L1931)). On character entry, each
fixed rift's `magic_pool` callback resolves object value 0 as the target room
and moves that character there. Its null-character callback has a 6% chance
per call to move the object to another eligible room in its zone
([pool callbacks](../../../src/specs/specs.teleport_pools.c#L19),
[fixed-room transfer](../../../src/specs/specs.teleport_pools.c#L107)). The
following twelve destinations all resolve to active WLD room owners:

| Object and reset room | Fixed target | Active destination |
| --- | --- | --- |
| [object #30403](../../../areas/obj/ardgral.obj#L21), [reset at #30457](../../../areas/zon/ardgral.zon#L24) | #12401 | [The Ethereal Plane](../../../areas/wld/ethereal_main.wld#L1) |
| [object #30404](../../../areas/obj/ardgral.obj#L29), [reset at #30441](../../../areas/zon/ardgral.zon#L18) | #25410 | [Plane of Fire](../../../areas/wld/plane_fire_one.wld#L362) |
| [object #30405](../../../areas/obj/ardgral.obj#L37), [reset at #30432](../../../areas/zon/ardgral.zon#L17) | #23901 | [Plane of Earth](../../../areas/wld/plane_earth_one.wld#L3904) |
| [object #30406](../../../areas/obj/ardgral.obj#L45), [reset at #30418](../../../areas/zon/ardgral.zon#L14) | #24401 | [Plane of Air](../../../areas/wld/plane_air_one.wld#L1) |
| [object #30407](../../../areas/obj/ardgral.obj#L53), [reset at #30414](../../../areas/zon/ardgral.zon#L13) | #23324 | [Plane of Water](../../../areas/wld/plane_water_one.wld#L5167) |
| [object #30408](../../../areas/obj/ardgral.obj#L61), [reset at #30458](../../../areas/zon/ardgral.zon#L25) | #33480 | [Killing Fields](../../../areas/wld/killing_fields.wld#L5694) |
| [object #30409](../../../areas/obj/ardgral.obj#L69), [reset at #30447](../../../areas/zon/ardgral.zon#L20) | #26205 | [Moonshae Island](../../../areas/wld/moonshae.wld#L105) |
| [object #30410](../../../areas/obj/ardgral.obj#L77), [reset at #30452](../../../areas/zon/ardgral.zon#L22) | #93641 | [Vella's Bordello](../../../areas/wld/bordello.wld#L909) |
| [object #30411](../../../areas/obj/ardgral.obj#L85), [reset at #30449](../../../areas/zon/ardgral.zon#L21) | #74009 | [Bloodstone Keep](../../../areas/wld/bs.wld#L309) |
| [object #30412](../../../areas/obj/ardgral.obj#L93), [reset at #30407](../../../areas/zon/ardgral.zon#L12) | #4082 | [Caves of Mt. Skelenak](../../../areas/wld/caves_skelenak.wld#L1888) |
| [object #30413](../../../areas/obj/ardgral.obj#L101), [reset at #30402](../../../areas/zon/ardgral.zon#L10) | #94702 | [The Deep Ravine of Passage](../../../areas/wld/minopass.wld#L36) |
| [object #30414](../../../areas/obj/ardgral.obj#L109), [reset at #30462](../../../areas/zon/ardgral.zon#L26) | #19706 | [The Astral Plane](../../../areas/wld/astral_main.wld#L201) |

Object #30402 uses `magic_map_pool`, whose character-entry branch chooses a
random surface-map room and filters out mountain, inside, ocean, and
`ROOM_NO_GATE` destinations. Its null-character callback has a 6% chance to
relocate the gateway within Ard'gral. That makes its outbound destination
dynamic, separate from the twelve fixed rifts. The source includes no fixed
reverse room edge for these journeys.

### Compiled spell route

`cast_ardgral` selects a random room number from #30401 through #30464
([range selection](../../../src/classes/sillusionist.c#L2209)) and creates a
one-way portal ([portal call](../../../src/classes/sillusionist.c#L2231)). The
spell returns early unless the caster is alive
([alive check](../../../src/classes/sillusionist.c#L2204)); it also refuses
origins with `ROOM_NO_GATE` or a hometown
([origin check](../../../src/classes/sillusionist.c#L2211)) and requires
`is_Raidable` to succeed
([raidability check](../../../src/classes/sillusionist.c#L2217)). Its
destination range is a direct compiled dependency on all 64 local rooms.

### Active incoming portal from Alatorin

Alatorin's active ZON resets object #83141 in room #83342. Its type-25
prototype stores destination #30453 and its long description explicitly says
the rift opens to Ard'gral ([Alatorin reset](../../../areas/zon/alatorin.zon#L1555),
[active prototype](../../../areas/obj/alatorin.obj#L589),
[duplicate source copy](../../../areas/obj/alatorin_objects.obj#L587),
[target room](../../../areas/wld/ardgral.wld#L1613)). The same prototype is
copied in `alatorin_objects.obj`, but the active Alatorin source inventory
uses `alatorin.obj`; the extra copy does not add a second active route.
Odeth's QST dialogue and the Alatorin story sidecar describe a separate potion
of Ard'gral recipe whose reward is object #83140. The explicit incoming
travel route is the reset rift, not that recipe ([Odeth QST](../../../areas/qst/alatorin.qst#L997),
[Alatorin story entry](../../../areas/story/alatorin.story.json#L37)). The
[Alatorin dossier](../zone-stories/ALATORIN.md#L221) already records the
route's access path.

## Inactive Yggdrasil overlap and candidate disposition

The all-95 inactive-WLD sweep parsed without errors. Dormant `ygg.wld` defines
12 rooms #30401–#30412, each colliding numerically with an Ard'gral room. It
also defines mobile and object #30401, colliding
with the local nightmare mobile and mask. The scanner reports 21 ygg exits
whose targets numerically fall inside Ard'gral; these are authored Yggdrasil
links to Yggdrasil's own overlapping rooms, not confirmed active inbound
routes. The separate Yggdrasil camp #30400 has a D0 exit to #30401, but its
target owner is ambiguous while the two source sets reuse the same VNUM
([camp](../../../areas/wld/ygg.wld#L1),
[D0 exit](../../../areas/wld/ygg.wld#L10)).

The dormant Yggdrasil portal objects #30400/#30401 target #30412/#30407 in
their own room-range story ([#30400 target](../../../areas/obj/ygg.obj#L7),
[#30401 target](../../../areas/obj/ygg.obj#L15)); `ygg.zon` resets them in its
overlapping rooms ([#30400 reset](../../../areas/zon/ygg.zon#L11),
[#30401 reset](../../../areas/zon/ygg.zon#L15)). Its SHP product references
#30408, #30412, and #30410 likewise refer to Yggdrasil's colliding object
range ([45](../../../areas/shp/ygg.shp#L45),
[61](../../../areas/shp/ygg.shp#L61),
[76](../../../areas/shp/ygg.shp#L76),
[78](../../../areas/shp/ygg.shp#L78)). None establishes an active
Alatorin-style dependency on Ard'gral. Other typed scans found no active
external QST item command or SHP product referencing an Ard'gral object,
external ZON reset importing a local mobile/object, active duplicate local
prototype, ITEM_SWITCH target, or story-sidecar item reference. The Alatorin
rift is the only active external type-25 target into Ard'gral; the other
numeric portal matches are the dormant Yggdrasil records and the duplicate
`alatorin_objects.obj` declaration.

Keep the existing `component:ygg` coverage and
[candidate row 122](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L122). Its current
`RETAIN IN PLACE` disposition remains necessary until the room, mobile,
object, portal, shop, and editor identities are reconciled. The numeric
source-only edge from Yggdrasil #30400 to #30401 is not treated as an active
or owner-resolved approach, and no new candidate row is added for Ard'gral.
Curated source-only totals therefore remain 3,439 overall, 205 for Surf, and
15 for `ud_map2`.

## Title and bounded history

After color-code normalization, `Ard'gral, plane of dreams` is unique among
442 scanned current `.zon` titles; there is no other lexical candidate.
Followed same-stem history remains on `ardgral`. The 2011 `Fixes to Ard'gral`
and 2013 `stuff` commits change mobile/object data and labels, but do not
establish another area owner. The 2015 quest-item flag pass is mechanical.
In 2016, commit `d92a697fe` added object #30413, a Deep Ravine rift, and
updated the reset; the prior near-invisible rift moved to #30414 while
retaining its Astral destination. Commit `c884c1c2f` later corrected
descriptions without changing that destination. No alternate-stem rename,
distinct predecessor, or whole-zone replacement was established. Dormant
Yggdrasil is a separate titled source component, and its VNUM collision is
not evidence of replacement. See the
[current title source](../../../areas/zon/ardgral.zon#L2) and bounded history
through commits `fa8597218`, `71d731c5b`, `d92a697fe`, and `c884c1c2f`.

## Archive disposition

Keep active `ardgral` DO NOT MOVE. Preserve the twelve fixed rift destinations,
the six dynamic gateways, the compiled #30401–#30464 spell range, and
Alatorin's active type-25 incoming rift to #30453. Keep `ygg` RETAIN IN PLACE
under its existing candidate row until the shared room/mobile/object IDs,
old local portals, and editor identity are reconciled. Source analysis does
not qualify current availability, route safety, or successful player return.
