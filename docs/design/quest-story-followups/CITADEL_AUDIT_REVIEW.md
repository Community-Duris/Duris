# The Citadel: repository-wide source audit supplement

This checkpoint supplements the [Citadel dossier](../zone-stories/THE_CITADEL.md). It checks active and unregistered WLD inputs, exact typed references, shared code data, normalized area titles, and tracked source history. These are static source findings; they do not establish live access or player history.

## Active WLD boundary and source-only scan

The Citadel has 475 resolving exits across 200 rooms in one local weak component. Its sole active cross-area boundary is reciprocal: Skulldrach #12796 north enters Citadel #13000, and Citadel #13000 south returns to #12796 ([Skulldrach row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L221), [Citadel return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L223)).

The refreshed scan parsed all 95 unregistered WLD files: 527,791 source rooms and 2,080,773 exits, with zero parse issues and no source-room VNUM overlap with Citadel. No unregistered WLD exit targets rooms #13000–#13199. Citadel adds no source-only route, so the curated source-only total remains 593 and the reviewed `surf.wld` subset remains 31.

## External typed references

Surface Keeps' QST has a `G C 13000` payment for its 13-platinum boar service; the matching structured sidecar records `give=C:13000` ([QST payment](../../../areas/qst/surfacekeeps.qst#L27), [sidecar contract](../../../areas/story/surfacekeeps.story.json#L272)). The `C` field is currency, not Citadel room/object #13000. All 231 structured sidecars parsed without error and contain no exact Citadel prototype reference. No external native QST consumes a Citadel prototype.

The only active foreign reset-role matches are for Citadel diamond #13036: Alatorin gives one at 15-percent chance and Caer Tannad resets one into room #78781 ([Alatorin reset](../../../areas/zon/alatorin.zon#L3341), [Caer Tannad reset](../../../areas/zon/caertannad.zon#L429), [diamond prototype](../../../areas/obj/citadel.obj#L557)). These are the already-described foreign diamond sources.

The only SHP stock hits are in unregistered `nexus56.shp`: its shop record #83038 lists Amberyl's book #13038 and the Book of Riddles #13039 ([shop record](../../../areas/shp/nexus56.shp#L493), [book stock](../../../areas/shp/nexus56.shp#L503), [riddle-book stock](../../../areas/shp/nexus56.shp#L504)). The same unregistered `nexus56.zon` has G resets for both prototypes ([source-only resets](../../../areas/zon/nexus56.zon#L1380); [book](../../../areas/obj/citadel.obj#L583), [riddle book](../../../areas/obj/citadel.obj#L656)). `nexus56` is absent from the active `areas/AREA` manifest, so these are source-only records. The shop loader can fall back to parsing all `areas/shp/*.shp` files when `areas/world.shp` is unavailable, but that does not show that this shop is active or operating ([fallback path](../../../src/economy/shop.c#L3274)).

A repository-wide code census found Citadel object #13053, “glyphs of power tattooed around one eye,” in both the shared standard and enhanceable anti-paladin CHAOS equipment profiles ([standard profile](../../../src/account/chaos_eq_data.h#L108), [enhanceable profile](../../../src/account/chaos_eq_data.h#L359), [profile selection](../../../src/account/chaos_eq_data.h#L593), [object](../../../areas/obj/citadel.obj#L911)). The shared CHAOS kit path reads the selected profile ([consumer](../../../src/account/nanny.c#L707)). A broad numeric hit in `unicode.c` is a Unicode range, not a prototype reference ([range](../../../src/net/unicode.c#L273)).

No external MOB/OBJ record defines a local Citadel mobile/object VNUM. The scan of 872 external type-25 portal declarations found no Citadel room destination. No other external QST, SHP stock, ZON reset-role, or exact structured story reference matched.

## Title and tracked history

The normalized title scan found one “The Citadel” title among 351 active area contributors. The active key is `citadel`, scope `*130-131`, with the matching title in its ZON ([area entry](../../../areas/AREA#L73), [zone title](../../../areas/zon/citadel.zon#L2)). The duplicate/replacement register has no separate retired or replacement Citadel area ([register](DUPLICATE_AND_REPLACED_ZONES.md)).

Tracked WLD/ZON history stays on the same `citadel` paths from the 2009 area-tree move (`642da5400`), through 2013 source edits (`71d731c5b`) and 2015 quest-item flag updates (`f18992b8f` for ZON). It records no separate predecessor or path replacement.

## Audit boundary

This review used repository source and static registers only. No tests, build, server/runtime, gameplay, database, migration, or operational script was run. The source-comprehensive queue is now **99/220 complete, 121 pending**; priority 100, The Elemental Groves (`element`), is next. Source mapping remains separate from played source custody, paid-service settlement, reset issuance, room access, and return-safety qualification.

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad resets Citadel object #13036 once. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).
