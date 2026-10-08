# The Lair of Tiamat: shards, five flights and the Dragonspawn Gateway

Priority 176 in the original 220-area roadmap. This dossier covers selected source in `azhural`, including both native requests, all local world/reset records, the actual entrance and flight gates, the two native teleport objects and the bounded crown special. Played qualification remains separate.

The [builder mapping](../../../areas/story/azhural.story.json) has two independent story cards, nine contacts, nine dialogue aliases, six optional material rows and two accepted completion receipts, for eight steps. The shard row requires eight copies of one exact prototype; the essence card has five separate singleton rows. The two requests contribute two achievements and zero potential dailies. Both native givers disappear, and the zone has reset mode zero. New discovery, encounter, journal, achievement and daily credit requires active, READY accounting.

Native quest, world, reset and combat mechanics are preserved. Builder decisions and universal prerequisites below remain follow-ups; no native repair or new boss/portal/reward activation is shipped by this mapping.

## Source closure and ownership

- Zone `1352`, source range `135172..135289`, actual rooms `135201..135289`, reset mode `0`, selected in [AREA](../../../areas/AREA#L388). All 89 room IDs are contiguous within the actual room records.
- Read all 97 [quest lines](../../../areas/qst/azhural.qst): two `QA` contracts, two full `MA` responses, nine aliases and both departure messages. `A` selects room echo; the separate `D` records cause departure.
- Read all 194 [reset lines](../../../areas/zon/azhural.zon), including all 129 commands: 18 `D`, four `O`, 75 `M`, 18 `G`, 14 `E`. All 48 normalized command/location/parent families and their actual source occurrences were qualified.
- Read all 89 [rooms](../../../areas/wld/azhural.wld): 34 full prose families, two complete headers, one complete metadata family, 69 complete relative-exit families and 11 exit-description/keyword families. All 216 exact exit targets and reset room targets close locally.
- Read all 26 [mobile bodies](../../../areas/mob/azhural.mob), their 23 full prose families and 130 numeric lines, and all 38 [object bodies](../../../areas/obj/azhural.obj), including every extra description and affect.
- No local shop, matching foreign shop, missing reset object/mobile prototype, ordinary boundary exit or foreign reset parent was selected. Global recipe/source scans find only the two local contracts for the selected local item prototypes.
- The selected global type-25 scan finds two incoming objects, both local. No foreign authored incoming portal was selected. This establishes the static source boundary, not a claim that spells, runtime transport or staff travel can never reach the zone.
- The bounded literal/typed C/C++ scan selects only `sphinx_prefect_crown` on object `135218`. No local mobile or room assignment was selected. Generic combat, mob conversion and item teleport handlers still apply.

Both root cards remain owned by zone 1352. Physical route order explains the journey; it does not add a personal bone-key receipt prerequisite to Ynndakaneil’s native exchange. A supplied key, existing open passage or a supplied essence set can fit native behavior independently of the player’s earlier journal history.

## First request: eight bone shards for the cavern key

[Shadowy figure request](../../../areas/qst/azhural.qst#L18), giver `135219`, reset `M94` at the massive entrance `135202`:

```text
give=I:135211,I:135211,I:135211,I:135211,I:135211,I:135211,I:135211,I:135211
receive=I:135210
disappear=1
```

The `MA2` aliases are `tiamat gate entrance pass hi`. These are five ways to request one native response, with encounter visibility; they do not create five achievements or first-learned conversation facts. Room echo also does not award nearby listeners the acting player’s completion.

There are four bone golems `135212` at the first Pillar of Skulls `135201` (`M86/88/90/92`, each followed by one shard `G87/89/91/93`) and four at the second pillar `135286` (`M185/187/189/191`, shard `G186/188/190/192`). Each shard declaration uses cap eight. Both pillars are connected through the exterior wasteland before the entrance gate; all eight source parents lie in the seven-room exterior component. The source is not limited to the four golems beside the first pillar.

The optional row shows eight exact loose shards as current preparation. Under active durable ownership, [offering selection](../../../src/world/quest.c#L1517) requires all eight distinct root objects in the acting player’s inventory together. Offering one matching shard selects the complete set and submits a batch; an incomplete set receives “bring all” feedback. This is not a persistent deposit of one shard at a time, shared NPC inventory progress or proof that the actor killed eight golems. The universal item-offering cap is 14, so this eight-item contract fits.

The accepted completion records this exchange, then the figure departs. The fused-bone key `135210` opens the paired, pickproof entrance `135202E ↔ 135203W`, raw door kind `3`, reset state `2`. Receiving or holding the key does not prove that the actor unlocked, opened or crossed the door. The key has a 100 percent break roll after a successful keyed unlock.

## The five flights and their ward stones

The inner tunnels branch to five independent lairs. Each forward lair door is pickproof and resets closed/locked. A protector before each lair is the declared ward source; obtaining its ward, using the ward and recovering the consort’s essence remain distinct outcomes.

| Flight | Ward source: protector `135211` | Forward gate | Consort and exact essence |
| --- | --- | --- | --- |
| White | Second protector at `135207`, `M97/E98/G99`, ward `135225` | `135207N → 135208` | `135209` at `135216`, `M109/G110`, white `135205` |
| Black | Second protector at `135228`, `M118/E119/G120`, ward `135226` | `135228N → 135229` | `135208` at `135237`, `M130/G131`, black `135204` |
| Green | Second protector at `135240`, `M134/E135/G136`, ward `135227` | `135240E → 135241` | `135207` at `135249`, `M146/E147/G148`, green `135202` |
| Blue | Second protector at `135251`, `M151/E152/G153`, ward `135228` | `135251S → 135252` | `135210` at `135260`, `M163/E164/E165`, blue `135203` in `HOLD18` |
| Red | Second protector at `135265`, `M168/E169/G170`, ward `135229` | `135265S → 135266` | `135206` at `135274`, `M180/E181/G182`, red `135201` |

The white reverse door `135208S` declares key zero, while its forward side uses `135225`. A successful forward unlock updates the reciprocal lock state, so the normal paired return remains distinct from attempting to unlock the reverse side after an unusual entry or reset. Record that asymmetry for builder review; do not retarget the key or advertise a PICK shortcut. All five wards have a 100 percent break roll.

The consort names and source assignments provide a coherent route through the white ice, black swamp, green forest, blue dunes and red volcano. The material rows preserve five exact colors: five red essences do not substitute for a complete flight set. A supplied exact set fits the native exchange without personal battle history. Recovering the blue essence from a held slot also differs from a loose carried source.

Generic `_no_move_` conversion gives cover/no-lure behavior, and the white `_spec3_` keyword selects ordinary class specialization when eligible. These tags do not register quest objectives. Red, green and blue reset weapons have existing combat affects; the blue weapon `135235` also has packed spell `62`, level `60`, denominator `10` and a `_room_msg`. Existing [weapon dispatch](../../../src/combat/attack_effects.c#L309) and [selected packed actions](../../../src/item/weapon_actions.c#L300) are context. Do not infer a five-consort victory controller, a guaranteed proc, or source/kill achievement credit from those fields.

## Second request: five essences for the chromatic talisman

[Ynndakaneil request](../../../areas/qst/azhural.qst#L70), giver `135220`, `M115` at `135221`:

```text
give=I:135201,I:135202,I:135203,I:135204,I:135205
receive=I:135214
disappear=1
```

The `MA54` aliases are `tiamat archway gateway passage`, four routes to one response. All five exact loose essences must be together for the active durable offering path. The five optional singleton rows are a checklist of current materials; the accepted exchange is one completion, not five smaller quest receipts. Each card’s receipt is independent, so returning the essences first cannot complete the shard request.

The stone dragon leaves after granting talisman `135214`. His prose describes lowering his wings, but the selected request handler produces the key and departure; it does not directly unlock the world door. The talisman opens the paired pickproof gateway `135221N ↔ 135289S`, raw `3`/reset `2`. Opening and crossing that door follow separately. The talisman has a 100 percent break roll.

`QA` and `MA` affect message echo. The [native parser](../../../src/world/quest.c#L1951) sets `echoAll` independently from disappearance, and the [actual quest dispatcher](../../../src/world/quest.c#L1746) validates the chosen target and acting player. Successful room echo is not durable learned-dialogue progress for every observer.

## Arches, Avernus, throne and vault

- Dragonspawn Arches `135230`, `O84` at `135289`: type `25`, values `[135275,7,-1,0,0,0,0,0]`. They lead into `135275` in the Avernus passage.
- The passage `135275..135281` leads to the throne; `135277DOWN → 135278` and the following downward turn are actual topology, not inferred prose.
- Throne occupant `135221`, `M183` at `135281`, is explicitly named **Tiamat Place Holder**, PH class, level one. `G184` declares ruby key `135232`. No selected local Tiamat special assignment provides the described five-headed finale. This finding is local to Azhural and does not classify other Tiamat encounters elsewhere.
- Vault `135281E ↔ 135282W` uses ruby key `135232`, raw `7` and reset `6`: pickproof, secret, closed and locked. Finding it, unlocking it, opening it, arriving inside and recovering any treasure are separate facts. The ruby key also has a 100 percent break roll.
- Fiery portal `135231`, `O82` at `135282`: type `25`, values `[135202,7,-1,0,0,0,0,0]`, returning to the entrance.
- Neither portal is takeable in the authored wear mask. Their selected native path is [check_item_teleport](../../../src/magic/spell_travel.c#L932), through [interpreter dispatch](../../../src/cmd/interp.c#L2405), not an explicit `portal_door` binding. Command `7` is ENTER; negative uses are not decremented. The portal path itself does not check the player’s earlier quest receipts.
- A successful [teleport_to](../../../src/magic/spell_travel.c#L714) moves the actor to the selected room. A portal’s existence, inspection, attempt or another player’s passage does not prove this actor’s arrival. Durable arrival and denial/replay handling are still separate integration work.

The broader source contains room `135283`, **The Gates of Malsperanze**, with no exits and no selected portal source/target/reset. It remains isolated even in the complete local graph plus both portal edges. The static graph also has no outside-world boundary. Builders need to establish the intended public entry and the named isolated room’s purpose using historical design and actual transport configuration. Do not invent either connection.

## Source availability, unfinished content and fair repairs

Mode zero has no ordinary reset scheduling at [boot](../../../src/world/new_events.c#L1982). Initial forced boot reset and deliberate operational repop are different from recurring supply. The [accounting reset guard](../../../src/world/db.c#L3333) refuses item issuance without a durable reset-generation identity. Journal discovery or midnight rollover therefore cannot guarantee shards, wards, essences, keys, portals or replacement givers.

The normal route has seven exterior rooms with OPEN/SEARCH, 34 with a supplied bone key, 79 with that key and all flight wards, and 88 with all supplied keys plus the two portals. Raw door topology reaches 80; raw topology plus portals reaches 88. All scenarios leave `135283` isolated. These are conditional directed graphs assuming successful commands, live instances, usable stock and survival. They do not qualify personal access, accounting boot stock or a played raid.

Only 27 room descriptions contain the full ice/swamp/forest scenes; 62 are blank. Only red-consort room `135274` has sector `36` and flags `142639104`; the other 88 rooms are sector zero with flags zero. Scene prose does not establish environmental damage or blocked movement. `Dragotha`, `Mordukhavar`, the invasion actors, unused mobs `135222..135226`, several named reward objects and the hidden generator have no selected local reset/request route. The functional crown `135218` is assigned, but has no selected local stock or quest reward. These can represent reserved or unfinished design; builder intent should decide completion, deprecation or truthful presentation.

The crown’s bounded [special](../../../src/specs/specs.lohrr.c#L129) requires HEAD wear and SAY sphinx, applies level-60 lucubration when its 300-second timer permits, and sends a periodic readiness message. Existing timers/effect dispatch do not themselves mint a story receipt, guarantee accounting authority or establish a local crown source. Its absence from the local reward path is not permission to stock a powerful item or link it to a new achievement.

Keyed UNLOCK changes door state before [key destruction submission](../../../src/cmd/actmove.c#L118); ownership mismatch or refusal can leave the key intact. Future durable door/key integration must represent actual admitted outcomes and recovery, rather than recording every attempt as an atomic unlock-and-consume success.

## Owned follow-ups and required capability

| # | Owner | Follow-up and acceptance evidence |
| --- | --- | --- |
| 1 | Accounting/source runtime | Give O/G/E issuance a durable reset-generation identity; qualify initial boot, forced repop, refusal and cold recovery before advertising live supply. |
| 2 | Story/source runtime | Record first actual shard or essence acquisition with actor, UID, prototype, source NPC/slot, generation and transfer reason; gifts and source recovery must remain distinguishable. |
| 3 | Quest/UI runtime | Expose all-eight and all-five batch acceptance clearly; test seven shards, repeated same UID, wrong color, held/nested items, refusal and preserved ownership. Avoid a deposit-progress display without persisted escrow semantics. |
| 4 | Quest/receipt runtime | Preserve independent actor-owned receipts through accepted reward/departure, replay and cold load; room echo and nearby group members do not share automatic credit. |
| 5 | Builder/access runtime | Qualify the bone door from supplied key, existing opening and ordinary source route; no personal source-kill prerequisite. |
| 6 | Builder/access runtime | Map each flight ward to its exact parent and door; record admitted unlock/OPEN/arrival, not possession or key text. |
| 7 | Builder | Review the white reverse key-zero asymmetry against intended return/reset behavior. If correction is warranted, isolate it as a named route fix with news treatment. |
| 8 | Accounting/door runtime | Coordinate keyed unlock and key destruction publication/refusal/recovery; the current 100 percent roll does not prove committed consumption. |
| 9 | Story/combat runtime | Design personal and group consort-defeat credit explicitly, including encounter identity, eligible contributors, death/source ordering and gifts. Do not substitute essence possession for battle success. |
| 10 | Builder/combat runtime | Qualify actual proc/class/equipment behavior and survival for the five flights; several hidden prototypes are unused or incomplete. Balance decisions belong in separate reviewed changes. |
| 11 | Quest/access runtime | Represent talisman reward, guardian departure and gateway door as separate outcomes; prose about lowered wings cannot mark an unlocked passage. |
| 12 | Story/transport runtime | Record admitted ENTER and exact arrival through each portal, including denial, recovery and other actors; current receipt history is not a native portal requirement. |
| 13 | Builder/world runtime | Establish intended public arrival through historical transport design and live configuration; absence of an ordinary boundary is a source lead, not proof of universal inaccessibility. |
| 14 | Builder | Decide the purpose of isolated Malsperanze room135283; add an intentional connection/controller or retire unused content only in a separate named fix. |
| 15 | Builder/encounter runtime | Decide Azhural’s Tiamat finale and replace/retire the explicit placeholder deliberately. A full multi-head encounter requires lifecycle, group credit, rewards and accounting recovery design. |
| 16 | Builder/reward runtime | Establish the ruby key and vault reward route, exact source loot versus supplied items and native treasure stocking; neither existing receipt completes the finale. |
| 17 | Builder/relic runtime | Decide intended local source of the crown and other reserved rewards. Record successful crown activation/timer/effect only after source and balance approval; do not auto-stock them. |
| 18 | Story/dialogue runtime | Add learned-response facts for the two MA blocks with content revision, actor and successful response; nine aliases remain two conversations, and bystander echo is separate. |
| 19 | Builder/editorial | Review blank rooms, clipped Ynndakaneil response and unused invasion/guardian prototypes fairly. Choose authored completion or honest retirement; publish actual repairs in clearly named fix/news commits. |
| 20 | Integration/testing | Run played active READY discovery, batch custody/denial, source/gift, departure, gates, portal arrival, consort/queen and reward persistence journeys before promotion; keep blockers and source qualification visible. |

## Validation and publication

Passed publication gates: full production catalog/inventory/audit regression, all189 compiled Python/C++ journal journeys, exact source/schema/quantity/independent receipt/story-only exclusion and custody/replay/cold-recovery assertions, changed/staged canonical format and exact scope/native/prior-map/roadmap/link/PR preservation. The maintained Linux server build passed; src is unchanged here.

The unchanged full production regression passed on an isolated native Linux filesystem with all6218 selected input files verified by SHA-256 against the worktree before recording the result. All189 compiled journal journeys ran successfully in the worktree on Windows with the maintained native compiler and static cJSON dependency. No played active-accounting discovery, source recovery, eight-shard/five-essence batch custody, giver departure, ward/door/key breakage, portal arrival, consort or queen encounter, crown use or vault persistence outcome is claimed. The source mapping and planned follow-ups are comprehensive for the selected local content; actual prerequisites need the durable facts and builder decisions above.
