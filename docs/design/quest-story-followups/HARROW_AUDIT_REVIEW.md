# Harrow - The Gnome Village audit supplement

**Checkpoint 46 — active and source-only connectivity, exact typed-reference,
code-reference, title, and tracked-path-history review.** The existing
[comprehensive source map](../zone-stories/HARROW_THE_GNOME_VILLAGE.md) remains
the authority for local story, quest, room, mobile, object, reset, shop, and
travel details. This supplement records bounded cross-area evidence without
changing native sources.

## Active WLD boundary

Room #29400 has two reciprocal Surface connections: south (`D2`) to Surface
room #547676, which returns north (`D0`), and west (`D3`) to Surface room
#547275, which returns east (`D1`). The four declarations are the two active
inbound/outbound pairs in the connectivity register ([Harrow south exit](../../../areas/wld/harrow.wld#L15),
[Harrow south target](../../../areas/wld/harrow.wld#L18),
[Harrow west exit](../../../areas/wld/harrow.wld#L19),
[Harrow west target](../../../areas/wld/harrow.wld#L22),
[Surface east return](../../../areas/wld/surface.wld#L992439),
[Surface east target](../../../areas/wld/surface.wld#L992442),
[Surface north return](../../../areas/wld/surface.wld#L1000848),
[Surface north target](../../../areas/wld/surface.wld#L1000851), [Harrow
ledger rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L414),
[second Harrow row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L415),
[Surface ledger rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1436),
[second Surface row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1438)). All 187
authored Harrow exits resolve. The local graph has three weak components, with
77 rooms in its largest component; the isolated lucky-star component remains
an unresolved access question in the source map.

## Unregistered WLD target matches

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored exits with zero parse issues. Harrow has 82 active room records and
187 active exits, also with zero parse issues. The scan found 111 unregistered
exit targets whose numeric values match active Harrow rooms:

| Unregistered source | Numeric target matches | Source-room VNUMs overlapping Harrow | Classification |
| --- | ---: | ---: | --- |
| [`dark.wld`](../../../areas/wld/dark.wld) | 36 | 10 | Source rooms reuse Harrow's active room VNUMs; map ownership is ambiguous. |
| [`darkop.wld`](../../../areas/wld/darkop.wld) | 32 | 19 | Source rooms reuse Harrow's active room VNUMs; map ownership is ambiguous. |
| [`infernal_gh.wld`](../../../areas/wld/infernal_gh.wld) | 26 | 16 | Source rooms reuse Harrow's active room VNUMs; map ownership is ambiguous. |
| [`bsghop.wld`](../../../areas/wld/bsghop.wld) | 8 | 4 | Source rooms reuse Harrow's active room VNUMs; map ownership is ambiguous. |
| [`sky_gh.wld`](../../../areas/wld/sky_gh.wld) | 8 | 5 | Source rooms reuse Harrow's active room VNUMs; map ownership is ambiguous. |
| [`surf.wld`](../../../areas/wld/surf.wld) | 1 | 0 | The source room is outside Harrow's active room VNUM set. |

The first five files account for 110 number matches from rooms whose own
VNUMs collide with Harrow's active room set. These records do not identify
Harrow as the destination map. The other record is a north exit from unlisted
`surf.wld` room #214566, “The Footsteps of Clangeddin Silverbeard,” to active
Harrow room #29400 ([source room](../../../areas/wld/surf.wld#L305638),
[north exit](../../../areas/wld/surf.wld#L305642),
[target](../../../areas/wld/surf.wld#L305645)). `surf` is outside
`areas/AREA`, so this authored exit remains a source-only route candidate. It
raises the curated source-only count from 582 to 583 and the reviewed `surf.wld`
subset from 22 to 23. Across all 95 files, 54 source-room records reuse an
active Harrow room VNUM; `surf.wld` has no such overlap.

The BSGHOP supplement separately compares its room, mobile, and object records against Harrow's
active #29440–#29443 owners and records the copied darkop room text. The eight WLD number matches
are identity-ambiguous source data, not proof of live BSGHOP access; see the
[BSGHOP audit](BSGHOP_AUDIT_REVIEW.md#conflicting-numeric-ownership-and-copied-room-text).

## Bounded typed and code references

The exact active-source scan found no external QST, ZON reset, or SHP records
with a numeric Harrow room, mobile, or object reference. One active Tundra
story-sidecar contract records the optional Bom painting exchange: giver mobile
#29444 accepts Harrow painting #29440 and supplies fish #318, #319, and #330
([Tundra contract](../../../areas/story/tundra.story.json#L406),
[Harrow native recipe](../../../areas/qst/harrow.qst#L114)). The sidecar
preserves Harrow's existing optional foreign supply route; it does not add a
native Harrow recipe.

Exact matches in unregistered reset and shop files are source-only. The
unregistered `brass-old-1.zon` has a `G` reset for Harrow flute #29479 at line
887 ([reset](../../../areas/zon/brass-old-1.zon#L887),
[Harrow flute prototype](../../../areas/obj/harrow.obj#L893)); the reset file is
outside `areas/AREA` and does not establish current placement. Other inactive
reset matches come from [bsghop](../../../areas/zon/bsghop.zon),
[dark](../../../areas/zon/dark.zon),
[darkop](../../../areas/zon/darkop.zon),
[infernal_gh](../../../areas/zon/infernal_gh.zon), and
[sky_gh](../../../areas/zon/sky_gh.zon), whose WLD rooms and local prototype
files reuse the same numeric ranges. Inactive shop matches appear in
[dark.shp](../../../areas/shp/dark.shp),
[infernal_gh.shp](../../../areas/shp/infernal_gh.shp), and
[sky_gh.shp](../../../areas/shp/sky_gh.shp). These numeric matches remain
separate from active Harrow dependencies.

An exact C/C++ scan found 83 static `chaos_kit_item` slot values for seven
Harrow object prototypes in `chaos_eq_data.h`:

| Object | Static slot values |
| --- | ---: |
| [#29401, large bag](../../../src/account/chaos_eq_data.h#L254) | 12 |
| [#29404, lucky alchemist sack](../../../src/account/chaos_eq_data.h#L76) | 44 |
| [#29424, gnomish ear amplifier](../../../src/account/chaos_eq_data.h#L83) | 2 |
| [#29437, tinkerer goggles](../../../src/account/chaos_eq_data.h#L186) | 18 |
| [#29443, hand-carved yo-yo](../../../src/account/chaos_eq_data.h#L204) | 3 |
| [#29458, ruby earrings](../../../src/account/chaos_eq_data.h#L135) | 2 |
| [#29459, gem choker](../../../src/account/chaos_eq_data.h#L307) | 2 |

These are equipment-table values; they do not establish kit selection,
creation, or player issuance. Other direct C/C++ references are the local
collapsible fishing pole #29439 in the recognized fishing-pole list
([trade-skill selector](../../../src/economy/tradeskill.c#L575)) and the inn
function assignment to room #29403
([special assignment](../../../src/specs/specs.assign.c#L2454)). The exact
numeric scan found no other direct C/C++ VNUM references in Harrow's room,
mobile, or object ranges. The existing source map's scan of all 713 active
type-25 teleport objects found no foreign fixed destination into Harrow's
lucky-star rooms.

## Active title and tracked history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“Harrow -The Gnome Village” owner: `harrow`. The current title also uniquely
contains “Harrow” and “Gnome” among the scanned titles. Both current indexes
list `harrow *294-296 (haruko)` ([full-area entry](../../../areas/AREA#L142),
[QST entry](../../../areas/qst/AREA#L132),
[zone title](../../../areas/zon/harrow.zon#L2)). The same parenthetical note
appears on adjacent active `newbie *292-293 (haruko)`, whose title is “Ailvio,
Duris Newbie Outpost” ([full-area entry](../../../areas/AREA#L141),
[QST entry](../../../areas/qst/AREA#L131),
[Newbie title](../../../areas/zon/newbie.zon#L2)). The shared note and adjacent
ranges do not by themselves identify a Harrow alias, predecessor, or
replacement.

Followed WLD history reaches the 2009 area-tree move and later room-title and
prose edits; the reviewed changes do not alter Harrow's exit endpoints.
Followed ZON history changes the editor-version comment and reset display
wording while preserving reset values. No alternate-stem predecessor,
retirement, or whole-zone replacement was found. The story sidecar was added
on 2026-10-04. The command-target alias and area-owner question remains open.

## Scope and disposition

Harrow remains an active `areas/AREA` contributor. Its four active Surface
boundary declarations remain in the reciprocal ledger, while the `surf.wld`
record stays a source-only candidate. The 110 colliding numeric targets retain
ambiguous map ownership. Actual access, door/reset state, safe traversal and
return, dynamic effects, and persisted-player outcomes remain unverified. This
documentation-only review did not run tests, a build, server/runtime/gameplay,
database, migration, or operational script.
