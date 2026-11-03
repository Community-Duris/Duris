# Barrow of the Quiosho (Priority 217) repository-wide audit review

This supplement adds active and inactive WLD reconciliation, typed cross-area references, title/history review, and migration disposition to the [comprehensive Barrow source dossier](../zone-stories/BARROW_OF_THE_QUIOSHO.md). These static records do not prove installed availability, access, or player qualification.

## Active map and authored reach

The Barrow of the Quiosho (`elftomb`) has 207 rooms (#20600–#20806) and 667/667 loader-resolving exits: 664 local and three outbound. Its weak components contain 206 rooms and one room. Raw directed traversal from root #20600 reaches 206/207 rooms, and 206/207 have paths back to the root; holding room #20804 is isolated from the ordinary local exit graph. The three active boundaries reciprocate with Surface: #20600 west ↔ #609100 east, #20611 east ↔ #612309 west, and #20615 west ↔ #613104 east. See the [root](../../../areas/wld/elftomb.wld#L1), [second boundary](../../../areas/wld/elftomb.wld#L196), [third boundary](../../../areas/wld/elftomb.wld#L275), and [isolated room #20804](../../../areas/wld/elftomb.wld#L4692); local exit-register [rows 339–341](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L339) and return rows [1595](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1595), [1604](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1604), and [1606](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1606); pair-summary [rows 131](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L131) and [496](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L496).

## Inactive WLD sweep

All 95 inactive WLD files parsed: 527,791 rooms and 2,080,773 exits, with no parse errors or room-VNUM overlap against the active Barrow. Four non-overlapping source-only approaches enter it from `surf.wld`:

| Source-only record | Active target | Evidence |
| --- | --- | --- |
| Room #275979 east (`D1`) | #20600 | [Source room](../../../areas/wld/surf.wld#L1594343); [exit target](../../../areas/wld/surf.wld#L1594354) |
| Room #275980 east (`D1`) | #20600 | [Source room](../../../areas/wld/surf.wld#L1594364); [exit target](../../../areas/wld/surf.wld#L1594375) |
| Room #276595 west (`D3`) | #20611 | [Source room](../../../areas/wld/surf.wld#L1607271); [exit target](../../../areas/wld/surf.wld#L1607290) |
| Room #277493 east (`D1`) | #20615 | [Source room](../../../areas/wld/surf.wld#L1626121); [exit target](../../../areas/wld/surf.wld#L1626132) |

These declarations are source-only, not active ingress. They raise curated totals from 3,426 to 3,430 overall and from 193 to 197 for Surf; `ud_map2` remains 15. Preserve them in the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); no candidate row is added.

## Typed item and cross-area dependencies

The local warrior Q2 contract accepts head #20619 and returns the Blood-eye amulet #20615 without disappearance; the complete local contract and story sidecar remain in the [source dossier](../zone-stories/BARROW_OF_THE_QUIOSHO.md), and the native QST is at [elftomb.qst lines 9–10](../../../areas/qst/elftomb.qst#L9). The relevant local prototypes include the glass eye #20604 at [elftomb.obj line 55](../../../areas/obj/elftomb.obj#L55), the amulet #20615 at [line 175](../../../areas/obj/elftomb.obj#L175), and the bloody head #20619 at [line 223](../../../areas/obj/elftomb.obj#L223).

Active Caves of Mt. Skelenak has an independent blind-monk contract, `G I 20604`, at [caves_skelenak.qst line 177](../../../areas/qst/caves_skelenak.qst#L177). Its structured sidecar requests `item_vnums` #20604 and identifies the skull pile #20603 as the declared source at [caves_skelenak.story.json lines 343–351](../../../areas/story/caves_skelenak.story.json#L343). The Caves journal owns this foreign exchange; local recovery does not complete it.

The unregistered `bgtk.shp` source lists local Barrow objects as shop products: shop #20607 lists #20602/#20603 at [lines 1–4](../../../areas/shp/bgtk.shp#L1), and shop #20615 lists #20611/#20612/#20613 at [lines 36–40](../../../areas/shp/bgtk.shp#L36). Although bgtk is not an areas/AREA contributor, in this checkout the non-mini shop loader falls back to all areas/shp/*.shp files because areas/world.shp is absent; these records therefore enter default full-mode shop initialization. The source does not establish usable shops: two keeper mobile lookups are missing, the existing keeper is reset in a different room, and six products have no current object definitions. See the [BGTK audit](BGTK_AUDIT_REVIEW.md#default-full-mode-loader-path-and-risk). Preserve the typed dependency with the existing [BGTK candidate row 16](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L16).

## Checkpoint 453: BGTK shop loader reconciliation

The earlier typed scan found the unregistered bgtk.shp products but did not
establish whether the source file entered the shop loader. This checkout has
no areas/world.shp aggregate. In non-mini mode, open_shop_stream falls back
to concatenating every areas/shp/*.shp file, and the normal boot sequence
calls boot_the_shops followed by assign_the_shopkeepers
([loader](../../../src/economy/shop.c#L3267),
[boot](../../../src/specs/specs.assign.c#L1258)). Thus the three BGTK SHP
records are included in that source path even though bgtk is not in
areas/AREA.

The records refer to active Barrow products #20602/#20603 and #20611–#20613,
but also to six missing objects #20631–#20636. Keeper #20607 exists in the
active Barrow mobile table but its zone reset is at #20710 rather than the
configured shop room #20630. Keepers #20615 and #20647 are absent from the
current mobile source set. real_mobile returns -1 when absent, and
assign_the_shopkeepers indexes mob_index[keeper] without a negative-index
check. This is a static full-mode boot-safety risk, not an observed crash or
runtime qualification. See the
[BGTK audit](BGTK_AUDIT_REVIEW.md#default-full-mode-loader-path-and-risk)
and [BGTK candidate row 16](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L16).

The all-source typed scan found no external reset-role reference to a Barrow room, mobile, or object; no other external QST item command; no local SHP file; no external duplicate mobile/object prototype; no external type-25 target into a Barrow room; and no exact compiled C/C++ VNUM binding. All 231 external story sidecars parsed without errors; the only structured external prototype match was Caves’ #20604 request (with #20603 in its descriptive source hint). The bounded object and portal scan covered 442 OBJ files, 21,682 object records, and 872 external type-25 declarations. Dynamic, editor/tooling, installed, and persisted references remain outside this closure.

## Title and bounded history

The normalized title “Barrow of the Quiosho” is unique among 442 scanned `.zon` files; see the [current title source](../../../areas/zon/elftomb.zon#L2). Followed `elftomb.wld` history stays on the same stem through the 2009 areas-tree move, 2011 edit, and 2013 edit. `elftomb.zon` history includes the 2015 quest-item-flag pass. This bounded path review found no alternate-stem rename, distinct predecessor, or whole-zone replacement. The four unregistered Surf approaches are separate source-only records and do not establish a replacement map.

## Migration disposition

Keep active `elftomb` **DO NOT MOVE**. Preserve its three reciprocal Surface boundaries, four source-only Surf approaches, Caves’ exact eye dependency, and the unregistered BGTK product references pending their owners’ separate source decisions. Keep `surf` and `surface2011` **RETAIN IN PLACE** with candidate row 97 and `bgtk` with candidate row 16. See [Barrow coverage row 87](ZONE_COVERAGE_REGISTER.csv#L87), [Caves coverage row 47](ZONE_COVERAGE_REGISTER.csv#L47), and [BGTK coverage row 367](ZONE_COVERAGE_REGISTER.csv#L367).
