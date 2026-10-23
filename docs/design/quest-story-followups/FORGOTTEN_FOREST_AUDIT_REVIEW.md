# The Forgotten Forest (Priority 213) repository-wide audit review

This review supplements the [Forgotten Forest source dossier](../zone-stories/FORGOTTEN_FOREST.md) with the active and inactive WLD sweep, typed cross-area references, compiled numeric review, title/history review, and migration disposition. These static records do not establish installed access, item publication, or player qualification.

## Active map, incoming routes, and directed reachability

The Forgotten Forest has 92 rooms (#82701–#82792) and 283/283 loader-resolving exits: 282 local and one outbound. The local graph has one weak component. With authored exits available, raw directed traversal from root #82701 reaches 90/92 rooms and has return paths from 91/92; #82702 is exitless and has no root return, while #82763 is not reached from the root. The dossier records the loading ledge at #82763; this traversal describes authored WLD exits only. See [root #82701](../../../areas/wld/forgotten_forest.wld#L1), [exitless #82702](../../../areas/wld/forgotten_forest.wld#L20), and [loading room #82763](../../../areas/wld/forgotten_forest.wld#L1547).

The single active cross-area boundary is reciprocal: Surface #516090 east enters Forest #82701, and Forest #82701 west returns to Surface #516090. See [Surface source](../../../areas/wld/surface.wld#L337827), [Forest source](../../../areas/wld/forgotten_forest.wld#L11), exit-register [rows 995](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L995) and [1340](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1340), and pair-summary [rows 147](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L147) and [503](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L503).

## Inactive WLD sweep

All 95 inactive WLD files were parsed: 527,791 rooms and 2,080,773 exits, with no parse errors or room-VNUM overlap with the active Forest. Fifteen source-only exits enter Forest rooms from 13 non-overlapping `surf.wld` rooms: #268152 south to #82764, #268153 south and #268454 west to #82768, #268754 and #269055 west to #82767, #269351 north and west to #82742, #269354 north and west to #82767, #269653 north to #82766, #269952 west to #82729, #270251 north to #82728, #270545 east to #82701, #270849 west and #271148 north to #82705. Exact source rooms are [#268152](../../../areas/wld/surf.wld#L1430088), [#268153](../../../areas/wld/surf.wld#L1430109), [#268454](../../../areas/wld/surf.wld#L1436426), [#268754](../../../areas/wld/surf.wld#L1442726), [#269055](../../../areas/wld/surf.wld#L1449047), [#269351](../../../areas/wld/surf.wld#L1455263), [#269354](../../../areas/wld/surf.wld#L1455326), [#269653](../../../areas/wld/surf.wld#L1461605), [#269952](../../../areas/wld/surf.wld#L1467884), [#270251](../../../areas/wld/surf.wld#L1474159), [#270545](../../../areas/wld/surf.wld#L1480333), [#270849](../../../areas/wld/surf.wld#L1486713), and [#271148](../../../areas/wld/surf.wld#L1492988). These exits are not active ingress. Curated source-only totals rise to 3,421 overall and 189 for Surf; `ud_map2` remains 15. Preserve the exits with existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Typed item, reset, shop, and compiled references

The local QST contains four independent recipes using local food objects, and no external active or unregistered QST item command or structured story `item_vnums` reference targets a Forest room, mobile, or object. No local SHP file exists, and no external SHP product or ZON reset-role reference targets a Forest prototype. No external mobile/object prototype duplicates a local VNUM. Among 872 scanned object type-25 declarations, none targets a Forest room. All 231 external story sidecars parsed without errors and had no numeric local-VNUM hits.

The compiled scan found one local object-VNUM dependency: the Chaos standard druid table lists feathered gloves #82707 in slot 9 at [line 133](../../../src/account/chaos_eq_data.h#L133), and the enhanceable druid table repeats it at [line 383](../../../src/account/chaos_eq_data.h#L383). Keep this object reference visible in any later source or content review. No other compiled exact-VNUM match was found. The local prototype is [object #82707](../../../areas/obj/forgotten_forest.obj#L83). The absence of bounded raw references does not qualify generated callers or installed behavior.

## Title and bounded history

The normalized title “The Forgotten Forest” is unique among 442 scanned `.zon` files; see the [title source](../../../areas/zon/forgotten_forest.zon#L2). Followed `forgotten_forest.wld` history reaches the 2009 areas-tree move and a 2011 same-stem edit. `forgotten_forest.zon` history reaches the same 2009 move and the 2015 quest-item-flag pass. These bounded paths establish no alternate-stem rename, distinct predecessor, or whole-zone replacement.

## Migration disposition

Keep active `forgotten_forest` **DO NOT MOVE**. Preserve the reciprocal active Surface boundary and all 15 non-overlapping source-only Surf approaches with existing candidate row 97. No candidate row is added. See [Forest coverage row 101](ZONE_COVERAGE_REGISTER.csv#L101), [Surface coverage row 286](ZONE_COVERAGE_REGISTER.csv#L286), and the [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Checkpoint 803: current typed census refresh

The local zone has 96 typed mobile/object reset operands: 83 M and three F
mobile slots, four O, three G, and three E object slots. All 25 local mobiles
and all seven local objects are reset-used. Every typed prototype and
explicit reset-room operand resolves locally; no import or unresolved value
was found. A role-aware scan of all active and source-only ZON files found no
outside reset reference to a Forest mobile, object, or room.
[Local reset source](../../../areas/zon/forgotten_forest.zon#L1).

The local QST has four independent `G I` food inputs: blackberries #82703,
small yellow mushroom #82702, chunk of meat #82701, and large chunk of meat
#82706. The four local story contracts mirror those item values in
`item_vnums` and `completion_key`; all resolve to local objects. No active or
source-only outside QST item command or giver header references a Forest
prototype. The all-source typed sidecar scan found no outside item, giver,
mobile, or room VNUM reference. No local SHP exists, and no active or
source-only shop product, WLD door-key field, or same-type duplicate
prototype references a Forest object.
[QST inputs](../../../areas/qst/forgotten_forest.qst#L8),
[sidecar input 1](../../../areas/story/forgotten_forest.story.json#L216),
[input 2](../../../areas/story/forgotten_forest.story.json#L253),
[input 3](../../../areas/story/forgotten_forest.story.json#L290),
[input 4](../../../areas/story/forgotten_forest.story.json#L327).

The local object file has no type-25 or type-29 destination. The full scan of
872 type-25 and 380 type-29 declarations found no outside destination into a
Forest room. These item-role scans add no WLD edge; the reciprocal Surface
pair and 15 source-only Surf approaches remain as recorded above.

The only direct compiled object-VNUM use is object #82707 in the standard
Chaos and enhanceable druid equipment tables. No other exact source numeric
intersection resolves as a Forest room, mobile, or object VNUM. Keep this
source table reference visible without treating it as proof of generated or
installed equipment availability.
[Object #82707](../../../areas/obj/forgotten_forest.obj#L83),
[Chaos table](../../../src/account/chaos_eq_data.h#L133),
[enhanceable table](../../../src/account/chaos_eq_data.h#L383),
[coverage row 100](ZONE_COVERAGE_REGISTER.csv#L101).

Keep active `forgotten_forest` DO NOT MOVE and preserve the local food
contracts, #82707 equipment-table reference, reciprocal Surface boundary,
and all 15 existing source-only Surf approaches with candidate row 97. No
WLD total, candidate disposition, area source, or owner question changed.
Item supply, contract settlement, installed equipment, and player access
remain unqualified.
