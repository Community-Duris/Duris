# BGTK SHP source audit

Canonical key: component:bgtk. Physical coverage row 366; attic candidate
row 16. The only source file is areas/shp/bgtk.shp. There is no bgtk WLD,
MOB, OBJ, ZON, QST, map JSON, or story sidecar, and no bgtk entry in
areas/AREA. The SHP contains generic shop messages and stock records, not
area narrative; the Barrow of the Quiosho dossier remains the story source
for the active rooms and objects referenced here.

## Three shop records and their typed references

| SHP record header | Keeper VNUM and current mobile source | Configured room | Products | Current object definitions |
| --- | --- | ---: | --- | --- |
| #20607 | #20607, active elftomb old orc warrior | #20630 | #20602, #20603 | Both defined by active elftomb OBJ |
| #20615 | #20615, no current mobile prototype; this number is the active elftomb Blood-eye amulet object | #20641 | #20611, #20612, #20613 | All three defined by active elftomb OBJ |
| #20647 | #20647, no current mobile prototype; this number is an active elftomb room | #20657 | #20631–#20636 | No current object prototype found for any of the six |

The shop record headers and keeper fields match within each record. Their
fields and products are at [record #20607](../../../areas/shp/bgtk.shp#L1),
[record #20615](../../../areas/shp/bgtk.shp#L36), and
[record #20647](../../../areas/shp/bgtk.shp#L88). The first two shops list
five object prototypes present in active areas/obj/elftomb.obj:
#20602 bloody piece of meat, #20603 pile of skulls, #20611 metal-studded
boots, #20612 spiked gloves, and #20613 rusty iron helm. The third shop's
six product VNUMs, #20631–#20636, have no current object definitions.
In total, five of eleven listed products resolve to current Barrow object
records and six do not.

All three configured rooms are active Barrow WLD rooms: #20630 is “On a
Small Lake,” #20641 is “By the Shore of a Small Lake,” and #20657 is “On an
Ancient Battlefield” ([#20630](../../../areas/wld/elftomb.wld#L648),
[#20641](../../../areas/wld/elftomb.wld#L915),
[#20657](../../../areas/wld/elftomb.wld#L1290)). The one mobile prototype
that resolves, #20607, is reset by active elftomb.zon at #20710, not at
the shop's configured room #20630
([mobile](../../../areas/mob/elftomb.mob#L107),
[reset](../../../areas/zon/elftomb.zon#L168)). Active #20615 is an object
record, not a mobile. Active #20647 is a room number, not a mobile. These
typed-number collisions do not supply the two missing keeper prototypes.

## Default full-mode loader path and risk

In this checkout areas/world.shp is absent. The configured SHOP_FILE is
areas/world.shp ([definition](../../../src/world/db.h#L22)). Outside mini
mode, open_shop_stream first tries that aggregate and then concatenates
areas/shp/*.shp, skipping only end.shp before appending that terminator
([fallback](../../../src/economy/shop.c#L3267)). The fallback therefore
includes all three bgtk records even though bgtk is not in areas/AREA.
Mini mode uses areas_mini/world.shp and does not follow this fallback.

boot_the_shops converts the listed keeper VNUM to a mobile table index with
real_mobile, then the boot sequence calls assign_the_shopkeepers
([keeper conversion](../../../src/economy/shop.c#L3493),
[boot calls](../../../src/specs/specs.assign.c#L1258)). real_mobile returns
-1 when a VNUM is absent
([lookup](../../../src/world/db.c#L8587)). In the current full-area mobile
sources, #20615 and #20647 are absent, so those two records produce negative
keeper indices. assign_the_shopkeepers then indexes mob_index[keeper]
without a negative-index guard
([assignment](../../../src/economy/shop.c#L3602)). This is a source-level
full-mode boot-safety risk, not a confirmed crash: no server start or runtime
test was performed.

The #20607 record also attaches the shop special to an active Barrow mobile
whose ZON placement differs from the configured shop room. Whether that
record was intended for the active old orc, another historical mobile, or a
separate map is not established. The missing #20631–#20636 prototypes and
the two absent keepers make the BGTK SHP unsafe to treat as an inert attic
file or as a qualified set of working shops.

## History and disposition

No ZON title or area annotation exists for bgtk. Followed bgtk.shp history
reaches the 2009 areas-tree move and the 2013 same-stem “stuff” commit; it
provides no alternate-stem predecessor or whole-area replacement evidence.
This history does not resolve who owns these shop records or whether their
Barrow references are intentional.

Keep candidate row 16 RETAIN IN PLACE until the shop owners and runtime
maintainers resolve the implicit full-mode fallback, keeper VNUMs, configured
rooms, and six missing product prototypes. Do not remove or relocate this
SHP on the assumption that exclusion from areas/AREA makes it unused. The
audit records a static loading risk only; it does not establish live shop
availability or current server configuration. See the
[Barrow audit supplement](ELFTOMB_AUDIT_REVIEW.md#checkpoint-453-bgtk-shop-loader-reconciliation),
[builder questions](BGTK_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[candidate row 16](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L16),
[Barrow coverage row 87](ZONE_COVERAGE_REGISTER.csv#L87), and
[BGTK coverage row 366](ZONE_COVERAGE_REGISTER.csv#L367).
