# oldmap8 source audit

Canonical key: `component:oldmap8`. Physical coverage row 431 / CSV line 432;
candidate dependency row 80 / CSV line 81. Reviewed October 11, 2026,
Checkpoint 518, against source snapshot
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static narrative,
connectivity, identity, and typed-reference review. It does not establish live
map loading, runtime path behavior, editor ownership, or safe migration.

## Source package, membership, and map content

The only same-stem source is [oldmap8.wld](../../../areas/wld/oldmap8.wld#L1).
No MOB, OBJ, ZON, SHP, QST, map-coordinate file, or story sidecar was found.
The WLD source is absent from [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA); the current
[RANDOM_AREA](../../../areas/RANDOM_AREA) file is empty. There is no ZON title
or header from which to infer an authored alias or reset policy. The filename
`oldmap8` is the only package label in this source set.

The WLD defines every VNUM from #180000 through #189999: a complete 100×100
room grid. Its 44 raw title strings describe named overworld regions, including
the Ocean of Gaolis, Sea of Lost Souls, Sea of Ulyra, Yellow Plains, Grey
Drylands, DragonClaw ranges, Werrun, Krethik Pass, Mount Banishment, Bastine
Castle, and the Forest of Tears. The file provides geographic setting and
cross-map routes, but contains no authored mobile, object, quest, or shop
records. The title evidence supports a regional world-map interpretation; it
does not establish who owns the file or whether it is meant to load.

## Local graph and reverse-exit gaps

The candidate has 39,918 directional exits: 9,978 north, 9,983 east, 9,975
south, and 9,982 west. All 10,000 room records belong to one weak component;
all targets that are in this WLD resolve locally. No fall (`F`) or current
(`C`) directives were found. There are 410 exit records whose target is
outside oldmap8's room set: 400 grid-edge crossings and ten additional
cross-map directions.

Eighty-eight within-file directions lack the opposite direction tuple. The
largest clusters are in Darkpeak Mountains (32) and Kur Mountains (31). Other
four-direction clusters occur at Werrun room #181362, `mount` room #184184,
Forestb room #187445, the DragonClaw foothills at #188134, and Bastine Castle
room #188272. Examples include #182452 south to #182552 at
[line 51409](../../../areas/wld/oldmap8.wld#L51409) and #182653 north to
#182553 at [line 55546](../../../areas/wld/oldmap8.wld#L55546). Their placement
at named terrain or route boundaries warrants builder review; the source
alone does not establish that they are mistakes.

## Outbound routes to current active owners

Active WLD sources contain zero target rows into #180000–#189999, and no
current active WLD owns any of those 10,000 room IDs. Thirteen oldmap8
directions target rooms with current active owners. These are source-only
approaches; the raw numeric target and active destination owner do not make
oldmap8 an active source or establish a return route.

| oldmap8 source direction | Current target owner | WLD line |
| --- | --- | ---: |
| #181462 north to #38300 | Werrun | [30695](../../../areas/wld/oldmap8.wld#L30695) |
| #182553 east to #20000 | Krethik | [53482](../../../areas/wld/oldmap8.wld#L53482) |
| #182654 north to #20085 | Krethik | [55567](../../../areas/wld/oldmap8.wld#L55567) |
| #184183 east to #9100 | Mountain | [87564](../../../areas/wld/oldmap8.wld#L87564) |
| #187735 west to #80600 | Yuan Ti2 | [162145](../../../areas/wld/oldmap8.wld#L162145) |
| #188135 west to #37409 | Mntcastl | [170537](../../../areas/wld/oldmap8.wld#L170537) |
| #188234 north to #37416 | Mntcastl | [172600](../../../areas/wld/oldmap8.wld#L172600) |
| #188271 east to #7600 | Bastine | [173381](../../../areas/wld/oldmap8.wld#L173381) |
| #189900 south to #120000 | SurfaceKeeps | [207586](../../../areas/wld/oldmap8.wld#L207586) |
| #189909 south to #120009 | SurfaceKeeps | [207775](../../../areas/wld/oldmap8.wld#L207775) |
| #189910 south to #120010 | SurfaceKeeps | [207796](../../../areas/wld/oldmap8.wld#L207796) |
| #189911 south to #120011 | SurfaceKeeps | [207817](../../../areas/wld/oldmap8.wld#L207817) |
| #189912 south to #120012 | SurfaceKeeps | [207838](../../../areas/wld/oldmap8.wld#L207838) |

The existing world-connectivity recount already records 13 eligible
source-only oldmap8 directions under its exact active-owner rule. Do not add
these again: the global total remains 11,987 (415 from Surf). The existing
route ledger documents earlier subsets; this table closes the remaining
source-local inventory without changing those totals.

The 410 external target rows divide by current WLD definitions as follows:
300 target records have a room definition in the separate unregistered
[`Duris3.wld`](../../../areas/wld/Duris3.wld#L1735252), of which five are also
owned by active SurfaceKeeps (#120000 and #120009–#120012); eight other rows
target the active owners in the table above; 102 have no destination room in
another `areas/wld/*.wld` file. The 102 include the 100-room target band
#199900–#199999. The Duris3 numeric definitions do not turn these exits into
oldmap8-local or active routes. The ten additional non-boundary crossings are
the eight active-owner approaches plus #182635 north to otherwise undefined
#33933 ([line 55176](../../../areas/wld/oldmap8.wld#L55176)) and #187444 east
to otherwise undefined #38281
([line 156033](../../../areas/wld/oldmap8.wld#L156033)).

## Same-number identity with Duris3

The unregistered Duris3 WLD also defines every VNUM #180000–#189999, so the
two source files have 10,000 same-number room pairs. None has the same raw
title or identical complete room block. For example, oldmap8 #182631 is
`The Vast Grey Drylands` while Duris3 #182631 is `The Ocean`
([oldmap8 line 55088](../../../areas/wld/oldmap8.wld#L55088),
[Duris3 line 1735252](../../../areas/wld/Duris3.wld#L1735252)). Across these
10,000 sources, 19,759 exact direction/source/target tuples occur in both
files; 20,159 oldmap8 direction tuples and 20,241 Duris3 tuples do not match
the other file's tuples. The common tuples coexist with different titles,
room blocks, and neighboring VNUMs. They are evidence of related grid
structure, not proof of a copy or successor.

Duris3's same-number routes remain authored in the separate Duris3 WLD; they
are not active ingress into oldmap8. The current active room table owns none
of the shared VNUMs. The [Duris3 audit](DURIS3_AUDIT_REVIEW.md) separately
records its broader room and active-owner conflicts. Keep the two map
identities distinct unless their owner supplies a documented relationship.

## Typed dependencies and bounded history

The 351 current active area WLD files have no exit targeting the oldmap8 room
set. Active ZON room-placement and door-state commands have zero room targets
in #180000–#189999. Among 713 active `ITEM_TELEPORT` prototypes scanned,
none has a destination in this range. No direct `oldmap8` code reference was
found. Two raw `R E` values in legacy QST files are #181000 and #185000
([smokev](../../../areas/qst/smokev.qst#L584),
[Alatorin](../../../areas/qst/alatorin.qst#L285)), but the parser maps `R E`
to an experience reward (`QUEST_GOAL_EXP`), not a room
destination ([quest.c lines 2243–2283](../../../src/world/quest.c#L2243)).
Room identity in editor projects, external deployment data, and persisted
state was not observable in this source review.

`git log --follow` shows the oldmap8 WLD path move in 2009 and a 2013 same-path
`stuff` commit. Duris3's current large WLD appears in 2011 and has a later
2026 line-ending-only commit. No explicit rename, successor, retirement, or
whole-map replacement decision was found. The later oldmap8 timestamp and
large shared VNUM/edge set do not by themselves prove it replaces or is
replaced by Duris3.

**Disposition: RETAIN IN PLACE.** Preserve the 10,000-room regional map, all
13 already-counted active-owner approaches, the cross-file Duris3 identity,
and the 88 within-file reverse gaps pending owner review. Do not move,
activate, merge, renumber, or archive either same-number map based only on
static source evidence. Resolve map provenance, grid/editor lineage, the
one-way terrain crossings, the 102 currently undefined destinations, route
counterpart intent, and persisted references before a separate migration
decision. No migration clearance is implied. See the
[20 builder questions](OLDMAP8_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md),
[candidate dependency row 80](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81),
and [coverage row 431](ZONE_COVERAGE_REGISTER.csv#L432).
