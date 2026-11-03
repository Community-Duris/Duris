# Shady Grove: world, history, and dependency supplement

**Checkpoint 98.** This repository-wide pass supplements the [source-comprehensive
Shady Grove dossier](../zone-stories/SHADY_GROVE.md). It reconciles the active
and inactive WLD graph, typed references, tracked source history, and attic
candidate records. No native gameplay, area source, database state, or zone
location was changed.

## Active connectivity and reachability limits

The un-commented [`areas/AREA`](../../../areas/AREA#L351) manifest loads
Shady Grove. The current all-active-WLD scan parsed all 351 contributors and
253,262 room VNUM records. Shady Grove has 258 rooms and 561 authored exits;
all 561 targets resolve in the active room set, with no parse issues. Of those
exits, 558 target Shady Grove and three leave for Surface. The undirected
authored graph has one weak component of 257 rooms plus isolated room #97758.
The existing dossier maps #97758 as the pet-shop stock room selected after
room #97757 by the `pet_shops` handler; that code relationship explains the
separate WLD component, but does not qualify live pet-shop access or stock
behavior. The coverage census records three fall-chance markers and no current
fields. These static counts do not prove directed reachability, door state,
fall safety, or a played return route.

Three active Shady exits pair with reciprocal Surface exits:

| Shady exit | Reciprocal Surface exit |
| --- | --- |
| #97648 east to #583895 | #583895 west to #97648 |
| #97717 south to #584294 | #584294 north to #97717 |
| #97755 west to #583893 | #583893 east to #97755 |

The rows are recorded at
[`WORLD_CROSS_AREA_EXIT_REGISTER.csv`](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1163),
[#L1164](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1164),
[#L1165](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1165), and the reciprocal Surface
rows [#L1528](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1528),
[#L1529](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1529), and
[#L1532](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1532). Two additional active
Heavens exits arrive at the inn, Shady #97663: room #26 north and room #41
west ([first](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L6),
[second](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L16)). Neither has a reciprocal
WLD exit from Shady. Treat these as authored one-way arrivals in the static
graph; the separate compiled portals and special travel documented in the
main dossier do not prove an ordinary or safe return.

## Inactive WLD sources and map identity

The current scan parsed all 95 inactive WLD files: 527,791 rooms and 2,080,773
exits, with no parse issues and no source-room VNUM overlap with Shady. It
found three non-overlapping source-only approaches, all in `surf.wld`:

| Source-only exit | Source evidence |
| --- | --- |
| #282224 east to Shady #97755 | [`surf.wld` exit](../../../areas/wld/surf.wld#L1725380) |
| #282226 west to Shady #97648 | [`surf.wld` exit](../../../areas/wld/surf.wld#L1725430) |
| #282525 north to Shady #97717 | [`surf.wld` exit](../../../areas/wld/surf.wld#L1731697) |

These add three curated source-only records: the reviewed Surf subset rises
from 125 to 128, and the global curated source-only total rises from 1,509 to
1,512. They remain inactive-source declarations, not active ingress.

Same-numbered rooms in `surface2011.wld` are different source records. For
example, its #282525 exits north to #282225 rather than Shady
([room](../../../areas/wld/surface2011.wld#L1733026)); the corresponding
`surf.wld` room exits north to #97717. Do not treat the matching VNUM as room
identity or replacement evidence. The existing unresolved register also
records `mobs_surf.wld` room #31404 down to #282525
([row](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L188),
[source](../../../areas/wld/mobs_surf.wld#L60)). That target is present in both
inactive `surf.wld` and `surface2011.wld` with different exits, so this is an
unresolved source-map junction, not another direct approach into the active
zone.

## Typed references and outside consumers

The local source set has 258 rooms, 106 mobiles, and 90 objects. Exact typed
scans found these outside references:

| Source | Reference into Shady Grove | Classification |
| --- | --- | --- |
| Active Alatorin QST | `G I 97549` at [line 5347](../../../areas/qst/alatorin.qst#L5347) | Active item input using Shady's orcish-forged sword. |
| Active Alatorin and Caertannad ZON | Six `M` reset-role rows: two Alatorin rows for #97561/#97563 ([first](../../../areas/zon/alatorin.zon#L2802), [second](../../../areas/zon/alatorin.zon#L3451)) and four Caertannad rows for #97550 ([#L1376](../../../areas/zon/caertannad.zon#L1376), [#L1377](../../../areas/zon/caertannad.zon#L1377), [#L1378](../../../areas/zon/caertannad.zon#L1378), [#L1379](../../../areas/zon/caertannad.zon#L1379)). | Active external mobile resets. Their authored records do not establish a successfully loaded or live reset outcome. |
| Active `end.shp` | Keeper #101 has `in_room` #97685 ([shop record](../../../areas/shp/end.shp#L95), [location field](../../../areas/shp/end.shp#L126)). | Active cross-area shop-location reference; this is not a stock-product reference, and operation was not played. |
| Active Alatorin story sidecar | One structured `item_vnums` reference to #97549 at [line 2738](../../../areas/story/alatorin.story.json#L2738). | Typed item reference. All 231 external sidecars parsed without errors. The six Alatorin numeric-token matches include this item and story/completion identifiers; they add no other typed dependency. |

The exact-number scan found 24 C/C++ source lines, already context-classified
in the main dossier across ambient assignments, town/service registrations,
justice, room tables, and special travel. Numeric matches outside those typed
contexts do not add WLD routes. The scan found no external type-25 portal
target to a Shady room among 872 active and source-only declarations, and no
external object-prototype definition duplicating a local object VNUM.

One unregistered source component also depends on a Shady object:

| Source-only component | Evidence and disposition |
| --- | --- |
| `nexus56` | Shop #83032 stocks object #97521 at [`nexus56.shp`](../../../areas/shp/nexus56.shp#L249), and its ZON has a matching `G` reset at [`nexus56.zon`](../../../areas/zon/nexus56.zon#L1324). `nexus56` is absent from `areas/AREA`. This checkout has no `areas/world.shp`; `open_shop_stream()` can fall back to globbing SHP files ([loader](../../../src/economy/shop.c#L3267)). Preserve the exact rows in the [Nexus56 plan](ATTIC_MIGRATION_PLAN.md#L40); possible parser exposure does not establish an operating shop or reset. |

The local Shady ZON and SHP also use imported prototypes. The existing dossier
maps 61 complete external object definitions, three missing bakery stock
prototypes (#6070, #6109, and #6110), and one imported mobile #101. Local QST
item commands and typed local `item_vnums` do not point outside the local
object set. Retain those distinctions in any later dependency closure.

## Tracked history and duplicate-source boundary

The normalized active `.ZON` title `Shady Grove` is unique among current
non-empty active zone titles. `git log --follow` first records `shady.wld` at
the 2009 path addition and shows later WLD edits through 2013; it records no
rename from a distinct zone stem. The unregistered `shady2.mob` first appears
as a separate addition in commit `642da5400` (2009-08-12) and has no later
tracked edits. It defines 90 mobile VNUMs, #97500–#97589, all also defined in
active `shady.mob`; Shady has 16 additional mobile VNUMs, #97590–#97605. The
shared records are not line-identical, and five alias lists differ, including
the ambient-role prototypes #97540, #97545, #97562, #97580, and #97582
([sample shared records](../../../areas/mob/shady2.mob#L590),
[active counterpart](../../../areas/mob/shady.mob#L590)). The duplicate
numeric range is evidence of a source collision, not proof that `shady2` is a
canonical predecessor or safe replacement. Its purpose and owner remain
unresolved; the [global history register](DUPLICATE_AND_REPLACED_ZONES.md)
records this boundary.

### Builder questions and recommended resolution

- **Are the three `surf.wld` approaches intended map connectors?** Keep all
  three exact records in the Surf candidate plan and retain the parallel map
  sources until their provenance and intended route ownership are resolved.
  Do not promote these inactive edges to active ingress.
- **What is the intended role of `shady2.mob`?** Preserve it separately from
  `shady.mob` until its original purpose, editor/tooling consumers, and owner
  are confirmed. Do not merge or archive it on VNUM overlap alone.
- **Are the Heavens arrivals and isolated pet-shop stock room behaving as
  intended?** Keep the two arrivals, three paired Surface exits, and room
  #97758 code role distinct. Qualify directed travel, doors, fall behavior,
  service access, and safe return before making gameplay or migration claims.

Shady Grove remains **DO NOT MOVE**. Preserve its active WLD boundaries and
typed consumers, the three already-counted source-only approach records, the
Nexus56 shop/reset references, and the `shady2` source collision pending the
separate owner and route decisions above.

## Checkpoint 356: builder-question coverage reconciliation

Coverage line 268 is `area:shady`. The source-comprehensive dossier and checkpoint 98 supplement already contain three explicit, source-backed builder questions with recommendations. This checkpoint verifies that review work and closes the coverage field; it does not treat the underlying decisions as answered.

1. **Are the three inactive `surf.wld` approaches intended map connectors?** Keep the exact records in the Surf candidate plan and retain their source identity until provenance and route ownership are confirmed. Do not count them as active ingress.
2. **What is the intended role and owner of `shady2.mob`?** Preserve it separately from `shady.mob` until its purpose, editor/tooling consumers, and owner are confirmed. The shared 90 mobile VNUMs and five differing alias lists do not establish a safe merge or replacement.
3. **Are the Heavens arrivals and isolated pet-shop stock room intended?** Retain the two one-way Heavens arrivals, three reciprocal Surface pairs, and room `#97758`'s code role as distinct source facts. Resolve door state, fall behavior, service access, route behavior, and safe return before changing gameplay or making a migration decision.

The review is complete because each question has an evidence-backed conservative recommendation. The builder answers remain open. Shady Grove remains in place; no candidate row or gameplay source changes are introduced. See the [source dossier](../zone-stories/SHADY_GROVE.md), [checkpoint 98 findings](SHADY_GROVE_AUDIT_REVIEW.md#builder-questions-and-recommended-resolution), and [coverage row 268](ZONE_COVERAGE_REGISTER.csv#L268).

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has four mobile reset rows for #97550, whose active owner is Shady; candidate Shady2 has a separate same-number prototype. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).

## Checkpoint 934: #97549 is an object and mobile VNUM

Alatorin's active QST and Brino story contract use #97549 as an item; the
active Shady OBJ prototype is the orcish-forged sword. The same number also
defines a baby-orc mobile in active `shady.mob` and the separate unlisted
`shady2.mob`. The item-typed contract resolves to the OBJ record and does
not select either MOB by number alone. The four item IDs in Shady's local
exchanges (#97514, #97570, #97572, and #97582) likewise resolve to active
Shady OBJ prototypes, though each overlaps the candidate mobile range. See
the [shady2 typed-reference audit](SHADY2_AUDIT_REVIEW.md#checkpoint-934-quest-and-story-item-ids-remain-object-typed),
[Alatorin QST item](../../../areas/qst/alatorin.qst#L5347),
[Brino story contract](../../../areas/story/alatorin.story.json#L2735),
[Shady sword OBJ](../../../areas/obj/shady.obj#L477),
[active baby-orc MOB](../../../areas/mob/shady.mob#L725), and
[candidate baby-orc MOB](../../../areas/mob/shady2.mob#L725).
