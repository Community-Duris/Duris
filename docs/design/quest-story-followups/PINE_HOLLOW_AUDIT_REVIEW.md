# Pine Hollow: connectivity, dependencies, history, and builder decisions

This CP320 supplement reconciles the existing [source-comprehensive Pine Hollow dossier](../zone-stories/PINE_HOLLOW.md) with the active graph, directed reachability, exact external references, title, and bounded file history. The area sources and relevant runtime sources are unchanged from the reviewed source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697` through current HEAD. The source findings remain distinct from played access, reset admission, stock, item custody, accounting persistence, and installed runtime behavior.

## Active graph and raw directed reachability

The current WLD has 166 rooms and 437 authored exits: 429 local and eight outbound. All 437 targets resolve in the active loader. The local raw graph has two weak components of 161 and five rooms. There are three active inbound records and eight outbound records, with two outbound records reciprocated. No exit has a negative or positive-unresolved target; there are no fall-chance or current fields.

| Active inbound | Return / status | Register |
| --- | --- | --- |
| Rftjngle #80398 down to Pine Hollow #16008 | No authored reverse exit | [row 972](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L972) |
| Twisted Wood #16364 east to Pine Hollow #16097 | Reciprocal with Pine Hollow #16097 west to #16364 | [inbound row 290](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L290), [return row 280](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L280) |
| Woodseer #16960 north to Pine Hollow #16065 | Reciprocal with Pine Hollow #16065 south to #16960 | [inbound row 301](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L301), [return row 279](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L279) |

From each of the three active inbound anchors (#16008, #16097, and #16065), the raw directed local graph reaches all 161 rooms in the main component, and every one of those rooms can return to each anchor in the raw graph. This establishes static WLD reachability only; door resets, class gates, hazards, and real player passage still need qualification.

The five-room component is the quest-mob staging group #16161–#16165. It has no authored local edge to the 161-room main component and no active inbound WLD route. Rooms #16162–#16164 have six one-way exits to other active areas; these are reset/load-room paths, not ordinary entrances. Room #16161 can reach the terminal holding room #16165, which has no outgoing exit and cannot return to its source. The existing dossier records that the three roaming trophy carriers load at #16161 and may enter this dead end; this is a source-supported availability risk, not proof that every deployed mobile is currently stranded. See the [roaming and holding-room review](../zone-stories/PINE_HOLLOW.md#roaming-sources-and-a-holding-room-availability-concern).

| Pine Hollow source | Outbound active target | Register |
| --- | --- | --- |
| #16065 south | Woodseer #16960, reciprocal | [row 279](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L279) |
| #16097 west | Twisted Wood #16364, reciprocal | [row 280](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L280) |
| #16162 east; south | Twisted Wood #16308; Twin Towers Forest #13595 | [rows 281–282](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L281) |
| #16163 north; east | Breale #2604; Twisted Wood #16344 | [rows 283–284](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L283) |
| #16164 south; up | Asylum #9287; Mistywood #95048 | [rows 285–286](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L285) |

The all-95 inactive-WLD sweep from CP71 remains applicable because those WLD inputs are unchanged: it found no source-room overlap and no inactive exit targeting Pine Hollow. No source-only ingress is added at CP320; the current curated source-only total remains 4,251 overall. See [WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md#checkpoint-320-pine-hollow-graph-and-reachability).

## Exact typed dependencies

The local QST and story sidecar use Pine Hollow object prototypes for their seven native contracts. The local sidecar's structured `item_vnums` all identify local objects. Three active external QST files make four exact item-command references into Pine Hollow objects. Three external story sidecars encode those same consumers: Twisted Wood lists #16014 and #16016 in `item_vnums`, Ice Crag records #16025 in a completion key, and Caves of Skelenak lists #16071 in `item_vnums`.

| External owner | Exact Pine Hollow item use | Source evidence |
| --- | --- | --- |
| Twisted Wood faerie guard #16311 | G `I 16014` black platemail and `I 16016` broken lance for token #16313; no disappearance outcome | [QST lines 36–37](../../../areas/qst/twstwd.qst#L36), sidecar items [#16014](../../../areas/story/twstwd.story.json#L67) and [#16016](../../../areas/story/twstwd.story.json#L78), [dossier analysis](../zone-stories/PINE_HOLLOW.md#other-cross-zone-consumers-and-lore-closure) |
| Ice Crag Masha #97006 | G `I 16025` fox pelt as one of seven ingredients; rewards #97110 and three #97136 | [QST line 151](../../../areas/qst/icecrag.qst#L151), [sidecar completion key](../../../areas/story/icecrag.story.json#L784), [dossier analysis](../zone-stories/PINE_HOLLOW.md#other-cross-zone-consumers-and-lore-closure) |
| Caves of Skelenak giver #4038 | G `I 16071` false marble eye for 100,000 coins, without a D outcome | [QST line 171](../../../areas/qst/caves_skelenak.qst#L171), [sidecar item #16071](../../../areas/story/caves_skelenak.story.json#L310), [dossier analysis](../zone-stories/PINE_HOLLOW.md#other-cross-zone-consumers-and-lore-closure) |

No external active or unregistered ZON reset-role reference targets a Pine Hollow room, mobile, or object. No active external shop product targets a local object in the supported `N` format; no duplicate room, mobile, or object definition was found in other active or inactive WLD/MOB/OBJ source files. The scan of 872 external type-25 object declarations found no target into Pine Hollow. No other external story sidecar contained a typed exact room/mobile/object reference to its local prototypes. The local area has no SHP file or direct local special-procedure assignment.

## Luiren candidate identity and old shop data

Existing candidate row 62 records only `luiren.qst` and `luiren.shp`; Luiren has no current WLD or MOB source and appears in neither current area manifest. Its unregistered QST contains 44 giver headers whose numeric VNUMs overlap Pine Hollow mobile prototypes. These are source-only numeric identity collisions, not 44 active quest givers or proven Pine Hollow routes.

The unregistered `luiren.shp` contains eight old-format `O` shop records. Across them, 16 listed product VNUMs overlap Pine Hollow objects, and eight keeper VNUMs overlap local mobiles. Two shop-room fields match local rooms #16008 and #16058.

| Luiren shop | Pine Hollow product VNUMs | Matching keeper / room VNUMs | Source |
| --- | --- | --- | --- |
| #16030 | #16024 | keeper #16030; room #16008 | [line 1](../../../areas/shp/luiren.shp#L1) |
| #16038 | #16034 | keeper #16038 | [line 32](../../../areas/shp/luiren.shp#L32) |
| #16039 | #16033 | keeper #16039 | [line 63](../../../areas/shp/luiren.shp#L63) |
| #16040 | #16027, #16028, #16029 | keeper #16040 | [line 94](../../../areas/shp/luiren.shp#L94) |
| #16041 | #16004, #16005, #16022, #16025 | keeper #16041 | [line 125](../../../areas/shp/luiren.shp#L125) |
| #16042 | #16014, #16011 | keeper #16042 | [line 156](../../../areas/shp/luiren.shp#L156) |
| #16051 | #16023, #16025, #16027 | keeper #16051; room #16058 | [line 189](../../../areas/shp/luiren.shp#L189) |
| #16065 | #16024 | keeper #16065 | [line 220](../../../areas/shp/luiren.shp#L220) |

Keep all these values on the existing [Luiren candidate row 62](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L62) as identity-ambiguous source-only references.

The current shop loader accepts `N` records and rejects an `O` marker through its old-shop fatal-error path ([format check](../../../src/economy/shop.c#L3362)). Its fallback opens every `areas/shp/*.shp` file if `areas/world.shp` cannot be opened ([fallback path](../../../src/economy/shop.c#L3267)); `areas/world.shp` is absent in this checkout. Thus the old Luiren data is not evidence of an operating shop, but it is a conditional loader compatibility hazard if this fallback is used. No server was started. Preserve Luiren on row 62 with RETAIN IN PLACE until owner identity and the fallback input are resolved.

## Compiled room-number references

A broad C/C++ exact-number scan found three meaningful Pine Hollow room VNUMs in the shared `guild_guard` procedure: room #16007 gates west passage to Warriors, room #16056 gates north passage to Clerics, and room #16145 gates east passage to Sorcerers ([#16056](../../../src/specs/specs.guards.c#L179), [#16007 and #16145](../../../src/specs/specs.guards.c#L209)). The WLD contains those rooms and exits, but `specs.assign.c` has no direct special binding to a Pine Hollow mobile prototype. Treat these as compiled conditional room-key references, not proof that an assigned guard currently enforces the gates. Preserve the numeric cases pending code-owner disambiguation.

## Title, history, and retention

After stripping color codes and normalizing case and spacing, “Pine Hollow” is unique among 442 current `.zon` titles; the active `pods` area is separately titled “Pharr Valley Swamp.” Followed WLD, QST, MOB, OBJ, and ZON history reaches the 2009 areas-tree move and later same-stem edits. WLD, QST, and ZON show the 2013 `stuff` change; MOB/OBJ also show 2012 field and 2015 quest-item flag changes. The story sidecar was added in October 2026. No alternate-stem rename, distinct predecessor, or whole-zone replacement was established. Luiren's QST/SHP numeric overlaps do not establish that it replaced Pine Hollow.

Keep active Pine Hollow **DO NOT MOVE**. Preserve its active boundaries, staging rooms, seven native contracts, all exact external QST consumers, the Luiren identity ambiguity, and compiled room-number references. Add no candidate row and make no route or gameplay change. See [coverage row 230](ZONE_COVERAGE_REGISTER.csv#L230) and [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md#checkpoint-320-pine-hollow-in-place-retention).

## Checkpoint 320: builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PH-01 | Should the warrior's “Atrucali” dialogue spelling be treated as a separate dragon from mobile #16086, Altrucali? | Preserve VNUM #16086 and the journal's verified Altrucali identity. Correct the prose spelling through an explicit content decision; do not add another NPC or trophy. |
| PH-02 | Are Auriam's request and the warrior's trophy request meant to require one side to be chosen or completed first? | Keep both native receipts independent. The source saves no faction choice, personal kill, or ordering; describe the competing NPC inventory explicitly. |
| PH-03 | Must the player personally kill Auriam, her children, the warrior, or Clamphorl to submit either exchange? | Do not infer kill history from dialogue. The QST checks supplied items; any personal encounter requirement needs its own designed event and receipt. |
| PH-04 | Should the warrior's lance story require a smith repair stage or reward a repaired lance? | Keep the broken lance as the exact input and its existing outputs. Correct the repair promise or scope a separate repair service; do not invent an intermediate contract. |
| PH-05 | Should Auriam's armor/lance and the Twisted Wood faerie's exchange be sequenced? | Preserve both independently authored requests. Their shared items create real availability competition, not a mandatory prerequisite; do not double-count one accepted foreign receipt in Pine Hollow. |
| PH-06 | Can Darlene's fox-pelt stole commission consume stock intended for Ice Crag Masha? | Keep the local and foreign exact receipts separate. Preserve the #16025 identity and document competing use; do not silently increase supply or make one zone's completion gate the other. |
| PH-07 | How should the two huge bear skins for Darlene's coat be supplied when the only active reset uses item cap one? | Preserve the two-item contract and cap evidence. Qualify fresh-generation accumulation before choosing a separate source or cap change; do not advertise universal supply from the static reset. |
| PH-08 | Are Darlene's five garment requests one ordered crafting chain or five independent trades? | Keep five independent exact-material requests with no coin fee, as the QST declares. Do not require an earlier garment or crafting service. |
| PH-09 | What should builders promise about fresh material availability under active accounting? | Preserve reset admission guards and source caps. Define and qualify a supported durable issuance path before promising fresh active-epoch stock. |
| PH-10 | Should the three roaming trophy carriers be treated as reliably present in the valley? | Keep reset owner and current location separate. Their load room #16161 is isolated from the main raw WLD graph and they may wander through foreign load exits; do not promise a fixed encounter location. |
| PH-11 | What is the intended behavior when a wandering carrier enters holding room #16165? | Reproduce the idle path and choose an explicit barrier, return route, or spawn relocation as a separate reviewed content repair. Preserve current NPC episodes and do not fabricate a currently available trophy. |
| PH-12 | Are exits from load rooms #16162–#16164 intended for NPC placement only, or should players use them as valley entrances? | Keep them classified as one-way staging exits until an owner designs player access. They do not provide an authored active inbound route. |
| PH-13 | Should the heavy green mine key and dull iron smelter key open both secured areas? | Keep the two key identities and doors distinct. Do not infer aliases or cross-key behavior from both items being called “key.” |
| PH-14 | Should the three concealed mine passages require kills, passwords, or an extra key? | Preserve their reset-closed secret-exit behavior and documented `rubble`, `rocks`, and `wall secret` discovery. Do not add gates absent source support. |
| PH-15 | Should discovery of the duergar refuge count as liberating or safely escorting the slaves? | Treat it as exploration only. A liberation campaign needs an explicit participant, NPC generation, release condition, destination, hostile/allied outcome, and persistent final policy. |
| PH-16 | Does trap #16062 make movement into the mine entrance hazardous, or does triggering it count as descent? | Preserve the trap and the blocked movement order. Distinguish the failed movement attempt and room-wide damage from any later successful arrival; do not claim a completed descent from a trigger. |
| PH-17 | Should opening the trapped desk immediately award the Skelenak eye quest outcome? | Keep trap survival, opening, and retrieving eye #16071 as separate events. Caves of Skelenak owns the foreign coin receipt; do not create a second local achievement. |
| PH-18 | Should fountain, corpse, herb-garden, illusion, or duergar-smith prose imply local poison cure, rescue, repair, or ritual quests? | Keep them as environmental story leads until a builder defines an objective and settled outcome. Descriptive prose and unused prototypes do not establish a live contract. |
| PH-19 | Should the six outbound staging exits and the three inbound area routes be changed to simplify access? | Preserve the two reciprocal pairs and the Rftjngle one-way inbound. Keep NPC staging edges distinct from ordinary routes; any topology repair needs a route-owner decision. |
| PH-20 | Are Luiren's overlapping QST/SHP numbers and the `guild_guard` room keys intended dependencies on Pine Hollow? | Keep Luiren's 44 giver IDs and old `O` shop references on candidate row 62; retain the three compiled room-key cases pending owner review. Do not treat them as active shops, active givers, or proven assigned guards. |
