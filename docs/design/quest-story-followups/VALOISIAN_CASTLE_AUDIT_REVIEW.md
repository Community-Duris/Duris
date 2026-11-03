# The Valoisian Castle audit supplement

**Checkpoint 45 — bounded repository-wide connectivity, typed-reference,
code-reference, active-title, and tracked-path-history review.** The existing
[comprehensive source map](../zone-stories/THE_VALOISIAN_CASTLE.md) remains the
authority for local room, mobile, object, reset, quest, and journal details.
This supplement records cross-area findings without changing native sources.

## Active WLD boundary

The active Valoisian Castle map has one paired cross-area boundary. Room
#38575 exits south to active Surface room #521073, whose north exit returns to
#38575 ([Val exit](../../../areas/wld/val.wld#L3902),
[Val target](../../../areas/wld/val.wld#L3905),
[Surface return](../../../areas/wld/surface.wld#L442403),
[Surface target](../../../areas/wld/surface.wld#L442406),
[Val outbound ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L547),
[Surface return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1362)). This remains
the single active inbound and outbound record in the coverage register; all
374 authored Val exits resolve, with no active unresolved target.

## Source-only WLD reference into Val

A stateful parse of all 95 current unregistered WLD inputs read 2,080,773
authored `D0`–`D9` exits without parse issues. It found one exit into an active
Val room:

| Unregistered source | Authored exit | Evidence |
| --- | --- | --- |
| `surf.wld` room #205053, “A Frozen Tundra Forest” | North (`D0`) to Val room #38575 | [room](../../../areas/wld/surf.wld#L106077), [north exit](../../../areas/wld/surf.wld#L106081), [target](../../../areas/wld/surf.wld#L106084) |

No source-room VNUM in the 95 files overlaps an active Val room VNUM. The
`surf.wld` exit remains **source-only**, not active-world ingress: `surf` is
outside `areas/AREA`, so this record does not add an edge to the active WLD
graph. It brings the curated source-only list to 582 records; the full table
and its previous classifications are in
[WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md).

## Bounded typed and code references

The existing Val dossier's exact native scan records eight local recipes and
no foreign QST consumer of its local prototypes. No local `val.shp` or
external SHP prototype reference was found, and no other structured story
sidecar contains a Val prototype VNUM. The one identified foreign reset source
is a Verspin group that G-loads Val dagger #38421 on the Bloodhawk leader
#28113 alongside the forearm guard and four F-loaded followers
([Verspin reset](../../../areas/zon/verspin.zon#L370),
[dagger G record](../../../areas/zon/verspin.zon#L372)). This source reference
does not make the dagger an input to a local recipe or establish its live
availability. The comprehensive dossier's scan of all 713 active type-25
teleport objects found no destination into Val's local rooms.

An exact numeric scan of C/C++ source found 13 static `chaos_kit_item` slot
values for three local Val objects across 11 equipment arrays:

| Local object | Static array references |
| --- | --- |
| #38444, glowing bard sack | Standard Psionicist ([line 92](../../../src/account/chaos_eq_data.h#L92)) and Conjurer ([line 169](../../../src/account/chaos_eq_data.h#L169)); enhanceable Psionicist ([line 344](../../../src/account/chaos_eq_data.h#L344)), Shaman ([line 393](../../../src/account/chaos_eq_data.h#L393)), and Conjurer ([line 417](../../../src/account/chaos_eq_data.h#L417)) |
| #38450, elven bracelet | Standard and enhanceable optional slots ([line 567](../../../src/account/chaos_eq_data.h#L567), [line 572](../../../src/account/chaos_eq_data.h#L572)) |
| #38455, silk panties | Standard Monk ([line 127](../../../src/account/chaos_eq_data.h#L127)) and Necromancer ([line 160](../../../src/account/chaos_eq_data.h#L160)); enhanceable Monk ([line 377](../../../src/account/chaos_eq_data.h#L377)) and Necromancer ([line 409](../../../src/account/chaos_eq_data.h#L409)) |

The local prototypes are defined in
[`val.obj`](../../../areas/obj/val.obj#L647) (#38444),
[`val.obj`](../../../areas/obj/val.obj#L744) (#38450), and
[`val.obj`](../../../areas/obj/val.obj#L810) (#38455). These are static table
references only; they do not prove kit selection, object creation, or player
issuance. The bounded exact scan found no other direct C/C++ VNUM reference to
the Val room, mobile, or object ranges.

## Active title and tracked history

The normalized scan of all 442 current `.zon` titles identifies “The
Valoisian Castle” as one exact owner, `val`
([manifest entry](../../../areas/AREA#L186),
[zone title](../../../areas/zon/val.zon#L2)). Following the WLD and ZON history
shows the 2009 source-path relocation, later prose/editor-version changes, and
a 2016 ZON edit adding a steel-key reset described by its commit as an escape
addition (`6e473aad1`). The reviewed WLD history contains no active boundary
endpoint change, and the tracked history does not establish a predecessor or
replacement for the current `val` key. Herders mobile prose names Verspin and
Valoisian Castle as separate possible origins, not as alternate owners
([source line](../../../areas/mob/herders.mob#L677)); the comparison is
recorded in [Checkpoint 767](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-767-valoisian-castle-and-verspin-references).
The key/reset history is consistent with the source map's existing gate and
key records; it does not prove live availability or passage.

## Scope and disposition

Val remains an active `areas/AREA` contributor and is not a move or archive
candidate. Its authored WLD census remains 374/374 resolving exits, one paired
Surface boundary, and no unresolved targets. The one unregistered `surf.wld`
record is separate from those active counts. Actual access, door/reset state,
safe traversal and return, dynamic effects, portal or spell behavior, and
persisted-player outcomes remain unverified. This documentation-only review
did not run tests, a build, server, gameplay, database, migration, or
operational script.
## Checkpoint 872: current tracked-source refresh

A current scan against the pinned source snapshot reconfirms that val.zon has no reset command importing an outside mobile or object. The only active external ZON consumer of a Val prototype remains Verspin line 372, which G-loads dagger #38421 on Bloodhawk leader #28113. No outside QST or SHP prototype reference or structured story-sidecar VNUM consumer was found; the eight QST recipes remain local. The 13 C/C++ exact-number matches remain static chaos-kit slot values for #38444, #38450, and #38455 across 11 arrays, not evidence of item creation or issuance. The 713-active-object type-25 scan still finds no destination in Val's rooms.

The unregistered surf.wld north approach from #205053 to #38575 remains source-only and is already preserved on existing Surf candidate row 96 / physical CSV line 97. It does not add active ingress or change cumulative source-only totals. The existing 374/374 WLD graph, Surface reciprocal pair, three components, title/history finding, and DO NOT MOVE disposition remain unchanged. Installed/editor, persisted-player, and runtime qualification remain outside this source checkpoint.
