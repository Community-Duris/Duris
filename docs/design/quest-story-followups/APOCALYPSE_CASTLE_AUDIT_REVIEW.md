# Apocalypse Castle: world, history, and dependency supplement

**Checkpoint 99.** This repository-wide pass supplements the [source-comprehensive
Apocalypse Castle dossier](../zone-stories/APOCALYPSE_CASTLE.md). It reconciles
the active and inactive WLD graph, exact typed consumers, tracked source
history, and the Surf attic-candidate plan. No native gameplay, area source,
database state, or zone location was changed.

## Active connectivity and reachability limits

The active [`areas/AREA` entry](../../../areas/AREA#L167) loads `4horse` at
zone 345. The current all-active-WLD scan parsed all 351 contributors and
253,262 room VNUMs. Apocalypse Castle has 312 rooms and 777 authored exits;
all targets resolve in the active room set, with no parse issues. There are
776 internal exits and one outbound exit. The undirected local graph has weak
components of 254, 57, and one room. The coverage census records two
fall-chance markers and no current fields. These static counts do not prove
directed reachability, door state, fall safety, or a played return route.

Two active WLD exits arrive in the zone. Surface room #515793 enters castle
room #34500, and that castle exit returns south to Surface room #515793. A
separate Realm room #14210 has a south exit into vault room #34804 with no
reciprocal castle WLD exit. Preserve the Realm arrival as its own boundary;
the source dossier describes it as a Staff Realm lead, not an ordinary player
route.

| Boundary | Source evidence | Register row |
| --- | --- | --- |
| Realm #14210 south to castle #34804 | [`realm.wld` exit](../../../areas/wld/realm.wld#L4742) | [#L259](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L259) |
| Surface #515793 north to castle #34500 | [`surface.wld` exit](../../../areas/wld/surface.wld#L331598) | [#L1337](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1337) |
| Castle #34500 south to Surface #515793 | [`4horse.wld` exit](../../../areas/wld/4horse.wld#L16) | [#L460](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L460) |

The Surface pair is reciprocal; the Realm edge is a one-way authored
arrival. The key/door, portal, fountain, slab, climbing, fall, secret-route,
and epic behavior qualifications remain in the main dossier. Actual access,
reset state, and safe return remain unqualified.

## Inactive WLD sources and map identity

The current scan parsed all 95 inactive WLD files: 527,791 rooms and 2,080,773
exits, with no parse issues and no source-room VNUM overlap with the castle.
It found one non-overlapping source-only approach, in `surf.wld`:

| Source-only exit | Source evidence |
| --- | --- |
| #271560 north to Apocalypse Castle #34500 | [`surf.wld` exit](../../../areas/wld/surf.wld#L1501636) |

This adds one curated source-only record: the reviewed Surf subset rises from
128 to 129 and the global curated source-only total from 1,512 to 1,513. It
remains an inactive-source declaration, not active ingress. Preserve it with
the parallel Surface records until map ownership and intended route history
are confirmed; do not infer a playable boundary from the VNUM match.

## Typed references and outside consumers

The local source set has 312 rooms, 71 mobiles, and 72 objects. Exact typed
scans found these outside references:

| Source | Reference into Apocalypse Castle | Classification |
| --- | --- | --- |
| Active `wh.qst` | `G I 34554` at [line 282](../../../areas/qst/wh.qst#L282), and `G I 34541` at [line 1177](../../../areas/qst/wh.qst#L1177) | Volo's cloak exchange and Sootfoot's Maelstrom armor exchange consume exact local objects. Their full recipes and giver identities are documented in the main dossier. |
| Active `wh.story.json` | Typed `item_vnums` #34554 at [line 870](../../../areas/story/wh.story.json#L870), and #34541 at [line 903](../../../areas/story/wh.story.json#L903) | Two structured item references. All 231 external story sidecars parsed; the six matching numeric tokens in `wh.story.json` include completion-key strings, not additional typed item dependencies. |
| Active foreign ZON resets | Twelve `G` reset-role rows for local object #34534: eight in `barovia2.zon` beginning at [line 363](../../../areas/zon/barovia2.zon#L363), and four in `ravenloft2.zon` beginning at [line 1326](../../../areas/zon/ravenloft2.zon#L1326). One `G` row for local object #34541 is in `vehicles.zon` at [line 839](../../../areas/zon/vehicles.zon#L839), under the Drak'Azath reset group. | Active external reset declarations. They do not establish that the prototypes loaded or that a live reset produced these items. |

No active outside SHP product row references a local object, and this zone has
no local `.shp` file. The two local ZON `O` rows for imported objects #360
and #51401 are at [lines 230–231](../../../areas/zon/4horse.zon#L230);
their effective epic/object handling is mapped in the main dossier.

The exact-number C/C++ scan found two enabled object bindings: #34545 to
`mankiller` at [`specs.assign.c` line 1546](../../../src/specs/specs.assign.c#L1546),
and #34559 to `brainripper` at
[`specs.assign.c` line 1585](../../../src/specs/specs.assign.c#L1585). The third
zone-context match is the #34804 entry in the `skill_beacon` table at
[`specs.character_progression.c` line 48](../../../src/specs/specs.character_progression.c#L48);
the main dossier records that table as unbound in the effective callback
inventory. One other exact-number match in `nanny.c` is example text, not a
zone-VNUM consumer. No external type-25 portal target among 869 scanned
declarations points into this zone, and no external MOB or OBJ prototype
duplicates a local VNUM.

Local outbound scans found no QST item command or typed sidecar item reference
outside the local object set. The local ZON's two imported-object reset rows
are the only exact external prototype references found in those local typed
fields. These source declarations remain distinct from successful loaded
resets, live stock, or a played quest outcome.

## Tracked history and replacement boundary

The normalized active `.ZON` title `Apocalypse Castle` is unique among the 351
current non-empty active titles. `4horse.wld` first appears as a tracked path
addition in commit `642da5400` (2009-08-12), with later world edits through
2013. The followed WLD history contains access and room adjustments, but no
rename to a different zone stem or explicit replacement claim. This is not
evidence that an untracked predecessor never existed; no distinct predecessor
or superseded source was established in the history reviewed. Keep the active
zone identity separate from the source-only `surf.wld` approach.

### Builder questions and recommended resolution

- **What is the intended status of the Surf room #271560 approach?** Preserve
  the exact source record with the other Surf map references until its map
  lineage and route owner are confirmed. Do not promote it to active ingress.
- **Should the Realm arrival into vault #34804 be available to ordinary
  players?** Keep it distinct from the reciprocal Surface route and from the
  vault's configured portal. Confirm the boundary's intended audience and
  qualify access and return before making reachability claims.
- **Are the foreign reset rows for local items #34534 and #34541 intentional
  shared sources?** Preserve the Barovia, Ravenloft, and Vehicles reset rows;
  confirm their reset ownership and availability before describing them as
  reliable item sources.
- **Does the zone have a distinct predecessor or replacement?** No such
  relation is established by the tracked history or normalized active-title
  comparison. Preserve current source identity unless builders supply
  separate provenance evidence.

Apocalypse Castle remains **DO NOT MOVE**. Preserve its reciprocal Surface
boundary, separate Realm arrival, one already-counted inactive Surf approach,
active outside quest and reset consumers, and imported local reset
prototypes. Continue to qualify doors, travel, falls, shared stock, reset
availability, and safe return through source evidence or a separate gameplay
journey.
