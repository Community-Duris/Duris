# forestb - source bundle and shop audit

Canonical key: component:forestb. Physical coverage row 394 / CSV line
395; candidate dependency row 44 / CSV line 44. Reviewed October 11, 2026,
Checkpoint 481, against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697.
This is a static source, typed-reference, history, and shop-loader review. It
does not establish runtime loading, shop assignment, quest availability, or
permission to activate or move these files.

## Source scope, title, and story

The same-stem source bundle contains [MOB](../../../areas/mob/forestb.mob#L1),
[OBJ](../../../areas/obj/forestb.obj#L1),
[QST](../../../areas/qst/forestb.qst#L1), and
[SHP](../../../areas/shp/forestb.shp#L1). There is no same-stem WLD or ZON
file, the component is absent from full [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA), the raw AREA annotation is blank,
and no story sidecar or same-stem editor-map file appears in the current tree.
No ZON title exists to compare with the filename.

The MOB file contains 45 prototypes #38200-#38244. They describe forest
wildlife, thorny and poisonous plants, tribal hunters and villagers, fiends,
and the village shaman and apprentice. The OBJ file contains 62 prototypes
#38200-#38261. The QST has three giver records and four Q contracts: two for
the huge bluebird, one for the young tribal woman, and one for the tribal
shaman. This is story-bearing source, not an empty placeholder. No local WLD
or ZON is present to provide a map or reset placements.

The typed QST directives are important to preserve. In the quest loader,
`G` goals are the items/coins a player offers and `R` goals are the reward
items, coins, skill, or experience ([parser](../../../src/world/quest.c#L2099));
completion handling validates and removes offered `G` goals and grants `R`
goals ([completion](../../../src/world/quest.c#L168)). The four contracts
resolve as follows:

| Giver and contract | Offered `G` goals | Reward `R` goals |
| --- | --- | --- |
| Huge bluebird #38200, first contract | OBJ #38200, red-feather staff | 500 coins and OBJ #38201, white worm piece |
| Huge bluebird #38200, second contract | OBJ #38204, blue-feathered nest | OBJ #38202, white worm piece |
| Young tribal woman #38230 | OBJ #38241, wooden comb | OBJ #38260, emerald brooch; 500 experience |
| Tribal shaman #38234 | OBJ #38244, green-spotted deerskin | OBJ #38224, bearhide amulet; 500 experience |

The latter two item exchanges match their broad quest descriptions. The
bluebird's first narrative speaks of meat and transferred feathers, while
its directives consume the red-feather staff and reward a white worm piece;
record that prose/directive difference for the quest owner instead of
silently interpreting or rewriting either side.

## Typed references, map ownership, and history

All eight QST item-goal VNUMs (four offered `G` items and four rewarded `R` items) resolve to prototypes in this candidate OBJ file.
All four SHP stock VNUMs also resolve to candidate OBJ records: #38231 siege
horn, #38246 yellow parchment, #38248 boiling red potion, and #38247 warding
scroll. The candidate MOB and OBJ both define #38200, which is a mobile/object
type overlap, not one shared record. SHP keeper #38235 resolves by number only
to the apprentice-shaman MOB in this candidate bundle. Its shop room is
#38259; no current WLD source defines room #38259, while this candidate OBJ
uses #38259 for a crude wooden comb. Keep the room, mobile, and object
namespaces distinct.

An exact same-type scan found no other current MOB prototype #38235, no other
current OBJ prototype for the four stocked items, and no current WLD room
#38259. There is no candidate WLD on which to compute exits, components,
fall/current counts, or range mismatch. No active-WLD inbound exit targets a
candidate room identity, and the curated inactive-WLD approach totals do not
change. The absence of a same-stem ZON also means there is no local reset
record placing the quest givers or shopkeeper; this does not rule out other
source-level or dynamic placement paths.

Bounded file history reaches the 2009 area-tree move for the four sources.
The MOB has a 2010 typo-only commit in its visible path; QST and SHP reach a
2013 commit titled `stuff`. No alternate-stem predecessor or explicit
whole-component replacement evidence was found in those paths. These history
entries do not identify an owner or prove that the files are obsolete.

## Conditional SHP fallback and keeper-index risk

`areas/world.shp` is absent in this checkout. In non-mini mode,
`open_shop_stream` first tries that catalog path, then globs
`areas/shp/*.shp` without checking membership in `areas/AREA`, concatenating
the source files ([loader](../../../src/economy/shop.c#L3267)). Therefore,
if that fallback path is used, it can include [forestb.shp](../../../areas/shp/forestb.shp#L1)
even though forestb is absent from the full AREA list.

The SHP record names keeper mobile #38235 and room #38259. `boot_the_shops`
converts the keeper VNUM through `real_mobile`; the lookup returns -1 when the
VNUM is not in the loaded mobile table ([lookup](../../../src/world/db.c#L8587)).
`assign_the_shopkeepers` then directly uses `mob_index[keeper]` without a
negative-index check ([assignment](../../../src/economy/shop.c#L3602)). As
forestb is unlisted and no other current MOB file defines #38235, a complete
valid catalog that reaches shop assignment while this MOB is absent would pass
keeper -1 to an unchecked `mob_index[keeper]` access. This is a conditional
source-code risk, not a runtime observation. The current shard fallback does
not establish that assignment is reached: its aggregate includes eight legacy
`O`-format records in `luiren.shp`, and the `N`-only parser calls
`fatal_boot_error` at that format gate ([parser](../../../src/economy/shop.c#L3360),
[legacy records](../../../areas/shp/luiren.shp#L1)). Source inclusion alone
does not establish a complete shop catalog.

## Disposition

Keep candidate row 44 **RETAIN IN PLACE**. Resolve the source owners and
manifest intent; reconcile the bluebird's first QST prose and G/R directives;
identify how the three quest givers and apprentice shaman are placed; confirm
the #38235 keeper and #38259 room identities; and review fallback shop-file
aggregation and negative keeper handling. Complete editor, deployment,
external typed-reference, and persisted-ID closure before any activation,
move, rename, archive, or merge proposal. See the
[24 builder questions](FORESTB_BUILDER_QUESTIONS.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-481-forestb-source-bundle-and-shop-references),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-481-forestb-title-history-and-typed-identities),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-481-forestb-retained-pending-quest-and-shop-review),
[candidate row 44](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L44), and
[coverage row 394](ZONE_COVERAGE_REGISTER.csv#L395). No source-area files,
code, tests, builds, runtime, server, database, or containers were changed or
used.


## Checkpoint 893: role-aware cross-source closure

This pass tracks Forestb's mobile IDs #38200–#38244 and object IDs #38200–#38261 by prototype type. Its SHP room field #38259 remains a separate, unresolved room-role number; no same-stem WLD exists and no current WLD record or literal WLD reference contains #38259. The earlier story findings are unchanged: three quest givers and four Q contracts, eight offered/rewarded candidate-OBJ item operands, and four shop products that resolve within the Forestb OBJ file.

A loader-role scan across all 442 ZON files found no external mobile or object prototype, reset-room, container, or door-room operand matching the candidate IDs or SHP room #38259. The 264 other QST files contain no candidate-range giver or `G I`/`R I` item field. All 232 story sidecars parse without errors and none has a typed `mob_vnum`, `giver_vnum`, or `item_vnums` match, or a completion-key `I:` token for a candidate object. No sidecar room-vnum field occurs in this corpus.

An exact same-type scan of current sources found no other MOB definition in #38200–#38244 or OBJ definition in #38200–#38261. The all-source OBJ record scan recognizes 872 type-25 `ITEM_TELEPORT` and 380 type-29 `ITEM_SWITCH` records; no external type-25 value[0] or type-29 value[1] target matches the SHP room #38259. Forestb's own [blood pentagram OBJ #38234](../../../areas/obj/forestb.obj#L537), a separate typed identity from the #38234 shaman mobile/QST giver, stores portal destination #38297, and its [black rift #38253](../../../areas/obj/forestb.obj#L818) stores #38295. Neither destination has a current WLD record or literal WLD reference anywhere under `areas/`; the two source-only outbound portals therefore raise an unresolved missing-map/room question, not an established route. No direct C/C++ numeric use of #38200–#38261 was found. The object scan covers 442 files and 21,683 records; the unparsed `end.obj` terminator is not a normal prototype. No room graph or inbound count applies because Forestb has no WLD.

The 180 shop shards yield 712 parser-shaped `N` records. The only matches in keeper, room, or product roles are Forestb's own record: keeper #38235, shop room #38259, and products #38231, #38246, #38248, and #38247. No other shop uses a Forestb mobile, object, or room number in those roles. This confirms the bundle's internal shop references but does not qualify the conditional fallback as a successful catalog: it includes the eight Luiren `O` records rejected by the current parser. If a separate valid catalog reaches `assign_the_shopkeepers()` with #38235 unresolved, the unchecked negative-index code path remains an owner-reviewed risk; the current shard scan does not show that it runs.

Keep candidate row 44 **RETAIN IN PLACE**. Checkpoint 893 adds no cross-area typed dependency or map edge and does not settle whether this source bundle is intended to be loaded. Preserve the #38235/#38259 shop identity question, local portals #38234→#38297 and #38253→#38295 with no current WLD destinations, four stock items, three QST givers, all eight item goals, bluebird prose/directive discrepancy, absent WLD/ZON and manifests, and conditional keeper-index concern. The 24 builder questions remain open, with FB-04/15/16/17/23 refined to include portal, loader, and exact-source evidence. See the [builder questions](FORESTB_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-893-forestb-has-no-static-map-ingress), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-893-forestb-retains-unplaced-quest-and-shop-identities), [candidate row 44](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L44), and [coverage row 394](ZONE_COVERAGE_REGISTER.csv#L395). No source/gameplay area, C/C++, test, build, runtime, server, database, migration, or container was changed or used.
