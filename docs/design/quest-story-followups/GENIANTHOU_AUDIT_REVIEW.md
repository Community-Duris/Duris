# Gimluk’s Cottage source audit

Canonical key: component:gianthou. Physical coverage row 399 / CSV line 400;
candidate dependency row 49 / CSV line 49. Reviewed October 11, 2026,
Checkpoint 486, against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static source,
connectivity, narrative, identity, history, and typed-reference review. It
does not establish runtime loading, travel, shop behavior, editor ownership,
persisted world state, or permission to activate or move these files.

## Source inventory and range ownership

The unregistered component consists of [WLD](../../../areas/wld/gianthou.wld#L1),
[MOB](../../../areas/mob/gianthou.mob#L1),
[OBJ](../../../areas/obj/gianthou.obj#L1),
[ZON](../../../areas/zon/gianthou.zon#L1),
[QST](../../../areas/qst/gianthou.qst#L1), and
[SHP](../../../areas/shp/gianthou.shp#L1). WLD has 271 rooms from #42600
through #42870; MOB has 55 prototypes; OBJ has 122; QST has 14 giver records
and nine native QA contracts; and SHP has four shops. ZON header #426 is
titled “Gimluk’s Cottage,” ends at #42870, and has reset mode 2. No same-stem
story sidecar, editor map, or cap file was found.

Neither [areas/AREA](../../../areas/AREA#L201) nor
[areas/qst/AREA](../../../areas/qst/AREA#L185) lists gianthou. Both assign
#425–#426 to Pworm; areas/AREA then marks #427–#428 open
([range entry](../../../areas/AREA#L202)). The candidate’s #426 zone header
therefore overlaps Pworm’s numeric manifest band, and its rooms extend into
the bands marked open. This is a manifest/range ownership conflict, not
evidence of exact same-number WLD records in Pworm. The candidate has no raw
AREA annotation. The ZON title is uniquely found among current ZON files.
Resolve the range owner before any manifest addition or activation.

## Authored map and source-only boundaries

The WLD contains 698 directions: 696 local edges and two nonlocal targets.
Its local graph is one weak component of 271 rooms, and directed traversal
from #42600 reaches all 271. The source-only edges are:

| Source exit | Authored target | Static finding |
| --- | --- | --- |
| #42811 D0 | #-1 | Negative target; it is not a positive room destination. |
| #42870 D3 | #274141 | The target is absent from the active loader list and has two different definitions in unlisted WLD files. |

Because gianthou is unlisted, none of the 698 targets resolves through the
current active WLD set: 697 positive targets are unresolved and one target is
negative. There is one cross-zone outbound edge, no cross-zone inbound or
reverse edge, and no active WLD inbound to the candidate’s room IDs. The
positive unresolved targets comprise 696 local references into this unlisted
map and the #274141 boundary.

Room #274141 appears in unlisted [surf.wld](../../../areas/wld/surf.wld#L1555793)
as “Vast Plains of Grassy Meadows” and in unlisted
[surface2011.wld](../../../areas/wld/surface2011.wld#L1556962) as “The Azure
Waters of a Stormy Sea.” Neither source is in areas/AREA; active
[surface.wld](../../../areas/wld/surface.wld#L1) covers a different numeric
band. The target records do not provide a direct return to #42870. Do not
describe this as an active Surf route or choose a target identity without the
map owners.

Twenty rooms have positive WLD fall chance, with authored values from 3
through 75; there are no current fields. WLD stores each chance as a
standalone F token followed by its numeric value
([example](../../../areas/wld/gianthou.wld#L401);
[parser](../../../src/world/db.c#L1395)). The 38 ZON F commands are
mobile-follow resets, a different field
([reset example](../../../areas/zon/gianthou.zon#L6813);
[loader meaning](../../../src/world/db.c#L7747)).
The candidate has no source room outside its declared numeric range and no
source exit from a range-mismatched room.

## Story and quest contracts

The WLD describes Gimluk’s oversized cottage, atrium, fireplace and chimney,
redwood tree, household, gardens and fields, storm cellar, and hidden
passages. Most room descriptions are blank, so much of the authored story
resides in MOB and QST dialogue. The fourteen QST giver records include nine
native QA contracts. They describe trouble from bugs and snakes in the
atrium, Gimluk’s origin and the captive dwarves, the key and family history
involving Gamdin and Djarill, and a request to kill Gimluk and return his
redwood stick as proof. The completion text points the player to Renthis’la
to have the weapon enchanted
([QST story](../../../areas/qst/gianthou.qst#L69)).

The story also has a refusal path: Ugala rejects an item and explicitly
refuses theft and slaughter involving Gimluk’s children
([Ugala QST](../../../areas/qst/gianthou.qst#L173)). Preserve that branch
alongside the revenge request. The source does not explain the zone’s
relationship to other Gimluk references, who owns its story continuity, or
whether blank room prose is intentional.

## Resets, typed references, and shops

The reset mode is 2. The ZON contains 7,136 commands: 37 O, 214 M, 6,634 P,
99 E, 80 G, 32 D, two R, and 38 F. All 54 distinct MOB identities used by
M/F commands resolve to local prototypes. All 115 distinct OBJ identities
used by resets are local, and all 26 QST item references (22 unique) resolve
to local objects. All 28 SHP product relationships also resolve to local
OBJ prototypes. These are source-level references, not evidence that the
resets or shops run. MOB #42618 is a young male ogre child that is not used
by the candidate M/F reset set and is not a QST giver
([prototype](../../../areas/mob/gianthou.mob#L206)).

The 6,634 P commands place objects inside other objects, a large authored
set that should be preserved and reviewed as typed relationships
([example](../../../areas/zon/gianthou.zon#L141)). The four SHP records use
keepers/rooms #42637/#42743, #42644/#42654, #42647/#42856, and
#42654/#42607. They list 1, 8, 5, and 14 products respectively; every
keeper and product has a local prototype. The #42637 shop contains the
refusal “I do not sell to your people!”
([SHP](../../../areas/shp/gianthou.shp#L1)).

The checkout has no areas/world.shp. In non-mini mode, the shop reader can
fall back to areas/shp/*.shp ([fallback](../../../src/economy/shop.c#L3276)).
It converts each keeper through real_mobile()
([lookup](../../../src/economy/shop.c#L3492)); assign_the_shopkeepers()
then indexes mob_index[keeper] without a negative-index guard
([assignment](../../../src/economy/shop.c#L3602)). If that fallback reads
gianthou.shp while its MOB source remains unlisted, an unresolved keeper can
reach the unchecked index. This is a conditional static loading risk, not
evidence of runtime behavior.

## Identity, history, and disposition

The exact source scan found no competing WLD, MOB, OBJ, or QST prototype
declarations for gianthou’s numeric identities. The Pworm band and open
bands are still an unresolved manifest ownership conflict. Followed
gianthou histories remain on the same stem through the 2009 area-tree move
and later same-stem edits; no alternate-stem rename or explicit whole-zone
replacement was found. The unique ZON title and historical continuity do not
resolve who owns the #426–#428 range, the two #274141 definitions, or the
editor/deployment copies.

Keep candidate row 49 **RETAIN IN PLACE**. Preserve all six source files,
the 271-room graph, both source-only exits, the Gimluk/dwarf QST branches,
the complete typed reset set, and all four shop records. Resolve Pworm/open
band ownership, the external #274141 identity and travel intent, story and
quest ownership, the large nested-object reset set, shop fallback behavior,
editor state, and persisted IDs before proposing activation, move, rename,
archive, or merge. See the [24 builder questions](GENIANTHOU_BUILDER_QUESTIONS.md),
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-486-gimluks-cottage-map-and-boundaries),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-486-gimluks-cottage-identities-and-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-486-gimluks-cottage-retained-pending-range-and-shop-ownership),
[candidate row 49](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L49), and
[coverage row 399](ZONE_COVERAGE_REGISTER.csv#L400). No source-area files,
code, tests, builds, runtime, server, database, or containers were changed
or used.


## Checkpoint 898: complete typed cross-source closure

The audited source snapshot remains unchanged from `72ba3deff3bb30bde880018cdebe7c6db675b697` through HEAD `c500ac51b8cd26540943257e7ca4e9807263ef74`; there are no `areas/` or `src/` changes. A role-aware exact-ID scan found no other WLD, MOB, or OBJ definition with a `gianthou` VNUM. No external ZON room, mobile, or object role matched any of the 271 rooms, 55 mobiles, or 122 objects; a raw scan also found no unclassified external ZON line with an exact candidate identity.

The full WLD scan found no outside direction targeting a `gianthou` room. No outside type-25 portal destination or type-29 switch room operand targets the map, and the candidate OBJ has no type-25 or type-29 destination of its own. Its one D3 remains #42870 to ambiguous unlisted #274141; this is still the only authored cross-zone WLD boundary. No external C/C++ source line contains an exact candidate room, mobile, or object VNUM.

Across the 265 repository QST files (excluding the local `gianthou.qst`) and all 232 story sidecars, no external exact candidate VNUM was found. All 708 external parser-shaped N-format SHP records were checked by stock-object, keeper, and room roles and returned no match; a raw exact-number scan also found no candidate-band value in any outside SHP file. These negative results do not qualify the conditional all-shard fallback or resolve the four source shops' keeper indices.

No new quest, story, route, same-type identity, or replacement evidence was found. The CP486 story, graph counts, Pworm/open-band question, #274141 identity and return-path uncertainty, reset set, and shop fallback conditions remain unchanged. Keep candidate row 49 **RETAIN IN PLACE** until area-range, map, quest/story, reset, shop, tooling/editor, and persisted-ID owners resolve them. See the [24 builder questions](GENIANTHOU_BUILDER_QUESTIONS.md).
