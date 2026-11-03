# Astral Areas repository-wide audit review

This review records the source story, active WLD graph, compiled travel
objects, typed reset dependencies, and bounded title/history findings for
active contributor `astral_areas`. It accompanies the
[Astral Areas source dossier](../zone-stories/ASTRAL_AREAS.md). The findings
describe repository source and compiled configuration; no live route,
availability, or safety behavior was tested. No area or gameplay source was
changed.

## Source identity and story evidence

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`,
`astral_areas` is an active zone-198 contributor titled `Astral Plane, Side
Areas`. It has 61 rooms (#19826–#19886), zero local mobile prototypes, zero
local object prototypes, 141 authored WLD exits, and reset mode 1. The room
text establishes the gate sanctuary, a Githyanki fortress on a floating black
landmass, and air/rock spheres with a phoenix nest. The final two rooms are
explicit load/failure rooms
([active entry](../../../areas/AREA#L102),
[zone](../../../areas/zon/astral_areas.zon#L1),
[rooms](../../../areas/wld/astral_areas.wld#L1)).

There is no `astral_areas.qst`, shop source, or story sidecar. The historical
`areas/qst/AREA` list contains the key but is not the production quest source
list. The empty local `.mob` and `.obj` files do not mean that the zone has no
reset-loaded inhabitants or objects: the active ZON imports externally owned
prototypes, detailed below.

## Active WLD graph and static routes

The parser resolved all 141 authored exits. There are 137 internal exits and
four cross-area exits; no Astral Areas exit has an unresolved active target.
Weak components and authored directed reach are:

| Component | Rooms | Raw directed reach and return from range start |
| --- | ---: | ---: |
| Rift and sanctuary | #19826–#19827 (2) | 2/2 forward and return |
| Githyanki fortress | #19828–#19866 (39) | 39/39 forward and return |
| Air and rock spheres | #19867–#19884 (18) | 18/18 forward and return |
| Load and failure pair | #19885–#19886 (2) | 2/2 forward; 1/2 can return to #19885 |

These counts use authored directed exits and ignore the zone's reset door
states. They do not establish actual movement through the closed/locked doors.
The active boundaries are:

| Source | Destination | Reverse WLD edge |
| --- | --- | --- |
| Astral Main #19755 south → Astral Areas #19826 | reciprocal north edge |
| Astral Main #19815 up → Astral Areas #19867 | reciprocal down edge |
| Astral Areas #19828 down → Astral Main #19798 | none |
| Astral Areas #19885 down → Astral Main #19806 | none |

The exact records are in the active exit register
([rows 319 and 323–327](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L319)) and pair
summary ([rows 27–28](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L27)). The normal
Astral Plane route from #19798 goes up to the separate active
`githyanki_fortress` room #132000, which returns down to #19798
([Astral Main](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L321),
[Fortress return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1301)). This is a
distinct WLD pair; it does not provide an inbound edge to the 39-room
fortress segment at #19828–#19866.

No active WLD edge enters the #19828–#19866 or #19885–#19886 components. The
all-95 inactive-WLD sweep found no source-only WLD approach or room-VNUM
overlap with this area. The active room-range scan found no type-25 portal
target into its rooms. One active-room VNUM collision exists elsewhere in the
world (#51800 in `hell_three_minauros.wld` and `hell_two_dis.wld`); no
Astral Areas room shares it.

## Door resets and keys

The zone applies 16 door resets: four pairs start closed and locked, and four
pairs start closed. The WLD's key references and reset states do not align in
every case.

| Door pair | WLD key | Reset state | Source-declared key placement |
| --- | ---: | --- | --- |
| #19826 ↔ #19827, rift gate | #19870 | closed/locked | Astral Key on one Astral Guardian at #19826 |
| #19833 ↔ #19840, fortress gates | #19871 | closed/locked | Large silverish key on a Githyanki Knight at #19833 |
| #19858 ↔ #19859, tiny hall/stair | #19879 | closed/locked | Silverish key nested in the desk at #19850 |
| #19860 ↔ #19861, tower rooms | #19875 | closed/locked | Ornate key on a Githyanki Knight at #19860 |
| #19853 ↔ #19854, audience hall/crypt | #19877 | closed, not locked | Marble key on the Githyanki High Mage at #19853 |
| #19863 ↔ #19864, hillside/cave | #19873 | closed, not locked | Dark metal key on the Githyanki Chief Knight at #19850 |
| #19840 ↔ #19843, courtyard/tower | none | closed, not locked | No key reference |
| #19879 ↔ #19882, sphere passage | none | closed, not locked | No key reference |

The exact door commands are at
[ZON lines 8–68](../../../areas/zon/astral_areas.zon#L8); WLD key numbers are
on the [rift gate](../../../areas/wld/astral_areas.wld#L17),
[fortress gate](../../../areas/wld/astral_areas.wld#L196),
[crypt](../../../areas/wld/astral_areas.wld#L718),
[tiny hall](../../../areas/wld/astral_areas.wld#L832),
[tower](../../../areas/wld/astral_areas.wld#L874), and
[cave](../../../areas/wld/astral_areas.wld#L967) records. Confirm whether
the crypt and cave doors are meant to require their reset-loaded keys.

## Compiled pool routes

Objects #19860 and #19861 are Astral Main prototypes with `magic_pool`
callbacks, reset into the library at #19842 and the Mercury Color Pool at
#19856. The callback treats object value 0 as a room target and checks
`real_room(target)` before teleporting
([bindings](../../../src/specs/specs.assign.c#L1902),
[callback](../../../src/specs/specs.teleport_pools.c#L107),
[object #19860](../../../areas/obj/astral_main.obj#L570),
[object #19861](../../../areas/obj/astral_main.obj#L586),
[placements](../../../areas/zon/astral_areas.zon#L74)).

| Local placement | Object target | Current owner | Source result |
| --- | ---: | --- | --- |
| #19860 in #19842, library | #12479 | active `ethereal_main` | The pool's second value is 400 damage for non-trusted entrants; its Ethereal destination is loaded. |
| #19861 in #19856, Mercury Color Pool | #163372 | only `Duris3.wld`, unregistered | Full-world `areas/AREA` omits Duris3, so `real_room(163372)` is `NOWHERE`; the callback returns its “busted” response without teleporting. |

The Ethereal source separately resets magic-pool object #12460 in room #12438;
that object targets Astral Main #19702. This is a possible onward return to
Astral Main, not a direct return to the fortress pool's source room
([Ethereal object](../../../areas/obj/ethereal_main.obj#L274),
[reset](../../../areas/zon/ethereal_main.zon#L14)). The pool target in
Duris3 is room “The Ocean” #163372, whose source record and exits are in the
existing [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36)
and [unregistered WLD](../../../areas/wld/Duris3.wld#L1330813). Keep that
candidate in place until the pool destination or the Duris3 source's intended
status is resolved.

## Typed source dependencies

The local ZON contains 43 unique entity-prototype references: 11 mobile
prototypes and 32 object prototypes. Every identifier resolves to an active
source owner. The complete owner crosswalk is:

| Active prototype owner | Mobile prototypes used | Object prototypes used |
| --- | --- | --- |
| `astral_main` | #19702, #19703, #19770, #19780, #19790, #19800, #19810, #19820, #19885 | #19700, #19703, #19704, #19770, #19790, #19795, #19800, #19810, #19815, #19820, #19835, #19840, #19850, #19855, #19860, #19861, #19865, #19870, #19871, #19873, #19875, #19877, #19879, #19885, #19890 |
| `astral_tiamat` | #19900, #19910 | #19920, #19930, #19935, #19950, #19960, #19980 |
| `heavens` | — | #64 |

`#64` is the active Heavens “pool liquid glowing” object. Inactive
`hostel.obj` and `mavulsk.obj` files also define #64, but neither stem is in
the current `areas/AREA` world list. The current loaded owner is therefore
Heavens. The reset dependency is at
[ZON line 90](../../../areas/zon/astral_areas.zon#L90) and the active object
is at [Heavens #64](../../../areas/obj/heavens.obj#L904).

Exact scans found no external QST item command, SHP product, or authored
story-sidecar item reference to local prototypes, and there are no local
mobile/object prototypes for those commands to consume. No ITEM_SWITCH or
type-25 portal target points into #19826–#19886. The compiled numeric matches
were typed by their source context: `mobact.c` and the succubus binding use
Astral Main mobile VNUMs #19840, #19870, and #19880; `specs.assign.c` binds
object VNUMs #19860–#19864 to `magic_pool`; and `specs.guards.c` uses room
#19859 in a guild-guard switch that blocks north. The authored #19859 room
has south and up exits but no north exit. A Demogorgon helper array also
contains #19860 as a mobile VNUM even though no current `areas/mob/*.mob`
file defines that mobile; this is a separate unresolved helper reference, not
a route to room #19860.

## Title and replacement history

After color-code normalization, `Astral Plane, Side Areas` is unique among
442 current `.zon` titles. The adjacent `astral_main` and `astral_tiamat`
contributors have distinct titles. Followed WLD/MOB/OBJ/ZON histories remain
on the same `astral_areas` stem through the areas-tree move and later edits.
The 2010 WLD edit changes the door flag on the route from #19858 to #19859.
Commit `6b16ee1d4` (2011) removes one ZON reset of object #359 from room
#19861; its commit message calls this “Removed epic stone from astral side
areas zone.” The current Heavens prototype #359 is a rune-covered stone.
The 2015 edit changes the editor-version comment in the ZON header. These
changes show same-stem maintenance, not a complete source retirement.

The 2010 `githyanki_fortress` addition explicitly says “the replacement for
old gith fort.” `astral_areas` contains a pre-existing 39-room fortress and
continues to reset Githyanki there, making it the strongest located candidate
for that phrase. The source history does not name the stem, and both maps
remain active. Record the relationship as a likely predecessor/parallel-map
question, not as proof that all of `astral_areas` was replaced. Preserve both
areas while the owner decides whether the old fortress segment is an intended
one-way spawn area or should gain a player-facing entrance.

## Disposition and builder decisions

Keep active `astral_areas` **DO NOT MOVE**. It is a full-world contributor and
its resets depend on active Astral Main, Astral Tiamat, and Heavens prototypes.
Add no attic candidate row for this active area. Keep Duris3 **RETAIN IN
PLACE** in existing candidate row 36 while its #163372 destination is
unresolved.

The area owner should decide:

1. Whether the 39-room fortress segment is the old fort named by the 2010
   replacement commit, and whether its active resets should feed roaming
   mobiles, support player exploration, or serve another documented purpose.
2. Whether object #19861 should be retargeted to an active room or whether
   room #163372 and its Duris3 source should be restored through an approved
   world-list change.
3. Whether to retain the 400-damage magic-pool route from #19842 and add an
   intentional return path.
4. Whether the keyed crypt/cave doors should reset locked, and whether
   #19859's compiled north-block has any intended behavior with the current
   room exits.

These are source-backed review questions. They do not establish live
teleport success, reset timing, mobile movement, or safety.
