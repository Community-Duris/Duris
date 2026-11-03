# Tradeskills repository-wide audit supplement

This review reconciles the active Tradeskills graph, item catalog, reset/shop
imports, registered and unregistered QST material references, story-sidecar
consumers, maintained-code bindings, and source identity. The source snapshot
is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is
changed. See the [source dossier](../zone-stories/TRADESKILLS.md).

## Active graph and dynamic portal

The active [Tradeskills WLD](../../../areas/wld/tradeskills.wld#L1) has two
rooms, #400000 and #400999, with two resolving local exits. The open DOWN and
SOUTH records form one weak and directed component. There are no authored
outside WLD exits, and the all-95 inactive-WLD scan found no external approach
or room-number overlap. No declared type-25 object points to either room.
At Checkpoint 396, curated source-only WLD totals stood at 9,479 overall and 297 for Surf; the current cumulative totals are 11,993 overall and 415 from Surf. Tradeskills contributes no source-only approach.

The map has a separate conditional code route: the death path creates object
#400220, writes the victim's room VNUM to its destination field, and places the
portal in room #400000 ([creation](../../../src/combat/fight.c#L4829),
[destination and placement](../../../src/combat/fight.c#L4835)). The lifecycle
and travel code also recognizes the portal object
([lifecycle](../../../src/magic/spell_corpse_lifecycle.c#L1023),
[travel](../../../src/magic/spell_travel.c#L972)). This is a dynamic,
condition-dependent connection, not an authored WLD arrival edge or a claim
of safe entry or return.

## Typed reset, quest, shop, and sidecar references

Eight active foreign ZON sources contain 182 typed reset rows into the
Tradeskills mobile/object range: Alatorin 94, Earthp 46, Khildarak 7, Lornecro
9, Ravenloft2 16, Shady 2, Tharnadia 7, and Vehicles 1. These include the
Alatorin equipment/material containers and rewards, repeated Earth Plane
#400104 resets, Lornecro ore sources, and reset imports into Khildarak and
Tharnadia. The latter two reset mobiles #400000 and #400001, objects #400216,
and #400223/#400224 ([Khildarak rows](../../../areas/zon/khildarak.zon#L2186),
[Tharnadia rows](../../../areas/zon/tharnadia.zon#L634)). These are configured
reset references, not proof that a prototype is currently present or
obtainable.

The active QST manifest has 717 give/remove rows into the local item range:
Alatorin 673 rows across 220 unique VNUMs, Earthp 10 rows for #400104,
Surfacemini 18 rows for #400280, and Vehicles 16 rows for #400069, #400179,
#400233, and #400235 ([Alatorin](../../../areas/qst/alatorin.qst#L497),
[Earthp](../../../areas/qst/earthp.qst#L86),
[Surfacemini](../../../areas/qst/surfacemini.qst#L186),
[Vehicles](../../../areas/qst/vehicles.qst#L790)). Their QST `G`/`R` rows
establish item commands, not actual acceptance or settlement.

Two further QST files contain references but are absent from `areas/qst/AREA`:
`dalvik.qst` has 420 give-item rows spanning every #400000–#400209 salvage
material, and `ravenloft2.qst` has eight remove-item rows for #400233
([Dalvik](../../../areas/qst/dalvik.qst#L6),
[Ravenloft2](../../../areas/qst/ravenloft2.qst#L2236)). Treat those files as
unregistered source declarations, not installed QST consumers. The local
`tradeskills.qst` is not in the QST manifest and contains only the `#400000 S`
sentinel.

The local SHP file has one shop record for Lout (#400004), with products
#400217, #400232, and #400233 ([shop](../../../areas/shp/tradeskills.shp#L1),
[reset stock](../../../areas/zon/tradeskills.zon#L11)). Five active external
shop records also offer #400223 and #400224 through Khildarak, Shady, and
Tharnadia ([Khildarak](../../../areas/shp/khildarak.shp#L803),
[Shady](../../../areas/shp/shady.shp#L330),
[Tharnadia](../../../areas/shp/tharnadia.shp#L631)). The coverage inventory's
previous zero-shop count was incorrect; row 309 now records one local SHP
record, seller #400004.

All 232 story sidecars parsed. Four external sidecars contain typed
Tradeskills item references: Alatorin has 220 unique VNUMs from the base
materials and other local objects; Earthp references #400104; Ravenloft2
references #400233; and Surfacemini references #400280
([Alatorin](../../../areas/story/alatorin.story.json#L345),
[Earthp](../../../areas/story/earthp.story.json#L102),
[Ravenloft2](../../../areas/story/ravenloft2.story.json#L1564),
[Surfacemini](../../../areas/story/surfacemini.story.json#L872)). These are
typed story records and completion-key material, not source-provenance
receipts.

## Maintained-code bindings

The local catalog is a shared numeric API. `randomeq.c` generates 42 salvage
families × five quality tiers (#400000–#400209), `material_rarity.c` recognizes
that range, and `tradeskill.c` maps material families to their five-item
blocks ([random generation](../../../src/item/randomeq.c#L567),
[family mapping](../../../src/economy/tradeskill.c#L2332)). Mining and the
Alchemist's three-size conversions use #400260–#400283
([mining](../../../src/economy/mining.c#L190),
[conversion](../../../src/classes/salchemist.c#L1371)). Attribute enhancement
checks #400238–#400258; encrusting uses #400291–#400299 with pouch #400300
([enhancement](../../../src/item/enhance.c#L858),
[encrusting](../../../src/classes/salchemist.c#L937)).

Additional exact bindings include the #400210 recipe-scroll procedure,
#400237 Thanksgiving-wings procedure, #400229 ward procedure, #400000/
#400001 epic/PvP mobile stores, #400003 conjuration, #400005/#400232 turkey
creation, and #400006/#400217 elf/faerie-bag creation
([special assignments](../../../src/specs/specs.assign.c#L1386),
[spell/class paths](../../../src/classes/drannak.c#L836)). Fight, lifecycle,
and travel code also use #400220/#400230; other code uses #400218, #400222,
#400231, and #400234. SQL shop-trophy accounting excludes object VNUMs
#400000–#400201 ([flatfile exclusion](../../../src/sql/sql.c#L136)). Numeric
matches were classified by object/mobile/room or currency role; the 4,000,000
copper QST values are not item #400000.

## Title, history, duplicates, and disposition

After color-code and spacing normalization, “Tradeskills” occurs once across
all 442 current ZON titles. `tradeskills` is the active contributor in
[areas/AREA](../../../areas/AREA#L406); no matching entry appears in the QST
manifest, and there is no local story sidecar. Followed WLD, MOB, OBJ, and ZON
history stays on the current stem from the 2009 area-tree import through later
edits; no alternate-stem rename, predecessor, or whole-zone replacement was
established. The 2015 WLD history describes #400999 as a VNUM inspection
ceiling, consistent with its current room text.

No external active or inactive WLD room or MOB prototype duplicates a current
Tradeskills ID, and no other active or inactive OBJ source duplicates one.
The distinct exception is the unlisted `tradeskills.old.obj`: it defines 234
overlapping IDs, of which 218 whole records match the current file, 16 differ,
and 59 current definitions are absent from the old file. The old path's
followed history reaches the active stem, but the tracked files do not record
why the duplicate was kept. Old #400232 is an elven equipment bag while
current #400232 is turkey innards, a VNUM maintained code names explicitly;
old #400220 also retains a differing type-25 record. Do not merge or load that
stale catalog with the active OBJ. See the dedicated
[old-catalog audit](TRADESKILLS_OLD_AUDIT_REVIEW.md) and
[builder questions](TRADESKILLS_OLD_BUILDER_QUESTIONS.md).

Keep active Tradeskills **DO NOT MOVE**. The hub is referenced by the dynamic
portal path, active resets and shops, registered QST and sidecar item
consumers, and direct compiled bindings. Add no attic candidate or migration.
See [coverage row 309](ZONE_COVERAGE_REGISTER.csv#L309),
[connectivity](WORLD_CONNECTIVITY.md),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md), and
[retention](ATTIC_MIGRATION_PLAN.md).

## Checkpoint 851: Current source boundary and catalog dependencies carried forward

The reviewed `areas/` and `src/` content remains identical to pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`; this checkpoint carries the Checkpoint 396 closure forward without claiming a second independent parse. The active WLD still consists of rooms #400000 and #400999 with two resolving reciprocal local exits, no authored external WLD edge, and no inactive-WLD approach or room overlap. No declared type-25 object targets either room. The conditional death path still creates portal #400220, sets its destination to the victim's room, and places it in #400000; this code-created route is separate from WLD and does not establish safe travel. The Checkpoint 396 global totals of 9,479 overall / 297 Surf are historical; current curated source-only totals are 11,993 overall / 415 Surf. Tradeskills adds no approach.

The previously recorded typed closure remains material: eight active foreign ZON sources contain 182 reset rows, the active QST manifest has 717 give/remove rows, five external shop records offer local items, and four external story sidecars contain typed item references. `dalvik.qst` and `ravenloft2.qst` remain unregistered QST sources; local `tradeskills.qst` remains only a sentinel. The 293-object active catalog also remains a maintained numeric API used by compiled reset, quest, shop, portal, salvage, mining, alchemy, enhancement, and other item-specific code. These source declarations do not prove runtime availability or settlement.

Keep active `tradeskills` **DO NOT MOVE** and add no candidate for the active zone. Preserve unlisted `tradeskills.old.obj` separately on existing candidate row 108 as **RETAIN IN PLACE**: its 234 overlapping IDs include 218 matching whole records and 16 differences, while the active file has 59 additional definitions. Do not merge or load the old catalog with the active one. No source activation, relocation, or migration clearance follows. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L309), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-851-tradeskills-graph-and-dynamic-portal-remain-distinct), [attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-851-tradeskills-stays-active-with-the-old-catalog-separate), and [old-catalog candidate row 108](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L108).
