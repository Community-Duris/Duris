# moriatst source and shop-dependency audit

Canonical key: component:moriatst. Coverage data row 423 / CSV line 424;
candidate dependency data row 72 / CSV line 73. Reviewed October 11, 2026,
Checkpoint 510, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source, content,
typed-reference, build-path, and bounded-history review. It does not establish
which shop stream a running service loaded or qualify live trades and resets.

## Source inventory and story content

The component is one unregistered source file,
[`areas/shp/moriatst.shp`](../../../areas/shp/moriatst.shp#L1). It contains
three NEW-format shop records headed #99035, #99036, and #99037. No matching
WLD, MOB, OBJ, ZON, QST, story sidecar, or other same-stem file is present.
The stem is absent from [`areas/AREA`](../../../areas/AREA#L359) and
`areas/qst/AREA`. There is no room description or standalone environmental
story. The file's stock, keeper prompts, and room associations duplicate the
three shop records in [active Moria's source](../../../areas/shp/moria.shp#L1).

The seven message strings in each shop are generic transaction prompts. Some
use coarse or profane wording; they do not establish an independent quest,
setting, or character arc. For the broader Neverwinter Woods story, see the
[Moria dossier](../zone-stories/NEVERWINTER_WOODS.md) and its
[repository-wide source review](MORIA_AUDIT_REVIEW.md).

## Typed shop references and exact active-source comparison

The three records directly bind candidate shop data to existing Moria
identities:

| Shop record | Keeper mobile | In-room field | Stock object VNUMs | Candidate raw buy/sell multipliers | Active `moria.shp` |
| --- | ---: | ---: | --- | --- | --- |
| #99035 | #99035 | #99306 | #99063–#99065 | 0.90 / 1.10 | Same keeper, room, and stock; active raw buy/sell fields are 0.80 / 1.10 |
| #99036 | #99036 | #99312 | #99066–#99068 | 1.00 / 1.10 | Same keeper, room, and stock; active raw buy/sell fields are 0.80 / 1.10 |
| #99037 | #99037 | #99318 | #99069–#99070 | 1.00 / 1.00 | Same keeper, room, and stock; active raw buy/sell fields are 0.80 / 1.10 |

The referenced keeper prototypes, stock objects, and rooms are defined in
active Moria: for example, the rooms are
[#99306](../../../areas/wld/moria.wld#L8567),
[#99312](../../../areas/wld/moria.wld#L8688), and
[#99318](../../../areas/wld/moria.wld#L8810). All three candidate shop
records therefore have resolvable typed references in the active Moria
source set; they do not define additional mobiles, objects, or rooms.

Normalized line comparison with `moria.shp` finds the same three shop IDs,
keeper and room bindings, stock lists, trade types, other shop settings, and
transaction text. The source differences are the three raw buy multipliers
shown above, #99037's raw sell multiplier (1.00 candidate versus 1.10 active),
and spacing before trailing tildes in three empty message strings in #99037.
The current [loader clamps](../../../src/economy/shop.c#L3425) `buy_percent`
at 0.80 and `sell_percent` at a minimum of 1.00. If these records are parsed,
the candidate buy fields become 0.80, but the #99037 sell field remains 1.00
rather than active Moria's 1.10. This source comparison does not prove that
either file was loaded or that trades occurred.

## WLD connectivity and conditional loader exposure

This is an SHP-only component with no room records or exits, so local WLD
component, range, and exit counts are not applicable. The candidate dependency
register records zero active WLD inbound exits and zero unresolved active WLD
exit references. Its `in_room` values #99306, #99312, and #99318 are typed
shop-placement references into active Moria rooms, not WLD edges.

The normal shop aggregation path is list-based: `make_shp` reads the root
`AREA` manifest and opens `shp/<listed-stem>.shp`; that manifest lists `moria`
but not `moriatst`. The full-world `m_slow` path runs this aggregation before
moving the generated shop stream to `areas/world.shp`. The current checkout
has no `areas/world.shp` output. Separately, runtime `open_shop_stream` first
opens `areas/world.shp` and, if it cannot, falls back to concatenating every
`areas/shp/*.shp` file (then appending `end.shp`). That fallback includes
`moriatst.shp` despite its omission from `areas/AREA`
([shop loader](../../../src/economy/shop.c#L3267),
[normal compiler](../../../areas/src/shp/make_shp.c#L16),
[full build wrapper](../../../areas/m_slow#L1)).

If that fallback is used, it reads both the active `moria.shp` records and
these duplicate keeper/room records. The reset-side
[`configured_shopkeeper_for_room`](../../../src/world/db.c#L6896) returns no
single shop match when more than one record has the same keeper and room; a
second native birth-tail selector also treats duplicate keeper/room matches
as ambiguous ([selector](../../../src/economy/shop.c#L3796)). This is a
conditional source-code exposure, not proof of a live boot path or observed
gameplay failure. Resolve the loader contract before treating the unlisted
file as safe to archive or move.

## Duplicate content, history, and disposition

The candidate and active Moria shop files are distinct paths with duplicated
content. `git log --follow` shows same-path history through the November 2013
update and the August 2009 area-tree move; it does not establish an
alternate-stem rename, editor ownership, retirement, or replacement. The
similarity and `moriatst` stem are not enough to decide whether this is an
intentional test copy, an abandoned alternate, or an accidental duplicate.

**Disposition: RETAIN IN PLACE.** Keep the file and its bytes unchanged while
the owner confirms its purpose and the operators resolve every shop-loader
entry path. A future migration decision must account for the wildcard
fallback, the duplicated keeper/room bindings, and any generated or deployed
`world.shp` outputs. No shop load, story completion, or file move is cleared.
See the [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-510-moriatst-retained-pending-shop-loader-closure),
[duplicate/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-510-moriatst-shop-copy-and-loader-path),
[connectivity note](WORLD_CONNECTIVITY.md#checkpoint-510-moriatst-has-no-wld-graph),
[12 builder questions](MORIATST_BUILDER_QUESTIONS.md),
[candidate dependency row 72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L73), and
[coverage row 423](ZONE_COVERAGE_REGISTER.csv#L424).

## Checkpoint 921: Moria role bindings and shop-factor correction

The source-role census confirms that every keeper, room, and stock reference
in the three `moriatst.shp` records resolves by type to active Moria: keeper
mobiles #99035–#99037, rooms #99306/#99312/#99318, and stock objects
#99063–#99070. Active [Moria mobile definitions](../../../areas/mob/moria.mob#L487),
[stock objects](../../../areas/obj/moria.obj#L611), and the three
[room definitions](../../../areas/wld/moria.wld#L8567) own these references.
Active [Moria reset #99035 to room #99306](../../../areas/zon/moria.zon#L358),
[reset #99036 to room #99312](../../../areas/zon/moria.zon#L385), and
[reset #99037 to room #99318](../../../areas/zon/moria.zon#L405) place each
keeper in the same room named by both shop files; following G rows place the
corresponding stock objects. The maintained
[Moria special assignments](../../../src/specs/specs.assign.c#L1184) bind
#99035–#99037 to `nw_brock`, `nw_merthol`, and `nw_vitnor`. A scan of all
source SHP files found no third shop record using the same shop IDs or typed
keeper/room/product roles.

The overlapping numerals remain role-specific. SHP header #99035 is a shop
record ID, while that record's keeper field is mobile VNUM #99035; Moria also
defines WLD and OBJ records for #99035–#99037. The stock fields are object
VNUMs even where same-number WLD rooms exist. All-source QST and 232 story
JSON scans found no typed giver, item, room, or completion-key item reference
to these Moria roles.

CP921 also corrects the earlier sell-factor comparison: [candidate #99037](../../../areas/shp/moriatst.shp#L75)
stores raw buy/sell values 1.00/1.00, while [active Moria #99037](../../../areas/shp/moria.shp#L75)
stores 0.80/1.10. The [buy clamp](../../../src/economy/shop.c#L3436) sets the
candidate buy factor to 0.80, while the [sell minimum](../../../src/economy/shop.c#L3427)
leaves its sell factor at 1.00. That record would retain a 0.10
sell-multiplier difference if the fallback reads both files. The duplicate
keeper/room selector ambiguity and
conditional fallback exposure remain; no live stream selection is inferred.
**Disposition remains RETAIN IN PLACE.** Preserve the source file and all
three records pending owner and loader-path review. See [MST-06, MST-07,
and MST-09](MORIATST_BUILDER_QUESTIONS.md), [Moria supplement](MORIA_AUDIT_REVIEW.md#checkpoint-921-moriatst-typed-shop-roles-and-effective-factors),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-921-moriatst-shop-rooms-are-not-wld-edges),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-921-moriatst-roles-confirm-duplicate-source-not-lineage),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-921-moriatst-role-closure-keeps-the-candidate-in-place),
[candidate dependency row 72](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L73), and
[coverage row 423](ZONE_COVERAGE_REGISTER.csv#L424).
