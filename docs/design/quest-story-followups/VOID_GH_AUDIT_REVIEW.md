# Void Guildhall source audit

Canonical key: `component:void_gh`. Coverage data row 469 / CSV line 470;
candidate dependency data row 118 / CSV line 119. Reviewed October 12, 2026
(Checkpoint 556) against checkout
`72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static source, graph,
typed-reference, identity, and followed-history review. It does not establish
live access, portal behavior, shop state, editor authority, or runtime safety.

## Source inventory and authored story

The same-stem files are [void_gh.wld](../../../areas/wld/void_gh.wld#L1),
[void_gh.mob](../../../areas/mob/void_gh.mob#L1),
[void_gh.obj](../../../areas/obj/void_gh.obj#L1),
[void_gh.zon](../../../areas/zon/void_gh.zon#L1), and
[void_gh.shp](../../../areas/shp/void_gh.shp#L1). The stem is absent from
[areas/AREA](../../../areas/AREA) and `areas/qst/AREA`. There is no same-stem
QST, story sidecar, or map/editor file in the checked-in source inventory.
The ZON header is #295, title “Main Guildhall for The Void,” top VNUM #29118,
reset mode 0. The normalized title is unique among 442 current ZON titles;
that does not resolve ownership of the map or its display alias.

The WLD has 19 described rooms #29100–#29118. The narrative presents The
Void as a fortress and guild fighting evil races in the Underdark. Room #29100
descends from the surface and its carvings show battles, including a conflict
with a five-headed dragon; the text says the carvings may depict past or
future events ([entrance](../../../areas/wld/void_gh.wld#L1)). Rooms
#29101–#29109 describe an obsidian citadel, the Slumbering Dragon Inn, a
shop, a branching corridor, a battle-planning room, storage, two unfinished
“portal of light” rooms, and a resting chamber. The meeting-room E descriptions
include a formula and a crude, sexually explicit joke
([meeting room](../../../areas/wld/void_gh.wld#L120)).

Rooms #29110–#29112 form a separate chasm, radiant passage, and portal scene.
Rooms #29113–#29115 describe a forest cave and elven outpost whose horns call
members to war against the Underdark, an Inn of the Eternals with forgotten
tomes, and a portal described as leading back to the main guildhall. Rooms
#29116–#29118 describe another surface stairway, the heart of the Void with
four supporting pillars and an altar, and a portal leading deeper into the
Void. These scenes have no same-stem formal QST contract or separate story
brief; they are authored WLD prose, not proof of current gameplay or a
connected quest chain.

The MOB text supplies five obsidian-golem variants with repeated guard lore;
Drac's description says he works for the guild and formerly served as its
innkeeper. The heart-room object descriptions give the four pillars letters
H/A/L/K, an altar with unexplained carvings, and a carvings object that repeats
the wall-scene narrative. Several object fallback descriptions warn that they
should not appear; the source alone does not confirm their runtime flags.

## Authored WLD graph and boundaries

The 19 rooms declare 33 exits: 30 target another `void_gh` room; three leave
the candidate. The local graph has four weak components and four strongly
connected components, sized 10, 3, 3, and 3. From #29100, raw directed
traversal reaches all ten rooms in its component; reverse traversal also
reaches all ten. The other components are #29110–#29112, #29113–#29115, and
#29116–#29118. All targets are positive; exit flags and key VNUMs are zero.
No fall-chance or current fields are present.

The three exits outside the source are all D4 (up): #29100 to #154553,
“Grassy Plains”; #29113 and #29116 to #163943, “Rolling Hills.” Both target
rooms occur in unregistered `Duris3.wld` and are absent from the active
`areas/AREA` world. The exact target rooms have no return exits to
`void_gh`; Duris3 neighboring rooms point to the Duris3 targets instead
([Grassy Plains](../../../areas/wld/Duris3.wld#L1145614),
[Rolling Hills](../../../areas/wld/Duris3.wld#L1342804)). These are source-only
boundaries and do not establish a live route or safe return.

All 30 local exit targets have numeric matches in active `torg.wld`, so the
coverage register records 30 loader-resolved target VNUMs. They resolve to
Torg room identities, not to an active Void map: all 19 Void room VNUMs also
belong to Torg with different WLD records. Active Torg has 21 authored exits
whose target numbers fall in #29100–#29118; those targets likewise resolve to
Torg's own rooms. Candidate-specific active WLD ingress remains zero. Other
unregistered guildhall files also have local exits into their own conflicting
same-number rooms; those numeric matches do not identify Void as their
destination.

The three Duris3 exits add no route into the active world and do not change
the curated source-only totals of 11,993 overall / 415 from Surf. See the
[world-connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-556-void_gh-source-graph-and-boundaries).

## Room, mobile, and object identity

Every Void room #29100–#29118 is also defined by active `torg.wld`, and none
of the 19 full room blocks is identical. Void #29100–#29109 also overlap
`afterlife_gh`, `scrubs_hall`, `solstice_gh`, and `wlust_gh`, all with
different WLD records. Void #29110–#29112 also occur in `afterlife_op` with
different records. The remaining Void rooms #29113–#29118 overlap Torg only.

The six Void mobiles are not defined by Torg. VNUMs #29100–#29104 overlap
`afterlife_gh`; #29100–#29103 also overlap `scrubs_hall`; #29100–#29102
overlap `solstice_gh`; and #29100–#29101 overlap `wlust_gh`. No same-number
mobile blocks are identical. Void objects #29100–#29102 overlap different
objects in `afterlife_gh`; #29100–#29105 overlap different objects in
`solstice_gh`. These are competing source records, not evidence that one
guildhall replaced another.

The normalized ZON title is unique, but header #295 is shared by
`genocide_hall` (“Genocide Guild Hall”) and `legion` (“Legion Guildhall”).
The room VNUM family and shared reset header do not settle the map owner. The
2013 followed-path commit includes WLD/ZON/SHP paths without text-line
changes. All five followed paths reach the 2009 area-tree move. A September
2026 ZON/SHP change added Heavens objects #491–#494 to Drac's inventory and
shop stock. No tracked alternate-stem rename, predecessor, successor,
retirement, or whole-zone replacement was found. Recent source maintenance
does not establish manifest membership.

## Typed references and bounded dependency closure

| Source or role | Finding | Classification |
| --- | --- | --- |
| ZON object resets | Sixteen O rows: nine import Player Castles objects #11054 (four placements), #11013 (one), and #11104 (four); seven reset Void objects #29101–#29106, including two placements of #29102. | Preserve the nine exact external imports and all local reset rows. The candidate ZON is not loaded from `areas/AREA`. |
| ZON mobile resets | Seventeen M rows: sixteen golem placements (#29100 three times and #29103 once in each of rooms #29100, #29110, #29113, and #29116) plus Drac #29101 in shop room #29103. | Preserve all commands and max values. Candidate MOB identities #29102, #29104, and #29105 have no same-stem M reset. |
| Keeper inventory | Fifteen G rows on Drac: #3050, #377, #398, #6089, #364, #53, #412, #347, #108, #109, #111, and #491–#494. | #3050 is in active `verz1`; #108/#109/#111/#347/#364/#377/#398/#412/#491–#494 are in active `heavens`; #53 has several competing OBJ sources; #6089 has no checked-in OBJ prototype. |
| SHP | One shop keyed by mobile #29101 at room #29103, with the same fifteen stock VNUMs. | `void_gh` is outside the normal AREA-based shop generator input, and no active AREA MOB source defines keeper #29101. If `areas/world.shp` is absent, `open_shop_stream()` falls back to concatenating `areas/shp/*.shp`, which includes this source; `areas/world.shp` was absent in this checkout. This conditional source-code path is not a live boot result. |
| Local teleport objects | Four type-25 objects: #29100 targets #29112, #29101 targets #29108, #29102 targets #29107, and #29103 targets #29115. ZON places them in rooms #29100, #29112, #29115/#29118, and #29107 respectively. | These destinations cross between the four WLD components if the object routes are usable. They do not prove runtime reachability. Objects #29102/#29103 point at rooms whose prose does not clearly describe those directions. |
| Other teleport targets | `afterlife_op.obj` targets #29106 and #29110; `afterlife_op2.obj` has three targets to #29109. | All are unregistered source declarations. Each numeric destination has multiple competing room records, so none identifies a Void-specific route. Torg's own object target #29117 resolves within active Torg. |
| Compiled room references | [`specs.assign.c`](../../../src/specs/specs.assign.c#L2448) binds an inn callback to raw room #29103; [`specs.torg.c`](../../../src/specs/specs.torg.c#L15) defines `INVASION_ROOM_VNUM` #29116, used by the invasion logic. | Both resolve to active Torg rooms. Torg #29116 is an invasion load room ([record](../../../areas/wld/torg.wld#L4586)); Void #29116 is a stairway ([record](../../../areas/wld/void_gh.wld#L296)), and Void #29103 is its shop. Neither reference establishes a Void callback or invasion room. Resolve code and room ownership before activation or range changes. |
| Quest source | There is no `void_gh.qst`. `afterlife_gh.qst` begins at giver #29101 and contains an Afterlife guildhall conversation; Afterlife MOB #29101 is a different record from Void's Drac. | A shared giver number is an identity conflict, not a Void quest link. No same-stem QST contract was found. |

The standard SHP generator reads `areas/AREA`
([make_shp.c](../../../areas/src/shp/make_shp.c#L6)); the runtime shop
fallback is in [`shop.c`](../../../src/economy/shop.c#L3267). Type 25 is
`ITEM_TELEPORT` ([defines.h](../../../src/core/defines.h#L140)). Static
source inspection did not qualify whether this checkout's fallback shop
record can be safely assigned without the matching Void mobile prototype.

## Disposition

Keep all five `void_gh` source files **RETAIN IN PLACE** outside
`areas/AREA` and `areas/qst/AREA`. Preserve the four WLD components, three
Duris3 boundary declarations, all overlapping room/mobile/object source
identities, nine Player Castles object imports, seven local object resets,
seventeen M commands, fifteen inventory references, the shop record, and the
raw #29103/#29116 code bindings. Do not activate, merge, renumber, regenerate, move,
or archive these files until guild/map ownership, the shared #291 namespace,
boundary intent and returns, object destinations, reset and shop identity,
editor/export authority, deployment, runtime, and persisted identity are
resolved. See [30 builder questions](VOID_GH_BUILDER_QUESTIONS.md), the
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-556-void-guildhall-title-history-and-shared-identities),
the [attic retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-556-void_gh-retained-with-source-only-boundaries-and-shared-identities),
[coverage row 469](ZONE_COVERAGE_REGISTER.csv#L470), and
[candidate dependency row 118](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L119).
