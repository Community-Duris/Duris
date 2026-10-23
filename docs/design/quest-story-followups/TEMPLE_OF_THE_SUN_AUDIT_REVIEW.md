# The Temple of the Sun: Surface boundary, source-only approach and typed dependencies

**Checkpoint 116 · 2026-10-09 · active source `suntmpl`, zone 824.** This repository-wide supplement adds graph, inactive-map, typed-dependency, title/history, and retention evidence to the [comprehensive Temple of the Sun story dossier](../zone-stories/TEMPLE_OF_THE_SUN.md). It does not change gameplay or move source files.

Sources: [`areas/AREA`](../../../areas/AREA#L296), [`suntmpl.wld`](../../../areas/wld/suntmpl.wld#L1734), [`suntmpl.zon`](../../../areas/zon/suntmpl.zon#L2), [`suntmpl.obj`](../../../areas/obj/suntmpl.obj#L83), [`suntmpl.qst`](../../../areas/qst/suntmpl.qst#L1), [active cross-area exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L988), [cross-area pair summaries](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L457), and the [coverage row](ZONE_COVERAGE_REGISTER.csv#L285).

## Active connectivity and directed source reach

The active `suntmpl.wld` defines 94 rooms and 217 exits. All 217 targets resolve in the active WLD set; 216 exits stay within the Temple and one leaves it. Its local undirected projection is one weak component containing all 94 rooms. The active graph has no negative or unresolved targets, numeric-range mismatches, fall markers, or current fields.

The Temple entrance #82493 west exits to Surface room #541275; Surface #541275 east returns to #82493. This is one reciprocal active pair, with one inbound, one outbound, and one reverse record. See [Temple outbound row 988](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L988), [Surface inbound row 1415](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1415), [Temple-to-Surface pair row 457](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L457), and [Surface-to-Temple pair row 600](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L600).

With every authored local exit treated as available, directed traversal from #82493 reaches all 94 rooms and every room has an authored path back. The story dossier records stricter authored-access layers: 51 rooms without SEARCH or keys, 54 with SEARCH, 88 with the vine key, 89 with the cellar key too, 90 with all three keys, and 94 when the separate locked wing is opened by a successful skilled PICK. These counts assume the described actor actions and key availability; they do not establish live access, key destruction, reset stock, hazard outcomes, or safe travel. See the dossier's [physical entrance and inner-temple progression](../zone-stories/TEMPLE_OF_THE_SUN.md).

## Inactive-map approach

All 95 WLD files outside the active `areas/AREA` contributor set were scanned: 527,791 rooms and 2,080,773 exits, with no parse issues or source-room VNUM overlap with the Temple. One source-only approach was found: inactive [`surf.wld` #237595 west to #82493](../../../areas/wld/surf.wld#L788759). `surf` is not selected by `areas/AREA`, so this is not active ingress.

This adds one record to the reviewed Surf subset, raising it from 140 to 141, and raises the curated source-only approach total from 1,529 to 1,530. Preserve the edge with its inactive map identity; keep `surf` and `surface2011` **RETAIN** pending map ownership and dependency/tool closure. This scan covers WLD exits only; code, transport, staff, and runtime arrival paths were not qualified.

## Typed references and dependency closure

The bounded typed scans found these direct relationships:

| Reference | Source record | Classification |
| --- | --- | --- |
| Active reset imports | [`suntmpl.zon:161–162`](../../../areas/zon/suntmpl.zon#L161) loads imported #72 spell pool and #359 epic stone. Later reset records load imported herb objects #821, #822, #823, and #826; [`suntmpl.zon:360`](../../../areas/zon/suntmpl.zon#L360) loads music box #67253. | Real active dependencies on Heavens prototypes: #72 ([spell pool](../../../areas/obj/heavens.obj#L1000)), #359 ([epic stone](../../../areas/obj/heavens.obj#L3725)), and #821/#822/#823/#826 ([herb family](../../../areas/obj/heavens.obj#L7703)); #67253 belongs to `unique.obj` ([prototype](../../../areas/obj/unique.obj#L816)). The source-comprehensive dossier maps the plant placement counts and the effective epic/music handlers. These are stock and service dependencies, not additional native Temple quest cards. |
| Active outside quest consumer | [`wh.qst:922`](../../../areas/qst/wh.qst#L922) consumes object #82405, the Temple's enchanted ring of stone ([prototype](../../../areas/obj/suntmpl.obj#L83)); the matching [`wh.story.json` request](../../../areas/story/wh.story.json#L902) names Enfil's three-item exchange. | One real active Winterhaven consumer of the Sun ring, with a corresponding story-sidecar reference. It is an independent outside receipt; it does not make Winterhaven a prerequisite for Grentkas or the whole WH campaign newly comprehensive. |
| Unregistered reset consumer | [`brass-old-1.zon:520`](../../../areas/zon/brass-old-1.zon#L520) has a G reset for Temple object #82416 ([prototype](../../../areas/obj/suntmpl.obj#L276)). | `brass-old-1` is outside the active loader list. This is a source-only reference and does not create active stock. Preserve the candidate pending its existing full source, shop, history, tooling, and dependency closure; see [candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17). |
| Direct code binding | [`epic_skills.c:216`](../../../src/classes/epic_skills.c#L216) binds mobile #82408, Frolikk, to `SKILL_EPIC_CHARISMA`. | A real custom epic-teacher/service binding. The dossier records that active accounting refuses the purchase; this table match is not a runnable fourth Temple Q/QA exchange or proof of a paid skill award. |

No external active QST item command other than the Winterhaven #82405 consumer, SHP product, active reset-role consumer, external MOB/OBJ prototype duplicate, or incoming type-25 target to a Temple room was found. No local `.shp` file exists. All 231 external story sidecars parsed; the only typed `item_vnums` hit is the corresponding Winterhaven ring request, whose completion key produces three raw #82405 token hits. Local QST item commands and local story `item_vnums` remain within Temple prototypes. Among 872 external type-25 declarations, none targets a Temple room. These are bounded direct scans, not exhaustive runtime, editor/tool, indirect-code, or persisted-player closure.

## Title, duplicate, and replacement history

After Duris color-code removal and normalization, “The Temple of the Sun” has one exact owner among 351 non-empty active `.zon` titles: `suntmpl`. The title scan found other Temple-named areas, including Temple of the Earth, Temple of Flames, Temple of Laduguer, and Temple to Skrentherlog, but each has a different normalized title and source stem. The Temple's `.zon` title is at [line 2](../../../areas/zon/suntmpl.zon#L2).

Followed `suntmpl.wld`, `.zon`, `.mob`, `.obj`, and `.qst` histories remain under the same stem after the 2009 repository-wide `areas/` move (`642da5400`). The reviewed history includes 2011–2013 same-stem edits, 2015 quest-item-flag updates (`f18992b8f`), a 2016 object-number correction (`20f072004`), and 2026 refactor/line-ending commits (`0bae49b7c`, `428ebfddf`). The story sidecar was added on 2026-10-06 (`74d3e1b51`). No alternate-stem rename, distinct predecessor, or explicit replacement claim appears in these followed paths; this does not establish the absence of an untracked predecessor.

Preserve active `suntmpl` as its own current identity. Keep it **DO NOT MOVE** as a current, un-commented `areas/AREA` contributor. Preserve the source-only `surf` approach and retain `brass-old-1` pending their independent ownership and closure reviews.

## Scope and remaining qualification

The [coverage row](ZONE_COVERAGE_REGISTER.csv#L285) links this supplement and the comprehensive story dossier. It now records the static graph, single reciprocal Surface pair, inactive Surf approach, one external Winterhaven recipe consumer, reset imports, title/history result, and retention disposition. Gameplay qualification, dynamic arrival, successful actor access, disposable-key timing/recovery, custom-service settlement, route safety, tooling/editor references, indirect code paths, and persisted-player references remain outside this static checkpoint. No source, area placement, or gameplay behavior changed.


## Checkpoint 372 builder-question coverage

Physical coverage line 285, `area:suntmpl`, already had its source-comprehensive story map, active graph, typed dependencies, title/history, and retention review. This checkpoint completes the builder-question review by turning its twenty source-backed follow-ups into explicit questions with conservative recommendations. The [Temple of the Sun story map](../zone-stories/TEMPLE_OF_THE_SUN.md) and [Checkpoint 116 audit](TEMPLE_OF_THE_SUN_AUDIT_REVIEW.md) contain the source evidence.

### Builder questions and conservative recommendations

1. Should `suntmpl` remain the active owner of its rooms, mobs, objects, resets, three QST exchanges, and sidecar? Keep it **DO NOT MOVE**; preserve its independent source identity and maintained imported-service bindings.
2. Should the entrance at #82493 west and Surface #541275 east remain the Temple's reciprocal active boundary? Preserve the exact pair and authored interior; qualify search, keys, successful passage, and return separately.
3. Should inactive [Surf #237595 west to #82493](../../../areas/wld/surf.wld#L788759) remain source-only? Keep its exact map identity on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); it does not add active ingress.
4. Should the unregistered `brass-old-1.zon` G reset for local object #82416 remain source-only? Preserve the reference on existing [candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17); it is not active Temple stock.
5. Should Trentloss, the oak spirit, and Grentkas retain three separate item-only exchanges? Preserve each exact giver, required item multiset, reward, and continuing giver; do not combine them into a single stage chain.
6. Should supplied matching materials remain eligible without the player's own earlier exchange, source discovery, or personal kill? Keep current exact-item contracts separate from any future actor/source episode.
7. Should the hidden cap-1 source placements and NORENT material identities remain as authored? Preserve source and parent relationships; qualify current root custody, visibility, transfer, depletion, and renewal before promising supply.
8. Should SEARCH and ordinary opening remain the authored way to reveal the pine, oak, and bear passages? Preserve the D5 state and require an actual actor action only where the builder defines it as an outcome; a shared open door is not a personal receipt.
9. Should the vine, cellar, and bright keys retain their three distinct door paths and break semantics? Preserve exact key/door identities while qualifying the asynchronous key-destruction and reciprocal-door sequence under failure, concurrency, and recovery.
10. Is the locked wing #82418/#82437 intended to be reached by successful skilled PICK? Keep key0 and the current lock traits until the builder confirms the route; do not insert a missing key or advertise container Knock as a room-door bypass.
11. Should the authored access tiers of 51/54/88/89/90/94 rooms be used as map guidance? Preserve them as source graph counts with their SEARCH, key, movement, and PICK assumptions; avoid promising live availability or safe travel from the counts.
12. Should the oak and stone-ring reward prose be clarified against the actual objects? Preserve ring #82409's type/wear and #82405's tail wear and outside recipe compatibility; make any text correction separately after intent review.
13. Should the oak's restoration dialogue imply a permanent woods-state change? Keep current accepted receipts separate from world mutation; define target, duration, authority, reset, and replay before adding a restoration outcome.
14. Should Frolikk's Epic Charisma service become a Temple story contract? Keep it a separate accounted service; the epic teacher refuses purchases during active accounting and needs its own fee, skill, and durable-result qualification.
15. Should a touch of imported epic stone #359 award Temple story credit? Preserve its existing group-award owner and readiness checks; bind any future story result to an eligible actor/group and committed outcome.
16. Should the imported spell pool #72 count a DRINK command as a selected buff? Keep service access separate from story progress; correct argument/target selection before any player-facing spell outcome is recorded.
17. Should collecting planted herbs count as personal foraging? Preserve the active-accounting refusal and distinguish stored stock, floor pickup, transfer, and an admitted harvest action.
18. Should Winterhaven and the other seven selected outside recipes create Temple prerequisites? Preserve their exact foreign giver and item ownership; keep local ring/herb compatibility without adding an order gate or implying full outside-campaign review.
19. Should the eleven aliases, periodic song, lyre/music box, and research tome define additional Temple achievements? Preserve actor-addressed conversations and typed targets; require an actual delivered response, performance, or research result before recording one.
20. Should the three repeatable returns remain candidates for a daily under reset mode 1? Preserve the source contracts and cap rules; require active ready accounting, eligible giver/source generations, renewable supplies, and committed receipt recovery before enabling any daily policy.

These recommendations complete the builder-question review; owner decisions remain open. The known Surf approach and `brass-old-1` reset remain on their existing candidate rows, so this checkpoint adds no candidate record and changes no source behavior. The builder review is documentation-only; no tests, build, runtime, database, or migration action was performed.

## Checkpoint 827 current-source reconciliation

The checked-out `areas/` and `src/` content still matches pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. This checkpoint rechecks the active and source-only WLD edges, reset/quest/story consumers, object targets, prototype collisions, title, and followed source history. The source-comprehensive story dossier and the twenty builder questions at [Checkpoint 372](#checkpoint-372-builder-question-coverage) remain the detailed story map and recommendations.

### Active and source-only WLD closure

The active `suntmpl.wld` still defines 94 rooms and 217 exits. All 217 targets resolve in the active WLD set: 216 are local and the only outbound record is Temple #82493 west to Surface #541275 ([exit](../../../areas/wld/suntmpl.wld#L1745), [active register row 988](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L988)). Surface #541275 east returns to #82493 ([return](../../../areas/wld/surface.wld#L866469), [pair summaries 457 and 600](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L457)). The local weak graph has one component of all 94 rooms; with every authored local exit treated as available, #82493 reaches all 94 and all 94 can return. SEARCH, key, lock, and skilled-PICK requirements remain the stricter authored access layers in the [story dossier](../zone-stories/TEMPLE_OF_THE_SUN.md).

The current 95-file inactive-WLD scan again parses 527,791 rooms and 2,080,773 exits without errors or any source-room overlap with Temple rooms. Its only approach is inactive `surf.wld` #237595 west to #82493 ([source room](../../../areas/wld/surf.wld#L788759), [west exit](../../../areas/wld/surf.wld#L788775), and [target](../../../areas/wld/surf.wld#L788778)). An exact room/target scan across all repository `.wld` files finds only that Surf record and the active Surface return outside the Temple file; the two nonstandard WLD files outside `areas/wld` have no match. The approach remains source-only and adds no active ingress.

A repository-wide WLD exit-key scan finds six key-field records, all in `suntmpl.wld`: key #82420 at [lines 150/266](../../../areas/wld/suntmpl.wld#L150), #82423 at [315/619](../../../areas/wld/suntmpl.wld#L315), and #82424 at [615/634](../../../areas/wld/suntmpl.wld#L615). No external WLD exit uses a Temple object VNUM as a key. These are the existing local vine, cellar, and bright-key controls in the dossier, not external edges.

### Typed references and prototype closure

The outside exact QST and story references remain the active Winterhaven recipe: [`wh.qst` line 922](../../../areas/qst/wh.qst#L922) consumes local stone ring #82405, and [`wh.story.json` line 902](../../../areas/story/wh.story.json#L902) carries the corresponding Enfil request. All 232 story sidecars parse without errors; `wh.story.json` is the sole outside sidecar hit, and the other 230 external sidecars contain no Temple prototype token. No external SHP product or active external reset-role consumer targets a Temple prototype. The complete exact-number ZON scan—including `areas_mini/mini.zon`—finds only the source-only `brass-old-1.zon` G reset of local object #82416 at line 520 ([reset](../../../areas/zon/brass-old-1.zon#L520), [object](../../../areas/obj/suntmpl.obj#L276)); it does not create active stock. The Temple has no local SHP file.

The Temple's own active resets still import Heavens objects #72, #359, #821, #822, #823, and #826, plus music box #67253 from `unique.obj` ([Heavens reset rows](../../../areas/zon/suntmpl.zon#L161), [music-box reset](../../../areas/zon/suntmpl.zon#L360)). The only direct compiled-number binding remains `epic_skills.c` mapping mobile #82408, Frolikk, to Epic Charisma ([binding](../../../src/classes/epic_skills.c#L216)). The Temple's local story `item_vnums` stay within its object prototypes. These are concrete dependencies to preserve; they do not establish stock availability, a paid epic award, or a completed Winterhaven exchange.

The current same-type scan covers active and unregistered WLD/MOB/OBJ sources, including nonstandard files, and finds no external room, mobile, or object prototype collision with the Temple. Across all 443 `.obj` files and 22,483 object prototypes, 873 are type-25 portals and 381 are type-29 switches, including the additional declarations in `areas_mini/mini.obj`; the Temple owns neither type locally, and no external type-25 destination or type-29 `value[1]` target names a Temple room. This repository-wide count extends the earlier canonical-directory-only portal count by the Mini source.

### Title, history, and disposition

The normalized title “The Temple of the Sun” remains unique among all 442 `.zon` files and all 351 active area titles. Followed `suntmpl` WLD, ZON, MOB, OBJ, and QST histories remain under the same stem after the 2009 areas-tree move; the 2026 story sidecar is a same-stem addition. No alternate-stem rename, distinct predecessor, or explicit whole-zone replacement appears in the reviewed history.

Keep active `suntmpl` **DO NOT MOVE**. Preserve the source-only Surf approach on existing [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and retain `brass-old-1` on existing [candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17) with its unregistered #82416 reset dependency. These identities remain separate from the active Temple and from each other; this checkpoint adds no candidate or migration clearance. No native source, build, test, game/server, database, migration, or operational action was performed.
