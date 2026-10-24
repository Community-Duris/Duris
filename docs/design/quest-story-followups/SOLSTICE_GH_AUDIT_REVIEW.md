# solstice_gh source-component audit

Canonical key: `component:solstice_gh`. Coverage data row 445 / CSV line
446; candidate dependency data row 94 / CSV line 95. Reviewed October 11,
2026, Checkpoint 532, against checkout
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
manifest, graph, typed-reference, identity, shop-loader, and followed-history
review. It does not establish that the unlisted component is loaded or that
its exits, resets, shop, or room special are usable.

## Inventory, title, and manifest status

The source component has ten WLD rooms #29100–#29109 in
[solstice_gh.wld](../../../areas/wld/solstice_gh.wld#L1), three MOB
prototypes #29100–#29102 in
[solstice_gh.mob](../../../areas/mob/solstice_gh.mob#L1), six OBJ prototypes
#29100–#29105 in [solstice_gh.obj](../../../areas/obj/solstice_gh.obj#L1),
31 ZON reset commands in
[solstice_gh.zon](../../../areas/zon/solstice_gh.zon#L1), and one shop
record #29100 in [solstice_gh.shp](../../../areas/shp/solstice_gh.shp#L1).
The ZON header is #1, its raw title is “Solstice Main Guildhall,” its last
room is #29109, and reset mode is 2. A normalized scan finds this title only
in `solstice_gh.zon` among the 442 current ZON source files; uniqueness does
not settle the canonical area name or alias.

The WLD describes a fortified Solstice Guild, an inner circle, and rooms
representing spring, summer, fall, and winter. The OBJ text adds food,
Celtic, and seasonal details. There is no same-stem QST or story sidecar.
These descriptions establish source narrative, not a current quest or story
link. The component is absent from
[areas/AREA](../../../areas/AREA),
[areas/qst/AREA](../../../areas/qst/AREA), and the empty current
[RANDOM_AREA](../../../areas/RANDOM_AREA). Active Torg lists the `*289–291`
range, but its same-number rooms have different descriptions and functions.

## Candidate map and current numeric resolution

The ten-room WLD has 19 authored exits. Eighteen target local Solstice
numbers and form one weak component containing all ten rooms. The remaining
edge is D4 up from #29100 to #154578.

| Source room | Solstice room text | Authored exits |
| --- | --- | --- |
| #29100 | Fortified EarthGate of Solstice | D0 → #29104; D4 → #154578 |
| #29101 | Stone Spiral Staircase Dusted with Hot Sand | D0 → #29102; D5 → #29109 |
| #29102 | Circle of Moss-Covered Stone Blocks | D0 → #29103; D1 → #29104; D2 → #29101; D3 → #29105 |
| #29103 | Stone Spiral Staircase Covered in Frost | D2 → #29102; D5 → #29108 |
| #29104 | Stone Spiral Staircase Covered in Fallen Leaves | D2 → #29100; D3 → #29102; D5 → #29107 |
| #29105 | Stone Spiral Staircase Covered in Vines | D1 → #29102; D5 → #29106 |
| #29106 | Small Garden Shrouded in Fresh Growth | D4 → #29105 |
| #29107 | Ancient Copse of Trees surrounded by Decaying Leaves | D4 → #29104 |
| #29108 | Small Cavern Encased in Ice and Snow | D4 → #29103 |
| #29109 | Small Cavern Dusted with Hot Sand | D4 → #29101 |

Current numeric room lookup resolves all 18 internal targets to different,
listed Torg rooms rather than to Solstice Guild room identities. The target
#154578 is “The Ocean” only in
[unlisted Duris3.wld](../../../areas/wld/Duris3.wld#L1146139); that room's
listed exits lead to #154278, #154579, #154878, and #154577, with no return
to #29100–#29109. The candidate has zero active WLD ingress. Thirteen active
Torg WLD exits also target #29100–#29109; each resolves to a Torg room.
Torg's active ZON has another 32 room-typed references to those numbers: nine
D rows and 23 M placement rows. These are Torg door and placement data, not
references to Solstice identities. There are no negative targets,
range-mismatch rooms, fall-chance fields, or current fields in the candidate
WLD; “air currents” occurs only in garden prose.

The active numeric room identities are materially different:

| VNUMs | Current Torg room texts (Solstice room identities remain in the table above) |
| --- | --- |
| #29100–#29101 | Small Hobgoblin Home; Small Hobgoblin Home |
| #29102 | Higher Order Barracks |
| #29103 | Roughly Built Escape Tunnel |
| #29104 | Curve in the Escape Tunnel |
| #29105 | Dead End in the Escape Tunnel |
| #29106 | Shop of Gornogo |
| #29107 | Higher Order Barracks |
| #29108–#29109 | Small Barracks in the City; Small Barracks in the City |

Other unlisted WLDs also use all ten room numbers with different maps:
`afterlife_gh.wld`, `scrubs_hall.wld`, `void_gh.wld`, and `wlust_gh.wld`.
The shared range does not establish that any one map supersedes another.

## Same-number prototypes, code binding, and quest boundary

Solstice MOB #29100–#29102 has no matching active Torg MOB prototype. The
same numbers also have distinct, unlisted mobile definitions in
`afterlife_gh.mob`, `scrubs_hall.mob`, `void_gh.mob`, and `wlust_gh.mob`.
The unlisted [afterlife_gh.qst](../../../areas/qst/afterlife_gh.qst#L1) begins
at giver/mobile VNUM #29101; it is not in the current QST loader list and
does not make Solstice a quest giver.

Solstice OBJ #29100–#29105 likewise has no matching active OBJ prototype.
Different unlisted OBJ definitions occur at #29100–#29102 in
`afterlife_gh.obj` and #29100–#29106 in `void_gh.obj`. The candidate's local
object names include a buttered scone, dragon-scale tea pot, charred ogre
skull, demon-hide basket, Celtic cross, and icy fresh-water spring. No active
QST object inputs/rewards, SHP products, or story-sidecar item references
were found for #29100–#29105.

The source-code room assignment at
[specs.assign.c line 2448](../../../src/specs/specs.assign.c#L2448) binds
`inn` to `real_room0(29103)`. Under the current manifest, #29103 is Torg's
roughly built escape tunnel; in the candidate WLD, it is a frost-covered
Solstice stairway. This is a code-to-room number collision, not proof that a
Solstice room or shop is active.

## Reset, imported-object, and shop dependencies

The 31 ZON commands comprise four O rows, five M rows, and 22 G rows.

- O rows 9–11 import active Player Castles objects #11053 (magic mouth),
  #11103 (board), and #11013 (bonding altar) into Solstice rooms #29100,
  #29102, and #29103. O row 12 places the candidate's local spring #29105
  in room #29105. Preserve the three external placements as typed imports.
- M rows 14–18 place source-only sprite #29101 three times with limit 3 and
  pixie #29102 once in room #29100, then source-only satyr #29100 once in
  room #29101. All three candidate mobile definitions are absent from the
  current full-area MOB table.
- The 22 G rows place candidate OBJ #29100–#29104 and imported object
  VNUMs #203, #204, #108, #109, #111, #1514, #5322, #8336, #1134, #5319,
  #16649, #590, #3050, and #491–#494. The five local prototypes are
  source-only. Fifteen of the 17 external IDs have active object
  definitions: #203/#204/#108/#109/#111, #1134/#590/#491–#494 in Heavens;
  #5319/#5322 in Marigot; #16649 in Woodseer; and #3050 in Verz1. #108 and
  #109 also have different unlisted Mavulsk OBJ variants, while current
  active lookup selects Heavens. #1514 has no matching OBJ definition.
  #8336 has no OBJ definition; that number is found as a mobile only in
  unlisted `sylvandawn.mob`, so it is not an object resolution.

SHP #29100 has the same 22 product VNUMs as the ZON G rows. Its keeper is
MOB #29100 and its `in_room` is #29101. The candidate intends the satyr in
its summer stairway; current active lookup has no MOB #29100 and maps room
#29101 to Torg's Small Hobgoblin Home. The five local Solstice products are
also outside current active OBJ ownership. Imported product gaps #1514 and
#8336 apply to the shop as well.

In mini mode `open_shop_stream()` reads `areas_mini/world.shp`. Full mode
first tries `areas/world.shp`, which is absent from this checkout, then
falls back to globbing `areas/shp/*.shp`; that glob includes
[solstice_gh.shp](../../../areas/shp/solstice_gh.shp#L1). If that record is
read, `boot_the_shops()` converts keeper #29100 with `real_mobile()`. The
resolved keeper is then used by `assign_the_shopkeepers()` without a local
negative-index guard before `mob_index[keeper]` access
([shop.c](../../../src/economy/shop.c#L3267),
[keeper parse](../../../src/economy/shop.c#L3493),
[assignment](../../../src/economy/shop.c#L3602), and
[unguarded access](../../../src/economy/shop.c#L3623), called from
[specs.assign.c](../../../src/specs/specs.assign.c#L1261)). This is a conditional
static loader hazard because no active keeper prototype matches #29100; no
shop boot or failure was tested. The second unlisted file
`afterlife_gh.shp` also has record #29100, so all-file fallback input and
same-key behavior warrant shop/deployment-owner review. The fallback path
does not prove an operating shop.

## Followed history, disposition, and limits

Bounded `git log --follow` history finds WLD #29100–#29109 last edited in
2013 (`71d731c5b`); MOB and OBJ history reaches only the 2009 area-tree move
(`642da5400`). ZON and SHP were both edited September 6, 2026
(`ed94862f0`) to add Heavens gathering-bag objects #491–#494 to the reset
and shop lists; their prior visible edit is 2013. This is recent source
maintenance, not evidence of current manifest membership, activation, or
replacement. No explicit predecessor, successor, retirement, or archive
decision was found in the followed file history.

Retain all five source files IN PLACE. Resolve the competing Torg and
unlisted WLD identities, all same-type mobile/object variants, the one-way
Duris3 outlet, code-bound room #29103, three Player Castles imports, the
external object gaps, and the conditional fallback shop issue with area,
shop, code, deployment, and tooling owners before any separate move, rename,
merge, activation, or archive decision. Editor, export, persisted-identity,
and deployment references were not available for this static review.

See [20 builder questions](SOLSTICE_GH_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-532-solstice_gh-seasonal-map-and-torg-number-resolution),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-532-solstice_gh-map-and-prototype-identities),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-532-solstice_gh-retain-map-shop-and-imports),
[candidate dependency row 94](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L95),
and [coverage row 445](ZONE_COVERAGE_REGISTER.csv#L446).

## Checkpoint 943: source-only Marigot shop and reset references

Exact typed closure confirms `solstice_gh.zon` lines 30 and 33 reset active Marigot objects #5322 and #5319, and `solstice_gh.shp` shop #29100 lists those two products at lines 15 and 18. Keep the records attached to Solstice GH **RETAIN IN PLACE** and preserve Marigot as prototype owner. The candidate is not in `areas/AREA`; these source records do not establish active resets or operating shop stock. They add no WLD edge, active ingress, or change to the existing graph. See the [Marigot source review](MARIGOT_STORY_REVIEW.md#checkpoint-943-bounded-typed-and-compiled-code-closure), [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L95), [coverage row](ZONE_COVERAGE_REGISTER.csv#L446), and [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-943-marigot-shop-and-reset-imports-stay-with-candidates).
