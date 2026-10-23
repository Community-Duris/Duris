# Keep of Evil (Priority 218) repository-wide audit review

This supplement adds current WLD reconciliation, inactive-map approaches, typed cross-area references, and title/history review to the [comprehensive Keep of Evil source dossier](../zone-stories/KEEP_OF_EVIL.md). These static findings do not qualify installed access, item publication, or player outcomes.

## Active map and directed reach

Keep of Evil (`evkeep`) has 147 rooms (#44801–#44947) and 401/401 resolving exits: 400 local and one outbound. Its weak components contain 91 and 56 rooms. With all authored local exits treated as available, raw traversal from root #44801 reaches 91/147 rooms and 91/147 have paths back to that root. The outbound west edge from #44801 pairs with Surface #583837 east. White Harbor #55634 has a separate one-way up edge into Keep room #44888. See the [root/outbound source](../../../areas/wld/evkeep.wld#L1), [White Harbor arrival](../../../areas/wld/wh.wld#L13105), exit-register [row 621](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L621), [Surface return row 1527](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1527), and [White Harbor inbound row 801](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L801), plus pair-summary [rows 135](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L135), [498](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L498), and [791](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L791).

## Inactive WLD sweep

All 95 inactive WLD files parsed: 527,791 rooms and 2,080,773 exits, with no parse errors or room-VNUM overlap against active Keep rooms. One non-overlapping source-only approach enters through `surf.wld`: room #270471 east (`D1`) to #44801 ([source room](../../../areas/wld/surf.wld#L1478783), [exit target](../../../areas/wld/surf.wld#L1478794)). It is not active ingress. Curated source-only totals rise from 3,430 to 3,431 overall and from 197 to 198 for Surf; `ud_map2` remains 15. Preserve this record with existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); no candidate row is added.

## Typed and compiled references

The local aged-librarian Q2 contract consumes golden desk key #44880 for 150,000 XP without disappearance; the exact QST entry is at [evkeep.qst line 8](../../../areas/qst/evkeep.qst#L8). Keep's local reset imports Heavens object #359 at [evkeep.zon line 266](../../../areas/zon/evkeep.zon#L266) and Winterhaven object #55186 at [line 448](../../../areas/zon/evkeep.zon#L448). These remain distinct from its own locally defined prototypes.

One unregistered reset source points back into a Keep object: `brass-old-1.zon` line 529 has `G object#44876`, the local short flaming rapier ([reset source](../../../areas/zon/brass-old-1.zon#L529), [object](../../../areas/obj/evkeep.obj#L1207)). This is a source-only reference, not an active reset. Preserve it with the existing [Brass Old 1 candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17).

The direct compiled scan finds references to three local object VNUMs in these table entries: [epic_skills.c line 189](../../../src/classes/epic_skills.c#L189) lists local object #44824 for `SKILL_SPELL_PENETRATION`; [chaos_eq_data.h line 158](../../../src/account/chaos_eq_data.h#L158) and [line 407](../../../src/account/chaos_eq_data.h#L407) list local #44827 in Chaos equipment arrays; and [line 376](../../../src/account/chaos_eq_data.h#L376) lists local #44877. Their object definitions are [#44824](../../../areas/obj/evkeep.obj#L356), [#44827](../../../areas/obj/evkeep.obj#L401), and [#44877](../../../areas/obj/evkeep.obj#L1224). Preserve the exact table bindings without inferring extra player history or effect from an item name.

No external QST item command, SHP product, structured story-sidecar VNUM, or active ZON reset-role reference to Keep prototypes was found. All 231 external story sidecars parsed without errors and had no typed VNUM match. External MOB/OBJ definitions do not duplicate a Keep prototype. No external type-25 target enters a Keep room among 871 external declarations; the 872nd declaration is local monument #44830 targeting local room #44892. See [monument object #44830](../../../areas/obj/evkeep.obj#L453). Local reset and compiled uses, installed callbacks, generated state, and persisted references remain separate. The object and portal scan covered 442 files and 21,682 object records.

## Title and bounded history

The normalized title “The Keep of Evil” is unique among 442 scanned `.zon` titles; see the [current title source](../../../areas/zon/evkeep.zon#L2). Followed `evkeep.wld` history stays on the same stem through the 2009 areas-tree move, 2010 arena-room change, 2011 edits, and 2013 edit. `evkeep.zon` history includes the 2015 quest-item-flag pass. This bounded same-stem review found no alternate-stem rename, distinct predecessor, or whole-zone replacement. The inactive Surf entrance is a single source-only map edge, not replacement evidence.

## Migration disposition

Keep active `evkeep` **DO NOT MOVE**. Preserve the reciprocal Surface boundary, one-way White Harbor inbound, source-only Surf approach, Keep resets that import Heavens #359 and Winterhaven #55186, the Brass Old 1 reset reference to #44876, and the compiled uses of #44824/#44827/#44877. Keep `surf` and `surface2011` **RETAIN IN PLACE** with candidate row 97 and `brass-old-1` with candidate row 17. See [Keep coverage row 93](ZONE_COVERAGE_REGISTER.csv#L93), [Surface row 286](ZONE_COVERAGE_REGISTER.csv#L286), [White Harbor row 345](ZONE_COVERAGE_REGISTER.csv#L345), [Heavens row 121](ZONE_COVERAGE_REGISTER.csv#L121), and [Brass Old 1 row 368](ZONE_COVERAGE_REGISTER.csv#L368).
