# shadow_hall source-component audit

Canonical key: component:shadow_hall. Physical coverage row 442 / CSV line
443; candidate dependency data row 91 / CSV line 92. Reviewed October 11,
2026, Checkpoint 529; extended October 13, 2026, Checkpoint 933, against
checkout c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source,
manifest, graph, identity, reset, and code-reference review. It does not
establish a booted world table or player travel.

## Inventory, title, and manifest status

The source set has three WLD rooms #30100–#30102 in
[shadow_hall.wld](../../../areas/wld/shadow_hall.wld#L1), one MOB prototype
#30100 in [shadow_hall.mob](../../../areas/mob/shadow_hall.mob#L1), a
tracked but empty [OBJ file](../../../areas/obj/shadow_hall.obj), and one ZON
header with six reset commands in
[shadow_hall.zon](../../../areas/zon/shadow_hall.zon#L1). The raw ZON title
is “Shadow Law Guildhall.” There is no same-stem QST, SHP, or story sidecar.

The stem is absent from [areas/AREA](../../../areas/AREA),
[areas/qst/AREA](../../../areas/qst/AREA), and the current
[RANDOM_AREA](../../../areas/RANDOM_AREA). Active
[underworld3](../../../areas/AREA#L145) claims the broad `*300-303` range
and defines different rooms #30100–#30102. Generated `areas/world.wld`,
`world.mob`, `world.obj`, and `world.zon` are absent in this checkout.

## Map topology and outlets

The three-room source has five exits. Four same-number local targets form
one weak component of all three rooms; one exit leaves the map.

| Source room | Authored exits |
| --- | --- |
| #30100, Walk of Law | D2 → #30102; D3 → #30101 |
| #30101, Chamber of the Select | D1 → #30100 |
| #30102, Courtyard of Law | D0 → #30100; D4 → #151052 |

Current numeric lookup resolves the four internal target IDs to different
listed Underworld3 rooms. The external target #151052 is “Thick Forest” only
in unlisted [Duris3.wld](../../../areas/wld/Duris3.wld#L1072093); no current
listed WLD defines it, and its exits do not return to #30100–#30102. No
active exit enters a shadow_hall identity. Eight active Underworld3 exits
also target the numbers #30100–#30102, but they lead within Underworld3's
listed identities. The candidate graph adds no current active route. There
are no negative exits, numeric-range mismatches, fall-chance records, or
current records.

## Same-number identities and typed reset imports

All three Shadow Law room VNUMs also have different records in listed
Underworld3. Unlisted [cabal_hall.wld](../../../areas/wld/cabal_hall.wld#L1)
and [nah.wld](../../../areas/wld/nah.wld#L1) define the same three numbers
with their own guild/castle scenes. MOB #30100 (“a judged soul”) has
different unlisted same-type records in cabal_hall and nah; there is no
active same-type MOB #30100. The Shadow Hall OBJ file is empty. These
identities do not establish that one whole component replaces another.

Six ZON commands reset MOB #30100 four times in room #30102 and import two
objects: OBJ #11110 into room #30101 and OBJ #11060 into room #30102. Both
object prototypes are in active [Player Castles](../../../areas/AREA#L63):
#11110 is a bulletin board and #11060 is a magic mouth. The source comments
mark them as objects not defined in the local OBJ file. The imports should
remain typed and separate from the room and mobile VNUM comparison.

Unlisted cabal_hall.shp and nah.shp also name rooms in the #30100–#30102
numeric range. Those source-only shop bindings do not provide active
shadow_hall ingress. The current active Underworld3 ZON places a water weird
in room #30101; numeric lookup identifies that placement with Underworld3,
not the absent Shadow Law room. No exact #30100–#30102 or #151052 reference
was found in C/C++ source.

## Provenance, story limits, and disposition

The rooms and judged-soul text establish a Shadow Law setting, but there is
no same-stem QST or story sidecar. Followed path history shows the 2013
`stuff` update and 2009 area-tree move, with no explicit predecessor,
successor, or retirement decision. Retain the four source files IN PLACE
while the area and editor owners confirm namespace ownership, the unlisted
Duris3 outlet, and the two Player Castles imports. No activation, rename,
move, merge, archive, or migration clearance is implied. See the
[21 builder questions](SHADOW_HALL_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-529-shadow_hall-underworld3-number-collision),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-529-shadow_hall-and-underworld3-room-identity),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-529-shadow_hall-retain-guildhall-source),
[candidate dependency row 91](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L92),
and [coverage row 442](ZONE_COVERAGE_REGISTER.csv#L443).

## Checkpoint 933: Player Castles imports have separate code consumers

The two imports are active Player Castles objects: #11060 is a magic mouth
and #11110 is a bulletin board ([OBJ #11060](../../../areas/obj/player_castles.obj#L201),
[OBJ #11110](../../../areas/obj/player_castles.obj#L321)). A fresh exact-value
scan found five ZON O rows importing them: two in `shadow_hall`, two in
unlisted `bastard`, and one in unlisted `sky_gh`
([Shadow rows](../../../areas/zon/shadow_hall.zon#L9),
[Bastard rows](../../../areas/zon/bastard.zon#L9),
[Sky GH row](../../../areas/zon/sky_gh.zon#L10)). No exact QST, SHP, or story
sidecar consumer was found. A raw #11110 match in `areas/food` is a price
field, not a VNUM reference ([food row](../../../areas/food#L295)).

The object #11060 is also explicitly bound to the `magic_mouth` object
special through `real_object0` ([specs.assign.c](../../../src/specs/specs.assign.c#L1636),
[binding](../../../src/specs/specs.assign.c#L1643)). The same-number OBJ #11110
appears in mobile-helper arrays for `realms_master_shout` and `navagator`
([specs.mobile.c](../../../src/specs/specs.mobile.c#L106),
[specs.minizones.c](../../../src/specs/specs.minizones.c#L432)). No MOB #11110
prototype exists in the checked-in full or mini mobile sources;
`realms_master_shout` has no assignment found, while `navagator` is assigned
to MOB #5739 ([specs.assign.c](../../../src/specs/specs.assign.c#L683)). These
are separate code-side mobile-helper numbers, not evidence that the bulletin
board is a mobile or that the helper path executes at runtime.

The imports remain typed OBJ placements. They add no WLD exit or inbound
route, and the #30100–#30102 room and mobile identities remain unchanged.
Keep all four `shadow_hall` files RETAIN IN PLACE and resolve the Player
Castles item uses, helper-list ownership, and the two other unlisted reset
sources with their area/code owners before any route, namespace, or path
proposal. Question 21 in
[SHADOW_HALL_BUILDER_QUESTIONS.md](SHADOW_HALL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions)
records the cross-area import and code-role review.
