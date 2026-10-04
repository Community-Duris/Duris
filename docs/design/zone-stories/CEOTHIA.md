# Ceothia: comprehensive source map

Priority 79 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The new [journal](../../../areas/story/ceothia.story.json)
uses schema three/revision one: six story cards bind all nine native recipes.
One card accepts any of the four thief-guild bargains; Lenbrea's three rewards,
the merchant's paired crates and the captain's legacy scroll remain independent.
Seventeen contacts retain all nine addressed dialogue families and 22 aliases.
Seventeen optional checks separate fourteen current material/access checks from
three earlier receipts. Discovery has its own achievement. All new tracking
requires **active, ready accounting**; frozen recovery is separate.

**No native zone or quest repair ships in this checkpoint.** The changes add
guidance, source evidence and projection coverage. The repair proposals below
need their own clearly named fix commits, before/after proof and prominent PR
news treatment if implemented. Existing shipped repairs retain their separate
news entries in the [execution register](../ZONE_STORY_ROADMAP_EXECUTION.md).

## Complete local and bounded foreign source closure

The active area is `ceothia`, zone 808. Registry bounds are 80789–81094, reset
mode two. Physical membership is 295 rooms 80800–81094, 110 mobiles 80800–80909
and 33 objects 80800–80832. Header levels 45–55 are metadata; they do not prove
enforced admission or uniform difficulty.

- [All 21 native blocks](../../../areas/qst/ceothia.qst): twelve M, eight Q,
  one QA; nine exact recipes. Nine M families are addressed conversations with
  22 aliases. The captain's other three M blocks use `qc_action 45/45/48`:
  timed ambient messages, not ASK keywords or command numbers. Each thief-guild
  bargain has D retirement; the other five recipes do not.
- [All 295 complete rooms](../../../areas/wld/ceothia.wld): 107 full title/prose
  families, seven complete headers, 619 exact exits and three complete exit-text
  families. Every repeated-family membership, direction, flag, key and destination
  was reviewed. There are no room E/F/T extras. The hidden guild entrances,
  audience chamber, time-portal storeroom and vault have distinct access states.
- [All 110 mobiles](../../../areas/mob/ceothia.mob): 88 complete long-description
  families and every distinct keyword, name, room appearance, numeric flag,
  race/class, level, money and remaining field. The many ordinary thieves,
  similarly named merchant guards, guild teachers and inn staff remain distinct
  from the exact recipients and ingredient holders.
- [All 33 objects](../../../areas/obj/ceothia.obj): full prototypes, descriptions,
  values, flags and affects. There is one unlimited type-25 ENTER portal and no
  type-29 switch. Local keys are type 18, crates type 12 OTHER, wagon type 15
  CONTAINER and the ordinary bluestone sword differs from Lenbrea's demonic
  reward. Names alone do not identify a source, usable effect or accepted kind.
- [All 510 resets](../../../areas/zon/ceothia.zon): D10/O11/P2/M327/E155/G5;
  299 exact rows and 386 parent-aware families. The tiny iron key belongs to
  the middle of three bored-guard resets; two P rows put two distinct crates
  inside one wagon. Four badges are equipped on four specific leaders.
- [The one shop](../../../areas/shp/ceothia.shp): the homely woman/general store
  at 81055, including full keeper, stock, messages, hours, trade restrictions
  and price fields. Descriptions of other shops do not bind a shop controller.
  The sole imported local-reset prototype, glowing pool 62 from
  [Heavens](../../../areas/obj/heavens.obj), was read in full.
- [Literal assignments](../../../src/specs/specs.assign.c): sword 80830 →
  `ogre_warlords_sword`; five rooms 81070/81028/81019/81078/81003 → `inn`;
  Golden Cat taproom 81021 → `crew_shop_proc`. These inn rooms lack ROOM_INN
  flags but have explicit bindings. Imported pool 62 → `stat_pool_agi` lies
  outside the local prototype range. The computed epic-teacher table assigns
  butcher 80862 → Toughness and captain 80907 → Dexterity; local ACT_TEACHER
  flags do not describe those assignments.
- The bounded foreign scan reviews all 713 active portal prototypes, all native
  recipe input/reward references to local or required kinds, and their foreign
  reset groups. Seventeen foreign recipe bindings were read with their complete
  response bodies. They consume ordinary Ceothian weapons or shared legacy
  heart/orb/tablet materials, without supplying a missing local story outcome.
  Quietus's responses call longsword 80808 a badge; its actual type is a weapon,
  not the common guild badge 80813. This copied-response mismatch is a proposal
  for the owning zone's next review, without a native repair here.
  Alatorin's weapon buyers and other scroll teachers retain their own owners.
- Nine relevant foreign reset groups supply the level potion, orb, heart, horn
  or thread. Full required prototypes 402/26614/32490/81410/81423 and reward
  404 were reviewed, together with their exact holder/room context. No active
  fixed reset, native recipe reward or explicit code producer of tablet 402
  was identified in the reviewed source; this is not a proof about every
  possible computed or saved-world source.
- Nine boundary edges touch four complete foreign rooms: reciprocal Surface
  forest 628287 ↔ southern road 80800, plus seven one-way staging/wander exits
  from Potions rooms 30709/30713/30714 into vault 81094. Staging exits are not
  advertised as ordinary player entrances. Foreign portal 81413 arrives in
  the present audience chamber 80980.
- The bounded timeline route includes complete portal prototypes, their fixed
  resets, arrival/return rooms, both named key prototypes and their immediate
  locked exits and reset parents. It also includes the complete future horn
  and thread holders, rare-load room 81674 and dispersal outlet 81675, plus
  Bel and the Dark's complete prototypes. It does not promote Past or Future
  Ceothia to comprehensive status; they remain separate roadmap work.

The [generated audit](../../reference/zone-story-audits/ceothia.md) retains
exact recipe, dialogue, reset and assignment references. The focused source
fixture checks complete local counts, exact contracts, source parents, types,
access/portal bindings, legacy supply limitations and the epic accounting guard.

## Six cards and nine accepted recipes

Inputs below are conjunctive exact kinds; two crates require two separate item
roots. The four recipes on the first card are alternatives. Each recipe still
has its own native ID and retained receipt; the grouped card counts once.
Supplied loose materials satisfy the native request without personal kills,
dialogue, access or time travel. Earlier receipt and current inventory checks
are optional preparation, without inventing a campaign order.

| Card / native line | Recipient | Exact inputs | Exact reward / lifecycle |
| --- | --- | --- | --- |
| Surviving guild / 20 | Moonstone leader 80801 | Jade 80806 + red 80810 + violet 80811 badges | Eyepatch 80803 + common badge 80813; D retirement |
| Same card / 60 | Jade wyrm leader 80802 | Moonstone 80805 + red 80810 + violet 80811 badges | Common badge 80813 + bracelet 80814; D retirement |
| Same card / 239 | Red shadow leader 80807 | Violet 80811 + moonstone 80805 + jade 80806 badges | Common badge 80813 + belt 80817; D retirement |
| Same card / 274 | Violet death leader 80808 | Moonstone 80805 + jade 80806 + red 80810 badges | Common badge 80813 + boots 80818; D retirement |
| Lenbrea's badge / 152 | Lord Lenbrea 80803 | Common badge 80813 | Bluestone key 80815 + 500,000 copper |
| Lenbrea's horn / 185 | Lord Lenbrea 80803 | Future horn 81410 | 500,000 copper + demonic sword 80830 |
| Lenbrea's thread / 202 | Lord Lenbrea 80803 | Thread of time 81423 | Glowing vault key 80832 |
| Paired crates / 305 | Merchant guard 80875 | Two copies of crate 80826 | 200,000 copper |
| Legacy Dexterity scroll / QA341 | Captain 80907 | Bel heart 32490 + orb 26614 + tablet 402 | Scroll 404; no observed learning grant |

All nine native definitions remain daily-eligible by their existing exact
item-only offering contracts. Six journal units replace nine fallback units;
the three-unit decrease groups alternatives without changing a native recipe.
Potential daily eligibility does not prove fresh supply, recipient availability
or played renewal. The legacy tablet issue remains explicit guidance.

## Sources, access and progression stories

Ask Lenbrea `hello/hi/howdy`, `legacy`, `service/town`, then `task`. His story
asks for one guild to survive. The source does not enforce a permanent player
faction or a durable “only one guild remains” campaign predicate. Four distinct
leaders wear their own badges: jade in 80923, moonstone in 80935, red shadow
in 80998 and violet death in 81001. Ordinary thieves carry no substitute badge.
The red entrance is the secret east door from 80943; the violet route begins
at the secret north door from 80959. SEARCH/OPEN and ordinary secret-door state
still apply. Removing equipped badges requires actual recovery into loose
custody; readiness is not personal source proof.

Each chosen leader leaves after its bargain. Collecting other leaders' badges
can remove them too, so giver/source availability and replenishment matter.
The common reward 80813 differs from all four leader badges. Lenbrea accepts
it independently for bluestone key 80815 and coins. That key opens the locked,
pickproof west door from 80980 to storeroom 80981 and breaks on a successful
ordinary unlock. The reverse door is also secret. The storeroom's portal 80816
uses ENTER7, unlimited charges, destination 81100 in Past Ceothia. An open route
or supplied later proof skips earlier personal access history.

The bounded onward route is Past vault 81389, portal 81111 → Future arrival
81400. Dragon 81124 in 81378 carries obsidian key 81117, which opens the
secret/pickproof trapdoor into that vault and breaks on use. In Future Ceothia,
dragon 81431 in 81669 carries mist key 81411 for the secret/pickproof vault
door to 81670. Portal 81422 there leads to Chronomancer 81454 in vortex 81676;
the northern bubble 81677 holds return portal 81413 → Lenbrea's present room.
These are exact routes and immediate prerequisites, without a complete foreign
zone walkthrough or evidence of personal traversal.

Huge beholder 81429 resets in room 81674 carrying horn 81410. Its rare-load
room has an exit to 81670 and several exits to empty dispersal room 81675.
The room title says “10%”; the native M reset itself says 100. Actual wandering
availability needs qualification; this journal does not promise a measured
10% reset chance. The horn is type-4 STAFF with separate magic, not a wand.
Chronomancer 81454 carries thread 81423 in 81676. Neither exact Lenbrea hand-in
requires the player to have used a portal, killed the supplier or changed time.

Glowing key 80832 opens the locked, pickproof north vault door 80980 ↔ 81094,
also breaking on successful ordinary unlocking. The actual imported object in
the vault is agility pool 62, with its bound DRINK effect. A dormant
`skill_beacon` table mentions 81094, but no live assignment places that beacon
here. The key receipt does not prove entry, drinking, a stat increase or learning.

The fixed wagon 80824 lies in 80845; its two P resets contain two oaken crates
80826. Wagon flags 15 mean closeable, closed, locked and compatibility-only
HARDPICK bit2. PICKPROOF is bit16 and is absent. The wagon therefore permits
ordinary lockpicking/knock attempts; tiny iron key 80827 is another route,
with a 15% break setting. Only the middle bored guard 80872 beside the wagon
gets that key. Crates are TAKEable type-12 OTHER items, not food or containers.
The exact recipient 80875 waits at the Steel Mug taproom 81088, separate from
the Golden Cat and similarly named guards. Nested/worn materials do not count
as loose ready inputs. Native collection and delivery remain unqualified.

Bel 32420 carries the trapped heart 32490 in 32469; the Dark 26642 wears orb
26614 in 26859. Both are shared materials used by other owners' recipes. The
captain's legacy QA asks for them plus exact tablet 402. Tablet and reward
404 are type-13 TRASH; generic READ follows LOOK. No bound scroll-learning
effect or current fresh exact tablet producer was found. The captain separately
teaches epic Dexterity and the butcher epic Toughness through PRACTICE164.
Those purchases explicitly reject active economic accounting. The legacy
recipe is not proof of an epic purchase or a learned skill, and its ambient
messages do not create achievements for individual questions.

## Shared/custom behavior and explicit capability expansion

| Evidence | Missing universal capability / qualification plan |
| --- | --- |
| Four guild alternatives with D retirement | Retain branch receipts but count one outcome. Add admitted actor instance, source recovery, death/retirement and reset availability; do not infer permanent faction or world-wide guild survival from a bargain. |
| Independent Lenbrea rewards inside a narrated timeline | Design an optional campaign with explicit all-stage facts and branch policy only after builder intent. Keep supplied proofs and open routes legal for native exchanges. Actual past/future/restoration outcomes need semantic events, not item names. |
| Keys break; secret/pickproof doors have reciprocal state | Record selected key/root, matched door, actual successful unlock/open/search and movement, with denial/failure and shared open-state handling. Historical reward receipt cannot restore a spent key or prove entry. |
| Two crates nested in a fixed wagon | Preserve container/child/reset lineage and exact two-root offering. Qualify key or skill route, OPEN/GET, partial bundles, transfers, capacity, simultaneous offers and durable settlement. |
| Foreign horn/thread/heart/orb and shared consumers | Separate source-issued custody from supplied or player-gifted proof; qualify equipped removal, traps, transfer/consumption and canonical recipient ownership. Names, inventory possession and foreign visits alone cannot prove first source acquisition. |
| Pool62 has level51 requirement, shared TAG_POOL two-day timestamp, heal and bounded random agility change | Use a targeted committed effect receipt containing selected object, cooldown admission and actual before/after stat/health delta. Denied, zero or negative delta must not count as positive gain. Qualify cooldown persistence, retry/replay and recovery. |
| Pool handler ignores DRINK argument; dispatcher calls room object specials before ordinary DRINK | Proposed shared handler repair: resolve the requested target before applying the pool. Cover empty/mistyped arguments, DRINK another container, multiple pools, pet restrictions, cooldown denial and capped stats. No pool repair ships here. |
| Legacy tablet/scroll narration differs from current epic training | Builder decides whether to restore or retire the legacy recipe. Identify a supported tablet producer and actual learning adapter if retaining it; audit all shared scroll teachers. Preserve the economic guard until atomic epic/currency/learning settlement is qualified. |
| Golden Cat crew service has faction/ship eligibility, funds, crew mutation and save | Carry forward coordinated wallet/ship settlement or an accounting guard, with failed purchase, capacity, competing hires, rollback and cold recovery. LIST/HIRE is a service, not native journal completion. |
| Demonic sword's damage-triggered random berserk | The whole local sword special was reviewed. Item reward acceptance is distinct from using its random combat effect; no invented “time restored” or boss-completion hook follows from equipping it. |
| Local level potion80831 also has foreign stock and procedural sources | Source adapters must cover admitted random-zone/relic issuance and foreign stock separately. No local hand-in consumes this potion; drinking/gaining a level is not a Ceothian journal outcome. |

The complete pool common handler and agility wrapper, ordinary special dispatch,
object-special pet restriction wrapper, crew service, local sword handler,
native quest/ambient execution, key/container access and legacy READ behavior
were reviewed. Bounded procedural potion branches and computed epic assignments
were checked. This source review does not qualify played custom effects,
currency/ship mutation, quest settlement or persistence.

## Repair proposals and builder decisions

| Finding | Fair assessment / proposed next repair |
| --- | --- |
| DRINK pool can intercept an unrelated target | Source-level target ambiguity in a shared handler; add a focused original-fails/repaired-passes executable regression, then isolate a target-resolution fix. Do not change cooldown, level, healing or stat rules as part of that fix. |
| No identified fresh tablet source or scroll-learning binding | The exact legacy exchange still exists. Establish intended availability and teaching behavior before deciding on producer restoration, real learning or deliberate retirement; do not describe every epic teacher as broken. |
| Steel Mug/Whip inn descriptions lack explicit inn bindings; most shop descriptions lack shops | Possible content/controller mismatch, not proof that every shop needs implementation. Builder decides intended rent/stock support and makes separately reported content fixes if desired. |
| Hidden guild door keywords say moonstone; copied directions/signs include other venue names | Review intended clues and aliases before changing prose. Existing numeric routes work; keep any eventual clue-only repair narrow with before/after news. |
| Story says surviving guild, trust and restored timeline without a campaign controller | Narrative aspiration, not a proven missing native payout. Specify durable campaign endpoints and acceptable supplied-history policy before adding completion credit. |
| Rare-load title, reset and dispersal behavior differ | Qualify actual spawn/wander behavior first; do not change reset probability based solely on the room title. |

No proposed fix is a shipped news item. Future native repairs require separate
named commits, a concrete player trigger, exact before/after scope, source or
executable evidence, played limits and a news-ready sentence. Journal prose and
generated audits must not obscure those repairs.

## Player presentation and verification limits

Show the surviving-guild card as one choice with four exact alternatives and
distinct gifts. Put the next native action next to current materials and access
hints. Keep the three Lenbrea outcomes separate while explaining the optional
badge → key → past/future route. Show two crates as two copies and the captain's
materials as three distinct kinds, with its legacy availability caveat visible.
Past receipts, current readiness and confirmed effects need separate states in
ANSI and GMCP. Orientation and learned-topic presentation are guidance; current
tracking does not dispatch achievements for every keyword or derive a campaign
automatically from lore. A per-zone sidecar remains the builder's explicit map.

Focused Python source/schema fixtures and C++ projection journeys cover discovery
and giver visibility, duplicate versus distinct badges, worn proof, one versus
two crates, supplied materials without personal access/history, independent
future proofs, one-credit/four-receipt guild choice, spent material/key readiness,
exact replay and cold recovery. Global checks cover the production catalog,
inventory/audits, all journal file loading, maintained build, formatting, source
links, prior journals/native definitions and the original queue.

Synthetic receipts do not qualify played source recovery, SEARCH/PICK/KNOCK/
UNLOCK/OPEN/GET/ENTER/DRINK/READ/PRACTICE/HIRE, offers, reward settlement, actor
retirement, wandering, database persistence or daily renewal. No accounting
activation, DB/server operation, migration, deployment or merge occurs here.
