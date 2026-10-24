# Storm Port repository-wide source audit

This current-source supplement reconciles the [Storm Port source-comprehensive dossier](../zone-stories/STORMPORT.md) with active map boundaries, inactive-WLD approaches, typed references, compiled numeric contexts, normalized title, and bounded same-stem history. It uses the pinned source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. No gameplay or area source is changed by this audit.

## Active map and boundary exits

The active Storm Port WLD has 147 rooms and 320 authored exits: 314 local and six outbound cross-area records. The loader resolves 319/320; the one unresolved positive target is Storm Port #22541 east to #144664 in Duris3, which is omitted by the normal build and pruned by renum_world ([unresolved exit register row 54](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L54)). There are three local weak components sized 139, 7, and 1. Authored directed traversal from #22400 reaches 137 rooms and has a return path from 139; no raw flag-8 exit changes those counts. The old ship rooms and their connecting gaps account for much of the unrooted portion, as documented in the dossier. These are static source-graph results, not proof of current availability, safe arrival, transport state, or playable access.

Four Surface boundaries reciprocate: Storm Port #22401 south to #587928, #22401 west to #587527, #22432 east to #587529, and #22479 north to #587128. The matching reverse exits are recorded in the [active pair summary rows 456 and 599](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L456). Two additional active inbound records are one-way source connections: Storm Port Stronghold #22649 down to Storm Port #22423 (pair summary row 452), and Winterhaven #55402 north to #22424 (row 815). Storm Port #22541 also exits down to Sarmiz #9597 without a reciprocal WLD edge (row 455). The cross-area register already records each authored source line and target ([Storm Port outbound rows 365–369](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L365), [Stronghold inbound row 377](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L377), [Winterhaven inbound row 750](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L750)).

## Inactive-WLD approaches

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues. It found no inactive source-room VNUM overlap with Storm Port and four non-overlapping exits from surf.wld into active Storm Port rooms:

| Source-only Surf exit | Evidence |
| --- | --- |
| #274843 south to Storm Port #22479 | [surf.wld exit](../../../areas/wld/surf.wld#L1570543) |
| #275142 east to Storm Port #22401 | [surf.wld exit](../../../areas/wld/surf.wld#L1576818) |
| #275144 west to Storm Port #22432 | [surf.wld exit](../../../areas/wld/surf.wld#L1576868) |
| #275443 north to Storm Port #22401 | [surf.wld exit](../../../areas/wld/surf.wld#L1583119) |

These records are source-only and do not add active ingress or establish a replacement map. Curated source-only totals increase from 3,328 to 3,332 overall; the reviewed Surf subset increases from 159 to 163, while the reviewed ud_map2 subset stays 15. Keep surf and surface2011 RETAIN IN PLACE. See the [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) and [coverage rows 284 and 286](ZONE_COVERAGE_REGISTER.csv#L284).

## Typed references and compiled numeric contexts

The active and unregistered exact-reference scans found no external QST item command, SHP product, STORY item field, or ZON reset-role reference to a Storm Port room, mobile, or object, except the active Tchan reset in tchan.zon: M mobile #22428 in room #4809 ([reset source](../../../areas/zon/tchan.zon#L19)). Storm Port's local #22428 is the Tchan mobile with clear_epic_task_spec assigned in compiled code ([special-procedure assignment](../../../src/specs/specs.assign.c#L194)); that is a foreign physical stock location tied to the dossier's separate quest-home context, not local reset stock.

Storm Port's own reset and shop sources do import prototypes outside its 43 local objects. Twelve reset-role references are known external objects: four direct room O rows and eight G rows; the SHP source lists six imported products at shop #22442 and two at #22445 ([ZON O row](../../../areas/zon/stormport.zon#L265), [O rows](../../../areas/zon/stormport.zon#L270), [O row](../../../areas/zon/stormport.zon#L272), [O row](../../../areas/zon/stormport.zon#L273), [G rows](../../../areas/zon/stormport.zon#L344), [later G rows](../../../areas/zon/stormport.zon#L385), [shop #22442](../../../areas/shp/stormport.shp#L40), [shop #22445](../../../areas/shp/stormport.shp#L129)). The local story sidecar has no external item_vnums. Across the other 231 story sidecars, all parse without errors and none contains an exact numeric token matching Storm Port's local room, mobile, or object VNUMs. No external local-prototype duplicate definitions were found, and none of 870 external type-25 declarations targets a Storm Port room.

The bounded C/C++ numeric scan found contextual uses rather than untyped object collisions:

- Mobile #22436 is in the SKILL_FIX epic-skill registry entry for the Storm Port smith ([epic_skills.c](../../../src/classes/epic_skills.c#L196)).
- Room #22410 is a CHAOS port destination with the aliases sp, storm, storm_port, and stormport; chaos_port resolves that value as a room and attempts a character move ([port table](../../../src/combat/chaos.c#L30), [destination resolution](../../../src/combat/chaos.c#L64), [room movement](../../../src/combat/chaos.c#L81)).
- Mobile #22428 receives clear_epic_task_spec. Room #22439 receives the inn callback. The mobile assignment tables also explicitly clear default special procedures on Storm Port mobiles #22467, #22434, #22415, #22435, #22420, #22441, and #22440; the same numeric #22441 is also a room with the ship-shop callback, so the mobile and room namespaces remain separate ([assignments](../../../src/specs/specs.assign.c#L194), [inn](../../../src/specs/specs.assign.c#L296), [teacher tables](../../../src/specs/specs.assign.c#L424), [ship shop](../../../src/specs/specs.assign.c#L2383)).
- Room #22481 is configured as the crew-shop room and appears in the ship crew-shop tables ([room binding](../../../src/specs/specs.assign.c#L2399), [crew-shop list](../../../src/ships/ship_shop.c#L3239), [ship configuration](../../../src/ships/ship_variables.c#L222)).
- The ship stop table names Storm Port room #22441 and Surface #587128. The modern ferry tables separately name rooms #22444 for Seaspray and #22445 for Stromvok; those code-defined services are not WLD exits ([ship stop](../../../src/ships/ship_variables.c#L44), [Seaspray](../../../src/world/ferryact.c#L109), [Stromvok](../../../src/world/ferryact.c#L135)).

The imported reset/shop rows, transport configurations, and compiled bindings are dependencies to retain during any future source disposition. Their presence does not qualify active accounting, settlement, save/recovery, travel completion, or service outcomes.

## Title, history, and disposition

The color-stripped active title “Storm Port” is unique among 442 scanned .zon files. The lexical scan also surfaced Storm Port Stronghold (spshold), Strathor Valley of the Storm Giants (stormht), and The Ruins of Port Skythic (portskythic); these are separate active titles and source keys, not duplicate title owners. The normalized title is from [stormport.zon](../../../areas/zon/stormport.zon#L2).

Followed stormport.wld history reaches the 2009 directory relocation, 2011 and 2013 edits; followed stormport.zon history reaches the same relocation and includes Tchan/spec-teacher edits, the 2015 quest-item-flag update, and the 2016 granite material fix. These bounded same-stem paths show no alternate-stem rename, explicit predecessor, or whole-zone replacement. The source-only Surf approaches and active boundaries are map evidence, not replacement evidence.

Keep active stormport DO NOT MOVE. Keep surf and surface2011 RETAIN IN PLACE. The zone's existing source dossier and this audit do not clear a source-path move, gameplay claim, or operational change. The source-mapping queue is now 194/220 complete, with 26 pending; Priority 195, Arachdrathos (arac-web), is next.

Validation: Checkpoint 154 documentation validation checked 130 changed/added Markdown files; all 5,009 local link targets and 2,351 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. No tests, build, server, gameplay session, database operation, migration, or operational script was run.


## Checkpoint 371 builder-question coverage

Physical coverage line 284, `area:stormport`, already had the source-comprehensive map, active and source-only route reconciliation, typed-reference closure, title review, and bounded history. This checkpoint completes the coverage register's builder-question review with twenty conservative, source-backed recommendations. The [Storm Port story map](../zone-stories/STORMPORT.md) and this [repository-wide audit](STORMPORT_AUDIT_REVIEW.md) remain the detailed evidence.

### Builder questions and conservative recommendations

1. Should `stormport` remain the active owner of its rooms, prototypes, resets, quests, shops, and ship-service configuration? Keep it **DO NOT MOVE**; review any proposed owner change against these maintained bindings.
2. Should the authored exit from #22541 east to Duris3 #144664 remain flagged for target-owner resolution? Preserve the source direction and unresolved-register record; resolve the Duris3 identity before proposing a redirect or removal.
3. Should the four reciprocal Surface pairs remain Storm Port's active WLD boundaries? Preserve all four authored pairs and their reverse directions; qualify actual access and safe return separately.
4. Should the Stronghold and Winterhaven inbounds, plus Storm Port's Sarmiz outbound, keep their current one-way directions? Preserve their source ownership and do not infer a return edge from proximity or prose.
5. Should the four inactive Surf exits to #22479, #22401, and #22432 remain source-only? Keep each exact edge on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); these are not active ingress and do not establish a replacement map.
6. Should Tchan's four exact loose proofs and the citysmith's granite-plus-straps bundle remain independent QST exchanges? Preserve the two native contracts and rewards; do not add turn-in order, personal kill, carve, or greeting requirements.
7. Should Tchan's empty Q response continue to issue its authored mantle reward? Preserve accepted settlement and reward identity; review any clarity text as a separate source change.
8. Should proof recovery, encounter, hidden SEARCH, opening the parent, GET, and accepted turn-in remain separate player evidence? Preserve exact object roots, current custody and NORENT flags; supplied matching materials remain eligible without invented source-history prerequisites.
9. Should Tchan's physical stock and quest-home identity remain distinct? Preserve `tchan.zon`'s reset at monastery room #4809, the `clear_epic_task_spec` assignment, and quest home #224; qualify player discovery without adding duplicate local stock.
10. Should the old frigate remain without an ordinary authored inbound route while builders resolve access to its captain and badge? Preserve the bounded route finding and exact source records; do not invent an entrance or free badge source.
11. Should the obsolete boat, ladder, navigator, schedule, and ticket remain separate from the modern ferry callbacks? Preserve their VNUMs and current reset evidence; distinguish the old UP ladder target from actual boarding and arrival.
12. What evidence should establish a modern ferry trip? Require the matching current ticket, boarded passenger and leg, boat movement, and actual disembark placement; a ticket, announcement, or lookout view does not establish arrival.
13. Should ferry ticket BUY remain unavailable under active accounting? Keep the current refusal until fee settlement, ticket issuance, recipient identity, replay, refund, and durable save can commit together.
14. Should the ferry ETA label remain a separate clarity issue? The function returns minutes while the automat labels the value as hours; track any correction separately and preserve route, fare, and timetable behavior.
15. Should citysmith repair consume exactly one matching material? Keep the source finding that the final loop extracts every matching loose item; make any balanced one-item repair separately and qualify both success and failure settlement.
16. Should ship, crew, shops, and imported stock create Storm Port story credit? Preserve their separate owner, location, race, fee, docking, maintenance, and item identities; the eight imported shop products remain foreign prototypes.
17. Should inn service continue at dock #22439 while #22486 carries the `ROOM_INN` flag and inn prose? Confirm the intended service location and qualify flag dispatch and save/rollback before changing service behavior.
18. Should the torch and red-book controls keep their current verbs and target states? Preserve the torch's missing `EX_BLOCKED` condition and the book's room-text PUSH versus registered PULL discrepancy until a builder chooses the intended barrier, command, and prose.
19. Should entering the waterwheel's type-25 DOWN portal count as restoring or operating the wheel? Keep the route and destination separate from a repair outcome; record any wheel action only after an owner defines and qualifies a real controller.
20. Should prisoner release, safe arrival, religious investigation, guild learning, family aid, and city defense receive persistent outcomes? Keep their current story leads descriptive until builders define recipients, actor contribution, completion conditions, and durable results; keep the single potential daily disabled until its sources and renewal are qualified.

These recommendations complete the builder-question review; owner decisions remain open. The four Surf approaches are already represented on candidate row 97, so this checkpoint adds no route record and changes no source behavior. This review is documentation-only; no build, test, runtime, database, or migration action was performed.

## Checkpoint 826 current-source reconciliation

The checked-out `areas/` and `src/` content still matches pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. This checkpoint rechecks Storm Port's active and source-only graph, typed references, duplicate prototypes, title, and bounded history. It adds no gameplay or installed-world claim.

### Active and source-only WLD closure

The active WLD still has 147 rooms and 320 authored exits: 314 local and six outward records. The full loader resolves 319; #22541 east to Duris3 #144664 remains the sole unresolved positive target ([source exit](../../../areas/wld/stormport.wld#L2861), [unresolved register row 54](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L54)). Storm Port #22541 also exits down to active Sarmiz #9597; the other four outward records form reciprocal Surface pairs. Active inbound WLD records remain the four matching Surface returns plus one-way connections from Stronghold #22649 and Winterhaven #55402. See [active exits rows 365–369](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L365), [inbound rows 377, 750, and 1539–1545](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L377), and the [pair summary rows 452, 455, 456, 599, and 815](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L452).

The local graph remains three weak components of 139, 7, and 1 rooms. Directed traversal from #22400 reaches 137 local rooms; 139 have a directed path back to it. The current 95-file inactive-WLD parse again covers 527,791 rooms and 2,080,773 exits with no parse issues or Storm Port room-VNUM overlap. Its only Storm Port targets are four non-overlapping exits in `surf.wld`: #274843 south to #22479, #275142 east to #22401, #275144 west to #22432, and #275443 north to #22401 ([source records](../../../areas/wld/surf.wld#L1570543), [#1576818](../../../areas/wld/surf.wld#L1576818), [#1576868](../../../areas/wld/surf.wld#L1576868), [#1583119](../../../areas/wld/surf.wld#L1583119)). A repository-wide scan of all 448 `.wld` files finds only the six active inbound records and these four Surf records targeting Storm Port; the two nonstandard WLD paths outside `areas/wld` contain no exact room or target match. Preserve Surf as source-only, not active ingress.

### Typed references, prototypes, and code bindings

Across active and unregistered QST, SHP, STORY, and typed ZON sources—including the nonstandard mini sidecars—there is no external exact QST item, shop product, story VNUM, or reset-role reference to Storm Port prototypes except active `tchan.zon` mobile #22428 at monastery room #4809 ([reset](../../../areas/zon/tchan.zon#L19)). The 232 story sidecars parse; none of the other 231 contains an exact numeric token matching a local Storm Port room, mobile, or object. Storm Port's own ZON imports 12 external object reset roles (four direct room `O` rows and eight `G` rows), and its shops list eight imported products (six at #22442, two at #22445). These are distinct foreign stock dependencies, not local prototype duplicates.

The repository-wide object scan covers 443 `.obj` files and 22,483 prototypes, including `areas_mini/mini.obj`. It finds no external same-type room, mobile, or object definition overlapping Storm Port. Across the canonical `areas/obj` sources, 872 type-25 objects include two local portals and 870 external declarations; the mini source adds one more external type-25 object. None of these 871 external declarations targets a Storm Port room. The 381 type-29 switch declarations, including the mini source, have no `value[1]` target in Storm Port, and the zone owns no local type-29 switch. A repository-wide WLD exit-key scan also finds no use of a Storm Port object VNUM as an exit key.

The bounded compiled-number scan reproduces the existing typed bindings: mobile #22436 in the SKILL_FIX table; CHAOS room #22410; `clear_epic_task_spec` on mobile #22428; the inn at #22439; explicit default-procedure clears on mobiles #22467, #22434, #22415, #22435, #22420, #22441, and #22440; the ship shop at room #22441; crew shop at #22481; and the configured ship/ferry stops #22441, #22444, and #22445. The mobile and room namespaces remain separate where a VNUM is shared. These references remain dependencies to retain; they do not establish service settlement, availability, or a completed journey.

### Identity and disposition

The normalized title “Storm Port” remains unique among the 442 current `.zon` titles. Followed same-stem WLD and ZON history still shows no alternate-stem rename, explicit predecessor, or whole-zone replacement. The twenty source-backed builder questions and conservative recommendations recorded at [Checkpoint 371](#checkpoint-371-builder-question-coverage) remain current.

Keep active `stormport` **DO NOT MOVE**. Preserve the four Surf approaches on existing [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and preserve the distinct active-to-inactive Duris3 target on [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36). Neither direction authorizes map activation, reassignment, or a path move. Runtime, database, editor, and operational closure remain separate. No native source, build, test, game/server, database, migration, or operational action was performed.
