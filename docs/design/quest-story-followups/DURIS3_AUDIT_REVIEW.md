# `Duris3` WLD source audit

Canonical key: `component:Duris3`. Physical coverage row 386 / CSV line 387;
candidate dependency row 36. Reviewed October 11, 2026, Checkpoint 473. This is
a static source, graph, history, and reference review. It does not establish
runtime loading, map ownership, safe activation, or migration approval.

## Source scope and map identity

The only tracked source file for this component is
[`Duris3.wld`](../../../areas/wld/Duris3.wld). It is absent from the full
[`areas/AREA`](../../../areas/AREA) and
[`areas/qst/AREA`](../../../areas/qst/AREA) manifests and has no companion
MOB, OBJ, ZON, SHP, QST, or story-sidecar file in this checkout. There is no
ZON title or declared numeric zone range. The WLD contains 90,000 rooms from
#100000 through #189999.

The room titles are predominantly terrain labels: “The Ocean” appears 64,366
times, “Grassy Plains” 8,828, “Rolling Hills” 5,098, “Mountains” 4,326, and
“Thick Forest” 3,598. Every room has one each of D0–D3, for 360,000 authored
directions. All 360,000 targets are VNUMs also declared in this same WLD.
The candidate graph has one weak component, and ordinary directed traversal
from #100000 reaches all 90,000 candidate rooms. The source contains no
`F` fall-chance or `C` current records. This describes the file graph; it
does not show that the map is loaded or reachable in the game.

## Active VNUM collisions and map boundaries

There are 22,064 room VNUMs shared with 27 active WLD source files. None of
the paired rooms has an equal normalized title or identical complete source
block. For example, Duris3 #100000 is “The Ocean,” while active `random.wld`
#100000 is “Unnamed” ([Duris3](../../../areas/wld/Duris3.wld#L1),
[`random.wld`](../../../areas/wld/random.wld#L1)). Duris3 #120000 is
“The Ocean,” while active SurfaceKeeps #120000 is “An Ancient Stone Road”
([Duris3](../../../areas/wld/Duris3.wld#L420001),
[SurfaceKeeps](../../../areas/wld/surfacekeeps.wld#L1)). Treat these as
different room identities sharing numbers; do not infer duplicate content or
replacement from the VNUM overlap.

The 360,000 candidate edges have no destination outside the Duris3 room set.
Separately, 88,256 target VNUMs also occur in active WLD tables; those are raw
number matches inside the Duris3 map graph, not authored exits out of its
candidate room set. Of these, 80,354 start at Duris3 VNUMs that also have an
active WLD owner, while 7,902 start at Duris3 VNUMs with no active owner.
This checkpoint does not convert the broad raw matches into active routes or
add them to curated approach totals. Existing zone-specific Duris3 findings
remain as previously documented.

The active unresolved-exit register has 26 WLD directions from 14 full-area
sources whose target VNUMs occur in Duris3 and have no active WLD room owner.
They point to 24 distinct candidate rooms. Examples include SurfaceKeeps
#120000 north to Duris3 #129900 and #120000 west to #120099
([active exits](../../../areas/wld/surfacekeeps.wld#L8),
[Duris3 #129900](../../../areas/wld/Duris3.wld#L627901),
[Duris3 #120099](../../../areas/wld/Duris3.wld#L422080)). The full 26-record
source list is in the [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv).
Those active-source edges remain unresolved under the current full-area
manifest; their target records in this unregistered map do not make them
loaded connections. If the 24 targets were traversed in the candidate graph,
they collectively reach all 90,000 Duris3 rooms, but that conditional graph
result is not current in-game access.

## History, references, and disposition

Followed Git history shows the 2009 area-tree move, a 2011 commit adding the
current 1.89-million-line WLD, and a 2026 line-ending normalization. It shows
no alternate-stem predecessor or explicit whole-map replacement. The history
does not identify the map owner, its authoring or regeneration process, or
whether the file is a retained alternate map.

This WLD-only review does not close code, editor, generator, plugin, saved-room,
or persisted-player references across a 90,000-VNUM span. Keep typed dependency
and external-tool closure open. Preserve the 26 current active-source targets
and all previously reviewed Duris3 map records while owners reconcile the
map’s intended identity, VNUM collisions, source-list status, and entry routes.
Keep candidate row 36 **RETAIN IN PLACE**; do not activate, archive, rename, or
merge the map based on these static matches. See the
[20 builder questions](DURIS3_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-473-duris3-map-identity-history-and-replacement),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-473-duris3-wld-graph-and-map-identity),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-473-duris3-retained-pending-owner-and-map-identity-review),
[candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and
[coverage row 386](ZONE_COVERAGE_REGISTER.csv#L387). No area/source files,
tests, builds, runtime, server, database, or container were changed or used.

## Checkpoint 502 supplement: type-1 random-lab link to map22

Static code review found an additional, conditional code binding to this
unlisted WLD. `reset_lab(1)` uses `map22` room VNUMs #900000–#909999 and
selects Duris3 room #145443 (“The Ocean”) as its entrance. `create_lab(1)`
starts at map22 #905050 and, after checking both required room lookups,
creates a down exit from #145443 to that room. The reset path removes that
dynamic entrance. The tracked `areas/RANDOM_AREA` file is empty and neither
Duris3 nor map22 appears in `areas/AREA`, so the code does not establish a
currently available route; this link is separate from the 26 active WLD
exits already counted into Duris3.

Type-1 creation also selects object VNUM #68. The current active Heavens
prototype is the Relic of the Spider Queen, while unlisted `hostel.obj` and
`mavulsk.obj` contain same-number prototypes. Keep the code's object key
separate from the WLD entrance relationship and have the object/code owners
confirm the intended resolver identity. See the
[map22 audit](MAP22_AUDIT_REVIEW.md),
[type-1 code](../../../src/world/random.zone.c#L1449),
[dynamic connection](../../../src/world/random.zone.c#L1605),
[Duris3 room #145443](../../../areas/wld/Duris3.wld#L954304),
[Heavens object #68](../../../areas/obj/heavens.obj#L952), and the
[Duris3 coverage row](ZONE_COVERAGE_REGISTER.csv#L387).

## Checkpoint 505 supplement: source-only Mara Guildhall boundaries

The unlisted `mara_gh.wld` contains three outbound directions into the
Duris3 candidate map: room #29600 up to #156766 (`Scorching-Hot Lava`),
#29610 up to #124408 (`Grassy Plains`), and #29617 up to #163376 (`The
Ocean`). Each target is defined in `Duris3.wld`; none is in the current full
area room table. A targeted scan found no Duris3 WLD return edge to a Mara
Guildhall room. The same unlisted `mara_op.wld` source room #29610 also exits
up to #124408, so that endpoint has two distinct source-file records under
shared room VNUMs.

Keep these four source-only Mara-to-Duris3 directions separate from the 26
active-source unresolved exits into Duris3, its 88,256 raw target-number
matches to active WLD VNUMs, and curated approaches into loaded areas. They
do not establish current ingress or change those aggregates. See the
[Mara Guildhall audit](MARA_GH_AUDIT_REVIEW.md#wld-graph-and-external-boundaries),
[Mara Guildhall source exit](../../../areas/wld/mara_gh.wld#L18),
[Mara Outpost source exit](../../../areas/wld/mara_op.wld#L20), and
[Duris3 coverage row](ZONE_COVERAGE_REGISTER.csv#L387).

## Checkpoint 776: Brass room-number overlap

The active Brass WLD uses 357 room IDs, #139000–#139357, all of which also occur in the unregistered Duris3 WLD. None of the 357 paired titles match, and none of the complete WLD records is identical. For example, Duris3 #139000 is “The Ocean” while active Brass #139000 is “The Mighty Gates Of The City Of Brass” ([Duris3 record](../../../areas/wld/Duris3.wld#L819001), [Brass record](../../../areas/wld/brass.wld#L1)). These same-number map records do not establish active ingress, shared room identity, or replacement. Preserve Duris3 under candidate row 36 **RETAIN IN PLACE** and Brass as the active manifest owner; see the [Brass typed-dependency audit](BRASS_AUDIT_REVIEW.md) and [candidate plan](ATTIC_MIGRATION_PLAN.md#checkpoint-560-brass-and-duris3-source-identities).

## Checkpoint 804: Fortb item target

Active Fort Boyard object #38143 stores type-25 target #149188 with CMD_PUSH
(270). Duris3 WLD defines #149188 as "The Ocean", but Duris3 remains
unregistered, so the item record does not establish an active loaded-room
route. Keep this typed item target separate from the 26 active-source WLD
exits to 24 Duris3-only rooms and from the four source-only Mara directions.
It changes neither count. Preserve Duris3 candidate row 36 **RETAIN IN PLACE**
while the map and item owners resolve identity and availability. See the
[Fort Boyard typed census](FORT_BOYARD_STORY_REVIEW.md#checkpoint-804-current-typed-dependency-census),
[Fortb brush values](../../../areas/obj/fortb.obj#L476),
[Duris3 room #149188](../../../areas/wld/Duris3.wld#L1032949),
[candidate dependency row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and
[Duris3 coverage row](ZONE_COVERAGE_REGISTER.csv#L387).

## Checkpoint 821: exact overlap with active Randomness

Every Duris3 room VNUM #100000–#109999 is also defined by active `random.wld`; all 10,000 paired titles differ (“The Ocean” in Duris3, “Unnamed” in Randomness). A current all-project WLD scan found 602 non-overlapping source-only Duris3 directions from its other room bands into that numeric interval. They remain source records in this separate, unregistered Duris3 map and add no active Randomness edge or candidate-route count. Keep Duris3 on candidate data row 35 / CSV line 36 **RETAIN IN PLACE**, preserve its separate map identity, and do not infer replacement from the shared VNUMs. See the [Randomness typed audit](RANDOMNESS_AUDIT_REVIEW.md#checkpoint-821-current-typed-dependency-closure), [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [Randomness coverage row](ZONE_COVERAGE_REGISTER.csv#L247).

## Checkpoint 884: typed room roles and compiled consumers

A role-aware scan separated Duris3 room references from same-number mobile, item,
quest, shop, and code values. The 735 candidate-only room operands in current
ZON commands occur only in two unregistered components: 576 operands in
`forofcon.zon` (219 distinct room numbers) and 159 in `brass-old-1.zon` (75
distinct numbers). Every operand resolves to a room in its own same-stem WLD
(`forofcon.wld` or `brass-old-1.wld`). They are local source identities, not
external resets into Duris3. See the [Forofcon ZON/WLD](../../../areas/zon/forofcon.zon)
and [Brass Old 1 ZON/WLD](../../../areas/zon/brass-old-1.zon). This adds no
active or candidate WLD route.

All 106 QST number appearances that fall in Duris3-only room-number space are
explained by QST field roles: 36 quester mobile VNUMs, 32 item VNUMs in `G/R I`,
14 coin amounts, and 24 experience amounts. The quest loader reads the `#`
record as a mobile and classifies the `G`/`R` goal subtype before reading its
number ([quest loader](../../../src/world/quest.c#L2099)). Across 232 story JSON
sidecars, candidate-range values occur under quest/story metadata and prose;
none uses a room-, destination-, target-, arrival-, teleport-, portal-, or
area-VNUM path. The current 720 parsed SHP records likewise contain no shop
room field in the Duris3-only set. These are not Duris3 room dependencies.

A typed OBJ scan covered 21,682 non-sentinel object records, including all 872
type-25 and 380 type-29 records. Twenty candidate-only room-role operands were
found: nineteen in unregistered `forofcon.obj` and one in active Fort Boyard.
Forofcon's fifteen type-25 `value[0]` teleport destinations and four type-29
`ITEM_SWITCH` `value[1]` blocked-exit rooms all resolve to its own 403-room
WLD, #139900–#140302. Active `fortb.obj` object #38143 has type 25 and
`value[0]` #149188 with `CMD_PUSH` (270); active `fortb.zon` places that object
in Fortb room #38100. Duris3 alone owns room #149188 among current WLD sources.
This is a typed item target into an unregistered map, not an active WLD edge.
See the [Fort Boyard object](../../../areas/obj/fortb.obj#L476), [Fortb reset](../../../areas/zon/fortb.zon#L81), and [Duris3 room](../../../areas/wld/Duris3.wld#L1032949).

The compiled-number scan confirmed four Duris3-only room operands have these
separate roles: `random.zone.c` uses #145443 as the conditional type-1
random-lab entrance to map22 #905050 (the existing CP502 code path), and #154938
as the type-0 random-lab entrance for #705050 in the #700000–#709999 generated room band;
`specs.assign.c` binds the shipwright `ship_shop_proc` service to #140854; and
the Fort Boyard target above names #149188. See the [shipwright room binding](../../../src/specs/specs.assign.c#L2381)
and [random-lab selectors](../../../src/world/random.zone.c#L1363). Current WLD ownership for #140854, #145443, #149188, and
#154938 is Duris3 only. The type-1 link checks for both room records before
creating its down connection, and the reset removes that connection; no active
AREA membership establishes it. No `map21.wld` or other checked-in WLD for the
type-0 band exists, so preserve that as an unresolved code/generator reference.
The shipwright handler operates on owned ships and normally requires the ship's
room location to match the service room ([handler](../../../src/ships/ship_shop.c#L2960));
this compiled assignment is a potential service endpoint, though the unregistered
Duris3 room is not evidence of present availability.

A repository-wide exact-text scan outside documentation and the already
role-parsed WLD/OBJ/ZON/QST data found no additional production map or tooling
consumer. Remaining matches are checked-in tests: a source-contract test asserts
the same #140854 binding, and catalog assertions confirm Duris3 is absent from
`areas/AREA` while distinguishing selected Duris3 WLD records from other maps.
These are test assertions, not runtime routes. See the [#140854 handler assertion](../../../tests/async/test_zone_story_quest_magma.py#L93), [Duris3 manifest assertion](../../../tests/async/test_zone_story_quest_production_catalog.py#L4529), and [WLD identity assertions](../../../tests/async/test_zone_story_quest_production_catalog.py#L10645). No tests were run.

Two additional code ranges can affect a loaded Duris3 room without creating a
route: `encounters.c` applies encounter/level rules to room indices between
`real_room(140000)` and `real_room(149999)`, while `spell_travel.c` contains a
#140000–#159999 random-room selection after an unconditional disabled-spell
return. The latter is not a current travel edge. See the [encounter rules](../../../src/mob/encounters.c#L151)
and [disabled spell return](../../../src/magic/spell_travel.c#L90). Other numeric
matches were capacity, geometry, or non-room values. The bounded C/C++ source
scan found no other exact candidate-only room API reference.

CP884 changes none of the Duris3 graph totals: 90,000 rooms, 360,000 internal
directions, 26 active WLD directions to 24 unresolved Duris3-only VNUMs, and
88,256 same-number target matches that remain internal to Duris3 source
identity. Keep candidate row 36 **RETAIN IN PLACE**. The coverage dependency
field stays **PENDING** because editor/generator/plugin/deployment data, saved
rooms, persisted player references, and the intended owner for the code
bindings remain unverified. Restore and use the [24 builder questions](DURIS3_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions) to resolve those decisions. See the [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-884-duris3-typed-and-code-room-references), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-884-duris3-retains-typed-and-code-references), [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36), and [coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387). No area or gameplay source was changed; no tests, builds, runtime, server, database, or container were used.
