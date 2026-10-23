# Verzanan source story and dependency review

Canonical key: area:verz1. Source title: Verzanan. Physical coverage row 339.

This is a static review of the current source snapshot. It records authored text,
placements, typed references, and bounded source history. It does not establish
that resets ran, items are in stock, a route is traversable, or a story was
completed in play.

## Source inventory and world shape

| Source | Finding |
| --- | --- |
| [Full area list](../../../areas/AREA#L15) | verz1 is a current full-area contributor. The list records *28-32 (using objs). |
| [Quest index](../../../areas/qst/AREA#L15) | The historical quest index also names verz1, with the different note *28-33 (using objs). There is no local verz1.qst file. |
| [World rooms](../../../areas/wld/verz1.wld#L1) | The only two room records are #2800 and #3257. The first has a down exit to #3257; the second returns up to #2800. Both descriptions call themselves bookend rooms and contain no city setting. |
| [Zone header](../../../areas/zon/verz1.zon#L1) | The file has zone header #28 and ends without M, G, E, O, P, or D reset commands. It places none of the local mobiles or objects. |
| [Mobile prototypes](../../../areas/mob/verz1.mob#L1) | 158 local MOB records, #2800–#3246. Their names and descriptions outline city residents and services, but no local reset places them. |
| [Object prototypes](../../../areas/obj/verz1.obj#L1) | 180 local OBJ records, #2800–#3257. The set includes maps, a clock tower, a city key, equipment, food, prison and casino objects, and the counter described below. |
| Local services and story files | No local SHP file or story sidecar is present. The coverage row records the source-file census and sidecar absence. |

The static room graph is only the two-room reciprocal pair. There is no active
WLD exit from another source into either local room, and no external WLD record
targets #2800 or #3257. This says nothing about routes implemented outside WLD.

## What the authored text supports

The mobile catalog sketches a large civic city: a high priestess and rogue
guildmaster, a banker, watch guards, bailiffs and a High Justice, casino staff,
merchants, artisans, commoners, and stable animals. Examples include the
[temple and guild prototypes](../../../areas/mob/verz1.mob#L762),
[banker](../../../areas/mob/verz1.mob#L1018),
[casino dealer](../../../areas/mob/verz1.mob#L1515), and
[court officials](../../../areas/mob/verz1.mob#L2041). These are prototype
descriptions, not evidence those characters currently spawn.

The object set supports parts of that city concept. It includes two map
prototypes, a clock tower and key to the city, and the #3097 counter. The
counter's sign says “Welcome to the Bank” and advertises deposit, withdrawal,
and balance commands ([counter and sign](../../../areas/obj/verz1.obj#L1199)).
The local QST source is absent, so the catalog does not establish a complete
quest chain or player-facing route through these locations.

The word “Verzanzan” appears in the two placeholder room descriptions while
the zone title and most other source text use “Verzanan.” The mismatch is
visible in the [room file](../../../areas/wld/verz1.wld#L2) and
[zone title](../../../areas/zon/verz1.zon#L2); it is a builder question, not a
license to rename the source.

## Code bindings and local placement

The source tree still compiles the Verzanan procedure module
([Makefile entry](../../../src/Makefile#L754)). However, all 72 explicit
mobile-special references to local Verz1 mobile VNUMs in specs.assign.c sit in
two preprocessor-disabled blocks, beginning at
[line 361](../../../src/specs/specs.assign.c#L361) and
[line 488](../../../src/specs/specs.assign.c#L488). The old portal bindings
are also in a disabled block at
[line 1823](../../../src/specs/specs.assign.c#L1823). Compiled procedure code
alone does not make those NPC or portal interactions active.

Object #3097 is a separate current code dependency. Its assignment uses the
storage-locker hook in [specs.assign.c](../../../src/specs/specs.assign.c#L1308);
the balance command also checks whether that object is in the player's room
([actoth.c](../../../src/cmd/actoth.c#L2431)). The source label “Bank” and the
code hook should both be preserved during any future owner review.

## Typed external dependency closure

The counts below scan current source records by the object or mobile VNUM
field read by the loaders. “Full area” means the consumer stem is present in
the current top-level areas/AREA list. Other source files are retained in the
count but are not treated as full-world runtime consumers.

| External source type | Full-area references | Full-area targets and files | Outside-full-area references | Outside-full-area targets and files |
| --- | ---: | --- | ---: | --- |
| ZON M mobile resets | 0 | No local MOB target | 0 | No local MOB target |
| ZON G object resets | 128 | 49 local OBJ VNUMs in 13 stems | 103 | 35 local OBJ VNUMs in 35 stems |
| ZON E object resets | 14 | 3 local OBJ VNUMs in 1 stem | 0 | None |
| ZON O object resets | 47 | 2 local OBJ VNUMs in 36 stems | 2 | 1 local OBJ VNUM in 2 stems |
| ZON P nested-object resets | 13 | 4 child OBJ VNUMs in 2 stems | 0 | None |
| SHP product lists | 109 | 46 local OBJ VNUMs in 12 shop files | 108 | 29 local OBJ VNUMs in 38 shop files |
| QST G I item grants | 2 | #2800 and #3003 in active shipy and surfacemini sources | 0 | None |

The ZON reset total is 202 full-area object references across 43 consumer
stems, plus 105 references across 35 stems outside areas/AREA. There are 58
distinct local object VNUMs in the ZON reset target set:

2800, 2801, 2805, 2847, 2997, 3000, 3001, 3002, 3003, 3004, 3008, 3009,
3010, 3011, 3012, 3016, 3018, 3021, 3023, 3024, 3025, 3029, 3033, 3034,
3036, 3038, 3040, 3041, 3042, 3043, 3044, 3045, 3046, 3049, 3050, 3054,
3055, 3057, 3060, 3061, 3069, 3070, 3071, 3075, 3076, 3080, 3085, 3088,
3091, 3097, 3100, 3102, 3220, 3222, 3234, 3237, 3239, 3247.

The 58-item set is exact for reset targets. Shop lists add two further
source-only prototypes, #2812 and #2813; the two QST item grants target #2800
and #3003, already in the reset set. Preserve the owner and VNUM of all
60 externally referenced prototypes. External resets are placements in other
areas, not links into Verzanan's two WLD rooms.

Two full-area consumer examples are well established: Lava resets #3097 to
#99725 ([Lava source](../../../areas/zon/lava.zon#L105)), and Ixxillikor
imports the same prototype ([Ixxillikor source](../../../areas/zon/ixxillikor.zon#L131)).
Across all external ZON files, #3097 has 41 full-area O reset rows and two
outside-full-area O rows. The other full-area O target is fountain #3088.
The currently active SHP product consumers are alatorin, bs, cbl_hall,
charing, desolate, khildarak, kimordril, kobold, player_castles, rtun,
surfacekeeps, and tharnadia. Product counts follow
[boot_the_shops](../../../src/economy/shop.c#L3322), which reads product VNUMs
after N until zero; numeric prices and later shop fields are not counted as
products.

No duplicate local MOB or OBJ definitions were found in other area prototype
files. No external WLD room target, external M reset, or structured story
sidecar VNUM reference points into Verz1's local room/mobile/object sets.
Conversely, verz1.zon has no outbound reset rows and its two WLD exits remain
local.

## Title, history, and replacement boundary

The normalized current [.zon title](../../../areas/zon/verz1.zon#L2) is
Verzanan and the active-title scan
reports no repeated title. The tracked WLD history shows the same two
bookend-room contents by commit 71d731c5b in November 2013; the current WLD
bytes match that version. Available history does not explain when the broader
city prototype set was drafted, why the room file became a two-room stub, or
whether any larger map was retired.

Keep Verzanan and Verspin as separate current contributors. Their source keys,
titles, and authored files differ; Verspin has its own
[200-room map](../../../areas/wld/verspin.wld#L1) and
[local QST source](../../../areas/qst/verspin.qst#L1). No source evidence
reviewed here identifies Verspin as a replacement for Verzanan. The old
hometown comment explicitly calls itself a relic kept for posterity
([constant.c](../../../src/core/constant.c#L1748)). An old room-procedure
message says “outside the great gates of Verzanan,” but the code moves the
character to room #3600
([specs.room.c](../../../src/specs/specs.room.c#L638)); it does not target
either Verz1 placeholder room.

## Review result

Keep area:verz1 DO NOT MOVE and do not merge it with Verspin. The high
inbound object dependency count, the active #3097 locker/balance binding, the
full-area membership, and unresolved history make transfer or attic retirement
unsafe on this static evidence. No candidate-specific attic move plan is
warranted for an active contributor. Any later builder proposal must first
account for the 60 referenced OBJ prototypes, each full-area consumer, the
source-only consumer files, and the storage hook. No area or gameplay source
was changed in this review.

See the [twenty builder questions](VERZANAN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[coverage row 339](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L339),
[static graph review](../quest-story-followups/WORLD_CONNECTIVITY.md#checkpoint-426-verzanan-static-room-graph-and-absent-ingress),
[title and history review](../quest-story-followups/DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-426-verzanan-title-and-bounded-source-history), and
[retention review](../quest-story-followups/ATTIC_MIGRATION_PLAN.md#checkpoint-426-verzanan-preserved-as-an-active-dependency-source).

## Checkpoint 786: Desolate reset and shop consumers

Active Desolate resets Verz1 object #3097 through O at
[desolate.zon line 292](../../../areas/zon/desolate.zon#L292) and #3050
through G at [line 494](../../../areas/zon/desolate.zon#L494). Its local shop
#22290 also lists #3050 as a product. Preserve the Verz1 owner identity,
including #3097's existing storage-locker binding. Reset and shop declarations
do not prove admission or current availability. See the
[Desolate typed audit](../quest-story-followups/DESOLATE_AUDIT_REVIEW.md#current-source-inventory-and-reset-sources).
