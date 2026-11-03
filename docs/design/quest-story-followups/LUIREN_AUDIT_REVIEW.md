# Luiren QST/SHP inactive-source audit

Canonical key: component:luiren. Physical coverage row 412 / CSV line 413;
candidate dependency row 61 / CSV line 62. Reviewed October 11, 2026,
Checkpoint 499, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source,
numeric-identity, quest-input, shop-format, and bounded-history review. It
does not establish live loading, gameplay, actual deployed shop input, saved
quest ownership, or permission to activate, move, or archive either file.

## Source inventory and quest input

The unlisted component has only [QST](../../../areas/qst/luiren.qst#L1) and
[SHP](../../../areas/shp/luiren.shp#L1) sources. It has no WLD, MOB, OBJ,
ZON, AREA annotation, or story sidecar. Neither [areas/AREA](../../../areas/AREA)
nor [areas/qst/AREA](../../../areas/qst/AREA#L1) lists luiren. There is no
room range, candidate map, authored WLD exit, or candidate graph to count.
The word “Luiren” is a source stem here; the files contain no ZON title that
establishes a canonical zone name or area boundary.

The old-format QST file contains 44 M dialogue blocks with giver headers in
four ranges: #16000–#16022, #16031–#16038, #16049–#16055, and #16058–#16063.
Each of these 44 numbers also identifies an active Pine Hollow mobile
prototype. The source has no native Q or QA contract records. This is a
source-only numeric collision, not evidence that 44 Luiren givers currently
load, that these dialogue blocks are Pine Hollow dialogue, or that Luiren
has a separate WLD map.

The standard QST builder takes its input from its AREA file
([make_qst.c](../../../areas/src/qst/make_qst.c#L6)); that list excludes
luiren. Keep its dialogue out of the active input until the source owner
identifies each giver identity and its intended quest-system format.

## Old-format shop data and Pine Hollow overlaps

The unlisted [luiren.shp](../../../areas/shp/luiren.shp#L1) contains eight
records marked O, using an older shop format. Across those records, 16 listed
product VNUMs overlap active Pine Hollow objects, all eight keeper VNUMs
overlap active Pine Hollow mobiles, and two shop-room fields overlap active
Pine Hollow rooms:

| Luiren shop | Pine Hollow product VNUMs | Matching keeper / room VNUMs | Source |
| --- | --- | --- | --- |
| #16030 | #16024 | keeper #16030; room #16008 | [line 1](../../../areas/shp/luiren.shp#L1) |
| #16038 | #16034 | keeper #16038 | [line 32](../../../areas/shp/luiren.shp#L32) |
| #16039 | #16033 | keeper #16039 | [line 63](../../../areas/shp/luiren.shp#L63) |
| #16040 | #16027, #16028, #16029 | keeper #16040 | [line 94](../../../areas/shp/luiren.shp#L94) |
| #16041 | #16004, #16005, #16022, #16025 | keeper #16041 | [line 125](../../../areas/shp/luiren.shp#L125) |
| #16042 | #16014, #16011 | keeper #16042 | [line 156](../../../areas/shp/luiren.shp#L156) |
| #16051 | #16023, #16025, #16027 | keeper #16051; room #16058 | [line 189](../../../areas/shp/luiren.shp#L189) |
| #16065 | #16024 | keeper #16065 | [line 220](../../../areas/shp/luiren.shp#L220) |

These are raw numeric matches, not confirmed live Luiren shops or purchases.
The standard [shop compiler](../../../areas/src/shp/make_shp.c#L6) reads its
AREA list and excludes luiren. This checkout has no areas/world.shp. In
non-mini mode, [open_shop_stream](../../../src/economy/shop.c#L3267) falls
back to globbing areas/shp/*.shp when the generated aggregate cannot be
opened, which can include luiren.shp. The runtime shop loader accepts the N
format and sends an O marker to its old-shop fatal-error path at
[shop.c:3361](../../../src/economy/shop.c#L3361). If that fallback runs with
this file present, the old records are a conditional loader compatibility
hazard. No server was started and deployed shop input was not inspected.

The earlier [Pine Hollow review](PINE_HOLLOW_AUDIT_REVIEW.md#luiren-candidate-identity-and-old-shop-data)
records the same 44 giver collisions and shop table. Pine Hollow remains an
active, separately titled area with its own WLD, MOB, and OBJ identities. The
Luiren numbers do not prove that it replaced Pine Hollow, connects to it, or
owns any of its active prototypes.

## History, narrative, and disposition

The 44 dialogue blocks provide authored conversation text, so the QST source
is narrative-bearing even without a map or story sidecar. The bounded file
histories for both files reach the 2009 area-tree move and a 2013 file-mode
change; the 2013 “stuff” commit changes file permissions but not these file
contents. No direct rename, retirement, or whole-area replacement decision
appears in those histories.

**Disposition: RETAIN IN PLACE.** Keep both files at their current paths and
out of both area lists. Preserve all 44 giver headers and all eight O-format
shops as identity-ambiguous source data. Resolve the QST format and giver
owners, every Pine Hollow mobile/product/room collision, shop input and
fallback behavior, editor/export/deployment copies, and persisted quest or
shop ownership before any loader, conversion, path, or archive proposal. Do
not infer a Luiren map or Pine Hollow replacement from shared VNUMs. See the
[20 builder questions](LUIREN_BUILDER_QUESTIONS.md),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-499-luiren-giver-and-legacy-shop-vnum-overlaps),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-499-luiren-retained-pending-qst-and-shop-identity),
[candidate dependency row 61](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L62), and
[coverage row 412](ZONE_COVERAGE_REGISTER.csv#L413).
