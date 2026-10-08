# The Underground Lava Caves: comprehensive story mapping

Priority 89 closes the source review for zone 355, registry 35300–35642,
source area `lavcav`, reset mode 1. The [schema 3/revision 1 sidecar](../../../areas/story/lavcav.story.json)
classifies both native recipes: one exact-return card and one explicitly excluded
coin purchase. Seven contacts cover six addressed aliases; three optional checks
comprise current horns, an access key and earlier purchase history. The
[generated audit](../../reference/zone-story-audits/lavcav.md) records exact bindings.
**No native zone or quest repair ships.**

All new discovery, encounter, journal, achievement and daily credit requires
active, ready accounting. Frozen reward recovery remains separate. Native
prices, stock, item types, locks, mobility, spawn traps and PvP are preserved.
Source-comprehensive mapping does not qualify played transactions or renewal.

## Complete source closure

| Source | Complete review and dispatch implications |
| --- | --- |
| [Quests](../../../areas/qst/lavcav.qst) | Six blocks: four M / two Q; four addressed response families / six aliases. Full clue, sale, return, reward and departure messages read; no qc_action. Lieutenant I35515→I35525+E50000; adventurer C100000→I35515+D. |
| [Rooms](../../../areas/wld/lavcav.wld) | All 142 physical rooms, 35501–35642; 38 full title/prose families, 11 full headers, 451 exact exits, 128 relative patterns, six complete exit-text families and one complete non-exit metadata family. Every destination resolves; only external boundary is Mountain Tracts. Registry ownership starts at 35300; do not invent rooms for the unused lower range. |
| [Mobiles](../../../areas/mob/lavcav.mob) | All 48 full prototypes, 35501–35548, 46 full prose families and all numeric tails. All 48 placed through M or F; 35538/35539 are the two F followers. Gnome, lieutenant, adventurer, gatekeeper, instructor, alchemist, Dark Knight, sentries, prisoners, warriors, fire creatures and dwarves reviewed. No ACT_TEACHER or epic-teacher binding identified. |
| [Objects](../../../areas/obj/lavcav.obj) | All 33 full prototypes, complete flags/values/effects/extra descriptions. Horns are TAKE+HORN-wear, QUEST-marked armor; wrist chain is wrist armor. Iron key is KEY18 with break value 0; onyx key is WORN11 with values [500,500,13,0,…]. Cracked wall is fixed SWITCH29 / PUSH270 / 35537 WEST. No local type25 travel object. |
| [Resets](../../../areas/zon/lavcav.zon) | All 163 commands: 87 M / 48 E / 20 D / five G / two F / one O. 134 exact and 155 M-parent-aware families; 111 expanded groups with all room locations/parent identities retained. Gnome backpack has 40-percent declaration; instructor onyx key has 24-percent declaration, both cap1. F followers retain conditional/native followable context, not a new escort quest. |
| Shared execution | [Quest dispatch](../../../src/world/quest.c): exact durable roots supported; all numeric GIVE coins refused when accounting is active, before legacy do_give/completion. [Currency formatting](../../../src/core/utility.c) divides base value by 1000 for platinum. [Door handling](../../../src/cmd/actmove.c) uses VNUM in has_key without requiring KEY type. [Loader/reset](../../../src/world/db.c), [switch](../../../src/specs/specs.object.c), [wandering](../../../src/mob/mobact.c), [fire terrain](../../../src/mob/specials.c) and [mode1 renewal](../../../src/world/events.c) reviewed. |
| Global closure | No local shop file/record or literal special assignment. Dark Knight35523 appears in highdrop_mobs, but that array’s only chance-selector loop is commented out; do not invent a special horns drop. Both recipes touching local items are local. One foreign reset group places lamp35589 in Ravenloft2’s marble slab59045, with watch59126, at59269; all three commands and full foreign room/slab/watch read. Across 713 active type25 prototypes none targets these rooms. Full reciprocal boundary21148 SOUTH↔35501 NORTH read. |

The only identified native horns producer is the adventurer’s Q reward.
There is no O/G/E/P horns reset locally or in the reviewed global closure,
and the adventurer starts without a reset horn. His waving-horn description
and the prisoner’s lake-search clue do not create an ordinary loot producer.
The Ravenloft lamp is incidental imported loot, not another horns source or
foreign quest ownership. Shared P chooses a matching container through global
prototype lookup; authored parent context does not prove current live custody.

## Actual progression, alternatives and unsupported producer

| Q line | Native terms | Journal classification and meaning |
| --- | --- | --- |
| Lieutenant35517 /43 | I35515→I35525+E50000; no departure | Exact pair of platinum horns returns the golden wrist chain and configured experience, subject to native settlement rules. Lieutenant starts35628. Supplied matching horns fit without personal purchase, capture, thief death, prison rescue or mine history. Horns are consumed; worn/nested copies are not current loose hand-in roots. |
| Adventurer35535 /62 | C100000→I35515; D departure | Sole identified native producer; coin-only GIVE is refused under active accounting. Keep explicitly excluded until qualified wallet debit, item issuance and selected-recipient retirement exist. Dialogue says1500 platinum, while base100000 converts to100 platinum. Neither changing the fee nor placing free horns is a safe workaround. |

The earlier purchase check is optional retained history, not an instruction to
use the blocked coin route. Supplied horns can satisfy the lieutenant independently.
A recorded purchase cannot restore spent/lost/gifted horns. A wrist chain in
inventory, a clue response or current key does not prove returning the horns.
The explicit purchase exclusion retains its definition and historical evidence
without a zone achievement/daily unit. The lieutenant remains one potential
native daily candidate; actual source availability and renewal still require
qualification before advertising reliable daily completion.

Ask gnome `tresure` or `secret`; lieutenant `person` or `horns`; adventurer
`horns` or `horn`. Six aliases select four addressed responses. Preserve native
`tresure` spelling and actual `seasonal` keyword despite the seasoned name.
The prisoner’s text says he faints, but that M response does not mutate his
posture; old positions6/6 load sitting/normal. Dialogue/encounter is not a
learned keyword, recovered treasure, torture, incapacitation or rescue receipt.

## Access, moving recipients and supporting endpoints

| Fact | Honest explanation and separate evidence |
| --- | --- |
| Isolated adventurer spawn |35635 has no ordinary incoming edge; WEST leads35544 on the lake, EAST leads35634 with no exits. Both are unreachable from35501 in ordinary exit topology. The adventurer is not SENTINEL, is STAY_ZONE and can use native wandering when admitted. EAST can trap the live cap1 recipient; WEST can make him appear on the lake. Preserve this deliberate random-spawn design; do not add reciprocal player exits or promise a guaranteed encounter. |
| Selected recipient identity | Durable context captures quester/completion/room, while quest_mobile_for reselects by template and room. Movement, reset replacement, departure and delayed publication need authoritative NPC instance/epoch qualification. An old result must not retire a later matching adventurer. This extends the shared retirement follow-up from Orcish Slave Camp, not a proven played bug claim. |
| Prison entrance |35620 EAST/35621 WEST raw kind2 is PICKABLE, keyed35505. D resets make entrance closed/locked and reverse side closed/unlocked. Gatekeeper35519 at35620 has cap1 iron key with no configured break chance. Picking/opened access remains possible under native rules; no personal gatekeeper-kill requirement. Unlock, OPEN, arrival, office access and terminal exchange differ. |
| Training gate and unusual onyx key |35580 SOUTH/35582 NORTH raw kind3 is PICKPROOF and keyed35524, but D resets open both. Onyx key G on Mharag35526 at35636 declares24 percent/cap1. Its WORN type does not itself disable has_key, which compares VNUM; if a relocked gate is keyed-unlocked, value1=500 exceeds the0–99 roll range and can break the item on accepted destruction. Not500 uses, not a mandatory starting gate, and not permission to change its type or chance. |
| Secret doors |35581 EAST→35636 instructor barracks and35605 SOUTHEAST→35606 keep use closed/secret doors, with ordinary reverse closed doors. Reveal and OPEN remain native actions. Dark Knight prose does not add a personal kill predicate or special permission endpoint. |
| Cracked wall | O places35592 at35537, TAKE0/weight1200, values[270,35537,3,0,…]. PUSH clears WEST EX_BLOCKED and the valid reverse side’s blocking; it does not OPEN either closed door. Raw/D state9 is closed/blocked, not SECRET despite secret-passage prose. Ordinary reveal/control/arrival and another actor’s already opened route are different facts. |
| Fire and room policy | Lake sectors are FIREPLANE11, not a cosmetic name. firesector schedules heat effects; event_firesector can remove protect-fire/fire-ward effects, damage and kill under its native conditions. Most corridors preserve NO_RECALL/NO_TELEPORT/NO_SUMMON/NO_GATE and darkness. Instructor room has HEAL/NO_PSI, not INN. No added flight, immunity or escape bypass. |
| Services and lore | Mharag’s teacher prose has neither ACT_TEACHER nor epic-teacher binding. Manufacturer35540 has no local Q/shop/special merchant binding; potion resets are loot (serendipity352, bless3, coldshield131, fireshield108), not a crafting recipe. Library guards, dwarven mines, arena practice, suffering prisoners and a still-moving burned body do not implement lessons, mining, freeing/escorting or body-recovery outcomes. |

Active accounting also refuses zone item reset issuance before read_object,
including O/G/E/P, until durable reset-generation ownership is available.
Source declarations therefore do not promise a live wall, key, potion, lamp or
renewed material. F followers use the retained followable M context even when
item issuance is skipped; do not infer their behavior from a simple key-roll
percentage. Mode1 resets when aged and empty, not while players occupy the zone.

## Fair findings and required capability work

| Required ID | Finding, plan and qualification |
| --- | --- |
| **ZSQ-LAVCAV-SOURCE-RENEWAL** | Horns have no ordinary reset/loot producer; active accounting refuses the sole coin producer and all reset item issuance. Qualify admitted source/root/UID/custody, original acquisition versus gifts/rewards, worn/nested material, stocks/caps, actual empty-mode1 renewal and repeated accepted returns. Daily availability must account for a qualified producer or legitimate current material, rather than imply reliable renewability from repeatable=true. Preserve scarcity and legacy evidence. |
| **ZSQ-LAVCAV-COIN-PURCHASE** | Exact configured fee100000 base value versus dialogue1500 platinum. Design atomic debit of the actual payer wallet and exact matched amount, horns issuance, accepted receipt, refund/rollback/retry/concurrency/replay/frozen recovery and recipient retirement. Do not use legacy aggregated NPC money, change prices, introduce item fees or place free horns to bypass active refusal. Deliberately version/reclassify the exclusion after qualification. |
| **ZSQ-LAVCAV-WANDERING-RECIPIENT** | Deliberate isolated spawn/exitless trap and cap1 wandering seller need live discovery/location/availability without fabricated arrival credit. Qualify east-trapped versus west-roaming, occupied-mode1 reset, moved/removed target, replacement instance/epoch, delayed settlement/disconnect/replay and original-target retirement. Preserve trap/mobility; confirmed shared repairs require a separate named fix and broader selected-recipient tests. |
| **ZSQ-LAVCAV-ACCESS-EVIDENCE** | Fixed PUSH wall, asymmetric prison lock, secret doors, optional picking/key and fire terrain are prerequisites only when actually admitted. Capture control actor/selected object/reset generation, reveal/unlock/OPEN/arrival, already-cleared/other-player route and current survival/protection state separately from outcomes. Qualify accepted/denied key destruction, reblock/relock/reset and elemental-fire effect cadence; no automatic protection, flight or travel grant. |
| **ZSQ-LAVCAV-ONYX-KEY-INTENT** | WORN11 and values500/500/13 appear unusual, but VNUM-based has_key can use it and gates normally reset open. Builder confirms data intent before changing type/values/rare24-percent source. Test both reset-open and relocked gates, direct key eligibility, break/retained-root rejection, ownership/custody and compatibility. Any actual data repair needs a separate named fix, original-fails/repaired-passes proof and news. |
| **ZSQ-LAVCAV-SERVICE-ENDPOINTS** | Instructor/manufacturer/library/prison/mine descriptions suggest services or stories without implemented local endpoints. Builder chooses whether these remain lore or defines precise lessons/crafting/liberation/mining/escort transactions and outcomes. Confirm actual special/class/shop dispatch before adding anything. No teacher/merchant/rescue or special Dark Knight horns drop is inferred from a name or inactive highdrop loop. |
| **ZSQ-LAVCAV-CLUE-CONSISTENCY** | Lake search/waving-horn descriptions lack a physical producer; sale1500 versus actual100 platinum; fainting/secret-wall prose differs from state; spelling merits review. Prefer minimal truthful captions after confirming intended price/source/story. Preserve native aliases and balance. Actual changes belong in separate named fixes with before/after proof and prominent news; no free producer, extra entrance, disabled mechanic or quest activation. |

The deliberate spawn trap, unusual key type and prisoner tragedy are design
facts until builder intent establishes otherwise. The confirmed coin-admission
limitation is a universal capability blocker, not license to bypass accounting.
Preserve [Fields Between’s required legacy-rift replacement design](FIELDS_BETWEEN.md).

## Validation and limits

Source/schema checks protect both bindings, sole horns producer, six aliases,
all source families, spawn/trap topology, fixed PUSH and closed-door behavior,
prison picking, normally open training gate, unusual key type/break values,
rare declarations, follower context, absent service bindings, inactive highdrop
selector, imported lamp and active coin/reset refusal. Existing Python/C++
journeys cover worn horns and reward possession versus loose materials,
supplied horns without purchase history, optional key without access credit,
excluded purchase history without achievement or restored horns, exact return,
replay and cold recovery without Mountain Tracts discovery.

Catalog107 journals/1583 achievement units/1441 potential dailies/2193 story rows.
One unsupported purchase fallback achievement/row is removed; its native
definition and historical evidence remain. All2668 native definitions,
fingerprint/revision2/registry and106 prior mappings stay unchanged. Full
production regression, maintained build, formatting, links, original220 queue,
prior PR archive and exact repair/news preservation are required checks.

Synthetic receipts do not qualify played SEARCH/GET/gifts/worn removal/coins/
issuance/fire survival/native picking/key destruction/control/OPEN/arrival/
wandering or retirement/reward settlement/actual renewal or database persistence.
No accounting activation, DB/server operation, migration, deployment or merge.
