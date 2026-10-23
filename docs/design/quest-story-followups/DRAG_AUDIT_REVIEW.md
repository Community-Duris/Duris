# `drag` empty QST/SHP source audit

Canonical key: `component:drag`. Physical coverage row 384; candidate dependency row 34. Reviewed October 11, 2026, Checkpoint 471. This is a static file, manifest, and bounded-history review; it does not establish why the placeholders remain or authorize removing them.

## Source scope and history

The only current `drag` files are [areas/qst/drag.qst](../../../areas/qst/drag.qst) and [areas/shp/drag.shp](../../../areas/shp/drag.shp). Both are zero bytes. There is no same-stem WLD, MOB, OBJ, ZON, map JSON, or story sidecar. `drag` is absent from the full [areas/AREA](../../../areas/AREA) and [areas/qst/AREA](../../../areas/qst/AREA) manifests, and has no raw area annotation or ZON title. Current `RANDOM_AREA` is empty.

Followed histories for both paths remain contentless in the tracked snapshots. Each reaches the 2005 initial placeholder path `Players/Declined/...`, a 2006 path named `Players/justice_list`, and the 2009 move into `areas/qst/drag.qst` or `areas/shp/drag.shp`; a 2013 same-stem `stuff` commit has a zero-line file diff. This records path history only. It does not explain the placeholder names, provide older content, identify a map, or prove an alternate-stem predecessor or replacement.

## Narrative, connectivity, and typed references

Neither file contains a QST key, contract, SHP record, shop header, keeper, room, object product, message, or numeric VNUM. No map or graph can be reconstructed from these files, and the coverage register records zero active inbound WLD exits. With no current candidate VNUMs or record keys, the tracked sources provide no exact typed dependency to resolve. External editor state, archived exports, scripts keyed only by filename, persisted quest/shop data, and runtime configuration remain outside this evidence.

The current checkout lacks `areas/world.shp`; the non-mini [`open_shop_stream`](../../../src/economy/shop.c#L3267) fallback can open and concatenate `areas/shp/*.shp`, but the zero-byte `drag.shp` contributes no shop record or typed field. This is a conditional loader-path observation only. It does not establish that the empty file is needed or that a shop is available. The QST file is also empty and is not listed in `areas/qst/AREA`.

## Retention

Keep both zero-byte files RETAIN IN PLACE until their owner confirms whether they are intentional placeholders, legacy path markers, or abandoned files, and whether an editor, build process, or persisted record depends on either path. Do not infer that empty means safe to delete. See the [11 builder questions](DRAG_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions), [candidate row 34](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L34), and [coverage row 385](ZONE_COVERAGE_REGISTER.csv#L385). No area/source files were changed, and no test, build, runtime, server, database, or container work was performed.
