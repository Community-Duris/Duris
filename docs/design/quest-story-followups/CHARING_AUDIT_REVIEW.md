# Charing repository-wide source audit

Checkpoint 201 audits active `charing` against the source snapshot in the [coverage register](ZONE_COVERAGE_REGISTER.csv#L55). This is a documentation-only review of files under `areas/`, compiled references under `src/`, and tracked history. It does not inspect a live server, player state, database contents, or editor state. The [source-story dossier](../zone-stories/CHARING.md) summarizes the setting; the earlier [detailed story review](CHARING_STORY_REVIEW.md) retains per-room narrative evidence and the 14-shop map.

## Inventory and authored story

The active source identity is header #450, title “Charing,” zone reset mode 2, with 210 rooms, 95 mobiles, 58 objects, 14 shops, and 496 ZON commands. There is no local QST file or native Q/QA contract. The `qst/AREA` membership is a legacy index entry. The sidecar declares `coverage: complete` and an introduction about resident requests, while its `contacts`, `stories`, and `exclusions` arrays are empty ([sidecar](../../../areas/story/charing.story.json#L1)).

The authored environment is an elven and centaur hometown with a mermaid fountain, forest approach, druid sanctuary and temples, palace and civic justice rooms, warrior/rogue/ranger/bard facilities, shops and a theatre, necromancy laboratory, city dump portal, river, and Great Tree canopy. Personal room and NPC descriptions add an oracle, magical teachers, visitors, lost or abandoned children, and captives. The [source-story review](CHARING_STORY_REVIEW.md) supplies the exact evidence anchors and full shop list.

Static local QST absence does not mean there is no quest interaction. `specs.assign.c` binds `world_quest` to mobile #45055, the ranger bartender ([binding](../../../src/specs/specs.assign.c#L766), [prototype](../../../areas/mob/charing.mob#L783)). The shared handler responds to player requests and uses `createQuestForGiverVnum`; the live offer pool is not determined by this area’s QST or story sidecar and was not inspected ([handler](../../../src/specs/specs.world_quest.c#L191)). Keep the authored-story and compiled shared-handler claims distinct.

## Active WLD connections

The Charing WLD has 467 direction slots: 464 local exits and three boundary records. There are 465 loader-resolved destinations and two positive unresolved destinations; no negative targets, range-mismatch rooms, or active room-ID overlaps were found. The local undirected exit graph has components of 208, 1, and 1 rooms. The isolated rooms are #45037 “Pet Shop” and #45151 “Stables” ([pet room](../../../areas/wld/charing.wld#L861), [stable room](../../../areas/wld/charing.wld#L3204)).

| Direction | Authored edge | Status |
| --- | --- | --- |
| Inbound | Surface #513692 east to Charing #45183 | Active and paired with Charing #45183 west to #513692 ([Surface](../../../areas/wld/surface.wld#L287517), [Charing](../../../areas/wld/charing.wld#L3917)). |
| Inbound | Surface #513693 east to Charing #45183 | Active one-way record; Charing has no second west record back to #513693 ([Surface](../../../areas/wld/surface.wld#L287538)). |
| Outbound | Charing #45183 west to Surface #513692 | Active; the single reverse match is the #513692 inbound pair ([boundary ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L622)). |
| Unresolved outbound | Charing #45185 north to #209060 | Target is absent from the active loader; it appears in `surf.wld` and `surface2011.wld` ([unresolved ledger](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L216)). |
| Unresolved outbound | Charing #45205 south to #209660 | Target is absent from the active loader; it appears in `surf.wld` and `surface2011.wld` ([unresolved ledger](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L217)). |

The full scan of all 95 unregistered WLD sources found two non-overlapping source-only approaches into Charing, both in `surf.wld`: #209060 south to #45185 and #209660 north to #45205 ([first source room](../../../areas/wld/surf.wld#L190156), [second source room](../../../areas/wld/surf.wld#L202716)). Those records mirror Charing’s unresolved targets, but they are not active entrances because `surf.wld` is not in `areas/AREA`. The same numbered `surface2011.wld` rooms do not add those reverse records. Preserve both routes on the existing Surf candidate plan. They add two records to the reviewed source-only Surf subset; the curated total is 3,447 overall, 211 for Surf, and 17 for `ud_map2`.

## Reset-aware local reach

Charing has 58 unconditional ZON `D` resets. The current `reset_zone()` implementation sets the closed/locked bits from the low two state bits and accumulates secret/blocked bits from the upper flags ([door reset handling](../../../src/world/db.c#L7857)). Applying these authored resets to WLD exits leaves 406 of 464 directed local exits open, non-secret, and non-blocked. The other 58 are 51 closed, six closed and locked, and one closed and secret.

Treating every local WLD exit as usable gives directed reach to 208 of 210 rooms from Surface arrival #45183. After the unconditional reset door states, simple open-exit reach is 40/210 from #45183, 77/210 from center #45000, and 77/210 from hometown room #45006. The reset-open local graph has 29 weak components, largest 77 and second-largest 40. These are static source calculations. They exclude keys, searching, picking, class/race rules, live reset timing, movement callbacks, saved state, and player safety, so a closed-only reach count is not a claim that the remaining rooms are inaccessible.

Three reciprocal locked-door pairs account for the six locked exit records: Shrine-to-Palace (key #45004), Queen’s chambers (key #45026), and the King’s throne room (key #45028). The Great Tree remains asymmetric: #45168 south is reset closed, while #45169 north is reset closed and secret. The jail cell door is reset closed, with key #45006 on the jailkeeper. Existing review links document the exact ZON and key placements. The 2014 route commit `227adc0f8` made the Charing-side tree exit non-hidden while leaving the outside-side entrance hidden.

## Compiled behavior and code-defined links

| Source binding | Source-level effect or limit |
| --- | --- |
| `specs.guards.c`, room #45017, north command | Blocks entry for races other than Grey, Half-Elf, and Centaur ([guard check](../../../src/specs/specs.guards.c#L249)). Charing’s home-selection row enables Grey and Centaur but not Half-Elf ([race constants](../../../src/core/defines.h#L911), [home table](../../../src/core/constant.c#L1792)); confirm this distinction with the builder. |
| `constant.c`, `hometown[]` and `guild_locations` | Uses #45006 as Charing’s hometown location and defines class-specific Charing guild arrival rooms ([home anchor](../../../src/core/constant.c#L1736), [guild map](../../../src/core/constant.c#L1962)). |
| `justice.c`, Charing hometown row | Retains justice-home and guard-room VNUM data, but `justice_send_guards()` returns `FALSE` immediately under its disabled `WIPE2013` path ([home row](../../../src/combat/justice.c#L88), [disabled path](../../../src/combat/justice.c#L122)). Do not assume that this guard-dispatch path currently spawns guards. |
| `chaos.c`, destination #45000 | The shared chaos-port table accepts the alias “charing grey” and resolves it to the active center room ([destination entry](../../../src/combat/chaos.c#L25)). This also resolves the compiled command-target alias in the title/name scan. |
| `specs.assign.c`, mobile #45049 | Has the `guild_guard` mobile procedure ([binding](../../../src/specs/specs.assign.c#L919)). |
| `specs.assign.c`, room #45006 | Has the `inn` room procedure ([binding](../../../src/specs/specs.assign.c#L2334)). |
| `specs.assign.c`, room #45036 | Has the `pet_shops` room procedure ([binding](../../../src/specs/specs.assign.c#L2335)). Its implementation reads hirelings from the next real room (`ch->in_room + 1`), which is the adjacent WLD room #45037 in the active Charing load order ([procedure](../../../src/specs/specs.room.c#L863)). This explains #45037’s isolated “Pet Shop” staging role without making it a player WLD route. |

Room #45151 remains different: it has reset-loaded mounts and no WLD exits, but the source scan found no direct compiled VNUM binding to that room. No local object is type `ITEM_TELEPORT` or `ITEM_SWITCH`. Across the current source OBJ files, none of 872 external `ITEM_TELEPORT` declarations targets a Charing room, and none of 380 `ITEM_SWITCH` declarations targets a Charing room. No reset-placed local type-25 or type-29 item creates a source-established alternate route.

## Typed inbound and outbound dependencies

Charing consumes outside object prototypes in its resets and shop stock. The previously mapped story review records the exact local reset/SHP locations and VNUMs:

| External prototype owner | Charing references |
| --- | --- |
| Active Heavens | #54, #191, #203, #204, #256, #283, #361, #363, #364, #368, #369, #376, #390, #396. |
| Active Verz1 | #3001–#3004, #3021, #3023–#3025, #3040–#3046, #3054, #3069–#3071, #3075–#3076, #3080, #3085, #3097. |
| Active Kobold | #1403. |
| Active Valley Crushk | #2020; this reset/SHP dependency is also recorded in that zone’s audit. |
| Active Plane of Fire One | #25400. |
| Active Orrak | #9017; this reset dependency is also recorded in Orrak’s audit. |
| No current OBJ definition | #3032, #3902, #5214, #12581, #12584, #18280. Preserve these unresolved numeric references until their intended prototype owners are established. |

Object #54 also has source-only definitions in Dalvik, Hostel, and Mavulsk; the active Charing load resolves the active Heavens definition. The six missing prototypes and all detailed line references are in the [existing story review](CHARING_STORY_REVIEW.md#residents-resets-and-shops).

The bounded inbound scan found no external ZON mobile reset targeting a Charing mobile, no active external SHP product targeting a Charing object, no typed external QST item command, no duplicate external MOB/OBJ definition for a local prototype, and no structured sidecar `item_vnums` reference. The recognized external object consumers are:

| Consumer | Charing prototype | State |
| --- | --- | --- |
| Active `shipy.zon` lines 516–517 | Object #45018, “dagger small” | Two active G reset references ([first](../../../areas/zon/shipy.zon#L516), [second](../../../areas/zon/shipy.zon#L517)). |
| Unregistered `nexus56.zon` line 1367 | Object #45037, “oak hand drum instrument” | Source-only G reset reference ([reset](../../../areas/zon/nexus56.zon#L1367)). |
| Unregistered `nexus56.shp` line 455 | Object #45037 | Source-only shop product. The conditional shop-reader fallback does not establish an operating shop. |

Exact `45000` matches in external QST and story-sidecar text are currency values in `R C`, `G C`, `R E`, or completion-key fields, not Charing room, giver, or item selectors. Other broad C/C++ number matches are the global money/experience constants, a hash-string substring, or commented-out code. The direct Charing code bindings are listed above.

## Title, tracked history, and attic disposition

The color-normalized `.zon` title “Charing” is unique among all 442 current zone sources; no active room-ID overlap or external duplicate mobile/object definition was found. Followed WLD/MOB/OBJ/ZON/SHP paths preserve the `charing` stem back to the 2009 source relocation. No separate whole-zone predecessor or replacement is established.

One partial content move is proven. Commit `06d31630e` moved the Charing Quartermaster prototype from #45095 to `siege` mobile #401030 and changed Charing’s reset to load it from the town center. Commit `7ddf5432f` retired `siege`, removed its source files and deleted Charing’s #401030 reset; the current source scan finds no remaining `#401030` reference under `areas/` or `src/`. The separate [history:siege coverage row](ZONE_COVERAGE_REGISTER.csv#L477) therefore records historical lineage, not a current zone that can be moved. Ask whether the Quartermaster’s removal was intentional; do not restore or relocate the NPC as part of this audit.

Keep active `charing` **DO NOT MOVE**. Keep `surf`, `surface2011`, and `nexus56` **RETAIN IN PLACE** under their existing candidate rows. The two source-only Surf approaches are preserved on Surf’s candidate row; Charing’s unresolved targets also remain among `surface2011`’s 145 active unresolved VNUM references. There is no Charing attic candidate row and no source placement change.

## Builder follow-up

1. Confirm the empty complete story sidecar’s intended scope beside the compiled world-quest bartender; add only verified authored story records.
2. Confirm why Half-Elf characters pass the southern gate but are not enabled to choose Charing as their hometown.
3. Confirm the intended paths and key acquisition for the Palace/jail doors and the Great Tree’s secret outside entrance.
4. Confirm that the #45151 mount room is intentional staging, including how it is used if the code and WLD have no direct player route.
5. Decide whether Charing’s Quartermaster was intentionally removed when `siege` retired.
6. Preserve unresolved object VNUMs, source-only Surf routes, and the conditional Nexus56 references pending their source owners and tool/editor/persisted-state closure.

No gameplay, source placement, server, database, build, or test operation was part of this checkpoint.
