# Valley of Crushk (Priority 212) repository-wide audit review

This review supplements the [Valley of Crushk source dossier](../zone-stories/VALLEY_CRUSHK.md) with the active and inactive WLD sweep, typed cross-area references, compiled numeric review, bounded title/history review, and migration disposition. These static records do not establish installed access or player qualification.

## Active map, incoming leaves, and exits

Valley of Crushk has 85 rooms, #2001–#2088 with three reserved gaps, and 182/182 loader-resolving exits: 180 local and two outbound. The local graph has one weak component; raw directed traversal from root #2001 reaches and returns from all 85 rooms. There are no fall-chance or current-flag rooms.

Two active WLD inbounds enter the Valley. Mini Zones #5806 south reaches #2001 without a reciprocal Valley exit. Pharr Valley #40300 west reaches #2006 and is reciprocated by Valley #2006 east to #40300. Valley #2060 south is a second outbound edge to Surface #548133; that Surface room has east, south, and west exits but no north return. See [Mini Zones source](../../../areas/wld/minizones.wld#L2905), [Pharr Valley source](../../../areas/wld/pharrvly.wld#L2009), [Valley to Pharr Valley](../../../areas/wld/valley_crushk.wld#L115), [Valley to Surface](../../../areas/wld/valley_crushk.wld#L1075), [Surface destination](../../../areas/wld/surface.wld#L1010425), active exit-register [rows 33](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L33), [34](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L34), [99](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L99), and [551](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L551), plus pair-summary [rows 273](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L273), [341](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L341), [767](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L767), and [768](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L768).

## Inactive WLD sweep

All 95 inactive WLD files were parsed: 527,791 rooms and 2,080,773 exits, with no parse errors. No inactive room VNUM overlaps the Valley, and one non-overlapping source-only exit enters it: `surf.wld` room #232517 north to Valley #2060. This is not active ingress. Curated source-only totals rise to 3,406 overall and 174 for Surf; `ud_map2` remains 15. Preserve the source exit with existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), citing [surf.wld line 682145](../../../areas/wld/surf.wld#L682145) and [Valley room #2060](../../../areas/wld/valley_crushk.wld#L1063).

## Typed item, reset, and shop dependencies

The Valley's native QST and story contract require the exact external Ship Yards bandit shiv #43143: [valley_crushk.qst line 20](../../../areas/qst/valley_crushk.qst#L20) uses `G I 43143`, and the story sidecar names #43143 in the optional item field at [line 311](../../../areas/story/valley_crushk.story.json#L311). Ship Yards defines [object #43143](../../../areas/obj/shipy.obj#L607) and has eleven active paired reset rows for bandit #43198 and shiv #43143 at room #43328, beginning at [shipy.zon line 917](../../../areas/zon/shipy.zon#L917). The Valley's accepted supplied-item eligibility does not prove current availability or first-source custody.

Active foreign ZON resets reference Valley objects #2009 (glowing white wand) and #2020 (black leather backpack) from Alatorin, #2020 from Charing, and #2012 (brown potion) from Surface Keeps: [Alatorin lines 2548 and 3460](../../../areas/zon/alatorin.zon#L2548), [Charing line 365](../../../areas/zon/charing.zon#L365), and [Surface Keeps line 244](../../../areas/zon/surfacekeeps.zon#L244). Active SHP products also list #2020 in Alatorin and Charing, and #2012 in Surface Keeps: [Alatorin shop line 272](../../../areas/shp/alatorin.shp#L272), [Charing shop line 411](../../../areas/shp/charing.shp#L411), and [Surface Keeps shop line 242](../../../areas/shp/surfacekeeps.shp#L242).

Valley resets and its grocer stock import Heavens objects #399 (lantern) and #606 (canvas sack): see [valley_crushk.zon lines 208–209](../../../areas/zon/valley_crushk.zon#L208), the grocer's [SHP products at lines 39–42](../../../areas/shp/valley_crushk.shp#L39), [Heavens lantern #399](../../../areas/obj/heavens.obj#L4153), and [canvas sack #606](../../../areas/obj/heavens.obj#L6370). A separate local reset at [valley_crushk.zon line 138](../../../areas/zon/valley_crushk.zon#L138) references mobile #207, for which no exact mobile prototype exists in the 445 raw MOB inputs scanned; retain this as an unresolved source reset reference rather than a confirmed external prototype.

No external QST item command or structured story item reference targets a Valley prototype. The only external SHP products targeting local objects are the #2020 and #2012 entries listed above. No external duplicate mobile/object prototype or external type-25 target was found. The local object #2012 also appears in Chaos equipment data at [chaos_eq_data.h line 579](../../../src/account/chaos_eq_data.h#L579). Other raw compiled-code matches are non-prototype values: `telemetry_failure.c` uses codes 2002, 2003, 2006, 2013, and 2055 at [lines 26–30](../../../src/telemetry/telemetry_failure.c#L26), while remaining matches are dates, comments, buffer sizes, or source-line text. No local custom handler binding was identified.

## Title and bounded history

The normalized title “Valley of Crushk” is unique among 442 scanned `.zon` files. Followed `valley_crushk.wld` history reaches the 2009 areas-tree move, a 2011 same-stem change, and an opaque 2013 `stuff` change; `valley_crushk.zon` history reaches the same move, the 2013 change, and the 2015 quest-item flag pass. These bounded histories establish no alternate-stem rename, distinct predecessor, or whole-zone replacement. See the [title source](../../../areas/zon/valley_crushk.zon#L2).

The semantic title comparison also distinguishes nearby valley names such as
Pharr Valley, Pharr Valley Swamp, and Valley of the Snow Ogres. The reciprocal
Pharr boundary is neighboring-map evidence; the Surf #232517 approach remains
source-only candidate context under row 97. White Harbor's QST mentions
“Crushk” in a list of places, not as another map owner. See [Checkpoint
769](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-769-valley-of-crushk-neighboring-titles)
and [the White Harbor text](../../../areas/qst/wh.qst#L1339).

## Migration disposition

Keep active `valley_crushk` **DO NOT MOVE**. Preserve both active inbounds, the reciprocal Pharr Valley boundary, and the one-way Surface departure. Preserve the inactive Surf #232517 approach with candidate row 97. Keep the Ship Yards shiv, Alatorin/Charing/Surface Keeps resets and shop references, Heavens imports, and the unresolved mobile #207 reset visible for any later migration review. No candidate row is added. See [Valley coverage row 334](ZONE_COVERAGE_REGISTER.csv#L334), [Mini Zones row 176](ZONE_COVERAGE_REGISTER.csv#L176), [Pharr Valley row 229](ZONE_COVERAGE_REGISTER.csv#L229), [Surface row 286](ZONE_COVERAGE_REGISTER.csv#L286), [Ship Yards row 273](ZONE_COVERAGE_REGISTER.csv#L273), [Alatorin row 6](ZONE_COVERAGE_REGISTER.csv#L6), [Charing row 55](ZONE_COVERAGE_REGISTER.csv#L55), [Surface Keeps row 287](ZONE_COVERAGE_REGISTER.csv#L287), and [Heavens row 121](ZONE_COVERAGE_REGISTER.csv#L121).

## Checkpoint 874: current typed-source refresh

CP874 rechecks the typed owners against the tracked source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. The references in the original review remain unchanged:

| Source relation | Current source evidence | Owner and classification |
| --- | --- | --- |
| Valley quest and story item | valley_crushk.qst line 20 issues G I #43143; the structured sidecar names #43143 in its optional item field at line 311 | Active Ship Yards OBJ #43143 remains the supplied-item source; its active reset records stay with Ship Yards. This does not prove live stock or custody. |
| Valley reset and grocer stock | valley_crushk.zon lines 208–209 reset Heavens OBJ #399/#606; grocer SHP product lines 39–42 list those alongside local Valley OBJ #2020/#2026 | Keep both foreign object imports with Heavens; the two local products remain Valley-owned. |
| Active external resets into Valley objects | Alatorin ZON lines 2548/#2009 and 3460/#2020; Charing ZON line 365/#2020; Surface Keeps ZON line 244/#2012 | Keep these reset references on their current source owners. |
| Active external shop products into Valley objects | Alatorin SHP line 272/#2020; Charing SHP line 411/#2020; Surface Keeps SHP line 242/#2012 | Keep the shop product references with those shop owners. |
| Unresolved local mobile reset | valley_crushk.zon line 138 requests M #207 | No exact mobile prototype #207 was found in the 445 raw MOB inputs. Preserve the reset as unresolved source data; do not infer a mobile owner or rename target. |

The exact external QST item-command scan and the structured story item_vnums scan found no reference into Valley-owned prototypes. No other external active ZON reset or SHP product, no duplicate external MOB/OBJ definition, and no external type-25 destination into Valley rooms was found. The active type-25 census covered all 713 current active objects. These results confirm the existing source table above rather than create additional dependencies.

The exact compiled scan still finds local OBJ #2012 in a static chaos equipment table at [chaos_eq_data.h line 579](../../../src/account/chaos_eq_data.h#L579); this table value does not prove kit selection or issuance. Other raw number matches remain dates, telemetry error codes, buffer sizes, comments, or line-number text, as classified in the original review. No local custom handler binding was found.

The active graph, the two active inbound routes, the reciprocal Pharr boundary, the one-way Surface departure, and the non-overlapping source-only Surf room #232517 north exit to #2060 remain as documented in the existing [route review](#active-map-incoming-leaves-and-exits) and [inactive WLD review](#inactive-wld-sweep). The Surf route remains on its existing candidate row and is not active ingress. Current curated source-only totals remain 11,993 overall / 415 from Surf.

Keep the previously recorded disposition: active valley_crushk stays DO NOT MOVE, no candidate is added, and external prototype ownership remains attached to the source areas listed above. The unresolved M #207 target, editor/tool closure, deployment/runtime state, and owner intent remain open for future qualification. No gameplay or source change is included in this review.

