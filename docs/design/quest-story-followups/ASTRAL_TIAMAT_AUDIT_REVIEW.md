# Astral Tiamat repository-wide audit review

This review records the environmental story, active WLD graph, door resets,
typed prototype dependencies, compiled bindings, external story consumers, and
bounded title/history findings for active contributor `astral_tiamat`. It
accompanies the [source story dossier](../zone-stories/ASTRAL_TIAMAT.md).
The findings describe current repository source; they do not qualify live
player access or safety, and no area or gameplay source was changed.

## Source identity and narrative coverage

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`,
`astral_tiamat` is active zone 199, titled `Astral Plane, Tiamat`. It has 31
rooms (#19900–#19955), 11 local mobile prototypes, 44 local object prototypes,
65 authored WLD exits, and reset mode 0. The active source inventory contains
`.wld`, `.mob`, `.obj`, and `.zon` files only. There is no local `.qst`, `.shp`,
or story-sidecar file
([active entry](../../../areas/AREA#L102),
[zone and resets](../../../areas/zon/astral_tiamat.zon#L1),
[rooms](../../../areas/wld/astral_tiamat.wld#L1)).

The mountain narrative establishes a fragment torn from the Prime Material
Plane, unusually low gravity, a burned cave, Lord Redferne's lost warriors, and
an unfinished warning about arriving mages. The lair chain centers on Tiamat,
her guardians, lava caverns, a hidden passage, and the Vault of Avernus. The
vault prose says Tiamat kept her private treasures there and that it appears
unseen for many millennia, while active one-way WLD exits from `realm`, six
rooms in `tiamat`, and Winterhaven enter the vault directly. Active resets also
place objects from several outside areas in the vault. This is a source-level
story/access mismatch for the builder to resolve, not proof of actual player
access. Two unnamed rooms, one typo-named loading room, and two load rooms carry
no meaningful story text. The complete prose questions are in the
[dossier](../zone-stories/ASTRAL_TIAMAT.md#builder-questions).

## Active WLD graph and boundaries

The scoped parser found 31 rooms, 65 exits, no parse issues in this WLD, 61 local
exits, three positive cross-area exits, and one negative target that the loader
drops. Sixty-four of 65 authored exits resolve to active rooms. The five weak
components and directed raw reach from a representative room are:

| Local component | Rooms | Directed reach and return |
| --- | ---: | --- |
| Mountain and caves | #19928–#19945 (18) | #19928 reaches and returns from 18/18 |
| Pit and lair chain | #19902 and #19950–#19954 (6) | #19902 reaches and returns from 6/6 |
| Load and outbound branch | #19947, #19948, #19955 (3) | #19947 reaches 3/3; only 1/3 can return to it |
| Unnamed pair | #19900–#19901 (2) | #19900 reaches and returns from 2/2 |
| Pool and dark chamber | #19946, #19949 (2) | #19946 reaches and returns from 2/2 |

The traversal calculation follows authored directed local exits and assumes
ordinary movement through every edge. It ignores reset door state, secret or
locked restrictions, callbacks, and pool behavior. The outbound WLD boundaries
are:

| Source | Destination | Reverse WLD edge |
| --- | --- | --- |
| #19928 south | Astral Main #19800 | Yes |
| #19948 down | Astral Main #19785 | No |
| #19955 down | Astral Main #19785 | No |
| #19946 north | negative target #-1 | Dropped by loader |

Nine active WLD exits enter this contributor: Astral Main #19800 returns to
#19928; Realm #14208, six rooms in `tiamat` (#19620, #19624–#19627, #19638),
and Winterhaven #55634 each enter #19953 without a reverse WLD edge. The
all-95 inactive-WLD sweep found no room overlap or inactive approach into these
rooms. The separate scan of 871 outside type-25 object declarations found no
target into an Astral Tiamat room. Exact active exit records are in
[rows 328–330](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L328),
inbound [row 243](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L243),
[rows 313–318](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L313), and
[row 800](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L800), and negative-target
[unresolved row 46](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L46).

## Door resets and key placement

The zone has twelve door-reset commands covering six two-sided passages. Their
raw WLD key values and reset-loaded key evidence do not form a consistent
complete key path:

| Passage | WLD key values | ZON reset and source-declared key |
| --- | --- | --- |
| #19944 east ↔ #19945 west, huge rock door | #19872 both sides | Both reset closed/locked/secret. Astral Main object #19872 is given to huge slaad #19901 at #19940. |
| #19946 east ↔ #19949 west, boulder | #19908 both sides | Door reset values are 2 and 6 on the two sides. Local guardian dragon highlord #19908 is loaded twice at #19946 and each load receives rusty key #19908. |
| #19950 north ↔ #19954 south | 0 both sides | Both reset closed/secret; no key reference. |
| #19950 south ↔ #19951 north | 0 both sides | Both reset closed/secret; no key reference. |
| #19951 south ↔ #19952 north, great doors | #19876 / #19929 | Reset states are 5 and 6. Local magma key #19929 is inside container #19932 in #19952; no reset of Main key #19876 was found in this zone. |
| #19952 south ↔ #19953 north, wall/vault door | #19878 / -2 | Reset states are 5 and 6. No reset of Main key #19878 was found here; the meaning of sentinel #-2 is not established. |

The exact reset commands are at
[ZON lines 8–54](../../../areas/zon/astral_tiamat.zon#L8), and the authored
keys are in the [door records](../../../areas/wld/astral_tiamat.wld#L437).
The `D` reset sequence does not prove which route a player can open or what
`-2` means in the current builder workflow. Confirm the key ownership and
one-sided reset differences before treating this graph as passable.

## Pools, portals, and compiled callbacks

Astral Main objects #19710 and #19715 are bound to `magic_pool`. The reset
places #19710 in Tiamat room #19945 with target #19946 and damage value 200;
#19715 is placed in #19946 with target #19945 and damage value 50. The callback
checks that the target room exists before teleporting and applies the damage
to non-trusted entrants. These are reciprocal compiled pool routes between two
otherwise separate WLD components
([objects and targets](../../../areas/obj/astral_main.obj#L158),
[bindings](../../../src/specs/specs.assign.c#L1892),
[callback](../../../src/specs/specs.teleport_pools.c#L107),
[placements](../../../areas/zon/astral_tiamat.zon#L57)).

Local object #19907 is also type 25, names Tiamat room #19600 as its target,
and is reset in #19902 and #19949. No `magic_pool` or other object callback
assignment for #19907 was found. The `do_enter` command source searches room
exits; the type and target fields alone do not establish object transport.
Record it as an unbound type-25 prototype with unresolved intended behavior,
not as an active dynamic route ([object](../../../areas/obj/astral_tiamat.obj#L108),
[placements](../../../areas/zon/astral_tiamat.zon#L57),
[`do_enter`](../../../src/cmd/actmove.c#L3378),
[room-exit search](../../../src/cmd/actmove.c#L3443)).

Other compiled bindings include object #19900 to `githyanki` and mobile #19920
to `demogorgon_shout` ([assignments](../../../src/specs/specs.assign.c#L1907)).
These are typed prototype bindings: local object #19900 is a Githyanki sword,
while local mobile #19900 is the Astral Guardian; local mobile #19920 is
Juiblex in astral form, and object #19920 is a dagger. No active reset of
mobile #19920 was found. The different types must not be conflated by VNUM.

Additional exact code and kit references are typed the same way. Mobile #19901
is the Huge Slaad and appears in the `demogorgon` helper list; object #19901 is
a crimson ring and appears in the standard and enhanceable ranger kit tables.
Object #19911 (Volo’s boots) appears in both necromancer kit tables, and object
#19916 (Isha’s cloak) appears in both warrior kit tables. The cloak is also
bound to `artifact_hide`. These compiled references record use or preset
equipment, not a reset placement or player acquisition path
([mobile](../../../areas/mob/astral_tiamat.mob#L15),
[ring](../../../areas/obj/astral_tiamat.obj#L21),
[Demogorgon helper list](../../../src/specs/specs.demogorgon.c#L101),
[standard ranger kit](../../../src/account/chaos_eq_data.h#L80),
[enhanceable ranger kit](../../../src/account/chaos_eq_data.h#L333),
[standard necromancer kit](../../../src/account/chaos_eq_data.h#L158),
[enhanceable necromancer kit](../../../src/account/chaos_eq_data.h#L407),
[standard warrior kit](../../../src/account/chaos_eq_data.h#L73),
[enhanceable warrior kit](../../../src/account/chaos_eq_data.h#L326), and
[cloak binding](../../../src/specs/specs.assign.c#L1392)).

The `guild_guard` procedure has southward-blocking switch cases for rooms
#19950, #19951, and #19954. The assignment under the Astral Tiamat comment
binds it to mobile #19705; no active ZON placement of mobile #19705 was found,
and the local guardian #19908 has no corresponding binding. The reset-loaded
highlord's description says it guards the portal to the outer planes, so the
intended guard prototype and enforcement need a builder decision
([assignment](../../../src/specs/specs.assign.c#L618),
[guard room cases](../../../src/specs/specs.guards.c#L258),
[local highlord](../../../areas/mob/astral_tiamat.mob#L92),
[local reset](../../../areas/zon/astral_tiamat.zon#L97)).

## Typed reset dependencies and external consumers

The local ZON has 23 `O`, 20 `M`, 12 `D`, 12 `E`, three `G`, and one `P`
command. Its outside prototype imports are:

| Active prototype owner | Imported prototypes in `astral_tiamat.zon` |
| --- | --- |
| `astral_main` | Mobile #19760 five times; objects #19710, #19715, #19740, #19750, #19760, #19780, #19872 |
| `hell_one_avernus` | Object #51003 |
| `heavens` | Objects #31, #72, #400 |
| `tiamat` | Object #19607 |
| `yuan_ti` | Object #80571 twice |
| `wh` | Object #55203 |

The Astral Main pool imports are reset at their target rooms as described
above. Astral Tiamat's own reset references also leave the area: active
`astral_areas.zon` loads mobiles #19900 and #19910 and objects #19920, #19930,
#19935, #19950, #19960, and #19980; active `astral_main.zon` loads objects
#19901–#19906, #19928, and #19931. Preserve these typed source dependencies
([Astral Areas reset table](ASTRAL_AREAS_AUDIT_REVIEW.md#typed-source-dependencies),
[Astral Main reset table](ASTRAL_MAIN_AUDIT_REVIEW.md#typed-reset-dependencies)).

Two external quest consumers require Astral Tiamat objects. Heavens quest input
#19996 is the bone-white skull-faced helmet; no active ZON placement or code
creation reference for this object was found. Winterhaven's Volo exchange
requires cloak #19916, and its story sidecar carries the same contract. The
Tiamat slave death callback randomly creates one of #19911, #19916, #19637, or
#19638; the first two are Astral Tiamat boots and cloak, not Tiamat-owned
prototypes. `chaos_eq_data.h` also lists #19901 in ranger kits and #19916 in
warrior kits. No external SHP consumer was found
([helmet](../../../areas/obj/astral_tiamat.obj#L646),
[Heavens requirement](../../../areas/qst/heavens.qst#L148),
[cloak](../../../areas/obj/astral_tiamat.obj#L233),
[Winterhaven requirement](../../../areas/qst/wh.qst#L279),
[mirrored contract](../../../areas/story/wh.story.json#L870),
[death callback](../../../src/specs/specs.winterhaven.c#L4626),
[kit references](../../../src/account/chaos_eq_data.h#L70)).

One ZON row, `M 0 19921 1 19947`, names mobile #19921, which has no active
mobile prototype. An object #19921 does exist, but it is a separate type. The
loader resolves `M` references in the mobile namespace and disables a missing
mobile reset; the object does not satisfy it
([reset row](../../../areas/zon/astral_tiamat.zon#L100),
[object prototype](../../../areas/obj/astral_tiamat.obj#L314),
[typed loader resolution](../../../src/world/db.c#L1634),
[invalid-command handling](../../../src/world/db.c#L1668)).

## Title, history, and disposition

After color-code removal, `Astral Plane, Tiamat` is unique in the current
442-title `.zon` scan. Followed histories for the WLD, MOB, OBJ, and ZON files
remain on the same stem through the 2009 area-tree move and later same-area
balance, typo, item-flag, and guardian changes. The ZON history includes a
2010 Tiamat key correction and a 2009 addition of guardian mobs to vaults. No
alternate-stem rename, distinct predecessor, or whole-zone replacement was
found. `tiamat` (`Tiamat`) and `azhural` (`The Lair of Tiamat`) remain separate
active contributors; the separate `tiamats_lair` history associated with
Azhural does not identify `astral_tiamat` as its predecessor
([title source](../../../areas/zon/astral_tiamat.zon#L2),
[history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-190-astral-plane-tiamat-title-and-path-history),
[Azhural review](AZHURAL_AUDIT_REVIEW.md)).

Keep `astral_tiamat` **DO NOT MOVE**. It has active inbound and outbound WLD
boundaries, cross-area reset imports and consumers, a reciprocal compiled pool
route, an unbound type-25 pool prototype, and unresolved quest item sourcing.
No attic candidate row is added. Do not treat `#19907` as a working portal or
#19996 as an available quest drop until the owner resolves those source gaps.

## Builder decisions

1. Confirm whether the Redferne battle and incomplete inscription belong to a
   named historical event.
2. Resolve the Vault of Avernus isolation claim against its active one-way
   inbound paths, and decide whether #19953 is a player-facing room.
3. Identify the intended behavior of type-25 object #19907 and the meaning of
   the #19953 north key sentinel -2.
4. Reconcile WLD key values, actual local key placement, and asymmetric door
   reset states for the six passages.
5. Decide whether #19908 or #19705 should carry the `guild_guard` behavior.
6. Identify the intended source or placement for Heavens input #19996.
7. Clarify the role of the unnamed and load-only rooms.
8. Record whether `astral_tiamat`, `tiamat`, and `azhural` share story history;
   current names and tracked file history do not establish succession.

## Source references

- [Astral Tiamat WLD](../../../areas/wld/astral_tiamat.wld#L1),
  [mobile prototypes](../../../areas/mob/astral_tiamat.mob#L1),
  [object prototypes](../../../areas/obj/astral_tiamat.obj#L1), and
  [reset source](../../../areas/zon/astral_tiamat.zon#L1).
- [Active cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L328),
  [negative target register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L46),
  [coverage row 18](ZONE_COVERAGE_REGISTER.csv#L18), and
  [world connectivity report](WORLD_CONNECTIVITY.md#checkpoint-190-astral-plane-tiamat).
- [Source story dossier](../zone-stories/ASTRAL_TIAMAT.md),
  [title and history register](DUPLICATE_AND_REPLACED_ZONES.md), and
  [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-190-astral-plane-tiamat-disposition).

No tests, build, runtime session, database operation, migration, or operational
script was run for this documentation-only checkpoint.
