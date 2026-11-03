# SurfaceKeeps (The Depths of Duris) audit review

Checkpoint 374 supplements the comprehensive [Depths of Duris source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md) with the current active graph, inactive WLD identity and approach review, typed references, and bounded title/history findings. The dossier remains the source for the fifteen native contracts and their story map. Static source findings do not establish installed access, live travel, settlement, or player outcomes.

## Active graph and boundaries

The active [SurfaceKeeps WLD](../../../areas/wld/surfacekeeps.wld) has 2,645 rooms and 9,452 authored directions. The loader resolves 9,446: 9,372 internal and 74 outbound; six positive targets remain unresolved in Duris3. There are 79 active inbound records: 76 from Alatorin, two from Tharn Rifts, and one from White Harbor. Those land in 76 distinct SurfaceKeeps rooms. All 74 outbound records go to Alatorin; 69 boundary records have a reverse-direction record. See the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv), [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv), and [unresolved exits 321–326](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L321).

The raw local directed graph has nine weak components sized 1,603, 673, 143, 87, 64, 33, 30, 11, and 1. With authored exits treated as available, traversal from the 76 inbound target rooms reaches 2,360 rooms; 285 are not reached from those anchors. Reverse traversal from all 74 outbound source rooms includes 2,644 rooms; #120000 is the only room with no static directed route to an outbound WLD edge. It is itself an inbound target, but its only two authored exits are unresolved. All 9,372 internal exits carry door flags zero and key zero. These are map-graph results only; they do not test terrain admission, item transport, scripts, or actual movement.

| Active unresolved edge | Source evidence | Current finding |
| --- | --- | --- |
| #120000 north → #129900 | [room #120000](../../../areas/wld/surfacekeeps.wld#L1), exit at [line 5](../../../areas/wld/surfacekeeps.wld#L5) | No active destination; loader prunes it. |
| #120000 west → #120099 | [exit at line 9](../../../areas/wld/surfacekeeps.wld#L9) | No active destination; loader prunes it. |
| #120819 south → #120919 | [exit at line 8707](../../../areas/wld/surfacekeeps.wld#L8707) | No active destination; loader prunes it. |
| #120826 south → #120926 | [exit at line 8866](../../../areas/wld/surfacekeeps.wld#L8866) | No active destination; loader prunes it. |
| #120918 east → #120919 | [exit at line 10313](../../../areas/wld/surfacekeeps.wld#L10313) | No active destination; loader prunes it. |
| #120925 east → #120926 | [exit at line 10410](../../../areas/wld/surfacekeeps.wld#L10410) | No active destination; loader prunes it. |

The active inbound boundaries also need to remain distinct. The two Tharn Rifts records target #120000 without an authored return. White Harbor #55403 east reaches #120823 without a reverse edge. Alatorin has 76 inbound and 74 outbound records with 69 reciprocated records. The [dossier’s access review](../zone-stories/THE_DEPTHS_OF_DURIS.md#mystardala-access-and-supplied-entry-points) also records sector-27 mountain movement limits and hostile guards. The local object #120057 is a separate type-25 lava-tube transport from room #122947 to Alatorin #83831; #83831 has a downward WLD return. Item travel is not an authored WLD boundary.

## Inactive WLD approaches and room identity

A targeted sweep of every inactive WLD file found 10,588 authored directions into loaded SurfaceKeeps room VNUMs. Of these, 9,131 begin at VNUMs owned by active WLD contributors and remain source-identity matches, not additional source-only approaches. The other 1,457 begin at VNUMs with no active room owner:

- Inactive [Duris3.wld](../../../areas/wld/Duris3.wld#L413713) contributes 1,451 directions from 958 source rooms to 1,091 SurfaceKeeps rooms: 521 north, 503 south, 213 east, and 214 west. The first example is #119700 south to #120000. Preserve this map-source evidence on [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).
- Inactive [oldmap8.wld](../../../areas/wld/oldmap8.wld#L207586) contributes five south directions: #189900 to #120000 and #189909–#189912 to #120009–#120012. Preserve them on [oldmap8 candidate row 81](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81).
- Inactive [ygg.wld](../../../areas/wld/ygg.wld#L14) contributes one up direction, #30400 to #123522. Preserve it on [ygg candidate row 122](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L122).

The 1,457 source-only records increase the curated total from 6,059 to 7,516; the reviewed Surf subset remains 285. These edges are inactive and do not add a loaded route.

Duris3 also declares all 2,645 room VNUMs in the active SurfaceKeeps WLD. None of the 2,645 paired room titles is an exact match. No other active contributor declares a SurfaceKeeps room VNUM, and no other inactive WLD stem overlaps this full room set. Record the complete numeric overlap on the Duris3 candidate; it is evidence of a separate inactive map identity, but by itself it does not prove a predecessor or replacement.

## Typed references and source ownership

The bounded exact scan covered active and unregistered QST, ZON, SHP, object, and story-sidecar sources, plus C/C++ numeric uses. It found these outgoing references from SurfaceKeeps-owned source:

- The local QST file has 25 exact item command roles referring to 14 external object VNUMs. Its sidecar has 11 typed `item_vnums` references outside the local prototype sets. The [source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md#exact-native-progression) distinguishes local contract inputs, outputs, and supplied materials.
- The local ZON file has 35 mobile reset roles over eight imported mobile VNUMs and 135 object reset roles over 74 imported object VNUMs. Its exact external reset lines remain in [surfacekeeps.zon](../../../areas/zon/surfacekeeps.zon); examples include active Lost City object #43708 under mobile #120050 at [line 122](../../../areas/zon/surfacekeeps.zon#L122), and Kobold object #1460 at [line 20](../../../areas/zon/surfacekeeps.zon#L20).
- The local SHP file contains ten shop records, #120003–#120038, and 46 product rows for objects owned outside `surfacekeeps.obj`. The coverage register’s previous zero-shop count is corrected to ten; see [surfacekeeps.shp](../../../areas/shp/surfacekeeps.shp#L1). The active Valley reference to object #2012 remains on its existing Valley review.
- Local object #120057 declares a type-25 destination to Alatorin #83831 at [surfacekeeps.obj line 915](../../../areas/obj/surfacekeeps.obj#L915). The route is separate from the WLD boundary totals.
- Maintained C++ code assigns `wh_corpse_decay` to object #120051 and `wh_corpse_to_object` to mobile #120051 in [specs.assign.c](../../../src/specs/specs.assign.c#L2628). Two Chaos kit entries also name object #120013 ([line 99](../../../src/account/chaos_eq_data.h#L99), [line 351](../../../src/account/chaos_eq_data.h#L351)). [map.h](../../../src/world/map.h#L17) defines #120000 as the Alatorin map-range start. These numeric uses are accounted for separately from QST and reset roles.

Exact outside references into local prototypes are bounded as follows:

- Active Alatorin QST has 14 item-command references over six SurfaceKeeps object VNUMs, and its story sidecar has eight typed `item_vnums` references to local objects. No external QST giver header matches a local SurfaceKeeps mobile VNUM.
- Active Alatorin ZON has 53 mobile reset roles for local mobiles #120051 and #120037, plus eleven object reset roles for local objects #120052 and #120053. Unregistered `brass-old-1.zon` has one object reset role for #120047 at [line 549](../../../areas/zon/brass-old-1.zon#L549); keep that source-only identity with its existing candidate row.
- No outside SHP product row names a local SurfaceKeeps object, and no external type-25 object targets a SurfaceKeeps room.
- No other mobile prototype file duplicates a local mobile VNUM. The unregistered [surfacekeeps_objects.obj](../../../areas/obj/surfacekeeps_objects.obj#L1) declares 54 object VNUMs also declared by active `surfacekeeps.obj`; 39 prototype bodies compare line-for-line and 15 differ. It is not in `areas/AREA`. Its followed history says it was added for “stub loading” in 2010 and records later object-file cleanup in 2016. Preserve this separate source on [candidate row 100](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L100); the overlap does not change active ownership or clear the candidate for movement.

All scanned story sidecars parsed without errors. The typed scan excludes numbers embedded in exchange IDs and prose; it records only fields named as VNUMs and exact command operands.

## Title, same-type identity, and followed history

After stripping Duris color codes and normalizing whitespace and case, “The Depths of Duris” is unique among all 351 non-empty active ZON titles and all 442 current ZON source files. No other active WLD contributor declares any SurfaceKeeps room VNUM. Duris3 is the only inactive WLD stem with room-number overlap: all 2,645 IDs overlap, with no exact room-title match. The only same-type OBJ overlap is the separate `surfacekeeps_objects.obj` stub source described above; no mobile overlap was found.

Followed WLD, MOB, OBJ, ZON, QST, and SHP history remains under the current `surfacekeeps` stem, with the source paths extending to the 2009 area history; the story sidecar was added in commit `3938f87d4` on 2026-10-04. The inactive stub object history is distinct. This bounded review found no alternate-stem rename or explicit whole-zone replacement in the followed paths or current title inventory. The 2,645-room Duris3 overlap remains a source-lineage question for the map owner. Checkpoint 755 completes the bounded title and semantic identity review without inferring a replacement; command-target alias and owner disambiguation remain separate coverage questions.

## Builder questions and recommended next steps

These twenty questions preserve unresolved design choices; none is a request to infer or ship a native-data repair.

| # | Builder question | Recommended next step | Source evidence |
| ---: | --- | --- | --- |
| 1 | Is #120000 north to #129900 an obsolete route, a missing room, or a wrong target? | Confirm the intended map destination before restoring, retargeting, or removing the edge. | [WLD line 5](../../../areas/wld/surfacekeeps.wld#L5) |
| 2 | Is #120000 west to #120099 still intended? | Decide whether the edge should reach an active room or be retired; do not substitute a guessed room. | [WLD line 9](../../../areas/wld/surfacekeeps.wld#L9) |
| 3 | Should #120819 south and #120918 east both lead to #120919? | Confirm the intended junction and reciprocal directions together. | [WLD lines 8707 and 10313](../../../areas/wld/surfacekeeps.wld#L8707) |
| 4 | Should #120826 south and #120925 east both lead to #120926? | Confirm the intended junction and reciprocal directions together. | [WLD lines 8866 and 10410](../../../areas/wld/surfacekeeps.wld#L8866) |
| 5 | Which of the 76 Alatorin arrivals and 74 departures are intentionally one-way? | Review all 69 reciprocal pairs and unmatched records as authored routes; qualify live doors and safe return separately. | [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv) |
| 6 | Are both Tharn Rifts arrivals to #120000 intended to lack a WLD return? | Confirm arrival placement and the desired way back, especially while #120000’s local exits remain unresolved. | [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv) |
| 7 | Is White Harbor #55403 east to #120823 intended as a one-way arrival? | Confirm destination and return route with the White Harbor owner. | [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv) |
| 8 | Are 285 rooms outside traversal from all active WLD arrival anchors intended side regions, or are authored links missing? | Compare intended map routes and entry points; keep the count as a static graph result until movement is qualified. | [connectivity census](#active-graph-and-boundaries) |
| 9 | What does Duris3’s complete same-VNUM room set represent relative to current SurfaceKeeps? | Preserve both sources and ask the map owner to identify the inactive Duris3 copy’s lineage; its room titles all differ. | [Duris3 room #120000](../../../areas/wld/Duris3.wld#L1), [active room #120000](../../../areas/wld/surfacekeeps.wld#L1) |
| 10 | Is the sector-27 mountain area meant to block ordinary travel, and does room #121257’s stair/wall prose describe that same barrier? | Confirm the access design, guard placement, sector, and wording as one authored route decision. | [source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md#world-properties-services-and-balanced-repair-plans) |
| 11 | Is the hidden type-25 lava tube from #122947 to Alatorin #83831 meant as the intended bypass or return path? | Confirm command admission, arrival, and return expectations without treating the portal as a WLD edge. | [portal prototype](../../../areas/obj/surfacekeeps.obj#L915), [source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md#mystardala-access-and-supplied-entry-points) |
| 12 | Should the hidden, locked, pickproof chest and key #83145 remain the intended way to find the crystal ball? | Confirm source reveal, key placement, cap, and the expected SEARCH/UNLOCK/OPEN/GET sequence. | [Mystardala access review](../zone-stories/THE_DEPTHS_OF_DURIS.md#mystardala-access-and-supplied-entry-points) |
| 13 | Are the unplaced guard #120000, captain #120001, and Gurk #120054 obsolete or missing encounters? | Confirm each prototype’s intended placement and reset behavior before adding any placement. | [mobile prototypes](../../../areas/mob/surfacekeeps.mob#L1), [source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md#world-properties-services-and-balanced-repair-plans) |
| 14 | Should blue glasses #120016 and #120054 retain prose that says they are pink? | Confirm intended copy, then make a wording-only repair if authorized; preserve item values and affects. | [object #120016](../../../areas/obj/surfacekeeps.obj#L245), [object #120054](../../../areas/obj/surfacekeeps.obj#L876) |
| 15 | Are Mystardala’s “kill while wielding the dagger” references meant as flavor, optional history, or required progression? | Choose the intended outcome and author an explicit accepted predicate only if the builder wants that requirement. | [QA115](../../../areas/qst/surfacekeeps.qst#L115), [source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md#mystardala-access-and-supplied-entry-points) |
| 16 | Should Borik’s fate be a separate player outcome from the lost-band return? | Decide whether a rescue/fate terminal is wanted; do not imply one from QA536’s item turn-in. | [QA536](../../../areas/qst/surfacekeeps.qst#L536) |
| 17 | Should the patrol disputes have explicit resolution outcomes? | Identify intended choices and replay/actor rules before adding state; dialogue alone has no accepted terminal. | [source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md#glendarla-gulranor-and-other-continuations) |
| 18 | Are the T records on objects #120040 and #120041 hazards only, or should successful trap handling have an accepted outcome? | Review trigger, damage, charges, and actor/room effects separately from any new quest or achievement. | [trap objects](../../../areas/obj/surfacekeeps.obj#L638), [source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md#world-properties-services-and-balanced-repair-plans) |
| 19 | What are the intended ownership and persistence rules for osquip death, corpse-to-object conversion, CARVE parts, and fresh-to-rotting decay? | Keep issuance, mutation, publication, decay, actor, and UID history distinct; document rejection, restart, and replay behavior before qualifying outcomes. | [procedure assignments](../../../src/specs/specs.assign.c#L2628), [source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md#qualification-and-preservation) |
| 20 | How should the coin-only provision service and nominal item/coin/XP rewards settle under active accounting? | Define supported atomic debit/reward publication and recovery rules before enabling credit; retain declared contract amounts as source data. | [QA22](../../../areas/qst/surfacekeeps.qst#L22), [source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md#exact-native-progression) |

## Retention

Keep active `surfacekeeps` **DO NOT MOVE**. Preserve its six unresolved exits, active boundaries, Alatorin item transport, all 1,457 source-only inactive approaches on candidate rows 36, 81, and 122, the inactive Duris3 same-VNUM room set, the `surfacekeeps_objects.obj` stub identity on candidate row 100, and the exact typed imports/consumers above. Preserve the ten local shops and their external stock identities. No WLD, prototype, quest, or placement repair is made here; gameplay, accounting, and migration remain separate owner decisions. See [coverage row 287](ZONE_COVERAGE_REGISTER.csv#L287), [the comprehensive source dossier](../zone-stories/THE_DEPTHS_OF_DURIS.md), and the [attic migration plan](ATTIC_MIGRATION_PLAN.md#checkpoint-374-surfacekeeps-in-place-retention-and-typed-dependencies).

### Checkpoint 755: Duris title families and retained source identity

A normalized comparison of all 442 current .zon titles finds one exact “The Depths of Duris” owner: active surfacekeeps, listed at *1200–1238 in areas/AREA and *1200 in areas/qst/AREA. Other Duris-worded title files have distinct identities: connectorzones, “The Great Realm of Duris”; newbie, “Ailvio, Duris Newbie Outpost”; surface, “The Surface Realm of Duris”; and the unlisted surf, ud, and underworld2 title files. None is an alternate title for this contributor. The local SurfaceKeeps QST’s line 546 explicitly clarifies “Storm Port Stronghold”; it names a separate area relationship and does not create another Depths of Duris map.

The significant source identity is the unregistered Duris3.wld candidate. It declares all 2,645 room VNUMs used by active surfacekeeps, but none of the paired room titles matches. It has no current .zon title or areas/AREA entry. The room-number overlap does not establish that Duris3 is a predecessor or replacement; its map-owner lineage remains the question recorded in builder question 9. Preserve both sources and keep Duris3 under candidate row 36, RETAIN IN PLACE, with its existing route and dependency classifications.

The unregistered surfacekeeps_objects.obj is a different, object-only source: 54 VNUMs overlap active surfacekeeps.obj, with 39 identical prototype bodies and 15 different bodies. Its history records stub loading in 2010; it is not another map or a whole-zone replacement. Preserve it under candidate row 100, RETAIN IN PLACE. No candidate-specific plan changes here.

Static title, tracked-history, same-number, and source-name classification is complete. Keep active surfacekeeps DO NOT MOVE. The unresolved Duris3 lineage and command-target alias/owner decisions remain explicitly assigned to the area owner rather than inferred from matching VNUMs.

## Checkpoint 777: Breale item consumers

Active SurfaceKeeps ZON lines 180, 341, 449, and 567–569 import Breale object #2679; lines 500–501 import #2658 and #2657. SurfaceKeeps shop products at lines 199–200 also name #2658 and #2657. Preserve Breale object ownership and both source consumer types; see the [Breale typed-dependency audit](BREALE_AUDIT_REVIEW.md#inbound-active-consumers).

## Bloodstone Checkpoint 778: reset and shop consumers

Active SurfaceKeeps ZON and shop #120010 use Bloodstone objects #74209–#74212. Preserve the active Bloodstone prototypes and both source consumer types; see the [Bloodstone typed-dependency audit](BS_AUDIT_REVIEW.md#inbound-active-consumers).

## Checkpoint 779: Caertannad typed dependency

Active Surface Keeps has six mobile reset rows into Caer Tannad #78438 and six object reset rows into #78414. See [Caertannad inbound consumers](CAERTANNAD_AUDIT_REVIEW.md#active-inbound-consumers).

## Checkpoint 784: Crakkaro blue-potion reset

Active surfacekeeps.zon line 561 has one G reset for Crakkaro object #87074
(the blue potion). Preserve Crakkaro as the prototype owner; this source
declaration does not prove shop stock, reset admission, or current availability.
See the [Crakkaro typed audit](CRAKKARO_AUDIT_REVIEW.md#active-and-candidate-inbound-consumers).


## Checkpoint 829: current-source dependency and graph reconciliation

The current `areas/` and `src/` trees still match pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. The 2,645-room active map, its nine weak components, six unresolved Duris3 targets, 79 inbound / 74 outbound / 69 reciprocated boundary records, and 20 builder questions remain as recorded above. No native source or gameplay state changed.

A fresh exact reset-role parse reproduces 35 mobile reset records over eight imported mobile VNUMs and 135 object reset records. It corrects the earlier distinct external object count from 74 to 79: five `O` placement operands were absent from the prior unique-ID total—Heavens objects #70, #193, and #434; Kobold object #1460; and Connector Zones object #53624. Each resolves to a current active object prototype. Source examples: [Heavens #434](../../../areas/zon/surfacekeeps.zon#L11), [Heavens #193](../../../areas/zon/surfacekeeps.zon#L16), [Kobold #1460](../../../areas/zon/surfacekeeps.zon#L20), [Connector Zones #53624](../../../areas/zon/surfacekeeps.zon#L23), and [Heavens #70](../../../areas/zon/surfacekeeps.zon#L22). The object-role correction does not change the 135 reset-record count or the WLD graph.

The current outgoing QST parse still finds 25 item-command roles over 14 external object VNUMs. The local story sidecar still has 11 `item_vnums` entries outside the local prototype sets. Local resets still import eight mobile VNUMs and 79 object VNUMs; the local SHP file has ten shop records and 49 product rows, of which 46 name outside object owners and three name local objects. Local object #120057 remains a type-25 route to Alatorin #83831. The maintained code references remain those listed in Checkpoint 374.

Outside references remain bounded: active Alatorin QST contributes 14 item-command roles (13 G, one R) across six local object VNUMs; its sidecar contributes eight matching `item_vnums` fields and eight distinct completion signatures across the same six objects. Active Alatorin ZON contributes 53 mobile reset roles for #120037/#120051 and 11 object reset roles for #120052/#120053. No external QST giver header matches a local mobile, no other mobile prototype file duplicates a local mobile, no external SHP product targets a local object, no external type-25 or type-29 object targets a local room, and no external WLD key field uses a local object. The one unregistered Brass Old 1 G reset of object #120047 remains source-only at [line 549](../../../areas/zon/brass-old-1.zon#L549); preserve it with its existing candidate identity. All 232 story sidecars parse without errors. Previously recorded Breale, Bloodstone, Caertannad, and Crakkaro source dependencies remain in force.

The static WLD census remains 9,452 authored / 9,446 resolved directions, including 9,372 internal and 74 outbound; all 79 inbound records and 69 reciprocated records remain. The six positive unresolved targets are #129900, #120099, #120919 twice, and #120926 twice, all present only in Duris3 source; see [unresolved-register rows 321–326](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L321). Traversal from 76 distinct inbound rooms reaches 2,360/2,645 rooms; reverse traversal from 74 outbound sources includes 2,644 rooms, while #120000 has no static path to an outbound WLD edge. The full inactive-WLD census remains 1,457 non-overlapping source-only approaches (Duris3 1,451, oldmap8 five, ygg one) plus 9,131 matches from source VNUMs already owned by active rooms. The title/history findings from Checkpoint 755 and the separate `surfacekeeps_objects.obj` stub identity remain unchanged.

Keep active `surfacekeeps` **DO NOT MOVE**. Preserve Duris3, oldmap8, and ygg on existing candidate rows 36, 81, and 122; preserve the object-stub source on candidate row 100 and Brass Old 1 on its own existing row. No candidate row, source route, or migration disposition changes at CP829.