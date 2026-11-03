# Vargan II repository-wide source audit

This supplement reconciles Vargan II's active map and boundary, inactive-WLD approaches, typed source references, compiled numeric matches, title, and bounded same-stem history with the [source-comprehensive dossier](../zone-stories/V2.md). It uses the pinned native source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed by this audit.

## Active map and Vargan boundary

The active [Vargan II WLD](../../../areas/wld/v2.wld#L1) contains 119 rooms (#7800–#7918) and 249 authored exits. All resolve: 248 are local and one crosses to active Vargan. The local map is one weak component. With every authored edge available, directed traversal from #7800 reaches and returns from all 119 rooms. A conservative graph pass that excludes the two raw flag-8 bits reaches and returns 118/119; the [V2 dossier](../zone-stories/V2.md) records that the legacy raw9 values on #7887/#7888 are masked by `setup_dir` to ordinary door state1 and local D0 resets leave them open on fresh load. The raw labels alone do not establish a blocked route or player admission.

The only active WLD boundary is reciprocal: Vargan II #7800 up ↔ Vargan #2182 down ([pair rows 762 and 770](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L762); [Vargan exit](../../../areas/wld/vargan.wld#L1887)). There are no negative or unresolved exits, authored falls, or currents. The source graph and reset declarations do not qualify current door state, actor conditions, hazards, or safe return.

## Inactive-map sweep

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no parser issues, source-room VNUM overlap, or exit target into Vargan II. It adds no source-only approach, so the curated source-only total remains 3,227 overall; the reviewed subsets remain two for `ud_map0`, 14 for `ud_map2`, and 159 for Surf. There is no new map record to preserve for Vargan II and no migration clearance follows from this scan.

## Typed references and broad numeric matches

Vargan II is listed in the active [world manifest](../../../areas/AREA#L44) and [QST index](../../../areas/qst/AREA#L40). Exact scans found no external QST item command, SHP product, ZON reset-role reference, or structured story `item_vnums` entry into a local room/mobile/object prototype. The local QST/reset declarations do not import an external item, and `v2` has no local SHP source. The external mobile/object census found no duplicate prototype definitions. Across the 442-object-file census, none of the 872 external type-25 declarations targets a Vargan II room.

All 231 external story sidecars parsed without errors and produced no typed VNUM reference. Three raw number matches in active `wh.story.json` occur only inside story-ID strings: #7876 at [line 900](../../../areas/story/wh.story.json#L900), #7886 at [line 1000](../../../areas/story/wh.story.json#L1000), and #7815 at [line 1023](../../../areas/story/wh.story.json#L1023). They are not item or room-reference fields. The broad C/C++ number scan found only the digit substring #7863 inside the floating-point lookup value `4.7863` in [`memorize.c`](../../../src/classes/memorize.c#L83), not a Vargan II destination. No direct compiled numeric route to a local Vargan II room was found.

## Title, history, and disposition

The normalized title `Vargan II` occurs once among all 442 `.zon` files and active titles, in [`v2.zon`](../../../areas/zon/v2.zon#L2). Followed `v2.wld` and `v2.zon` histories reach the 2009 area-tree move, the 2012 format-field edit, and 2013 zone edits; no alternate-stem rename, distinct predecessor, or whole-zone replacement was established. Active `vargan` is a separate source identity; their reciprocal boundary does not imply that either replaced the other.

Keep active `v2` **DO NOT MOVE**. The inactive-map scan found no source-only approach, and the typed-reference scan found no outside prototype dependency requiring a new attic record. The broader story/access findings remain in the source-comprehensive dossier. This audit is static source review, not gameplay qualification.

The source-mapping queue is now 188/220 complete, with 32 pending. Priority 189, Phantasmagoric Caverns (`valdrak`), is next.

Validation: Checkpoint 148 documentation validation checked 118 changed/added Markdown files; all 4,754 local link targets and 2,160 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.
