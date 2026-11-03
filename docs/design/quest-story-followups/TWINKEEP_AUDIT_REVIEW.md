# Twinkeep shop-only source audit

This review covers the two records in
[`areas/shp/twinkeep.shp`](../../../areas/shp/twinkeep.shp#L1) at source
snapshot `c500ac51b8cd26540943257e7ca4e9807263ef74`. It is an SHP-only
component with no matching WLD, MOB, OBJ, ZON, QST, map, or story-sidecar
source. No gameplay or area source was changed. The generated
`areas/world.shp` is absent from this checkout; deployed aggregate state and
runtime behavior were not tested.

## Shop records and typed references

| Shop key | Source line | Keeper | `in_room` | Roaming | Product VNUMs | Current typed resolution |
| ---: | ---: | ---: | ---: | --- | --- | --- |
| #6500 | 1 | #6500 | 0 | Yes | #6500 | Active Shaughin MOB #6500 and OBJ #6500; the OBJ is a type-25 wooden trapdoor targeting room #6600. |
| #6519 | 38 | #6519 | #6656 | No | #203, #204, #6537 | #6519 has no current MOB definition; #6656 is an active Shaughin2 room; #203/#204 are active Heavens objects; #6537 has no current OBJ definition. |

There are two shop records and four distinct product VNUMs. Three product
VNUMs resolve to current active OBJ prototypes; #6537 does not, although that
number exists as a WLD room in Shaughin. The type-specific distinction matters:
SHP products are OBJ references, and `in_room` is a room location, not an exit.
The #6500 record is marked roaming with `in_room` 0. The #6519 record is
non-roaming at active Shaughin2 room #6656
([shop records](../../../areas/shp/twinkeep.shp#L1),
[keeper](../../../areas/mob/shaughin.mob#L1),
[trapdoor object](../../../areas/obj/shaughin.obj#L1),
[Heavens products](../../../areas/obj/heavens.obj#L2378),
[Shaughin2 location](../../../areas/wld/shaughin2.wld#L1581)).

## Shared shop keys and source history

The unregistered [`tharnadia1.shp`](../../../areas/shp/tharnadia1.shp#L666)
also defines keys #6500 and #6519 with the same keeper and `in_room` fields,
but neither full record matches its `twinkeep.shp` counterpart. Both #6500
records are roaming, use keeper #6500 and location 0, and list product #6500;
their buy percentages, messages, and killable flags differ. Both #6519
records are non-roaming for keeper #6519 at room #6656; `tharnadia1` lists
additional product #3050, which `twinkeep` does not. Their prices, messages,
and killable flags also differ. Preserve these as two source identities.

Active `tharnadia.shp` has shop #132650, but that remapped record uses keeper
#132650, room #133026, and products #203/#204/#3050; it is not an exact
replacement for the Twinkeep #6500 or #6519 record
([active record](../../../areas/shp/tharnadia.shp#L669)). The tracked
`twinkeep.shp` path reaches the 2009 area-tree move and was edited in 2013.
Those commits contain no explicit retirement or replacement statement.
Source history and shared numbers do not establish which shop an owner wants
to preserve.

## Selection and conditional loader path

The top-level area manifest lists [`shaughin`](../../../areas/AREA#L34) and
[`shaughin2`](../../../areas/AREA#L35) but has no `twinkeep` or `tharnadia1`
entry. The normal shop
[aggregator](../../../areas/src/shp/make_shp.c#L7) reads `AREA` and opens each
listed `<area>.shp`, so it excludes both sources. Runtime
[`open_shop_stream`](../../../src/economy/shop.c#L3264) first opens the
combined `areas/world.shp`; if a non-mini process cannot open that file, it
globs every `areas/shp/*.shp` and appends `end.shp`. With no `areas/world.shp`
in this checkout, that fallback would include `twinkeep.shp` and
`tharnadia1.shp` if the server were started from this tree. No server was
started, so this documents a source path rather than an observed boot.

If both #6500 records enter that fallback, the active Shaughin keeper #6500
has two matching roaming shop records. The generic
[`singleton_shop_id`](../../../src/world/world_singletons.c#L75) returns no
unique fallback shop ID after it sees multiple roaming matches unless a
valid explicit shop binding selects one. The duplicated non-roaming #6519 /
room #6656 pair is likewise unresolved by source identity, and no active MOB
#6519 currently exists. These are conditional loader and binding findings;
they do not establish a live shop assignment or failure.

## Story boundary and disposition

The source contains shop messages about shiny metal wealth, a butterfly, and
ordinary trade refusals. It contains no map, room descriptions, quest, or
story-sidecar evidence from which to infer a setting or journey. Keep
`twinkeep.shp` **RETAIN IN PLACE** as separate shop-source evidence. Do not
merge its records with `tharnadia1`, activate either source, or archive/move
them until the owner reconciles generated/deployed `world.shp` selection,
keeper bindings, and editor or persisted shop identities. See the
[20 builder questions](TWINKEEP_BUILDER_QUESTIONS.md),
[Tharnadia1 audit](THARNADIA1_AUDIT_REVIEW.md),
[Shaughin dependency review](SHAUGHIN_AUDIT_REVIEW.md#typed-dependencies-and-bounded-negative-checks),
[coverage row 459](ZONE_COVERAGE_REGISTER.csv#L460),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-546-twinkeep-has-no-wld-graph-or-room-edges),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-546-twinkeep-and-tharnadia1-share-shop-keys-but-differ),
and [retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-546-twinkeep-retained-pending-shop-loader-owner-review).
