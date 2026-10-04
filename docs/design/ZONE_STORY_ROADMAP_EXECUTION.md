# Zone story roadmap execution register

The full [priority roadmap](ZONE_STORY_ZONE_PRIORITIES.md) remains the work queue.
The [expanded plan](ZONE_STORY_INTEGRATION_PLAN.md) remains the shared capability
contract. This register records concrete progress and findings without treating
static binding coverage as comprehensive script or gameplay qualification.

## Completion criteria

For each touched zone, review the complete active Q/M source, resets, rooms,
prototypes, special assignments and implementations, and relevant shared execution.
Map every named story, intermediate exchange, alternative, service, rejection,
lore closure, scripted gate, and quest-like orphan. Record exact prerequisites,
counts/fees/rewards, source alternatives, personal versus supplied materials,
recipient/branch/attempt policy, and cross-zone ownership. Add all guidance that
the shipped schema can safely express. Unsupported objectives need a precise
event/transaction proposal and qualification matrix, not an invented completion.

Source-comprehensive means no inspected quest interaction is left unexplained.
Gameplay-qualified requires actual committed journeys and recovery evidence.
These labels are independent. A single successful terminal exchange proves
neither every branch nor every historical prerequisite.

## Repair reporting and news handoff

Native zone and quest repairs must be easy to identify independently of
journal authoring, audit-tool corrections and proposed capability work.
Use a separate, clearly named `fix` commit where practical. Every implemented
repair belongs in the PR's **Zone and quest repairs (news)** section with:

- Zone and affected quest or interaction, plus the fix commit.
- Concrete player trigger and before/after behavior.
- Validation performed and remaining material limitations.
- A short player-facing news sentence stating only the shipped improvement.

For example, a repaired missing exit should name the restored route and
tested access; a proposed exit repair is still a pending finding. Preserve
the source evidence and builder decisions in the dossier. Do not present
new journal hints, static source review, unsupported objectives or planned
fixes as repairs that players can already use. Accounting guards remain
until their required qualification is complete. The Mini Zones checkpoint
below ships no native zone/quest content repair.

Existing completed feature fixes are listed explicitly for the PR/news handoff:

| Fix reference | Shipped behavior | News treatment |
| --- | --- | --- |
| [b0e4ea60a](https://github.com/Community-Duris/Duris/commit/b0e4ea60a9937da5fd1789edd43402cb59672ef5): quest journal naming | Journals/dailies qualify duplicate area names with the source area, and lookup accepts those exact qualified alternatives. Previously the ambiguity prompt could request a full name that was still ambiguous. | Player-facing: “Quest journals now distinguish areas that share the same name.” Feature/journey lookup regression evidence is recorded in the qualification document. |
| [b0e4ea60a](https://github.com/Community-Duris/Duris/commit/b0e4ea60a9937da5fd1789edd43402cb59672ef5): quest state integrity | Save/recovery validation rejects missing required metadata and mismatched observation IDs; later accounting integration preserves caller-owned deletion and retry obligations. | Player-facing: “Zone quest progress saving and recovery now reject inconsistent records.” SQL/flat-file and feature regression evidence is recorded in the qualification document. |
| [492c6f1bd](https://github.com/Community-Duris/Duris/commit/492c6f1bd7635f22782888a3567763b7b90a39eb): shared prototype audit | Builder source lookup includes active administrative-area prototypes, resolving valid paper 5 without adding discoverable ownership. Production regression verifies both facts. | Builder-facing audit correction. The Tharnadia map exchange already existed; do not announce a repaired paper quest. |
| [7297b964e](https://github.com/Community-Duris/Duris/commit/7297b964ed3996c33a90dae4172cd0875addf174): Hall Shadow of Sin | Its gaze checks the actual opponent for Freedom of Movement rather than the callback actor. Periodic null actors no longer crash; bystander protection is not consumed, and target text substitutes correctly. | Player-facing: “The Shadow of Sin now correctly checks its opponent's Freedom of Movement, fixing a combat crash and misleading target text.” Actual procedure/null/unrelated-actor/effect/immunity regression and server build passed; live gameplay qualification remains open. |
| [b28262d8c](https://github.com/Community-Duris/Duris/commit/b28262d8cc8dfe6156df70f162981399d18be717): Halfcut crossbow ambusher | Setup registers periodic scheduling without firing; pulses keep three lanes/four bolts/player-only targets. Runtime identity checks stop volleys after death, movement, removal or storage reuse; missing lanes do not hide later lanes. Unbound procedures remain unbound. | Player-facing: “The Halfcut Hills kobold crossbow ambusher now fires on its scheduled pulses and stops interrupted volleys safely.” Actual-procedure regression passes and original fails setup; maintained build/format pass. Live balance/reset/accounting journey remains unqualified. |
| [348eccdf3](https://github.com/Community-Duris/Duris/commit/348eccdf3261e62aa8984ac0868b98adfa815a06): Halfcut struck-player warning | The crossbow's direct warning uses TO_CHAR for its player actor/recipient, so the shared audience filter delivers it. Previously TO_VICT suppressed that message; the room warning and four-bolt damage remain unchanged. | Player-facing addition: “The Halfcut Hills crossbow warning now reaches each struck player.” Audience regression fails the preceding procedure and passes with the fix; maintained build/format pass. This is a separate actual repair from journal authoring. |

## Progress

| Priority | Zone | Source story map | Deployable journal | Remaining qualification |
| ---: | --- | --- | --- | --- |
| 1 | Twin Towers Forest | [Comprehensive source dossier](zone-stories/TWIN_TOWERS_FOREST.md): 84 Q contracts, 58 M blocks, three special implementations, 30 assignments, 345 reset commands | Revision 3: ten stories/requests, twelve support services, forty rejections; optional belt preparation and complete addressable topic guidance | Active reset sources; flower/access journey; learned topics; animal birth/decay/lineage; atomic mixed fees; full alternative journeys; shared client projection |
| 2 | Plains of Life | [Comprehensive source dossier](zone-stories/PLAINS_OF_LIFE.md): Q-free tutorial, all four specials, 21 reset commands, creation tag and travel semantics | Revision 2: full route, optional sign aids, both racewar aliases, encounter guidance; no invented terminal receipts | Active reset scenery; transactional tag/sword; validated travel; durable scripted objectives; played failure/restart cases |
| 3 | Ailvio | [Comprehensive source dossier](zone-stories/AILVIO.md): 116 Q contracts, 160 addressable and 35 ambient M responses, five local procedures, shared fishing/bandaging/forage/pet/home rules, 314 reset commands | Revision 2: 22 stories/requests, 17 services; all 78 fish pairs grouped into one story; optional medicine/note/jar/seal routes; readable topic guidance | Script map/search grants; successful bandage event; active forage; missing eye source; actual NPC closure; fish grant result and personal source; confirmed hometown departure |
| 4 | Braddistock Mansion | [Comprehensive source dossier](zone-stories/BRADDISTOCK_MANSION.md): both Q contracts, one addressable and two ambient M responses, maul procedure, 186 reset commands; other same-name area's level gate distinguished | Revision 2: final story plus pet-rescue service; optional key/rescue steps for supplied collars | Active key/container reset sources; hidden-exit journey; rat-purge/later-payment prose decision; personal recovery; optional maul effect evidence |
| 5 | Breale | [Comprehensive source dossier](zone-stories/BREALE.md): six Q contracts, six M responses, nine town-special assignments, five shops, 399 resets, full Passage of Clarity puzzle | Revision 2: six distinct achievements; optional key and earlier Triad receipts; source/clue guidance and exact rewards | Active item/shop sources; accepted puzzle/topic events; all-stage Triad family; bracelet/escort decision; reward-spell prose review |
| 6 | Abandoned Elven Homestead | [Comprehensive source dossier](zone-stories/ABANDONED_ELVEN_HOMESTEAD.md): four Q contracts, six M/MA responses, 138 resets, two key targets, shared speech doors and three item teleports | Revision 2: two achievements plus two services; optional egg/access/statue steps; source and post-reward key guidance | Active material/scenery sources; live access and confirmed travel; potion lineage; narrated elf closure; accepted tapestry examination |
| 7 | Lord Krimeneha's Mansion | [Comprehensive source dossier](zone-stories/KRIMENEHAS_MANSION.md): nine Q contracts, nine M responses, 176 resets, all fragment rescues, roaming staff source and exact key/container targets | Revision 2: eight achievements plus staff service; optional access/preparation/family receipts; distinct gardener guidance | Active reset sources; replacement fragment lineage; roaming NPC/competing staff consumers; all-stage household and servants' blessing decision |
| 8 | Bastine Castle | [Comprehensive source dossier](zone-stories/BASTINE_CASTLE.md): 14 Q contracts, four M responses, 428 resets, twelve promotion sources, prince/tower key and complete bounded Morlanthra continuation | Revision 2: twelve independent commissions in narrated order, prince and Victor requests; optional promotion/tower history; precise external sources and rewards | Active local/foreign sources; replacement hide lineage; all-stage rank campaign; owned cross-zone rescue; term/concealment decisions |
| 9 | Pine Hollow | [Comprehensive source dossier](zone-stories/PINE_HOLLOW.md): seven Q contracts, ten M responses, 352 resets, roaming dragon sources, both mine keys, shared hazards and three external consumers | Revision 2: seven independent achievements; exact skin counts/types, source/availability guidance, real rewards and mine exploration | Active source generation; two-skin/one-cap conflict; holding-room availability; explicit branch attempts; hazard/access and owned foreign journeys |
| 10 | Quietus Quay | [Comprehensive source dossier](zone-stories/QUIETUS_QUAY.md): 16 Q contracts, 14 M families, four special assignments, five shops, 216 resets; reachable world-quest/ship/shared money execution | Revision 2: four missions plus seven services; optional briefings, ten contacts/all native topic families, exact proof/rewards and rare-source guidance | Active resets; filled-badge tree/parent policy; rare episode availability; world-quest refunds/exact attempts; coordinated ship coin/epic settlement |
| 11 | Torg | [Comprehensive source dossier](zone-stories/TORG.md): 15 Q contracts, 15 M families, three literal assignments, table-driven teacher/forge reachability, two shops, 597 resets, complete local death specials and bounded foreign continuations | Revision 2: twelve achievements plus two services; optional hide/rose/locket preparation, twelve contacts/all topics, exact rings/relics/chisels and access guidance | Active resets; holding/cap availability; confirmed invasion; committed random scroll/heart lineage and expiry intent; service/unused-content decisions; owned foreign journeys |
| 12 | Vast Hidden Grove | [Comprehensive source dossier](zone-stories/VAST_HIDDEN_GROVE.md): 15 Q contracts, 12 M families, 100 rooms, 41 mobiles, 72 objects, seven shops, 152 resets and thirteen scenery teleports | Revision 2: three equipment stories, two bird requests and ten services; optional recipe history, fourteen contacts/all topics, exact inputs/rewards and shared-material guidance | Active resets; three mixed fees; Valin/miner roaming/holding; ordinary dead-mouse pickup; invalid inn target; loaded access versus prose; confirmed source/travel/recipe lineage |
| 13 | Winterhaven | [Comprehensive source dossier](zone-stories/WINTERHAVEN.md): all 221 Q/QA contracts, 350 M/MA responses, 599 rooms, 314 mobiles, 483 objects, thirteen shops, 1,284 resets, 92 literal assignment candidates and bounded foreign continuations | Revision 2: 135 requests, 84 services, two exclusions, 92 contacts/152 nonempty topic families, 38 optional preparation checks and exact source/key/timer/recipe guidance | Active resets; 49 mixed fees; foreign sources/recipient availability; timed births/decay; random scroll/gift lineage; physical access, source gaps and selected prose/power/service repairs |
| 14 | Shairak and Smokeveil Forest | [Comprehensive source dossier](zone-stories/SHAIRAK_AND_SMOKEVEIL_FOREST.md): eleven Q/QA contracts, 32 M families, 100 rooms, 74 mobiles, 59 objects, one shop, 435 resets, table-driven support roles and bounded Tezcat/Alatorin/Raxthan continuations | Revision 1: ten achievements plus one service, nine contacts/all topics; exact simultaneous hearts, competing trophies, ordinary animal recovery and optional helm route | Active reset/shop supply; cap-one/recipient availability; committed source versus gift; owned foreign journeys; all-stage cure/reunion and supported forge/teaching |
| 15 | Twin Keeps of Devastated Tharnadia | [Comprehensive source dossier](zone-stories/TWIN_KEEPS_OF_DEVASTATED_THARNADIA.md): 30 Q/QA exchanges, 86 M/MA blocks, 451 rooms, 123 mobiles, 121 objects, 1,614 resets, Zorana handler and bounded foreign continuations | Revision 1: 29 achievements plus one service, 24 contacts/all 37 addressable families, fifteen optional producer checks, distinct shards, physical receipt and complete crystal/message/access guidance | Active sources; mixed potion fee; confirmed switch/key/travel/retrieval; recipient/epic-reset episodes; conditional sources, personal versus supplied proof and selected prose repairs |
| 16 | Bloodstone Keep | [Comprehensive source dossier](zone-stories/BLOODSTONE_KEEP.md): 65 Q/QA exchanges, 158 native blocks, 918 rooms, 255 mobiles, 300 objects, 34 shops, 1,624 resets, complete local special code and bounded foreign producers/consumers | Revision 1: 31 achievements, 32 services/two exclusions, twenty contacts/all 74 addressed families, 35 optional producer checks; complete local, artifact and alchemy guidance | Active supply; absent ordinary makers/missing components; exact source/access/transform episodes; mixed earring fee, unfinished Pellops/orphan gameplay and selected prose/assignment repair |
| 17 | Neverwinter Woods | [Comprehensive source dossier](zone-stories/NEVERWINTER_WOODS.md): seven Q/M blocks, 335 rooms, 38 mobiles, 75 objects, three shops, 424 resets, all 28 assigned routines and complete mirror-maze topology | Revision 1: five native bindings → one story/achievement/daily unit; seven contacts/both topic families; optional bridge wand, exact runes and current ruby-eyepatch reward | Active sources, cap-one/current recipient, selected-reward terms, checked maze topology, confirmed passage, source/gift history and orphan endpoints |
| 18 | The Clawed Caverns | [Comprehensive source dossier](zone-stories/THE_CLAWED_CAVERNS.md): twenty Q/eight M, 89 rooms, forty mobiles, 55 objects, 171 resets/all 106 grouped families; all five specials, 26 mobile/one object binding including computed range | Revision 1: one rainbow-delivery story, six services/thirteen excluded returns, ten contacts/all topics; optional keys and complete custom switch/death/mage/access guidance | Active stock, target-safe damage/recipient parsing, committed death-container and key/output/recipient retirement, confirmed travel and paid-clue wallet settlement |
| 19 | Defense of Longhollow | [Comprehensive source dossier](zone-stories/DEFENSE_OF_LONGHOLLOW.md): fifteen Q/63 M, 100 rooms, 77 mobiles, 74 objects, one shop, 213 resets/all 150 grouped families; zero literal assignments, one computed epic teacher and shared execution reviewed | Revision 1: four stories/five requests, five clothing services/one empty exclusion, forty-one contacts/all sixty addressed families and six optional producer receipts; exact source/access/quantity/reward guidance | Active reset supply; three cap/quantity conflicts; four mixed payments and epic teaching; fixed powers versus prose; absent rescue/healing/siege/title outcomes; confirmed source/gift and all-stage history |
| 20 | The Black Pearl | [Comprehensive source dossier](zone-stories/THE_BLACK_PEARL.md): 31 Q/QA and fourteen M, 84 rooms, 63 mobs, 98 objects, 391 resets/all 187 families, no local shops/literal specials; disabled ship and foreign keys/procedures/topology reviewed | Revision 1: two stories/twelve requests, seventeen services, 26 contacts/all topics and 25 optional checks; complete exact fragment/courier/hilt/skin/gem guidance | All campaign NPCs and entrance-key spirits held; missing invitation/sewer/scepter/chests; no reciprocal entry/disabled mobile ship; paid gadget and active generation; owned campaign discovery/reveal, replacement lineage and fixed party reward terms |
| 21 | The Ravenloft Catacombs | [Comprehensive source dossier](zone-stories/THE_RAVENLOFT_CATACOMBS.md): 37 Q/QA and 162 M/MA, 400 rooms, 98 mobs, 327 objects, two shops, 1,457 resets/all 665 families; computed shared roles and bounded foreign sources/routes reviewed | Revision 1: twelve stories/thirteen requests/eight services, one five-artifact family, 25 contacts/all 135 addressed blocks and fourteen optional checks; exact five-coin roles and three distinct skull finales | Active sources/shopping; exact repeated-container parent; key/switch/travel/source evidence; stateful cards; narrated resurrection/forms/key promises and selected mechanism repair |
| 22 | The Realm of Barovia | [Comprehensive source dossier](zone-stories/THE_REALM_OF_BAROVIA.md): nine exchanges/78 M blocks, 168 rooms, 57 mobs, 66 objects, 405 resets/all 194 families; shared roles, foreign keys/Gertruda and legacy combat/session policy reviewed | Revision 1: five stories/one request/three services, 24 contacts/all 41 addressed blocks and ten optional checks; exact collection, independent letters, heart and daughter routes | Active sources/access; quest-person lineage; ordered group combat/attempt bridge; foreign brittle gates; selected brooch/letter/orb/barricade/reveal policies |
| 23 | Lost Temple of Tikitzopl | [Comprehensive source dossier](zone-stories/LOST_TEMPLE_OF_TIKITZOPL.md): 29 Q/nine M, 234 rooms, 76 mobs, 97 objects, 340 resets/all 202 families, five assigned procedures and bounded foreign components/routes reviewed | Revision 1: two stories/two requests/25 services, nineteen contacts/all topics and 44 optional checks; exact key, distinct/duplicate ingredients and competing Orb guidance | Active sources/mode-zero availability; exact allocation/lineage; accepted lore and confirmed access/return; successful reflection/class outcomes and builder-selected crypt/prose/closure |
| 24 | The Jade Empire | [Comprehensive source dossier](zone-stories/THE_JADE_EMPIRE.md): 37 Q/four M, 340 rooms, 134 mobs, 130 objects, 583 resets/all 331 families, three shops, two literal room services/computed smith and bounded foreign proofs/routes | Revision 1: eight stories/nine requests/seventeen services/two exclusions, 38 contacts/all three addressed topics and 35 optional checks; equivalent fish and exact allocation/access guidance | Paid net/map/mixed fees, legacy mithril and deliberate forge/mining refusal; accepted fishing/source/capture/recipient, same-kind replacement, load-room encounters and real key/water/foreign travel; builder-selected belt/lore/prose repairs |
| 25 | Savannah of Broken Trusts | [Comprehensive source dossier](zone-stories/SAVANNAH_OF_BROKEN_TRUSTS.md): seventeen Q/nineteen M, 167 rooms, 59 mobs, 39 objects, 315 resets/all 105 families; shared native/access/falling/bard and bounded foreign Mitashi/Air/Hostel review | Revision 1: two stories/three requests/twelve equipment services, seventeen contacts/all nineteen raw topics and twelve optional checks | Active parts/sword stock, exact allocation, retiring appearances and well/tunnel access; source/learned-topic evidence; builder-selected absent sister/tribal endpoints and alias/prose/protection decisions |
| 26 | Alatorin - the Forge City | [Comprehensive source dossier](zone-stories/ALATORIN.md): 495 contracts/961 blocks, full local world/prototypes/shop/reset and bounded foreign/shared review | Revision 1: 253 rows, 90 achievements/82 potential dailies, 163 services/four returns, 94 contacts/all 296 addressed families, 548 optional checks | Active source/preparation/payment and typed-money admission; target/custody/random/class-gift recovery; exact alternative predicates; builder source/access/prose/slot repairs; district presentation and full-stage campaigns |
| 27 | The City of Newhaven | [Comprehensive source dossier](zone-stories/NEWHAVEN.md): nine Q/two addressed/53 ambient blocks, all local world/prototypes/four shops/252 resets and bounded foreign/shared review | Revision 1: three named outcomes/six services, seventeen contacts/twelve optional checks; exact source, fee, counterpart and unfinished-lore guidance | Active source/personal proof, five mixed payments, badge choices, tail trap/custody and actual travel/inn; builder-selected pipe/reel, collar-table, stale pool/prose and rift/prisoner endpoints |
| 28 | Faerie Realm | [Comprehensive source dossier](zone-stories/FAERIE_REALM.md): all seven Q/ten M, 211 rooms/74 mobs/123 objects/one shop, 454 resets/213 families, four local procedures and bounded foreign/shared review | Revision 1: three stories/two services/one rejection, ten contacts/thirteen optional checks; independent retiring outcomes, equivalent forge recipes and exact access/source guidance | Active source/recipient episodes; both mixed fees; held/carried keys, speech/orb travel; unserved combat helpers; targeted Fix effects; builder-selected riddle/prose/missing-target repairs |
| 29 | Verspin | [Comprehensive source dossier](zone-stories/VERSPIN.md): all twelve Q/QA/nine addressed/31 ambient blocks, 200 rooms/79 mobs/57 objects/five shops, 389 resets/206 families, two literal services, computed teachers/inn and bounded foreign/shared review | Revision 1: six stories/six services, eighteen contacts/eighteen optional checks; exact five-count/color inputs, optional producer history, foreign proof ownership and actual access guidance | Active source/cap/retirement qualification; five mixed fees; virtual stat/wallet/save effects; denied crew-payment ordering/ship settlement; XP cap policy; builder-selected sign/cap/clue/lore repairs |
| 30 | Ship Yards | [Comprehensive source dossier](zone-stories/SHIP_YARDS.md): all 124 blocks/26 exchanges/twenty addressed/78 ambient, 229 rooms/102 mobs/46 objects/seven shops, 719 resets/265 families, eight literal services and bounded foreign/shared review | Revision one: nineteen outcomes/six services, thirty-two contacts/thirty-four optional checks; supplied proofs, optional briefing/note, exact quantities/kinds and two crate-price alternatives | Active source/slot/cap/dispersal/retirement and travel qualification; four missing exits/ocean viper ecology; builder-selected count/clue/reward repairs; fishing ownership publication and wallet/epic/ship/crew settlement |
| 31 | Ultarium | [Comprehensive source dossier](zone-stories/ULTARIUM.md): all 22 blocks/seventeen exchanges/five addressed, 268 rooms/82 mobs/71 objects/two shops, 338 resets/227 families, three literal assignments, computed teachers/shared effects and bounded foreign continuation | Revision one: seven outcomes/eight support rows, twenty-three contacts/twenty-two optional checks; competing distinct souls, same-name kinds, keys, supplied proof and foreign ownership | Active rare/holding/capped/nested/roaming sources/appearances; source/gift/episode/access/travel/effect/actor-state adapters; paid rename/pet/epic lesson settlement; balanced soul/clue/name/claim repair |
| 32 | Surface Realm | [Comprehensive source dossier](zone-stories/SURFACE_REALM.md): all 141 blocks/31 exchanges/36 addressed, 160,004 rooms in 562 prose/56 metadata/311 exit groups, 234 mobs/78 objects, 1,736 resets/320 families, eight literal assignments/104 computed teachers and bounded foreign evidence | Revision one: seventeen outcomes/six support rows, 37 contacts/31 optional checks; exact competing campaign materials, repeated supplies, distinct bass, actual tomb continuation and supplied routes | Active source/recipient/episode qualification; accepted treant/fishing/helper/rift/actor/region and paid ship/class/fee events; targeted two-edge/clue repair and deliberate bracer/invasion/descent policy |
| 33 | Tharnadia | [Comprehensive source dossier](zone-stories/THARNADIA.md): all 44 native blocks/nineteen exchanges/24 addressed families, 584 rooms/176 mobs/206 objects/nineteen shops, 1,480 resets/661 families, 31 literal assignments/computed teachers and bounded foreign/shared review | Revision two: eight outcomes/eight services/one typed-food exclusion, 26 contacts/23 optional checks; exact toys, supplied medicine finale, paper-map correction and independent commissions | Active source/container/search/appearance qualification; accepted first-source/dialogue/access/healing/lesson/pet events; typed/coin-only and paid town settlement; builder-selected missing-mobile/clue/source/claim repairs |
| 34 | Mini Zones | [Comprehensive source dossier](zone-stories/MINI_ZONES.md): all 21 blocks/seven exchanges/eleven addressed families, 300 rooms/125 mobs/124 objects/five shops, 548 resets/358 families, six literal assignments and bounded shared/foreign review | Revision one: three outcomes/four services, eighteen contacts/sixteen optional checks; exact maze/cutpurse, distinct sword and five-crystal guidance | Active source/recipient/episode and learned access/travel/effect evidence; guarded fee/charm/pet settlement; builder-selected missing-edge/identity/claim/prose repairs with clear fix/news reporting |
| 35 | City of Torrhan | [Comprehensive source dossier](zone-stories/CITY_OF_TORRHAN.md): all 46 blocks/26 exchanges/eighteen addressed families, 289 rooms/152 mobs/125 objects/six shops, 447 resets/324 families; both literal and table/automatic bindings | Revision one: eight outcomes/fourteen services/four refusals, 23 contacts/28 optional checks; exact potion, ring, sword, crown and cloak cycle guidance | Active source/recipient episodes, competing materials, lineage/property presentation, access/choice and actual actor-state events; selected throne/stock/prose repairs require explicit fix/news reporting |
| 36 | Golden Hall of the Crown | [Comprehensive source dossier](zone-stories/GOLDEN_HALL_OF_THE_CROWN.md): all 42 blocks/sixteen exchanges/seventeen addressed families, 300 rooms/91 mobs/106 objects/two shops, 534 resets/351 families, four literal assignments and automatic/shared bindings | Revision one: nine outcomes/seven services, 27 contacts/34 optional checks; exact note/key/totem/sword guidance, supplied finale and three independent rescues | Active source/recipient/returned-key lineage, three mixed fees, accepted access/travel/actor and pegasus events; builder-selected boulder/teacher/note/route/prose repairs require separate fix/news reporting |
| 37 | Ashrumite Village | [Comprehensive source dossier](zone-stories/ASHRUMITE_VILLAGE.md): all 25 blocks/twelve exchanges/thirteen addressed families, 153 rooms/53 mobs/65 objects/twelve shops, 275 resets/181 families and fifteen literal/shared bindings | Revision two: twelve support services, sixteen contacts/21 optional checks; actual five-copy and same-name material guidance, complete intended/current repair matrix; no authored quest/daily units | All payments guarded; missing disc/two rewards/teacher equipment, stale eastern boundary and builder-selected crafting/price/guard/merchant/pet/prose work; source, paid-lore and full crafting lineage unqualified |
| 38 | The Hall of the Ancients | [Comprehensive source dossier](zone-stories/THE_HALL_OF_THE_ANCIENTS.md): all 25 blocks/eleven exchanges/fourteen addressed families, 247 rooms/55 mobs/53 objects/one shop, 395 resets/153 families, twelve literal procedures and shared dispatch | Revision one: five outcomes/four services/two elder exclusions, 27 contacts/23 optional checks; actual sixteen-piece and ten-piece recipes, supplied foreign proof and source/access guidance | Separate Sin actual-opponent/null-actor combat fix ships; three mixed fees and oversized belt guarded; elder ordering, source budgets/chance, collector transaction, early death arming, GET settlement and campaign/access qualification pending |
| 39 | Sarmiz'Duul | [Comprehensive source dossier](zone-stories/SARMIZ_DUUL.md): 21 blocks/eight exchanges/thirteen raw addressed families, 583 rooms/57 mobs/61 objects/four shops, 535 resets/199 families, five literal procedures plus foreign custom moonstone execution | Revision one: eight outcomes, 24 contacts/nineteen optional checks; exact courtship/relic/royal/conspiracy recipes, supplied foreign proof and custom guidance | No native repair ships; active-accounting pirate core guard retained; recipient targeting, partial seed/assembly/payment ordering, unfinished multi-core behavior, missing mount stock, potion prose/type and campaign/access/episode qualification pending |
| 40 | Duke Delwyn | [Comprehensive source dossier](zone-stories/DUKE_DELWYN.md): 53 blocks/eleven exchanges/nine addressed + 33 ambient families, 207 rooms/96 mobs/41 objects, 278 resets/170 families; automatic teacher/shared trap/falling and reciprocal surface boundary | Revision one: six outcomes/five paid services, nineteen contacts/seventeen optional checks; exact bell/cog, four-stage banner, knight and two-item warning guidance | No native repair ships; mixed/coin payments guarded, live source/container lineage, trap/fall/safe return, occasion/advertised service/peaceful handover and campaign decisions pending |
| 41 | Home of the Divine | [Comprehensive source dossier](zone-stories/HOME_OF_THE_DIVINE.md): all 52 blocks/32 exchanges/twenty raw M families, 122 rooms/62 mobs/83 objects/one shop, 240 resets/156 families; shared item teleport, shop/forge and epic teacher | Revision one: twenty outcomes/twelve services, 27 contacts/48 optional checks; exact tokens/hearts/treasures, independent crafting/access and foreign returns | No native repair ships; missing bounty reward/preflight, guarded scale/six mixed fees, two-copy weapon supply, rare wandering, access/trap/fall/container and recipient episodes pending |
| 42 | The Halfcut Hills | [Comprehensive source dossier](zone-stories/THE_HALFCUT_HILLS.md): 28 blocks/thirteen deliveries/fifteen addressed, 470 rooms/83 mobs/sixty objects/one shop, 413 resets/177 families; bound crossbow plus shared switch/teleport/inn/shop/epic teacher | Revision one: thirteen independent outcomes, nineteen contacts/twenty-four optional checks; exact miners/badges/note and competing trophies | Separate crossbow scheduler/continuation fix ships; missing drow reward/preflight, source/recipient/container/wandering/access, real home arrival and branch/attempt campaign decisions pending |
| 43 | The Scorched Valley | [Comprehensive source dossier](zone-stories/THE_SCORCHED_VALLEY.md): twenty blocks/nine deliveries/eleven addressed; 132 rooms/sixty mobs/fifty-five objects/no local shop, 211 resets/118 families; three literal assignments and shared keys/container/rifts/artifact/combat | Revision one: nine outcomes, twenty-two contacts/twenty-two optional checks; exact foreign proof and blood-to-four-colors-to-necklace | No native repair ships. Advisor source/recipient episode, foreign encounters, nested/access evidence, Yeenoghu dispatch/safety/balance, truthful clue/departure and rod/state/finale design pending |
| 44 | Court of the Muse | [Comprehensive source dossier](zone-stories/COURT_OF_THE_MUSE.md): fifteen blocks/nine deliveries/six addressed; ninety-nine rooms/thirty-six mobs/forty-three objects/one shop, 209 resets/ninety-two families; shared teachers, doors, traps, portals, fall/current | Revision one: nine outcomes, twenty-five contacts/sixteen optional checks; four distinct seasonal producers/admission, twelve scales and exact independent requests | No native repair ships. Cave/pouch/trap decisions, competing dew/source/retiring episodes, actual access/key destruction/reset return and seasonal/audience endpoints pending |
| 45 | Valley of the Snow Ogres | [Comprehensive source dossier](zone-stories/VALLEY_OF_THE_SNOW_OGRES.md): fifteen blocks/nine exchanges/six addressed; one hundred rooms/forty-four mobs/thirty-eight objects, two hundred resets/eighty families; twelve literal assignments/shared switch and bounded surface source | Revision one: seven outcomes/one service/one refusal, sixteen contacts/fifteen optional checks; distinct shards, exact trophies and six-hide/full-fee armor | No native repair ships. Stalk/hide generation/renewal, mixed payment, control/access/foreign owner and burn/golem/toss/equipment fixes/endpoints pending |
| 46 | The Mountain Valley of Dawndale | [Comprehensive source dossier](zone-stories/THE_MOUNTAIN_VALLEY_OF_DAWNDALE.md): seventeen blocks/thirteen exchanges/four addressed; 150 rooms/sixty-nine mobs/sixty-six objects, 321 resets/175 families; automatic switches/shared teleport, bounded foreign sand/forge/book | Revision one: nine outcomes/three services/one referral, twenty-seven contacts/thirty-one optional checks; refugee preparations, rival bundles and foreign flute return | No native repair ships. Guarded coin purchase/mixed lens fee, key availability/shared caps/source content, fossil/Ender repairs and actual camp/tunnel/curse/install endpoints pending |
| 47 | The 222nd Layer of the Abyss | [Comprehensive source dossier](zone-stories/THE_222ND_LAYER_OF_THE_ABYSS.md): all 48 blocks/23 exchanges/25 addressed; 183 rooms/117 mobs/113 objects, 378 resets/257 families, six literal procedures and bounded foreign routes/producers | Revision one: 22 outcomes/forty contacts/39 optional checks; 19 potential dailies, equivalent old-leash aliases, exact competing bundles and optional histories | No native repair ships. Phase insertion blocker, lake/generator/legend creation, dormant Ebb, Flow effects, clue/departure and actual scoped world-effect plans pending |
| 48–220 | Remaining roadmap | Pending comprehensive review; earlier rough proposals and complete Q classification remain useful evidence | Existing authored maps/native fallback retained | Work through original queue; record each reviewed family and custom dependency |

The next area is The Minizones of the Surface (`surfacemini`).
Source and gameplay qualification remain distinct throughout the full queue.
Do not advance a zone's status merely because the map parses, the Q denominator
matches, or a candidate item graph was extracted.

## Findings that expand or reorder the implementation plan

| Finding | Status / scope | Concrete next action |
| --- | --- | --- |
| ZSQ-JUIBLEX-PHASE / GENERATION | First Juiblex attempts already-linked room insertion, which the current handler rejects; second phase, vault seal and invisible key remain staged. Lake can retain first allocation if second fails. Generator/consumed legend creation use unchecked results; prototypes exist. | Separate identity/room-preflight/unlink/placement repair with original-failing procedure tests and public access/reset/recovery journey. Qualify staged creation cleanup and random consumable output/source recovery without changing ambient population or silently guaranteeing both tale halves. |
| ZSQ-JUIBLEX-RECIPIENT / EFFECT / CONTENT | Uz refill and either equivalent old-leash offer retire recipients; first Marvin body competes. Medallion clue names merchant but source is elder. Final Marvin D0 contradicts fade/color text. Ebb unbound; Flow may restore nothing and still cool down; inventory vibration suppressed. Lich/rebirth/escape/escort/victory are narrative. | Builder-selected clue/departure/ability changes with separate fix/news proof; surviving-recipient/alias and exact current proof policy. Extend accepted effects/cooldowns, containers/shared crystal, source versus handoff and scoped AND/world/party endpoints. No actual native repair ships. |
| ZSQ-DAWNDALE-SOURCE / KEY / PAYMENT | Coin-only 250-platinum city-key purchase is guarded; admission retains it. Office/alcove keys have 100-percent break declarations. Mixed sand/25-platinum lens service is guarded. Captain bundles each consume two sunlight vials and the book; three vials declared, book shares global cap one with an administrative source. Sand is a real Aravne death output. | Atomic accepted purchase/mixed settlement and key availability/office-alcove destruction; qualify normal/forced renewal and recovery before intentional cap/isolation repairs. Extend loose container recovery, liquid content/volume and personal source-versus-handoff evidence without bypassing active generation guards. |
| ZSQ-DAWNDALE-CONTENT / WORLD EFFECTS | Fossil referral has no reviewed exact consumer. Ender declares `_vict_msg`, both packed paths expect `_victim_msg`. Refugee BOOM/camp/followers, drow tunnel, captain ritual/curse and telescope installation lack accepted terminals; great lens is not described as broken. Secret open rock switches clear each direction separately. | Builder-selected truthful fossil endpoint; separate message fix with all-audience proof. Define scoped actors/party, control/camp/tunnel/effect/reading/install endpoints, return and idempotent recovery before deeper credit. No actual native repair ships; future fixes need separate commits and news. |
| ZSQ-SNOGRES-SUPPLY / FEES / OWNERSHIP | Stalk 87719 lacks an active producer. Four remorhaz have three hides/cap three, versus six-copy armor; inactive brass-old-1 does not supply the live world. Leppts is supplied from Surface 660001 and wanders, owning a Snow contract. Full fee is 2,500 platinum; mixed payment guarded. Mode zero alone does not settle renewal. | Choose sufficient intended stalk/hide generation and caps; qualify accepted renewal, retained/supplied proof and exact distinct weapons/full atomic payment. Keep active reset guards; improve already-discovered foreign owner referral without auto-discovering it. Preserve service/refusal non-credit classification. |
| ZSQ-SNOGRES-ACCESS / ACTORS / EFFECTS | Push rock clears forward blocked state but secret/closed and a separate pit-fiend guard remain. Two real falling rooms have a lake alternative. Remorhaz burn damages the owner; golem counts 87743 but spawns 87734 and retreats to raw vnum 87798. Berserker is disconnected from normal combat; axe/whip and lich hunt are connected. Leggings move equipment directly. | Qualify actual controls/reveal/open/return. Separate target, helper/prototype/real-room/preflight and repop guard repairs; repair cadence/selection/continuation before enabling throws. Design accepted actor/effect/custody/reunion episodes with recovery and focused proofs. No native fix ships; later fixes need separate commits and explicit news. |
| ZSQ-COURT-ACCESS / CONTENT / TRAPS | Cave locket clues actually key to book/dew; shared lookup permits both. Dew is also consumed for Spring. Pouch description conflicts with whole-flower text. Spring/Summer/Autumn tokens, autumn mask and diamond stud declare damage codes absent from trap dispatch; Winter has real cold pickup damage. Forward admission key breaks; reverse key zero is normally opened reciprocally but reset/restart is separate. | Choose truthful cave/pouch wording and intended supported trap data in separate fix/news commits. Qualify reveal/unlock/open, competing-use ordering, exact GET/retry/charge persistence, damage continuation, key destruction and ordinary versus restored-lock return. Do not infer all visits are stranded or treat unreset world-file four as secret. |
| ZSQ-COURT-SOURCES / RITUALS / AUDIENCE | Four exact tokens accept supplied materials without personal favors. Fifteen koi-scale declarations support twelve; fisherman retires. Two friends share one contract; eight initial essence carriers do not cover every later matching mob. Seasonal rituals, fishing lesson, soul extraction, Muse audience and elk-heart finale lack accepted endpoints. | Qualify live source/recipient generations and personal lineage separately; preserve nine independent outcomes. Add explicit actor/effect/access/audience/finale transactions and AND/attempt policy only for builder-selected objectives. Shop and shared class-matched level teachers remain context, not extra credit. |
| ZSQ-SCORCH-ACCESS / EPISODES / OWNERSHIP | Five distinct keys lead to a locked, pickproof blood chest. Final key holder is advisor/recipient for three other requests. Bodyguard/recipients have foreign holding routes; exact Grog/Zuzon heads come from foreign zones. | Qualify key/open/container ancestry and source/recipient replacement episodes. Physical-room discovery is required and normal arrival supplies it; the immediate hint still uses the physical journal. Plan an owning-journal referral without remote discovery or changed credit, preserving supplied proof and contract ownership. |
| ZSQ-SCORCH-DISPATCH / CONTENT / STATE | Yeenoghu rejects CMD_MOB_COMBAT and declines periodic registration; outer whirlwind/fetid traversal needs callback continuation review before activation. Head clue promises either commander but only one supplies accepted head; Q206 says departure without D. Captive/resummon/society/curse and rod/finale are narrative or lack endpoints. | Separate dispatch/safety/balance fix and truthful text or builder-selected actual alternatives/lifecycle. Define real entity/effect/assembly/finale transactions before new credit. No native repair ships; every implemented repair requires clear fix/news reporting. Curated contact topics cover all families without increasing the current limit. |
| ZSQ-HALFCUT-REWARD / RESCUE / EPISODES | Drow request literally names absent item 25000 after potion 27056; reward item admission lacks loadability preflight. Four jar declarations support three miners/Bartis; acceptance retires recipients without home arrival. Bartis badge bundle must precede final jar in the same episode; faction scalps compete with six-proof delivery. | Builder selects intended reward; add pre-consumption/new-credit preflight preserving frozen obligations, then a separate native recipe fix. Qualify jar/container/scalp source generations and retiring NPC episodes. Select narrative rescue versus atomic movement, branch/attempt/all-stage policy without rejecting supplied independent proof. |
| ZSQ-HALFCUT-PROCEDURES / ACCESS | Bound crossbow setup/periodic bug and unsafe continuation are repaired in a separate fix commit. Defender/blowgun procedures remain unbound; blowgun has a similar setup issue. Real switches/teleports use grab, say, enter and pull, with same-name stones; inn/teacher have shared bindings. | Qualify live volley balance/reset/combat accounting; choose placement/balance before enabling unbound hazards. Test numbered stone lookup, switch/opening/key/travel/fall and safe arrival, four jar ancestry and wandering encounters. Review distinct scalp aliases and spelling as separate future fix/news work; do not invent access/history achievements. |
| ZSQ-DIVINE-REWARD / SCALE / SETTLEMENT | Relazier bounty names missing object 31341; native admission does not validate reward-item loadability before offering consumption. Generic scale 392 has an inactive-accounting-only death source. Six mixed crafting fees stay guarded; Wicks's item-only cash rewards already have a durable path. | Builder selects intended bounty reward. Add pre-consumption item preflight/new-credit guard while preserving frozen obligations, then a distinct tested native recipe fix. Qualify recoverable scale death issuance and atomic fee/XP/output settlement; do not conflate reward cash with paid services or weaken guards. |
| ZSQ-DIVINE-SUPPLY / ACCESS / EPISODES | Final forge route needs two cap-one Riser/shard copies and 300000 copper. Real enter/touch routes, two different keys, closed P containers, trapped claw, falls, rare holding/dead-end wandering and retiring recipients affect supply; token and Relazier branches compete. | Qualify exact UID allocation, empty-zone resets, actor survival/arrival, parent/location lineage, recipient episodes and foreign ownership. Builder selects scarcity/branch policy; preserve supplied independent receipts. Safe travel, freeing prisoners, personal kills and full campaigns need their own accepted terminals. |
| ZSQ-DIVINE-CONTENT / SHARED ROLES | Four heart extra-description aliases name the wrong heart; several success strings and rare-room prose are empty. Emition shop/smith and Snent epic teaching have real shared bindings despite no local literal special. Reln's thousandfold jade fee matches dialogue. | Add truthful aliases/messages and qualify holding stranding before any builder-approved layout change. Keep price and binding facts intact. Actual later repairs require identifiable fix commits and prominent trigger/before-after/proof/news entries; this journal checkpoint ships none. |
| ZSQ-MINI-ACCESS / EPISODES / SOURCES | Three independent outcomes; optional knight history and supplied sword pieces; hidden nested letter/hilt; exact maze route and cutpurse door. Nine outgoing targets are absent, while city/tavern/outpost have valid surface entries. | Preserve supplied native inputs. Qualify actual reset/shop/GET/P-parent and recipient generations, accepted phrase/unlock/open/arrival and explicit all-stage episode policy. Builder selects intended missing-edge repairs, reported separately when shipped. |
| ZSQ-MINI-FEES / PETS / CONTENT | Four six-item plus 400,000-copper armor services are guarded. Ghostly shop takes coins rather than narrated marks; prose claims five types but four recipes exist. Pet claim restoration is commented out. | Coordinate exact material/wallet/output receipt and mount/owner/ticket settlement before enablement. Choose truthful armor/shop/sword prose or balanced new mechanics. Treat these as pending repairs, with player-facing PR/news entries only after implementation and validation. |
| ZSQ-MINI-IDENTITY / DISPATCH | Dryad assignments/branch/current room identities disagree; navigator attaches to an insect swarm; miner really owns random world quests. Magik receives CMD_MELEE_HIT 1000 and its dispel branch is reachable. Legacy room M metadata is ignored. | Builder chooses intended bindings/destinations; bound dryad messages and freeze validated actor/follower state. Preserve real generated tasks. Add accepted targeted effect evidence if desired; do not repair an already reachable Magik dispatch or invent a metadata mechanic. |
| ZSQ-PEARL-PLACEMENT / MISSING-SOURCES | All twenty campaign NPCs and four entrance-key spirits load only in exitless 142200. Invitation, sewer fragment, scepter and several chest/key prototypes lack ordinary placement; a matching key does not seed its chest or contents. | Builder chooses restored permanent expedition versus retired staff-event content. Select exact named-zone placements/reset owners/caps, add complete item/container sources and preserve native IDs, balance and supplied stock. Qualify directed first-copy/recipient journeys and disappearance/recovery before promising availability. |
| ZSQ-PEARL-ENTRY / SHIP / ACCESS | Static exit goes outward to surface mountains without a reviewed reciprocal entrance. Mobile hunter ship/linking is disabled; early keys are held, later key-zero doors need approved routes, and 82 room descriptions are empty. | Choose static entry or coordinated mobile restoration; resolve loaded map/interior topology, key/source/lock policy and critical navigation text. Keep Ghalasax separate from the wreck's skeletal dragon; qualify confirmed arrival, traps/secret doors and recovery without inventing a personal kill. |
| ZSQ-PEARL-OWNERSHIP / TRANSFORMS / REVEAL | Foreign contact encounters can persist under physical discovery, but hints link the current area's journal while the owned Black Pearl journal remains locked until wreck discovery. Three same-named fragment replacements and two letters to Lyle have distinct recipe roles; nine returns are meaningful guidance. | Add explicit owned campaign introductions/reveal and correct journal hints, retaining physical discovery and discovered-zone daily gating. Capture exact transform UID lineage; qualify supplied/spent pieces, nine-root allocation, all-stage attempts and pre/post-discovery receipt replay. |
| ZSQ-PEARL-REWARD-TERMS / GADGET | Final skin yields fixed six large/six small gems to the actor, rather than party allocation. Six class-themed hunter trades have no giving class check and empty success strings. Travel cash reward already works; gadget coin-only purchase is separately unavailable. | Builder confirms fixed versus per-recipient terms and adds truthful hunter text. Qualify selected item/XP rewards, capacity/frozen party/replay; add recoverable coin purchase without confusing reward coins with fees or awarding extra service achievements. |
| ZSQ-LONG-SUPPLY / PHYSICAL-RECIPES | Boots need four cap-one skins, scarf needs two cap-one furs and Viper's Delight needs two cap-one sacs. Bracer/moonstone, five heads, three vials, raw/filled outputs and rooted/loose plants are distinct; personal producer receipts cannot replace spent stock. | Builder chooses bounded cap/source repair preserving chance, price and difficulty. Qualify held first copies, competing consumers, live/recovered/forced reset episodes, distinct UID allocation and supplied higher outputs without producer history. |
| ZSQ-LONG-TERMS / POWERS / ORPHANS | Boots dialogue says 15 gold versus native 20. Green potion lacks narrated darkvision; poison outputs are fixed potions or take-only armor without local dipping/randomization. Wife rescue, templar aid, Rolane's empty Q and siege/title closure have no reviewed endpoint. | Decide approved prose/price/powers and endpoints with builder; implement durable confirmed effect or rescue/aid/world outcome before credit. Preserve exact current delivery rewards; do not silently add gloves, title flags or personal kills. |
| ZSQ-LONG-CURRENCY / TEACHING | Witch's 25,000-copper plus vial reward already uses native committed item/wallet recovery; four paid clothing offerings and computed summon-familiar teaching are separately unavailable under active accounting. Three qc_action lessons are timed ambient speech. | Preserve and qualify cash reward replay with the vial. Add coordinated paid item/coin settlement and epic/copper/skill publication; record committed teaching or accepted topics only after actual outcomes, never from ambient speech. |
| ZSQ-CLAW-TARGET / HOT-SWITCH | Burn wrapper applies damage before checking the named object; mage give branch parses an object without validating the recipient. Native dispatcher visits nearby procedures before normal command execution. | Resolve actor/object/recipient and command before effects or consumption. Add target/alias/wrong-command, other nearby objects/recipients, fatal/protected damage and repeated-use fixtures while preserving remote switch and difficulty. |
| ZSQ-CLAW-TRANSFORM / DEATH-DISPOSITION | Mage extracts the key before output confirmation and removes itself on read failure. Guardian directly creates a nested death container/money without committed custody or decay; generic shatter text does not create rainbow proof. | Commit exact input/output and recipient episode as one recoverable transform; publish prose/removal after success. Define committed nested death placement/currency/decay and fallback/recovery with missing/wrong prototypes/room and publication failure; preserve supplied final proofs and avoid false personal-kill credit. |
| ZSQ-CLAW-SERVICES / ACCESS | Five shaping/moss routes and fixed 2,000-copper disappearing sage are support; thirteen returns are feedback. Entry compares levels 21–50 and trust on NPC; vault fragility prose does not set break chance. | Qualify active stock/recipient episodes and paid wallet/clue settlement. Preserve deliberate level/door policy, correct actor/prose decisions with builder and confirmed travel. Optional paid clue/vault/producer route must not gate the king's exact-shard receipt. |
| ZSQ-NW-REWARD | All five rune exchanges have identical offerings. Native prepending plus first-complete matching select the ruby eyepatch, without a reward-choice step. | Keep one recovery family and all historical bindings. Builder chooses fixed reward/retirement or explicit selection; freeze chosen binding/output and attempt before consuming exact owned runes, with retry/recovery and revision fixtures. |
| ZSQ-NW-MAZE / TOPOLOGY | All 36 raw maze rooms lead internally; exterior entries have no inbound reverse. Boundary search examines the actor rather than each candidate; reset passes virtual identity as real index and assumes reciprocal exits. | Freeze intended 6×6/wrap/exterior topology with builder. Add validated old/new reciprocal transitions, alternate real-index and missing-edge fixtures, failed/repeated moves and confirmed player-passage events; preserve probability/difficulty. Source gaps do not prove a played crash. |
| ZSQ-NW-ACCESS / ORPHANS | Wand inscription says wave but trigger is use; bridge resets open. Agatha debt/horseshoe, farm/logging/escort promises have no local outcomes; actual horseshoes have Wicks-owned foreign consumer. | Decide barrier versus reset and align command prose without imposing new cost. Qualify actual source/access/return episodes; author absent endpoints or keep as lore. Preserve owned foreign receipts and separate optional exploration from Malchor's finale. |
| ZSQ-BS-SUPPLY / MISSING-MAKERS | Five advanced makers have no ordinary active placement; scalp and ten foreign component prototypes lack confirmed active producers. | Builder chooses placement/source restoration or retirement; qualify actual NPC/reset generations, cap/difficulty and recovered/computed stock. Do not infer absence solely from names or silently borrow Winterhaven ingredients. |
| ZSQ-BS-IDENTITY / CONDITIONAL-RECIPES | Four same-named quarters and original/returned heads differ; fifteen artifact bindings, two cosmic dust, matching scrolls and Bloodstone's Storm/elixir recipes expose exact and competing prerequisites. | Preserve independent giver/binding ownership; add conditional producer/source alternatives, object allocation and committed source-versus-gift/transform lineage. Producer history does not replace spent stock or require a personal route. |
| ZSQ-BS-UNFINISHED / SERVICES | Pellops and several room promises have no implemented finale. Nine-earring mixed payment and stat purchases are unavailable under active accounting; scroll recite extracts before publishing output. | Decide endpoints/gameplay with builder; qualify coordinated fee/effect/receipt and recoverable random transformation. Repair verified prose and ambient targets separately without inventing tracking completions. |
| ZSQ-KEEPS-RECIPIENT / EPIC-RESET | Hindis's head reward removes the figurine recipient; CrowFoot's message removes the egg recipient; Brothedin leaves despite continuing-supply wording. Mode-zero zone reset depends on epic/reset admission rather than ordinary timer scheduling. | Qualify both orders, current NPC generation, normal/force/epic/recovered reset episodes and retry. Builder decides retained removal versus follow-up/replacement; do not promise daily stock or change reset mode from a difficulty-column guess. |
| ZSQ-KEEPS-IDENTITY / PHYSICAL-RECEIPT | Four life shards share a name but require distinct VNUMs. Marny's second exchange needs the physical receipt in addition to ore. Void crystal has an independent chance-loaded elemental source; final staff needs four crystals plus a shadow staff. | Keep exact live checks and optional source receipts. Extend conditional recipe/source edges and shared ingredient allocation with source/actor/UID lineage; qualify spent receipts, supplied shards/components and independent finales. |
| ZSQ-KEEPS-ACCESS / OWNERSHIP | Crate auto-binds a pull switch; keys break on use; pirate crevice is loaded by Tharnadia Rifts; Blackbeard's chest/shard and ether crystal occupy different containers. Foreign city givers supply an outer gate key. | Add confirmed switch/unlock/travel/retrieval with exit/reset episode and original object identity. Qualify exact nested recovery, partly-open routes and key replacement. Preserve owned foreign receipts and normal versus administrative sources. |
| ZSQ-KEEPS-CONTENT / CUSTOM-COMBAT | Heads/hearts and names differ; Woten/Weskel award one crystal despite plural prose; CrowFoot's egg response is unfinished and the final staff has empty success text. Zorana calls existing helpers without a story event. | Builder chooses bounded prose/count repairs; publish success after committed outputs. Qualify confirmed helper arrival and approved encounter/closure semantics before counting war, cure, dragon defeat or advertised item powers. |
| ZSQ-RESET-AUTHORITY | Confirmed source blocker for fresh active-accounting item reset generation, including scenery and nested supplies. Existing recovered items may still exist; this is not proof every deployed area is empty. | Qualify committed reset generation before the active-world pilots. Stable reset occurrence/command keys, exact custody, parent dependencies, retry/replay, NPC equipment, non-takeable portals/signs, and cross-area reset ownership are required. |
| ZSQ-TUTORIAL-GRANT | Confirmed ordering weakness: the tag is removed before sword creation/publication is confirmed. | Implement committed script grants with recoverable tag outcome; avoid success prose before confirmed grant. Keep failure distinct from repeated completed lesson. |
| ZSQ-TUTORIAL-TRAVEL | Confirmed failure-path weakness: refresh/success text precede destination validation; room index zero is excluded. | Validate destination and mutation admission first, then publish confirmed arrival. Test valid index zero and missing room, alongside normal Ailvio travel. |
| ZSQ-ANIMAL-LIFECYCLE | Direct custom death creation and decay replacement lack committed source/transform evidence. | Add actor-aware birth, freshness/deadline, placement-aware transformation, and exact input/output retirement. Reject personal credit for unknown origins; preserve gifts for delivery. |
| ZSQ-OPTIONAL-PREPARATION | Implemented first portion: schema 3 optional steps cannot take `Next:` priority. Native goals and receipt IDs are preserved. | Qualify conditional subrecipes, shared reveal rules, and all-stage campaigns separately. An optional check is not a new historical event or branch model. |
| ZSQ-TWIN-TERMS | Confirmed prose/contract differences; some giver price differences are legitimate alternatives. | Review the complete term table before deciding whether to fix prose or execution; do not normalize prices without a world-content decision. |
| ZSQ-TUTORIAL-NOTE | Isolated Grandma invitation prototype; no literal active source route found. | Confirm retirement or restore a complete supported route. Keep it out of player promises meanwhile. |
| ZSQ-SCRIPT-OBJECTIVES / ZSQ-LEARNED-LORE | Current completion steps require Q receipts; printed commands or cleared tags are not durable story evidence. | Define committed accepted-topic, scripted grant, and successful travel events with explicit terminal/one-time policies. |
| ZSQ-AILVIO-SCRIPT-GRANTS / MAP-CONTEXT | Map grant and hidden-ingredient replacement use direct publication; map handler trusts raw command/context without separately verifying Burbul. | Commit exact item grant/transfer plus replacement generation; define source-presence, alias and retry policy. Publish prose/objectives after successful outcome. |
| ZSQ-AILVIO-BANDAGE | Reward tests a scheduled bandage attempt, not successful revival or an exact victim generation. Existing durable bandage consumption is already present. | Add successful-healing/aid adapter tied to the exact actor/victim episode, recover one reward and closure, and test aborted/concurrent/failed aid. |
| ZSQ-AILVIO-FORAGE / FISHING | Forage is explicitly unavailable under active accounting; fishing already uses creation authority but prints success and grants experience before item outcome. | Port forage generation/source evidence. Qualify post-grant fishing effects and distinct catch provenance without replacing working creation authority. Pet buy/rent refusals are another explicit unsupported service, not a teacher-story gate. |
| ZSQ-AILVIO-EYES / LESSON-ALIASES | Distinct eyes 29241 have no confirmed ordinary route; six Red lesson keyword lists are repeated-letter placeholders. | World builder reviews restored source versus corrected requirement, and readable lesson aliases. Journal preserves exact requirements, advertises only valid Red topic, and explains the missing source. |
| ZSQ-AILVIO-ROLE-DRIFT | Redeemed-theurgist descriptions conflict with Taiz's retained murder/undead lessons and bone offering. | Builder aligns intended role, narrative and terms while preserving historical receipts; no purification campaign is inferred from prose alone. |
| ZSQ-AILVIO-DEPARTURE | Home/birthplace mutation precedes teleport confirmation. | Qualify resolved destination and confirmed travel with recoverable home outcome, including race/class/default selection. |
| ZSQ-AILVIO-NARRATIVE / BRADD-NARRATIVE | Medicine note, voice restoration, captive release, rat purge and later payment contain prose beyond verified native effects. Some may intentionally be narrated closure. | Separate accepted delivery, narrated closure, actual NPC state, encounter generation and personal action. Make content decisions before adding effects, prerequisites or rewards. |
| ZSQ-BRADD-PREPARATION | Fixed supplied-collar guidance and displayed intermediate rescue without another achievement. | Qualify optional and ordinary routes separately; retain exact receipts and require Lord's terminal delivery for final story. Same-name area/procedure ownership and maul spell ownership remain explicit review boundaries. |
| ZSQ-BREALE-RIDDLE / SPELLS | Passage drawings explain the witches' ingredient vocabulary; it is a deliberate riddle. Separately, promised learned fire spells do not match potion/amulet rewards. | Preserve the puzzle; author staged clue/solution reveals and accepted examination. Builders review spell reward versus prose intent separately, preserving receipts and balance until that decision. |
| ZSQ-BREALE-ESCORT | Wrist/bracelet narrative suggests guidance, but no assigned bracelet/escort gate exists; Hagatha really does hold the trapdoor key. | Journal explains actual key/access. Builder chooses narrative clarification or committed escort/arrival. Do not invent bracelet immunity or report the key as missing. |
| ZSQ-ACCESS-STATE / MAGIC-DOORS | Shared mechanics make property data executable: Homestead key `-2` plus last exit keyword unlocks the locked outer gate through speech. The hidden beach has the same negative key but is reset unlocked. | Add reviewed exit/key/password/property leads and live blocked reasons. Separate current access from accepted unlock and confirmed arrival history; open routes and supplied keys work without local preparation receipts. |
| ZSQ-ELVISH-KEY-IDENTITY / POTION-LINEAGE | Two identically aliased spider keys have different targets; preparation statues are consumable potion types. | Fixed journal distinctions and optional preparation. Future projection links exact target/item identity and committed recipe UIDs, handles consumed/gifted ingredients, and does not infer personal recovery or terminal release from preparation alone. |
| ZSQ-ELVISH-NARRATIVE / SHARED-TRAVEL | Leaf/petals and serpent/dragon differ in prose; final elf change is narrated closure. Deity statues invoke generic item teleports, including a cross-area destination. | Builder reviews narrative/effects deliberately. Qualify accepted examination/topic and confirmed shared travel; echoed MA/QA text is not recipient objective evidence. Spellcase magical opening remains unintegrated flavor pending builder intent. |
| ZSQ-REUSED-REWARD-LINEAGE | Krimeneha's seven rescues consume a fragment and generate another with the same VNUM. Bastine consumes original hide 41411 and supplies same-named hide 41304 with a different property. | Freeze exact input/output UIDs, replacement semantics, NPC episode and actor/recipient in committed evidence. Same-name/type return does not prove immutable custody, and a fragment-only rescue remains a real accomplishment. |
| ZSQ-ROAMING-SOURCE / COMPETING-CONSUMERS | Eckraldu can leave the mansion's load room for three other areas; Quietus also accepts his staff. This is a supported source lead, not proof of a missing staff. | Keep prototype owner, reset generation, current location and encounter separate; qualify movement and competing exact-staff consumption. Confirm intended roaming route before changing placement. |
| ZSQ-KRIMENEHA-ALL-STAGES / BASTINE-RANK | Staff preparation and household rescues are distinct; servants' blessing has no extra encoded reward. Bastine's twelve promotions do not enforce earlier ranks. | Add explicitly authored all-stage achievements and accepted membership/rank rules only after content decisions. Retain gifted terminal inputs and independent receipts. |
| ZSQ-CROSS-ZONE-OWNERSHIP | Victor's Bastine trust exchange starts Highway's ordinary four-lock route and Morlanthra's own final Q. The tree is not a bespoke four-item admission check. | Add owned story links, shared clue/access adapters, encounter/stage visibility and terminal references without annexing or double-counting native receipts. Source range guesses must not override prototype-file ownership. |
| ZSQ-BASTINE-TERMS / PRINCE | Staff/wand, helm/ankh and patch/face wording differ; bedroom secret/reset state differs. Prince head is reset inventory; grief, disease and banishment wording is not a Q state mutation. | Journal now uses actual items/keys and optional routes. Builder reviews intended narrative, concealment and later dragon challenge; qualify shared combat/gear separately before adding kill, cure or banishment objectives. |
| ZSQ-RESET-SUPPLY-CAP | Pine Hollow's coat requires two huge skins, but the only active producer has an ordinary live item cap of one. Forced repopulation or existing stock can differ. | Reproduce fresh-world accumulation after reset admission; review raising the cap or adding a source. Exact count and stock capacity need separate static checks; preserve the two-skin Q pending a balance decision. |
| ZSQ-BRANCH-AVAILABILITY / HOLDING-ROOM | Pine's dragon contracts are independent but remove their giver's remaining materials; three roaming proof carriers can enter a one-way holding room. | Qualify both completion orders and live source episodes. Author branch/attempt policy explicitly; reproduce the holding path and review barrier, return or placement repair without guessing a missing trophy. |
| ZSQ-DATA-HAZARDS / FOREIGN-EYE | Pine's mine movement trap prevents that move; opening the trapped desk differs from retrieving the eye for Skelenak. The duergar liberation and fountain-death narrative has no local completion. | Add confirmed access/movement/open/retrieval with hazard outcome, source identity and owned foreign terminal; builders decide liberation/investigation intent before durable objectives are added. |
| ZSQ-CONTAINER-OFFERING | Quietus credential input captures/destructs the full badge tree and creates an empty replacement; filled badges can lose the captain's key or other contents. | Journal warns to empty badges. Reproduce exact child UID outcomes, then author original-tree return, safe contents transfer or pre-submission refusal; preserve receipt policy and prevent duplication. |
| ZSQ-RARE-SOURCE-AVAILABILITY | Quietus captain/Aresliean share a chance-load room with four trap exits and one home exit; bat has its own rare-load route. Prose explicitly describes intentional chance. | Qualify load/wander/trap/cap/reset episodes and live availability. Builder reviews shared pirate placement or lifecycle repair without blanket-removing intended rarity; unused negative-exit room is not a foreign source. |
| ZSQ-QUIETUS-CONTAINER-STATE | Two treasure chests have locked/pickproof bits but no closed bit, unlike the captain's closed chest. | Use executable live state in access hints; reproduce initial get versus close/reopen and review flags/prose deliberately. A named key is not automatically required for already-open treasure. |
| ZSQ-WORLD-QUEST-SERVICE | Existing fee callback and committed item rewards are present, but generic wallet_spend is refused before active fee admission. Downstream failed/stale refunds use active-refused ADD_MONEY; map/abandon context lacks original quest generation. | Port identified fee admission together with compensation from the original debit and exact attempt/target. Test failed creation, replaced task, retained payments across activation, refunds/restart and zero-fee policy; preserve separate namespace and existing reward generation. |
| ZSQ-SHIP-SERVICE-SETTLEMENT | Crew/service handlers ignore active-refused SUB_MONEY or ADD_MONEY. Reachable hull callback waits for epic commit, but coin outcome is ignored; sale assets can clear without credit. | Extend existing epic hull adapter and port services to coordinated coins/epics/ship revision/publication/save with compensation. Qualify rejection/replay/changed owner/ship/timers/market before service objectives. Preserve inn save ordering and deliberate shop/whole-ship-sale refusals. |
| ZSQ-TORG-INVASION | Confirmed source weakness: Timoro's death announces invasion before validating leader/movement, with an unchecked room lookup; follower movement is conditional. Dranar/master 29024 begin in the incoming-only staging room. | Guard destination and exact NPC generations; confirm leader/partial/full-force arrival and actor/episode/replay policy. Do not infer liberation or personal kill from a delivery receipt. |
| ZSQ-TORG-HOLDING | All eight legends and Lanella can wander into exitless 29063 while live cap one prevents ordinary replacement. Legend resets are 100%; Tranug's three relics have separate 50% rolls. | Reproduce live source movement/availability; builder reviews return/barrier/placement without blanket-removing intentional variation. Keep source ownership, random stock and current reachability distinct. |
| ZSQ-TORG-SCROLL-TRANSFORM | Foreign-prototype legend reward is consumed before unconfirmed direct publication of one random Uz half. | Commit exact selected scroll UID and generation, freeze the one-of-two result, and recover input/output together. Test invalid target/output, rejection, retry/crash and foreign terminal ownership. |
| ZSQ-TORG-LANELLA-BIRTH / TIMER | Custom death directly publishes one random intact/broken heart without actor/source evidence. Duration is assigned to value[0], but no reviewed timer consumer or decay affect implements it. | Commit one-of-two actor-aware birth and placement; builder confirms intended expiry before implementing exact UID/deadline/recovery. Gifts permit foreign delivery without invented personal kill credit. |
| ZSQ-TORG-CONTENT / FORGE | Normal shop boot leaves Kordor's smith table unattached; old master/placeholder are unplaced and altar procedure is commented out. Shared smith menu/material paths need review before enabling. | Builder chooses intended forge/altar/retired content; qualify compatible shop-secondary dispatch, exact choices/materials and service settlement. Preserve deliberate active refusals, valid alternate receipts and permanent-character safeguards. |
| ZSQ-GROVE-AVAILABILITY | Valin and the pickaxe miner load in incoming-inaccessible 30607, can wander north into the glades or south into exitless 30612, and retain live cap one. | Reproduce fresh/recovered episodes and current location; builder reviews placement, wander barrier or return. Supplied ingredients and episode-specific D closure remain valid. |
| ZSQ-GROVE-RECOVERY | Dead mouse 30668 is ITEM_TRASH with no TAKE bit; ordinary ground/external-corpse recovery refuses it, unlike the dead worm. | Reproduce normal-player refusal and supplied/staff distinctions; review TAKE or an explicit committed recovery action with source/actor/UID evidence. |
| ZSQ-GROVE-ACCESS | Striker's downward world state loses upper bits and has no D reset, so the mimic route is open. Centaur passage is hidden/closed but unlocked; negative key alone does not require speech. | Derive initial/live exit state from executable loading/reset rules. Builder decides narrative clarification versus intended gates before changing behavior; qualify alternate/supplied routes and falling outcomes. |
| ZSQ-SPECIAL-TARGET | Literal inn assignment 30511 is inside Solonar's derived span but absent from active rooms; real_room0 falls back to world index zero. | Resolve against actual active entities, skip/log missing assignments and review intended location/owner. Preserve legitimate index-zero targets; range membership is only a lead. |
| ZSQ-GROVE-RECIPES | Two finales consume the same raw thread/filament; lich and Striker need separate orbs. Lich supplies two scrolls, and Valin requires a small pickaxe beyond the two items emphasized in dialogue. | Current journal uses exact inputs, separate consumer guidance and optional receipts. Extend explicit recipe edges/counts, distinct UID output recovery, current stock and accepted all-stage policies; keep three fee refusals separate from seven coin rewards. |
| ZSQ-WINTER-ACCESS / CONSUMED-KEYS | Painting uses `stare`; the chief key breaks after each unlock, and his four-locket finale removes the recipient. Dragon hearts unlock three different seals and retain individual expiry. | Model confirmed verb/scenery travel, live door/attempt state, key consumption and replacement availability. Qualify four locked doors, partly open routes, supplied lockets, early finale and reconnect without inventing prerequisite receipts. |
| ZSQ-WINTER-RANDOM / GIFT | Attribute/legend scrolls consume before unchecked random publication. Lancer's gem helper returns zero after creation, and the gift is retired before two failed publications. | Verify exact selected UID/context, return/check the created gem, freeze both outputs/prices, commit input/output lineage and print success only after outcome. Test wrong target, failure, replay/crash, foreign and supplied input. |
| ZSQ-WINTER-TIMERS | Same-VNUM animal birth, carried/ground/container decay and individually timed dragon hearts lack committed source/transform events. Animal worn handling is absent; heart missing-output failure leaves the counter below zero. | Commit actor/NPC episode, exact UID/deadline and placement-aware retirement/replacement. Review intended timers before generalizing to puffadder/Incarnate/plant; qualify retry/reconnect and personal versus supplied proof. |
| ZSQ-WINTER-SOURCES | Torg memory exists without a found ambassador reset; the city-roof/key route and several artifact inputs have no confirmed ordinary static producer. Volo and Adryv are placed abroad; cap-one Chibbleniffle has two competing placements. | Review computed/foreign/recovered sources and live location/custody. Builder chooses restore/retire/clarify without guessed exits, key rewards or personal-credit assumptions. |
| ZSQ-WINTER-TERMS / POWERS | Prose differs from actual cloak count, egg/blueprint reward, helmet metal and Incarnate output count. Several advertised powers are commented/unassigned; a Living Legend combat-periodic branch follows an unconditional return. | Propose targeted content corrections/restoration and separately qualify intended effects. Keep native receipts valid; no inactive power, discussion or lore-only king/vault becomes a completed objective. |
| ZSQ-WINTER-CAPACITY / FAMILIES | 219 detailed rows exceed the old 256 KiB authoring limit. 49 memory requests, competing supplies and 49 mixed payments need distinct family/service treatment. | Raise the guarded source bound to 512 KiB with boundary regression; add district/family projection, conditional recipe/ingredient allocation and committed multi-output settlement before collapsing campaigns. |
| ZSQ-SMOKEVEIL-EPISODES / TERMS | Ivar consumes both hearts together despite sequential prose; Ivar and Dorno disappear after either of two independent rewards. Talon/heart sources have cap one and multiple consumers. | Keep exact delivery receipts; clarify journal now. Builder decides retained disappearance, persistent follow-up or explicit replacement episodes before prose/behavior changes. Qualify current stock, source generation and actual recipient presence. |
| ZSQ-SMOKEVEIL-SOURCE / OWNERSHIP | Grishnak's head is F-carried by Diabolus in Tezcat; Azcatlipoca's animal is O stock at 20216, distinct from scenery. Ravi in Alatorin also consumes the Tentabeast heart with Raxthan's Bloodbeast heart. | Bind actual carrier/ground/source kind and exact item UID, distinguish gifts, starter grants and personal recovery, and preserve foreign receipt ownership. Confirm carrier intent before moving the trophy; no generic animal kill or reunion is inferred. |
| ZSQ-SMOKEVEIL-SERVICES / LEGACY | Normal boot attaches Dorno's smith table; Keebo is an epic teacher. Their active purchases refuse. Shared smith choice uses table index and searches NPC ore; unassigned coral-golem code references stale local identities. | Qualify complete menu/player-owned ore/payment/result recovery and successful teaching. Builder resolves the dormant golem's owner/item/slot/destination before assignment. Preserve explicit economic refusals; findings are source-level, not observed player-loss incidents. |
| ZSQ-SMOKEVEIL-CLOSURE | Tarlator's narrated cure/message/reunion is two native deliveries; Raltron accepts a supplied helm. No typed letter, transformation or reunion event is implemented. | Optional producer history is shipped. Add explicitly owned all-stage/closure events and shared projection only after builder intent and committed episode evidence; neither receipt alone represents the entire cure. |
| Evidence extractor gaps | Fixed chained/multiline literal assignment detection and ignored commented assignments. Object/mobile membership follows source prototypes rather than room bounds. | Continue review for aliases, computed VNUMs, preprocessor branches, table-driven teacher/smith assignments, F-selected parents, and foreign reset sources; extraction remains a lead list, not execution proof. |
| ZSQ-NEWHAVEN-CONTENT | Dibbly requests a reel but accepts a snorkel pipe; armorer tail success says collar and hammer-badge ground text names the other badge. Existing exact deliveries are preserved. | Builder chooses pipe prose/retained service or a deliberate revised reel contract with receipt compatibility. Correct tail/badge/cloak prose without changing equipment balance; qualify both badge allocations and competing reel consumers. |
| ZSQ-NEWHAVEN-ACCESS / PROPERTIES | Collar-source table resets inside its own closed/blocked alcove and uses pull on the inside return. Displacer starts in a mainland dispersal room. ROOM_INN bootstrap binds both inns; literal pool targets are missing. Tail has an actual get/put trap. | Review table placement/target/opening and supplied collars; qualify forest wandering, accepted travel/return, both inns, stale-target validation and trap charge/damage/custody. Property-driven source review supplements literal assignment candidates. |
| ZSQ-NEWHAVEN-LORE / ENDPOINT | Rift inquiry/confession, erinyes boasting and chained Dibbly's plea have no accepted closure, rescue or mercy-kill endpoint. Living and corrupted appearances are independent. | Retain lore or author deliberate actor/party/episode/branch endpoints before adding semantic events. Separate door unlock, current supplies, source recovery, actual kills and full-stage credit. Five paid recipes stay guarded until atomic material/fee/output settlement is qualified. |
| ZSQ-REALM-CONTENT / LORE | Finn accepts armor where he asks for a scroll; shop wall advertises teleport but actual stock identifies. The golden-gate riddle is undetermined; lost Song and Oberon revival have no accepted terminal. | Builders choose corrected prose or deliberate compatible content. Retain real independent retirement receipts; author episode/branch endpoints before escort, revival or full-stage credit. |
| ZSQ-REALM-COMBAT | The assigned tree spirit declines periodic registration, handles helpers only on old command 0 and rejects modern combat -102. Its helper state is shared static data. | Choose supported combat/tick dispatch, per-appearance counters, cadence and cap with builder difficulty review before restoration. Qualify concurrent/replaced spirits, death reset, draws and accepted creature publication; no invented helper requirement. |
| ZSQ-REALM-SOURCE / ACCESS | Spirit and makers start in load/dispersal networks; orb targets the realm approach. Speech unlock leaves closed state; tomb policies differ. Golden-gate destination, two exits and reset object 141120 are missing. | Qualify actual parent/slot/generation, wandering, encounters, held/carried keys, opening/travel/return and retiring recipients. Select restoration or retirement without guessed IDs; distinguish valid optional access from personal source proof. |
| ZSQ-REALM-FORGE / REPAIR | Alternative makers require the same five distinct parts plus 5,000,000 copper. Fix spell 595 restores one chosen item's condition; no quest repair receipt exists. | Keep paid refusal until atomic material/fee/output settlement. Extend exact device target/custody work for accepted scroll retirement and durable target mutation/recovery. Test missing/substituted parts, both recipients, abort/replay/restart and alternate Fix sources; preserve repair strength. |
| ZSQ-VERSPIN-CONTENT / SOURCE | Unplaced ten-offer sign has only five recipes; four-red shield exceeds reviewed ordinary cap three; Vulm's crabmen clue differs from actual jewel thief. Persuasion/reunion/contest/purge have no accepted terminal. | Builder selects sign placement/offer correction, bounded supply/recipe balance and clue/description repairs. Qualify actual sources, exact quantities/colors, rare gem, one foreign sigil parent, retirement and supplied terminals; add explicit endpoints before campaign credit. |
| ZSQ-VERSPIN-STAT / CREW | Nine guarded virtual stat purchases apply +1 rather than creating potions. Crew hiring ignores denied SUB_MONEY before changing crew/chief and saving. | Keep stat refusal until wallet/expected-stat/effect/save settlement is qualified. Reject denied crew debit before mutation, then coordinate wallet/ship revision/save and replay. Preserve native prices/stat caps/hire eligibility; confirm room placement separately. |
| ZSQ-VERSPIN-XP / ACCESS | Frozen XP preserves actor tenth-level and companion full-level caps. Bartender spectator access does not use secured-pit keys; deep-water/secret/falling paths have ordinary alternatives. | Confirm XP intent before balance changes and preserve admitted recipients/amounts on recovery. Qualify actual reciprocal key/open/travel/water routes and current source evidence; no personal kill or mandatory key/boat history from delivery alone. |
| ZSQ-SHIPY-CONTENT / ACCESS | Pol's note is not final input and Fishfetcher is not an output; four/six shiv text, absent fillet and transformation/research promises exceed native state. Four unloaded exits are removed at bootstrap; one viper dispersal destination is ocean. | Builder chooses accurate clues/products and bounded topology/ecology/lock corrections. Preserve valid supplied proofs, exact quantities/prices and existing terminal receipts; author real endpoints before deeper credit. |
| ZSQ-SHIPY-FISH / PAID | Catch/effects/XP precede ownership grant; six paid native services are guarded. Shared ship/crew paths ignore denied cash, with mixed epic/coin hull settlement. | Extend existing accepted fishing and paid-service continuations. Freeze selected draw/material/output/price and stable ship revision; settle/save/refund before success. Qualify denial, interruption, concurrency, replay/restart and legitimate supplied proofs. |
| ZSQ-COSMIC-SOUL / IDENTITY | Director's three branches consume distinct souls without declared rewards; box needs all four together. Study/wind proof and output share aliases; Xavier owns the foreign delivery. | Preserve native allocation/receipts and exact kinds. Confirm donation/clue/name/campaign intent; add accepted source/gift lineage and attempt/world outcomes before deeper credit. |
| ZSQ-COSMIC-CUSTOM / PAID | Rare box/windkin, cap-one proof, directional keys/secret boulders, teleports and effect/teacher state need accepted evidence. Rename/pet/epic purchases are guarded; legacy pet claim has no restoration call. | Extend shared reset/appearance/access/arrival/effect/actor-state and wallet/identity/pet/lesson continuations. Deliberately restore or retire pet claims; qualify denial/refund, partial identity/save failure and replay/restart before removing guards. |
| ZSQ-SURFACE-ALLOCATION / SOURCE | Four different trophies buy a book; four wardens consume separate instances of one heart kind; mystic needs five current kinds. Claws/crystals/eggs and retiring hunter compete; bracer 729 has no ordinary loaded repeatable producer. | Preserve exact supplied routes and independent receipts; qualify recipient/source episodes and imported custody. Add allocation/first-source/gift evidence and builder-selected supply/branch policy before deeper campaign credit. |
| ZSQ-SURFACE-CUSTOM / REPAIR | Two Mril exits target unloaded 33807; wood has a death-created source without confirmed decay binding; guardian/rift need accepted effects/arrival. Devil invasion is commented out; descent is disabled; fishing text/XP precedes grant. | Targeted loaded-edge/proof/clue repair; accepted source/helper/race-context arrival and post-grant publication. Deliberately restore or retire dormant endpoints with coordinated custody/identity/actor recovery; keep fee/epic guards and qualify shared ship settlement. |
| ZSQ-THARNADIA-AUDIT / CONTENT | Administrative paper 5 was omitted only from the audit lookup; missing mobile 132677 remains real. Two toy descriptions and the pup/desk clues disagree with actual sources. | Include all active prototype sources without a limbo discovery owner; restore the paper-map service. Builder selects mobile/clue/placement/lock repairs, retaining exact native identities and balance. |
| ZSQ-THARNADIA-CONTAINER / SOURCE | P resets choose a global matching live container rather than the immediately preceding local O instance. Search reveals hidden contents without carried custody; loans differ from toy proofs. | Freeze actual parent UID/generation/location and qualified policy in reset admission. Test moved/carried/capped containers, denied grants, open/search/GET and personal versus supplied lineage before source milestones. |
| ZSQ-THARNADIA-PREPARATION / SEMANTICS | Herb-to-vial service supports the retiring vial-plus-pendant finale, while supplied vials skip producer history. Lesson/healing/escape/pet prose does not create accepted actor/NPC outcomes. | Preserve eight independent deliveries and optional preparation. Extend owned semantic/attempt/actor-state adapters only after explicit builder endpoints; qualify consumed ingredients, exact collections and supplied finales without fabricated history. |
| ZSQ-THARNADIA-PAID / CUSTODY | Wand is coin-only; child accepts any food type. Active refusals, direct dice drop, legacy pet claim and shared crew payment need separate support. | Keep guards until exact wallet/typed-item/output/recipient and draw/custody/effect/save settlement is qualified. Restore or retire pet claims deliberately; reject denied ship debit before mutation. |

## Verification record

Record exact test/build results for each implementation batch here. Source audit
exports are read-only and can run without accounting or database activation:

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence twin_towers_forest
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence newbie2
```

The inventory regression checks real chained mob/object/room assignments,
commented/cleared functions, exact Q/M counts, reset counts, and prototype
membership beyond room bounds. The story regression checks native parsing of
all maps, optional versus required `Next:` behavior with a supplied plant,
equipment slot accuracy, service exclusion from achievements/dailies, and old
schema compatibility. These checks do not activate accounting or prove the
unimplemented reset/grant/lineage paths.

### First comprehensive mapping batch — October 2, 2026

Passed both maintained C++20 builds:

```bash
make -C src -j6 CC=g++-12 BIN_ROOT=../bin
make -C src -j6 CC=g++-12 BIN_ROOT=../bin PERSISTENCE_BACKEND=flatfile \
  DMS_BINARY=../bin/server/dms_zone_story_builder_flatfile
```

The host's existing hiredis TLS library directory was supplied with
`LIBRARY_PATH`; no dependency was installed. Focused regressions passed:

- `python3 tests/async/test_zone_story_quest_story.py`: all 36 maps parsed in
  native C++; optional preparation and supplied-plant next action; exact waist
  slot; schemas 1/2 compatibility; invalid optional fields; distinct feathers;
  achievement/daily projections; receipt preservation and read-only/restart checks.
- `python3 tests/async/test_zone_story_quest_production_catalog.py`: full catalog
  snapshot, generated world inventory and both source indices, exact assignment
  chains, omitted comments/quoted examples/cleared functions, reset counts,
  and prototype membership checks.
- `python3 tests/async/test_zone_story_quest_feature.py`.
- `python3 tests/async/test_zone_story_quest_arrival.py`.
- `python3 tests/async/test_zone_story_quest_production.py`.
- `python3 scripts/zone_story_quest_home_coverage.py --check`: all 27 required areas.
- `./scripts/format.sh --check`: changed lines and complete touched files.

Document links resolve. All 2,668 native definitions, zone registry, source
fingerprint, content revision, and the other 34 maps are unchanged as parsed
objects. Twin Towers still contributes ten achievements and three daily groups;
the twelve newly displayed services increase total projected rows without
changing achievement/daily eligibility. No production operation or accounting
activation was performed. Fresh active-world gameplay remains open because the
reset, script grant, and transformation adapters above are not implemented.

### Second comprehensive mapping batch — October 2, 2026

Ailvio and Braddistock source dossiers and generated audit indices are complete.
Native C++20 warnings-as-errors schema/projection regressions passed for all 36
maps and schemas 1/2/3. Added checks prove every one of the 78 native fish pairs,
same-kind/mixed live counts, one feeding achievement, original alternate receipt
recovery, optional supplied-note/collar guidance, and rescue-only versus terminal
mansion completion. Service rows remain outside achievement counts.

The full production catalog, generated world inventory, all four source indices,
and 27 required home mappings passed focused regressions. SQL `make -C src`
completed using the existing hiredis library path; no production C/C++ source
changed in this batch. Changed-line and complete touched-file formatting passed.
All 594 checked document links resolved and `git diff --check` passed.

The global projection is now 2,359 achievements, 1,962 potential daily units,
and 2,509 rows including services/administrative content. All 2,668 native
definitions, zone registry, source fingerprint, content revision, and other
34 sidecars are unchanged as parsed objects. No database operation, migration,
accounting activation, or merge occurred. Active-world qualification remains
open for the exact reset, grant, forage, successful-aid and narrative dependencies
recorded above. Priorities 5–220 still require comprehensive source review.

### Third comprehensive mapping batch — October 2, 2026

Breale and Abandoned Elven Homestead source dossiers and reproducible source
indices are complete. Schema 3 revision 2 journals preserve six independent
Breale achievements, two Homestead achievements, both preparation services,
and all ten original native bindings. Optional access/earlier-exchange checks
cannot displace a supplied final offering's required turn-in action.

The native C++20 warnings-as-errors story regression passed for all 36 maps
and schemas 1/2/3. New journeys cover supplied Triad reagents with no earlier
history, exact duplicate mushroom counts, supplied Homestead remedy without
local preparation, service-only progress, independent terminal completion,
read-only projection and original receipt restart. Production catalog/snapshot,
generated inventory, all six source indices, and the 27 required home mappings
passed. The maintained SQL server `make -C src` completed; this batch changed
no production C/C++ source. Whole touched-file and changed-line formatting,
818 checked document links, and `git diff --check` passed. Existing corrupted
arrow/multiplication glyphs in the priority document were repaired.

All 2,668 raw native definitions, source fingerprint, content revision, zone
registry and other 34 sidecars remain unchanged as parsed objects. Global
projection stays at 2,359 achievements, 1,962 potential daily units and 2,509
rows including services/administrative content. No database operation,
migration, accounting activation or merge occurred. Source mapping now covers
priorities 1–6 of 220; the remaining 214 areas and active-world journeys remain
open. Continue with Krimeneha's Mansion and Bastine, retaining the original queue.

### Fourth comprehensive mapping batch — October 2, 2026

Krimeneha's Mansion and Bastine source dossiers and reproducible indices are
complete. Schema 3 revision 2 journals preserve all 23 original native bindings
and classifications. Krimeneha retains eight achievements plus its staff
preparation service; Bastine retains twelve independent commissions, prince
request and Victor trust exchange. Native admission and rewards are unchanged.

Native C++20 warnings-as-errors regressions passed for all 36 maps and schemas
1/2/3. New journeys cover supplied family keepsakes without staff/access/rescue
history, read-only projection, preparation outside achievement counts, a real
fragment-only rescue, supplied final wand without earlier promotions, early
commission versus final completion, Victor trust without invented Highway
rescue, and original independent receipt recovery after restart.

Production catalog/snapshot, generated inventory, all eight source indices
and all 27 required home maps passed. The maintained SQL server build completed
from its existing artifacts; no production C/C++ source changed in this batch.
Whole touched-file and changed-line formatting passed. Document checks resolved 294 local links,
and `git diff --check` passed. The priority document's remaining corrupted em
dash was repaired.

All 2,668 raw definitions, source fingerprint, content revision, zone registry,
native story bindings/classifications and other 34 maps remain unchanged as
parsed objects. Global projection stays at 2,359 achievements, 1,962 potential
daily units and 2,509 rows including services/administrative content. No
database operation, migration, accounting activation or merge occurred.

The source queue is now eight of 220 complete, with 212 areas pending. Continue
with Pine Hollow and Quietus. Live roaming/source generation, exact replacement
lineage, all-stage household/rank campaigns, accepted clues/access and owned
cross-zone rescue remain qualification work; this pass does not certify them.

### Fifth comprehensive mapping batch — October 2, 2026

Pine Hollow's complete source dossier and reproducible audit index are recorded.
Revision 2 improves sources, exact garments/materials, rewards, encounter guidance
and competing-route availability while preserving seven independent achievements.
The mine/duergar exploration, shared hazards and Skelenak eye continuation are
mapped without creating an invented local terminal. Source-cap and holding-room
concerns include precise reproduction and builder repair plans.

Native C++20 warnings-as-errors regressions passed for all 36 maps and schemas
1/2/3. New cases cover supplied dragon equipment/trophies without source, kill,
topic or earlier branch history; independent receipts; one huge skin versus
two exact huge skins; two versus three brown skins; ordinary/huge type separation;
read-only next actions; and reload without invented other clothing or foreign
achievements. These cases qualify projection/receipt behavior, not NPC wandering,
source generation, trap execution or a played foreign quest journey.

Production catalog/snapshot, generated world inventory and all nine reproducible
source indices passed, including Pine's exact reset/prototype counts and the
documented single huge-skin producer. All 27 required home maps passed. The
maintained SQL `make -C src` completed from its existing artifacts; no production
C/C++ source changed. Complete touched-file and changed-line formatting,
`git diff --check`, and 282 local document links passed.

All 2,668 raw definitions, source fingerprint, content revision, zone registry,
native story bindings/classifications and other 35 maps are unchanged as parsed
objects. Global projection remains 2,359 achievements, 1,962 potential daily
units and 2,509 rows including services/administrative content. No database
operation, migration, accounting activation or merge occurred.

The source queue is now nine of 220 complete, with 211 areas pending. Continue
with Quietus Quay, then Torg. Fresh active-world source capacity, roaming/holding
availability, branch attempts, exact replacement lineage, shared hazard/access
events and owned cross-zone journeys remain implementation/qualification work.

### Sixth comprehensive mapping batch — October 2, 2026

Quietus's complete source dossier and reproducible audit index cover all 16 Q
contracts, 14 native M families, 95 rooms, 54 mobiles, 55 objects, five shops,
216 resets and four special assignments. Shared execution review includes
reachable ship services/hull completion, world-quest admission/rewards/refunds,
ordinary shop refusals, inn terminal save, container offerings, locks/liquids,
reset parent selection and mobile availability. Bounded foreign sources establish
Rodev's sword, Ceothian credentials and the mansion's seal/staff; those reads
do not comprehensively qualify the other areas.

Revision 2 retains four mission achievements and seven supporting service rows.
Optional briefing receipts do not change native admission or require membership,
personal kills or all earlier missions. Exact note/head delivery, foreign proof
leads, named rewards, consumed seal versus returned dagger, all topic families,
empty-badge advice and source availability are represented in the journal.

Native C++20 warnings-as-errors regressions passed for all 36 maps and schemas
1/2/3. New cases prove that a supplied head alone is incomplete, note/head
together suffice, all four supplied mission sets reach their delivery without
optional history, services never add achievements, reads never write, and
reload retains four independent receipts without invented Sarmiz/mansion
accomplishments. The source regression checks every native M family has a valid
representative topic/contact alias, exact source counts/assignments, actual
captain/Aresliean placement and nested badge key data.

Production catalog/snapshot, generated world inventory and all ten reproducible
source indices passed. All 27 required home maps are present. The maintained
SQL `make -C src` completed from existing artifacts; no production C/C++ source
changed. Complete touched-file and changed-line formatting, whitespace checks
and 311 local document links passed.

All 2,668 raw definitions, source fingerprint, content revision, zone registry,
native bindings/classifications and other 35 maps remain unchanged as parsed
objects. Global projection remains 2,359 achievements, 1,962 daily candidates
and 2,509 rows including services/administrative content. No database operation,
migration, accounting activation or merge occurred.

New findings have explicit repair/qualification plans: container inputs that
retire children despite return prose; intentional rare-source/trap availability;
open-but-locked treasure flags; refused active world-quest fee admission and its
downstream refund/attempt gaps; and ship service coin denial despite subsequent
asset mutation. Existing committed item rewards, epic hull ordering, active
shop refusal and inn save handling remain acknowledged. Source findings do not
claim an observed production loss or a completed economic repair.

The source queue is now ten of 220 complete, with 210 areas pending. Continue
with Torg, then Vast Hidden Grove (Solonar). Full active-world source generation,
semantic events, service settlement and played/recovery journeys remain open.

### Seventh comprehensive mapping batch — October 2, 2026

Torg's complete source dossier and reproducible index cover all fifteen Q
contracts and fifteen native M families, 282 rooms, 126 mobiles, 100 objects,
two shops and 597 resets. Reviewed three literal special assignments, complete
Timoro/Lanella death handlers, table-driven teacher/forge reachability, shared
door/teleport/follower/holding behavior and bounded foreign legend/heart/head
continuations. These foreign reads do not qualify whole foreign areas.

Revision 2 retains twelve achievement rows and two preparation services, all
fifteen bindings and the two equivalent fine-chisel recipients. Optional
curing, favor/rose and locket history preserves supplied materials. Every native
topic family has a valid representative contact/keyword. The journal explains
the Colosseum password and medallion gate, two identically named ring kinds,
eight distinct legendary relics, three independent Tranug relics, different
chisel commissions and actual invasion-dependent recipient availability.

Native C++20 warnings-as-errors regressions passed for all 36 maps and schemas
1/2/3. New cases prove supplied terminal materials skip optional preparation,
two copies of one ring cannot replace the other kind, eight copies of one relic
cannot replace all eight kinds, services add no achievement, both fine-chisel
alternatives count once, reads never write, and reload retains twelve local
receipts without inventing Winterhaven/Juiblex/Scorch Valley completion. These
are isolated projection/receipt tests, not played source or invasion journeys.

Production catalog/snapshot, generated world inventory and all eleven source
indices passed. Source checks include the actual F-parent fine chisel, its 20%
roll, the unplaced alternate master, incoming master, eight 100% cap-one legend
resets and three separate 50% Tranug rolls. All 27 required home maps remain
present. The maintained SQL `make -C src` completed from existing artifacts;
no production C/C++ source changed. Complete touched-file/changed-line
formatting, whitespace checks and 314 local document links passed.

All 2,668 raw definitions, source fingerprint/content revision, registry, native
bindings/categories and other 35 maps remain unchanged as parsed objects.
Projection remains 2,359 achievements, 1,962 potential daily units and 2,509
rows including services/administrative content. No database operation, migration,
accounting activation or merge occurred.

New findings have explicit repair/qualification plans: validate and confirm
invasion arrival rather than announcing success first; review one-way holding
availability without guessing missing sources; commit one frozen random scroll
replacement or heart birth with exact lineage; decide intended heart expiry;
and review unattached forge/disabled altar content before enabling mechanics.
Existing native accounting, deliberate service refusals and shared inn ordering
are preserved. Source weaknesses are not claims of observed production loss.

The source queue is now eleven of 220 complete, with 209 areas pending. Continue
with Vast Hidden Grove (Solonar), then Winterhaven. Full active-world generation,
semantic events, service settlement and played/recovery journeys remain open.

### Eighth comprehensive mapping batch — October 2, 2026

Vast Hidden Grove's [source dossier](zone-stories/VAST_HIDDEN_GROVE.md) and
[reproducible index](../reference/zone-story-audits/solonar.md) cover all fifteen
Q contracts, twelve M families, 100 rooms, 41 mobiles, 72 objects, seven shops
and 152 resets. Reviewed every local room/prototype, thirteen shared scenery
teleports, loaded/reset access, elemental/plant/animal sources, generic wandering,
normal pickup/corpse recovery, falling and bounded foreign source/arrival leads.

Revision 2 retains three independent equipment stories, two bird requests and
ten services. Optional specialist receipts explain the arch-magi, piwafwi,
Mage Bane and lich routes without requiring personal preparation. Fourteen
contacts cover all native topic families. Current guidance distinguishes raw
golden from lavender thread, competing ingredient copies, the lich's two-scroll
reward, Valin's additionally required small pickaxe, three fees versus seven
coin rewards, actual scenery commands, hidden/unlocked versus speech-locked
passages and falling hazards.

Native C++20 warnings-as-errors schema/projection regressions passed for all 36
maps and schemas 1/2/3. New cases cover supplied recipes skipping optional history,
exact pickaxe/raw-thread requirements, all three accounting-service fee warnings,
spent ingredients remaining missing after preparation receipts, ten services
adding no achievements, read-only projection and five recovered local achievements
without fabricated Homestead/Alatorin completion. These fixtures project recovered
receipts; they do not execute the unavailable mixed fees or qualify live sources.

Production catalog/snapshot, generated world inventory and all twelve source
indices passed. Source checks include contact/topic aliases, exact Valin/miner
load episodes, physically absent inn room 30511 and Striker's missing D reset.
All 27 required home maps remain present. The maintained SQL build completed
from existing artifacts with `make -C src -j6 CC=g++-12 BIN_ROOT=../bin`; relative
artifact paths avoid this worktree's space-containing default target paths.
No production C/C++ source changed. Touched-file and changed-line formatting,
whitespace and 331 local document links passed.

All 2,668 raw definitions, source fingerprint/content revision, registry, native
bindings/categories and other 35 maps remain unchanged as parsed objects.
Projection remains 2,359 achievements, 1,962 potential daily units and 2,509
rows including services/administrative content. The previous Torg dossier's rose
and hearts now use their actual ITEM_TRASH type; the current durable offering
limit is fourteen. Those source corrections change no receipt or behavior.

Repair/qualification plans cover Valin/miner holding availability, ordinary
mouse recovery, missing assignment target resolution, narrative versus loaded
access, exact competing recipe consumers and two-output lineage/recovery.
Existing reset-generation, mixed-offering and confirmed travel/semantic plans
remain prerequisites for active-world qualification. No database operation,
migration, accounting activation or merge occurred.

The queue is now twelve of 220 source-comprehensive areas, with 208 pending.
Continue with Winterhaven, then Shairak and Smokeveil Forest. Full active-world
generation, semantic events, service settlement and played/recovery journeys
remain open.

## Winterhaven source pass and verification — October 2, 2026

The [Winterhaven dossier](zone-stories/WINTERHAVEN.md) and
[reproducible index](../reference/zone-story-audits/wh.md) cover all 571 native
blocks, 599 rooms, 314 mobiles, 483 objects, thirteen shops and 1,284 resets.
Bounded foreign reads resolve source ownership, actual heart births/keys,
scroll lineage, artifact inputs and related consumers. The 92 literal assignment
candidates include 21 animal decays recovered by multiline resolver/field-chain
extraction. Macro aliases remain manually resolved. All 48 reset-sourced memories
and the engineer-reward Ultarium memory were checked separately.

Revision 2 retains 219 row IDs and their bindings/categories, two exclusions,
135 achievements and 84 services. It adds 92 contacts covering all 152 nonempty
addressable topic families, 38 optional preparation checks and exact sources,
colors/marks, counts, keys, timers and competing consumers. Unsupported personal
or semantic history is described in the plan rather than falsely recorded.

Focused checks passed:

- `test_zone_story_quest_production_catalog.py`: catalog/snapshot and generated
  inventory agreement, thirteen reproducible indices, nonempty topic/alias
  coverage, multiline literal assignments and the authoring byte boundary.
- `test_zone_story_quest_story.py`: all 36 native maps and schemas 1/2/3;
  thirteen-root supplied Storm recipe, two cosmic-dust copies, distinct same-name
  marks, optional key preparation, 49 fee warnings, spent equipment/orbs staying
  missing despite service receipts, independent memories and 135 recovered
  achievements without invented foreign completion. Native apply and file-load
  accept exactly 512 KiB and reject one byte over without partial application.
- `test_zone_story_quest_production.py` and `test_zone_story_quest_catalog.py`:
  runtime production/bootstrap and catalog contracts.
- Maintained SQL C++20 build with `make -C src -j6 CC=g++-12 BIN_ROOT=../bin` and
  `EXTRA_LDFLAGS=-L/tmp/plan2-hiredis/home/wsl/.local/duris-build-deps/hiredis-1.4.1/lib`.
  The existing matched hiredis/TLS libraries were used; no dependency installed.
  Formatting and whitespace checks passed.

All 2,668 native definitions, source fingerprint/content revision, registry,
bindings/classifications/exclusions and other 35 maps remain unchanged as parsed
objects. Global projection remains 2,359 achievements, 1,962 potential daily
units and 2,509 rows including services/administrative content. Python and native
source-sidecar bounds now agree at 512 KiB; per-story/objective/text bounds remain.

The findings register adds consumed-key/access policy, timed birth/decay,
recoverable random gifts/scrolls, missing/foreign sources, selected prose/power
repairs and district/family authoring. These are explicit implementation and
qualification plans; mixed fees, fresh active sources and full played/recovery
journeys remain pending. No accounting activation, DB operation, migration or
merge occurred. The queue is thirteen of 220 source-comprehensive areas, with
207 pending; continue with Shairak and Smokeveil Forest, then the Twin Keeps.

### Shairak and Smokeveil Forest checkpoint — October 2, 2026

Completed the full source review of 43 native blocks, 100 rooms, 74 mobiles,
59 objects, one shop and 435 resets. All eleven Q/QA contracts now have authored
guidance, with ten achievement rows and one repeatable bottle service. Nine
contacts cover all 32 nonempty addressable topic families. No literal assignment
belongs to the local prototypes/span; Dorno's smith and Keebo's teacher tables
were checked separately. Bounded foreign review resolves Diabolus's F-carried
head in Tezcat and Ravi's Alatorin request with the Raxthan Bloodbeast source.

Revision 1 explains the simultaneous heart pair, independent disappearing-giver
rewards, three local talon consumers, two local and one foreign Tentabeast-heart
consumers, actual dead-animal ground stock, ordinary access and optional
Tarlator history for Raltron's supplied helm. The dossier records unsupported
cure/reunion semantics, source/recipient episodes, inactive forge choice/custody
defects, deliberate service refusals and stale unassigned golem ownership.
The builder guide now consistently describes shipped schema-3 optional history.

Focused checks passed:

- `test_zone_story_quest_production_catalog.py`: checked-in catalog and generated
  inventory, fourteen reproducible indices, complete contact/topic alias coverage,
  cap-one actual trophy parents and the foreign F-selected head source.
- `test_zone_story_quest_story.py`: all 37 maps and schemas 1/2/3, exact live
  materials, optional supplied helm route, simultaneous hearts, independent
  same-input recipients, spent helm/trophies remaining missing, service exclusion
  and ten recovered achievements without invented Ravi completion.
- `test_zone_story_quest_production.py`: production/bootstrap native coverage.
- Maintained SQL build completed with the existing matched hiredis/TLS path;
  no dependency installation was needed. Formatting and whitespace checks pass.

All 2,668 native definitions, fingerprint/content revision, zone registry and
the other 36 mappings remain unchanged. There are now 37 authored journals;
projection is 2,358 achievements, 1,961 potential daily units and 2,509 rows
including services/administrative content. The single denominator change is
Forvos's reviewed service classification. Local documentation links resolve.

Player journals and new discovery/encounter/daily eligibility still require
actual active accounting. This batch performed no accounting activation,
database operation, migration or merge. Fresh active sources and actual played
failure/reconnect/recovery journeys remain pending. The queue is fourteen of
220 source-comprehensive areas, with 206 pending; the Twin Keeps and Bloodstone
Keep are next.

### Twin Keeps checkpoint — October 2, 2026

Completed the full source review of 116 native blocks, 451 rooms, 123 mobiles,
121 objects and 1,614 resets, together with Zorana's assigned combat handler,
shared switch/teleport/key/container/movement execution and bounded foreign
source/consumer reads. The [dossier](zone-stories/TWIN_KEEPS_OF_DEVASTATED_THARNADIA.md)
and [reproducible index](../reference/zone-story-audits/caertannad.md) distinguish
actual prototype/reset/receipt ownership from derived spans and current location.

Revision 1 adds all 30 exchange rows: 29 achievements and Mungir's one potion
service. Twenty-four contacts cover all 37 addressable topic families. Fifteen
optional producer checks include the foreign outer-gate key, tower key,
Marny's physical receipt, Jenzuel figurine, Keshia key and the moonstone/crystal/
scouting-message routes. Four same-named shards remain four exact checks, and
the final staff requires four crystals plus the shadow staff. Source history
does not replace a consumed physical item or require a personally performed
producer route for supplied materials.

The plan now covers Hindis/CrowFoot removal order, Brothedin's repeat wording,
mode-zero epic-reset availability, broken/consumed keys, the auto-bound crate
switch, Tharnadia Rifts' crevice/silverleaf ownership, exact nested sources and
conditional void-crystal sourcing. Selected head/name/count discrepancies,
unfinished egg prose and the empty staff response have bounded builder repair
proposals. Zorana's existing-helper hunt does not record spawning or war closure.
Foreign scepter-collector token wording remains an Alatorin content decision.

Focused checks passed:

- `test_zone_story_quest_production_catalog.py`: snapshot/catalog/inventory
  agreement, all fifteen source indices, contacts/topic aliases, exact nested
  sources, mode-zero metadata and foreign crevice/silverleaf ownership.
- `test_zone_story_quest_story.py`: all 38 native mappings and schemas 1/2/3;
  four distinct same-named shards, supplied finale/figurine without producer
  history, spent physical receipt despite recorded preparation, visible mixed-fee
  refusal, independent head/figurine receipts, service exclusion and 29 recovered
  achievements without invented foreign key or collector credit. This tests
  projection and receipt recovery, not unavailable fee execution or played access.
- `test_zone_story_quest_production.py`: runtime catalog/bootstrap regression.
- Cached SQL C++20 `make -C src` with the existing matched hiredis/TLS library
  path; whole-file harness formatting check and whitespace check.
- All 2,668 native definitions, registry, fingerprint/revision and other 37
  mappings remain unchanged as parsed objects; all local document links resolve.

The catalog now has 38 authored journals, 2,357 achievement units, 1,961 potential
daily units and 2,509 total rows. Only the potion-service classification changes
the achievement denominator; native contract IDs and reward terms are preserved.
Accounting remains a player-surface prerequisite. No activation, DB operation,
migration, server operation or merge occurred. The queue is fifteen of 220
source-comprehensive areas, with 205 pending. Continue with Bloodstone Keep,
then Neverwinter Woods; full active-world qualification remains open.

### Bloodstone Keep checkpoint — October 2, 2026

Completed the full 158-block source review: 65 Q/QA exchanges, 74 addressed
dialogue families, 918 rooms, 255 mobiles, 300 objects, 34 shops and 1,624 resets.
All local special code, relevant shared execution and bounded foreign ingredient
producers/consumers were reviewed. The [dossier](zone-stories/BLOODSTONE_KEEP.md)
and [source index](../reference/zone-story-audits/bs.md) preserve exact terms,
actual stock/parent/location, door/key semantics and foreign receipt ownership.

Revision 1 classifies all exchanges into nine stories, 22 requests, 32 services
and two exclusions. Twenty contacts cover all 74 addressed families; 35 optional
producer checks explain connected preparation without requiring personal routes.
Four same-named quarters remain distinct, as do original/returned missionary
heads. Fifteen artifact commissions retain their exact bindings, including two
cosmic dust and a Storm recipe different from Winterhaven. The planar elixir
requires five potions and six distinct essences; basic earrings require two
matching scrolls, and the final service requires nine kinds plus 500,000 copper.

The plan records five makers without ordinary active placement, missing local/
foreign material sources, absent Pellops gameplay, quest-like room promises,
random attribute-scroll publication ordering, mixed fee/stat service gaps and
bounded prose/ambient-assignment repair decisions. Ordinary lizard/baby/trophy
stock does not prove personal source, freshness or rescue. Cerrio's world-quest
callback remains reachable through secondary shop dispatch. Mode-two timed resets
remain distinct from the Twin Keeps' epic/reset scheduling.

Focused checks passed:

- `test_zone_story_quest_production_catalog.py`: snapshot/inventory agreement,
  all sixteen source indices, contact/topic aliases, missing ordinary maker
  placements, initial fish/cat source parents and actual quarter/key/head stock.
- `test_zone_story_quest_story.py`: all 39 native maps and schemas 1/2/3;
  exact same-named quarters and transformed head, supplied finales skipping
  producer history, spent head remaining missing after captain receipt, two dust,
  eleven elixir ingredients, matching scrolls, visible mixed-fee refusal, service/
  rejection exclusion and 31 recovered independent achievements. Equal-output
  Winterhaven Storm credit does not complete Bloodstone's commission. Assertions
  inspect the owning row when recipes share a giver/final-step text.
- `test_zone_story_quest_production.py`: runtime catalog/bootstrap regression.
- Cached SQL C++20 server build, whole-file harness formatting and whitespace.
- All 2,668 native definitions, registry, fingerprint/revision and other 38 maps
  remain unchanged as parsed objects; all local document links resolve.

The catalog has 39 authored journals, 2,323 achievement units, 1,930 potential
daily units and 2,507 total rows including services/administrative content.
These are projection/source checks, not played fee/source/access qualification.
Accounting remains required. No migration, activation, DB/server operation or
merge occurred. Sixteen of 220 roadmap areas are source-comprehensive, with
204 pending. Continue with Neverwinter Woods, then the Clawed Caverns.

### Neverwinter Woods checkpoint — October 2, 2026

Priority 17 (`moria`) is source-comprehensive at journal revision 1. Reviewed
all seven native blocks (five Q/two M), 335 rooms, 38 mobiles, 75 objects,
three shops, 424 resets/all 120 grouped families, all 28 assigned mobile
routines and the complete 1,516-line local implementation. Exact rune sources,
switch/loading/matching, movement and reciprocal maze topology were reviewed;
the actual horseshoes' foreign Wicks consumer retains Divine Home ownership.

Five equal-offering contracts project one five-rune story/achievement/potential
daily unit. Native Q prepending and first-complete matching currently select the
ruby eyepatch, without a reward-choice step. Every native binding is retained
for historical receipts. Seven contacts cover both addressable topic families;
five live rune checks distinguish the kinds. Supplied runes qualify, optional
bridge-wand preparation cannot block `Next:`, and readiness does not record
source, combat, learned lore or forest-restoration history.

The bridge normally resets open; the advertised wave command differs from the
wand's actual use trigger. All 36 mirror-maze rooms have internal raw targets,
with exterior entries lacking inward reverse connections. The complete special
review records incorrect boundary lookup, virtual/real room identity, unchecked
reciprocal mutations and failed-move closure. Concrete builder decisions and
deterministic topology/arrival fixtures are planned while preserving difficulty.
Agatha debt/news/horseshoe, farming/logging and escort promises remain lore until
deliberately authored. No live crash or completed played journey is claimed.

Verification passed:

- `test_zone_story_quest_production_catalog.py`: exact catalog/inventory and
  seventeen evidence indices, contact/topic ownership and cap-one rune sources.
- `test_zone_story_quest_story.py`: all forty native maps and schemas 1/2/3;
  missing/spent kinds, supplied sets, optional bridge preparation, side-effect-free
  reads, unseen-giver visibility and once-only historical-variant recovery.
- `test_durable_quest_offering.py`: actual native durable matching selects the
  first loaded equal-offering binding for each presented rune with accounting
  active/inactive. The fixture was repaired for the existing V6 continuation,
  shared fourteen-root limit and frozen daily interface; prior skill/XP/item/
  currency publication and recovery cases pass. This fixture tests matching/
  frozen terms, not a full world boot or active-stock journey.
- `test_zone_story_quest_production.py`, harness formatting/check, whitespace,
  reviewed local documentation links and cached SQL server build.

The catalog has forty authored journals, 2,319 achievement units, 1,926 potential
daily units and 2,503 rows including services/administrative content. All 2,668
raw definitions, registry/revision/fingerprint and the other 39 journals remain
unchanged. Accounting remains mandatory for player surfaces and new eligibility;
no activation, migration, DB/server operation or merge occurred. Seventeen of
220 roadmap areas are source-comprehensive, with 203 pending. Continue with the
Clawed Caverns, then the Defense of Longhollow; active-world qualification remains
open.

### Clawed Caverns checkpoint — October 2, 2026

Priority 18 (`clwcvrn`) is source-comprehensive at journal revision 1. Reviewed
all twenty Q/eight M blocks, 89 rooms, forty mobiles, 55 objects, 171 resets/
all 106 grouped families and the entire 309-line local implementation. All five
procedures and 26 mobile/one object binding, including the computed 23-kind
death range, were reviewed with shared dispatch, death/custody, switch, movement,
keys and quest matching. There is no local shop. Foreign review is bounded to
the reciprocal Underdark entry and actual 827 livingstone prototype.

The initial crafting-only proposal is expanded to the actual king story:
remote hot switch → guardian rainbow key → imprisoned mage's key-to-shards
transformation/removal → exact shards for king's rainbow mask. One story,
five shaping/moss services, one paid clue service and thirteen excluded returns
explain all twenty native bindings. Ten contacts cover all eight addressed
families. Optional glowing/rainbow keys preserve supplied final shards; no
native receipt is invented for the custom transformation or personal kill.

Concrete additions to the plan cover named-target validation before hot-crystal
damage and mage consumption, committed nested guardian death/currency disposition,
recoverable exact key/output and recipient retirement, confirmed passage/retrieval
and paid-clue settlement. The mage removes the key before confirming output and
extracts itself even on creation failure. The normal ward blocks attack/casting;
its custom extraction is not a combat kill. Entrance levels, unlocked/locked/
concealed doors, remote push command and zero-break-chance vault keys are documented
without imposing them on supplied-shard delivery. Returned-kind rewards may be
replacement instances, so feedback must preserve lineage rather than mint personal
source credit. No active-world journey or played crash is claimed.

Verification passed:

- `test_zone_story_quest_production_catalog.py`: exact snapshot/inventory,
  eighteen evidence indices, complete contact/topic/binding ownership, source
  placements and no ordinary rainbow-shard reset producer.
- `test_zone_story_quest_story.py`: all 41 maps/schemas 1/2/3; unseen-king
  visibility, intact-key/ordinary-pile rejection from readiness, exact shaping
  colors, authored coin-only-service warning, supplied shards without optional
  keys/history, side-effect-free reads, service/return exclusion and recovered
  final delivery. Shared-row assertions inspect the owning story/service.
- `test_zone_story_quest_production.py`, harness formatting/check, whitespace,
  reviewed local links and cached SQL server build. A transient WSL connection
  failure during the first format invocation cleared on retry; formatting and
  native checks completed successfully.

The catalog has 41 authored journals, 2,300 achievement units, 1,921 potential
daily units and 2,490 rows including services/administrative content. All 2,668
raw definitions, registry/revision/fingerprint and the other forty journals are
unchanged. Accounting remains mandatory; no activation, migration, DB/server
operation or merge occurred. Eighteen of 220 roadmap areas are source-comprehensive,
with 202 pending. Continue with the Defense of Longhollow, then the Black Pearl;
active-world source/transform/access qualification remains open.

### Black Pearl checkpoint — October 2, 2026

Priority 20 is now source-comprehensive with
[its dossier](zone-stories/THE_BLACK_PEARL.md) and
[reproducible index](../reference/zone-story-audits/blackpearl.md).
Reviewed all 31 Q/QA and fourteen addressed M families, 84 rooms, 63 mobile
and 98 object prototypes, 391 resets/all 187 grouped families, zero local shops
and literal specials, disabled mobile-ship implementation/linking and relevant
foreign key/procedure/topology and shared execution. The checked-in optional
world trigger file is absent; no local data procedure binding was found.

Revision 1 preserves all native terms and explains the complete intended
fragment/courier/hilt/reconstruction/skin/gem route. Two finales and twelve
requests contribute fourteen achievements and six potential dailies.
Seventeen services include nine meaningful returned-piece briefings, the
invitation, paid gadget and six hunter trades. Twenty-six contacts cover all
fourteen topic families; 25 optional checks preserve supplied physical pieces,
letters, skin and gems. Three original fragments must become Warthehr's exact
replacements; four horn kinds and nine reconstruction roots are distinct despite
shared names. The two letters to Lyle feed different exchanges. A final skin
receipt does not prove personal combat, sword use or the whole expedition.

The significant source findings are all twenty campaign NPCs and four entrance-key
spirits loading only in exitless 142200; missing ordinary invitation, sewer
fragment, royal scepter and chest sources; an outward mountain exit without a
reviewed reciprocal entry; and the disabled mobile hunter ship. Named foreign
destinations remain intended placements rather than available sources. The
source/recipient restoration proposal preserves stable identities, balance,
supplied stock and explicit reset ownership. Ghalasax differs from the wreck's
skeletal dragon, whose skull is not the required skin. Holding-room sources,
recovered stock, normal boot and mode-zero recurring availability remain distinct.

Foreign NPC encounters can persist under physical-area discovery while the owned
Black Pearl journal remains gated by wreck discovery and hints use the physical
area. The plan adds explicit owned campaign introduction/reveal and correct links
without falsely granting discovery or bypassing discovered-zone daily eligibility.
Same-name transform lineage, fixed twelve-gem actor payout versus party prose,
six empty hunter success strings and 82 empty room descriptions have balanced
repair/qualification proposals. The supported 100,000-copper travel reward is
separate from the unavailable coin-only gadget purchase and fresh reset generation.

Validation completed:

- `test_zone_story_quest_production_catalog.py`: exact catalog/inventory agreement,
  all twenty source indices, Black Pearl identities/topics, holding-only campaign
  placements, absent ordinary item/chest sources and native repeatability/reward
  direction. All 2,668 native definitions, registry, fingerprint/revision and the
  other 42 journals are unchanged.
- `test_zone_story_quest_story.py`: all 43 maps and schemas 1/2/3, native C++20
  warnings as errors, foreign contact without false wreck discovery, unseen-giver
  visibility, exact replacement pieces/four horns/two letters, supplied higher
  inputs without producer history, spent pieces despite receipts, source blockers,
  service exclusion, read-only readiness and independent finale recovery.
  The initial new four-horn fixture omitted one kind; its complete-set setup was
  corrected and the full focused regression rerun passed.
- `test_zone_story_quest_production.py`, actual runtime arrival/accounting-gate
  regression, harness formatting/check, whitespace, all 551 reviewed local
  document links and cached SQL server build passed. The fixtures qualify
  projection/receipt behavior, not restored placements or a played expedition.

The catalog has 43 journals, 2,277 achievement units, 1,913 potential daily units
and 2,489 projected rows. Accounting remains mandatory; no accounting activation,
migration, DB/server operation or merge occurred. Twenty of 220 roadmap areas
are source-comprehensive, with 200 pending. Continue with Ravenloft Catacombs,
then the Realm of Barovia. Active source, access, foreign campaign reveal and
full player-journey qualification remain open.

## Ravenloft Catacombs checkpoint

Priority 21 is source-comprehensive with
[its dossier](zone-stories/THE_RAVENLOFT_CATACOMBS.md) and
[reproducible index](../reference/zone-story-audits/ravenloft2.md).
Reviewed all 199 native blocks: 37 exchanges, 135 addressed responses and
27 ambient actions; 400 rooms, 98 mobs, 327 objects, both shops and 1,457 resets
across all 665 grouped families. Computed shop/switch/teleport roles, shared
dispatch and container placement, temporary vampire spells, brittle keys and
bounded foreign sources/routes are included. No local Strahd form-transition
routine or checked-in world trigger binding was found.

Revision 1 adds twelve stories, thirteen requests and eight services: 25
achievements and eighteen potential daily groups, 25 contacts and fourteen
optional checks. Five clockwork artifacts share one Blinsky recovery achievement.
Four Rahadin favor commissions require five physical coin items and a distinct
scroll, with optional earlier proof history. Three Strahd-skull recipients have
different rewards and retire independently. Two ash recipients remain independent;
neither one delivery nor supplied stock invents another delivery or a full campaign.
The eight services and mode-zero disappearing contacts earn no daily credit.

The source findings expand the universal plan as follows:

| Capability / repair | Evidence and qualification proposal |
| --- | --- |
| ZSQ-RAVEN-CONTAINER-PARENT | P population resolves a matching container globally, including repeated marble slabs and mummified remains. Bind source evidence to an exact parent generation; explicitly decide population compatibility. Test distinct rooms, skipped parents, caps and recovery before promising exact crypt contents. |
| ZSQ-RAVEN-SHOP-STOCK | Blinsky's toy woman is real carried stock and the shared shop can sell it despite its absence from the replenishing product list. Favor scrolls are carried by Rahadin's following cloak, also a shop. Preserve sources; qualify active creation and atomic existing-stock purchase, roaming/fixed-room behavior and merchant retirement separately. |
| ZSQ-RAVEN-FORTUNE / WALLET-SERVICES | Favor coins are five physical objects, not wallet money. Their native item-only rewards and Izek's wine cash rewards use supported recovery. Paid reading/dues use unsupported wallet offerings; add frozen atomic fee/outcome settlement without charging failed or duplicate attempts. |
| ZSQ-RAVEN-READING / LORE | Twelve card faces are stateless addressed responses, accessible without payment or a selected session. Define accepted paid session, choices and terminal policy if intended; add accepted topic/examination events without per-alias achievement inflation. Sasha's Dayheart/Sunsword journal remains optional history. |
| ZSQ-RAVEN-FORMS / RESURRECTION / FACTION | Wolf/human/bat/gas are independent reset mobs; the Kasimir corpse and Patrina banshee are already stocked. Vampire potion effect is temporary, with a Theurgist alternative. Define committed actual outcomes and generations before claiming personal kills, resurrection, permanent race/faction or healed sunlight. |
| ZSQ-RAVEN-ACCESS / OWNERSHIP | Old and spectral keys have 100 break chances; returned old key has a new identity. Carried versus ground switches differ, and foreign gateways are not direct reciprocal entrances. Qualify actual key/switch/travel with exact source/destination and consumption recovery; foreign encounters must not fabricate owned-area discovery. |
| ZSQ-RAVEN-SKELETON / EPITAPH | Search switch 59301 targets nonexistent room-59145 direction 8; apparent reciprocal is direction 9. Builder confirms intended target, then tests both sides and safe failure. Alternate Sasha epitaph 59132 has no ordinary placement; choose restoration with a clue or retirement. |
| ZSQ-RAVEN-TERMS / REVEAL | Phylactery and Izek key lack confirmed access targets; Ezmerelda/Rictavio promise keys absent from actual outputs. Preserve IDs/balance while deciding ceremonial reward versus destination or prose repair. Stage future personal-source and Dayheart campaign clues by accepted evidence. |

Source-comprehensive and gameplay-qualified remain separate. Active stock,
shopping, exact crypt contents, real traversal/key fracture, chosen repairs and
full committed player journeys are still open. The updated catalog has 44 journals,
2,265 achievement units, 1,905 potential daily units and 2,485 projected rows;
all 2,668 native receipt identities and the source fingerprint are preserved.
Twenty-one of 220 roadmap areas are source-comprehensive, with 199 pending.
Continue with the Realm of Barovia, then the Lost Temple of Tikitzopl. Accounting
remains mandatory throughout this work.

Validation for the Ravenloft checkpoint:

- Windows production-catalog/source regression and the native all-map schema/
  journal regression passed, including exact quantities/roles, supplied preparation,
  Blinsky alternatives, independent skull/ash recipients, service exclusion,
  foreign encounter ownership, read-only readiness and saved receipt recovery.
- Runtime production and actual arrival/accounting-gate regressions passed.
  The cached SQL server build, authoritative harness format/check and whitespace
  passed; all 682 reviewed local document links resolve.
- All 2,668 raw definitions, native reward terms, receipt IDs, registry, revision/
  fingerprint and the other 43 sidecars remain unchanged. No active-world source
  qualification, accounting activation, migration, DB/server operation or merge
  was performed.

## Realm of Barovia checkpoint

Priority 22 is source-comprehensive with
[its dossier](zone-stories/THE_REALM_OF_BAROVIA.md) and
[reproducible index](../reference/zone-story-audits/barovia.md). Reviewed all 87
native blocks, 168 rooms, 57 mobs, 66 objects and 405 resets/all 194 families;
shared inn, door/key, falling, switch, teleport, trap, combat and giving roles;
bounded foreign Gertruda/key producers and incoming routes; and the existing
ordered Doru/Chernovog/Castle Strahd achievement, eligible group callers and login
cleanup. No local shops, literal specials or data procedure bindings were found.

Revision 1 maps all nine exchanges into five stories, one request and three
services: six achievements/four potential dailies, 24 contacts and ten optional
checks. Bildrath requires nine distinct directly carried items. Ismark's two
notes remain independent; optional Ireena returns are meaningful services with
replacement identities. Supplied heart, plan, brooch and daughter remain valid
without invented personal sourcing or campaign history. Retiring Ephon/Mary and
all three services earn no daily credit. Ismark's cash reward is supported;
Parriwimple's wallet-only clue is still unavailable under active accounting.

| Capability / repair | Evidence and qualification proposal |
| --- | --- |
| ZSQ-BAROVIA-LEGACY-CAMPAIGN / SESSION | Actual ordered combat tags use Doru 91031, Chernovog 58835 and Castle Strahd 58383; nearby eligible group members can receive progress, and login clears incomplete tags. Define an explicit campaign/attempt bridge with exact victim generation, frozen recipients and recoverable reward. Builder decides retained session policy versus durability; gifts and Catacomb skulls cannot infer kills. |
| ZSQ-BAROVIA-OBJECT-RESCUE | Gertruda 58412 is a foreign treasure object, consumed by Mad Mary; no NPC escort/reunited pair exists. Add authored representation/source/transfer lineage and distinguish narrated rescue from actual movement. A real escort requires selected outcome and balance, not automatic reinterpretation. |
| ZSQ-BAROVIA-PROSE / TERMS | Mathilda actually equips Ashlyn's brooch despite Thendrick thanks. First letter is in Kolyan's body and consumed for cash despite robe/return narration. Select bounded source/prose or replacement repairs without changing historical IDs/payouts inadvertently. |
| ZSQ-BAROVIA-TRAP | The anti-magic orb's type-9 trap lacks primary trigger flags and supported damage dispatch. Builder chooses effect/trigger or a committed custom procedure; test actual activation, failure/replay and bounds. Existing falling and ambush stock stay separate. |
| ZSQ-BAROVIA-BARRICADE / REVEAL | Several keyless barricades have asymmetric reset states; glen raw secret bits lack concealment resets. Define intended loaded door/breach/reveal policy, then qualify exact accepted unlock/switch and confirmed travel instead of inferring a gate from prose. |
| ZSQ-BAROVIA-FOREIGN-GATES | Ephon's Barovia key and foreign outer-castle key have 100 break chances and multiple targets. Megosh is physically at the castle gate but owned by continued Barovia. Preserve exact key/receipt/owner identity; qualify fracture, alternative entry, confirmed arrival and owned journal links without false discovery. |
| ZSQ-BAROVIA-NARRATIVE / LORE | Bible/Pelor journals, Sunsword, restored companions, liberated village and family welcome have no verified local durable terminal. Stage accepted topic/examination and explicit selected closure; preserve optional lore and supplied finals without per-keyword achievements. |

The current catalog contains 45 journals, 2,262 achievement units, 1,905 potential
daily units and 2,485 projected rows, preserving all 2,668 native identities.
Twenty-two of 220 roadmap areas are source-comprehensive, with 198 pending.
Continue with Lost Temple of Tikitzopl, then the Jade Empire. Actual active-world
source, fee, key/travel, ordered combat and selected repairs remain unqualified.
Accounting remains mandatory; the full roadmap goal remains in progress.

Validation for the Barovia checkpoint:

- Windows production-catalog/source regression and the native all-map schema/
  journal regression passed for 45 maps. New cases cover nine distinct kinds,
  equipped versus carried ring/pin, physical electrum coin, optional clue/key/
  earlier-note history, supplied finals, service exclusion, supported cash versus
  unavailable clue fee, unseen-giver visibility, read-only readiness and independent
  saved receipt recovery. Source tests verify every local offering's actual parent
  or carrier and all addressed topic aliases.
- Runtime production and actual arrival/accounting-gate regressions passed.
  The cached SQL server build, authoritative harness format/check and whitespace
  passed; all 564 reviewed local document links resolve.
- Review corrected Megosh's physical castle placement versus continued-Barovia
  quest ownership and preserved canonical generated-catalog formatting. All 2,668
  raw definitions, terms, registry, fingerprint/revision and the other 44 sidecars
  are unchanged. No played active source/access/combat/fee journey, accounting
  activation, migration, DB/server operation or merge was performed.

## Lost Temple of Tikitzopl checkpoint

Completed priority 23 with the [source dossier](zone-stories/LOST_TEMPLE_OF_TIKITZOPL.md)
and [reproducible index](../reference/zone-story-audits/tikitt.md). Reviewed all
38 native blocks, 234 rooms, 76 mobs, 97 objects and 340 resets/all 202 families;
all five literal object procedures and relevant shared giving, examination,
switch/door/key, reflection, epic class-change and teleport execution; all fifteen
foreign recipe kinds, kitty/approach key and actual foreign entrance/return.
No local shop or data procedure binding exists.

Revision 1 classifies 29 exchanges as two stories, two requests and 25 equipment
services: four achievements/potential dailies, nineteen contacts and 44 optional
checks. Exact current copies, distinct attribute/metal/racial kinds and the three
same-named sapphire necklaces are required. Optional producer/key receipts cannot
replace spent supplies. Native gifts remain valid. Each Orb service consumes its
Orb; elemental armor also competes between Zynar and enhancement. The existing
100-EP altar debit precedes its class mutation, while durable class outcome and
full reflection/cooldown recovery still need qualification.

| Capability / repair | Evidence and qualification proposal |
| --- | --- |
| ZSQ-TIKITT-RECIPES / ALLOCATION | Repeated 5/10/4/5/5 counts fit declared caps; nine distinct emblems/earrings, eight racial rings and three same-name necklaces are not interchangeable. Fifteen services compete for cap-one consumed Orb; recipients also equip crafting stock. Add conditional producers, exact UID allocation and admitted recipient/reset episodes, preserving supplied materials and service exclusion. |
| ZSQ-TIKITT-ACCESS / CLUES | Gem key is inside cot, rack has vials; push clears block but leaves royal passage locked/secret. Seven accepted speech words precede an already-open final edge. Foreign Winterhaven entry and one-way array/leaf-key return are valid alternatives. Add accepted examination, switch/unlock/open/retrieval and confirmed travel tied to actual target/generation; optional history cannot gate supplied finals. |
| ZSQ-TIKITT-CLASS-SERVICE | Existing 100-EP debit already waits for committed settlement before secondary class/spec/spell mutation. Preserve it; qualify frozen actor/attempt and durable class effect through admission, failure, disconnect, retry and restart. Prayer is separate from Orb recovery. |
| ZSQ-TIKITT-POWER-OUTCOME | Mace schedules cooldown/vibration after void reflection call, including refusal or partial mobile creation. Add typed successful/partial outcome and deliberate publication policy; test titan conflict, load failure, early-repeat damage, remove/all/extraction and event recovery. Preserve mangler's re-resolved attack continuations and working armor/shield effects. |
| ZSQ-TIKITT-NARRATIVE / ORPHANS | Resistance prose has no reviewed takeover or permanent mage/undead closure; Ancient Crypt is blank/empty. Shield procedure retaliates without explicit knockdown. Builder chooses narrative/terminal and bounded text/power/crypt repairs, rather than inferring failure of ordinary bash or inventing quest history. |

The current catalog contains 46 journals, 2,237 achievement units, 1,880 potential
daily units and 2,485 projected rows, preserving all 2,668 native definitions.
Twenty-three of 220 roadmap areas are source-comprehensive, with 197 pending.
Continue with the Jade Empire, then the Savannah of Broken Trusts. Actual active
source, access, power and class-change journeys remain unqualified. Accounting
remains mandatory; the full roadmap goal remains in progress.

Validation for the Tikitzopl checkpoint:

- Windows production-catalog/source regression and native all-map schema/journal
  regression passed for 46 maps. Every recipe was checked with exact supplied
  ingredients, one missing root, equipped substitutes and extra copies of a
  different kind. Optional key/producer history, spent stone/Orb, independent
  mirror/clover outcomes, service exclusion, read-only readiness and saved receipt
  recovery passed. Source tests verify native terms, all addressed topic aliases,
  actual cot/altar parents, magician placement and royal switch/reset target.
- Runtime production and actual arrival/accounting-gate regressions passed.
  Cached SQL server build, authoritative harness formatting and whitespace checks
  passed; all 563 reviewed local document links resolve.
- Review removed a duplicate optional clover receipt and invalid apostrophe
  command aliases, and verified room journal visibility before object descriptions.
  All 2,668 native definitions/terms, registry, fingerprint/revision and the other
  45 mappings are unchanged. No played active source/access/power/class journey,
  accounting activation, migration, DB/server operation or merge was performed.

## Jade Empire checkpoint

Completed priority 24 with the [source dossier](zone-stories/THE_JADE_EMPIRE.md)
and [reproducible index](../reference/zone-story-audits/jade.md). Reviewed all
41 native blocks, 340 rooms, 134 mobiles, 130 objects and 583 resets/all 331
families; all three shops; literal crew/shipwright and computed smith services;
shared native exact giving, keys/switches/teleports, fishing ownership,
mining/forging refusal and relevant foreign proof, parent and rescue boundaries.
No local explicit data procedure adds a custom quest handler.

Revision 1 projects all 37 contracts into eight stories, nine requests,
seventeen supporting exchanges and two exclusions, with equivalent one-fish
alternatives sharing one request. Thirty-eight contacts cover all givers and
three addressed blocks; 35 optional checks preserve supplied proofs. Exact
five-rice/four-meat/two-orchid/four-directional quantities and competing
rice/turtle/net/heart/invitation/electium/egg supplies remain distinct. A current
heart is returned with new lineage in the optional briefing. Native cash rewards
are supported; paid offerings do not acquire support from journal metadata.

| Capability / repair | Evidence and qualification proposal |
| --- | --- |
| ZSQ-JADE-SUPPLY / PAYMENT | New nets require an unsupported 10,000-copper purchase; foreign maps cost 50,000 copper; four commissions mix items and money. Exact legacy mithril 233 has no declared producer, while current mining uses different kinds. Add durable fee/material/output admission and selected guarded source/recipe repair; keep deliberate forge/mining refusal until qualified. |
| ZSQ-JADE-CAPTURE / RESCUE | Native knife/net/shackle deliveries retire runtime recipients and grant carried turtle/captive/token outputs; no follower/escort is implemented. Link accepted actor/attempt, exact consumed and output UIDs and generation; personal recovery/rescue objectives are explicit opt-ins, never inferred for donated proofs. |
| ZSQ-JADE-ACCESS / TRAVEL | Five valid switches, breaking guild key, distinct dice/royal/brig/shackle kinds and keyless helm trapdoor have alternate cave/mirror/crack/water routes. Brig key opens the foreign cell; shackle keys pay the princess, whose token pays the Emperor. Emit real accepted access/arrival episodes, preserving supplied finales and foreign ownership. |
| ZSQ-JADE-ALLOCATION / EPISODES | Chance-limited plants/templates/ore, four meat roots, competing food/capture/metal/egg uses, same-kind heart replacement and retiring/load-room mobiles need exact UID allocation and generation-aware availability. NORENT and the word fresh do not establish hunting/freshness. Qualify Kyan/eye/Nansuo movement before declaring unreachable sources. |
| ZSQ-JADE-FISHING | Fishing has a real ownership grant, but catch narration, pole damage and XP precede its accepted result. Freeze the catch/actor/pole episode and deliberately publish/compensate effects on accepted/refused/partial/recovered outcome; add a specific source adapter separately from broad crafting provenance. |
| ZSQ-JADE-INCOMPLETE | Huge-hide retirement is a deliberate rejection. White silk belt is consumed with no outcome; select reward/service or removal and audit nested ampoules. Legacy ore, absent army/escort/rickshaw/religion closure, blank inn/load prose and canner fee mismatch need bounded builder decisions rather than invented progress or balance changes. |

The current catalog contains 47 journals, 2,217 achievement units, 1,866 potential
daily units and 2,482 projected rows, preserving all 2,668 native definitions.
Twenty-four of 220 roadmap areas are source-comprehensive, with 196 pending.
Continue with Savannah of Broken Trusts, then Alatorin. Actual active sources,
payment, access, capture, fishing and persistence journeys remain unqualified.
Accounting remains mandatory and the full roadmap goal remains in progress.

Validation passed: production-catalog source/snapshot/inventory and 24 evidence
indices; native story/projection, production-runtime and arrival regressions;
authoritative harness formatting, whitespace and cached SQL build. All 2,668
native definitions/terms and the other 46 maps remain unchanged as parsed
objects; 567 changed-document links resolve. No accounting activation,
migration, DB/server operation or merge is part of the source mapping.

## Savannah of Broken Trusts checkpoint

Completed priority 25 with the
[source dossier](zone-stories/SAVANNAH_OF_BROKEN_TRUSTS.md) and
[reproducible index](../reference/zone-story-audits/savannah.md). Reviewed all
36 native blocks, 167 rooms, 59 mobs, 39 objects and 315 resets/all 105 families;
there is no literal local special, explicit local data procedure or shop file.
Shared exact giving, stock generation, ordinary followers/movement, doors/search,
examination, falling and bard use/sleep boundaries were reviewed. Bounded
foreign evidence covers six Mitashi swords/carriers, Kunji/Retribution and
warehouse rooms, the nine-item Air consumer and retained but inactive Hostel data.

Revision 1 projects seventeen contracts into five named delivery achievements/
potential dailies and twelve separate instrument equipment services. Seventeen
contacts cover all nineteen addressed topic families; twelve optional checks
retain matching-base and Kunji history without requiring it for supplied items.
No single instrument exchange completes a collection. Three tusks and three
skins pay different gifts; feather, bones and Kahir essence have their own
outcomes. Native cash reward remains supported. Exact stock declarations fit
the three-part counts, but actual generation and availability remain unqualified.

| Capability / repair | Evidence and qualification proposal |
| --- | --- |
| ZSQ-SAV-SOURCE / ACCESS | Parts are ordinary stock, not automatic fresh kill products. Well drops into a pit with no up edge; its secret western exit connects to termite tunnels, while Kahir uses a different closed/unkeyed wall and the laboratory route has falling. Add explicit accepted source/search/open/arrival episodes and test both approaches, gifts, hazard and escape. |
| ZSQ-SAV-ALLOCATION / EPISODES | Six cap-one sword kinds pay six separate base recipes or one six-kind Kunji recipe. Each epic consumes matching base plus current Retribution and retires Lynstar; Kunji also retires. Fresh all-six epic construction uses seven copies of each katana and six appearances of each giver, while gifts can shorten preparation. Qualify exact UID allocation, stock generation, recipient episodes, foreign competing relic and recovery. |
| ZSQ-SAV-COLLECTION / TRIBAL | No active consumer accepts the six epic instruments; referenced Hostel is absent from the active list. M'Bele/god claws exist without god stock or reviewed chief-death finale. Builder chooses real recipient/reward/episode or lore retirement; sidecar metadata cannot implement collection, war, god appearance or homeward escort. |
| ZSQ-SAV-DIALOGUE | Conservative extractor drops two whole families because `m'bele` is among otherwise valid aliases. Chief/Watcher `contest` topics are authored now. Plan a focused usable-alias/raw-source distinction and affected-index regression; accepted learned topics remain a separate adapter. |
| ZSQ-SAV-PROSE / NOSLEEP | Correct repeated lyre versus actual recipes and village-versus-pit description if selected; clarify absent sister/god/escort lore fairly. One invisible NOSLEEP item is carried instead of worn, which protects against reviewed bard sleep but not the equipment-only sleep spell path; qualify intended protection before changing its reset. |

Current catalog: 48 journals, 2,205 achievement units, 1,854 potential daily
units and 2,482 projected rows, preserving all 2,668 native definitions and
other 47 parsed maps. Twenty-five of 220 roadmap areas are source-comprehensive,
with 195 pending. Continue with Alatorin - the Forge City, then Newhaven.
Alatorin's 495 contracts require a complete district/service-family audit.
The full roadmap goal remains active; played active-world qualification is
independent of source completeness. Accounting remains mandatory.

Focused source/native checks, cached SQL build, formatting, term/map preservation
and local document link verification accompany this checkpoint. Fixtures are
synthetic states, not played sources, travel, songs or recipient episodes.
No accounting activation, DB migration/server operation or merge is included.

## Alatorin completed source map

Completed priority 26 with the [source dossier](zone-stories/ALATORIN.md),
[revision 1 journal](../../areas/story/alatorin.story.json) and
[reproducible index](../reference/zone-story-audits/alatorin.md). All 495 native
contracts are classified into eighteen stories, 72 requests, 163 services and
four rewardless-return exclusions: 90 achievements, 82 potential dailies and
253 rows. Ninety-four contacts cover every one of 296 raw addressed families,
including five with usable aliases alongside apostrophes; 548 optional checks
explain exact stock and selected earlier exchanges without inventing personal
history. The conservative source index reports 291 topic blocks.

Reviewed all 961 native blocks, 952 rooms, 425 mobiles, 602 objects, fifteen
shops, 3,596 reset rows/1,821 families and 336 foreign item prototypes. Bounded
foreign supply review covers 49 representative inputs, 41 carriers, all selected
area M/F declarations and 76 rooms, plus the githzerai locker and Crodog route.
Only loaded AREA world files establish current source identity. The local
procedures, relevant shared source/publication boundaries and command dispatch
were reviewed; this does not claim comprehensive foreign-area maps or played
active-world availability.

The journal preserves the 209 ordinary fragment trades as one equal-outcome
service; separate nebula/vellum/random/tinkerer outcomes remain explicit. Exact
arcanum, steel/cold-iron and wood/leather alternatives use complete native
recipes and single-kind live checks. Four crystal buybacks share a service with
four precise terminals; Brino's eleven prices remain separate. Different
recipients, competing proofs, seven divine gifts, meaningful shard replacements
and both overlapping blue-tunic contracts keep distinct identities. The mapping
fits existing row/step/topic/item/byte limits without a new schema or API.

Findings incorporated into the shared plan:

- Active reset-generation admission remains the first prerequisite. Mining,
  smelting, salvage, refining, forage and paid native offerings remain guarded;
  supplied/admitted stock is a separate valid route. Repair refining's confirmed
  released-input reads using stable pre-retirement data, then qualify complete
  material/tool/fee/catalyst/random settlement before enabling preparation.
- Typed money-pile gifts, actual slot flags, static clues, rare/nested supplies,
  shrine `hit`, treasury `push`, vine `grab`, brittle keys, the malformed wall
  and blocked royal gate require precise qualification or builder decisions.
- Lancer supplies all five elemental materials for one gem-studded hide.
  Crodog's five nightcrawler claws produce the mountaineer insignia. Foreign
  ownership, dispersal starts and supplied proofs do not establish local personal
  hunting, travel, allegiance or full-stage history.
- No ordinary native/reset/explicit compiled producer was found for tinkerer
  token 83458 or prison key 83294. The thug's barrier alias has no corresponding
  binding; the old parchment lesson is disabled. Restore or retire those pieces
  through builder-selected sources/targets/terms, retaining valid supplied inputs.
- Resolve exact command target, actor and custody before the broadly matched
  scroll transform or ring interception. Freeze random results and preserve
  first-level-50 githyanki gift entitlement through rejected grants and restart.
  Ignored publication results and stale prose do not prove a played crash.

Current catalog: 49 journals, 1,800 achievement units, 1,545 potential daily
units and 2,240 projected rows, preserving all 2,668 native definitions,
revision two and the native fingerprint. **26 of 220 roadmap areas are
source-comprehensive; 194 remain.** Continue with Newhaven, Faerie Realm,
Verspin and Ship Yards. Source completion is separate from active gameplay
qualification; accounting activation, migrations, DB/server operations and
merge have not been performed.

Validation: focused production-catalog and native story projection regressions;
exact contract/recipe/price/topic classification and optional-supply checks;
generated catalog/index/inventory reproducibility; source/access/reset/material
metrics and local links; changed-line formatting, whitespace and cached server
build. See the PR/checkpoint result for executed outcomes and limitations.

## Newhaven completed source map — October 3, 2026

Completed priority 27 with the [source dossier](zone-stories/NEWHAVEN.md),
[revision 1 journal](../../areas/story/newhaven.story.json) and
[reproducible index](../reference/zone-story-audits/newhaven.md). All nine
native exchanges become one story, two requests and six services: three
achievement/potential daily outcomes and nine rows. Seventeen contacts cover
all thirteen native speakers/both addressed families and four useful carriers;
twelve optional checks preserve supplied materials and foreign receipt ownership.

Reviewed all 64 Q/M/MA blocks, 100 rooms, 91 mobs, 44 objects, four shops,
252 resets/all 187 families, literal and property-driven dispatch, exact
teleports/keys/traps/inns and bounded foreign proof suppliers and surface
destinations. The blacksmith, berries and tobacco are independent accepted
deliveries. Five mixed-fee equipment recipes stay unavailable under active
accounting. Dibbly's actual pipe buyback is a service while its reel wording
awaits a deliberate content decision; no native reward or binding was changed.

Concrete plan additions cover the collar table inside the blocked alcove,
tail get/put trap and custody, mainland beast dispersal, ROOM_INN automatic
assignment, stale literal pool targets, real counterpart/ground parents and
unfinished librarian/erinyes/prisoner endpoints. Supplied proofs remain valid;
no conversation, unlocked cell, counterpart death or optional earlier recipe
becomes invented personal-source or full-campaign credit.

Current catalog: **50 journals, 1,794 achievement units, 1,544 potential daily
units and 2,240 projected rows; 27 of 220 roadmap areas are source-comprehensive,
with 193 remaining.** The native 2,668 definitions, revision two and fingerprint
and all prior 49 maps are unchanged. Service classification replaces six raw
achievement units and one pipe daily candidate without changing gameplay rewards.
Continue with Faerie Realm, Verspin, Ship Yards and Ultarium. Active-world
qualification remains separate; no accounting activation, migration, DB/server
operation or merge has been performed.

Validation: focused production-catalog and native story/file-loader regressions,
exact fees/identities/source parents/optional foreign history, generated evidence
reproducibility, switch/inn/teleport source checks, document links, changed-line
formatting, whitespace and cached server build. These fixtures do not substitute
for played source, paid, trap, travel or restart journeys; publication records
the executed checks and remaining limitations.


## Faerie Realm completed source map — October 3, 2026

Completed priority 28 with the [source dossier](zone-stories/FAERIE_REALM.md),
[revision-one journal](../../areas/story/realm.story.json) and reproducible
[review index](../reference/zone-story-audits/realm.md). Reviewed every local
Q/M, room, mobile, object, shop and reset family; full local procedures and
their actual dispatcher; five planar supply carriers/rooms, alternate Fix
sources, and all 23 loaded foreign boundary/dispersal destinations. Bounded
foreign evidence does not claim comprehensive foreign-zone qualification.

The journal separates Finn's signet, his retiring key exchange and Celriya's
retiring blade from two services and one unrewarded walnut response. Ten
contacts cover all ten addressed families; thirteen optional checks explain
exact supplies and earlier receipts. Equivalent makers retain both identities
and five separate component kinds. Supplied key/blade routes remain valid,
without personal kill, full history, escort or revived-king assumptions.

Recorded concrete plans for the unserved combat-helper callback, per-appearance
state/difficulty, wandering sources, exact speech/key/orb behavior, ring/scroll
and shop prose, unfinished riddle, missing exits/reset object, alternative
source ownership, five-item fee settlement and targeted Fix consumption/effect.
Builder review selects world repairs before changing native content or balance.

Current catalog: **51 journals, 1,790 achievement units, 1,543 potential daily
units and 2,238 projected rows; 28 of 220 roadmap areas are source-comprehensive,
with 192 remaining.** All 2,668 native definitions, revision two, fingerprint,
zone registry and prior fifty parsed maps remain unchanged. Services/exclusion
and equivalent recipe presentation change denominators without changing rewards.
Continue with Verspin, Ship Yards and Ultarium. Active-world qualification
remains pending; no accounting activation, migration, DB/server operation or
merge has been performed.

Passed `test_zone_story_quest_production_catalog.py` and the C++20
`test_zone_story_quest_story.py`: exact bindings/classification/topics/reset
parents, all 28 reproducible indices, all 51 maps/native loading, a supplied-key
journey without ring history, independent receipt recovery, service exclusion
and five distinct material checks. The maintained SQL `make -C src` build
completed with its existing objects current. Changed-line/touched-file
formatting and `git diff --check` passed. Source/invariant review confirmed
605 local/source-line links, the 23 loaded boundary targets and unchanged
native definitions and fifty prior maps. These fixtures do not certify played
combat, source, paid, travel or targeted-repair journeys.


## Verspin completed source map — October 3, 2026

Completed priority 29 with the [source dossier](zone-stories/VERSPIN.md),
[revision-one journal](../../areas/story/verspin.story.json) and reproducible
[review index](../reference/zone-story-audits/verspin.md). Reviewed every local
native block, room, prototype, shop and reset family, both literal room services,
computed teacher/inn properties, packed weapon behavior and shared admission,
reward, XP, movement and access paths. Bounded foreign evidence includes the
actual rare gem thief, single sigil parent among three apprentices, all foreign
reset supplies/rewards and both incoming room records.

The journal separates circus totems, lion collar, stolen amethyst, shrine
symbols, Transo's three colors and the monk's foreign sigil from Ramous's
producer and five paid equipment services. Eighteen contacts cover all nine
addressed families; eighteen optional checks preserve current counts/kinds and
producer history. Supplied proofs remain valid. Native retirement, group XP and
foreign supply do not establish personal kills, persuasion, reunion or a full
tower campaign. All twelve exact contracts and original rewards remain intact.

Expanded repair/qualification plans cover the absent ten-offer sign/five actual
recipes, four-red/three-cap conflict, misleading gem clue, copied descriptions,
real spectator versus pit access, secret/falling/water paths, nine virtual stat
purchases and ignored denied crew payment. Expected-stat/effect/save and
wallet/ship settlement extend existing service work; actor/companion XP policy
needs explicit intent before a balanced correction. No native world, price,
reward, cap or combat-power change is made by the journal.

Current catalog: **52 journals, 1,784 achievement units, 1,542 potential daily
units and 2,238 projected rows; 29 of 220 roadmap areas are source-comprehensive,
with 191 remaining.** All 2,668 native definitions, revision two, fingerprint,
zone registry and prior fifty-one parsed maps remain unchanged. Service
classification changes denominators without changing native receipts/rewards.
Continue with Ship Yards, Ultarium and the Surface Realm. Active-world
qualification remains pending; no accounting activation, migration, DB/server
operation or merge has been performed.

Focused validation protects exact bindings/topics/reset parents, producer versus
terminal independence, supplied-bone acceptance, repeated quantities, separate
colors, service exclusion, foreign proof ownership and receipt recovery. The
catalog/index/invariant/link review, formatting, whitespace and maintained
server build complement these fixtures; they do not certify played source,
combat, paid, stat, crew or travel journeys.

Validation completed: the production catalog/source regression and native
C++20 regression passed for all 52 maps and all 29 reproducible source indices.
The supplied-bone route, five-count collections, three exact colors, six service
exclusions, foreign ownership and six independent outcomes survived recovery.
The maintained SQL server build completed with current objects; changed-file
formatting and whitespace passed. Source/invariant review verified 615 local
links, exact boundary/property/cap evidence and unchanged native definitions,
revision/fingerprint, registry and fifty-one earlier maps. These fixtures do
not qualify played source, paid, stat, crew, combat or travel journeys.


## Ship Yards completed source map — October 3, 2026

Completed priority 30 with the [source dossier](zone-stories/SHIP_YARDS.md),
[revision-one journal](../../areas/story/shipy.story.json) and reproducible
[review index](../reference/zone-story-audits/shipy.md). All 124 native blocks,
229 rooms, 102 mobiles, 46 objects, seven shops, 719 resets/265 families, eight
literal services and computed properties were reviewed. Bounded foreign evidence
includes all accepted proof producer/reset occurrences, primary vendor/room/shop
records, a nested ring, captive and foreign alternatives, support prototypes,
loaded boundary/dispersal destinations and actual teleport/locked/ocean access.

The journal ships nineteen independent outcomes, six support services,
thirty-two contacts and thirty-four optional checks. Grimashk's two differently
priced crates share one recovery story with separate receipts; Chundel remains
independent. Exact seafood/material counts, four shivs, six potion kinds and
optional Pol history/note preserve supplied final proof. Native cash/XP/items,
recipient retirement and foreign source ownership stay unchanged.

Recorded balanced plans for Pol's refunded offer/missing pole/note predicate,
Cairme's count conflict, absent fillet, unimplemented transformation/research/
military state, four missing exit targets and ocean viper ecology. Extend the
existing accepted fishing publication and wallet/epic/ship/crew settlement work;
do not require guarded support fees or imaginary personal sailing/hunting stages
for valid native delivery. Actor/companion XP intent needs explicit confirmation
before balance changes, while admitted frozen awards remain recoverable.

Current catalog: **53 journals, 1,777 achievement units, 1,541 potential daily
units and 2,237 projected rows; 30 of 220 roadmap areas are source-comprehensive,
with 190 remaining.** All 2,668 native definitions, revision two, fingerprint,
zone registry and prior fifty-two parsed maps remain unchanged. Classification
changes denominators without changing native receipts/rewards. Continue with
Ultarium, the Surface Realm and Tharnadia. Active-world qualification remains
pending; no accounting activation, migration, DB/server operation or merge.

Focused validation covers all bindings/topics/source parents, supplied terminals,
exact repeated/mixed collections, optional history, two crate alternatives,
independent recipients, service exclusion and receipt reload. Catalog/index/
invariant/link review, formatting, whitespace and maintained server build
complement the fixtures; actual source, combat, paid, fishing, ship and travel
journeys remain unqualified.

Validation passed: production catalog coverage and all thirty review indices,
C++20 native story/schema/projection and receipt-recovery regression, full
harness formatting, whitespace and maintained SQL server build. Source/invariant
review verified 672 local/source-line links, complete local evidence, four
missing exits and the ocean viper destination, with unchanged native definitions,
registry, revision/fingerprint and fifty-two earlier parsed maps. These checks
do not certify played source, fishing, paid, ship, combat or travel journeys.


### Ultarium completion

Priority 31 is source-comprehensive. The [dossier](zone-stories/ULTARIUM.md)
reviews all 22 native blocks/seventeen exchanges/five addressed families,
268 rooms, 82 mobs, 71 objects, two shops and 338 resets/227 families. It
includes all three literal assignments, computed class/epic-teacher roles,
shared pool/stone/ward effects, every exact proof source and bounded foreign
continuation/access. Every loaded local exit/reset reference resolves.

Revision one adds seven independent outcomes and eight support rows, with
twenty-three contacts and twenty-two optional checks. Three unrewarded soul
branches remain one support row, six trapper barters and the gorget remain
services. Four different council souls are required together for the box;
prior donations and duplicate kinds cannot replace current proof. Same-name
study/wind outputs cannot substitute for original proofs. The foreign Xavier
delivery retains Myrabolus ownership and accepts a supplied exact copy.

Source/gift lineage, rare/holding/capped/nested/roaming stock, recipient
appearances, directional keys/boulders and confirmed teleport/fall/effect/class
state need active qualification. Keep rename/pet/epic lesson guards until
coordinated settlement is supported. The commented pet restoration call is
an incomplete legacy claim path; repair or deliberately retire it. Three
unrewarded soul branches, sparse crafting clues and duplicate-name kinds
have balanced builder decisions. No liberation, rescue, golem construction,
personal kill, learned keyword or equipment-use achievement is invented.

Verification covers complete contracts/classification/topics, exact sources,
four-kind and duplicate-name preparation, read-only/supplied routes, independent
receipts, service exclusion, foreign ownership and serialized recovery. Focused
native/Python tests, generated catalog/index, source/link checks, authoritative
formatting, whitespace and incremental server build passed. The invariant/link
review covers 619 local/source-line links and the unchanged native catalog and
earlier mappings. This record claims source coverage; played active journeys
remain unqualified.

Current catalog: **54 journals, 1,767 achievement units, 1,531 potential daily
units and 2,235 projected rows; 31 of 220 roadmap areas are source-comprehensive,
with 189 remaining.** All 2,668 native definitions, revision two, fingerprint,
registry and fifty-three earlier maps remain unchanged. Continue with the
Surface Realm, Tharnadia and Mini Zones. Active accounting remains mandatory;
no operational accounting activation, DB migration or merge is authorized.


### Surface Realm completion

Priority 32 is source-comprehensive. The [dossier](zone-stories/SURFACE_REALM.md)
reviews all 141 native blocks/31 exchanges/36 addressed blocks, 234 mobiles,
78 objects and 1,736 resets/320 families, plus eight literal assignments,
104 computed teacher roles, all proof sources and bounded foreign ownership.
All 160,004 room records were reviewed in 562 exact title/prose groups,
56 complete metadata groups and 311 exit families; 370 loaded outgoing and
423 incoming boundary edges were checked. Two exits reference unloaded 33807;
all reset references resolve. Source coverage remains distinct from played
active-world qualification.

Revision one adds seventeen independent outcomes and six support rows with
37 contacts/31 optional checks. Four different trophies, four separately
consumed heart instances and the five-kind mystic collection retain exact
native terms. Three fish markets group eleven barters; three recipes remain
services, including two guarded mixed fees. Gifts, distinct bass kinds, repeated
claws/crystals/eggs, opposing requests and the retiring hunter's competing
service have explicit guidance. The mystic's reward key opens the connector
tomb, not the separate transparent gateway. No personal hunt, allegiance,
membership, rebirth, study or permanent campaign completion is invented.

First-source/gift/episode, actual helper/rift/travel/actor outcomes and regional
presentation need qualification. Treant death wood, catch publication before
grant, two missing boundary edges, absent ordinary bracer stock, copied proof/
alias/clue wording, commented devil invasion and disabled descent have precise
balanced plans. Ship/class/fee settlement remains shared work with existing
refusals; no operational accounting activation, migration or merge occurred.

Verification covers full binding/classification/topic/source evidence, distinct
versus duplicate five-kind materials, repeated quantities, worn versus carried
proof, supplied potion without producer credit, different bass kinds, independent
opposing receipts, six service exclusions and persisted recovery. Catalog/index,
source/invariant/link checks, formatting, whitespace and maintained build
complement focused native/Python fixtures; live journeys remain unqualified.

Validation passed: focused production catalog/all 32 source indices, C++20
native journal/schema/projection and receipt recovery, authoritative formatting,
whitespace and the maintained server build. The invariant review verifies
672 local/source-line links, grouped world/boundary counts, exact tomb versus
gateway keys and unchanged native definitions/revision/fingerprint/registry
and all 54 earlier parsed maps.

The accounting base advanced to `100bee62f`. The subsequent feature-branch
integration resolves fifteen conflict files across migration/runtime
compatibility, flat-file/SQL quest state, special assignment and their fixtures.
Published alchemy/room migrations retain slots 54/55 and unchanged bytes; the
unpublished daily step now appends at 56 in all three supported histories.
Compiled history contracts and validators were regenerated together.

Integration retains bounded record stores, saved discoveries/encounters and
native receipt recovery while incorporating caller-owned deletion and authority
locking. SQL erasure reads/locks every bucket, retains unrelated legacy records
during conversion and never resolves the caller's transaction. Normal writes
refuse an existing transaction and roll back only their own failed writes.
Flat-file erasure uses the owner's existing journal, and a subsequent delta
reloads a replaced snapshot to prevent resurrection. The accounting-owned boot
path now handles a null service while accounting is inactive, including boots
with specials disabled. Published migration histories are never silently changed.

Focused erasure checks pass for legacy and current stores, cross-season names,
discoveries/encounters, unrelated PIDs, caller rollback, retry and allocation
cleanup; SQL exercised 1,626 allocation failures, including 47 after escaping.
The 24 migration-runner and ten boot-compatibility tests, three-history static
validator, schema/boot/serialization checks, 6,000-completion flat-file capacity
check and maintained SQL server build pass. Full database-engine migration and
played active-world journeys were not rerun during this integration.
The broader character/account deletion fixture now follows the production
catalog revision and passes corruption refusal/repair, all eighteen authority
journal interruption boundaries, publication and retry checks. Twenty-two source
indices were refreshed for assignment-line movement without changing bindings,
classifications, world data or the source-comprehensive area count. Changed-line
formatting is clean and whole-file checks pass for the feature diff; the general
pre-commit comparison also sees inherited formatting drift in 23 incoming-only
dependency files, which this integration does not rewrite.

The Tharnadia checkpoint completes the full nineteen-exchange/44-block source
review, 584 rooms/176 mobiles/206 objects/nineteen shops, 1,480 resets/661
families, all 31 literal assignments, computed roles and bounded foreign/shared
execution. Revision two ships eight independent stories, eight services and one
typed-food exclusion, with 26 contacts and 23 optional checks. Administrative
paper lookup now includes active limbo without adding discoverable ownership.
Initial orientation uses role descriptions; encountered contacts retain real
names/aliases. The dossier's medicine diagram shows both prepared and supplied
vial routes without inventing an all-stage prerequisite.

Validation passed: `test_zone_story_quest_production_catalog.py` with all 33
reproducible indices; `test_zone_story_quest_story.py` with C++20 warnings as
errors, exact toy/loan identities, distinct versus duplicate materials,
carried versus equipped alternatives, supplied medicine without producer
history, support exclusion and eight-outcome receipt recovery. The maintained
SQL server build, all 27 starter/town mappings, authoritative changed/staged
content formatting and whitespace checks pass. Source/link review verifies
3,893 local/source-line links across 72 documents, exact current generated
catalog agreement and unchanged native definitions/revision/fingerprint/registry
and the other 54 maps. The WSL staged auto-formatter exposed file-mode churn;
the tracked C++ mode remains 100644 and content checks pass. No native world,
server or migration content was changed in this checkpoint. Played source,
search, combat, payment, travel and recipient availability remain unqualified;
accounting activation and operational DB/server work were not performed.

Tharnadia checkpoint catalog: **55 journals, 1,750 achievement units, 1,516 potential daily
units and 2,227 projected rows; 33 of 220 roadmap areas are source-comprehensive,
with 187 remaining.** All 2,668 native definitions, revision two, fingerprint,
registry and fifty-four other maps remain unchanged in this checkpoint. Continue
with Mini Zones and the City of Torrhan. Active, ready accounting remains mandatory.


## Mini Zones completed source map — October 3, 2026

Priority 34 now has a [comprehensive dossier](zone-stories/MINI_ZONES.md),
revision-one sidecar and reproducible source index. The review covers all
21 native blocks/seven exchanges/eleven addressed families, 300 rooms in
257 exact prose/36 header/71 metadata/180 exit groups, 125 mobiles/124
objects/five shops, 548 resets/all 358 families and six literal assignments.
Complete local procedures, computed inn/pet roles, actual sword dispatch,
shared speech/shop/world-quest execution and bounded foreign entry/prototype
sources are explained. All local reset targets resolve; all mobiles are placed.

The journal has two stories, one request and four services, eighteen contacts
and sixteen optional checks. Dishwasher jar delivery, Worach's release and
Magik restoration are independent. Four exact sword pieces can be supplied
without producer history. Four armor recipes retain five crystals, matching
ghostly base and full fee; they remain guarded services. General discovery
does not assert that every mini-area was visited or the city was restored.

The full dossier separates working maze/door/weapon behavior from nine missing
boundary targets, dryad/navigator identity drift, ignored metadata, unimplemented
pet restoration and lore mismatches. Each has a bounded qualification or
builder repair decision. **No native zone, quest, reward, procedure, schema or
migration repair is shipped in this checkpoint.** Future native fixes follow
the separate commit and PR/news reporting convention above.

Validation passed: `python tests/async/test_zone_story_quest_production_catalog.py`
under Windows Python, including all 34 reproducible source indices; the initial
WSL run stopped on an environment memory-allocation error. The final native
`test_zone_story_quest_story.py` run passes C++20 warnings-as-errors, all 56
maps/native file loading, all 27 starter/town mappings, distinct/duplicate
strips, worn/carried hilt, supplied pieces without producer history,
read-only readiness, four/five crystals, service exclusion, three independent
receipts and foreign-credit exclusion. The maintained SQL `make -C src`
build, changed-line formatting and whitespace checks pass. Source/link review
checks 3,937 local/source-line links across 74 documents, exact generated
catalog agreement, unchanged native definitions/revision/fingerprint/registry
and the other 55 maps, and the original order of all 220 priorities.
Played source, search, travel, charm, targeted combat effects, paid services
and recipient availability remain unqualified. Accounting activation and
operational DB/server journeys are not performed by this source checkpoint.

Current catalog: **56 journals, 1,746 achievement units, 1,516 potential daily
units and 2,227 projected rows; 34 of 220 roadmap areas are source-comprehensive,
with 186 remaining.** Four guarded armor recipes become services, reducing
the achievement denominator without changing native definitions or rewards.
All 2,668 native definitions, revision two, fingerprint, registry and the
other fifty-five maps are preserved. Continue with the City of Torrhan.
Active, ready accounting remains mandatory.

## City of Torrhan completed source map — October 3, 2026

Read all 46 native blocks (23 Q, three QA, twenty M), all 26 exchanges and
eighteen addressed families; Nirrel's two ambient action blocks are not player
lessons. Reviewed 289 rooms/202 exact prose groups/36 headers/39 metadata
groups/224 exit families, 152 mobiles, 125 objects, six shops and 447 resets
across 324 source families. Both room assignments, complete local searches,
default/table teaching, automatic switch and bounded shared execution are
recorded in the [dossier](zone-stories/CITY_OF_TORRHAN.md). All positive
boundary targets resolve; the administrative dispersal entrance is distinct.

Revision one adds eight independent outcomes, fourteen support exchanges,
four refusals, 23 contacts/all addressed topics and 28 optional checks.
Aineila's ring and Marthona's scales have exact sources; boy/halfling consume
the same feather, and shell services compete for one material. Full yellow
to owl and half-empty to king are separate terminals, without a required
Zrilxa kill or invented royal cure/transformation. Initial cloak crafting
is one outcome; eight same-named but different-property reworkings are support.
Raw guard sword → Thurdorf → Oblivion to Torrok is optional producer guidance
for supplied Oblivion. Worn/reward crowns remain distinct kinds.

The mighty throne is a push-triggered switch whose current target is an
unblocked hallway exit. The actual throne-room route loads open because
the loader masks raw state four to zero, and the hallway has no south return.
Builder must settle intended target/reset/return before a native repair;
this finding does not prove the whole royal story inaccessible. Torrok's
assistant/shop reference absent stock 6087, without invalidating other real
stock or his sword delivery. Laboratory random-output wording, royal prose,
probabilistic one-choice plaque and minor text artifacts have balanced repair
options. **No native zone or quest repair ships in this checkpoint.** Every
selected implemented repair requires a clear fix commit and PR/news before/after
statement, with validation and remaining limits.

Extend qualification with exact acquisition cause and item/output lineage,
recipient incarnation/retirement, permitted same-name property presentation,
accepted dialogue/access/travel, actual actor state and durable branch/attempt
events. One-cap material declarations, follower chains and reset mode one are
leads, not proof of personal recovery or daily availability. Optional producer
history never replaces consumed materials or creates an all-stage campaign.

Native regression verifies exact full/half potion and cloak readiness,
supplied king/Torrok finales without predecessor history, no read mutation,
all services/refusals excluded from achievement credit, independent eight-outcome
receipt recovery and no invented Surface/Mini Zones credit. Production regression
checks all bindings/topics, exact prototype/source parents, boundary and throne
configuration and missing stock evidence. Active native journeys, source generation,
actor state, access and payments remain unqualified.

Current catalog: **57 journals, 1,728 achievement units, 1,502 potential daily
units and 2,223 projected rows; 35 of 220 roadmap areas are source-comprehensive,
with 185 remaining.** Four refusals are removed from the projected rows; fourteen
services remove additional achievement/daily credit. All 2,668 native definitions,
revision-two fingerprint/registry and other fifty-six maps are preserved.
Continue with Golden Hall of the Crown. Active, ready accounting remains mandatory.

## Golden Hall of the Crown completed source map — October 3, 2026

Read all 42 native blocks (sixteen Q, 26 M), sixteen exchanges and seventeen
addressed families; nine ambient action blocks are not player lessons.
Reviewed 300 rooms/100 exact prose groups/25 headers/thirty metadata groups/
250 exit families, 91 mobiles, 106 objects, two shops and 534 resets across
351 source families. All local mobiles have M/F declarations. Four literal
assignments, complete local searches and automatic switch/teleport/teaching
plus bounded shared execution are recorded in the
[dossier](zone-stories/GOLDEN_HALL_OF_THE_CROWN.md). All positive boundary,
key and reset targets resolve; administrative access remains distinct.

Revision one adds nine independent outcomes, seven support services, 27
contacts/all addressed topics and 34 optional checks. The chalice/inn clues,
bed, corpse note, optional royal briefing, captive key routes and trainer's
long amulet chain explain progression. The final Prince receipt requires
the exact head, balanced sword and hair pin together, accepting supplied
proofs without earlier receipts. Three rescues record retirement; two return
new same-kind keys and award experience. Both retain achievement credit
and existing same-item daily exclusions. Seven outcomes are potential dailies.
Seven support exchanges earn no new achievement; three mixed-fee services
remain guarded under active accounting. Supplied later materials still work.

Preplaced corpse contents and head/totem reset items do not prove witnessed
death or personal combat. The captain's writing is prose, not a new note
grant. Local captive animals carry the delegate's exact feathers/hide despite
distant homeland dialogue. The bed/return-boulder/chest switches have blocked
targets; tunnel boulders target an already-open east exit and do nothing.
Pool/platform returns are directionally asymmetric but resolve, and standing
on unlimited-charge inscriptions links the platform/spy chamber. Tield alone
has the local teacher flag; other teacher titles require an intentional role
decision. No entire quest is declared broken based only on these differences.

Reliance calls distinguish fresh creation from relocating an existing linked
pegasus. Only fresh creation updates its item timer; link-type/owner, summon
and recovery events require qualification before a milestone. Expand accepted
source-versus-gift, container/incarnation, output/returned-key lineage, access/
travel, actor-retirement/arrival and optional full-campaign event plans.
Keep accounting gates and frozen recovery obligations distinct. Builder selects
boulder, teaching-role, note/source, directional and wording repairs deliberately.
**No native zone or quest repair ships in this checkpoint.** Every selected
implemented repair needs a clearly named fix commit where practical and a PR
before/after, validation and player-facing news sentence.

Native regression verifies exact note/totem/sword/key readiness, supplied finale
without predecessor history, no read mutation, service exclusion, independent
nine-outcome receipt recovery and both rescue daily exclusions. Production
regression checks all bindings/topics, exact source/container parents, ordinary
boundaries, switches, portals and teacher flags. These fixtures do not qualify
live acquisition, payment, access, pet creation or actual rescued-actor travel.

Current catalog: **58 journals, 1,721 achievement units, 1,498 potential daily
units and 2,223 projected rows; 36 of 220 roadmap areas are source-comprehensive,
with 184 remaining.** All 2,668 native definitions, revision-two fingerprint/
registry and other fifty-seven maps are preserved. Continue with Ashrumite
Village. Active, ready accounting remains mandatory.

## Ashrumite Village completed source map — October 3, 2026

Read all 25 native blocks: twelve Q and thirteen addressed M families with
four speakers; none ambient. Reviewed 153 rooms/111 exact prose groups/34
headers/36 metadata groups/177 exit families, 53 mobiles, 65 objects, twelve
shops and 275 resets/181 source families. All chance fields are 100 and
reserved fields zero. Fifteen literal assignments plus automatic/shared
quest, teaching, shop, justice, pet, mining and wonder behavior are recorded
in the [dossier](zone-stories/ASHRUMITE_VILLAGE.md). Four ordinary boundaries
resolve and have foreign returns; the fifth points to an inactive old-world
room. Missing prototypes include disc 4372, rewards 66066/66067 and unrelated
thief-teacher inventory item 6089. A different active disc prototype is not a
valid substitute or proof of its source. Class headers are not item grants.

Revision two upgrades the existing basic map from eight requests/four excluded
contracts to twelve explicit support services, sixteen contacts and 21 optional
checks. The silverworker's raw-gem recipes, three matching gem cuts, two
cut-gem-to-necklace exchanges, two unavailable jewelry rewards, six-material
gold exchange, gold/disc-to-pyrite commission and paid forest rumor are all
explained. The intended ore/bar/five-gem/disc/rainbow progression is documented
separately. The zone projects no authored quest achievements or dailies;
all paid native exchanges remain guarded under active accounting.

Static extraction exposes exact terms, producers and missing references but
does not choose intended balance, valid disc/source or campaign policy.
Expand mixed-fee settlement, frozen-term rebinding/recovery, same-name variant
presentation, accepted mining/ground/shop/random-source lineage, paid lore and
optional all-stage crafting qualification. Current inventory and supplied
items remain distinct from personal acquisition or earlier producer receipts.
Five matching gems need five real copies; ordinary/magical necklaces and
real gold/pyrite remain separate kinds despite identical visible names.

Five guildguard prototypes are unplaced locally, four assigned gates dormant,
while the cleric teacher has an active gate role. Bakery/shaman-store stock
lacks local shop records. The bank's money-changer now redirects to the Royal
Bank; wall-map/sign/price wording is incomplete or inconsistent. Pet claim
restoration is commented out behind the existing accounting guard. A stable
storage room has no exits by design; the historical wagon setup is disabled.
These distinctions avoid claiming that all local commerce, entry or questing
is broken. Builder-selected recipe/source/reward/price, stale boundary,
equipment/role and pet/prose repairs remain explicit, fairly scoped proposals.
**No native zone or quest repair ships in this checkpoint.** Report each
implemented repair in a separate fix commit where practical and PR/news notes
with concrete player trigger, before/after, validation and limitations.

Native regression checks encounter visibility, exact same-name variants,
five-copy readiness, optional history, eleven generic payment warnings and
the authored paid-rumor guard, read purity
and service-only receipt recovery without local/foreign achievement credit.
Production regression checks all bindings/topics, source/key parents, absent
prototypes, boundary/guild/store evidence and the reproducible index. These
checks do not qualify actual paid crafting, live stock, mining, travel or pets.

Current catalog: **58 journals, 1,713 achievement units, 1,498 potential daily
units and 2,227 projected rows; 37 of 220 roadmap areas are source-comprehensive,
with 183 remaining.** All 2,668 native definitions, revision-two fingerprint/
registry and other fifty-seven maps remain unchanged. Continue with The Hall
of the Ancients (`hall`). Active, ready accounting remains mandatory.

## The Hall of the Ancients completed source map — October 3, 2026

Reviewed all 25 native blocks: eleven exchanges/fourteen addressed M families,
with six addressed speakers and none ambient. Complete source review covers
247 rooms/116 exact prose groups/48 headers/56 metadata groups/182 exit
families, 55 mobiles, 53 objects, one shop and 395 resets/153 families. Five
chance declarations differ from 100; every reserved reset argument is zero.
Twelve literal procedures, all Hall special implementations and relevant
shared command/death/periodic/offering/reset/equipment behavior are explained
in the [dossier](zone-stories/THE_HALL_OF_THE_ANCIENTS.md). The ordinary
Underdark entrance/return is distinct from administrative city dispersal;
all positive boundary/reset references resolve in the active inventory.

Revision one has five independent outcomes, four preparation/access services,
two elder exclusions, 27 contacts/all fourteen addressed topic families and
23 optional checks. The exact recipes retain sixteen belt pieces and ten
armor pieces, including only one genius potion despite the two-sample prose.
The two visible tiny keys have different identities/targets; the foreign
dagger's accepted receipt remains Hall-owned. Supplied materials skip optional
producer histories without inventing kills, first recovery or a saved son.

**Shipped native repair:** separate fix commit
[7297b964e](https://github.com/Community-Duris/Duris/commit/7297b964ed3996c33a90dae4172cd0875addf174)
repairs Sin's Freedom of Movement target and room substitution. The compiled
actual procedure passes periodic null actors, unrelated actors, protection
consumption, unprotected paralysis, paladin/staff immunity, absent opponent and
non-trigger roll. Maintained server build and changed-line formatting pass.
News: “The Shadow of Sin now correctly checks its opponent's Freedom of
Movement, fixing a combat crash and misleading target text.” This is separate
from journal authoring and every pending repair below.

**Pending native repair/qualification:** the consuming elder refusal runs
before the identical ore reward because Q loading prepends contracts. Belt
size exceeds the fourteen-root durable limit; several repeated ingredients
have cap one, while normal O resets cannot load the chance-ten gear. Builder
must select balanced recipe/supply/rarity and elder eligibility policies.
Armor dialogue omits ebony shards and asks for two potions instead of one.
Collector death creates guardian/potion outside an atomic source transaction;
its prototype already carries the death flag. Death's own flag is armed later
and needs early-kill qualification. The cathedral coin procedure dispatches
before pickup and can change doors on unrelated/failed GET. Keys, traps,
portals, holding sources, retiring recipients and family/shadow/dragon closure
need accepted event and episode semantics. No recipe, world-data or cathedral
repair ships in this checkpoint; report each implemented fix separately.

Focused journal/source fixtures cover exact quantities/kinds, worn versus
carried foreign proof, one potion, optional producer histories, guarded fees,
oversized guidance, read-only readiness and historical service/exclusion/
independent-outcome recovery. These are native in-memory and source checks,
not live acquisition, combat, crafting or travel certification. All 2,668
native definitions, content fingerprint, revision and zone registry remain
unchanged. The other 58 journals remain exact; Hall alone replaces fallback.
The complete roadmap order remains 220 areas: first 38 comprehensive, 182
pending. Continue with Sarmiz'Duul.

Current catalog: **59 journals, 1,707 achievement units, 1,496 potential daily
units and 2,225 projected rows; 38 of 220 roadmap areas are source-comprehensive,
with 182 remaining.** Three paid Hall services stay guarded: two display the
generic mixed-offering warning, while the retiring snapped-key recipe has the
existing story-only classification and explicit authored payment guidance.

## Sarmiz'Duul completed source map — October 3, 2026

The [dossier](zone-stories/SARMIZ_DUUL.md) explains all 21 native blocks:
seven Q, one QA and thirteen raw addressed M families across nine speakers.
Complete world review covers 583 rooms/59 exact prose groups/sixteen headers/
sixteen metadata groups/322 exit families, 57 mobiles, 61 objects, four shops
and 535 resets/199 families. All five literal assignments, cleared procedures,
teacher/tradeskill leads and relevant shared dispatch/source/reset execution
are reviewed. Positive boundaries resolve; four mount-stock object resets
remain unresolved, distinct from quest material sources.

Revision one has eight independent outcomes, 24 contacts and nineteen optional
material/history checks. The courtship and revenge/relic chains feed separate
receipts, while royal three-part and conspiracy four-part recipes preserve
exact supplied materials and foreign producer ownership. Two blue outputs are
different kinds, and the sparkling one is a generic quest token. The diplomat's
plain `sarmiz` topic is advertised despite a shared extractor skipping its
apostrophe-containing family. No personal kills, extra assassinations, learned
topics, poison effects, royal succession or completed romance/war are inferred.

Custom moonstone review traces two boot fragments, pirate-chest core, periodic
root-material assembly and alternative ring or paid automaton crew. **Pirate
ship/core setup is deliberately disabled with accounting active.** Do not
remove this guard to make a source available; durable treasure/key ownership
must precede fresh issuance. The same finished stone also feeds a foreign
Winterhaven commission. Optional assembly history cannot replace spent custody.

**Actual native repairs in this checkpoint: none.** Erzul's missing addressed
target validation, partial boot seed ordering, consumed-before-allocation
assembly/ring rewards, compound crew settlement, unfinished multi-core branch,
four missing mount-stock references and potion prose/type discrepancies have
specific repair/qualification proposals. Builder decisions precede balance,
effects or mutually exclusive campaign changes. Every implemented repair must
have a distinct fix commit where practical and prominent PR/news trigger,
before/after, evidence and limitations; these proposals are not news claims.

Focused existing source/native fixtures cover exact bindings/topics/materials,
encounter visibility, supplied three/four-item recipes, worn/different proof,
read-only readiness, optional producers, independent recovery and the guarded
custom source. These checks are not live acquisition, assembly, ship, combat,
reset or travel certification. All 2,668 native definitions, revision-two
fingerprint, registry and other 59 maps remain unchanged. Priority order remains
220 areas: first 39 comprehensive, 181 pending. Continue with Duke Delwyn.

## Duke Delwyn completed source map — October 3, 2026

The [dossier](zone-stories/DUKE_DELWYN.md) explains all 53 native blocks:
eleven Q, nine addressed M and 33 ambient M families. Complete world review
covers 207 rooms/170 exact prose groups/seven headers/nine metadata groups/
145 exit families, 96 mobiles, 41 objects and 278 resets/170 source families.
There is no local shop file or literal special assignment; automatic teacher,
shared trap/falling, native dispatch and source/reset execution are reviewed.
The surface highway 549230 and outer gate 82805 are reciprocal and resolve.

Revision one has six independent outcomes, five paid services, nineteen
contacts and seventeen optional material/history checks. Exact bell → cog →
blade, four-stage paid textiles → banner → slippers, ivory knight → boots,
assessment → note and note + braid → signet recipes preserve supplied proof.
The local textile fees total 60000 copper and remain guarded with accounting
active. A supplied exact banner fits the separate item-only outcome without
personal producer receipts. First recovery, source kills, chess lessons,
spy allegiance, scheduled invasion, clock/mill state and flag hanging need
separate accepted evidence; no such endpoint is invented.

The military assessment is inside a closed ebony desk with a one-charge
level-thirty opening acid trap. The banner recipient is atop a midair flagpole:
the ladder and two midair rooms have real fall chances/downward routes. These
expand qualification for container ancestry, post-interaction actor survival,
successful access and safe return. Rendering custody cannot certify those
journeys, pay fees or distinguish first recovery from gifts.

**Actual native repairs in this checkpoint: none.** The note's wedding versus
the Duke's birthday reply, unbound advertised shop/lodging services and optional
peaceful source handovers have bounded builder repair proposals. Some trades
may be intentional scenery; recipes and balance stay intact until decisions
and focused journeys support actual changes. Later repairs require distinct
fix commits where practical and prominent PR/news trigger, before/after,
validation and limitations. Pending findings are not shipped news claims.

Focused existing source/native fixtures cover exact bindings/topics/materials,
encounter visibility, supplied evidence without earlier history, wrong/worn
proof, read-only views, paid-service credit exclusions and independent recovery.
They do not certify live payment, desk/fall survival, acquisition, resets or
travel. All 2668 native definitions, revision-two fingerprint, registry and
other sixty maps remain unchanged. Catalog: 61 maps, 1702 achievements,
1496 potential dailies and 2225 rows. Original 220-area order remains intact:
first forty source-comprehensive, 180 pending. Continue with Home of the Divine.

## Home of the Divine completed source map — October 3, 2026

The [dossier](zone-stories/HOME_OF_THE_DIVINE.md) explains all 52 native
blocks: thirty Q/two QA/twenty M, including one empty default. Complete world
review covers 122 rooms/89 exact prose groups/nineteen headers/24 metadata
groups/113 exit families, 62 mobiles, 83 objects, one seven-stock shop and
240 resets/156 families. No literal local special is assigned; shared native
reward, shop/forge, epic teacher, teleport, trap/falling, wandering and source
execution are reviewed. Seven item teleport declarations and the reciprocal
surface peak boundary resolve; the prison loom uses touch, not enter.

Revision one has twenty independent outcomes, twelve preparation/access
services, 27 contacts and 48 optional checks. Four elemental tokens form one
siren request, seven hearts remain independent bounties, and Wicks's nine
exact curiosities pay cash without becoming paid services. Prince/Raith token
and foreign Pure-Dark exchanges do not enforce allegiance or require earlier
personal routes. Four item-only crafting, six mixed crafting and two key
services remain outside achievement/daily credit. All 32 native identities
remain intact, including same-named eggs, flute/weapon variants and typed XP.

**Concrete missing-reward blocker:** the Relazier heart bounty promises
object 31341, absent from all object files. Current durable admission validates
reward kinds, not item loadability; consumption and later failed reward
instantiation remain possible. A journal warning is not a native guard.
Builders must choose the intended reward, and reward-reference preflight
before consuming/new credit must preserve existing frozen obligations. No
replacement or fail-closed fix is claimed in this checkpoint.

**Intentional accounting blockers:** generic scale 392 exists but its automatic
dragon-death issuance is disabled with accounting active. Six mixed fees and
ordinary forge/epic teaching have separate settlement requirements. Retain
guards until recoverable source, item/wallet/XP/skill/output and recipient
publication are qualified. Starting the final weapon from Riser requires two
maces, two shards and 300000 copper; global cap-one and mode-one empty-zone
reset policies do not promise fresh same-visit supply.

Prison access needs the real touch loom and cloudy key; the palace uses its
different wispy key. Closed statue/nest/anvil contents, the flaming claw's
get/put fire trap and local/foreign fall chances expand accepted ancestry,
actor survival and confirmed travel qualification. Rare holding NPCs can
wander through one-way routes or into dead ends; source layout is neither
categorically unreachable nor guaranteed usable. Relazier retirement removes
carried heart; prince/Raith consume competing tokens. Explicit attempts,
branches and source/recipient generations must precede personal campaign,
escape, royal-drain, kill or full-stage credit.

**Actual native repairs shipped: none.** Missing bounty/admission, wrong heart
extra aliases, empty success strings and rare-room usability have balanced
proposals. Reln's jade fee is consistent, and shared procedures are bound.
Only actual tested repairs belong in player news, with distinct fix commits
where practical, zone/trigger/before-after/proof/limits and a news sentence.
The existing separate Hall Shadow of Sin fix remains clearly identifiable.

Existing source/native fixtures verify exact recipes/topics, source/gate and
same-name identities, encounter visibility, four-token preparation, wrong/worn
inputs, supplied proof without producer history, spent material despite
history, twelve service exclusions and twenty independent frozen receipts
through cold recovery. They do not execute live ownership, settlement, missing
reward, first source, trap/fall survival, travel or reset episodes. All 2668
native definitions, revision-two fingerprint, zone registry and other 61 maps
remain unchanged. Catalog: 62 maps, 1690 achievements, 1490 potential dailies,
2225 rows. Original 220-area order remains intact: first 41 comprehensive,
179 pending. Continue with The Halfcut Hills.

## The Halfcut Hills completed source map — October 3, 2026

The [dossier](zone-stories/THE_HALFCUT_HILLS.md) explains all twenty-eight
native blocks: twelve Q, one QA and fifteen addressed M families. Complete
source review covers 470 rooms/208 prose groups/38 headers/48 metadata groups/
242 exit families, 83 mobiles, sixty objects, one six-stock shop and all
413 resets/177 families. The sole literal special is the crossbow ambusher;
shared switches, teleports, inn, shop, epic teaching, doors, falling, wandering,
native acceptance and reward recovery are reviewed. The surface boundary
is reciprocal, and switch/teleport destinations exist. Inactive similarly
named area files are not substituted for active Halfcut source.

Revision one has thirteen independent outcomes, nineteen contacts and
twenty-four optional checks. Three jar-to-badge deliveries, the exact badge
bundle, Bartis's final jar and sentry note return explain the longer rescue
route without enforcing personal rescue history. The bundle should precede
Bartis's retirement in one episode. Four P jar declarations are sufficient
declared quantity, with live ancestry/stock qualification open. Type-13 jars
are quest props; disappearing miners do not record actual home arrival.
Same-named old miners have different badges and E2000/E20000 rewards.

The wounded dwarf's green potion is the wagon content, not the supplier's
flaming green stock. Remi's raid-leader scalp and the leader's six distinct
proofs remain independent. Four faction requests compete for the larger
bundle's material; Bartis retirement can remove its carried scalp source.
One accepted bundle does not prove six personal kills or mine takeover.
Producer history cannot replace spent material or restore a recipient.

**Shipped actual repair:** the bound crossbow now registers without setup
damage, fires three periodic lanes/four bolts at players, and stops interrupted
volleys by re-resolving runtime identities. The focused actual-procedure test
passes; the original fails setup. A second separate fix changes the suppressed
struck-player warning to the correct audience, with a preceding-version failure
and fixed delivery-count regression. Maintained build and changed/staged formatting
pass. See the distinct fix commit/news entry above. Unbound defender/blowgun
procedures and all native world/Q terms are unchanged. No live balance,
reset availability or complete combat/accounting journey is claimed.

**Pending actual quest repair:** the drow's second reward is literally
`R I 25000`, absent from all object files. Do not guess currency, experience
or object substitution. Builder selects intended terms; add reward loadability
preflight before offering consumption/new credit, preserving frozen obligations.
Journal readiness/static candidacy cannot certify payout, and the valid first
potion reward does not resolve the missing second item. No reward repair or
admission guard ships in this checkpoint.

Grab rope and grab the two different same-name black stones; enter the well;
say windship at the crypt slab; pull the office lever, then open the bedroom
door. LARGE key opens a different cavern gate. Closed container ancestry,
source generations, wandering/numbered object selection, door/trap/fall and
confirmed destination/survival need accepted evidence. Inn and A'den's Improved
Listen teaching really are bound shared roles. Minor aliases/spelling and
unbound procedure placement remain balanced proposals. Actual future fixes
need separate identifiable commits and prominent player-news evidence.

Existing source/native fixtures verify thirteen exact identities/all topics,
materials and actual access/reset constants, missing reward, supplied badge/
note preparation, wrong/worn proof, read-only views, spent material despite
producer history, independent receipts and cold recovery. Synthetic receipt
recovery does not instantiate missing reward or prove a player's actual journey.
All 2668 definitions, revision-two fingerprint, registry and other 62 maps
remain unchanged. Catalog: 63 maps/1690 achievements/1490 potential dailies/
2225 rows. Original 220-area order remains intact: first 42 comprehensive,
178 pending. Continue with The Scorched Valley.

## The Scorched Valley completed source map — October 3, 2026

The [dossier](zone-stories/THE_SCORCHED_VALLEY.md) explains all twenty native
blocks: nine Q and eleven addressed M. Complete review covers 132 rooms/
seventy-eight prose/seventeen headers/seventeen metadata/eighty-three exit
families, sixty mobiles, fifty-five objects and 211 resets/118 families. No
local shop exists. Reviewed all three literal assignments, shared combat and
periodic dispatch, artifact effect, keys/containers, item rifts, F follower
ancestry, wandering, foreign producers and native settlement/recovery.

Revision one has nine independent outcomes, twenty-two contacts and twenty-two
optional checks. Survivor heirloom, captive sack, three exact heads, bodyguard
essence, collector banners/hide and charm, council component, blood and four
rings retain exact native identities. All recipients stay after acceptance.
One reward object is named two bubbles. Two same-named commanders carry
different head/magic proof; the pit hide is ground proof, not a skinning event.
Five keys explain the temple blood route; the final chest key is carried by
the advisor who receives three other requests. Killing him can remove those
interactions until replacement. Supplied keys/blood/rings fit without enforced
personal source history; earlier receipts cannot restore NPCs or spent proof.

Bodyguard essence leads from a holding source into Mount Banishment. Council
advisor and seeker lead into Torg and Fields Between. Grog and Zuzon provide
foreign heads. The accepted contracts remain Scorched Valley outcomes even
when a recipient is encountered elsewhere; qualify actual visibility, arrival,
recovery and completion instead of deriving ownership from room location. Physical-area discovery is
required before encounters, and normal arrival supplies it. The immediate
runtime hint still points to the physical journal; plan an owning-journal
referral for discovered owners and honest guidance for unknown owners,
without auto-discovering remote areas or changing contract credit.

**Pending actual repairs, none shipped:** Yeenoghu's bound procedure rejects
the server's -102 combat command and declines periodic scheduling. Ordinary
combat/gear exist, but the authored heal/breath/whirlwind/bite lack a normal
dispatch path. Choose one intended cadence and qualify actor/target callback
survival, outer room traversal, each branch and live balance/accounting before
restoration. The commander-head clue exceeds the one exact accepted source,
and council-advisor runs-off text conflicts with disappear=0. Prefer truthful
text or deliberate builder-selected alternatives/lifecycle, with separate
fix commits and prominent PR/news trigger/before-after/proof entries.

Sack-captive, resummoning, society membership and a council curse narrate state
without actual entity/effect transactions. The Rod of Seven Parts is one full
weapon object with no reviewed active reset/producer/assembly/finale terminal.
Keep that as lore until intended sources and endpoints are designed. The worn
fog wisp has actual say-invisible cooldown behavior, and the two enter-rifts
have reciprocal destinations; neither is a delivery receipt. Holding exits
can strand mobiles, and rare prose is not a rare reset percentage. Live
distribution and actual source generations need qualification before repair.

Advisor forty-six native aliases exceed the thirty-two-topic display bound.
Its contact selects thirty-one valid aliases covering all five families; the
index keeps every alias. No parser/schema expansion is needed to show useful
clues. Focused fixtures cover exact terms/families/source parents/access values,
supplied four-color proof, optional history, spent/wrong/worn proof, read-only
rendering, independent receipts and cold recovery. Synthetic receipts do not
certify an actual chest/foreign/captive/boss or full-accounting journey.

All 2668 definitions, revision-two fingerprint, registry and other sixty-three
maps remain unchanged. Catalog: 64 maps/1690 achievements/1490 potential
dailies/2225 rows. Original 220-area order stays intact: first forty-three
source-comprehensive, 177 pending. Continue with Court of the Muse. Journal
guidance and pending findings must not be reported as restored native gameplay.

## Court of the Muse completed source map — October 3, 2026

The [dossier](zone-stories/COURT_OF_THE_MUSE.md) explains all fifteen native
blocks: seven QA/two Q and five MA/one M. A means room echo, not alignment.
Complete review covers ninety-nine rooms/eighty prose/twenty-eight headers/
forty-one metadata/eighty exit families, thirty-six mobiles, forty-three
objects, one actual apple shop and 209 resets/ninety-two families. There are
no literal local special assignments; automatic class-matched teachers and
shared doors, traps, item teleports, fall/current, follower and quest paths
were traced. All local declared targets and reciprocal surface boundary exist.

Revision one has nine independent outcomes, twenty-five contacts and sixteen
optional checks. Snowflake/petals/dew/leaf give four exact tokens and native XP.
Their distinct bundle gives a separate admission key. Four producer histories
remain optional: supplied tokens fit, and receipts cannot restore spent proof
or certify seasonal effects/audience. The fisherman requires twelve scales,
with fifteen declared fish-and-scale sources, and retires. Two friend instances
share one obsidian outcome; eight initial satyr/sprite sources carry essence,
not every matching mob. Larissa's exact wand lies on a falling ice floor.

The book and dew are real key references accepted by generic lookup despite
locket-shaped cave clues. Use dew at its optional gate before offering that
copy. Shared loading masks door type; D resets set secret/closed/locked state.
Autumn leaf has secret-stump or enter-stump access. Jump waterfall, enter arch,
seasonal portal and branch portal have distinct actual commands/destinations.
Forward admission key has a 100 percent break declaration, subject to durable
destruction acceptance. Reciprocal opening and court down permit ordinary
return; restored reverse lock/key zero needs a separately played recovery
episode before a return repair is selected. Currents/falls are actual hazards.

**Pending repairs, none shipped:** align cave clues with working book/dew
keys or design intended lockets; align pouch/whole-flower text; select intended
trap data. Spring/Summer trap damage 9, Autumn token/mask 12 and diamond stud
16 have no case in the shared 0–8 handler. Charged GET can consume a charge
and reject pickup without a matching effect/message. Winter cold damage is
real. Qualify single/bulk pickup, retry, persisted charge, callback survival
and actual accounting before repairing content. Keep separate clear fix commits
and prominent PR/news before-after entries when an actual repair ships.

Narrated seasonal changes, skipping/fishing rituals, soul extraction and a
Muse audience have no accepted terminal. Elk heart is carried loot, with an
elf/elk alias discrepancy, without a reviewed consumer. Three teacher flags
give shared level/runestone guidance rather than local epic contracts. Potion
and wand use are separate effects. These roles can remain useful lore until
builders select explicit supported endpoints and campaign/attempt policy.

Focused source/native fixtures cover exact terms, all dialogue, source parent
ancestry, working access/trap values, supplied four-season tokens, eleven versus
twelve scales, wrong/worn/spent proof, optional history, read-only readiness,
independent receipts and cold recovery. Synthetic completion is not actual
fishing, gate opening, safe trap recovery, ritual or a full-accounting journey.
All 2668 native definitions, revision-two fingerprint, registry and other
sixty-four journals stay unchanged. Catalog: 65 maps/1690 achievements/1490
potential dailies/2225 rows. Original order: forty-four source-comprehensive,
176 pending. Continue with Valley of the Snow Ogres (`snogres`).

## Valley of the Snow Ogres completed source map — October 3, 2026

The [dossier](zone-stories/VALLEY_OF_THE_SNOW_OGRES.md) explains all fifteen
blocks/eight Q/one QA/six M, nine exchanges, one hundred rooms/seventy-one
prose groups/twenty-nine headers/thirty metadata/sixty-eight exit families,
forty-four mobs, thirty-eight objects and two hundred resets/eighty families.
Twelve literal assignments resolve to eleven bindings because the golem's
specific procedure replaces block_dir; an automatic switch adds a shared binding.
Complete local procedures, relevant dispatch, generation/control/quest paths
and bounded foreign source/boundary evidence were reviewed. No local shop.

Revision one projects seven independent outcomes, one support armor service
and one excluded hide refusal, sixteen contacts/all six addressed families and
fifteen optional checks. Chieftain hourglass, rare astereater eye and enslaver
tentacle give three distinct shards for a separate pyramid delivery. Optional
producer histories do not reject supplied matching shards or replace spent
proof. Independent hydra head, stalk and scale retain exact rewards. Narrated
kills, ritual and lich transformation have no extra accepted terminal.

**Pending repairs, none shipped:** choose the missing active sundew-stalk source
and sufficient hide supply. Four remorhaz have three hide declarations/cap
three for six-copy armor. Inactive brass-old-1 is not live stock; same-kind
lich refusal does not multiply supply. Source generation/renewal and historical
copies need qualification before claiming absolute impossibility. Leppts has
an active Surface dispersal reset and wanders; Snow ownership stays separate
from physical discovery. Mixed payment for two exact cold-iron weapons/six
hides/2,500 platinum remains guarded and gives no achievement/daily credit.

Push rock clears the blocked grotto wall, leaving secret/closed and pit-fiend
conditions independent. Two hundred-percent falling rooms have an alternate
lake/lava-tube route. World F20 without a down exit is cleared by the loader.
The lich's assigned shout overrides automatic teaching, while actual hunt
uses opponent/runtime IDs. Axe/whip melee 1000 callbacks are connected.

Remorhaz retaliation damages its owner rather than attacker. Golem counts
small helpers but creates large ones, passes a null periodic target to initial
combat, and uses a virtual room number for low-health replacement. Cadaver
repop lacks missing-template/null-spawn checks. Berserker rejects the normal
combat command and needs target-selection/callback repairs before reactivation.
Skull leggings need durable custody across automatic equipment replacement;
axe follow-up needs effect/survival qualification. Separate deliberate fixes,
actual-procedure tests and accepted actor/effect/source/reunion endpoints are
planned; no unsafe dormant path is enabled as journal authoring.

Production/native fixtures cover exact classification, dialogue, reset ancestry,
rare chance and source gaps; supplied distinct shards/wrong/worn/spent proof,
five-versus-six hides, read-only readiness, foreign owner encounter and independent
receipt recovery. A synthetic service receipt is not a successful guarded
coin transaction. All 2668 definitions, revision-two fingerprint, registry
and other sixty-five maps are unchanged. Catalog: 66 maps/1688 achievements/
1490 potential dailies/2224 rows. Two fallback achievement units are removed
by service/refusal classification. Original order: forty-five source-comprehensive,
175 pending. Continue with Dawndale. Active accounting remains mandatory.

## The Mountain Valley of Dawndale completed source map — October 3, 2026

The [dossier](zone-stories/THE_MOUNTAIN_VALLEY_OF_DAWNDALE.md) explains all
seventeen blocks/eleven Q/two QA/four M, thirteen exchanges, 150 rooms/eighty-three
prose groups/twelve headers/twelve metadata/135 exit families, sixty-nine mobs,
sixty-six objects and 321 resets/175 families. No local shop, literal procedure
assignment or teacher flag. Shared switch, teleport, quest/key/container/trap,
packed-message and reset paths were reviewed, with bounded foreign material,
surface, Aravne death output, Emition forge and administrative book evidence.

Revision one projects nine independent outcomes, three support services and
one excluded fossil referral, twenty-seven contacts/all four addressed families
and thirty-one optional checks. Portrait/stones/dust/device explains preparation;
the key/device finale accepts supplied materials without four earlier histories.
Both captains consume independent seven-copy/six-kind bundles. Whetstone returns
a distinct same-named flute after Emition's foreign forge, with optional history.
Treasure differs from coin piles/sack; portrait/bow/larvae have exact producers.
Actual moonstone 34464 exists; local 77559 is an unused placeholder, not a missing
reward. Retiring recipients remain separate from actual movement/settlement.

**Pending repairs, none shipped:** choose a truthful fossil follow-up and fix
Ender's `_vict_msg` mismatch separately with actual audience proof. Qualify the
guarded 250-platinum coin-only key purchase and 25-platinum mixed lens service;
ordinary admission retains the persistent city key, while office/alcove keys can break. Three sunlight vials do not establish
four-copy captain supply. Scholar book/global cap one competes with an active
administrative-library copy; forced boot resets bypass cap, so qualify ordinary
renewal, retained supply and recovery before changing isolation or stock.
Aravne sand is genuinely produced on death; accepted generation/recovery and
post-placement callback continuation need qualification rather than a claim of
missing source. Rock switches clear one direction of an already open secret
passage; search/crossing/return remain separate. Cliff lore has no fall declaration.

Expand exact loose container recovery and liquid content/volume semantics,
atomic purchase/payment, learned reading/ritual and scoped world/party effects.
Refugee BOOM/new camp/follower relocation, larvae tunnel, captain curse and lens
installation need builder-owned endpoints and recovery before credit. Great-lens
lore does not establish a broken telescope. Defense/captive/bounty lore has no
reviewed extra terminal. Do not conflate delivery text with actual player effects.

Production/native fixtures cover exact source/classification, competing bundles,
one-versus-two vials, supplied final proof/optional histories, spent/worn material,
foreign forge ownership, same-named flute kinds, read-only checks and independent
cold recovery. Synthetic support receipts do not execute guarded coin settlement.
All 2668 native definitions, revision-two fingerprint, registry and other sixty-six
maps are unchanged. Catalog: 67 maps/1684 achievements/1489 potential dailies/
2223 rows. Four fallback achievements/one daily/one row removed by classification.
Original order: forty-six source-comprehensive, 174 pending. Continue with The
222nd Layer of the Abyss (`juiblex`). Active accounting remains mandatory.

## The 222nd Layer of the Abyss completed source map — October 3, 2026

The [dossier](zone-stories/THE_222ND_LAYER_OF_THE_ABYSS.md) covers all 48 blocks,
22 Q/one QA/25 addressed M and 23 exchanges; 183 rooms/109 prose groups/nine
headers/thirteen metadata/all 164 exit families; 117 mobs/113 objects, one empty-
production shop, 378 resets/all 257 families and six literal procedures. All local
reset and input/reward references exist. The two old-leash offers share one outcome:
revision one has 22 outcomes, forty contacts, 34 material/five optional producer
checks. Nineteen potential dailies preserve three native Story-only story rows.

Trace exact five heads, two same-named bodies, four parts/four essences and paired
foreign tales. Histories remain optional, including Torg eight-artifact legend and
Fields brewer recipe, warrior glove treatment, Uz brain and either leash offer.
Legend recitation consumes its input and randomly supplies one half, not both.
Uz's refill retires the shared recipient. Both Marvin aliases retire; excited form's
separate given-leash request remains vulnerable to branch order. Body recovery also
competes. Winterhaven's Adryv consumes the same machine crystal in another bundle.

**Pending repairs, none shipped:** phase transfer lacks unlink before the current
room insertion handler, so second Juiblex/Zuggtmoy cannot move through that call.
Plan separate preflight/identity/unlink/placement/continuation repair, original-failing
procedure proof and actual death→wormhole→vault/invisible-prison source journey.
Lake partial allocation, generator and consumed random-legend creation need failure
qualification; current prototypes are present. Select truthful elder/merchant and
Marvin fade/color text. Ebb's custom assignment remains commented; Flow resource,
cooldown/audience/event rejection/recovery needs qualification. Doombringer already
revalidates attack continuations and must not be called an unguarded legacy volley.

Four local essence/Marvin staging rooms have ordinary one-way DOWN exits into
foreign zones and no F metadata. Non-sentinel/non-stay-zone actors may wander;
they are not guaranteed fall dispersals. Actual falling bridges/stairs need loader
cleanup, protection and return qualification. Green-switch/closed-door, breakable
keys, generated lake/wormhole, public sources and surviving recipients remain
separate from accepted deliveries. Expand accepted source/handoff, random reading,
container custody, staged relocation and scoped lich/rebirth/escape/escort/victory
endpoints before deeper credit.

Production/native fixtures cover exact classification/sources, same-named bodies,
five distinct heads, worn/spent proof, supplied final materials, foreign ownership,
optional histories, equivalent leash receipt/replay, independent branches and cold
recovery. Synthetic receipts do not qualify actual custom generation or access.
All 2668 native definitions, revision-two fingerprint, registry and other 67 maps
remain unchanged. Catalog: 68 maps/1683 achievements/1489 potential dailies/2222
rows; equivalent aliases remove one fallback achievement/row. Original order:
forty-seven source-comprehensive, 173 pending. Continue with The Minizones of the
Surface (`surfacemini`). Active accounting remains mandatory.
