# Father Tel's Holy Cloister: comprehensive source map

Priority 72 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The revision-one [journal](../../../areas/story/cloister.story.json)
classifies all eight native offerings as seven stories and one supporting
rejection exchange. Sixteen contacts retain all nineteen addressed topic families
and 28 aliases. Eleven optional checks cover nine current materials and two
earlier receipts. All seven stories are potential daily candidates; the rejection
exchange has no achievement or daily unit. Discovery has its own achievement.
Every new discovery, encounter, journal update, achievement and daily credit
requires **active, ready accounting**. Frozen obligations retain separate recovery.

One native wording repair ships separately: [208a56840](https://github.com/Community-Duris/Duris/commit/208a56840acb100fc20dbec4b0af9a80a6cd0914)
corrects Mahr's acceptance message from “letter” to “tablet.” No other native
quest, prototype, reset, route or server behavior changes in this checkpoint.
Other findings below are proposals, rather than completed fixes.

## Complete source closure

The active `areas/AREA` row is `cloister *671`; the zone is 671, registry range
66904–67170, reset mode two, levels 40–50. Physical membership is 71 rooms,
67100–67170, rather than every number in the registry range. Reviewed sources:

- [All 36 native blocks](../../../areas/qst/cloister.qst): 26 M, two MA,
  seven Q and one QA. Nine M are ambient `qc_action` routines, leaving nineteen
  addressed families and 28 distinct per-contact aliases. Ambient meditation,
  searching and memoirs remarks are not learned-topic or terminal receipts.
- [All 71 complete rooms](../../../areas/wld/cloister.wld): 33 full title/prose
  groups, six numeric header families, all six extra descriptions and 157 exits,
  including every numeric family and all four description/keyword families.
- [All 24 mobiles](../../../areas/mob/cloister.mob) and
  [31 objects](../../../areas/obj/cloister.obj), including descriptions, flags,
  values, affects, the tablet/note text, keys, containers, switches and egg trap.
- [All 113 resets](../../../areas/zon/cloister.zon): 97 exact and 99 parent-aware
  families; D26/O8/P7/M45/E14/G5/F8. The assistant, three worm followers and four
  elder guards retain their M leader; equipment after F belongs to the follower.
  No R, imported mob/object, local shop, ROOM_INN or computed ACT_TEACHER occurs.
- The complete assignment scan has no explicit local object/mobile/room binding.
  This does **not** mean no procedures: [object creation](../../../src/world/db.c#L3166)
  automatically binds all four type-29 switches, while
  [quest assignment](../../../src/world/quest.c#L2124) binds the native actors.
  Reviewed [switch implementation](../../../src/specs/specs.object.c#L309),
  [command dispatch](../../../src/cmd/interp.c#L2771),
  [selection](../../../src/world/handler.c#L5980),
  [search](../../../src/cmd/actobj.c#L9583),
  [unlock](../../../src/cmd/actmove.c#L2969), key destruction, opening, admitted
  GET/PUT, trap, corpse/source custody, movement, native offer/reward/XP,
  accounting guards, renumbering, reset generation and recipient retirement.

The bounded active-world scan closes three foreign item prototypes, four full
foreign reset groups, four foreign source-mobile bodies and five relevant foreign
room bodies. Two complete foreign native requests compete for the egg. All 713
active type-25 portals were scanned; none targets a local physical room. Four
cross-registry edges have two foreign room IDs, both active: the Surface tundra
approach and Clan Stoutdorf's rogue office. No dangling exit was found. The
[generated audit](../../reference/zone-story-audits/cloister.md) supplements this
review and does not replace full bodies or prove a played journey.

## Native offerings and progression

`E` denotes nominal experience before actual recipient/group policy. IDs in this
table are maintainer bindings; player hints use names and actions.

| Native block / giver | Exact requirement → declared reward; departure | Journal meaning |
| --- | --- | --- |
| [2 / Doss 67100](../../../areas/qst/cloister.qst#L2) | I76728 → I67129; D1 | Mande's blood-stained robes → priest's holy collar. The separately loaded head76730 and ordinary carved parts are not accepted substitutes |
| [34 / Mahr 67102](../../../areas/qst/cloister.qst#L34) | I67100 → I67101; D1 | Intruder's adamantite tablet → formal recommendation. The corrected caption now identifies the tablet; poison is a separate material |
| [67 / Tel 67103](../../../areas/qst/cloister.qst#L67) | I67102 → I67104; D0 | Bakarakh's named loaded head → glowing jade earring. No earlier recommendation or personal-kill predicate exists |
| [75 / Tel 67103](../../../areas/qst/cloister.qst#L75) | I67101 → I67101; D0 | Refusal and same-kind replacement note. Supporting exchange only; no achievement, daily, formal admission or original-UID restitution claim |
| [119 / Bakarakh 67104](../../../areas/qst/cloister.qst#L119) | I83374 → I67130; D0; QA | Kirrb's exact clan-lore tome → leather belt with crimson-coral buckle. Independent of head/ring requests; QA audience is part of the native response |
| [164 / upset disciple 67107](../../../areas/qst/cloister.qst#L164) | I67101 → E15000; D1 | Recommendation → experience plus an outpost clue. No physical reward or formal admission; Mahr receipt is optional |
| [207 / clan priest 67114](../../../areas/qst/cloister.qst#L207) | I67116 → I67117; D1 | Troggahn's egg → bone key to the locked podium. The egg is consumed and the priest leaves |
| [256 / adviser 67120](../../../areas/qst/cloister.qst#L256) | I67113 + I67103 → I67111; D1 | Elder ring and assassin poison together → rib bone. Local bone-key history is optional; later helmet-case access/GET is not this terminal |

The local route is **tablet → recommendation → disciple clue** alongside the
independent **egg → bone key → podium ring + poison → rib bone → helmet case**.
The latter includes actual world interactions after/between offerings, rather than
a single native all-stage campaign. Supplied notes/rings can skip earlier local
producer receipts. An earlier receipt cannot restore a spent, worn, nested or
handed-away ingredient. The nine material checks describe current loose items;
they neither record first recovery nor complete an offering.

Tel's note refusal is unconditional in its native contract: neither Mahr history,
class, faction, a student test nor prior topic is an admission predicate. It
consumes an item and grants a newly created item of the same kind; treat kind
replacement and original UID custody separately. The service receipt may be
retained as evidence, but it contributes no story denominator or daily credit.

## Exact sources, competing stock and foreign ownership

- The [sly assassin](../../../areas/zon/cloister.zon#L138) begins at67111,
  carrying hidden tablet67100 and hidden poison67103, both capped one. Search
  the actual source body and GET the exact loose items. The tablet's written
  “ten minutes” and poisoning instructions have no corresponding timer, food
  poisoning mission or accepted assassination episode in this source.
- [Bakarakh](../../../areas/zon/cloister.zon#L190) begins at67168, wearing
  hidden head67102 at HOLD18, with four following guards. This is a loaded
  trophy; CARVE does not create the required kind. His caption about concealing
  a dagger agrees with dagger67119 equipment, while his claimed ring ownership
  does not identify the current reset source.
- The [local ring](../../../areas/zon/cloister.zon#L122) is P67113/cap1 inside
  podium67118 at67150. The podium is closed, locked, pickproof (flags29), with
  bone key67117. [Hargorigard's Alatorin placement](../../../areas/zon/alatorin.zon#L2596)
  also gives ring67113/cap1 after a 20-percent M chance. Both sources share the
  same kind/world cap; native reset order and existing copies can suppress local
  stock. Do not promise a new ring or require Bakarakh's defeat for this material.
- [Troggahn](../../../areas/zon/cloister.zon#L188) starts at67167 and carries
  egg67116/cap1. Its declared T2/4/1/100 is object GET/PUT acid, one charge,
  level100, rather than a hundred-percent probability field. Qualified trap
  activation, surviving actor and accepted custody are separate requirements.
- [Brother Mande](../../../areas/zon/jade.zon#L608) at76786 carries hidden
  robes76728 and head76730, each cap1. Doss accepts the robes. Jade's
  [Saint Macavor request](../../../areas/qst/jade.qst#L268) accepts the head
  **and** egg67116 for I55324 plus two separately indexed I32019 rewards.
- [Kirrb Faktl](../../../areas/zon/alatorin.zon#L3353) at83552 carries hidden
  tome83374/cap1. Its full prose describes the clan's assassinations and Alatorin
  alliances. Bakarakh's old sea-rescue clue does not move Kirrb from the current
  Alatorin den. This source encounter belongs to Alatorin; the delivery belongs
  to Cloister. [Lancer's Winterhaven offering](../../../areas/qst/wh.qst#L2613)
  consumes egg67116 for I55420, E250000 and C350000. Foreign continuations do
  not complete another local parent story.

Five recipients depart: Doss, Mahr, the disciple, clan priest and adviser. Tel and
Bakarakh remain. Both key/reward supplies and singleton renewal need admitted
reset-generation evidence. Repeatable metadata and seven daily candidates do not
guarantee a present actor, available unique treasure, trap-free pickup or fresh
ingredients. No local paid shop/training/inn request is invented from actor titles.

The disciple's declared 15,000 experience is nominal. Shared reward policy caps
the offering player at one tenth of the next-level table value and other credited
same-room group members at the full next-level value. The durable continuation
freezes each admitted recipient's level and award separately. Record that existing
asymmetry as policy, rather than promising every player 15,000 XP or treating it
as a Cloister defect. Any later rebalance needs an explicit design decision and
independent failing/passing settlement and recovery proof.

## Access is more than a keyword or carried key

| Action / source | Actual result and boundary |
| --- | --- |
| SAY Khildarak to type29 wall67105 at67140 | CMD_SAY17 selects the no-show switch and clears EX_BLOCKED north. Object-special dispatch handles this before ordinary speech. A handled boolean or a spoken word alone does not prove a new gate mutation |
| Black key67107 from the first guardian placement | Unlock/open the gates67142→67143. Both gate sides have this key; the first side starts locked, the return starts closed. The shared guardian name does not grant a key to the second placement |
| Thin key67112 from guild officer67115 | Unlock/open the short-hallway approach67149→67152. Both local black/thin keys declare break rate100; unlock mutates the door before separately admitted key destruction. Busy/rejected destruction may leave the key intact; recovery must not invent another unlock |
| PUSH statue67114 at67152 | CMD_PUSH270 clears the ordinary blocked southern tunnel. PUSH is the actual command; PULL does not match its values |
| PUSH boulder67115 at67164 | Clears only BLOCKED from the secret west exit67167. SEARCH must then discover that local SECRET exit before ordinary movement. The return side is initially ordinary |
| PUSH tapestry67120 at67149 | Clears only BLOCKED from the secret north helmet room67169. SEARCH discovers the entrance. Its south return is independently SECRET and may require another successful search |
| Bone key67117 / podium67118; rib bone67111 / stone case67121 | Two distinct pickproof closed/locked containers use different keys. The podium has hidden ring67113; the case has helmet67122/cap1. Unlock, open, reveal/GET and actual source custody are separate from receiving a quest key |
| Stoutdorf34351↔67138 | Both exits carry SECRET4 and key34230, without ISDOOR/CLOSED/LOCKED. Search reveals the passage; the key field alone does not create a usable locked door. The rogue master has that foreign key, but this is not a journal prerequisite |

All four no-show switch objects are selectable through the shared lookup and
auto-bound type, despite absent literal assignments. The switch rejects bad room,
direction or missing target and reports “Nothing happens” when already unblocked.
Secret near-side exits retain SECRET; ordinary switch paths clear reciprocal
BLOCKED. Search reveals a local unblocked secret exit without automatically
revealing the other side. Shared access, someone else's switch action and already
open containers do not imply personal prerequisite history.

Room67170 has a one-way exit to the entry arch and reused arch prose, but no
selected incoming active exit or type25 portal. The surface and Stoutdorf approaches
remain valid. Record this orphan as a builder question, not an inaccessible whole
zone or a fabricated mandatory travel objective.

The egg declares a GET/PUT acid trap with one charge at level 100; the last
number is trap level, not a pickup probability. The shared GET path checks the
trap before accepted custody. A triggered trap consumes its charge, damages the
player when its effect applies and rejects that pickup attempt; a later successful transfer is a separate
fact. Source-recovery credit must not arise from the failed attempt or damage.

## Repair/news ledger and balanced proposals

**Implemented native fix, separate commit:** Mahr accepts tablet67100 and produces
recommendation67101, yet his old acceptance caption said “letter.” Commit
[208a56840](https://github.com/Community-Duris/Duris/commit/208a56840acb100fc20dbec4b0af9a80a6cd0914)
changes exactly that native word to “tablet,” plus its focused source regression.
The original wording fails the caption/contract check; corrected wording passes.
Native requirements, rewards, IDs, quantities, departure, aliases and all other
zone bytes are preserved. This is a wording repair, not a change to acceptance,
balance, formal admission, item ownership or saved state. Played turn-in and
settlement remain unqualified.

**News-ready:** “Brother Mahr now correctly identifies the intruder's tablet when
accepting it, making the Cloister's recommendation quest clearer.”

| Pending finding / confidence | Evidence and fair implementation plan |
| --- | --- |
| Formal admission / clearly unfinished narrative | The note promises recommendation, Tel only rejects it, and meditation speech offers a lesson without a native training/admission endpoint. Builder chooses a real student predicate/lesson/endpoint or clearer lore; preserve the note's disciple use and qualify supplied/refusal/branch behavior before changing recipes |
| Adviser ring and Kirrb clues / confirmed source mismatch, intent open | The ring is in the podium or on Hargorigard, while the General allegedly wears it. Kirrb is in the current Alatorin den despite sea-rescue lore. Choose accurate clues or deliberate source changes; prove exact acceptance, alternate supply/caps and no unintended mandatory kill or lost ring |
| Tablet assassination/poison deadline / lore only | The tablet directs poisoning within ten minutes, but no accepted food-target/timer/mission routine exists. Do not invent completion. Builder may retain backstory or design an admitted actor/target/deadline/attempt campaign with rejection, interruption and cold recovery proof |
| Orphan room67170 and stale passage key field / topology/design question | Valid approaches already exist; the orphan has reused arch prose and no incoming selected edge. Stoutdorf's secret-only exits cannot use their key field as a door. Decide restoration/retirement or intended flags, then validate reciprocal topology, mortal search/travel and source generations |
| Switch/key/trap persistence / qualification gap | Native switch/search/door mutations and trap charges are world facts, separate from item custody. Define accepted mutation and key/trap survival/recovery evidence before milestone credit; do not claim a reproduced live crash or broken access without played proof |

Every later native repair needs its own clear fix commit and prominent PR/news
trigger, before/after, failing/passing evidence, limits and news-ready sentence.
Prior actual repair/news entries remain preserved; the above proposals are not
announced as shipped fixes.

## Capability expansion and qualification

| Capability | Required evidence and acceptance scope |
| --- | --- |
| Semantic refusal / same-kind replacement | Preserve native offering ID, rejection/service category, zero achievement/daily unit and original versus replacement UID. Qualify destruction, replacement entitlement/index, actor/group context and partial/replay/cold publication; never infer enrollment from receipt |
| Accepted switch / secret search / arrival | Identify exact selected no-show object, trigger command, actor/room/reset generation and before/after BLOCKED/SECRET bits. Separate ordinary/secret reciprocal effects, already-unblocked outcomes, local reveal and arrival. Include automatically bound object types in source discovery |
| Key/container and trap custody | Freeze key UID and exact lock target, unlock/open/reveal, destruction result and container ancestry. Egg GET/PUT acid/trap charge, actor survival and committed custody need qualification before first-recovery or trap milestones |
| First source versus supplied handoff | Record exact robes/head/tablet/poison/tome/ring UID/kind, original carrier or container generation, successful reveal and accepted custody. Another player's handoff can satisfy native delivery while remaining distinct from personal recovery or kill credit |
| Multi-stage and competing materials | Local egg→bone key→ring→rib bone→case can be optional producer guidance today. Add builder-owned ALL/ANY families only with accepted predicates; preserve supplied ring/note shortcuts and separate Jade/Winterhaven egg ownership. A receipt cannot restore spent material |
| Frozen XP/rewards and renewal | Experience-only disciple outcome still needs actual group XP entitlements and recipient retirement. Same-kind refusal replacement, foreign duplicate rewards and local item issuance need per-index settlement. Qualify singleton/cap supply, rare20-percent source, reset generation and daily renewal |

Focused source, schema/file-loader and projection checks cover all eight bindings,
service exclusion, exact material readiness, worn/incorrect proof, supplied notes
and rings, optional history after material consumption, foreign owner rejection,
one credit per story, replay and cold state. Full catalog/audit/inventory regression,
maintained build, formatting and preservation checks are required before
publication. Synthetic receipts do not qualify actual SAY/PUSH/search/GET/key/
trap/combat/offer/XP/reward/retirement/reset/persistence or daily renewal.
No accounting activation, database/server operation, migration, deployment or merge
is part of this source checkpoint.
