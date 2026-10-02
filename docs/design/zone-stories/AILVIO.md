# Ailvio: comprehensive source story map

Reviewed October 2, 2026. Source area `newbie`, zone 292, journal revision 2.
All native exchanges and assigned quest-like scripts are classified below.
This is source-comprehensive mapping; active-world gameplay remains unqualified.

## Evidence boundary

Reviewed the active [Q/M source](../../../areas/qst/newbie.qst),
[resets](../../../areas/zon/newbie.zon), [rooms](../../../areas/wld/newbie.wld),
[mobiles](../../../areas/mob/newbie.mob), [objects](../../../areas/obj/newbie.obj),
and [shops](../../../areas/shp/newbie.shp). There are 116 distinct Q contracts,
160 addressable M responses, 35 automatic `qc_action` responses, 314 reset
commands, 105 mobile prototypes, and 133 object prototypes. The automatic
responses describe practice, demonstrations, and greetings; they print text
through `execute_quest_routine`, not the spells or item grants they describe.

The [literal assignment index](../../reference/zone-story-audits/newbie.md)
finds eleven assignments. Two more use constants from
[vnum.mob.h](../../../src/world/vnum.mob.h): `VMOB_AILVIO_INCAPACITATED` = 29303
and `VMOB_AILVIO_WIFE` = 29304, assigned `bandage_mob` and
`bandage_reward_mob` in [specs.assign.c](../../../src/specs/specs.assign.c).
These macro assignments are deliberately documented manually; the literal
extractor does not establish an exhaustive procedure list.

Reviewed all five [Ailvio procedures](../../../src/specs/specs.ailvio.c),
shared [spell assistance](../../../src/specs/specs.winterhaven.c),
[pet services](../../../src/specs/specs.room.c),
[native quest dispatch](../../../src/world/quest.c),
[special dispatch](../../../src/cmd/interp.c),
[fishing and bandaging](../../../src/economy/tradeskill.c),
[forage](../../../src/cmd/actoth.c),
[newcomer kits](../../../src/account/newbie_kit_plan.c),
[starting-home selection](../../../src/account/nanny.c), and
[creation grant admission](../../../src/item/item_movement_transaction.c).
Ordinary shopkeepers are assigned by the shared shop loader, beyond the literal
special index. Other generic combat, spell, and movement mechanics are support
rules, not proof that a local story stage occurred.

## Progression families and actual terms

All quantities below are exact native turn-in requirements. Items supplied by
other players satisfy these deliveries; the Q contracts do not require personal
fishing, searching, killing, foraging, or preceding receipts. Named teachers are
guidance, not newly imposed class admission. Native dispatch checks visibility,
awake/non-fighting giver, exact addressed target, and item terms. It does not
enforce a teacher-specific class campaign.

| Family | Source progression | Native boundary and journal treatment |
| --- | --- | --- |
| Welcome and supplies | Meet Burbul, learn commands/guilds, ask for a map, take a letter to a teacher | Scripted map grant is separate from the unusual Q map exchange. Sixteen teacher kit exchanges consume introduction letter 29319; services, not achievements. Osule has no letter exchange. |
| Feed the hungry family | Brodi explains fishing; obtain any two fish; give them to the tired father; take Charity Seal 29223 to Creyset | Father gives one seal and disappears. All 78 pair recipes represent one feeding story. Creyset separately accepts the seal for 2,500 experience and wooden earth totem 29224. Feeding is optional preparation for supplied seals. |
| Cure the sick child | Roshenbly suggests the grandmother; Red requests three reagents; give medicine to grandmother; bring her note to Roshenbly | Red: wing 29271 + hidden eyes 29272 + vine stalk 29333 → medicine 29255. Grandmother: medicine → note 29287. Roshenbly: note → 2,500 experience + emblem 29302. These are three independent accepted exchanges; note text narrates recovery, but the sick-child mobile is not transformed by a Q receipt. |
| Restore Silva's voice | Foet describes the stolen voice; Maerg offers her magical jar for captured drow spores; return jar to Foet | Maerg: spores 88502 → jar 29291. Foet: jar → 2,500 experience + delicate bard's flute 29292. Jar delivery is accepted without a Maerg receipt. Silva's actual speaking/singing state has no verified transition in these sources. |
| Recover dark stones | Saelryn or Drinnan points toward stolen stones in a concealed tree | Each independently consumes stones 29238. Saelryn gives 2,500 experience + tome 29293; Drinnan gives 2,500 experience + veil of shadows 29239. Keep separate giver requests; a single item is spent in the selected exchange. |
| Bandits and the abducted girl | Llerrad, Agare, Kord, or Risp describes bandits in the east woods; recover their three proofs | Each consumes ear 29262 + scalp 29263 + toe 29264. Llerrad: 2,500 experience, bow 29273, three arrows 29311, quiver 29312. Agare: 2,500 experience, two hands of justice 29265, beacon of courage 29266. Kord: 2,500 experience, 3,000 copper, bounty-hunter dagger 29237. Risp: 2,500 experience, dagger 29300. Delivery is tracked; negotiations, personal kills, girl release, and an exclusive branch are not. Kord's mixed currency reward requires the normal native reward path; this is not a mixed offering. |
| Nalk's combat trial | The warrior points east to the xorn | Skin 29267 → 2,500 experience + sword 29270. The skin is carried by the reset xorn. The receipt proves delivery, not survival of a personal duel. |
| Lapney's woodland gathering | Ask about quest, then items; find an acorn, branch, and feather | Acorn 29282 + branch 29283 + red feather 29284 → 2,500 experience + gnarly walking stick 29286. These are woodland objects, not fly body parts. |
| Recover an amulet half | Quek describes a bear cave; Esera describes the stolen half | Both consume half 29294, found beneath the bear den. Quek gives 2,500 experience + dagger of thieves 29295; Esera gives 2,500 experience + glowing black tome 29206. Different giver outcomes remain separate. |
| Perr's wolf materials | Find the black wolf deeper in the northern woods | Paw 29274 + skin 29275 + tooth 29276 → 2,500 experience + apprentice monk robes 29277. Delivery does not prove who killed or skinned the wolf. |
| Osule's meteor fragment | Search the southern forest for a buried fragment | Fragment 29330 → 2,500 experience + incomplete elemental chainmail 29331. No later local completion recipe was found for this incomplete suit; its description is not another tracked stage. |
| Taiz's bone offering | Taiz asks for a holy person's bones | Bones 29296 → 2,500 experience + death skull 29297. An acolyte carries the item at reset; receipt identity does not establish personal murder or a necromantic transformation. |
| Chyron's search lesson | Search the hut for salt and basilisk tears; bring both back | Salt 29220 + tears 29221 → detect-magic potion 29222. Custom replacement of hidden ingredients supports repeated search practice, but has no accepted-search history. |
| Red's separate foraging commission | Gather mandrake, dust, blood, herb, and the correct kind of frog eyes | 822 + 824 + 825 + 826 + 29241 → 5,000 experience, ceramic chillum 835, hefty bag 29310. Eyes 29241 differ from cure ingredient 29272. No ordinary source for 29241 was confirmed; preserve the terms and record the content gap. |

The journal has 39 rows: 22 requests/stories and 17 services. It classifies all
116 raw contracts. The reduction from 99 achievement rows to 22 removes 77
duplicate fish recipe achievements; it does not delete their receipt identities.
Do not combine all independent teacher deliveries into one any-terminal campaign.

### Family-feeding alternatives

The twelve allowed fish prototypes are 293, 294, 295, 318, 319, 330, 332, 333,
334, 335, 355, and 356. The native source contains exactly every unordered pair
with repetition: 12 × 13 / 2 = 78. Every pair gives the same seal and removes
the same father. Therefore an aggregate live count of two across these twelve
kinds accurately represents the offering set. One unknown fish kind does not
count. Repeat completion of different recipes retains both receipts while
contributing one story achievement and at most one daily unit per day, subject
to the existing eligibility and verified telemetry rules.

Fishing checks learned skill, standing readiness, a supported pole in carried
inventory, water terrain, continued presence, vitality, and normal failure rolls.
The success handler already uses `grant_tradeskill_item`, which calls the
creation grant authority. This is useful existing support, not the direct void
publication used by Burbul. Its generic `crafting` source does not identify a
first personal catch of a specific fish. Success prose and fishing experience
precede the grant result; qualify/fix that ordering before adding catch objectives.
Fish no longer use the old decay timer in this handler.

## Source atlas and preparation policy

| Material / prerequisite | Ordinary source in the active area |
| --- | --- |
| Introduction letter 29319 | Ailvio-specific newcomer kit plan; consumed by one of sixteen teacher exchanges. Not a reset-loaded letter. |
| Spores 88502 | Drow mother 29225, prisoner cage 29275. Prototype belongs to `udmini`, but G reset is local; cross-area prototype ownership is not travel proof. |
| Stones 29238 | O reset in hidden tree room 29293, reached from concealed north exit of 29286. |
| Wing 29271, eyes 29272, stalk 29333 | Wing O at 29285; eyes P inside fallen log 29240 at 29287; vine 29305 carries stalk in garden 29211. |
| Acorn, branches, feather | Acorn O at 29298; branches O at 29289/29291; feather O at campsite 29245. |
| Ear, scalp, toe | Three different captors in bandit camp 29290. Their corpse loot supplies delivery materials; girl 29280 has no assigned release script. |
| Xorn skin, acolyte bones | G resets on xorn 29281 in 29304 and acolyte 29276 in 29227. Prose about killing does not authenticate the player who acquired the proof. |
| Amulet half | O at 29302 below bear den 29301; opening described beneath bones. Tight-space movement is a support mechanic. |
| Wolf pieces | G resets on wolf 29285 in 29307. Do not confuse it with tame teacher wolf 29265. |
| Meteor fragment | Hidden O at forest room 29248. No prerequisite item or topic token is enforced. |
| Salt and tears | Hidden O items at hut 29247 plus replacement procedure object 29329 there. |
| Medicine, note, seal, jar | Explicit Q outputs. Jar is also equipped on Maerg; its physical existence alone is not proof that the spores exchange occurred. |

The eight ordinary shops are Burbul 29201/room 29201, minstrel 29206/29269,
cook 29214/29276, serving wench 29215/29260, robed merchant 29219/29271,
Brodi 29226/29257, Burnarg 29256/29268, and Hizzy 29283/29305. Brodi's stock
includes pole 336 and skiff 29232; Burbul sells bandages 393. Prices and shop
admission follow the shared shop system. These supplies do not create quest
achievements. Pet shops at 29280 and 29282 use adjacent holding rooms. Shared
code explicitly refuses pet purchases and rentals under active accounting.
Listings and lessons can still be useful; do not make owning a pet a prerequisite.

The slab/grotto, coffin/crypt, mirror back to Mystic Alley, bank/locker scenery,
inns, ship display, combat practice, decorative religious objects, empty rooms,
and race-land descriptions are exploration/support content. They do not prove
a second executable departure campaign. The journal offers departure guidance
for the reset archway 29236 at 29240, whose procedure selects a race/class
hometown through `find_starting_location(ch, 0)`, updates home/birthplace, and
calls teleport. It does not enforce finishing the teachers, holding a map, or
a level cap. Successful arrival and one-way policy need explicit travel evidence;
do not infer them from entering a command or the item's stale numeric destination.

## Scripted bandaging encounter

Both wounded man 29303 and woman 29304 reset in forest room 29289. The woman
asks the player to `bandage human`. The man's periodic handler continually
sets hit points to -3 and keeps him incapacitated while alive and not fighting.
Normal bandaging requires a learned skill, a usable bandage, an injured target,
and no active battle. Durable bandage consumption is already submitted through
item movement, with `begin_bandaging` called after committed consumption and
target/room checks.

The woman's reward handler tests whether a player has a scheduled
`event_bandage_check` targeting any NPC of the man's VNUM. It immediately calls
stand/speech, calls `gain_exp(..., 1000, EXP_QUEST)`, and extracts both mobiles.
It does not require positive healing, a recovered position, continued valid
bandaging conditions, or the exact co-located encounter instance. The scheduled
event can still fail its later descriptor, exhaustion, skill, hiding, or location
checks. This is a confirmed attempt-versus-success mismatch, not proof that
normal bandaging consumption itself is unaccounted.

Until repaired, describe the encounter as a scripted request without a completed
journal stage. Repair should connect a successful healing/revival result to
the exact encounter generation, freeze one reward recipient, and commit/recover
reward plus completion before narrative departure. Explicitly decide whether
attempted aid is the intended win condition; if so, rename the outcome and
explain it honestly rather than pretending the target was revived.

## Findings and repair plan

| ID | Finding and boundary | Concrete fix / qualification |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | Fresh active accounting suppresses item-producing resets, including hidden supplies, recipe materials, portal and procedure objects, NPC gear, and containers. Recovered items may exist. | Qualify reset generation/custody with nested dependencies, hidden flags, foreign prototype ownership, and NPC inventories before active Ailvio journeys. |
| ZSQ-AILVIO-SCRIPT-GRANTS | Burbul prints a map grant then calls void `obj_to_char`; active publication rejects an unowned creation candidate. Chyron removes the old room item and calls void player publication before hiding a replacement. | Use committed grant/transfer results and stable script/reset identities. For Chyron, qualify exact old-item custody and replacement generation together; no success text or irreversible removal before admission. Test missing context object, wrong item, hidden item, unknown source, failure, retry, and restart. |
| ZSQ-AILVIO-MAP-CONTEXT | The map handler matches raw `burbul map` arguments and trusts its procedure object's dispatch context. It does not independently verify a visible Burbul target. Special dispatch includes equipped/carried and room objects. | Define intended presence, alias and replay policy before adding learned/map outcomes; bind to actual actor/source room/generation. Preserve ordinary repeat map service unless builders choose otherwise. |
| ZSQ-AILVIO-BANDAGE | Reward recognizes a scheduled attempt rather than confirmed revival and matches prototype rather than exact encounter generation. | Successful-bandage semantic adapter; exact victim/room instance; single recipient/episode; recovered reward. Test failed/aborted aid, depleted vitality, target death, concurrent helpers, movement, repeated reset and restart. |
| ZSQ-AILVIO-FORAGE | `do_forage` explicitly refuses active accounting. Red's commission therefore lacks its ordinary forage route while accounting is required. Existing fish creation authority is different and must not be called equally blocked. | Port forage birth/actor/terrain/attempt evidence with committed item grant; respect its existing failure and terrain rules. Qualify fishing result/prose/experience after grant outcome, using a distinct fish catch source for personal objectives. |
| ZSQ-AILVIO-EYES | Red requests prototype 29241; the supplied cure eyes are 29272. No ordinary reset, local shop, Q producer, or literal script source for 29241 was found. | World builder decides whether to restore the intended distinct source or correct the requirement/prose. Keep exact current terms, make the missing route explicit, and test the chosen fix rather than silently accepting either kind. |
| ZSQ-AILVIO-LESSON-ALIASES | Red's first six lesson keyword lists are repeated-letter placeholders; the prose advertises normal words they do not match. The seventh, `quest`, is usable. | Restore readable canonical lesson aliases with builder review and actual dispatch tests. Current contact advertises only the valid quest topic. Other teacher topics are fully listed; no learned-topic achievements are invented. |
| ZSQ-AILVIO-ROLE-DRIFT | Taiz's mobile/room descriptions call him a redeemed theurgist purifying the crypt, while his quest asks for killing an innocent holy person and his dialogue teaches undead pets. This is inconsistent content, not an enforced class/provenance gate. | Builder reviews the intended theurgist story and lessons, then aligns narrative and any changed executable terms deliberately. Retain historical receipt IDs; do not erase old completions or infer a purification campaign from the newer description. |
| ZSQ-AILVIO-DEPARTURE | Home/birthplace changes precede teleport confirmation. | Validate chosen hometown/destination and publish confirmed move before durable departure stage; recover exact home/birthplace outcome. Test invalid destination, race/class defaults, hometown-choice fallback, refusal and reconnect. |
| ZSQ-AILVIO-NARRATIVE | Cure note describes recovery, bard reward describes restoring a voice, and paladin describes negotiation/release, without verified corresponding world-state transitions. | Distinguish accepted delivery, narrated closure, actual rescued NPC, and personal combat. Add exact generation/recipient events only if builders implement those actions. Do not turn decorative incomplete chainmail into a fabricated recipe. |

## Qualification matrix

Exercise every fish pair against native bindings, same-kind/mixed live counts,
unknown fish rejection, alternate receipt preservation, one projected achievement,
one daily group after verified eligibility, replay and cold restart. Test supplied
seal/jar/medicine/note with no predecessor receipts, and a complete local cure
route with distinct intermediate outcomes. Cover all giver alternatives without
inventing branch lock or class requirements. Qualify hidden search/containers,
all grants, failed bandaging, active forage, pet-service refusals, and selected
hometown arrival independently. Reads must remain mutation-free.

The shipped revision implements safe native grouping, named families, complete
useful topic guidance, material source hints, and optional predecessor steps.
Script objectives, personal source history, rescued NPC states, and active source
adapters remain open in the [shared plan](../ZONE_STORY_INTEGRATION_PLAN.md).
