# Kimordril: connectivity, dependency, and history supplement

**Checkpoint 26; expanded by Checkpoint 97.** The original review adds
source-only WLD routes, bounded typed dependency evidence, and tracked
endpoint history to the [Kimordril dossier](../zone-stories/KIMORDRIL.md).
The Checkpoint 97 section below reconciles those findings with the current
repository-wide graph, external-reference, and attic registers. No area
source, active-list membership, or runtime behavior was changed or qualified.

## Active and source-only connectivity

Kimordril remains active zone 955. Its existing three reciprocal Surface
boundaries are recorded in the exact cross-area exit ledger:

| Active pair | Evidence |
| --- | --- |
| Kimordril #95502 west ↔ Surface #543256 east | [Kimordril exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1147); [Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1421) |
| Kimordril #95655 east ↔ Surface #542862 west | [Kimordril exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1148); [Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1419) |
| Kimordril #95658 south ↔ Surface #543661 north | [Kimordril exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1149); [Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1423) |

An exact-target scan of all 95 current WLD files outside the active
[area list](../../../areas/AREA) found exactly two records pointing into
Kimordril. Both are in unregistered `surf.wld`; they are source-only
references, not active-world ingress:

| Source-only record | Active target | Evidence |
| --- | --- | --- |
| `surf.wld` room #216660 east (`D1`) | Kimordril #95502 | [Source room](../../../areas/wld/surf.wld#L349544); [exit target](../../../areas/wld/surf.wld#L349555) |
| `surf.wld` room #216664 west (`D3`) | Kimordril #95655 | [Source room](../../../areas/wld/surf.wld#L349628); [exit target](../../../areas/wld/surf.wld#L349647) |

The same-numbered rooms in unregistered `surface2011.wld` and
`underdark2011.wld` do not point into active Kimordril. Their `#216660` east
exits lead to `#216661`; their `#216664` west exits lead to `#216663`:

| Source file | `#216660` east (`D1`) | `#216664` west (`D3`) |
| --- | --- | --- |
| `surface2011.wld` | [Room](../../../areas/wld/surface2011.wld#L349861), [D1 target](../../../areas/wld/surface2011.wld#L349872) | [Room](../../../areas/wld/surface2011.wld#L349945), [D3 target](../../../areas/wld/surface2011.wld#L349964) |
| `underdark2011.wld` | [Room](../../../areas/wld/underdark2011.wld#L61317), [D1 target](../../../areas/wld/underdark2011.wld#L61328) | [Room](../../../areas/wld/underdark2011.wld#L61401), [D3 target](../../../areas/wld/underdark2011.wld#L61420) |

Their bodies also describe different map cells: `surf.wld` calls the two
rooms “Steep Trail Through the Mountains of Clangeddin Silverbeard” and “The
Footsteps of Clangeddin Silverbeard,” while the parallel files use stormy-sea,
grassland, and underdark-cavern descriptions. These records show divergent
source-map content; they do not establish a runtime route or identify one map
file as Kimordril's predecessor.

## Tracked boundary history and replacement evidence

Kimordril's normalized active title is unique in the current active list. Git
history records commit `71d731c5b` (2013-11-13), which changes the west exit
from Kimordril room #95502 from #214592 to the current Surface room #543256.
The current target is visible in
`kimordril.wld` ([source line](../../../areas/wld/kimordril.wld#L64)). That
commit contains one WLD data-line change and a file-mode change; it does not
identify a replacement area or predecessor.

The old number #214592 remains declared in three unregistered map sources, but
with different descriptions: “A Bubbling Lake of Lava” in
`surf.wld` ([source](../../../areas/wld/surf.wld#L306184)), “An Immaculate
Highway Paved with Obsidian” in
`surface2011.wld` ([source](../../../areas/wld/surface2011.wld#L306433)), and
“The Massive Obsidian Walls the Underdark” in
`underdark2011.wld` ([source](../../../areas/wld/underdark2011.wld#L49662)).
The parallel numeric records do not establish which map, if any, supplied the
former endpoint or demonstrate that Kimordril replaced another zone.

**Builder question:** Which, if any, of the unregistered map families preserve
an intended historical route or context for Kimordril's old #214592 boundary
and the two source-only `surf.wld` links? **Recommendation:** retain
Kimordril and all three unregistered map sources in place; keep the active
Surface pairs as the current static boundary evidence until their provenance
is established.

## Bounded typed dependencies

The existing source dossier records 11 matching foreign reset groups, one
fully reviewed matching foreign shop, two complete outside Alatorin QA blocks,
and 18 exact compiled-reference rows. It distinguishes active callbacks,
room and justice registrations, gear/tool roles, and commented or unrelated
numeric context instead of treating every matching number as a dependency.
For example, General #95535's equipped hammer #95526 is accepted by two
outside Alatorin QA recipes; that is separate from Kimordril's local supper
and skin trades. See the dossier's
[outside-boundary review](../zone-stories/KIMORDRIL.md#imported-and-outside-boundaries)
and [compiled-reference evidence](../zone-stories/KIMORDRIL.md#evidence-and-validation).

This is bounded source evidence, not an exhaustive inventory of editor and
builder tooling, user-controlled persisted references, or indirect and
dynamic lookups. Those broader dependency questions and installed/gameplay
qualification remain open. The active contributor remains “do not move,” and
this review makes no archival or source-list recommendation.

## Checkpoint 97: repository-wide graph and dependency reconciliation

The current un-commented [`areas/AREA`](../../../areas/AREA) manifest still
loads Kimordril. Its 159 rooms have 351 authored exits: 348 stay within the
zone and three leave for Surface. The all-active-WLD scan resolved all 351
targets without parse issues; the undirected authored graph has one weak
component containing all 159 rooms. Each Kimordril exit has a reciprocal
Surface return, at Kimordril #95502 west, #95655 east, and #95658 south
([cross-area rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1147),
[#L1148](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1148),
[#L1149](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1149)). The reverse Surface
rows are recorded at [#L1421](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1421),
[#L1419](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1419), and
[#L1423](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1423). The coverage census
reports no fall-chance or current fields. These are authored graph facts, not
proof that doors, portals, resets, or ordinary player travel succeed.

The current scan parsed all 95 inactive WLD files: 527,791 rooms and
2,080,773 exits, with no parse errors and no source-room VNUM overlap with
Kimordril. It reconfirmed the same two non-overlapping `surf.wld` records
already documented above: #216660 east to #95502 and #216664 west to #95655.
Their same-numbered `surface2011.wld` and `underdark2011.wld` counterparts
stay in those maps, leading to #216661 and #216663. This recheck adds no new
source-only reference: the two Kimordril records were included in Checkpoint
26's curated total and in the current reviewed Surf subset of 125; the global
curated source-only total remains 1,509.

### Typed references and outside consumers

The current exact-item scan found two active Alatorin QST `G I` records for
General's silverstone hammer #95526
([first](../../../areas/qst/alatorin.qst#L5484),
[second](../../../areas/qst/alatorin.qst#L7143)). The active
`player_castles.shp` shop #11005 stocks objects #95547 and #95540
([first](../../../areas/shp/player_castles.shp#L7),
[second](../../../areas/shp/player_castles.shp#L12)). Twelve active foreign
ZON reset-role records across Alatorin, Player Castles, and Surface Keeps
reference Kimordril objects or mobile #95531; representative groups are
[Alatorin](../../../areas/zon/alatorin.zon#L1485),
[Player Castles](../../../areas/zon/player_castles.zon#L16), and
[Surface Keeps](../../../areas/zon/surfacekeeps.zon#L123). No external mobile
or object prototype definition duplicates a local VNUM. Across 872 external
type-25 declarations (713 active and 159 source-only), none targets a
Kimordril room.

Two structured external sidecar `item_vnums` references point to #95526 in
Alatorin's story data
([first](../../../areas/story/alatorin.story.json#L2807),
[second](../../../areas/story/alatorin.story.json#L3851)). The deeper numeric
scan found ten token matches in those Alatorin transactions, including the
two typed fields and their step/completion identifiers, plus one #95572 token
embedded in a `wh.story.json` story ID; they add no other typed prototype
references. All 231 external story sidecars parse without errors. The
exact-number scan returned 18 C/C++ match lines across nine files. The
existing dossier context-classifies the callback, justice, equipment,
portal, banking, mail, and generic table hits; numeric coincidence is not an
additional world route.

### Outbound and source-only shop dependencies

Kimordril's local ZON has 23 reset-role rows to 21 externally defined object
VNUMs, and its local SHP has 15 product rows using external object
prototypes. The source dossier maps these imported reset and stock roles.
Local QST item commands and typed sidecar `item_vnums` add no outbound
references beyond local prototypes.

Two unregistered SHP families also reference active Kimordril objects:

| Source-only records | Evidence |
| --- | --- |
| `nexus56.shp` shop #83032 stocks #95510 and #95508; `nexus56.zon` has matching `G` resets | [shop #95510](../../../areas/shp/nexus56.shp#L251), [shop #95508](../../../areas/shp/nexus56.shp#L252), [reset #95510](../../../areas/zon/nexus56.zon#L1326), [reset #95508](../../../areas/zon/nexus56.zon#L1327) |
| `player_guilds.shp` has nine product rows across shops #34202, #34301, #34402, #35000, #35050, #35101, #35554, and #35952, for objects #95547, #95540, and #95536 | [#34202](../../../areas/shp/player_guilds.shp#L7), [#34301](../../../areas/shp/player_guilds.shp#L71), [#34402](../../../areas/shp/player_guilds.shp#L126), [#34402](../../../areas/shp/player_guilds.shp#L127), [#35000](../../../areas/shp/player_guilds.shp#L183), [#35050](../../../areas/shp/player_guilds.shp#L245), [#35101](../../../areas/shp/player_guilds.shp#L305), [#35554](../../../areas/shp/player_guilds.shp#L575), [#35952](../../../areas/shp/player_guilds.shp#L745) |

Neither SHP family is listed in `areas/AREA`. This checkout has no
`areas/world.shp`; `open_shop_stream()` can fall back to globbing all SHP
files ([loader](../../../src/economy/shop.c#L3267)). That source path is
possible parser exposure, not proof of an operating shop. Keep these exact
rows visible in the [Nexus56 attic plan](ATTIC_MIGRATION_PLAN.md#L40) and
[`player_guilds` attic plan](ATTIC_MIGRATION_PLAN.md#L29) until
their owners and the fallback behavior are resolved.

**Builder question:** Are the Nexus56 and Player Guilds stock/reset records
intentional sources for Kimordril objects, or retained data with only
conditional shop-reader exposure? **Recommendation:** retain both source
groups, preserve the exact references in any separate attic proposal, and
defer any archive decision until the source owner confirms intent and the
shop-reader path is qualified. Kimordril remains **DO NOT MOVE**. The current
Surf migration record must also preserve its two already-counted approaches
and all three reciprocal active Surface pairs.
