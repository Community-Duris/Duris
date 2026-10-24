# Tradeskills old object catalog audit

This review covers the object-only source
[`areas/obj/tradeskills.old.obj`](../../../areas/obj/tradeskills.old.obj#L1)
at source snapshot `c500ac51b8cd26540943257e7ca4e9807263ef74`. It adds no WLD,
mobile, reset, quest, shop, map, or story-sidecar component. No gameplay or
area source was changed. The generated `areas/world.obj` and deployed runtime
catalog were not inspected.

## Inventory and record comparison

The file defines 234 objects, continuously numbered #400000–#400233. Every
VNUM also has a definition in the selected
[active catalog](../../../areas/obj/tradeskills.obj#L1), which has 293
records. Text-normalized whole-record comparison finds 218 exact matches and 16 differing
records; the active catalog has 59 additional VNUMs and the old file has no
VNUM absent from the active file. The differing IDs are #400094, #400098,
#400120–#400122, #400128, #400133, #400155, #400199, #400201, #400216–#400218,
#400220, #400232, and #400233. Differences include prose and formatting as
well as numeric object fields, so the 16 records are not all cosmetic copies.

The clearest identity change is #400232: the old record describes an elven
equipment bag, while the current record is turkey innards. Maintained code
defines #400232 as `TURKEY_INNARDS_VNUM`, and the active catalog also defines
the rare faerie bag at #400235
([current constants](../../../src/economy/tradeskill.h#L11),
[old record](../../../areas/obj/tradeskills.old.obj#L1932),
[active record](../../../areas/obj/tradeskills.obj#L1936)). Do not treat the
old object's number as a path-specific identity that current consumers can
select.

The old #400220 is a type-25 object with destination #554045. The active
#400220 has the same destination but different numeric flags, and maintained
corpse-portal code creates and recognizes the active VNUM
([old record](../../../areas/obj/tradeskills.old.obj#L1809),
[active record](../../../areas/obj/tradeskills.obj#L1813),
[creation](../../../src/combat/fight.c#L4829),
[travel](../../../src/magic/spell_travel.c#L972),
[lifecycle](../../../src/magic/spell_corpse_lifecycle.c#L1023)). The Surface
review records the active #400220 → #554045 destination and treats the old
copy as source-only rather than counting it as a loaded travel object
([Surface audit](SURFACE_AUDIT_REVIEW.md#L43)).

## Selection, story, and history

The top-level [area manifest](../../../areas/AREA#L406) selects `tradeskills`
and has no `tradeskills.old` entry. The old component has no matching WLD,
MOB, ZON, QST, SHP, or story-sidecar file, and `areas/qst/AREA` does not select
an old Tradeskills quest path. Its prose is object-description content for
salvage materials, crafting items, and other prototypes; it does not establish
an independent place or quest narrative.

The tracked history adds `tradeskills.old.obj` on 2013-11-18 while also
modifying `tradeskills.obj`. `git log --follow` traces the old file's earlier
similarity to the active stem; it does not record a semantic replacement
decision. The old path continued to receive edits through 2016-08-15, while
the active file continued through 2026. The tracked record establishes a
separate catalog copy and later divergence, not why it was kept or whether an
owner intended it as an archival snapshot.

## Connectivity and typed references

No WLD file belongs to this component, so there is no candidate room graph,
local reachability, or room-ingress count to calculate. The #400220 type-25
destination is an object-to-room declaration, not a WLD exit. It does not add
a candidate room or an active WLD boundary.

All 234 old VNUMs also resolve to current definitions in
`tradeskills.obj`. A ZON, QST, SHP, or code reference expressed only as one of
those numbers cannot identify `tradeskills.old.obj`; in the current source
manifest the definition belongs to the active file. The repository-wide
[active Tradeskills closure](TRADESKILLS_AUDIT_REVIEW.md#typed-reset-quest-shop-and-sidecar-references)
records its reset, quest, shop, story, and code consumers. A source search
found no maintained code or manifest entry selecting the `tradeskills.old`
path. Runtime-generated and deployed aggregates remain outside this static
source review.

## Disposition

Keep `tradeskills.old.obj` **RETAIN IN PLACE** as historical source evidence.
Keep it out of the active source manifest and never merge or load it alongside
`tradeskills.obj` without a reviewed migration: shared VNUMs can carry
different identities and fields, and current code expects active definitions
such as turkey innards at #400232. Any later archive or path change should
first reconcile the generated/deployed object catalogs and any editor or
external tooling that may retain the old filename. See the
[source dossier](../zone-stories/TRADESKILLS.md),
[20 builder questions](TRADESKILLS_OLD_BUILDER_QUESTIONS.md),
[coverage row 458](ZONE_COVERAGE_REGISTER.csv#L459),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-545-tradeskills-old-has-no-wld-graph-or-room-ingress),
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-545-tradeskills-old-is-a-separate-catalog-copy-with-active-vnum-collisions),
and [retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-545-tradeskills-old-retained-as-an-inactive-catalog-source).
