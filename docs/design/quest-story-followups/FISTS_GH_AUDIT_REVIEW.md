# fists_gh — Fists of Fury Guildhall source audit

Canonical key: component:fists_gh. Physical coverage row 392 / CSV line
393; candidate dependency row 42 / CSV line 42. Reviewed October 11, 2026,
Checkpoint 479, against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697.
This is a static source, connectivity, identity, history, and typed-reference
review. It does not establish runtime shop loading, a canonical map owner, or
permission to activate or move these files.

## Source scope, title, and story

The candidate has [WLD](../../../areas/wld/fists_gh.wld#L1),
[MOB](../../../areas/mob/fists_gh.mob#L1),
[OBJ](../../../areas/obj/fists_gh.obj), [ZON](../../../areas/zon/fists_gh.zon#L1),
and [SHP](../../../areas/shp/fists_gh.shp#L1) files. It is absent from full
[areas/AREA](../../../areas/AREA#L147) and
[areas/qst/AREA](../../../areas/qst/AREA); its raw AREA annotation is blank.
There is no same-stem QST, story sidecar, or editor-map file. The ZON title is
Fists of Fury Guildhall. The WLD names a guild guard room, corridor, store,
conference room, inn, board room, portal room, and colored path rooms; the
MOBs are three golems and a greedy merchant. These records do not establish a
quest sequence or story relationship to active Patrols.

The ZON header is #305, top VNUM #30509, reset mode 0. Its ten room records
#30500–#30509 are within the conventional #30500–#30599 range. The same-stem
OBJ file currently has zero bytes and defines no object. Its tracked history
shows that it formerly defined an unnamed object #30500; a 2016 commit removed
that eight-line prototype.

## Candidate WLD graph and active-table ownership

The candidate WLD defines ten rooms and 21 authored directions:

| Room | Authored directions |
| ---: | --- |
| #30500 | north to #30501; up to #157556 |
| #30501 | east to #30504; south to #30500; west to #30502 |
| #30502 | north to #30503; east to #30501 |
| #30503 | south to #30502; down to #30509 |
| #30504 | north to #30505; west to #30501 |
| #30505 | south to #30504; down to #30506 |
| #30506 | west to #30507; up to #30505 |
| #30507 | east to #30506; west to #30508 |
| #30508 | east to #30507; west to #30509 |
| #30509 | east to #30508; up to #30503 |

Twenty directions target another candidate room. They form one weak component
of size ten, and all ten rooms are directed-reachable from #30500. The
remaining direction goes up from #30500 to inactive Duris3 #157556, “Grassy
Plains” ([target room](../../../areas/wld/Duris3.wld#L1208677)); that room's
four authored exits do not return to #30500–#30509. No fall-chance or current
records occur.

The #30503 down to #30509 and #30509 up to #30503 records are reciprocal.
Earlier CP318 notes described the second direction as down; the raw WLD has
D4, which is up. The two directions are distinct from the one-way Duris3
boundary.

All seven candidate room numbers #30500–#30506 also occur in active
[Patrols WLD](../../../areas/wld/patrols.wld#L1), but all seven complete room
blocks differ. Fists calls them a guard room, main corridor, store, conference
room, inn, board room, and portal room. Active Patrols uses the same numbers
for Elven, Barbarian, Tharnadian, Halfling, Gnomish, Dwarven, and Centaur
Guard Load Rooms. The full [AREA entry](../../../areas/AREA#L147) assigns
active zone 305 to Patrols; it does not list fists_gh. The candidate's
#30507–#30509 rooms have no active Patrols room identity.

A prior all-95 inactive-WLD comparison found 14 candidate directions whose
numeric targets are Patrols rooms. Twelve originate in the overlapping
candidate rooms #30500–#30506. The other two are source-only approaches from
#30507 east to #30506 and #30509 up to #30503. Those two records were already
included in the curated source-only totals at CP318; do not count them again.
They are source-only numeric approaches, not active ingress into the Fists
identity. Candidate-identity active inbound remains zero. Current active
tables resolve 14 of the 21 targets; the seven positive-unresolved target
records are the one Duris3 boundary and six targets in candidate-only
#30507–#30509 rooms. The Duris3 reference is the single target stem recorded
as unregistered. Curated inactive-WLD totals remain 11,987 overall / 415
from Surf.

## Mobiles, resets, imported object, and shop references

The four candidate mobiles are red golem #30500, cyan golem #30501, blue
golem #30502, and greedy merchant #30503. Active Patrols defines different
same-number mobiles: Tharnadian soldier, Ugta barbarian soldier, gnomish
soldier, and halfling soldier. The candidate ZON has five M commands: two
#30500 golem records and one each for #30501, #30502, and #30503. The first
four place golems in candidate room #30500; the merchant is placed in
candidate room #30502. These are source-only reset identities, not Patrols
resets.

The candidate ZON also imports Player Castles object #11013 into candidate
room #30504, the Fists of Fury inn
([reset](../../../areas/zon/fists_gh.zon#L9);
[object prototype](../../../areas/obj/player_castles.obj#L121), “altar of
bonding”). It is not defined in the empty candidate OBJ file.

One SHP record names keeper #30503, room #30502, and 14 product VNUMs:
#53, #377, #364, #347, #108, #109, #111, #3050, #6089, #357, and #491–#494.
The same 14 IDs appear in G resets in the candidate ZON. Current OBJ sources
define 13 of these products: twelve occur in Heavens OBJ, whose area is
listed in full areas/AREA, and #3050 occurs in Verz1 OBJ, also listed there.
There is no current OBJ prototype for #6089. Some matching product IDs also
occur in unlisted OBJ files; keep those source definitions separate from
active ownership. The 2026 source commit added gathering bags #491–#494 to
both this SHP list and the ZON G resets; their current prototypes are in
Heavens OBJ ([bag prototypes](../../../areas/obj/heavens.obj#L5055)).

In this checkout areas/world.shp is absent
([configured path](../../../src/world/db.h#L22)). When the non-mini loader
cannot open that file, [open_shop_stream](../../../src/economy/shop.c#L3267)
globs areas/shp/*.shp without checking AREA membership and appends end.shp.
The shop parser converts keeper VNUM #30503 through real_mobile and retains
room VNUM #30502 ([keeper and room fields](../../../src/economy/shop.c#L3489)).
If that conditional fallback path is used with current active tables, those
numbers identify Patrols' halfling soldier and Tharnadian Guard Load Room,
rather than the Fists greedy merchant and store. This is a static source-code
mapping, not a runtime observation.

Compiled code also binds room #30504 to the inn function
([assignment](../../../src/specs/specs.assign.c#L2445)). In current active
Patrols WLD, #30504 is Gnomish Guard Load Room; the candidate's same-number
room is Fists of Fury inn. The same code block names room #30511, for which
no current WLD owner was found. Preserve both compiled numeric references
for code-owner review; neither establishes a live Fists inn.

## History and disposition

WLD, ZON, and SHP histories reach the 2009 area-tree move and a 2013
mode-only commit. MOB history reaches the 2009 move. OBJ history reaches the
2009 move and the 2016 removal of the unnamed #30500 object. In 2026 commit
ed94862f0, four gathering-bag product entries #491–#494 were added to ZON and
SHP. No alternate-stem predecessor or explicit whole-component replacement
appears in these paths. Numeric collisions, the deleted unnamed object, and
later stock additions do not establish that Fists of Fury Guildhall is an
earlier or later Patrols version.

Keep candidate row 42 **RETAIN IN PLACE**. Resolve the competing room and
mobile identities, both Duris3 and candidate-only route targets, the shop
keeper/room mapping, product #6089, Player Castles import #11013, compiled
inn assignment, story/editor/deployment owners, and persisted-VNUM closure
before any activation, move, rename, archive, or merge proposal. See the
[24 builder questions](FISTS_GH_BUILDER_QUESTIONS.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-479-fists_gh-guildhall-map-and-patrols-collisions),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-479-fists_gh-title-history-and-source-identity),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-479-fists_gh-retained-pending-identity-and-shop-review),
[candidate row 42](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42), and
[coverage row 392](ZONE_COVERAGE_REGISTER.csv#L393). No area/source files,
tests, builds, runtime, server, database, or containers were changed or used.

## Checkpoint 891: Patrols role matches and shop-stream qualification

This pass separates Fists of Fury Guildhall's room IDs #30500-#30509 and mobile IDs #30500-#30503 from Patrols' same-number active records. Candidate `areas/obj/fists_gh.obj` is zero bytes, so the candidate owns no current OBJ prototype. ZON fields are classified under the actual loader roles ([`renum_zone_table`](../../../src/world/db.c#L1612)).

The WLD findings remain unchanged: the ten-room/21-direction graph has one local component and one up exit to inactive Duris3 #157556. Candidate-identity active inbound remains zero, and its two already-counted source-only approaches are not added again. A role-aware scan found 70 external ZON room-role operands and 24 mobile-prototype operands, all in active `patrols.zon`. They are Patrols placements of Patrols' own guard-room and soldier identities, not resets into Fists of Fury's separate rooms or golems ([Patrols resets](../../../areas/zon/patrols.zon#L10)). No external reset references an Fists-owned OBJ because the current candidate OBJ file defines none.

No external OBJ type-25/type-29 stored destination targets #30500-#30509. No external QST item/giver field or story-sidecar typed value matches the candidate room/mobile sets. In this range the direct compiled code match is `real_room0(30504)`, binding the inn callback to active Patrols' Gnomish Guard Load Room; it is not evidence that the unlisted Fists inn is loaded ([inn binding](../../../src/specs/specs.assign.c#L2445)). The separate #30511 code reference remains outside the candidate room range and unresolved as recorded in Checkpoint 479.

The only parser-valid `N`-format SHP record whose keeper or room falls in the candidate range is the candidate's own record: keeper #30503, room #30502, fourteen products. No other parser-valid shop keeper or room field targets #30500-#30509. If the missing `areas/world.shp` causes the non-mini fallback to glob all shards, #30503/#30502 resolve through current active tables to Patrols' halfling soldier and Tharnadian Guard Load Room. The combined fallback also contains eight legacy `O`-format `luiren.shp` records that the current parser rejects; source inclusion does not establish a successful shop catalog or live keeper mapping ([fallback and parser](../../../src/economy/shop.c#L3267), [format gate](../../../src/economy/shop.c#L3360), [legacy records](../../../areas/shp/luiren.shp#L1), [candidate shop](../../../areas/shp/fists_gh.shp#L1)).

Keep candidate row 42 **RETAIN IN PLACE**. Preserve the empty current OBJ source and the 2016 deletion evidence without recreating an object; keep the Player Castles #11013 import, five M resets, fourteen G/SHP product IDs including missing #6089, active Patrols room/mobile identities, the two prior source-only approaches, the #30504 inn binding, and the Duris3 boundary. No WLD total changes. See the [24 builder questions](FISTS_GH_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-891-fists_gh-role-aware-resets-and-shop-input), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-891-fists_gh-retains-patrols-and-shop-identities), [candidate row 42](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L42), and [coverage row 392](ZONE_COVERAGE_REGISTER.csv#L393). No area/gameplay source, tests, builds, runtime, server, database, or container was changed or used.
