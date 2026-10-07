# Quintaragon Castle: family heirloom, Povtail and the keys through the haunted house

Priority 177 in the original 220-area roadmap. This dossier covers selected source in `castle`: both Remy requests, all world/reset/mobile/object records, the actual keyed route and optional containers, dynamically assigned Gilman teaching, the imported rune stone and the separate external memory exchange. Played qualification remains separate.

The [builder mapping](../../../areas/story/castle.story.json) has two independent quest cards, thirteen contacts, two native MA responses with five aliases, two optional exact loose material rows and two accepted receipts, for four steps. The local requests contribute two achievements and two potential dailies. Remy stays after each exchange. New discovery, encounter, journal, achievement and daily credit requires active, READY accounting.

No native world, quest, combat, economy, teaching, stone or transport repair ships in this mapping. Builder decisions and universal capability follow-ups below remain separate from the journal.

## Source closure and ownership

- Zone `24`, selected `castle`, range `2393..2532`, reset mode `1`. Actual [rooms](../../../areas/wld/castle.wld) are `2401..2532` except `2526` and `2531`: 130 rooms and 392 exact exits. All ordinary exit and reset-room targets exist.
- Read all 38 [QST lines](../../../areas/qst/castle.qst): `MA2` and `MA14`, five aliases, `Q18` and `Q31`, both exact native signatures and retained giver. The MA suffix chooses room echo; it does not award observers the actor’s response or receipt.
- Read all 414 [ZON lines](../../../areas/zon/castle.zon), all 283 commands and 139 normalized command/location/parent families: `D40/O19/P24/M100/E60/G12/F28`. Full source occurrences, follower leaders, equipment slots, global caps and conditional stock are retained.
- Read all 76 complete room-prose families, 17 numeric headers, 37 metadata families, 108 complete relative-exit patterns and 125 complete exit-description/keyword families. Clues, extra descriptions, raw keys, reset door states, room flags, sectors and the single F field are separate evidence.
- Read all 39 full [mobile bodies](../../../areas/mob/castle.mob) and all 51 full [object bodies](../../../areas/obj/castle.obj), including numeric bodies, extras and affects. No local or matching foreign shop, missing reset prototype or selected foreign stock parent for local prototypes was found.
- The active public boundary is `minizones5760N ↔ castle2401S`. Read the complete [foreign path](../../../areas/wld/minizones.wld) and both edges. Global selected local-item recipes find only the two Remy requests; a separate bounded import closure covers the rune stone and WH memory.
- The global 713-object type-25 scan selects unstocked local object `2443` as a potential incoming teleport to `2401`. No public live source is inferred from that prototype. The two other local type-25 objects target absent rooms.
- The bounded literal/typed C/C++ scan has no ordinary local assignment in `specs.assign.c`, but it does select [Gilman’s epic teacher row](../../../src/classes/epic_skills.c#L188). Actual [epic initialization](../../../src/world/epic.c#L1368) assigns his handler. The imported object `359` has an explicit `epic_stone` assignment. Selected generic/native handlers are therefore part of the closure even when the ordinary assignment-file scan is empty.

Both root cards remain owned by zone 24, with unchanged native receipts and no prerequisites. Route order explains access; native completion accepts a supplied exact item without personal key recovery, battle, dialogue or the other receipt. The external memory contract remains owned by its WH source rather than becoming a third Castle request.

## The two independent Remy exchanges

Remy `2417`, `M303` at chief’s office `2443`, is protected by five following guards `2412`. His equipment and ordinary castle sword do not represent either reward.

| Native request | Exact signature | Declared source | Reward |
| --- | --- | --- | --- |
| [Q18](../../../areas/qst/castle.qst#L18) | `give=I:2441;receive=I:2447;disappear=0` | Llamanby `2435`, `M406` at sky room `2511`, `E408` necklace in NECK slot 3 | Blue sword of revenge `2447` |
| [Q31](../../../areas/qst/castle.qst#L31) | `give=I:2435;receive=I:2448;disappear=0` | Povtail `2433`, `M358` at dog house `2481`, `E359` bone in HOLD slot 18 | Flaming orb of revenge `2448` |

`MA2` aliases `hi hello quest revenge` request one heirloom response. `MA14` alias `gilman` supplies a separate lead about Povtail. These are two native responses, with encounter visibility, rather than five achievements. Native room echo does not create learned conversation facts or award bystanders completion.

Each optional material row counts one exact loose item. The equipment slots on the source NPC explain where its declared item is stocked; the player’s worn, held or nested copy must become a loose offering before the active durable request path can select it. A gift can prepare the request without proving a personal kill or first recovery. A generic bone, another necklace, keys, the sword, the orb, the rune stone or memory cannot substitute for either exact prototype.

The two accepted completions are independent in either order. Remy stays. Daily eligibility requires enabled, reviewed policy and sufficient accessible telemetry from distinct players, matching observed level/faction and known party context, alongside discovery, active accounting and the UTC day limit. The shipped daily policy remains disabled. Day rollover preserves accepted history but does not qualify a candidate, create a replacement item or reset the zone. No new reward bonus is created by this map.

## Actual key and access progression

| Source and slot | Key | Admitted world gate |
| --- | --- | --- |
| Tev `2401`, `M213` at `2401`, `E215 HOLD18` | `2402` castle key | `2401N ↔ 2402S` |
| Juku `2406`, `M261` at library `2427`, `E263 HOLD18` | `2405` basement key | `2414DOWN ↔ 2415UP` |
| Giauque `2415`, `M286` at basement study `2439`, `G289` | `2418` second-floor key | `2427UP ↔ 2428DOWN` |
| LARGE rat `2418`, `M331` at storage `2452`, `G332` | `2422` holy key | `2446W ↔ 2448E` |
| Cetyla `2419`, `M333` at `2456`, `E335 HOLD18` | `2425` third-floor key | `2460UP ↔ 2461DOWN` |
| Glynn `2420`, `M351` at `2477`, `E352 NECK2 slot4` | `2437` fourth-floor key | `2480N ↔ 2488S` |
| Gilman `2428`, `M388` at `2493`, `E390 HOLD18` | `2438` sky key | `2480S ↔ 2498N` |

All listed doors reset closed/locked. The forward entrance has raw kind `2`, which [setup_dir](../../../src/world/db.c#L1511) makes PICKABLE; the reverse side has raw `3`, PICKPROOF. All other main keyed route doors use raw `3`, so their normal key route differs from the entrance’s ordinary PICK attempts. A successful attempt, usable key, unlocked door, OPEN and arrival are separate outcomes; no guaranteed skill bypass is claimed.

Tev’s key has break value zero and also matches the early bookcases. Keys `2405/2418/2422/2425/2436/2437/2438` have a 100 percent break roll after keyed unlock. Tiny key `2434` has a 10 percent roll. These are break chances rather than 100 or 10 uses. The existing door handler changes paired lock state before key destruction submission; committed consumption, ownership refusal and recovery remain a separate integration problem.

The conditional directed graph from `2401`, assuming admitted OPEN/SEARCH/keyed UNLOCK and survival, reaches 1 room with no key; 26 with the castle key; 38 with basement access; 54 after the second-floor key; 60 after the holy key; 85 after the third-floor key; 95 after the fourth-floor key; 122 after the sky key; and 123 including the optional secret-closet key. Remy becomes reachable at the second-floor stage, Povtail at the third-floor stage and the necklace source after sky access. These counts exclude personal custody, live stock, alternate transport, flight/fall behavior and played success.

## Optional bookcases, clues and source companions

- Povtail carries key `2436`, `G360`. Its real target is secret, pickproof `2471N ↔ 2479S`, raw `7`, reset `6`. The item’s text says north of Povtail’s dog house, but that dog-house exit is actually unkeyed. Preserve the real source route and record the text mismatch for builder review.
- The secret closet’s bookcase `2450`, `O197` at `2479`, has close/closed/locked flags plus legacy HARDPICK, mask `15`, key `2434` and eight potion `2433` P declarations. PICKPROOF is bit16 and is absent. The current [container PICK branch](../../../src/cmd/actmove.c#L3230) therefore permits ordinary attempts subject to skill, held pick, combat, timer and roll; the old HARDPICK chance adjustment is commented. Hildebrand `2421`, `F355` following Glynn, is the declared tiny-key source `G357`. Secret-room entry, container unlock, OPEN and exact potion recovery differ.
- Earlier bookcases `2444`, `O171/O176` at `2411/2412`, and `2409`, `O180` at `2422`, use key `2402` with flags `13`; these permit normal container PICK attempts subject to actual eligibility. Scrolls, gems and potions are P contents, not rewards for looking at a room clue. Sword racks are open containers.
- Povtail has eleven following guard `2434` declarations and a separate guard `2412`. Gilman has four large elemental followers and a conjured demon; Giauque has guards and skeletons; Glynn has Hildebrand; Cetyla has a holy assistant. The actual [F reset](../../../src/world/db.c#L3952) creates followers of the last M leader, and subsequent E/G commands stock the current spawned mobile. Neither follower count nor source battle history is a Remy completion prerequisite.
- Foyer maps, portraits, Renardien’s notes, holy altars represented by room extras, burnt meeting notice and “Get the bone” signs provide investigation context. A LOOK or ASK attempt does not record reading or response success. Lore that urges friendliness to ghosts is not an implemented pacification controller.
- Cook `2423` has no selected local Q/M controller. The six cracked eggs `2430` are type-13 props without TAKE; their text describes already scrambled food. A visible egg does not establish an acquisition/cooking quest. Dragon claw `2440` has no selected stock or local request; do not create a reward path from its prose.

## Sky hazards, teaching, rune stone and memory

The 26 rooms `2499..2524` have sector `8`, NO_GROUND, and usable downward exits in authored topology. [Falling admission](../../../src/world/falling.c#L113) checks current support, levitation/flying, mounted support, climbing, downward state and event scheduling. Supply and possession of a flight item do not prove an active effect, survival, arrival or Llamanby defeat.

Burnt room `2463` has `F25` but no downward exit, so the [room loader](../../../src/world/db.c#L1362) clears its chance_fall. Nearby prose about broken floors cannot establish a working 25-percent fall trap. This can reflect intentional old content or an incomplete hazard; builders should decide the intended topology rather than adding an unreviewed fall destination.

Gilman’s typed teacher row teaches **infuse life**, with eligible conjurer/necromancer/theurgist/shaman/summoner classes, current configured minimum epic-skill level, full command name, skill cap, epic/coin costs and funds checks in [epic_teacher](../../../src/classes/epic_skills.c#L453). His selected row has no prerequisite or denial skill. Merely meeting him or listing costs does not learn the skill. The [active-accounting guard](../../../src/classes/epic_skills.c#L684) rejects purchase before mutation. The story system’s own active-accounting requirement therefore cannot yet support a new successful teaching objective through this service; an accounting-compatible purchase and committed skill fact must come first.

Llamanby carries imported rune-covered stone `359`, `G411`, from [heavens](../../../areas/obj/heavens.obj), assigned to `epic_stone`. [Periodic handling](../../../src/world/epic.c#L1120) sets its loaded zone metadata and removes TAKE after it is in a room. [Touch handling](../../../src/world/epic.c#L1155) checks the exact selected stone, current power, pending reward, peaceful room, actor level, loaded zone and group eligibility, then submits a secured zone-touch payload. [Committed publication](../../../src/world/epic.c#L1067) applies eligible participant outcomes separately from touch messaging. Keep first loot, drop, actor touch, eligible group award, zone completion, refused/pending/recovered claims and stone power state separate. Do not double-award the existing zone-touch achievement.

Llamanby’s separate memory `55417`, `G412`, is a WH type-13 `_noquest_` object with its existing T field. The [external ambassador request](../../../areas/qst/wh.qst#L2498), giver `55137`, consumes that memory for scroll `55362`, native `C1000000` and token `55033`; ambassador stock is declared at WH `55005` and `55400`. This source route retains its external owner and economic authority. Recovering the memory does not prove a personal Castle kill; exchanging it elsewhere does not satisfy Remy’s necklace or bone receipt. No new Castle card or currency mutation is added for it.

The literal highdrop list names Llamanby, but its selected use loop is [commented out](../../../src/item/randomeq.c#L649). Do not advertise that as an active boss bonus or infer a new drop entitlement. The ordinary dragon race, combat equipment and generic behavior remain intact; no full played boss qualification is claimed.

## Reserved content and fair builder decisions

Seven rooms `2472/2525/2527/2528/2529/2530/2532` form a private/editorial component with outgoing edges into public sky content and no directed public incoming route. Even the complete raw public graph reaches only 123 of 130 rooms. Generic/staff transport could still reach them. Their personal room descriptions and legacy design require review before any public access objective or link repair.

Three local unstocked type-25 objects remain prototypes: Teemu altar `2421` targets missing room `2526`, and Gulko altar `2449` targets missing `2531`, both with command value `111`, **FART**, rather than PRAY. Jeremy Roenick altar `2443` targets entrance `2401` with command `197`, **WORSHIP**. Negative uses are not decremented by [item teleport](../../../src/magic/spell_travel.c#L932). No selected local or foreign reset/request source stocks these objects, so this finding is reserved content, not a claim that players currently use a broken public portal. Builders should choose intentional completion or retirement before stocking, retargeting or changing commands.

Key `2425` is titled third-floor but its extra describes second-floor access; actual `2460UP` is the third-floor stair. Key `2436` describes the dog-house exit rather than the secret closet. Clipped exit texts, spacing in the Povtail response, personal reserved-room prose and cooked egg descriptions are editorial review leads. Repair only a verified intended experience, with a separately named fix/news commit and clear before/after PR treatment.

Reset mode one requires the native empty-zone/reset policy, live global caps and admitted item generation. New day or discovery does not reset doors, restock boss/source equipment, replenish containers or create a replacement giver. Active-accounting reset issuance still requires a durable generation. This limits guaranteed availability, without changing the two requests’ existing daily classification.

## Owned follow-ups and required capability

| # | Owner | Follow-up and acceptance evidence |
| --- | --- | --- |
| 1 | Accounting/source runtime | Qualify native mode-one empty-zone resets, global caps and durable item-generation issuance; discovery/midnight must not create stock. Test refusal and cold recovery. |
| 2 | Story/source runtime | Record first exact necklace/bone recovery with actor, UID, source mobile/slot, generation and transfer reason; supplied gifts remain distinct from personal source acquisition. |
| 3 | Quest/receipt runtime | Preserve the two actor-owned Remy receipts in either order through replay, cold/raw recovery and daily rollover; source possession, the other receipt and room echo must not add credit. |
| 4 | Quest/UI runtime | Show loose-item readiness and independent return instructions; test held/worn/nested/wrong/reward/spent/reacquired items, denials and original ownership. |
| 5 | Story/dialogue runtime | Record two successful learned responses with content revision; five aliases remain two conversations and bystanders receive no automatic personal credit. |
| 6 | Story/investigation runtime | Record successful reading of the exact map/note/sign/extra rather than any LOOK attempt; separate narrative clues from new rewards. |
| 7 | Builder/access runtime | Map all seven main key sources and real gate sides; preserve forward entrance PICKABLE/reverse PICKPROOF and ordinary attempt denial without a guaranteed bypass. |
| 8 | Accounting/door runtime | Coordinate actual unlock, reciprocal state and key destruction/refusal/recovery for zero/10/100-percent break rolls; possession is not a consumed-key success. |
| 9 | Builder/container runtime | Qualify secret closet, tiny-key source, exact bookcase/contents and admitted container recovery. Preserve ordinary PICK attempts for masks13/15; bit16 PICKPROOF is absent and legacy HARDPICK does not guarantee denial or success. Review failure/pick-break behavior separately before any repair. |
| 10 | Builder/editorial | Review key2425 floor text and key2436 dog-house text against actual doors. If corrections are warranted, use named editorial fix/news commits. |
| 11 | Story/combat runtime | Design Povtail/Llamanby personal and group defeat credit explicitly; source slots, follower counts and supplied items do not prove personal kills. Preserve native encounter balance. |
| 12 | Story/movement runtime | Record admitted sky arrival, active support and actual fall/survival outcomes, including dispel/mount/climb/denial/recovery; no-ground route is not qualified by a flight item alone. |
| 13 | Builder/world runtime | Decide whether suppressed F25 hazard2463 is intentional or unfinished; verify intended DOWN target and safety/balance before changing topology. |
| 14 | Accounting/teaching runtime | Make Gilman’s existing epic purchase compatible with active accounting before a teaching objective; qualify eligibility, costs, mutation, refund/denial and committed skill acquisition. |
| 15 | Story/loader tooling | Discover teacher-table and initialization bindings alongside ordinary assignments; expose effective handler/property state rather than concluding there is no special from one file. |
| 16 | Story/zone-touch runtime | Reuse secured rune-stone participant/zone outcomes; distinguish loot/drop/touch/pending/denied/recovered/group award and avoid duplicate zone completion. |
| 17 | Story/external ownership | Present the WH memory exchange as a separate externally owned service; retain its exact memory, scroll, native coin value/token, caps and economic authority. |
| 18 | Builder/reserved content | Decide seven private/editorial rooms and three unstocked altars with missing targets/legacy commands; complete or retire deliberately, never auto-connect or activate. |
| 19 | Builder/editorial | Review clipped/spacing/lore discrepancies, unused claw and non-takeable kitchen props fairly; distinguish intentional atmosphere from verified incomplete quest mechanics and publish actual repairs separately. |
| 20 | Integration/testing | Run played active READY discovery, source/gift custody, both returns, gates/containers, sky encounter, stone group award and external economy/teaching journeys before promotion. Keep capability gaps and native repair plans explicit. |

## Validation and publication

Passed publication gates: full production catalog/inventory/audit regression, all190 compiled Python/C++ journal journeys, exact source/schema/independent receipt/daily and custody/replay/cold-recovery assertions, changed/staged canonical format and exact scope/native/prior-map/roadmap/link/PR preservation. The maintained Linux server build passed; src is unchanged here.

The unchanged full production regression passed on an isolated native Linux filesystem with all6221 selected input files verified by SHA-256 against the worktree before recording the result. All190 compiled journal journeys ran successfully in the worktree on Windows with the maintained native compiler and static cJSON dependency. After successful snapshot adoption, generated catalog JSON was restored to the generator’s canonical key order and escaping; every parsed value was independently verified against the tested bytes and current generator. No played active-accounting discovery, source/gift recovery, Remy hand-in, key/door/container breakage, sky support/fall, Povtail/Llamanby encounter, Gilman teaching, rune-stone group award or WH memory exchange outcome is claimed. This is comprehensive selected source mapping with owned follow-ups; durable objectives require actual successful facts and builder decisions above.
