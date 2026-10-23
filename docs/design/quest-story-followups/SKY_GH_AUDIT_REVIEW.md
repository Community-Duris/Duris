# sky_gh source-component audit

Canonical key: `component:sky_gh`. Physical coverage row 444 / CSV line
445; candidate dependency data row 93 / CSV line 94. Reviewed October 11,
2026, Checkpoint 531, against checkout
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
manifest, graph, identity, reset, shop, and history review. It does not
establish that the unlisted component is loaded or that a route or shop is
usable.

## Inventory, title, and manifest status

The source set has five WLD rooms #29400–#29404 in
[sky_gh.wld](../../../areas/wld/sky_gh.wld#L1), four MOB prototypes
#29400–#29403 in [sky_gh.mob](../../../areas/mob/sky_gh.mob#L1), a tracked
but empty [OBJ file](../../../areas/obj/sky_gh.obj), a ZON header with 18
reset commands in [sky_gh.zon](../../../areas/zon/sky_gh.zon#L1), and one
shop record in [sky_gh.shp](../../../areas/shp/sky_gh.shp#L1). The raw ZON
title is “The Hall of the Clouds.” The WLD calls the setting “the Hall of
the Skies”; this wording does not by itself establish a canonical alias.
There is no same-stem QST or story sidecar.

`sky_gh` is absent from [areas/AREA](../../../areas/AREA),
[areas/qst/AREA](../../../areas/qst/AREA), and the empty current
[RANDOM_AREA](../../../areas/RANDOM_AREA). Active
[Harrow](../../../areas/AREA#L142) owns the `*294–296` band and defines
different WLD rooms #29400–#29404, MOB prototypes #29400–#29404, and its
own object prototypes at shared numeric IDs. Unlisted `dark` and
`infernal_gh` sources also reuse these WLD numbers; `dark.mob` and
`infernal_gh.mob` reuse #29400–#29402 with different records.

## Candidate map and current numeric resolution

The five-room map has nine exits. Eight same-number targets connect all
five rooms as one weak component; the remaining exit is a D4 up from
#29400 to #153760.

| Source room | Authored exits |
| --- | --- |
| #29400, Before a Colossal Door | D0 → #29401; D4 → #153760 |
| #29401, Main Hallway in the Hall of the Skies | D0 → #29402; D2 → #29400 |
| #29402, The Hall Continues | D1 → #29403; D2 → #29401; D3 → #29404 |
| #29403, Inn of the Hall of the Skies | D3 → #29402 |
| #29404, Armory in the Hall of the Skies | D1 → #29402 |

Current numeric lookup resolves the eight internal targets to different
listed Harrow rooms, not to the unlisted sky_gh rooms. The outside target
#153760 is “Mountains” only in unlisted
[Duris3.wld](../../../areas/wld/Duris3.wld#L1128961); its authored exits
lead to #153460, #153761, #154060, and #153759, with no return to
#29400–#29404. There is no active exit into a sky_gh identity. Fifteen
active WLD records target numbers #29400–#29404: 13 Harrow-local links and
two Surface entrances to Harrow #29400. They resolve to Harrow's current
rooms. The separate source-only Surf approach to #29400 is likewise
classified in the [Harrow audit](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches)
as a numeric approach to Harrow, not proof of sky_gh ingress. There are no
negative targets, range-mismatch rooms, fall-chance fields, or current
fields in the candidate WLD.

The two active Surface records are at
[surface.wld line 992442](../../../areas/wld/surface.wld#L992442) and
[line 1000851](../../../areas/wld/surface.wld#L1000851); their target is
active Harrow room #29400. They are not sky_gh-identity ingress despite
the shared room number.

## Reset, object, shop, and code dependencies

Active Harrow's ZON contains nine `M` rows using #29400–#29403; they place
the current Harrow gardeners, tomatoes, and cucumbers. Those active resets
resolve by the current manifest to Harrow prototypes, not to the candidate
golems or merchant ([Harrow resets](../../../areas/zon/harrow.zon#L225)).

The 18 ZON commands include two local mobile resets, two object resets, and
14 `G` object rows that match the shop product list. The mobile rows place
#29402 in room #29400 and #29403 in room #29404. Under the current manifest
those VNUMs resolve to Harrow's Merill and cucumber records, while the
unlisted source describes a golem and a merchant. Objects #11013 (Player
Castles bonding altar) and #11110 (Player Castles board) are reset in
candidate room #29403; these imports remain distinct from the empty local
OBJ file and from the candidate room identity.

The shop record is keyed as #29403, names keeper MOB #29403, and sets
`in_room` #29404. Its 14 products are #108, #109, #111, #377, #3050, #398,
#6089, #53, #364, #412, and #491–#494. Current typed item lookup finds
active prototypes for 13 products: #108, #109, #111, #377, #398, #53,
#364, #412, and #491–#494 are in Heavens, while #3050 is in Verz1. No
current OBJ source defines #6089. #108 and #109 also have different
unlisted Mavulsk records; #53 has other unlisted same-number object
definitions in Dalvik, Hostel, and Mavulsk, while the active record is
Heavens' rope coil. See the current
[#53 Heavens prototype](../../../areas/obj/heavens.obj#L788),
[#491 gathering bag](../../../areas/obj/heavens.obj#L5055), and
[#3050 Verz1 prototype](../../../areas/obj/verz1.obj#L850). The two O-row
imports are current Player Castles
[#11013](../../../areas/obj/player_castles.obj#L121) and
[#11110](../../../areas/obj/player_castles.obj#L321) objects. Preserve the
object type and current active owner when reviewing these shop rows.

Harrow QST/story item values #29400, #29402, and #29404 are active Harrow
OBJ inputs or rewards, not sky_gh mobile references. The same sidecar
separately declares `mob_vnum: 29404` for Lomya, an active Harrow mobile.
Harrow has distinct same-number WLD, MOB, and OBJ prototypes; in `sky_gh`,
#29404 is the Armory room and the SHP `in_room` value. The candidate OBJ
file is empty, so the field-typed story contact remains Harrow's mobile,
its exchange values remain objects, and the candidate SHP room value is
resolved by the current loader to Harrow's room when that source is read
([QST](../../../areas/qst/harrow.qst#L78),
[story contact](../../../areas/story/harrow.story.json#L115),
[story contract](../../../areas/story/harrow.story.json#L398),
[Harrow MOB](../../../areas/mob/harrow.mob#L52),
[Harrow OBJ](../../../areas/obj/harrow.obj#L53),
[Harrow room](../../../areas/wld/harrow.wld#L136),
[candidate room](../../../areas/wld/sky_gh.wld#L70),
[candidate SHP location](../../../areas/shp/sky_gh.shp#L49)). Keep these
same-number cross-type uses separate from the four candidate MOB
prototypes.

The September 6, 2026 gathering-bag commit added Heavens OBJ #491–#494 and
matching sky_gh ZON and SHP entries. This is recent source maintenance; it
does not add sky_gh to either manifest. In this checkout
`areas/world.shp` is absent. In full mode,
[shop.c](../../../src/economy/shop.c#L3267) can fall back to globbing
`areas/shp/*.shp`. If that path consumes sky_gh's record, its keeper #29403
and room #29404 resolve by current number to Harrow's cucumber and Fountain
Square, rather than the unlisted merchant and Armory. The separate code
binding assigns `inn` to current Harrow room #29403
([specs.assign.c](../../../src/specs/specs.assign.c#L2454)); that numeric
room is Fountain Square in Harrow and the Inn in sky_gh. These are
conditional source bindings, not observed boot behavior. Other code-level
matches in `chaos_eq_data.h` use shared values such as #29401 and #29404 as
Harrow object prototypes, not sky_gh mobiles; see the
[Harrow audit](HARROW_AUDIT_REVIEW.md#bounded-typed-and-code-references).

## Provenance and disposition

Followed WLD history reaches the 2013 `stuff` update and the 2009 area-tree
move; the MOB and empty OBJ histories reach the 2009 move. The ZON and SHP
were most recently edited together on September 6, 2026. No tracked history
records an explicit sky_gh predecessor, successor, or retirement decision.
The recent bag-stock additions do not establish the component's manifest
status or resolve its Harrow, Dark, and Infernal same-number identities.

Retain all five source files IN PLACE, including the empty OBJ file. Keep
the eight authored local routes, the one-way Duris3 outlet, both
Player Castles imports, all mobile resets, all 14 product rows, and the
shop record intact while owners resolve the conflicting room/mobile
identities, unregistered outlet, missing #6089 prototype, #53 source, and
conditional shop fallback. No activation, rename, merge, move, archive, or
migration clearance is implied. See the
[21 builder questions](SKY_GH_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-531-sky_gh-graph-and-harrow-number-resolution),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-531-sky_gh-and-harrow-competing-identities),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-531-sky_gh-retain-map-shop-and-imports),
[candidate dependency row 93](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L94),
and [coverage row 444](ZONE_COVERAGE_REGISTER.csv#L445).

## Checkpoint 935: Harrow contact and item roles stay separate

A focused exact-value scan confirms active Harrow story `mob_vnum: 29404`
identifies Lomya, while the Harrow QST/story contract uses #29400 and
#29402 as item inputs and #29404 as an OBJ reward. The same number #29404
is a distinct active Harrow WLD room, MOB, and OBJ, and `sky_gh` reuses it
as its Armory WLD room and SHP `in_room` value. If the unlisted shop record
is read, current room lookup resolves that location to Harrow's room; it
does not identify the sky_gh Armory. Preserve each source field's type and
current owner. This adds no WLD edge, candidate-identity ingress, lineage,
or migration clearance. See the [Harrow QST/story fields](../../../areas/qst/harrow.qst#L78), [story mobile contact](../../../areas/story/harrow.story.json#L115), [story exchange](../../../areas/story/harrow.story.json#L398), [Harrow mobile](../../../areas/mob/harrow.mob#L52), [Harrow object](../../../areas/obj/harrow.obj#L53), [Harrow room](../../../areas/wld/harrow.wld#L136), and [sky_gh room/shop fields](../../../areas/wld/sky_gh.wld#L70).
