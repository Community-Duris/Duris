# The Church of the Eternal Dusk repository-wide audit supplement

This supplement reconciles the active Church WLD, reciprocal Surface boundary, inactive-map scan, typed prototype references, and bounded title/history evidence with the [source-comprehensive Church dossier](../zone-stories/CHURCH_OF_THE_ETERNAL_DUSK.md). The frozen source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed.

## Active graph and boundary

The active [Church WLD](../../../areas/wld/church.wld#L1) has 33 rooms and 69 exits; all 69 resolve. Of these, 68 are local and one crosses areas. The local graph is one weak component of 33 rooms. Raw directed traversal from exterior #87820 reaches and returns from all 33 rooms when authored exits are treated as available. The dossier separately models the spoken-keyword passage, cell keys/PICK alternatives, and admitted movement; raw graph reach does not mean every gate is open or every route was played.

The only active boundary is reciprocal with Surface: Paladin camp #87847 east → #527483, paired with Surface #527483 west → #87847 ([Church exit](../../../areas/wld/church.wld#L549), [Surface exit](../../../areas/wld/surface.wld#L577022), [outbound pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L78), [inbound pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L480)). There are no unresolved or negative targets, inbound-only records, falls, or currents.

## Inactive-map sweep

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parser errors. It found no source-room VNUM overlap with Church rooms #87820–#87852 and no inactive exit target into them. No source-only approach is added. Curated source-only totals remain 2,796 overall and 156 for the reviewed Surf subset. This static scan does not establish the availability or safety of runtime movement and non-WLD systems.

## Typed references and the retained Brass source

No active outside QST command, SHP product, ZON reset, or type-25 portal target into a Church prototype/room was found. Church has no local SHP source. Exact typed scans also found no outside story-sidecar VNUM, no outside C/C++ numeric binding, and no external mobile/object prototype definition overlapping the 55 local mobiles or 55 local objects. All 231 outside story sidecars parsed with no exact numeric token hits. None of the 872 outside type-25 declarations targets a Church room.

The all-file reset-role scan does find one source-only legacy reference: unregistered `brass-old-1.zon` line 924 `G`-loads active Church object #87859, the staff of swirling smoke and flame ([reset](../../../areas/zon/brass-old-1.zon#L924), [object](../../../areas/obj/church.obj#L597)). `brass-old-1` is not listed in current [areas/AREA](../../../areas/AREA#L397), so this is not a selected live reset. Preserve this typed dependency in the existing [Brass candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17); keep `brass-old-1` **RETAIN** pending its remaining source and tooling closure. It adds no active stock and does not imply a whole-zone replacement relationship.

## Title, history, and disposition

After color removal and whitespace/case normalization, “The Church of the Eternal Dusk” has one exact owner among 351 active `.zon` contributors: `church` ([title source](../../../areas/zon/church.zon#L2)). Followed Church WLD, MOB, OBJ, ZON, and QST paths show the 2009 area-tree relocation and later same-stem maintenance, including the 2015 quest-item-flag pass. Those bounded paths show no alternate-stem rename, explicit predecessor, or whole-zone replacement.

`church` remains a current contributor in [the world list](../../../areas/AREA#L308) and [QST source manifest](../../../areas/qst/AREA#L274). Keep the active source **DO NOT MOVE**. The separate Church ownership repair and preserved Kelek receipts remain documented in the existing dossier; this audit adds no source repair, placement change, playable result, or archive clearance. See [coverage row 56](ZONE_COVERAGE_REGISTER.csv#L56), the [Surface boundary summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L78), [connectivity checkpoint](WORLD_CONNECTIVITY.md), [history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).
