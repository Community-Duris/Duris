# Ashrumite Village audit supplement

Checkpoint 772 completes the static typed-dependency inventory for active
Ashrumite Village. It reconciles reset references in both directions, the
external quest consumer, the local shop records and their loader semantics,
and compiled references that use Ashrumite room or prototype numbers. The
review is source-only; it does not establish live stock, reset availability,
travel, or persisted-player behavior.

## Scope and source inventory

Ashrumite remains the active full-area contributor in areas/AREA, with WLD,
MOB, OBJ, ZON, QST, SHP, and story-sidecar files. Its current bounds and 12
native Q/QA contracts are recorded in the [coverage register](ZONE_COVERAGE_REGISTER.csv#L15)
and the [source-comprehensive story dossier](../zone-stories/ASHRUMITE_VILLAGE.md).
The SHP file has 12 record headers: #66015–#66018, #66020–#66021, and
#66040–#66045. The coverage row previously listed zero shops and blank bounds;
it is corrected to first #66015, last #66045, count 12. These are source
records, not proof of currently operating shops.

## Outbound reset references

The Ashrumite ZON has 38 cross-source prototype reset commands: one mobile
reference and 37 object references. Exact command locations and current
prototype owners are grouped below. “Heavens” is the active owner for the
listed Heavens objects; #90 and #101 also appear in the unregistered Mavulsk
object file, so their active owner is resolved by the current source list,
not by the numeric match alone.

| Reset target | Ashrumite ZON lines | Current owner / finding |
| --- | --- | --- |
| Mobile #1252 | 60 | Heavens mobile |
| Objects #90, #101, #188, #189, #203, #204, #256, #361, #363, #368, #369, #385, #386, #390, #396, #399, #835 | #90: 25; #101: 78, 185; #188: 87; #189: 88; #203–#204: 215–216; #256: 53, 97, 252, 255, 271; #361: 41, 48; #363: 100; #368–#369: 101–102; #385–#386: 164–165; #390: 209, 223, 240, 259, 279; #396: 210, 224, 241, 260, 280; #399: 211, 225, 242, 261, 281; #835: 86 | Heavens objects; #90 and #101 also have unregistered Mavulsk definitions |
| Object #3097 | 47 | Verz1 object |
| Object #6089 | 235 | No current object prototype found; thief-guildmaster inventory reference remains unresolved |

The ZON labels these external prototype rows as types not in the local file.
Their declarations do not by themselves prove successful runtime loading.
The missing #6089 differs from the active Heavens and Verz1 imports and is
already described in the story dossier as a stale/missing inventory item.
See the [Ashrumite ZON](../../../areas/zon/ashrumite.zon#L25) and
[prototype source](../../../areas/obj/ashrumite.obj#L1).

## Inbound zone resets and quest consumer

| Source | Exact external reference | Classification |
| --- | --- | --- |
| Active Alatorin ZON | Line 2107: G resets Ashrumite object #66004. Line 2805: M resets Ashrumite mobile #66004 into room #83361 at 4%. | Active cross-area reset references to distinct OBJ and MOB prototypes that share a VNUM. |
| Active Brass ZON | Lines 885–886: G resets Ashrumite objects #66048 and #66045. | Active cross-area reset references. |
| Unregistered brass-old-1 ZON | Lines 895–896: repeats G references to #66048 and #66045. | Candidate-source records only; preserve with the existing RETAIN IN PLACE plan. Brass Old 1 already records these rows in its [audit](BRASS_OLD_1_AUDIT_REVIEW.md#L103). |
| Unregistered nexus56 ZON | Line 1385: G resets Ashrumite object #66025. | Candidate-source record only; preserve with Nexus56 RETAIN IN PLACE. The exact import is now called out in the [Nexus56 audit](NEXUS56_AUDIT_REVIEW.md). |
| Active IceCrag QST | Line 157: G I #66058, an Ashrumite parchment object. | One external QST item consumer found. IceCrag's sidecar completion key repeats #66058 at line 784; it is not a new prototype owner. |

The Ashrumite QST itself has 12 native contracts. Its existing dossier
documents the missing requested disc #4372 and absent output prototypes #66066
and #66067. The active platinum disc #4022 in Caves of Skelenak is a different
VNUM and is not evidence that #4372 is supplied. Preserve the existing
contracts until their intended materials and outputs are selected. The
[IceCrag QST](../../../areas/qst/icecrag.qst#L157),
[IceCrag sidecar](../../../areas/story/icecrag.story.json#L784), and
[Ashrumite dossier](../zone-stories/ASHRUMITE_VILLAGE.md) carry the source
details.

## Shop records and numeric-list ambiguity

The 12 Ashrumite SHP headers are source records. Their numeric buy lists
include values in the Ashrumite object range, but matching a number to an OBJ
VNUM is not sufficient to treat it as a shop product. In
[shop.c](../../../src/economy/shop.c#L3190), read_type_list describes and
parses these values as item types; boot_the_shops calls that reader and stores
each BUY_TYPE as a byte at lines 3466–3472. Numeric values above 255 therefore
also require owner clarification about their intended category and conversion.
The [Ashrumite SHP](../../../areas/shp/ashrumite.shp#L1) and the other exact
numeric-list hits—Alatorin SHP line 4 contains 66004 and Nexus56 SHP line 545
contains 66025—should be treated as source-list evidence, not as proof that
those VNUMs are stocked. See the [Alatorin SHP](../../../areas/shp/alatorin.shp#L4)
and [Nexus56 SHP](../../../areas/shp/nexus56.shp#L545) records.

This checkout has no areas/world.shp. In non-mini mode,
[open_shop_stream](../../../src/economy/shop.c#L3267) falls back to globbing
areas/shp/*.shp when the combined file cannot be opened. That conditional
loader path can encounter unregistered candidate files too, but it does not
establish which shops boot successfully or what players can buy. No shop
runtime was started for this review.

## Code and room dependencies

| Source location | Ashrumite IDs and use |
| --- | --- |
| [constant.c](../../../src/core/constant.c#L1728) | Home/start and hometown tables refer to Ashrumite rooms, including #66001 and the mapped town rooms. |
| [specs.assign.c](../../../src/specs/specs.assign.c#L765) | Mobile #66041 receives the world_quest procedure. Lines 1073–1083 assign local mobile procedures for drunk, city guard, guild guards, janitor, and money changer; lines 2548–2550 bind inn, dump, and pet-shop room procedures to #66054, #66080, and #66116. |
| [specs.guards.c](../../../src/specs/specs.guards.c#L288) | Five room gates use Ashrumite rooms #66065, #66088, #66028, #66084, and #66078. |
| [justice.c](../../../src/combat/justice.c#L64) | Gnome-town justice row names home #66069, guard #66091, jail #66001, and return/report room #66074. |
| [specs.highway.c](../../../src/specs/specs.highway.c#L277) and [wonder_actions.c](../../../src/item/wonder_actions.c#L327) | The wonder-gem table names #66034–#66043; effect selection chooses #66034 plus a random offset from 0 through 9. This is a code producer path, not proof of personal recovery. |

A scan of current object declarations found no external ITEM_TELEPORT
destination or ITEM_SWITCH value[1] room field into Ashrumite rooms
#66001–#66153, and Ashrumite owns no local ITEM_TELEPORT or ITEM_SWITCH
object. Regular WLD exits are already inventoried: four outbound and
four inbound cross-area edges, with one positive unresolved target at #66149
east to inactive #224166. See [WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md)
and keep this existing route finding separate from typed object ownership.

## Identity, candidate plans, and disposition

The [duplicate/history review](DUPLICATE_AND_REPLACED_ZONES.md) and CP702
coverage row establish Ashrumite as the current active source identity. The
tracked Quartermaster move from Ashrumite mobile #66054 to Siege #401020,
followed by Siege's 2026 retirement of the stale reset, is service/reset
history rather than a whole-area replacement. No same-type active duplicate
of Ashrumite's local mobile or object prototypes was found. Preserve the
unregistered Mavulsk #90/#101 definitions and its existing candidate plan;
their numeric overlap does not make Mavulsk the active source of Ashrumite's
Heavens imports.

Keep Ashrumite **DO NOT MOVE** as an active areas/AREA contributor. Keep
brass-old-1 and nexus56 under their existing RETAIN IN PLACE plans and
preserve their candidate-source references to Ashrumite. Do not add an attic
candidate row for this active area, merge sources, or change native gameplay.
The new [owner question 20](../zone-stories/ASHRUMITE_VILLAGE.md#area-owner-questions-and-recommended-resolutions)
asks how the oversized SHP trade-list numbers are intended to map to trade
categories; until resolved, retain the declarations and do not call them
stock. Static source inventory is complete; live loading, player access,
availability, and persisted state remain unqualified.
