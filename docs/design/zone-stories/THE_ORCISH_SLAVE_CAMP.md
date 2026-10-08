# The Orcish Slave Camp: comprehensive story mapping

Priority 88 closes the source review for zone 532, registry 53200–53213,
source area `shortc`, reset mode 2. The [schema 3/revision 1 sidecar](../../../areas/story/shortc.story.json)
classifies all three recipes: two exact-exchange cards and one explicit unsupported
ordinary-food exclusion. Five contacts advertise all 13 addressed aliases; four
optional checks comprise three current materials and one earlier master receipt.
The [generated audit](../../reference/zone-story-audits/shortc.md) records exact
binding identities. **No native zone or quest repair ships.**

All new discovery, encounter, journal, achievement and daily credit requires
active, ready accounting; frozen reward recovery remains separate. Source and
journal projection do not qualify played acquisition, access, transactions or
renewal. Native data, item types, key chance, locks and PvP remain unchanged.

## Complete source closure

| Source | Reviewed closure and dispatch implications |
| --- | --- |
| [Native quests](../../../areas/qst/shortc.qst) | All seven blocks: four M / three Q. Four addressed response families/13 aliases; no qc_action. Exact key and steak exchanges differ from G T19 ordinary food. Full response/departure messages and all input/reward identities read. |
| [Rooms](../../../areas/wld/shortc.wld) | All 14 physical rooms53200–53213,14 full title/prose families/four headers/27 exact exits/14 relative patterns/four complete exit texts/three complete non-exit metadata families. Mud/path extra descriptions, all pens and the hidden trapdoor included. |
| [Mobiles](../../../areas/mob/shortc.mob) | All 31 full prototypes53200–53230,27 full prose families and every numeric tail. All 31 have local M resets; no unplaced mobile. Hero’s old positions5/5 convert to sitting/resting, despite “incapacitated” in his name. Shared visibility/awake/not-fighting quest admission still applies. |
| [Objects](../../../areas/obj/shortc.obj) | All 18 full prototypes53200–53217, including hidden key, TRASH steak, mace, actual FOOD ration, worn bag/armor, well and two scrolls. Numeric values, flags, effects and full extra descriptions read. No type 25 travel/type 29 switch identified. |
| [Resets](../../../areas/zon/shortc.zon) | All 64 commands:31M/16D/11E/three G / two P / one O;64 exact/64 parent-aware families and 54 expanded groups. All proofs and loot are local, cap1; three unrelated M records have 50-percent chance. No F reset. Full P bag and trapdoor/ordinary door states reviewed. |
| Shared procedures | [Quest handler](../../../src/world/quest.c) separates addressed dialogue/exact-item/G T19 requests and refuses unsupported accounting offerings. Exact durable hand-ins select distinct loose roots. [Door/key handling](../../../src/cmd/actmove.c), key destruction publication and [EAT](../../../src/cmd/actobj.c) reviewed. [Loader/reset](../../../src/world/db.c) registers quests, converts old positions, decodes D states and globally selects P target bags. |
| Global closure | No local shop file/record, literal special assignment, ACT_TEACHER, epic-teacher, ROOM_INN or exact five-digit custom source reference identified. Across 713 active type 25 prototypes none targets local rooms. The only boundary is reciprocal Split Shield 10327 EAST↔53200 WEST; its full room record read. No foreign reset group or foreign consumer of local objects identified. Two exact-item recipes touch local objects; the third is the separate type-based branch. |

All supporting prisoner families, dog pen, well, bag/scroll loot and backstories
were reviewed. Their descriptions do not implement slave liberation, escort,
chain removal, family rescue, mage lessons or travel to other named settlements.

## Player progression and accepted outcomes

| Native Q line | Exact terms and authored result | Honest journal explanation |
| --- | --- | --- |
| Master 53200 /16 | I53200→C1500+I53201; no departure | Guard 53206 at 53203 carries the SECRET cap1 metallic key. Master starts53202, equipped with whip and carrying cap1 steak stock. Coins are a configured reward, not payment. Returning the key consumes it; owning a supplied/reset-stock steak does not prove this exchange. |
| Hero 53201 /35 | I53201→E15000+I53203; D departure | Hero starts resting in cell53212. Exact steak earns the lightweight spiked mace and configured experience, subject to native reward rules. The scene says he is poisoned and dies; shared retirement extracts him. It is not a rescue or combat kill, nor does the corpse text create a normal corpse. |
| Hero 53201 /53 | T19→no rewards; no departure | Ordinary FOOD is consumed, and the hero asks for more. Ration53204 is a local type 19 example, carried hidden by human53222 at 53210. This is distinct from the exact steak and is excluded until active accounting supports qualified type-based admission. |

The master’s receipt is an optional earlier route for the steak. A supplied exact
steak fits without personal key-return history. The master also begins with a
matching steak under cap1, so possession cannot prove his reward was earned.
Generic food cannot replace the exact terminal item or grant its mace/experience.
The hero’s tragic scene must not be titled saving, freeing or successfully
rescuing him. Other prisoners have no accepted local liberation endpoint.

Ask master `slaves`, `camp` or `encampement`; hero `food`, `hi`, `hello`,
`howdy`, `greetings`, `help`, `quest`, `camp`, `orc` or `encampenet`.
These13 aliases select four response families. Native misspellings are actual
keyword strings; a spelling repair would be a separate compatibility decision.
No per-keyword achievement or learned prisoner history is inferred.

## Key allocation, access and item-type boundaries

| Native fact | What can be explained; what remains separate evidence |
| --- | --- |
| Same key serves access and hand-in | Key 53200 opens DOWN53202→53212 and UP53212→53202, and the master’s exchange consumes it. Plan reveal/unlock/open before handing it over, or obtain another admitted key/use already opened native access. A receipt is neither a current key nor an unlock/arrival record. |
| Closed/locked/secret trapdoor | DOWN raw/reset6 is secret+closed/locked; UP raw/reset2 is closed/locked. Raw door kind2 on both sides adds EX_PICKABLE, not EX_PICKPROOF. Native lockpicking after reveal remains an alternative subject to skill/tool admission; no mandatory key-only gate is inferred. Native UNLOCK rejects hidden/blocked doors for ordinary players, so reveal precedes unlock. Unlock clears locking on the reciprocal side; OPEN and actual traversal remain separate. No new key, magic-only gate, ladder controller or guaranteed escape is authored. |
| Fragile key | Key value 1=20 is a20-percent break roll on each successful keyed unlock, not 20 guaranteed uses. Shared durable destruction can reject and retain a cracked key; actual accepted root destruction/custody matters. Native has_key accepts loose or HOLD equipment, whereas the journal’s optional carried check only inspects loose items. A held key may still open a lock. |
| Steak name versus type |53201 is TRASH13, TAKE, QUEST-marked, with values `[24,0,0,3,0,0,0,0]`. Its exact hand-in works by VNUM. Normal EAT below AVATAR rejects non-FOOD; the narrative death is scripted, not NPC EAT/poison simulation. Do not automatically change its type to 19:that would enable consumption and overlap the generic-food matcher. |
| Food admission blocker | submit_durable_quest_offering matches only exact QUEST_GOAL_ITEM roots, requiring every selected goal to be exact. A T19-only recipe never matches, and the active path refuses its offering; no legacy fallback is allowed while accounting is active. Schema current-material checks also use exact IDs, not arbitrary types. Keep the branch excluded rather than claim actual readiness/completion. |
| Recipient retirement | Both legacy and current exact paths publish D text and extract equipment/inventory/recipient. Durable context stores quester/completion/room; quest_mobile_for reselects by mobile template and room, without a captured NPC instance/epoch. Delayed commit versus retirement/reset replacement needs a concrete qualification case before assuming only the original recipient can be affected. |
| Supporting loot and water | Well 53214 is stationary type 17, not travel. Gnome 53227 equips bag53205 at 53211; two P resets target it with scrolls 53215/53216. Shared P uses get_obj_num on the matching prototype, so authored holder context is not a guarantee of the live bag’s current custody. Scrolls configure mage flame 297/globe of darkness 302; no local teaching receipt. |
| Light and services | Cell53212 carries DARK/NO_PRECIP/NO_HEAL; most camp rooms carry TWILIGHT and some NO_PRECIP. Neither a room name nor the hero’s “incapacitated” label implies a rent/teacher service or an additional waking objective. Native vision and current recipient admission still matter. |

## Fair repair findings and required capability work

| ID | Finding, proposed work and qualification |
| --- | --- |
| **ZSQ-SHORTC-SOURCE-RENEWAL** | Hidden cap1 key/ration, cap1 alternate steak stock, consumed materials and departing hero need actual source/root/UID/custody/actor/gift provenance. Qualify first source versus supplied/reset/reward, held/nested input, denied visibility, concurrent hand-ins, retries/rollback/reward settlement, recipient reappearance and actual mode 2 renewal. Preserve scarcity. |
| **ZSQ-SHORTC-KEY-ACCESS-ALLOCATION** | One fragile key is both the trapdoor tool and master input. Explain accepted reveal/unlock/open/arrival separately from current key and earlier receipt. Qualify20-percent break accepted/rejected destruction, held key versus loose journal check, already opened/other actor access, relock/reset and return without a key. Any reservation guidance needs actual admitted roots and selected use. No automatic bypass or new escape route. |
| **ZSQ-SHORTC-TYPED-FOOD-ADMISSION** | T19-only offering is refused with active accounting and has no native reward. Keep excluded. A future universal type offering must bind actual selected root UID/type/count/owner to the matched recipe, reject type drift/duplicates/worn/nested/invalid roots, atomically consume and record accepted no-reward outcome, preserve fixed-versus-type precedence, retry/rollback/replay/cold recovery and existing refusal boundaries. Reclassify deliberately after qualification; do not change steak type to bypass admission. |
| **ZSQ-SHORTC-RECIPIENT-RETIREMENT** | Scene text says death while code extracts recipient; template/room reselection can lose the original instance distinction. Define authoritative retirement versus combat death/rescue and capture admitted NPC instance/epoch. First qualify delayed settlement/removal/reset replacement/actor disconnect/replay so a later recipient is not retired by an older outcome. Any confirmed shared repair needs separate fix/news and broader selected-recipient cases, not a native narrative rewrite. |
| **ZSQ-SHORTC-STEAK-TYPE-INTENT** | Food-looking TRASH may intentionally protect the exact tragic quest item from EAT and generic T19. Builder confirms prop-versus-consumable intent before changing type/value/poisoning. Prefer explicit player explanation if it is a prop. A type repair would need normal/privileged EAT, exact and generic GIVE ordering, poison/effect balance, ownership/recovery, stock and receipt compatibility. Separate named fix/news if implemented. |
| **ZSQ-SHORTC-CLUE-CONSISTENCY** | Mud glint has no matching source reset; southern barracks says gnomes instead of actual dwarves; copied child/family descriptions and spelling merit intent review. Prefer minimal truthful source/cell captions after builder confirmation. Do not place extra keys, promise rescue or create mines routes from prose. Actual repairs require separate named fix, original-fails/repaired-passes proof and prominent news. |

The scene’s deliberate tragedy is not itself a broken quest. Likewise TRASH type,
key break chance and guard custody are design facts until builder intent says
otherwise. The documented admission limitation and selected-recipient hazard
need universal capability work with transaction/recovery proof, not unsafe
quest-data shortcuts. Preserve [Fields Between’s deliberate escape hotfix and
required replacement design](FIELDS_BETWEEN.md).

## Validation, counts and limits

Source/schema checks protect all three exact bindings, two-card/one-exclusion
classification, guard key/master steak/human ration, key 20-percent value,
closed/locked/secret reciprocal trapdoor, TRASH versus FOOD, bag/scroll P targets,
hero resting conversion, unsupported type-only admission and 13 grouped aliases.
Existing Python/C++ loader/projection journeys cover supplied steak without
master history, ration/worn/held materials versus loose checks, material readiness
without access or outcomes, two distinct receipts, spent key versus old master
history, explicit exclusion without destroying historical evidence, replay and
cold recovery without foreign discovery. All earlier journeys remain.

Catalog 106 journals/1584 achievement units/1441 potential dailies/2194 story rows.
The totals lose one unsupported fallback achievement/row because T19 is now
explicitly excluded; no native recipe/definition is removed. All 2668 definitions,
fingerprint/revision 2/registry and 105 prior journals remain intact. Production
catalog/inventory/audit, maintained build, formatting, links and exact original
queue/PR repair-news preservation are required checks.

Synthetic receipts do not qualify real SEARCH/GET/gifts/key destruction/reveal/
unlock/open/arrival/native exact offers/typed-food admission/poisoning/recipient
retirement/reward settlement/actual renewal or DB persistence. No accounting
activation, DB/server operation, migration, deployment or merge.
