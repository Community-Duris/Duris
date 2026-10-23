# Astral Main repository-wide audit review

This review records the source story, authored graph, portal and pool routes,
typed reset dependencies, compiled numeric bindings, and bounded title and
history findings for active contributor `astral_main`. It accompanies the
[source story dossier](../zone-stories/ASTRAL_MAIN.md). It describes current
repository source and compiled references. It does not assert live access or
runtime safety, and no area or gameplay source was changed.

## Source identity and narrative coverage

At audit snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`,
`astral_main` is an active zone-197 contributor titled `The Astral Plane`.
It has 125 rooms (#19701–#19825), 24 mobile prototypes, 56 object
prototypes, 748 authored WLD exits, and reset mode 1
([active list](../../../areas/AREA#L100),
[zone and resets](../../../areas/zon/astral_main.zon#L1),
[rooms](../../../areas/wld/astral_main.wld#L1)). Its title is unique after
color-code normalization among 442 current `.zon` files.

The QST file contains only `#19800` followed by `S`; there are zero native
Q contracts. There is no story sidecar or local shop file. The room and
mobile descriptions establish three environmental landmarks and an Illithid
invasion against the Githyanki, but do not give that conflict an outcome or
timeline. A reset-loaded note adds a Githzerai spying plot in the Githyanki
chief's quarters in Astral Side Areas. The [source dossier](../zone-stories/ASTRAL_MAIN.md)
records these story questions.

The source also contains `areas/mob/astral_main.wld`, a byte-identical copy
of `areas/wld/astral_main.wld` (same Git blob and file hash). The active
`areas/AREA` source inventory names `astral_main`; the ordinary WLD generator
reads from `areas/wld/`, not `areas/mob/`
([generator](../../../areas/src/wld/make_wld.c#L6)). Treat this as a misplaced
duplicate source file, not another loaded area. Confirm its intended status
before any separate cleanup.

## Active WLD graph and boundaries

All 748 authored exits resolve: 743 are internal and five cross area. The 125
rooms form one weak component. Raw directed traversal from #19701 reaches
and returns from all 125 rooms when all authored exits are treated as
available. All 748 exits have door flags 0 and key VNUM 0; `astral_main.zon`
has no D door-reset commands. This is authored-graph evidence, not a live
movement or safety result.

| Main source | Direction | Active destination | Reverse WLD edge |
| --- | --- | --- | --- |
| #19755 | South | Astral Side Areas #19826 | Yes |
| #19794 | West | Gith Hometown #19400 | Yes |
| #19798 | Up | Githyanki Fortress #132000 | Yes |
| #19800 | North | Astral Tiamat #19928 | Yes |
| #19815 | Up | Astral Side Areas #19867 | Yes |

The 21 active inbound WLD exits come from `astral_areas` (4),
`astral_tiamat` (3), `gith_ht` (1), `githyanki_fortress` (1),
`interspa` (1), `prison` (5), and `tribal` (6). Only the two reciprocal
Astral Side Areas pairs, the Tiamat #19928/#19800 pair, Gith Hometown, and
the Fortress pair have reverse records. The other one-way inbound groups
are Astral Side Areas #19828 and #19885, Astral Tiamat #19948 and #19955,
Interspace #99999, Prison #7488, and Tribal Oasis #42297. Exact outbound
and inbound edges are in exit-register [rows 154-158](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L154),
[311](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L311),
[319-330](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L319),
[569-574](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L569),
[1189](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1189), and
[1301](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1301), with the directed zone
pairs in [rows 27-32](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L27),
[154-155](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L154),
[202](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L202),
[358](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L358), and
[668](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L668).

All 95 current unregistered WLD files parsed with zero room-ID overlaps,
source-only target references into Astral Main, or source-only approaches.
The only duplicate loaded room VNUM is #51800 in two unrelated Hell files.

## Portals, pools, and spell routes

### Reset-loaded type-25 portals

| Owner and prototype | Active placement | Destination | Evidence |
| --- | --- | --- | --- |
| Alatorin #83301 | Room #83487 | Astral Main #19721 | [object](../../../areas/obj/alatorin.obj#L3168), [reset](../../../areas/zon/alatorin.zon#L1622) |
| Celestia #45535 | Room #45500 | Astral Main #19721 | [object](../../../areas/obj/celestia.obj#L526), [reset](../../../areas/zon/celestia.zon#L153) |
| Jotunheim #96005 | Room #96004 | Astral Main #19733 | [object](../../../areas/obj/jotun.obj#L102), [reset](../../../areas/zon/jotun.zon#L209) |
| Ravenloft 2 #59081 | Room #59169 | Astral Main #19721 | [object](../../../areas/obj/ravenloft2.obj#L1227), [reset](../../../areas/zon/ravenloft2.zon#L916) |

Celestia object #45536 is the authored return portal to room #45500 and is
reset twice in Astral Main, at #19721 and #19796
([object](../../../areas/obj/celestia.obj#L534),
[Main resets](../../../areas/zon/astral_main.zon#L9)). An unregistered
`alatorin_objects.obj` file also defines #83301 with the same #19721 target
and object text as active Alatorin #83301; it has no active ZON placement and
is not an additional active portal
([source-only copy](../../../areas/obj/alatorin_objects.obj#L3093)). Keep
that candidate in place until its owner and tooling status are resolved.

### Compiled magic pools

`specs.assign.c` binds Astral Main objects #19710, #19711, #19715, and
#19860–#19864 to `magic_pool`. The callback checks that the target room
resolves, teleports entrants, and applies the object's damage value to
non-trusted entrants ([binding](../../../src/specs/specs.assign.c#L1894),
[callback](../../../src/specs/specs.teleport_pools.c#L107)). Current targets
and reset placements are:

| Object | Target and owner | Active reset placement | Source implication |
| ---: | --- | --- | --- |
| [#19710](../../../areas/obj/astral_main.obj#L158) | #19946, Astral Tiamat | #19945 ([reset](../../../areas/zon/astral_tiamat.zon#L58)) | Placed in #19945; object #19715 in #19946 targets back to #19945. Damage value 200. |
| [#19711](../../../areas/obj/astral_main.obj#L178) | #25410, Fire Plane | None found | Active destination; no active reset placement found. |
| [#19715](../../../areas/obj/astral_main.obj#L221) | #19945, Astral Tiamat | #19946 ([reset](../../../areas/zon/astral_tiamat.zon#L60)) | Placed in #19946; object #19710 in #19945 targets back to #19946. Damage value 50. |
| [#19860](../../../areas/obj/astral_main.obj#L570) | #12479, Ethereal Main | Astral Side Areas #19842 ([reset](../../../areas/zon/astral_areas.zon#L74)) | Active route; damage value 400. |
| [#19861](../../../areas/obj/astral_main.obj#L586) | #163372, inactive `Duris3.wld` only ([room](../../../areas/wld/Duris3.wld#L1330813)) | Astral Side Areas #19856 ([reset](../../../areas/zon/astral_areas.zon#L85)) | Full-world room lookup fails; the callback reports a broken pool. |
| [#19862](../../../areas/obj/astral_main.obj#L608) | #14389, Crypt | None found | Active destination; no active reset placement found. |
| [#19863](../../../areas/obj/astral_main.obj#L628) | #12492, Ethereal Main | None found | Active destination; no active reset placement found. |
| [#19864](../../../areas/obj/astral_main.obj#L643) | #163372, inactive `Duris3.wld` only ([room](../../../areas/wld/Duris3.wld#L1330813)) | None found | Full-world room lookup fails if this object is used. |

Object #19860 is also an important typed collision: #19860 exists as an
Astral Main object but no current mobile prototype #19860 exists. The three
local ZON `M` commands for mobile #19860 are invalid reset targets, not pool
mobiles. `renum_zone_table` resolves the mobile namespace and turns invalid
reset commands into disabled `!` entries
([invalid-command handling](../../../src/world/db.c#L1668),
[resets](../../../areas/zon/astral_main.zon#L21)).

### Plane spells and other compiled references

Plane gate spells use room #19701 as the Astral zone anchor and select a
room within that plane zone; they do not encode one fixed destination room
([plane table and gate selection](../../../src/magic/spells.c#L295)). The
prime-shift innate also uses #19701 to identify the Astral zone before
applying race-specific destination lists
([innate](../../../src/classes/innates.c#L2003)). Shared movement and map
code defines the Astral VNUM range as #19701–#19825
([range definitions](../../../src/core/utils.h#L1085)).

Other exact typed code references include Tiamat #19700 in the high-drop
mobile table, a separate item proc that reads mobile #19790, and a Demogorgon
helper search list that includes mobile #19860 even though that mobile is
not defined. `shout_and_hunt` compares the helper list against live mobile
VNUMs, so the #19860 entry cannot find a current mobile; it does not create
or place one. Astral succubus #19880 is bound to a callback that calls other
same-procedure succubi to help when one is fighting. `mobact.c` exempts
Astral-native PCs above level 50 from Githyanki #19870 aggression, and above
level 40 from smoke elemental #19840 aggression
([high-drop list](../../../src/item/randomeq.c#L115),
[item proc](../../../src/specs/specs.underworld.c#L1906),
[Demogorgon helper list](../../../src/specs/specs.demogorgon.c#L21),
[succubus callback](../../../src/specs/specs.astral_main.c#L7),
[mobile binding](../../../src/specs/specs.assign.c#L1036),
[aggression exceptions](../../../src/mob/mobact.c#L6686)). Special mobile
and object callbacks are assigned to several Astral prototypes in
`specs.assign.c`.

One compiled room binding is unresolved. `specs.assign.c` assigns
`GithyankiCave` to `world[real_room0(19890)]`, but there is no WLD room #19890;
#19890 is the Astral Main oak desk object. `real_room0` returns array index 0
for a missing room rather than -1, and the assigned callback handles the
down command by transferring trusted or Illithid characters to room #96524
([assignment](../../../src/specs/specs.assign.c#L2287),
[lookup behavior](../../../src/world/db.c#L8480),
[callback](../../../src/specs/specs.room.c#L503),
[object](../../../areas/obj/astral_main.obj#L775)). Confirm the intended room
or remove/replace the obsolete binding.

## Typed reset dependencies

The Astral Main ZON imports these active external prototypes:

| Owner | Reset roles and imported prototypes |
| --- | --- |
| `astral_tiamat` | `G` object prototypes #19901-#19906, #19928, and #19931 ([Main reset](../../../areas/zon/astral_main.zon#L22), [prototype file](../../../areas/obj/astral_tiamat.obj#L21)). |
| `celestia` | `O` object #45536 x2; each returns to Celestia room #45500 ([Main resets](../../../areas/zon/astral_main.zon#L9), [object](../../../areas/obj/celestia.obj#L534)). |
| `wh`, `realm`, `heavens` | `G` objects #55203, #14124, and #359, respectively ([resets](../../../areas/zon/astral_main.zon#L42)). |
| `unique` | `G` object #67206 ([Main reset](../../../areas/zon/astral_main.zon#L45), [prototype](../../../areas/obj/unique.obj#L120)). |

All these external reset targets have active typed prototype owners. The
local ZON also has four missing targets: mobile #19860 three times and object
#19997 once. No current active or unregistered `.mob` owns #19860; no current
active or unregistered `.obj` owns #19997. The object #19860 coincidence
does not satisfy a mobile reset. The invalid rows are disabled by the
renumbering pass above. Bounded Git searches found no valid mobile prototype #19860 or object
prototype #19997 in the post-move source paths. Keep the unresolved source
rows documented until their intent is confirmed.

Inbound typed reset and authored-story references to Astral Main include:

| Source | Typed reference to Astral Main |
| --- | --- |
| `astral_areas.zon` | Imports Main-owned mobiles #19702, #19703, #19770, #19780, #19790, #19800, #19810, #19820, #19885 and objects #19700, #19703, #19704, #19770, #19790, #19795, #19800, #19810, #19815, #19820, #19835, #19840, #19850, #19855, #19860, #19861, #19865, #19870, #19871, #19873, #19875, #19877, #19879, #19885, #19890. |
| `astral_tiamat.zon` | Imports mobile #19760 and objects #19710, #19715, #19740, #19750, #19760, #19780, #19872. |
| `bctdl.zon` | Imports object #19730. |
| `tiamat.zon` | Imports mobile #19700. |
| `surfacemini.story.json` and `tiamat.story.json` | Story contacts reference Demogorgon mobile #19704 and Tiamat mobile #19700, respectively ([Surf contact](../../../areas/story/surfacemini.story.json#L263), [Tiamat contact](../../../areas/story/tiamat.story.json#L53)). |
| Unregistered `nexus56.zon` and `nexus56.shp` | Reset and shop product reference Main object #19770 ([ZON](../../../areas/zon/nexus56.zon#L1373), [SHP](../../../areas/shp/nexus56.shp#L496)). The shop row is source-only and does not establish active shop stock; preserve it with Nexus56 candidate [row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79). |

No active external QST item reference, SHP product, or story-sidecar item
reference to a Main object was found. No external ITEM_SWITCH room destination,
duplicate definition of a Main-owned mobile/object VNUM, or external type-25
target into a Main mobile/object VNUM was found. The four active room-targeting
portals are listed above. The `nexus56` SHP product is unregistered and is
therefore separately retained as candidate evidence.

The `astral_areas` reset imports Main object #19815, the note about Githzerai
spying, as a `P` child of Main object #19810, a small wooden desk. That desk is
reset in room #19850, whose room text identifies it as the Githyanki chief's
quarters ([note](../../../areas/obj/astral_main.obj#L417),
[desk](../../../areas/obj/astral_main.obj#L404),
[nested reset](../../../areas/zon/astral_areas.zon#L80),
[room](../../../areas/wld/astral_areas.wld#L632)). The numeric collision
between note object #19815 and Astral Main room #19815 is in separate typed
namespaces and is not a duplicate room or object definition.

## Title, history, and disposition

The normalized title `The Astral Plane` is unique among 442 current `.zon`
titles. Followed WLD, MOB, OBJ, and ZON paths remain on the `astral_main`
stem through the 2009 area-tree move and later same-stem revisions. Bounded
history shows content and balance changes, including the 2010 move of the
epic stone from the succubus to Demogorgon, but no alternate-stem rename or
whole-zone replacement. The separate 39-room Githyanki fortress in
`astral_areas` is a possible but unconfirmed referent for a later “old gith
fort” replacement note; both areas remain active.

**Disposition: DO NOT MOVE.** Keep `astral_main` as an active contributor.
Preserve the five reciprocal WLD boundaries, the four active incoming
portal sources, Celestia's return portals, all active typed reset imports,
cross-area reset users, story-sidecar contacts, pool references, and compiled
bindings. Do not migrate `nexus56` or `alatorin_objects` based on this review;
both remain RETAIN IN PLACE under their existing candidate rows. Builder
decisions remain open for invalid reset targets, the missing room callback,
a reciprocal pool pair between Astral Tiamat rooms #19945/#19946 and the Duris3-only pool destination.
