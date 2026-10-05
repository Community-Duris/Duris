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
| [dc586e34c](https://github.com/Community-Duris/Duris/commit/dc586e34c75fea9dc4016416cc95bd1637499e63): Tempest Court directions | Inspecting Si'Ciltron's return mist door now says east; the prison describes its actual north and south cells. Exactly two native words change; reciprocal exits and all other native bytes are preserved. | Player-facing: “The Tempest Court's war-chamber return door and prison-cell directions now match their actual exits.” Each original clue fails the focused regression; corrected clues/exact-byte scope pass. Live LOOK/traversal remains unqualified. |
| [f5d5b2a5c](https://github.com/Community-Duris/Duris/commit/f5d5b2a5ccfbb9fccc8f5570079dd3394c74ed54): Rift Valley Jungle / Moonhollow directions | Five description words now match reciprocal exits: wall cave south, office west, armory east, forge south and branch home north. Exits, locks, recipes and resets are unchanged. | Player-facing: “Moonhollow's wall, guard office, armory, forge and canopy-home directions now match their actual exits.” Original source regression fails; all five repaired routes and exact native-byte scope pass. Live LOOK/traversal remains unqualified. |
| [b1ff082bc](https://github.com/Community-Duris/Duris/commit/b1ff082bc03ae579aceec97b5d3ebd878bf2eea7): Desolate Master trial directions | Endurance's button is described on the southern wall and Courage's on the western wall, matching their real controls. Only two description words changed. | Player-facing: “Desolate's Master trial now gives the correct button directions in the Tests of Endurance and Courage.” Original-fails/repaired-passes source and exact-byte checks passed; live trial journey remains unqualified. |
| [b0e4ea60a](https://github.com/Community-Duris/Duris/commit/b0e4ea60a9937da5fd1789edd43402cb59672ef5): quest journal naming | Journals/dailies qualify duplicate area names with the source area, and lookup accepts those exact qualified alternatives. Previously the ambiguity prompt could request a full name that was still ambiguous. | Player-facing: “Quest journals now distinguish areas that share the same name.” Feature/journey lookup regression evidence is recorded in the qualification document. |
| [b0e4ea60a](https://github.com/Community-Duris/Duris/commit/b0e4ea60a9937da5fd1789edd43402cb59672ef5): quest state integrity | Save/recovery validation rejects missing required metadata and mismatched observation IDs; later accounting integration preserves caller-owned deletion and retry obligations. | Player-facing: “Zone quest progress saving and recovery now reject inconsistent records.” SQL/flat-file and feature regression evidence is recorded in the qualification document. |
| [492c6f1bd](https://github.com/Community-Duris/Duris/commit/492c6f1bd7635f22782888a3567763b7b90a39eb): shared prototype audit | Builder source lookup includes active administrative-area prototypes, resolving valid paper 5 without adding discoverable ownership. Production regression verifies both facts. | Builder-facing audit correction. The Tharnadia map exchange already existed; do not announce a repaired paper quest. |
| [7297b964e](https://github.com/Community-Duris/Duris/commit/7297b964ed3996c33a90dae4172cd0875addf174): Hall Shadow of Sin | Its gaze checks the actual opponent for Freedom of Movement rather than the callback actor. Periodic null actors no longer crash; bystander protection is not consumed, and target text substitutes correctly. | Player-facing: “The Shadow of Sin now correctly checks its opponent's Freedom of Movement, fixing a combat crash and misleading target text.” Actual procedure/null/unrelated-actor/effect/immunity regression and server build passed; live gameplay qualification remains open. |
| [b28262d8c](https://github.com/Community-Duris/Duris/commit/b28262d8cc8dfe6156df70f162981399d18be717): Halfcut crossbow ambusher | Setup registers periodic scheduling without firing; pulses keep three lanes/four bolts/player-only targets. Runtime identity checks stop volleys after death, movement, removal or storage reuse; missing lanes do not hide later lanes. Unbound procedures remain unbound. | Player-facing: “The Halfcut Hills kobold crossbow ambusher now fires on its scheduled pulses and stops interrupted volleys safely.” Actual-procedure regression passes and original fails setup; maintained build/format pass. Live balance/reset/accounting journey remains unqualified. |
| [348eccdf3](https://github.com/Community-Duris/Duris/commit/348eccdf3261e62aa8984ac0868b98adfa815a06): Halfcut struck-player warning | The crossbow's direct warning uses TO_CHAR for its player actor/recipient, so the shared audience filter delivers it. Previously TO_VICT suppressed that message; the room warning and four-bolt damage remain unchanged. | Player-facing addition: “The Halfcut Hills crossbow warning now reaches each struck player.” Audience regression fails the preceding procedure and passes with the fix; maintained build/format pass. This is a separate actual repair from journal authoring. |
| [f8090481d](https://github.com/Community-Duris/Duris/commit/f8090481df837bb4e1a675c901488f13090657d1): Tribal Forest exit clues | Inspecting west at 42204 and 42231 now says west; inspecting south at 42261 now says south. Exactly three description words change, preserving reciprocal routes and all other native bytes. | Player-facing: “Tribal Forest's two western forest exits and the southern village exit now describe their actual directions.” All three original clues fail the focused regression; repaired source/exact-byte scope pass. Live LOOK/traversal remains unqualified. |
| [b07b560cc](https://github.com/Community-Duris/Duris/commit/b07b560ccf2d3b7bbcb61227cfa5423dd6fa6924): Ironstar exit clues | Inspecting west at 138903 now says west; inspecting south at 138957 now says south. Exactly two native description words change, preserving reciprocal routes and all other native bytes. | Player-facing: “Ironstar's western valley exit and southern Fairlocke exit now describe their actual directions.” Both original color-normalized clues fail the focused regression; repaired source/exact-byte scope pass. Live LOOK/traversal remains unqualified. |
| [d18758098](https://github.com/Community-Duris/Duris/commit/d18758098083b28463279c130a3e7ec24bb48b24): Brass Imix Avenue exit | Inspecting east at 139017 now says east, matching D1 to 139016 and reciprocal D3. Exactly one native word changes. | Player-facing: “Brass's eastern Imix Avenue exit now describes its actual direction.” Original color-normalized clue fails; repaired source/exact native bytes pass. Live LOOK/traversal remains unqualified. |
| [3f1ecf2be](https://github.com/Community-Duris/Duris/commit/3f1ecf2be064a63321c0e0243b4168727cb079c2): Tower directions/passwords | South clues at 134011/134014 now say south; four locked magic keywords lose only trailing color resets, restoring plain Sargon/elemental passwords. | Player-facing: “The Tower of Darkness's south-facing shrine and stair clues now describe their actual directions. Its Sargon and elemental magic doors now accept the intended plain passwords.” Original source fails all six; actual C++ matching/reciprocal unlock passes, closed state retained. Live speech/LOOK/traversal remains unqualified. |
| `0fff62e70` — Smoke vault key / Discontent targeting | Vault south return lock now uses rewarded key 139818; newly loaded Discontent accepts its own name while retaining old aliases. Actual production key/name regressions fail before and pass after; key settlement/live travel/existing-object migration remain unqualified. | **The Plane of Smoke's vault key now works from either side of the portcullis, and Discontent can be selected by its own name.** |
| [5a2b93d6d](https://github.com/Community-Duris/Duris/commit/5a2b93d6d387e192ea501d6d6e5bbf71796e927b): Northern Lakes pile-of-bones clues | Cathedral clue now says east and pasture clue west at room75263, matching reciprocal D1/D3 routes. Exactly two native words change, in a separate fix commit with the focused regression. | **Northern Lakes' pile-of-bones exits now correctly point east to the cathedral and west to the pasture.** Both original clues fail; corrected source, reciprocal exits and exact native-byte scope pass. Live LOOK/traversal remains unqualified. |
| [02788c573](https://github.com/Community-Duris/Duris/commit/02788c5738a11963647ceb9b4b344463f4f57704): Kobold temple guardians | Jkyl uses altar1481/pit1484, golems tomb1482, demon pit1484/ledge1483 and the actual room list. Seven native lines and the focused executable regression are isolated in the fix commit. | **Kobold Settlement's temple guardians now defend their actual altar, tomb and sacrificial pit, restoring the high priest's imp summoning and the pit demon's ledge attacks.** Original altar regression fails; repaired actual procedures, server build and formatting pass. Played combat/difficulty/movement remain unqualified. |
| [e456b3403](https://github.com/Community-Duris/Duris/commit/e456b3403): Centaur quest/travel clues | Tamilea now points to Banitoor's eastern cave; grotto west exit, both forest approaches, forest east intersection and dead-end entrance now match actual directions. Exactly six native text lines, focused regression in a separate fix commit. | **Centaur Villages' quest and travel clues now point in the correct directions, including Tamilea's route to Banitoor's cave.** All six original clues fail; corrected source/destinations/reciprocal routes pass. Played ASK/LOOK/traversal remains unqualified. |
| [ac8e2de48](https://github.com/Community-Duris/Duris/commit/ac8e2de48f3aba3915d95abb7ee1d0fefd580f14): Opal Phoenix sand hand-in | Newly instantiated student sand70823 clears only SECRET, allowing ordinary mortal inventory display and named selection for Alazia. All other prototype bytes/native contracts are preserved; hidden chest/key/intestines retain their search paths. Separate fix commit with actual production-function regression. | **Opal Phoenix’s student sand reward is now visible and can be selected for Alazia’s delivery quest.** Original flags fail; corrected aliases and both lookup modes pass under ASan/UBSan, with negative hidden/blind/wrong/ordinal controls. Full played offering and existing saved-item remediation remain unqualified. |
| [208a56840](https://github.com/Community-Duris/Duris/commit/208a56840acb100fc20dbec4b0af9a80a6cd0914): Cloister Mahr acceptance caption | Brother Mahr accepts adamantite tablet 67100 for recommendation 67101; the acceptance caption now says tablet instead of letter. Exactly one native word changes, in a separate fix commit with its focused regression. Requirements, rewards, departure and all other native bytes remain intact. | **Brother Mahr now correctly identifies the intruder's tablet when accepting it, making the Cloister's recommendation quest clearer.** Original wording fails; corrected caption/contract and exact-byte scope pass. Played turn-in/settlement remains unqualified. |
| [7baa78c3c](https://github.com/Community-Duris/Duris/commit/7baa78c3c8ee67a2059c314e6372f5ae1843e989): Ixarkon direction clues | LOOK descriptions in11 rooms now match12 reciprocal routes: slave pens, garden path, tower gate, camp approaches, sandstone passage, Rockspire tunnel, dwelling and water cavern. Exactly12 direction words change; all routes/metadata/quests/resets remain intact. | **Ixarkon's slave pens, fungal gardens, towers, camp approaches, Rockspire tunnels and water cavern now give directions that match their exits.** Original12 clue checks fail; corrected clues, reciprocal exits/door metadata and exact native scope pass. Live LOOK/traversal remains unqualified. |
| [45bb3c948](https://github.com/Community-Duris/Duris/commit/45bb3c94896411cb06ce835b42824da8144d7936): Du'Maathe northwest parapet clue | LOOK at37248 now says north rather than south, agreeing with the title and northern/western reciprocal wall roads. Exactly one native word changes; all other bytes remain intact. | **Du'Maathe Castle's northwest parapet now correctly identifies its position in the room description.** Original clue fails; corrected clue/title/reciprocal routes and retained southwestern corner pass. Live LOOK/traversal remains unqualified. |
| [7b916b887](https://github.com/Community-Duris/Duris/commit/7b916b887): Moregeeth letter desk | Exactly one native keyhole field changes -1→0, allowing normal PICK/KNOCK attempts. Closed/locked state, opening trap, letter placement and Tala exchange remain intact. | Player-facing: “Moregeeth’s trapped letter desk now permits normal lockpicking or knock attempts, restoring access to the letter for Tala’s quest.” Original regression fails; repaired source/exact bytes pass. Played skill/trap/pickup/offer/settlement remains unqualified; saved instances are not rewritten. |
| [5b4c4f34a](https://github.com/Community-Duris/Duris/commit/5b4c4f34a151f624d21558fb86cb815f05cf76b1): Braddistock entry refusal | Spirit's existing speech now goes to the blocked player with CRLF, rather than the descriptorless NPC. Exactly two native lines; level threshold, gate and observer actions remain intact. Separate fix commit with actual production-function regression. | **Braddistock Mansion's spirit now delivers its entry refusal directly to the blocked player.** Original function fails; repaired player-recipient/exact-line, level14/15, trusted/null/periodic/self/unrelated-command controls pass. Maintained build/format/exact native-byte scope pass; played network entry remains unqualified. |
| [d2a64432d](https://github.com/Community-Duris/Duris/commit/d2a64432d81c6f98c497f14a47334a85c6dd39e0): withdraw Fields Between portability repair | Restores original TAKE=0 and weight1,000,000 for unlimited rift71030; full object file matches pre-fix bytes. Prior05eeca928 introduced an unjustified portable escape capability. | **Withdraw earlier pickup/delivery news.** Quest/source inconsistency remains unresolved. Unsafe prototype fails restriction regression; restored data passes. No deployment or saved-instance change occurred. |
| [27c78dec3](https://github.com/Community-Duris/Duris/commit/27c78dec31370e23452b81e6a4324d11f0633f17): Desolate Under Fire trial directions | Exactly two room words: Endurance77412 eastern→southern; Courage77414 eastern→western, matching existing PUSH controls/exits. Separate fix with extended existing regression. | **Desolate Under Fire’s Master trial now gives the correct south and west directions in the Endurance and Courage chambers.** Each original fails independently; repaired both variants/exact native bytes pass. Live LOOK/PUSH/traversal unqualified. |
| [4b25c5f17](https://github.com/Community-Duris/Duris/commit/4b25c5f177ebb11d4a2eabab286b5c2a2e5a55f9): Storm Port Stronghold outside clue | Exactly one native word in22607:outpost east→west. Forest stays east; all four existing reciprocal exits intact. Separate fix/source regression. | **Storm Port Stronghold’s outside-path description now correctly points west toward the outpost.** Original caption fails; corrected caption/four routes/exact bytes pass. Live LOOK/traversal unqualified. |

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
| 48 | The Minizones of the Surface | [Comprehensive source dossier](zone-stories/THE_MINIZONES_OF_THE_SURFACE.md): all 59 blocks/25 exchanges/32 addressed, 119 rooms/25 mobs/42 objects, 145 resets/66 families, three literal procedures and bounded foreign producers/hosts/routes | Revision one: five stories/sixteen services, 38 contacts/73 optional checks; five potential dailies, exact copies/kinds and any-one cleansing | No native repair ships. Guarded fees, source/recipient choices, wand/pet creation and custody, clue/flame cleanup fixes, missing ordinary entries and dormant igloo/foreign laboratory plans pending |
| 49 | The Mountain of Peril Peaks | [Comprehensive source dossier](zone-stories/THE_MOUNTAIN_OF_PERIL_PEAKS.md): all 21 blocks/ten deliveries/eleven addressed; 196 rooms/162 mobs/85 objects/two shops, 297 resets/236 families, automatic switches/shared travel and reciprocal Mountain Tracks boundaries | Revision one: ten outcomes, 28 contacts/16 optional checks; ten potential dailies, exact parts and three optional intermediate histories | No native repair ships. Companion-fate finale, causal source/custody, accepted controls/key/arrival and scoped campaigns pending; ring/book, aliases and dormant content need builder decisions |
| 50 | Crakkaros' Liar | [Comprehensive source dossier](zone-stories/CRAKKAROS_LIAR.md): all twenty blocks/eleven QA/nine MA; 372 rooms/37 mobs/68 objects, 501 resets/199 families, mounted centaur, shared P/switch/key execution and inactive ocean boundary | Revision one: six stories/five services, 29 contacts/27 optional checks; five authored story daily candidates, nine native candidates unchanged | No native repair ships. Fourteen-root guard versus seventeen furs, R owner extraction, actual shelf custody, secret/key/retiring episodes and finale adapters planned; sculpture/aliases/reverse key/exterior intent pending |
| 51 | Rogue Plains | [Comprehensive source dossier](zone-stories/ROGUE_PLAINS.md): all twelve blocks/nine Q/three addressed M; 158 rooms/58 mobs/57 objects, 223 resets/127 families, direct Master procedure, mount/container/portal/aerial execution and foreign Balance owner | Revision one: seven outcomes, 24 contacts/16 optional checks; seven grouped story daily candidates, nine native candidates unchanged | No native repair ships. Accepted source/effect/access/reset episodes and R/owner extraction planned; identical proof, sword/hammer wording, meat/wand/set intent and historical northern connection need builder qualification |
| 52 | Desolate | [Comprehensive source dossier](zone-stories/DESOLATE.md): all twelve blocks/eleven Q/one addressed M, 168 rooms/114 mobs/92 objects/three shops, 400 resets/289 families; direct Master/computed teacher, container/fee/control/phase and foreign letter execution | Revision one: nine stories/two services, 27 contacts/16 optional checks, eight story daily candidates | Separate native south/west trial-button text fix ships with original-fails/repaired-passes source proof and news; missing stock/other intent pending. Atomic fee, content-tree preview/predicates, control/phase/trial/effect and foreign scope planned |
| 53 | Rift Valley Jungle | [Comprehensive dossier](zone-stories/RFTJNGLE.md): 65 blocks, 470 rooms/220 mobs/220 objects/eight shops/828 resets; direct procedural hosts/computed teachers, source instances/secret search/paired portals/foreign boundaries | 12 outcomes/12 services/3 excluded contracts, 49 contacts/30 optional checks; 11 story candidates | Separate five-direction native fix/news ships; 15-root batch/seven fees guarded, plot/dragon/historical boundary pending; provenance/role/travel/actor/AND/procedural lifecycle planned |
| 54 | The Transparent Tower | [Comprehensive dossier](zone-stories/TRNSPTOW.md): all thirty blocks/100 rooms/40 mobs/75 objects/208 resets; five specials, fragile keys, command portals and imported epic/artifact roles | Four outcomes/fourteen contacts/eleven optional checks; one daily candidate | No native repair ships; pulse/misdirection/shortcut/trash intent pending. Accepted source/access/command/epic/ALL/escape evidence planned |
| 55 | The Tempest Court | [Comprehensive dossier](zone-stories/TEMPEST_COURT.md): all 26 blocks/200 rooms/52 mobs/63 objects/307 resets; full active dagger, shared execution and foreign proof chains | Seven outcomes/eight contracts, 21 contacts/28 optional checks; five candidates, supplied proof and fragment allocation | Separate two-word direction fix ships; upgrade/portal/lightning/dispersal intent pending. Source-generation/access/branch/actor/epic/ALL events and live supply qualification planned |
| 56 | The Caverns of Armageddon | [Comprehensive dossier](zone-stories/CAVERNS_OF_ARMAGEDDON.md): all 33 blocks/149 rooms/94 mobs/69 objects/393 resets; imported procs/shared execution and foreign follow-ups | Eighteen independent outcomes, 40 contacts/27 optional checks; exact heads/tags/parts/amulets and supplied acceptance | Roland placement; admitted supply, custody/source, successful access/key/trap, actor/ALL and allocation events; boundary/key/prose/effect intent pending. No native repair ships here |
| 57 | Tribal Forest | [Comprehensive dossier](zone-stories/TRIBAL_FOREST.md): all 25 blocks/175 rooms/67 mobs/103 objects/377 resets; shop/orb/shared execution and source ownership | Nine outcomes/one refusal exclusion; seventeen contacts/21 optional checks; exact grain/clothing/crystal/parts/egg progression | Admitted staged supply, recovery/transfer/replacement, reused key/control/travel/HP/actor events; trap/text/grove/shop-audit intent pending. Separate three-word native direction fix/news ships |
| 58 | The Ancient Halls of Ironstar | [Comprehensive dossier](zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md): all 22 blocks/100 rooms/34 mobs/66 objects/198 resets; native/shared source and effective access | Four stories/three services; fifteen contacts/fourteen optional checks; exact rings/crown/paid key/three locks and independent equipment routes | Guarded atomic fees, reset door state, source/transfer/allocation and retiring supply; river/prose/lore/epic concerns pending. Separate two-word native direction fix/news ships |
| 59 | Plane of Fire, Brass | [Comprehensive dossier](zone-stories/PLANE_OF_FIRE_BRASS.md): all 20 blocks/357 rooms/147 mobs/170 objects/18 shops/779 resets; native/shared source and effective access | Four stories/one service/two exclusions; 25 contacts/18 optional checks; collectible coins, quantity-two vials, competing heads and guarded bracer | Actor/barrier/access episodes, duplicate reward/proof allocation, rare wandering/perception and atomic fees; builder-selected djinn/ambient/prose/topology proposals. Separate one-word native exit fix/news ships |
| 60 | The Tower of Darkness | [Comprehensive dossier](zone-stories/TOWER_OF_DARKNESS.md): all 32 blocks/142 rooms/170 mobs/146 objects/one shop/580 resets; full exit text/passwords and shared execution | Six stories/seven owned bindings; 27 contacts/12 optional checks; guarded alternative, five-sword bundle, three-key puzzle and five physically local foreign-owned requests | Physical affiliation/discovery separate from owner, per-branch fee support, accepted access/source/campaign events and builder-selected rescue/holding/entry/text proposals. Separate native direction/password fix/news ships |
| 61 | Mushroom Caverns | [Comprehensive dossier](zone-stories/MUSHROOM_CAVERNS.md): all sixteen raw blocks/132 rooms/eight local mobiles/nine objects/54 resets; imported actor/material sources, shared execution and modern identity comparison | Three outcomes/ten contacts/22 representable aliases/five optional checks; exact same-name halves, optional history and explicit source/accounting blockers | Missing goblet/unplaced actors/five absent dispersal destinations, money-proof admission and reward/get/drop, physical affiliation/renewal, literal topic tokens and route/scenery intent. No native repair ships |
| 62 | Para-Elemental Plane of Smoke | [Comprehensive dossier](zone-stories/PARA_ELEMENTAL_PLANE_OF_SMOKE.md): all fourteen blocks/153 rooms/26 mobiles/36 objects/172 resets; foreign planar entrances, current-mobile supply and shared key/terrain/weapon/epic execution | Two stories/four services, twelve contacts/16 aliases/12 optional checks; exact two-kind bundles, optional routes, non-credit reversible jewelry | Shipped native fix 0fff62e70 corrects vault return key and Discontent alias. Negative-plane forge, permanent Power proc, key settlement, rare admitted supply/renewal and builder intent remain pending |
| 63 | Fishermans Wharf | [Comprehensive dossier](zone-stories/FISHERMANS_WHARF.md): all thirteen blocks/70 rooms/23 mobiles/20 objects/119 resets, shop, ordinary Surface boundary, imported skull and shared fishing/breathing/key/fall execution | Five independent outcomes, thirteen contacts/44 aliases/10 optional checks; exact 8-root/four-copy bundles and optional bait/line history | No native repair. Fishing narration/XP precedes ownership grant; effect alternatives, source provenance, admitted supply, command-time fall/access and restricted foreign custody require qualification |
| 64 | Northern Lakes and Settlements | [Comprehensive dossier](zone-stories/NORTHERN_LAKES.md): all fourteen blocks/219 rooms/63 mobiles/82 objects/414 resets, both shops, five foreign boundaries, imported visage/consumer and shared quest/reset/current/fall/boat execution | Six independent stories, thirteen contacts/20 aliases/9 optional checks; courier stage receipts stay optional with supplied exact materials | Separate fix 5a2b93d6d corrects two exit clues. Distinct-source scales, overlapping recipient/supplier, typed actor/current/fall episodes, campaign all-stage display and actual daily renewal need qualification |
| 65 | Kobold Settlement | [Comprehensive dossier](zone-stories/KOBOLD_SETTLEMENT.md): all seventeen blocks/147 rooms/58 mobiles/66 objects/303 resets/three shops; ten forge rows, guardian/death/switch/epic/inn/shared paths and foreign closure | One guarded story/three services; sixteen contacts/16 aliases/7 optional checks; supplied materials and historical support receipts remain independent | Separate guardian fix 02788c573 restores actual rooms and ledge targeting. Atomic fees/overlapping recipe order, admitted supply, custom death/access/forced travel and legacy forge/rod/inn intent remain pending |
| 66 | Troll Caves | [Comprehensive dossier](zone-stories/TROLL_CAVES.md): all ten blocks/82 rooms/28 mobiles/37 objects/193 resets, 142 exact/152 parent families, computed smith/teacher/switch/teleport and foreign stock | One story/four services; eight contacts/9 aliases/8 optional checks; exact same-name kinds, supplied materials and producer history independent | No native repair. Atomic fees, wand mismatch, secret control returns, PUNCH sign/foreign copy, guarded forge and accepted source/travel/fall/survival/renewal qualification remain plans |
| 67 | Centaur Villages | [Comprehensive dossier](zone-stories/CENTAUR_VILLAGES.md): all eleven blocks/100 rooms/29 mobiles/31 objects/199 resets,117 exact/125 parent families; dynamic switches/teachers, actual hidden-name lookup/current/fall/offer/retirement and foreign closure | Four stories/three services; eight contacts/8 aliases/16 optional checks; two distinct roots of one half kind, optional supplied and reset sources, independent current/history | Separate six-direction clue fix e456b3403. Active source/selection/movement/two-root/two-output settlement, Hateeu retirement/reappearance and renewal remain unqualified; new semantic access/knowledge/effect episodes require accepted proof |
| 68 | Enclave of the Opal Phoenix | [Comprehensive dossier](zone-stories/OPAL_PHOENIX.md): all6 blocks/76 rooms/18 mobiles/24 objects/84 resets/76 exact and parent families; actual hidden selection/search/key, indexed item/XP/D1, shop/teacher/inn and bounded foreign closure | Three outcomes, nine contacts/8 aliases/4 optional checks; supplied optional student route, independent forest reagent and exact hidden-source distinctions | Separate sand70823 visibility fix ac8e2de48. Live source/handoff/search/key/GET, items+XP, recipient/stock removal/reappearance, service/rent and renewal remain unqualified; richer accepted episodes are planned |
| 69 | Myrabolus | [Comprehensive dossier](zone-stories/MYRABOLUS.md): all17 blocks/188 rooms/78 mobiles/62 objects/316 resets,257 exact/272 parent families; two shops, literal/computed/imported handlers and bounded foreign closure | Thirteen stories/three support entries,21 contacts/4 aliases/24 optional checks; exact returned-note/new-half, same-name study/parts, four treasure allocations, awake Alexis and supplied shortcuts | No native repair. Missing treasury kind, Roland/load-room/set/wording intent, shared denied crew-payment ordering and admitted fee/source/access/reward/recipient/renewal episodes remain plans |
| 70 | The Depths of Duris | [Comprehensive dossier](zone-stories/THE_DEPTHS_OF_DURIS.md): all103 blocks/2645 rooms/75 mobiles/60 objects/611 resets,406 exact/530 parent families; ten shops, custom death/decay/CARVE and bounded foreign closure | Nine outcomes/three services,17 contacts/109 aliases/15 optional checks; four brew alternatives, same-name feather kinds, ten loose roots and supplied seer shortcuts | No native repair. Six absent destinations, wall/stair intent, blue/pink wording and unplaced residents remain proposals; accepted source/part mutation/publication/expiry/payment/reward/retirement/renewal qualification remains open |
| 71 | IceCrag Castle | [Comprehensive dossier](zone-stories/ICECRAG_CASTLE.md): all67 blocks/243 rooms/66 mobs/138 objects/620 resets,374 exact/460 parent families; twenty local assignments, full speech gates/command portals/custom encounters/foreign closure | Eight stories/two services/one incomplete exclusion,25 contacts/129 aliases/16 optional checks; exact 3-page/2+2-bottle/two-heart bundles and optional shoes history | No native repair. Missing Masha ingredient/dungeon, wine cap, duplicate kinds and dormant/global behavior remain proposals; accepted access/source/control/transform/fee/settlement/renewal qualification remains open |
| 72 | Father Tel's Holy Cloister | [Comprehensive dossier](zone-stories/FATHER_TELS_HOLY_CLOISTER.md): all 36 blocks, 71 rooms, 24 mobiles, 31 objects and 113 resets; four automatically bound switches, complete shared and foreign source closure | Seven stories/one supporting rejection; 16 contacts/28 aliases/11 optional checks. Exact robes/tablet/head/tome/note/egg and paired ring/poison, supplied shortcuts and actual access actions | Separate Mahr tablet-caption fix 208a56840. Formal admission, lore/source mismatch, ten-minute mission and orphan intent remain proposals; accepted source/trap/control/search/key/XP/settlement/renewal qualification remains open |
| 73 | The Ruins of Turolopolis | [Comprehensive dossier](zone-stories/RUINS_OF_TUROLOPOLIS.md): all25 blocks/154 rooms/42 mobiles/56 objects/199 resets, all six type25 portals and complete shared/foreign source closure | Six stories;26 contacts/39 aliases/12 optional checks. Exact five-colour ALL memorial → lesser blade + caecilia stinger → upgrade; independent horn, letter, ooze and skull offerings | No native repair ships. Minotaur source/recipient generation, foreign Lothrell home-journal navigation, rare dispersal/sink, rescue/purification intent and accepted access/custody/XP/coin/renewal qualification remain plans |
| 74 | Ixarkon | [Comprehensive dossier](zone-stories/IXARKON.md): all18 blocks/201 rooms/51 mobiles/44 objects/385 resets/five shops and complete shared/foreign closure | Revision2 upgrades existing three identities: two stories/one service;16 contacts/22 aliases/four optional checks. Independent spore and amulet returns; optional paid banker route and supplied amulet | Native direction clues repaired separately in7baa78c3c. Mixed fee guards, absent veil ingress/guarded binding, random arrival, open bridge controller, source custody and actual rescue/peace/renewal remain plans |
| 75 | Du'Maathe Castle | [Comprehensive dossier](zone-stories/DU_MAATHE_CASTLE.md): all21 blocks/506 rooms/92 mobiles/49 objects/419 resets/one shop and bounded foreign/shared closure | New schema3/revision1: four lord stories/four potion services;15 contacts/15 aliases/14 optional checks. Exact two-input/three-output granular batch and distinct keys; supplied potion/tooth branches | Separate one-word parapet repair45bb3c948. Missing blue horn, foreign price mismatch/fee guards, loader/reset visibility, consumed recipe lineage and actual potion/campaign/renewal predicates remain plans |
| 76 | Tundra | [Comprehensive dossier](zone-stories/TUNDRA.md): all20 blocks/175 rooms/26 mobiles/32 objects/246 resets/one shop and bounded foreign/shared closure | New schema3/revision1: six stories/one paid armor service;16 contacts/16 aliases/14 optional checks. Four different books → snowy boots → Eleadora; three seafood kinds → fishbone key; independent gland/shell rewards and head retirement | No native repair. Branch/trap availability, inactive/signposted routes, land-coded docks, absent fish shop, paid guards, partial foreign supply, actual switch/key/source and campaign/renewal remain plans |
| 77 | The Fields Between | [Comprehensive dossier](zone-stories/FIELDS_BETWEEN.md): all17 blocks/153 rooms/68 mobiles/33 objects/286 resets and bounded foreign/shared closure | New schema3/revision1: seven stories;22 contacts/81 aliases/17 optional checks. Timmy alloy → letter → roaming mother; shared mithril, manuscript/bananas, orders/gift, five distinct heads and unresolved fixed-rift request | Portability fix05eeca928 withdrawn; correctiond2a64432d restores original restrictions. No pickup-fix news. Roaming/ownership, source lineage, portal consumption, narration versus campaign and unresolved prose remain plans |
| 78 | The Town of Moregeeth | [Comprehensive dossier](zone-stories/THE_TOWN_OF_MOREGEETH.md): all20 blocks/353 rooms/109 mobiles/96 objects/549 resets/ten shops and bounded foreign/shared closure | Schema3/revision2 preserves seven IDs: five stories/two paid services;21 contacts/22 aliases/15 optional checks. Pouch → key → four planar components; exact head, killer’s sword and trapped letter | Actual desk-keyhole repair is separate fix7b916b887 with news. Actor availability/retirement, access/portal commands, container/trap lineage, fees and narrative campaigns remain qualification plans |
| 79 | Ceothia | [Comprehensive dossier](zone-stories/CEOTHIA.md): all21 blocks/295 rooms/110 mobiles/33 objects/510 resets/one shop and bounded timeline/shared closure | Six cards bind nine recipes: one four-way guild choice, three independent Lenbrea rewards, paired crates and legacy scroll;17 contacts/22 aliases/17 optional checks | No native repair ships. Actor availability, source/gift custody, keys/container/travel/effects and campaign prerequisites remain plans; pool targeting and legacy tablet/learning are explicit repair proposals |
| 80 | Braddistock Mansion (brad) | [Comprehensive dossier](zone-stories/BRADDISTOCK_MANSION_1350.md): all14 owned Tower blocks/48 mansion rooms/14 mobiles/73 objects/175 resets; full local gate and bounded foreign closure | Schema3/revision2 preserves5 IDs/bindings;16 contacts/32 aliases/ten optional checks; local exploration guidance and five physically Tower exchanges | Separate entry-refusal recipient/CRLF fix ships. Physical affiliation/discovery/renewal, learned clues/source/access, alchemy/boat/campaign and missing stock are explicit plans |
| 81 | The Desert City of Venan'Trut | [Comprehensive dossier](zone-stories/DESERT_CITY_OF_VENAN_TRUT.md): all16 blocks/677 rooms/245 mobiles/194 objects/1256 resets/ten shops and shared/imported closure | Eight independent exchanges;22 contacts/46 aliases/11 optional checks explain mine, rival signets, compass, contraband and Queen→Goranon→Eriic→Winterhaven | No native repair ships. First source/gift, actual switch/door/river travel, computed services/group claims, foreign memory/fabrics and actor renewal remain plans; orphan portals/clue/inn overlap need builder intent |
| 82 | Past Ceothia | [Comprehensive dossier](zone-stories/PAST_CEOTHIA.md): all14 blocks/293 rooms/46 mobiles/25 objects/129 resets and bounded timeline/shared closure | Four cards bind six recipes: blossom, three components, any-color pelt and current black-pelt mismatch;13 contacts/21 aliases/11 optional checks explain actual sources and three breaking-key gates | No native repair ships. Wolfspeed proof mismatch, rare staging probability/availability and mode0 renewal are builder follow-ups; source/gifts, committed access/travel and timeline campaigns remain plans |
| 83 | The Basin Wastes | [Comprehensive dossier](zone-stories/THE_BASIN_WASTES.md): all16 blocks/200 rooms/15 mobiles/32 objects/204 resets and bounded imported/shared closure | Seven cards bind ten recipes: ring ritual, four distinct crafts, any-one cash sale and zero-credit heartstone refusal;8 contacts/14 aliases/14 optional checks explain sources and offered-item dispatch | No native repair ships. Dispatch choice, learned books, source renewal, same-kind refusal semantics and clue consistency are builder follow-ups; no new Crystal City exit or spider bypass |
| 84 | Nakral's Crypt | [Comprehensive dossier](zone-stories/NAKRALS_CRYPT.md): all11 blocks/294 rooms/59 mobiles/201 objects/521 resets, F-current-holder and bounded imported/shared closure | Five cards/five recipes/nine contacts/21 aliases/14 optional checks; exact four/two wood, five same-name chunks, trophy→token→bracelet and distinct collar upgrade | No native repair. Learned words, four control actions, exact follower sources/renewal, collar presentation, stale surface exit and clue/epic investigation are builder follow-ups; preserve fixed mobility and Fields protection |
| 85 | The Valoisian Castle | [Comprehensive dossier](zone-stories/THE_VALOISIAN_CASTLE.md): all13 blocks/174 rooms/93 mobiles/57 objects/320 resets, shared inn and bounded Surface/Verspin closure | Eight cards/eight recipes/18 contacts/26 aliases/15 optional checks; separate family/royal seals, four models, wine→dinner→token, two rose gifts and overdue note | No native repair. Personal sources/renewal/access/learning, political endpoints, blank flower responses, unfinished components and clue review require builder decisions; preserve Fields protection |
| 86 | Harrow -The Gnome Village | [Comprehensive dossier](zone-stories/HARROW_THE_GNOME_VILLAGE.md): all29 blocks/82 rooms/59 mobiles/88 objects/280 resets, nine shops/shared dispatch and bounded imported/Surface closure | Eight cards/eight recipes/14 contacts/12 aliases/20 optional checks; ring→fresh tokens for four crafts, painting→fish, fish food→experience, pot item→clover/coins | No native repair. Source/renewal/token choice, committed travel, lucky-star entrance, clue consistency and learning remain builder work; preserve Fields hotfix |
| 87 | Mountain Tracts of the Untamed | [Comprehensive dossier](zone-stories/MOUNTAIN_TRACTS_OF_THE_UNTAMED.md): all8 blocks/245 rooms/98 mobiles/63 objects/259 resets, shop/shared controls and bounded foreign/boundary closure | Four cards/four recipes/eight contacts/19 aliases/six optional checks; miniature→leggings, marble→bracer, scale+tooth→potion, fresh potion→gloves | No native repair. Source/renewal, potion allocation, accepted access, lore endpoints, boundary intent and clues remain builder work; preserve Fields hotfix |
| 88 | The Orcish Slave Camp | [Comprehensive dossier](zone-stories/THE_ORCISH_SLAVE_CAMP.md): all 7 blocks/14 rooms/31 mobiles/18 objects/64 resets and shared key/quest/retirement/food/boundary closure | Two cards/three classified recipes/one typed-food exclusion/five contacts/13 aliases/four optional checks; key→coins+steak, exact steak→mace+XP+tragic departure | No native repair. Source/renewal, fragile-key access/allocation, type admission, selected recipient retirement, steak-type intent and clues remain builder work; preserve Fields hotfix |
| 89 | The Underground Lava Caves | [Comprehensive dossier](zone-stories/THE_UNDERGROUND_LAVA_CAVES.md): all6 blocks/142 rooms/48 mobiles/33 objects/163 resets and shared coin/reset/key/control/fire/wandering/boundary closure | One card/two classified recipes/one coin-purchase exclusion/seven contacts/six aliases/three optional checks; supplied horns→wrist chain+XP | No native repair. Sole producer/renewal, coin debit, moving recipient, access/fire evidence, unusual onyx-key intent, services and clues need builder work; preserve Fields hotfix |
| 90 | The Cimmerian Nomad Encampment | [Comprehensive dossier](zone-stories/THE_CIMMERIAN_NOMAD_ENCAMPMENT.md): all5 blocks/49 rooms/21 mobiles/23 objects/102 resets and shared ALL roots/echo/wake/page/decay/custody/kit/arrival closure | Two cards/two QA/seven contacts/five aliases/six optional checks; shards→collateral ring; distinct heads+ring→crown+Septimus departure | No native repair. Sources/renewal, hidden transient proof, collateral allocation, investigation evidence, recipient retirement, prop/services and geography/clues need builder work; preserve Fields hotfix |
| 91 | The Ruins of Undermountain | [Comprehensive dossier](zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md):all5 blocks/441 rooms/95 mobiles/135 objects/731 resets; full dormant versus active controller and global source closure | Two cards/two hand-ins/three contacts/eight aliases/three optional checks; intact key→Tamsil note→Durnan scimitar; receipts independent | No native repair. Key allocation/break, visibility, note provenance, rescue retirement, access, source renewal and dormant intent need builder work; preserve Fields hotfix |
| 92 | Desolate Under Fire | [Comprehensive dossier](zone-stories/DESOLATE_UNDER_FIRE.md):all17 blocks/165 rooms/110 mobiles/96 objects/332 resets/full shop/phase and global closure |17 cards/16 story outcomes/1 service/16 contacts/27 optional checks;8 rescues→8 current bindings;3 hands;monkey→chain | Separate two-word trial-direction fix; eight builder follow-ups for supply, allocation, nesting, phase, retirement, missing repair/drink, trial and services |
| 93 | Storm Port Stronghold | [Comprehensive dossier](zone-stories/STORM_PORT_STRONGHOLD.md):all4 Q/65 rooms/44 mobiles/34 objects/129 resets/master set/crew shop/foreign treasure closure | Four cards/six contacts/eight optional checks;coal+valve→ticket→helm;two independent maps;optional Thetis referral | One-word direction fix separate; eight follow-ups. Shared crew debit refusal ignored:required accounting fix pending, no live repair claimed |
| 94 | The Mountain Settlement of the Harpies | [Comprehensive dossier](zone-stories/THE_MOUNTAIN_SETTLEMENT_OF_THE_HARPIES.md):all3 Q/161 rooms/44 mobiles/19 objects/176 resets/2 shops/4 specials | Replaces generic journal with schema3/revision2:2 cards/5 contacts/one custom ASK/3 optional rows/one shadowed exclusion | Eight follow-ups; native recognition differs from custom allegiance. Recipient/actor/atomic transition and dormant corpse/exit policy require separate work; no native repair |
| 95 | The Behemoth Herders | [Comprehensive dossier](zone-stories/THE_BEHEMOTH_HERDERS.md):all12 Q/29 dialogue/347 rooms/144 mobs/97 objects/624 resets/full shop; full shared source/foreign closure | Schema3/revision1:12 cards/6 stories/6 services/15 contacts/52 aliases/19 optional materials; exact halves/eyes/sword+soul/diorite consumers | Nine follow-ups; actual follower custody/global P selection, mixed fees, shared access and narrative episodes; mismatched reverse key/text pending intent; no native repair |
| 96 | Jotunheim | [Comprehensive dossier](zone-stories/JOTUNHEIM.md):all15 Q/34 ASK plus two timers/296 rooms/77 mobiles/82 objects/392 resets/full procedures and shared/foreign closure | Schema3/revision1:15 cards/11 stories/4 services/10 contacts/47 aliases/19 optional materials; five distinct proofs, competing sword and independent rivals | Eleven builder follow-ups; shared passwords/rare renewal/mixed fees/trainer override/narrative episodes/proof custody/combat/return intent/unfinished stock/foreign ownership; no native repair |
| 97–220 | Remaining roadmap | Pending comprehensive review; earlier rough proposals and complete Q classification remain useful evidence | Existing authored maps/native fallback retained | Work through original queue; record each reviewed family and custom dependency |

The next area is Temple of Flames (`temple`).
Source and gameplay qualification remain distinct throughout the full queue.
Do not advance a zone's status merely because the map parses, the Q denominator
matches, or a candidate item graph was extracted.

## Findings that expand or reorder the implementation plan

| Finding | Status / scope | Concrete next action |
| --- | --- | --- |
| ZSQ-BRAD-PHYSICAL-AFFILIATION / DISCOVERY / RENEWAL / EXPLORATION | Five Tower contracts retain Braddistock credit ownership; Tower encounters do not discover this journal, and owner reset mode differs from physical retiring recipients. Local exploration has no native terminal. | Keep canonical receipt identities. Add explicit physical affiliation/referrals and real source/actor renewal; admitted clue/access/custody and builder-designed lab/smuggler/campaign endpoints. Entry speech repair ships separately; missing key/weapon supply and copied prose remain proposals. |
| ZSQ-CEOTHIA-CHOICE / TIMELINE / TARGETED-EFFECT / LEGACY-TRAINING | Four retiring guild alternatives, three independent Lenbrea rewards, breaking keys, two nested crates, foreign timeline proofs, imported agility pool and legacy tablet/scroll differ from modern epic purchases. | Six source-guided cards ship. Qualify source/recipient reset episodes, actual access/travel/targeted effects and shared-material ownership; define all-stage campaign and restore/retire legacy training only with builder intent. Pool target ambiguity is a pending shared repair, not shipped news. |
| ZSQ-UNDERMOUNTAIN-SOURCE-RENEWAL / GRATE-KEY-ALLOCATION / PROOF-VISIBILITY / NOTE-PROVENANCE / RESCUE-RETIREMENT / ACCESS-EVIDENCE / DORMANT-STORY-INTENT | Key competes between unlock breakage and offering; blank writable note links two independent exchanges; source/reward hidden; NPC specials compiled out. | Qualify source/renewal, key allocation, visibility, provenance, retirement and admitted access facts. Builder decides dormant mechanics; no stock/flag/lock/portal/controller repair. |
| ZSQ-NOMADS-SOURCE-RENEWAL / HIDDEN-TRANSIENT-PROOF / COLLATERAL-ALLOCATION / INVESTIGATION-EVIDENCE / RECIPIENT-RETIREMENT / SERVICE-PROP-INTENT / BOUNDARY-CLUE-CONSISTENCY | Hidden/transient heads are carried while NPCs live; collateral ring is consumed; page/wake clues and room narration are separate from accepted bundles; crown has CHAOS source. | Two existing-schema cards ship. Qualify source/decay/custody, exact current ALL roots, optional actor/page/status facts and NPC instance/epoch. Builder decides material-only trophies versus true kill prerequisite; no pickup/type/flag/stock/mobility repair. |
| ZSQ-LAVCAV-SOURCE-RENEWAL / COIN-PURCHASE / WANDERING-RECIPIENT / ACCESS-EVIDENCE / ONYX-KEY-INTENT / SERVICE-ENDPOINTS / CLUE-CONSISTENCY | Sole horns producer is refused active coin purchase; seller can wander from isolated spawn into lake or trap; native gates/control/fire and unusual key need precise evidence. | One supplied-horns return card and explicit purchase exclusion ship. Qualify actual payer wallet/item issuance, selected NPC instance/epoch, legitimate current material/renewal and admitted prerequisites. Preserve spawn trap, rarity, types, locks and PvP. No native repair. |
| ZSQ-SHORTC-SOURCE-RENEWAL / KEY-ACCESS-ALLOCATION / TYPED-FOOD-ADMISSION / RECIPIENT-RETIREMENT / STEAK-TYPE-INTENT / CLUE-CONSISTENCY | Hidden cap1 key;20-percent break versus exact hand-in; TRASH steak tragic departure versus refused T19 food; template/room recipient reselection and stale clues. | Two existing-schema cards and one explicit unsupported exclusion ship. Define qualified type roots and recipient instance/epoch; review prop/clue intent before mechanics. No native repair, edible-prop conversion or access bypass. |
| ZSQ-MOUNTAINTRACKS-SOURCE-RENEWAL / POTION-ALLOCATION / ACCESS-RESULTS / STORY-ENDPOINTS / BOUNDARY-INTENT / CLUE-CONSISTENCY | Hidden/NORENT cap1 proof; optional history versus consumed potion; valid GRAB vine and PUSH without OPEN; blank/lore endpoints; reserve/missing directions and misleading travel clues. | Four existing-schema cards ship. Authoritative sources/allocation/access/learning and intended boundary/text design remain builder work. No native repair, pickup, opcode change or activation. |
| ZSQ-HARROW-SOURCE-RENEWAL / TOKEN-PROGRESSION / TRAVEL-ARRIVAL / LUCKY-STAR-ACCESS / CLUE-CONSISTENCY / LEARNED-CRAFTS | One consumed token per craft; finite alternate reward stock; exact hidden/worn sources; fixed travel and isolated lucky-star component; room echo versus ambient scenes. | Eight existing-schema cards ship; authoritative sources/reservation/access/learning and intended entrance/text need builder design. No native repair, pickup or route activation. |
| ZSQ-VAL-SOURCE-RENEWAL / ACCESS-LEARNING / POLITICAL-ENDPOINTS / FLOWER-EXPLANATION / UNFINISHED-COMPONENTS / CLUE-CONSISTENCY | Exact F-held models, source alternatives, breaking keys, secret closed ladders, supplied dinner and two blank flower exchanges;15 isolated unfinished Veralis rooms. | Eight existing-schema cards ship. Plan authoritative source/learning/access and meaningful political endpoints; native copy/component/stock design remains builder work, no native repair or mobility activation. |
| ZSQ-CRYPT-LEARNED-WORDS / CONTROL-ACCESS / SOURCE-RENEWAL / COLLAR-PRESENTATION / STALE-SURFACE-EXIT / CLUE-CONSISTENCY / EPIC-INVESTIGATION | Three-note magic word works; four fixed switches; F changes current proof holder; same-name chunk/collar identities differ; old surface target absent active. | Five guided cards ship. Plan authoritative learning/control/source/arrival/group facts and actual recipient renewal. Stale route/clue repairs remain builder proposals; no native fix or portability change. |
| ZSQ-BASIN-DISPATCH-CHOICE / LEARNED-BOOKS / SOURCE-RENEWAL / HEARTSTONE-REFUSAL / CLUE-CONSISTENCY / CAVE-ACCESS | Part offers select cash before crafting; cap-one elixir serves four crafts; book reading has no durable milestone; same-kind refusal replaces identity; false-city cave is a deliberate dead end with spider return barrier. | Seven source-guided cards ship. Explain actual selection and sources; builder chooses qualified selection, reading, source/renewal and identity policy. Clue-only repairs remain proposals; preserve existing scarcity, barrier and disabled mobility. No native repair ships. |
| ZSQ-CEOPAST-WOLFSPEED-PROOF / RARE-AVAILABILITY / RENEWAL | Native skull dialogue differs from black-pelt recipe; two initial hair sources, three breaking-key gates, rare staging geometry and reset mode0 affect progression/availability. | Four source-guided cards ship; any pelt counts once with all branch receipts retained. Builder decides proof repair/versioning, measured rare availability and actual renewal before changing native mechanics. Qualify source/gifts, access/arrival and campaign facts; no native repair ships. |
| ZSQ-VENAN-TRUT-SOURCE / REMOTE-ACCESS / REFERRAL / COMPUTED-SERVICE | Eight exact hand-ins coexist with a fixed remote boulder, falling/river routes, pickproof gates, secret floor loot, computed epic teachers, imported group stone and foreign memories/fabrics. | Eight source-guided cards ship. Qualify source/custody, actual target/transition/arrival, committed group/service settlement, explicit cross-zone referral and renewal. Orphan templates/clue/inn-handler overlap remain proposals; native mobility unchanged. |
| ZSQ-MOREGEETH-AVAILABILITY / KEY / PORTAL / CONTAINER / FEE | Moreg carries another recipient’s sword; Gimbatul leaves after crown; pouch grants a real access key; four planar routes use different commands; letter is inside a locked trapped desk; paid crafts require all item roots and fees. | Five source-guided outcomes and two zero-achievement services ship. One-field keyhole fix7b916b887 is actual repair; qualify source/current custody, skill/trap/portal actions, NPC retirement, wallet settlement and reset. Campaign/prose additions remain builder decisions. |
| ZSQ-FIELDS-BETWEEN-RIFT-HOTFIX-REPLACEMENT | Owner confirmed non-takeable rift71030 was an intentional old hotfix for a game-breaking escape mechanic. Original restrictions are restored; legacy delivery source remains unresolved. | Required builder follow-up: preferably distinct inert non-teleport proof while live portal stays fixed, or source-bound interaction/retirement. Define source/gifts, recipe/receipt versioning, PvP restrictions and accounting/renewal qualification; separate reviewed implementation and final-behavior news. |
| ZSQ-FIELDS-BETWEEN-SHARED-SUPPLY / PORTAL / OWNERSHIP / CAMPAIGN | Two consumers share scarce alloy; floor portal is consumed; mother crosses into Scorched Valley; foreign seekers retain foreign ownership; narration promises unconfirmed escape, transformation and reunion. | Seven outcomes ship as source guidance/projection. Portability fix05eeca928 is withdrawn; original restrictions restored by correctiond2a64432d; qualify admitted sources, transfer, ENTER/offer, actor roaming/retirement, ownership, settlement and renewal. Builder decides other prose/controller intent. |
| ZSQ-TUNDRA-AVAILABILITY / FISHING / ROUTES / MIRROR / ALL-INPUTS | Eleadora head retires giver; actors can wander into loading traps. Dock is land and fishmonger lacks a shop; signposted/legacy routes are unusable. Four books and three seafood kinds need ALL; Bom supplies only two seafood kinds. Mirror automatically binds PUSH with asymmetric reset state. | Six stories/one service ship as guidance/projection. Builder chooses availability/shop/route intent before isolated native fixes. Qualify actual catch/source, partial foreign supply, shared switch/keys, paid order, actor retirement and daily renewal. No native Tundra repair ships. |
| ZSQ-MNTCASTL-SUPPLY / BATCH / LOADED-STATE / CAMPAIGN | Exact blue horn has no active producer; granular batch consumes sand and foreign recipe for three outputs. Foreign clothing prices disagree with captions and paid routes remain guarded. Raw hidden bits are discarded by loader; D resets determine actual gate visibility. | Four stories/four services ship as guidance/projection. Builder selects horn/price/visibility intent; qualify indexed input/output lineage, actual SEARCH/UNLOCK/key break and potion use/foreign campaign before new objectives. One-word parapet repair ships separately. |
| ZSQ-IXARKON-FEE / VEIL / BRIDGE / CAMPAIGN | The red cap plus1000platinum preparation is blocked under active accounting; sealed veil chamber has no ordinary ingress and assigned room19890 is absent. Bridge starts open and its one-way unblocking switches cannot retract it. | Two stories/one supporting service ship as guidance/projection. Design atomic fee settlement, guarded intended ingress and accepted random arrival/restoration, controller semantics and explicit rescue/peace endpoints. Direction text alone is repaired separately. |
| ZSQ-TUROLOPOLIS-ALL / GENERATION / FOREIGN / CAMPAIGN | Five different badge keys are required together; minotaur holds the emissary letter but retires after blue ooze, discarding remaining stock. Lothrell loads in Surface while the accepted memorial belongs to71. Rare recipients can disperse into public rooms or a no-exit sink. | Six independent offerings ship as guidance/projection. Add exact key/root custody, source/recipient-generation branches, truthful home-journal links and accepted movement/rescue/purification predicates before campaign or renewal credit. Native repairs remain builder-selected proposals; none ships here. |
| ZSQ-CLOISTER-REFUSAL / SWITCH / TRAP / CAMPAIGN | Tel's recommendation exchange is an unconditional refusal with same-kind replacement; auto-bound no-show switches use SAY/PUSH, and secret exits need subsequent local SEARCH. Egg trap can consume a charge and hurt the player without accepted pickup. Supplied note/ring bypass earlier producers; three competing egg requests retain separate owners. | Seven stories/one rejection ship. Add accepted UID/source/custody, precise switch/search/door/trap generations, indexed replacement/XP settlement and builder-owned ALL/ANY campaigns. Mahr's one-word caption fix/news ships separately; formal admission, source clues, deadline and orphan intent remain proposals. |
| ZSQ-ICECRAG-CONTENT / SOURCE / ACCESS / ACTOR | Cuisine requires missing6551 and mismatched pelt/parchment; exact book/onion/page kinds, 2+2 bottles versus cap-one elven source; speech unlock/open/arrival, GET interruption, NPC rescue and global wolf/death replacement. | Eight stories/two services/one excluded incomplete recipe ship. Builder selects native recipe/source/route repairs; add accepted UID/source/custody, door/control/actor generation and transfer/publication/lifecycle, atomic fee and indexed reward/retirement/reset qualification. No native repair ships here; proposed news remains distinct. |
| ZSQ-RIFT-BATCH / ROLE / ACTOR / BRANCH | Feather cloak needs 15 roots beyond 14 maximum; seven mixed-fee crafts are guarded. Source instances/color counts, secret reveal, token/head disagreement, unspawned dragon alternative and wandering foreign ranger. Two procedural hosts have their own assignment/count/payout authority. |12 outcomes/12 services/3 exclusions ship with 30 optional checks. Extend complete-batch bounds and atomic craft payments; accept source/reveal/dialogue/role/travel/actor and scoped AND/faction/procedural events. Native five-direction fix/news separate; plot/dragon/historical boundary decisions pending. |
| ZSQ-DESOLATE-PAYMENT / TREE / PHASE / TRIAL | Rod + wheel + five platinum is guarded; monkey container holds a separately required chain. Timed random exit changes north route and zone flags. Secret switches clear one side; trial controls lack ordered/defeat terminal; foreign referral returns original letter. | Nine outcomes/two services and optional histories ship. Add atomic wallet/item/reward/recovery, destructive descendant preview, content predicates, accepted targeted controls/arrival, phase generation/episode and scoped trial/foreign AND campaigns. Native south/west direction repair ships separately with news; three absent stock refs and other intent remain pending. |
| ZSQ-ROGUE-ALTERNATIVES / FOREIGN / EFFECT | Two giant recipes each give one outcome; distinct promises/flesh share visible names. Sijona resolves through foreign Balance; sigil uses direct seven-member slot counting, unlike eight-member adapter. Containers are pickable; mounts change actual E/G ownership. | OR outcomes plus separately optional producer histories ship; native owner preserved. Expand causal UID/source/custody, R/rider/owner, successful key/pick/portal/aerial/actual set effects and resetting/retiring episodes. Wording, aliases, item/set intent and historical route are proposed decisions, not shipped repairs. |
| ZSQ-CRAKKARO-BATCH / MOUNT / CONTENT | Seventeen-fur reward is beyond fourteen-root durable support; mounted centaur is omitted by M/F-only sources and post-R E/G belongs to the mount. Four required badges plus unrelated badge look identical; P selects actual same-prototype container. Sculpture expects absent block; exterior room only exists in inactive ocean maps. | Versioned bounded batch/recovery, committed UID/source/custody and R/rider/owner extraction; qualified key/control/arrival/retiring/reset episodes and builder-selected AND finale. Separate future native badge/key/control/route decisions and news proof; none ships here. |
| ZSQ-NEXUS-COMPANION / ACCESS / CONTENT | Traveler requests companion fate after medallion, but no later Q exists. Five companions are placed in a connected load room and can wander. Medallion, NPC-carried same-named emeralds and portals are distinct prerequisites; ring promise contradicts actual prayer book. | Builder designs investigation/report/rescue; add causal source/custody, accepted control/key/arrival and scoped AND episodes with supplied-proof branches. Qualify exposed controls, stock/reset and surviving instances. Separate builder-reviewed clue/alias/dormant-content fixes and news proof; none ships here. |
| ZSQ-SURFACEMINI-CREATION / CONTENT | Flame collar lacks extraction on narrow-room failure; both collars continue after unchecked placement. Wand consumes before unchecked random replacement. Frost clue says Fire rather than Ice; flame room text says ice. | Separate focused original-failing cleanup/text fixes preserving balance/cooldowns/random output; qualify placement/identity/custody and accepted pet/equipment effects. All replacement prototypes exist; normal lesser/greater predicates are identical, so high-roll split remains conditional. No native repair ships. |
| ZSQ-SURFACEMINI-SOURCE / HOST / QUANTITY | Four pockets lack ordinary inbound edges; igloo/three local mobiles have no active sources. Foreign scientist/apparatus share caps; chest has no local contents/key producer. Retiring bases, one sphere, five-kind versus any-one cleansing and guarded fees need exact semantics. | Builder-reviewed routes/dormant content/foreign key flow with separate news proof; expand accepted source/handoff, quantities/mixed payment, surviving-recipient episodes, shared resource reservation, choices/scoped all-stage completion and random transformation lineage. Preserve original foreign ownership and supplied materials. |
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

## The Minizones of the Surface completed source map — October 4, 2026

The [dossier](zone-stories/THE_MINIZONES_OF_THE_SURFACE.md) covers every one of
59 blocks: twenty Q/five QA/thirty M/four MA, with thirty-two addressed and two
ambient families. Read all 119 rooms/68 prose groups/15 headers/24 metadata/all
86 exit families, 25 mobs/42 objects and 145 resets/all 66 families, including the
actual farseer R mount. Three literal wand/collar bindings and complete handlers,
shared command/reset/creation/placement predicates, every boundary and bounded
foreign material producers and laboratory hosts were reviewed. All local native
input/reward and reset references exist; unbound igloo mobiles remain dormant.

Revision one has five stories and sixteen services covering all twenty-five
contracts, thirty-eight contacts and seventy-three optional checks (53 materials,
20 histories). Five equivalent cleansing offers share one service and accept any
one exact kind. The six-essence story requires six distinct kinds; time vials and
lesser healing require five distinct Incarnate materials. Same-named local hermit
clothes do not fit foreign clothing inputs. Exact eight glands, three wand/ore
copies and supplied final proof retain current readiness without mandatory
producer histories. Services award no achievement or daily credit.

Incarnates and psychomia treants generate their items on death; missing O/G/P is
not absence of a source. Lancer also supplies all five Incarnate materials for a
sphere of the void. Retiring foreign adventurer choices supply one Damnation base,
not all three; a single essence delivery supplies one cosmic sphere for one of
six commissions. Raw ultimate healing requires nine five-kind bundles and 2,650
platinum across nine lesser/three greater/one ultimate fees; mixed settlement
remains guarded. Foreign recipe, drug makers, ring producers and laboratory
contracts keep their original ownership. Overhead griffon/key-assembly narration
does not establish an actual mount or key source.

**Pending repairs, none shipped:** native frost clue names Fire, flame room text
says ice and ethereal-wand aliases include earth. Flame collar abandons a created
mobile on narrow-room rejection; both collars ignore placement continuation.
Random wand replacement consumes before checking creation. Current generic
elemental/replacement prototypes exist, and lesser/greater follower checks are
identical, so their hypothetical high-roll split is not a normal gameplay failure.
Plan original-failing procedure tests, balanced preflight/cleanup/identity changes,
correct clue/effect text, accepted equipment/pet journeys and separate fix/news
entries. Four pockets lack ordinary inbound links; final igloo and three local
mobiles lack active sources. Verify builder intent before linking/activating them.
Foreign laboratory cap, chest contents/key narrative and an incomplete alternate
Astral essence reset need bounded source/content decisions and actual journeys.

Expand universal accepted generation→custody→first recovery versus handoff,
transformed-item lineage, quantities/wallet/mixed payment, surviving-recipient
episodes, shared resource reservation, full-stage/choice endpoints, mount/pet
arrival and effect/cooldown/removal. Current multiple bindings mean alternatives,
not an AND campaign. Source/native fixtures cover exact counts/kinds/classification,
supplied/worn/spent proof, foreign ownership, alternative cleansing, support credit
exclusion, replay and cold recovery; synthetic receipts do not qualify those live
custom procedures or fees. Accounting remains mandatory.

Native definitions, revision-two fingerprint, registry and other 68 journals stay
unchanged. Catalog: 69 maps/1663 achievements/1478 potential dailies/2218 rows.
Original order: forty-eight source-comprehensive, 172 pending. Continue with
the Mountain of Peril Peaks (`nexus`). No database, account/server operation,
native repair or merge was performed at this checkpoint.

## The Mountain of Peril Peaks completed source map — October 4, 2026

The [dossier](zone-stories/THE_MOUNTAIN_OF_PERIL_PEAKS.md) covers all 21 blocks:
ten Q and eleven addressed M, with no ambient/default families. Read all 196
rooms/143 prose groups/41 headers/42 metadata/all 236 exit families, 162 mobiles,
85 objects/two shops and 297 resets/all 236 families. No local literal procedure
is bound; automatic switches/shared teleport, key/container/dispatch/wandering,
fall/current and both reciprocal Mountain Tracks boundaries were reviewed.
All native input/reward and reset references resolve; no foreign material is required.

Revision one has ten outcomes, twenty-eight contacts and sixteen optional checks
(thirteen current materials, three producer histories). Three two-stage errands
explain silver/stud/Roxon, arm/parchment/Gooran and head/eye/traveler. The spirit's
three distinct scales, two distinct tentacles and draco eyes, plus separate bear
fur, remain independent outcomes. Supplied proof does not require or invent
personal producer history; intermediate receipts cannot restore consumed objects.

The medallion is a persistent key. Gardener/library emeralds have different exact
control targets despite shared names and need exposed room custody; another
NPC's carried object is not dispatched. Looking at twilight reaches the observatory,
touching the sphere returns, and black-gate/tent travel uses enter. Unused cave/
oval portals, plaque and small iron key do not establish broken existing routes.
The five displaced companions have active connected placements and ordinary
wandering, not absent sources. The traveler nevertheless has no companion-fate
report/rescue contract. This needs builder design plus qualified encounter,
survival/ownership and actual report/effect endpoints, not fabricated completion.

**Pending decisions, none repaired:** native dialogue/parchment promise a ring,
but Gooran actually gives a prayer book. Builder must choose intended reward or
coherent clues. Preserve old aliases while improving bag/stud/kraken/key command
discovery if approved. Confirm dormant portal command/placement/riddle/key intent
before enabling content. Actual repairs need separate clear fix/news entries and
original-failing command/source/access tests; stock caps/wandering/custody are
qualification limits rather than automatically classified bugs.

Expand causal item source versus player handoff; accepted emerald control/key
use/opening/arrival and reset generation; scoped AND campaigns with supplied-proof
branches; companion fate/escort/report and truthful healing/regeneration/kill
effects. Current completion arrays are OR alternatives. Do not add inert builder
fields before implementing parser, events, persistence and replay semantics.

Active, ready accounting remains mandatory. All 2668 native definitions, source
fingerprint, content revision two, registry and other 69 journals stay unchanged.
Catalog: 70 maps/1663 achievements/1478 potential dailies/2218 rows. Original
order: forty-nine source-comprehensive, 171 pending. Continue with Crakkaros'
Liar (`crakkaro`). No database, account/server operation, migration, native repair
or merge occurs; gameplay qualification remains open.

## Crakkaros' Liar completed source map — October 4, 2026

The [dossier](zone-stories/CRAKKAROS_LIAR.md) covers every twenty native blocks:
eleven QA/nine addressed MA, with no ambient/default families. Read all 372 rooms,
44 prose groups/23 headers/28 metadata/all 225 exit families, 37 mobiles/68 objects
and 501 resets/all 199 families. No literal/proclib procedure or shop is local.
Shared durable offering/reward, mount/follower reset, P container selection,
automatic switch, key/door/dispatch/wandering and boundary assembly were reviewed.
All native prototypes/local reset references resolve; no foreign material is needed.

Revision one has six stories/five services, twenty-nine contacts and twenty-seven
optional checks: twenty-four materials and three earlier-exchange histories.
Centaur tail, five ogre kinds → shield → bracer → ring → earring and four distinct
badges → ice key preserve separate receipts and supplied-proof branches. Four
other part commissions and fur work are services. Nine native daily shapes remain;
four commissions cease to inflate authored daily progress, leaving five story
candidates. The departing woman is story only; reset-mode-zero still permits
candidate contracts with surviving recipients, without guaranteeing fresh supply.

**New precise blockers:** seventeen furs exceed the fourteen-root durable limit
and are refused before accepted transfer/reward; larger batches require versioned
continuation and recovery compatibility, atomic currency settlement and replay/
crash tests. M/F-only source extraction omits the R-loaded centaur and attributes
post-R saddle/quiver to the elf; runtime E/G actually targets the mount. Same-kind
P selection uses the live object list, so a bookshelf badge has no permanent
reset-room guarantee. Four required badge kinds and unrelated corpse badge look
identical; independent source labels improve journal clarity without native edits.

**Pending builder decisions, no native repair ships:** sculpture envy control
expects EX_BLOCKED, but the secret locked/closed reset has no block; key access
is a distinct existing route. Ice key lacks key alias, northeast reverse key is
zero, and exterior target is absent from active static AREA rooms, appearing in
inactive ocean maps. Qualify actual generated world/entry/return before selecting
a route repair. Unused heart/key/level-potion and extra badge/token need intended
content decisions. No lover rescue, burial or dragon/campaign terminal exists.

Expand committed source/container/mount UID custody versus handoff, accepted
key/control/opening/arrival/reset-generation and retiring-recipient episodes;
builder-defined actual effects and scoped AND campaigns retain supplied proofs.
Existing completion arrays are OR alternatives. Do not add inert schema fields,
make every word an award or complete a dragon finale from the badge receipt.

Verification covers exact contracts/source relationships, actual C++ distinct
badges/ogre parts, supplied intermediates, guarded-fur inventory visibility,
spent/worn proof, no read-side mutation/credit, services, replay and cold recovery;
catalog/daily report, accounting gates/tracking, maintained server build, formatting,
whitespace and local/source links. The catalog is 71 maps/1658 achievements/1474
potential dailies/2218 rows. All 2668 native definitions, revision two, fingerprint,
registry and seventy prior maps are unchanged. Live access, mount movement,
source hunting/stock, recipient retirement, traps, larger payment and finale stay
unqualified. The queue is 50/220 source-comprehensive, 170 pending; Rogue Plains
(`roguerai`) is next. No database/account/server operation, generated world edit,
activation or merge occurred; this checkpoint is not full-roadmap completion.

## Rogue Plains completed source map — October 4, 2026

The [dossier](zone-stories/ROGUE_PLAINS.md) covers every twelve native blocks:
nine Q/three addressed M, all 158 rooms/fifty prose/nineteen headers/nineteen
metadata/109 exit families, 58 mobiles/57 objects and 223 resets/127 families.
The one literal local Master sigil procedure and separate adapter were reviewed,
with shared submission/reward, source/reset ownership, container/door/picking,
portal/aerial movement and active versus historical boundary assembly.

Revision one has seven outcomes, twenty-four contacts and sixteen optional checks
(thirteen current materials, three producer histories). Each giant accepts either
silence-potion kind for one promise; the mediator needs one of each promise.
Four identical flesh kinds cannot substitute; two hides are the orc's reward.
Boots produce a partial soul for the reaper's two-input exchange. A supplied soul
or promise pair remains valid without personal producer history. Optional key
guidance does not prevent a supplied medal or successful lockpick branch. Native
nine daily shapes remain; OR grouping leaves seven authored candidates.

**Precise expansions:** Sijona's three tokens lead to Balance on Myrabolus' pier,
an actual foreign-owned contract rather than an invented local all-kills terminal.
R creates steeds and changes post-R E/G ownership; followers versus ordinary
connected staging sources need actual custody/location qualification. Two locked
containers are not pickproof: the obelisk's hardpick bit is not pickproof sixteen.
Secret/closed routes are neither locked nor blocked; same-name portals have
different destinations. Air-plane entry allows levitation, horizontal departure
requires flight, and mounted movement checks the mount. Record actual success
rather than deriving access from a key, prose or a receipt.

Master sigil's direct legacy procedure counts matching worn slots against seven
members, caps at six and grants effects beginning at two pieces. The separate
adapter table lists an eighth member/deduplicates, but direct assignment governs
the sigil. Add stable set identity/membership and committed equip/actual effect/
cleanup/restart events only after builder intent and executable lifecycle proof.
No full-set achievement is inferred from owning or equipping one sigil.

The staff-named reward's wand type supports a room-corpse spell; the channeling
staff path rejects that object-only target, so do not blindly convert its type.
The spell requires a same-room corpse of level at least 46, valid caster/form
and control capacity; dragon-scale consumption is commented out. Device activation,
accepted durable NPC-room/player-corpse transformation, hostile versus controlled
follower and pet recovery are separate causal stages. Future effect objectives
must subscribe to accepted outcomes rather than a charge or existing reset mob.

**Pending native findings, no native repair ships:** sword/blade prose versus hammer,
identical flesh/promises, flesh typed as cure-light potion, staff-named create-
dracolich wand, absent trapper/climb/quiet/ritual terminals and Master membership
intent. The northern target appears only in historical data; deployed assembly
needs qualification. Two existing reciprocal surface approaches remain valid
static routes. Ordinary staging mobs are loaded and can wander, not missing bosses.
Future actual repairs require separate fix commits and prominent PR/news proof.

Verification covers exact contracts/alternatives/sources, foreign owner, container
flags, mount ownership and procedure/table distinction; actual C++ wrong duplicate
kinds, either potion, supplied histories, spent/worn proof, optional key, encounter
visibility, no read-side credit/mutation, replay and cold recovery. Catalog/daily
report, accounting gates/tracking, maintained build, formatting, whitespace and
local/source-line links are checked. All 2668 native definitions, revision two,
fingerprint, registry and seventy-one prior journals stay unchanged. Catalog:
72 maps/1656 achievements/1472 potential dailies/2216 rows. Live acquisition,
wandering patrol, access/return, orc renewal, foreign delivery, spell and set
lifecycle remain unqualified. Queue: 51/220 source-comprehensive, 169 pending;
Desolate (`desolate`) is next. No database/account/server operation, native repair,
generated-world edit, activation or merge occurred; the full roadmap remains active.

## Desolate completed source map — October 4, 2026

The [dossier](zone-stories/DESOLATE.md) covers all twelve native blocks/eleven
Q/one addressed M, 168 rooms/143 prose/nineteen headers/twenty-two metadata/135
exit families, 114 mobiles/92 objects/three shops and 400 resets/289 families.
Direct Master/computed teacher, shared item-only quest acceptance/tree extraction,
mixed-fee guards, actual switches/portals/falls/mount ownership, timed invasion
route and bounded foreign letter/referral/source/boundary evidence were reviewed.

Revision one maps nine stories/two services, twenty-seven contacts and sixteen
optional checks (twelve materials, four earlier receipts). Tankard filling is a
supply service; the halfling is its separate story. The monkey container holds
the minotaur's chain: extract that chain before the destructive hand-in. Rod +
broken wheel + five platinum is guarded; supplied repaired wheel still fits the
retiring driver. Hand + storm badge differs from the delegate's armageddon badge.
The original Myrabolus letter/referral remains foreign-owned optional history.
Native nine candidates stay unchanged; eight authored story candidates remain.

**Implemented native repair, separate commit and news:**
`b1ff082bc03ae579aceec97b5d3ebd878bf2eea7`, `fix: correct Desolate Master trial
button directions`. Endurance look text formerly said east while its button/exit
is south; Courage formerly said east while its button/exit is west. Two room
descriptions now agree. Focused source test fails on original prose and passes
after repair, comparing actual control values/placement/blocked exits/resets.
No rules, switches, guardians or rewards changed. **News:** “Desolate's Master
trial now gives the correct button directions in the Tests of Endurance and
Courage.” Live look/control/return remains unqualified.

**Expanded plan:** accepted atomic wallet + item-tree + reward/receipt/recipient
transactions with rejection/replay/reconnect/crash proof; coin-only commerce;
preview/reject-or-consent policy for valuable descendants of destructive roots;
content predicates for actual drink fullness rather than vnum; successful targeted
secret control/reveal/open/pass and return; ordered scoped guardian/trial attempts;
actual set equip/effect/cleanup; world-control route generation, normal/invaded
owner and causal episode lifecycle; foreign scoped AND campaigns without duplicate
credit. Static completion lists remain OR and current material checks remain
loose-carried kind checks. Do not add unsupported fields or imply live endpoints.

The random-exit object has twenty-percent reset admission, then a scheduled
hundred-percent callback when valid. It reroutes north into Desolate Under Fire,
rewrites target south and changes closed flags; current static discovery does
not capture that phase. Desolate Under Fire retains its priority-92 owner review.
Master controls do not require recorded kills; Speed rogues are in an upstairs
side room, final north returns to start. Joust/betting has no active local terminal.

**Pending native findings:** trinket shop/G stock 6070/6109/6110 is absent from
static prototypes; unresolved G commands are disabled. Confirm intended restored
or replacement stock before fixing both sources and validating commerce. This
does not establish a boot crash or broken delivery recipe. Closed-drawer/open
prose, fullness, container affordances, collar spelling and campaign endings need
builder decisions. Proposed payment support is not a shipped quest repair.

Verification: exact source/classification/fees/ownership/controls/phase and missing
stock; actual C++ encounter/wrong badges/nested-worn-spent proof/supplied branches,
independent receipts/no read mutation/replay/cold recovery; catalog/daily report,
tracking/accounting gates, maintained build, formatting/whitespace and links.
Catalog: 73 maps/1654 achievements/1471 potential dailies/2216 rows. All 2668
native definitions/revision two/fingerprint/registry and seventy-two prior
journals remain unchanged. Live acquisition/hand-ins, mixed payment, wandering,
trial/phase/fall/return/set and renewal remain unqualified. 52/220 source maps
complete, 168 pending; Rift Valley Jungle is next. No DB/account/server operation,
generated-world edit, activation or merge; the full roadmap remains active.

## Rift Valley Jungle completed source map — October 4, 2026

The [dossier](zone-stories/RFTJNGLE.md) covers all 65 blocks/28 Q/37 addressed M,
470 rooms/325 prose/34 headers/69 metadata/267 exit families, 220 mobs/220
objects/eight shops and 828 resets/499 families. Both procedural hosts, 12 computed
teachers, secret searches, exact source instances, portal pairs, current/fall
hazards and active/historical foreign boundaries were reviewed.

Twelve outcomes/twelve services/three excluded referral/refusals classify every
native exchange. Forty-nine contacts and 30 optional checks explain egg counts,
distinct skins/colors, actual faction tokens, staff recipient alternatives,
elemental proofs and ancient sword recovery. Seventeen native daily shapes remain;
eleven authored story candidates remain. Fifteen-feather acceptance exceeds 14
durable roots; seven mixed-fee crafts remain guarded. Source-comprehensive mapping
does not claim those guarded native transactions or actor effects are implemented.

Expand full-batch request/continuation/tree/serialization/recovery bounds together,
atomic item/wallet/reward/receipt/recipient craft settlement, committed source/
instance/owner/reveal provenance, role-sensitive accepted dialogue, exact travel/
arrival/return and actual actor transformation/relocation/ghost lifecycle. A scoped
four-element AND campaign and optional faction policy need explicit builder design;
current independent native receipts remain valid. Procedural assignments need
actor/start/target/type/count/payout/abandonment episode identity and foreign owner,
without duplicate local static daily credit. Mercenary payout guard stays visible.

**Implemented native repair:** `f5d5b2a5ccfbb9fccc8f5570079dd3394c74ed54`,
`fix: correct five Moonhollow route descriptions`. Wall cave north→south;
Moonstone return east→west; armory entrance south→east; forge return east→south;
narrow-branch home west→north. Actual reciprocal exits prove the directions.
Original focused test fails; repaired test passes; only five scoped native words
changed. **News:** “Moonhollow's wall, guard office, armory, forge and canopy-home directions now match their actual exits.” Live LOOK/traversal remains unqualified.
Keep this separate from journal guidance and pending proposals in PR/news.

**Pending native findings:** dialogue asks for heads but actual amulet/rod recipes
accept tokens; real mangled head is refused. Engineer cave clue conflicts with
plans in elven guard quarters. Alligator inputs yield azurian-named armor. Crystal
dragon alternative has no active reset/transition; ordinary woodcarver route
exists. Decorative iron amulet/axe has no opening/code-word endpoint. 80460S
targets 228371 found only in inactive historical surface files; valid modern
surface and three Woodseer entrances remain. Confirm builder intent before each
separate native fix and qualify its actual interaction; do not call all quests broken.

Source and actual C++ journal qualification cover exact counts/kinds, worn versus
loose proofs, optional/non-credit histories, service exclusion, one staff outcome,
independent faction/elemental receipts, read-only render/replay/cold recovery and
accounting gates. Catalog 74 maps/1638 achievements/1465 potential dailies/2212
rows; 2668 native definitions/revision 2/fingerprint/registry/prior 73 maps unchanged.
Live source recovery, searches, hand-ins, guarded batch/payments, roaming, portal
survival/return, actor effects and renewal remain unqualified. 53/220 complete,
167 pending; The Transparent Tower is next. No DB/account/server operation,
generated area output, accounting activation or merge. Full roadmap stays active.

## Transparent Tower completed source map — October 4, 2026

The [comprehensive dossier](zone-stories/TRNSPTOW.md) reviews all thirty native
blocks (four Q/twenty-six M), 100 rooms, forty mobiles, seventy-five objects,
208 resets/144 families and five literal special bindings, with full custom
procedures, shared execution and bounded foreign sources/boundaries. The
[journal](../../areas/story/trnsptow.story.json) adds four independent outcomes,
fourteen contacts and eleven optional checks (eight material/count, three
personal receipts). One native/story daily candidate remains unchanged.

Three companions return the scepter kind and give identical token kinds; the
librarian accepts any three matching tokens plus the scepter. Separate optional
receipts explain a full tour without falsely requiring all three sources or
turning an OR list into an ALL campaign. Supplied proofs fit; prior receipts
do not restore consumed items. The scepter is inside the locked marble desk,
whose opening triggers a three-charge room-wide acid trap. Its ordinary route
uses three fragile keys; successful unlock destroys a normal key, so current
possession cannot substitute for accepted access history. All actual portal
commands, spoken-word gates and return routes are explained.

Expand accepted UID/source/transfer and key destruction/access evidence,
speech/controller/arrival episodes, builder-defined ALL/actor/escape closure,
and reuse the existing durable epic-touch result for the closet's imported
rune. Prototype ownership does not make foreign globe/mace sources Tower
rewards. Custom Aceralde pulse registration, illusion-themed direction/prose
conflicts, ordinary stair return versus closet-only narrative, unplaced
trash stone bindings and orphan desk/sign/portal need intent/qualification.
These remain **pending findings**; **no native Tower repair ships**. Earlier
shipped repairs and news handoff remain separate and prominent.

Focused source and actual C++ journeys cover exact tokens/loose scepter,
supplied final acceptance, independent companion outcomes, optional history,
read-only rendering, replay and cold recovery. Live access/traps/boss/retirement,
epic group settlement, portals and player/account persistence remain unqualified.
Catalog: 75 maps, 1638 achievements, 1465 potential daily units, 2212 rows,
2668 native definitions/content revision two. Original order: 54/220 source maps
complete, 166 pending; The Tempest Court is next. Active, ready accounting
remains mandatory. No DB/account/server operation, deployment or merge.

The shared epic absorption loop reads/advances after extraction releases its
object and clears its content link. Plan a separate shared epic lifecycle fix
with actual-procedure original-fails/repaired-passes consecutive/overlapping
victim, room/carried, identity, single-destruction and accounting/recovery tests.
No live crash or player loss was reproduced. The source finding remains pending
and must not appear as a shipped repair/news claim.

## Tempest Court completed source map — October 4, 2026

The [comprehensive dossier](zone-stories/TEMPEST_COURT.md) reviews all 26 native
blocks (eight Q/seventeen M/one addressed MA), 200 rooms, 52 mobiles, 63 objects,
307 resets/135 families, the active wind dagger, relevant shared execution and
bounded foreign sources/consumers. The [journal](../../areas/story/airp.story.json)
adds seven outcomes/eight contracts, 21 contacts and 28 optional checks
(25 carried, three receipts). Equivalent Al'Hajib recipients share one outcome;
five units retain native daily shape. Rescue and Cloudseeker retire recipients
under reset mode zero and remain story-only. All 2668 native definitions and
revision-two fingerprint stay unchanged.

Progression explains the actual smoke-key cell/rescue/medallion route, three
different lord essences for the palace key, Ixteal heart, five distinct Duke
essences and the consumed fragment's two independent later bundles. Cloudseeker
needs nine exact loose roots, Fearfrost four; supplied proofs fit without
personal foreign journeys, kills or optional producer history. F/R source
ownership, flying/mount/perception restrictions, actual ENTER portals and
key retention/breakage are documented. Earlier receipts do not restore stock.
The MA family is addressed dialogue with a room audience; the apostrophe topic
is preserved in prose despite the current structured-token limitation.

Expand accepted reset generation/current supply and NPC/container custody,
first recovery/transfer, unlock/arrival/perception, material allocation/branch
attempts, scoped ALL/actor restoration and committed epic-touch integration.
Active accounting intentionally refuses legacy reset item issuance without
owned generation. Nominal rare M chances do not guarantee normal unforced
renewal. Preserve the guard and qualify actual admission before daily assignment.
Promised eyepiece upgrades, empty cloud portal, missing lightning movement bit,
sealed rare staging, orphan cells/branches and unplaced divine/attack prototypes
have balanced builder-intent/repair plans; they are **pending findings**.

**Actual native repair ships separately:** [dc586e34c](https://github.com/Community-Duris/Duris/commit/dc586e34c75fea9dc4016416cc95bd1637499e63)
corrects exactly two direction words: Si'Ciltron's return door now says east;
the prison's two cells are described north and south. Each original clue fails
the focused source regression, repaired clues and exact-byte scope pass.
Live LOOK/traversal remains unqualified. **Player news:** “The Tempest Court's
war-chamber return door and prison-cell directions now match their actual exits.”
This fix is distinct from journal authoring and all pending repair proposals.

Focused source and actual C++ fixtures cover exact kinds/counts, loose versus
worn proof, supplied acceptance, independent fragment consumers/equivalent
recipients, optional history, read-only rendering, replay and cold recovery.
Live access/source/combat/rare retirement/epic/persistence remain unqualified.
Catalog: 76 maps/1637 achievement units/1464 potential dailies/2211 rows;
grouping Al'Hajib removes one duplicate unit. Original order: 55/220 complete,
165 pending; The Caverns of Armageddon is next. Active, ready accounting is
mandatory. No migration, DB/account/server operation, deployment or merge.

## Caverns of Armageddon completed source map — October 4, 2026

The [comprehensive dossier](zone-stories/CAVERNS_OF_ARMAGEDDON.md) reviews all
33 native blocks (eighteen Q/fifteen M), 149 rooms, 94 mobiles, 69 objects,
393 resets/187 families, the imported epic rune and spell pool, relevant shared
execution and bounded foreign producers/consumers. The [journal](../../areas/story/hunt.story.json)
adds eighteen independent outcomes, 40 contacts and 27 optional checks
(26 carried, one producer receipt). All eighteen retain native daily shape;
classification does not certify current supply, difficulty or renewal. Native
definitions/fingerprint/revision two/registry and prior 76 maps stay unchanged.

Progression covers fourteen local heads and foreign Scralack; two different
tags from closed soldier container objects; four distinct creature parts from
the archangel, behemoth, beholder and Krazzi's following fire elemental; and
two consumed amulets for the Dragon Queen. Blicatch produces the pair; Alexis
in Myrabolus can also produce the hazy blue kind. Optional producer history
does not restore spent material or become a hidden acceptance prerequisite.
Roland's longbow/four-arrow reward is one receipt. Treasure chests, lost monkey
and Abbadon's heart have foreign recipients, without duplicate local credit.

Trace the elite orc's shaft key, blood-basin chipped key and guard captain's
stone key; distinguish the latter's actual east lock from key metadata on open
steps. Secret waterfall/grate, Blicatch's blocked return, ENTER rift and static
one-way rescue/disembark routes are actual access clues. Wings have a one-charge
GET/PUT sleep trap that rejects the triggering pickup. Abbadon's cosmos key
breaks on normal unlock and serves two pickproof locks; supply/shared-door
policy must be qualified before promising both treasures from one key.

Expand admitted reset generation/current stock, UID/corpse/container custody,
first recovery versus transfer, accepted access/key-break/arrival and trap
mutation/recovery events, branch allocation, actor/escort/scoped ALL and existing
committed epic/effect integration. Preserve active-accounting reset guards.
Roland has a recipe but no declared active world spawn: choose builder placement
and lifecycle before a separate repair/daily admission. Prisoner departure
narration, mixed-company tags, nonreciprocal surface edge, key lifetime and
spell-pool target routing have balanced intent/qualification plans. The unused
cave viper alone is not an unfinished quest. **No native repair ships here.**

Keep actual repairs in separate fix commits and prominent PR/news records,
with zone/interaction/trigger/before-after/proof/live limits and news wording.
Earlier Tempest, Moonhollow, Desolate, Shadow of Sin and Halfcut fixes remain
visible. Journal additions and these pending proposals are not shipped repairs.

Focused source and actual C++ fixtures cover exact bundles, loose versus worn
proof, optional route/history checks, supplied amulets, consumed pairs versus
producer receipts, independent outcomes, read-only rendering, replay and cold
recovery. Live source/perception/container/trap/key/portal/combat/retirement,
group epic and played persistence remain unqualified. Catalog: 77 maps/1637
achievement units/1464 potential dailies/2211 rows. Original order: 56/220
complete, 164 pending; Tribal Forest (`tribal`) next. Active, ready accounting
is mandatory. No DB/account/server operation, migration, deployment or merge.

## Tribal Forest completed source map — October 4, 2026

The [comprehensive dossier](zone-stories/TRIBAL_FOREST.md) reviews all 25
native blocks (thirteen M/ten Q/two MA), 175 rooms, 67 mobiles, 103 objects,
377 resets/234 families, the real apprentice shop, fixed amethyst orb,
imported spell pool, shared teacher/switch/portal/trap execution and bounded
global sources/consumers. The [journal](../../areas/story/tribal.story.json)
adds nine outcomes, one explicit refusal exclusion, seventeen contacts and
21 optional checks (twenty carried, one hunter receipt). Both MA families are
addressed dialogue; all native aliases remain. Definitions/fingerprint/revision
two/registry and prior 77 maps stay unchanged.

Progression includes larger worm meat → hunter grain → bluebird staff; separate
small meat → bird nest; bluish key/shoes/skirt/blanket → wife's departure;
beautiful comb → brooch; spotted deerskin → amulet; five different ingredients
→ crystal; five different scattered underground parts → Xazapath bow/quiver;
and ancient-tree egg → Queen Spider ring. Supplied matching material is accepted;
producer history neither becomes a hidden prerequisite nor restores spent grain.
The larger-meat bird refusal reissues the same kind without a meaningful reward
and is excluded, removing one false achievement/row, without removing a daily.

The kitchen key can break on normal unlock and is also an offering input.
Lookout holds the crystal, the runed bone is inside an unlocked container,
the following devil has the horn, and different following trees have root/egg.
PUSH/STOMP switches clear blocking but can leave closed/secret state. Mirror
STARE is travel; orb STARE/GLANCE is viewing, RUB selects a runtime room index,
and TOUCH can travel or burn and reduce HP to zero. Loading rooms and nominal
source chance/caps do not certify current reachable NPCs or admitted stock.

Expand committed UID/root/container/NPC ownership, first recovery versus handoff,
same-kind replacement, admitted M/F/O/P/G/E sources and retiring/reset episodes.
Add exact control/unlock/key-break/door state and actual actor arrival/HP effects;
separate actor/escort/scoped campaign and builder-selected racial predicates.
Preserve supplied independent receipts, existing settlement and accounting guards.
Trap codes 9/10 lack handler payloads; select supported effects before a separate
repair. Other leaf/tunnel/redbird wording, grove/escort lore and orb/spell-pool
target qualification remain balanced pending findings. The apprentice's valid
tilde shop record was checked directly; do not call that real shop a broken quest.

**Actual native repair:** separate [fix f8090481d](https://github.com/Community-Duris/Duris/commit/f8090481df837bb4e1a675c901488f13090657d1)
changes exactly three direction words in room 42204 west, 42231 west and 42261
south descriptions. All three original clues fail the focused regression; repaired
clues, reciprocal routes and exact native-byte scope pass. Live LOOK/traversal is
unqualified. News: “Tribal Forest's two western forest exits and the southern
village exit now describe their actual directions.” Journal additions and the
other proposals are separate from this shipped fix. Earlier repair/news records
remain visible and preserved.

Focused source and actual C++ fixtures cover exact kinds, optional hunter history,
supplied crystal acceptance without first commission, loose versus worn materials,
duplicate limbs, nest versus egg, spent grain versus producer history, one paired
reward receipt, refusal exclusion, read-only rendering, replay and cold recovery.
Live generation/encounter/GET/source/transfer, access/trap/travel/HP/combat,
retirement/renewal and played persistence remain unqualified. Catalog: 78 maps/
1636 achievements/1464 potential dailies/2210 rows. Original order: 57/220
complete, 163 pending; The Ancient Halls of Ironstar (`lornecro`) next. Active,
ready accounting is mandatory. No DB/account/server operation, migration,
deployment or merge.

## The Ancient Halls of Ironstar completed source map — October 4, 2026

The [comprehensive dossier](zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md)
reviews all 22 blocks (fifteen M/seven Q), 100 rooms, 34 mobiles, 66 objects,
198 resets/143 families, exact NPC/container ownership, bounded global
sources/consumers/boundary and shared quest/reset/door/magic-word/current
execution. No local shop, literal local special or ACT_TEACHER establishes an
extra terminal. Imported ore is tradeskill stock; the epic node remains a
separate shared interaction with its previously recorded qualification concern.

The [journal](../../areas/story/lornecro.story.json) adds four stories and
three equipment services, fifteen contacts and fourteen optional checks
(twelve carried, two producer receipts). Office wedding ring → Larra’s ring
→ Robert/Soulcatcher and king’s crown → axe → paid vault key remain independent
exchanges; supplied material fits without producer history. Dragonbone mail,
dagger and hammer preserve native receipts without zone achievement/daily
inflation. All four coin recipes retain the durable offering guard.

Tomb and grate have configured keys, but their D resets open/unlock them.
The room loader reads low-two-bit door kind rather than an initial closed/locked
state. Avoid a false mandatory mold/tomb cycle or password prerequisite.
The three real forward vault locks use third-baby-dragon key, gravel key and
paid Dralor key; reverse state differs. Saying Datherlion can unlock an actually
locked grate and clear secret state, but does not open it. The spellbook/globe
prose does not implement portal travel or a learned-topic achievement.

Expand atomic owned item-plus-normalized-fee allocation, effective door/reset
and shared episode state, successful unlock/open/reveal and arrival, exact
UID/root/container/NPC provenance and competing reward-to-offering lineage.
Robert’s 75-percent M and D1, apprentice’s 80-percent M, and cap-one material
need admitted placement/generation/renewal qualification. First recovery, supplied
proof, surviving combat and accepted delivery remain distinct. Reunion, clan
rebuilding, dwarf-friend title, teaching, letter and globe terminals need explicit
builder design before credit. River bend metadata, copied dagger/scroll/weapon
descriptions and the shared epic-node concern remain balanced pending proposals.

**Actual native repair:** separate [fix b07b560cc](https://github.com/Community-Duris/Duris/commit/b07b560ccf2d3b7bbcb61227cfa5423dd6fa6924)
changes exactly two exit-description words: room 138903 west now says west,
and room 138957 south now says south. Both color-normalized original clues
fail the focused regression; repaired clues, reciprocal routes and exact native
bytes pass. Live LOOK/traversal remains unqualified. News: “Ironstar's western
valley exit and southern Fairlocke exit now describe their actual directions.”
Journal additions and pending proposals are separate; earlier native repair/news
records remain visible and preserved.

Focused source and actual C++ fixtures cover exact independent inputs/fees,
wrong mold/intact blade/worn equipment, paid guard guidance, supplied acceptance,
history versus spent material, independent service receipts, replay and cold
recovery. Live generation, access/current/traps, original source/handoff,
coin settlement, actor retirement/renewal and played persistence remain pending.
Catalog: 79 maps/1633 achievements/1464 potential dailies/2210 rows. The three
equipment commissions remove three generic achievement units; all seven native
definitions/rows and daily candidates remain. Native fingerprint/revision two/
registry and prior 78 maps stay unchanged. Original queue: 58/220 complete,
162 pending; Plane of Fire, Brass (`brass`) next. Active, ready accounting
is mandatory. No DB/account/server operation, migration, deployment or merge.

## Plane of Fire, Brass completed source map — October 4, 2026

The [comprehensive dossier](zone-stories/PLANE_OF_FIRE_BRASS.md) reviews all
20 native blocks (12 M/1 MA/6 Q/1 QA), 357 rooms, 147 mobiles, 170 objects,
eighteen valid new-format shops and 779 resets/452 families; bounded global
sources/consumers, reciprocal Plane of Fire boundary and imported/shared
execution. No local teacher or literal local mobile procedure adds a quest
terminal. M/F/R supply, equipment replacement, effective doors and actual
assigned combat/hide/inn behavior remain distinct from native receipt credit.

The [journal](../../areas/story/brass.story.json) adds four stories, one
equipment service and two exclusions, with 25 contacts/eighteen optional
carried checks. Herl’s blood request yields currency; the collector wants
four exact collectibles, including two item-kind coins rather than wallet
payment. Yodono consumes three heads and awards two same-kind vials under
one outcome. The spy consumes six distinct heads, sharing two with Yodono,
and departs. The full two-scale/7500-platinum bracer is a guarded service;
the smaller scale/fee return is a refusal. The empty, unplaced dying-djinn
recipe has no verified rescue/second-task endpoint and loses named credit.

Entrance golem 25400 guards north; death of 25401 separately clears EX_BLOCKED
through its foreign quest-control family. Opening and arrival are separate.
Palace key issuance is not access; effective reset doors and asymmetric
reverse routes prevent false self-key cycles. Two same-name blue-fire keys
have different kinds. The rare outward-only room’s non-sentinel actors may
wander into the palace, while spy M10 and pyrohydra M60 need admitted initial/
forced presence and supply qualification. Fire-ward labels do not certify
safe travel, and external adventure notes need destination verification.

Expand accepted actor/barrier/access episodes and provenance, one-use UID
allocation across requests, duplicate reward identity/recovery, exact I versus
C semantics, atomic mixed-fee service settlement and rare hidden/wandering
recipient renewal. Builder intent must define dying-djinn placement/outcome,
Yodono’s second task and missing ambient interval. Copied body/shop/direction
text and road topology remain balanced proposals. No forced spawns, invented
healing, fee bypass or new unsupported objective credit ships.

**Actual native repair:** separate [fix d18758098](https://github.com/Community-Duris/Duris/commit/d18758098083b28463279c130a3e7ec24bb48b24)
changes one word in room 139017’s east exit clue from west to east, matching
D1 to 139016 and reciprocal D3. Original color-normalized clue fails; repaired
source and exact native bytes pass. Live LOOK/traversal remains unqualified.
News: “Brass's eastern Imix Avenue exit now describes its actual direction.”
Journal/classification changes and pending proposals are separate. All earlier
native repair/news entries remain.

Focused source and actual C++ fixtures cover collectible coin versus C reward/
fee, quantities, competing heads, worn versus loose preparation, guarded service
guidance, exclusions, replay and cold recovery. Live generation, perception,
wandering, source/handoff, barriers/access/heat, fee settlement, retirement,
renewal and played persistence remain pending. Catalog: 80 maps/1630
achievement units/1463 potential dailies/2208 rows; two excluded generic
outcomes and one equipment achievement are removed, with one fewer potential
daily from the empty djinn. Native definitions/fingerprint/revision two/registry
and prior 79 maps remain. Original queue: 59/220 complete, 161 pending; The
Tower of Darkness (`lortower`) next. Active, ready accounting remains
mandatory. No DB/account/server operation, migration, deployment or merge.

## Tower of Darkness completed source map — October 4, 2026

The [comprehensive dossier](zone-stories/TOWER_OF_DARKNESS.md) reviews all
32 native blocks (20 M/12 Q), 142 rooms, 170 mobiles, 146 objects, one valid
shop and 580 resets/482 families, all exit text/passwords, exact sources,
bounded global consumers/entry and relevant shared execution. Seven Q/eleven M
are Tower-owned; five Q/nine M are physically local but Braddistock-owned.
The [journal](../../areas/story/lortower.story.json) covers the seven owned
contracts as six stories, with 27 contacts/twelve optional checks. The two
giant key alternatives form one OR outcome; the cash branch stays guarded.
Amelia’s release/locket and Dorthan’s five-sword/locket bundle remain independent
accepted receipts. Three planar keys yield one stasis key. Questions, reading,
source, first recovery, keys, portal use and surviving arrival earn no invented
credit; supplied exact offerings do not require personal earlier history.

The physical campaign also includes Azlion’s staff/redemption sword, Joseph’s
fate, five Star Stone pieces/Star Key, and Isabia’s bone key/ring/Danthas delivery.
Current catalog/runtime giver ranges credit these to Braddistock and forbid
borrowing bindings into Tower. Tower discovery/encounters do not discover that
owner. Plan explicit physical affiliation/referral and cross-zone campaign
references independently of immutable receipt ownership, with discovery policy,
versioned upgrade and historical compatibility tests. No owner migration ships. Azlion/Darrin D1 are catalog-repeatable because
Braddistock reset mode two is used, despite physical Tower mode zero. Plan
physical spawn/retirement/renewal episodes independently of credit-owner mode;
static potential-daily metadata is not actual replenishment. The oak entrance
resets closed but unlocked in both directions; both stasis directions reset
locked, with differing pickability. Explain effective state without an
invented inside-key prerequisite.

Magic speech clears lock/secret state but retains closed doors; illusion-maze
objects override direction exits; the elemental “portal of darkness” is a trapped
container. Fixed portal, fall and prison routes need actual success/survival
attribution. Five distinct Star pieces and five same-kind swords require exact
root allocation. Isabia/Katalia D0 do not implement departure; Azlion’s rift and
Darrin’s sacrifice are narration. Earlion’s past/present/future are distinct
actors, not proved transformations. Danthas can roam from holding/distribution
rooms; outward-only Troll Hills entry and no foreign fixed portal need builder
qualification. Source declarations are not guaranteed admitted renewable supply.

**Actual native repair:** separate [fix 3f1ecf2be](https://github.com/Community-Duris/Duris/commit/3f1ecf2be064a63321c0e0243b4168727cb079c2)
corrects two south-exit direction words and removes four trailing color resets
from locked magic-password keywords. Plain sargon/thothrontithos previously failed
exact matching; intended passwords now unlock the selected/reciprocal doors.
Original source fails all six cases; repaired source passes actual maintained
C++ password/door functions, wrong-word rejection and closed-state preservation.
Live speech/LOOK/traversal remains unqualified. News: “The Tower of Darkness's south-facing shrine and stair clues now describe their actual directions. Its Sargon and elemental magic doors now accept the intended plain passwords.”
All prior repair/news records remain. Hammer Testing proc messages, copied text,
rescue/rift intent, holding/entry topology and receipt-owner decisions are balanced
pending proposals, separate from this shipped repair and journal additions.

Offering support must be displayed independently of daily eligibility: the
D1/reset-zero cash recipe is classified Story-only before its unsupported coin
reason, and the grouped supported note route hides an all-unavailable warning.
The journal explicitly explains the cash guard; plan per-branch support and
recoverable atomic fees without bypassing active-accounting admission.

Source and actual C++ journal journeys cover exact owners/quantities, optional
history versus supplied input, per-branch guidance, readonly readiness, shared
one-outcome alternatives, replay and cold recovery. Live generation, hidden
recovery/traps, source/handoff, roaming/perception, passwords/access/portals/falls,
combat, retirement/renewal and played persistence remain pending. Catalog:
81 maps/1629 achievements/1463 potential dailies/2207 rows; 2668 native
identities/fingerprint/revision two/registry and prior 80 maps remain. Original
queue: 60/220 complete, 160 pending; Mushroom Caverns next. Active, ready
accounting remains mandatory. No DB/account/server operation, migration,
deployment or merge.

## Mushroom Caverns completed source map — October 4, 2026

The [dossier](zone-stories/MUSHROOM_CAVERNS.md) reviews all sixteen raw
shared-source blocks (thirteen M/two Q/one QA), 132 rooms/74 prose groups,
eight local mobiles, nine objects, 54 resets/33 families, all twelve imported
physical prototypes, the actual piercer procedure, exact legacy actor/item
sources and bounded modern Underdark identity comparison. The generated index
lists eleven identifier-safe M families; the journal includes all twenty-two
representable aliases and explains the two literal punctuation aliases.

The [journal](../../areas/story/mushroom_caverns.story.json) supplies three
outcomes, ten contacts and five optional checks. Haz’on’wyz’s missing goblet
yields one legacy half; Ozman’s exact Bregnar bracelet yields a different half
and C150000 as a reward; Kryz’s two-root bundle yields the legacy seal, two
weapons and experience. Same displayed names do not make halves interchangeable.
Supplied exact materials need no personal assassination or producer receipts;
historical completion cannot restore spent supplies. Native house dialogue is
not a validated allegiance branch. Readiness is read-only and does not certify
available actors or supported accounting admission.

**Concrete source/execution blockers:** object 1515 is absent; Ozman/Kryz have
no active reset placement; Haz’on’wyz’s sole shared-room 24015 spawn has five
absent dispersal destinations. Ozman’s half 24014 is ITEM_MONEY with 100 silver
values. Durable item policy excludes it, so either selected half path cannot
complete Kryz’s bundle under active accounting. Ordinary pickup uses coin
settlement; money drop can merge away proof identity, and I reward issuance
needs qualification. Static all-I/reset-two metadata still calls all three
potential daily candidates. Plan availability and per-recipe support independent
of daily eligibility, with actual source/recipient/retirement/reset episodes.
Retain accounting guards and frozen obligation recovery; no bypass ships.

Modern Underdark actors 700034–700036, halves 700000/700001 and seal 700005
remain distinct. Its two half carriers have M50/equipped half declarations,
Kryz M100 at 847218, two non-retiring contracts and different XP/currency terms.
Winterhaven Lancer requires the modern seal, not legacy 24016. Draknah’s actual
golden goblet 500119 is also a different kind from missing 1515. Preserve
identities and receipts rather than infer substitutions from names or lore.

All five reciprocal ordinary boundary pairs resolve. Two pairs of enter-command
pool objects have exact destinations; one reverse-pool description claims
no return, two lake/river pools are NOSHOW, and floating does not mean hidden.
The fallen-mushroom passage resets closed/blocked with no declared local clear;
the rocks passage resets closed/unlocked. Shaft falling is actual shared behavior;
lava/wagon/chain/ladder are scenery, and the ruined lift has no repair endpoint.
The second young F follows the first young M, not the adult aboleth. An unplaced
level-one grell and seventeen empty room descriptions require builder intent.

**No native zone or quest repair ships.** Missing supply/actors/dispersal,
money-proof type/recovery, pool/obstacle/parent/scenery decisions and the modern
foreign roper reward/prose mismatch remain pending repair/qualification leads.
Each implemented repair must have a clearly named fix commit, prominent PR
zone/trigger/before-after/validation/limits and exact player news wording.
Earlier shipped native repair/news records remain unchanged.

Focused source and actual C++ projection journeys cover exact bindings, modern
separation, same-name kinds, optional history, worn versus loose preparation,
warnings, physical encounter/owner discovery, independent outcomes, replay and
cold recovery. Synthetic receipts do not qualify currently impossible live
offerings. Full production/source, schema/all-map loader, accounting gates,
daily projection, maintained build, formatting/whitespace and source-line links
are checked. Live supply/money/source/handoff/roaming/perception/travel/falling/
combat/retirement/renewal/persistence remain unqualified. Catalog: 82 maps/1629
achievements/1463 potential dailies/2207 rows; all 2668 native definitions,
fingerprint/revision two/registry and prior 81 maps unchanged. Original queue:
61/220 complete, 159 pending; Para-Elemental Plane of Smoke next. Active, ready
accounting remains mandatory. No DB/account/server operation, migration,
deployment or merge.

Haz’on’wyz also receives the shared teacher from ACT_TEACHER. Its `level`
guidance lists matching-class live runestones, without a quest or lesson
receipt. The shared handler lacks addressed-recipient/visibility resolution
and appends to a 512-byte buffer without bounds; plan actual-function recipient,
class and long-output qualification/hardening. No live crash or teacher repair
is claimed. External encounter publication first requires discovery of the
physical area; it does not automatically discover the Mushroom credit owner.

## Para-Elemental Plane of Smoke completed source map — October 4, 2026

The [dossier](zone-stories/PARA_ELEMENTAL_PLANE_OF_SMOKE.md) reviews all fourteen
native blocks (five M/three MA/four QA/two Q), all 153 rooms/fourteen prose
groups/thirty numeric exit families, twenty-six mobiles, thirty-six objects and
172 resets/sixty-six families. Bounded global sources/consumers, both foreign
fixed planar entrances, every local portal/lock, actual current-mobile follower
supply, flight/terrain hazards, packed weapon callbacks and imported epic stone
execution are covered. All local references resolve; no ordinary foreign boundary
or additional local shop/teacher/inn/arena quest terminal was found.

The [journal](../../areas/story/smoke.story.json) supplies two delivery outcomes,
four equipment services, twelve contacts, sixteen native aliases and twelve
optional checks. Ehkahk's ordinary heart yields Rijak's veil/vault key and
retirement; spectacles yield Korli's obsidian key and retirement. Erk accepts
two different broadswords as one forging bundle, converts jewelry in either
direction, and enhances the exact ordinary staff plus ring. Services preserve
history without achievements/dailies. Supplied materials require no personal
kill, vault visit, rescue, producer history or original acquisition. Shared
staff keywords do not make the upgraded kind a valid ordinary input.

**Native repairs ship separately in fix commit 0fff62e70.** The vault south
return lock incorrectly named portal 139819; it now names rewarded key 139818,
matching the reciprocal side. Discontent's displayed name was absent from item
aliases; add `discontent` and retain every old alias. Actual production door/
key/lock/unlock and item-name lookup failed on original data and pass after.
Both directions, carried/held keys, wrong-key rejection, reciprocal state,
closed/pickproof preservation and Hate-before-Discontent selection are covered.
Key destruction/durable settlement is a boundary stub; live travel/ownership
and migration of already loaded/strung items are not qualified.

Ready player news: **“The Plane of Smoke's vault key now works from either side
of the portcullis, and Discontent can be selected by its own name.”** Journal,
plan and source-map authoring is a separate commit. All prior shipped native
repair/news records remain unchanged.

**Pending, with concrete execution evidence:** forge 139945 is Negative Plane
sector 35 and schedules life-force drain despite fire imagery. Forged greatsword
139831's packed bundle includes permanent Power spell 545; actual legacy and
opt-in durable dispatch call its registered callback, which raises base/current
Power toward 95 without caller authority. No played exploit is claimed. Plans
require builder-selected terrain/current spell IDs/balance, approved typed
effects, attacker/target routing and existing-instance/version/recovery policy.
Earring finger-slot intent, copied fire-elemental air description, Korlia wording
and unused/unfinished scenes need builder decisions. The earlier shared epic
absorb post-extraction traversal lead remains pending; no crash or epic fix
is claimed.

Spectacles attach to the second F25 mephit, not the leader or every mephit.
Discontent and vault materials depend on M40, Etrita on M20 and commander on
M60; caps, conditional chains and active-accounting admission still matter.
Both keys have a 100-percent break roll. Unlock, break settlement, open, arrival
and reset are distinct; producer history restores none. Two older-plane entrances
and local returns are declared, but magical darkness, flight, heat and current
portal selection/survival remain actual prerequisites. Epics require committed
group claim receipts, not holding a stone or delivering a heart. Captivity lore
and dialogue audience do not create rescue, learned-topic or observer outcomes.

Focused source and actual C++ projection journeys cover exact kinds, worn/loose
preparation, optional supplied history, read-only readiness, service exclusion,
independent outcomes, owner boundaries, replay and cold recovery. Full production/
source, schema/all-map loader, accounting gates, daily projection, maintained
build, formatting/whitespace and source-line links are checked. Live admitted
sources/handoff, perception/flight/terrain travel, key destruction/settlement,
combat/stat persistence, epic group claims and retirement/renewal remain pending.
Catalog: 83 maps/1625 achievements/1459 potential dailies/2207 rows; four former
crafting fallback credits removed. All 2668 native definitions, fingerprint/
revision two/registry and prior 82 maps unchanged. Original queue: 62/220 complete,
158 pending; Fishermans Wharf next. Active, ready accounting remains mandatory.
No DB/account/server operation, migration, deployment or merge.

## Fishermans Wharf completed source map — October 4, 2026

The [dossier](zone-stories/FISHERMANS_WHARF.md) covers all thirteen native
blocks (eight MA/five QA), seventy rooms/twenty-five prose groups/eighty-nine
numeric exit families, twenty-three mobiles, twenty objects, 119 resets/106
exact argument families and Widoc's shop. Full exit text/properties/membership,
the ordinary Surface boundary, imported skull and bounded global producers/
consumers/713 teleport prototypes are reviewed. All local references resolve.
Shared quest root allocation, ownership credit, reset admission, boat movement,
worn breathing/drowning, totem key lookup, command-time chance fall and actual
fishing execution are covered.

The [journal](../../areas/story/fishermans_wharf.story.json) supplies five
independent credited outcomes (two stories/three requests), thirteen contacts,
forty-four native aliases and ten optional checks (eight current quantities/two
receipts). Baltik accepts one egg/four stick bundles/three pelts for a cavern
totem. Dimbled's guide yields line; four bottles yield bait; the adult accepts
bait plus line for a carrying net. Four frog jellies earn a worn snorkel.
Supplied exact items bypass earlier routes. Duplicate fishermen share definitions;
questions, custody, personal kills, guide-reading, fire-building, breeding,
fishing and later hydra entry are not inferred accepted outcomes.

**No native zone or quest repair ships in this checkpoint.** Actual totem
lookup accepts its vnum without a key-type guard. Pole reset hold/back slots
match its present flags, and equipment applies snorkel face/nose or belt waist
breathing effects. Net negative shell and bottle FLOAT versus sunk prose remain
builder-intent leads. No silent item-type/slot/balance replacement is made.
Prior native fix/news entries remain unchanged and separate from journal work.

**Concrete pending execution lead:** actual fishing narrates catch and grants
XP before generic-crafting ownership submission, ignores the helper result,
and can report no created item on rejected submission. No played failure or
repair is claimed. Plans require actual accepted/rejected grant, interruption,
publication/reward ordering and replay/recovery qualification before a separate
fix, and typed committed catch/source/UID/session/zone evidence before story
credit. Equipped poles are not selected by the existing loose-vnum scan; quest
bait/line/net/jelly are not consumed by fishing. Alternative active-effect
readiness, source versus handoff, admitted capped quantities, key/open/arrival,
command-time chance fall and surviving underwater access need actual events.

The imported skull belongs to Qin's separate five-kind Dream bundle. Its owner-
death lore has no identified callback in reviewed active paths. NODROP ordinary
handoff differs from native direct-root offering; neither custody nor a local
kill completes Qin's quest. Dibbly's snorkel buyback and line-dialogue mismatch
remain in the unchanged earlier Newhaven map. Shared underwater/healing macros
mix formal argument and outer ch, but all reviewed callers pass ch; no wrong-
target call or crash is established. Record hardening as pending, not a fix.

Focused exact-source and actual C++ projection journeys cover counts, equipped/
loose roots, optional supplied history, read-only preparation, independent
outcomes, immutable owner, replay and cold recovery. Full catalog/source,
schema/all-map loader, accounting gates/daily projection, maintained build,
formatting/whitespace, prior source/map preservation and links are checked.
Live supply/handoff, eight-root commit, fishing XP/ownership, breathing/travel,
fall survival, restricted skull custody/Qin completion and daily renewal remain
pending. Catalog: 84 maps/1625 achievements/1459 potential dailies/2207 rows;
all 2668 definitions, fingerprint/revision two/registry and prior 83 maps unchanged.
Original queue: 63/220 complete, 157 pending; Northern Lakes and Settlements next.
Active, ready accounting remains mandatory. No DB/account/server operation,
migration, deployment or merge. Every later actual repair requires an isolated
fix commit, prominent PR before/after and precise news with validation limits.

## Northern Lakes completed source map — October 4, 2026

The [dossier](zone-stories/NORTHERN_LAKES.md) covers all fourteen native blocks
(eight M/six Q; seven addressed families/twenty aliases plus Tein ambient), 219
rooms/138 prose groups/139 numeric exit families/264 text-keyword pairs, sixty-
three mobiles, eighty-two objects, 414 resets/309 exact argument families/226
parent-aware families and both shops. Full properties and memberships, all five
ordinary foreign boundaries, imported stock/visage, global producer/consumer/
713-teleport scans and shared quest/reset/boat/current/fall execution are reviewed.

The [journal](../../areas/story/nlakes.story.json) maps six independent stories,
thirteen contacts and nine optional checks (seven materials/two receipts).
Heart → Tamara bottle → Aerin note → Tamara earring/copper explains three
accepted stages. The human's scroll, dragon's quest vial and Artek's two-scale/
amulet bundle remain independent. Exact supplied bottles/notes bypass personal
history; historical acceptance never restores spent materials or grants skipped
stages. The same-named ordinary vial is not the quest item. Native dialogue
aliases, periodic boasts and narrated recall/cure/flight/experiment/rescue do
not create unsupported outcomes.

**Separate actual repair:** [5a2b93d6d](https://github.com/Community-Duris/Duris/commit/5a2b93d6d387e192ea501d6d6e5bbf71796e927b)
corrects exactly two direction words at pile-of-bones75263. News: **Northern
Lakes' pile-of-bones exits now correctly point east to the cathedral and west
to the pasture.** Both original clues fail; corrected clues/reciprocal routes
and exact native-byte checks pass. Live LOOK/traversal is unqualified. All prior
native repair/news entries remain intact. No other native repair ships here.

Plans expand for optional parent campaigns with explicit all-stage semantics,
committed original-source versus handoff evidence, same-kind quantities versus
one-from-each source, overlapping live actor/supplier roles, typed retirement/
recall/flight and current/fall/access episodes. Helping the green dragon retires
a scale source; killing it for the scale prevents using that instance as the
vial recipient. Qualify actor identity, actual source recovery, cap/reset and
peaceful alternatives before builder choices. Preserve supplied deliveries.
Artek's empty Q response and blue-dragon/expedition narration versus Maur's
preloaded heart are bounded intent leads, not proof of impossible quests.

Melbh's opening-message naming mismatch is inactive: announcement dispatch is
commented out. No live wording repair is claimed. Current warnings precede
attempted movement, on arrival and command; boat custody does not prevent
sweeps. Falls are command-time chances. Closed/secret doors lack declared key
locks, and well return differs. Aevenyl's foreign visage request/earlier
Winterhaven map stays unchanged; `_noquest_` does not disable explicit recipes.

Focused source and actual C++ journal journeys protect quantities, item identity,
optional supplied-route history, six independent outcomes, read-only readiness,
owner rejection, replay and cold recovery. Full catalog/source/schema/all-map
loader, accounting gates/daily projection, maintained build, changed/staged
formatting, native/prior-map preservation and local/source links are checked.
Actual offerings, original acquisition/handoff, peaceful source/recipient order,
D1 extraction/reset, currents/falls/travel/survival, foreign completion and daily
renewal remain unqualified. Catalog:85 maps/1625 achievements/1459 potential
dailies/2207 rows; all2668 definitions/fingerprint/revision two/registry/prior84
maps unchanged. Original queue:64/220 complete,156 pending; Kobold Settlement
next. Active, ready accounting is mandatory; frozen recovery stays separate.
No DB/account/server operation, migration, deployment or merge.

## Kobold Settlement completed source map — October 4, 2026

The [dossier](zone-stories/KOBOLD_SETTLEMENT.md) reviews all seventeen blocks
(13 M/4 Q; six addressed families/sixteen aliases, seven ambient), 147 rooms/
116 prose groups/92 numeric exit families/158 text-keyword pairs, fifty-eight
mobiles, sixty-six objects, 303 resets/201 exact argument families/150 parent
families and all three shops. Full properties/memberships, all three ordinary
foreign boundary rooms, imported items, global producers/consumers/713 teleport
prototypes, nine literal local bindings, dynamic smith/ten forge rows, imported
epic stone and relevant shared execution are reviewed.

The [journal](../../areas/story/kobold.story.json) adds one guarded spectacles
story and three supporting services, sixteen contacts and seven optional
checks. Nuggets → smelt → two blocks → shield and statue gems + corpse frames
→ spectacles are explained. Exact supplied materials skip producer/inspection
history; an old receipt does not refill spent supplies or prove a first source,
kill, learned language, unlocked door or safe escape. All mixed-fee offerings
remain guarded with accounting active. Gem/frames share recipe inputs: later
spectacles selection precedes inspection and can hit the unsupported fee.
Support services do not count as quests/dailies. Classification deliberately
removes three previous support achievement units; native definitions stay intact.

**Separate actual repair:** [02788c573](https://github.com/Community-Duris/Duris/commit/02788c5738a11963647ceb9b4b344463f4f57704)
restores guardian room numbers and the demon's ledge room-list traversal.
News: **Kobold Settlement's temple guardians now defend their actual altar,
tomb and sacrificial pit, restoring the high priest's imp summoning and the pit
demon's ledge attacks.** Original source fails the altar regression. Repaired
actual procedures pass barriers/pit, imp cadence/cap/failure, tomb/pit escape,
proper ledge selection and immunity/chance conditions. Native changes occupy
seven lines, with their executable regression in the isolated fix commit.
Played combat, difficulty and movement remain unqualified. Prior repairs/news
remain intact; all other findings below are plans, not shipped native fixes.

Plans expand for atomic fee/material/reward settlement and overlapping-input
selection, admitted supply across reset/save/reload, original versus handed-off
items, custom death-pile lineage/failure/cash, accepted translated speech and
switch/access, forced travel/guardian/escape episodes, epic versus foreign
ownership and explicit all-stage parent views. Eight nuggets exceed the normal
five-live-copy reset cap; saved/forced supply may differ, so no impossible-quest
claim or cap change ships. Four rod pieces lack reviewed local supply/reassembly;
builder intent must choose actual sources/outcome. A literal inn on guard post
1444 adds RENT despite the real inn at1443; current home/persistence and policy
need qualification before removal. Guarded legacy smith choice bounds/player
inventory and disabled parchment learning need separate qualification/repair.

Full catalog/source checks, actual C++ all-map/schema/file-loader journeys,
accounting gates, server build, changed/staged formatting, whitespace and
native/prior-map/link preservation are checked at publication. C++ coverage
includes quantities, worn/loose items, supplied-route optional history, service
exclusion, retained historical receipts, owner rejection, replay and cold
recovery. Injected historical paid receipts do not qualify live fees. Actual
gathering/handoff, offers/rewards, custom deaths, combat/travel/survival, words/
doors, epic/ambassador/rent persistence and renewal remain unqualified.

Catalog:86 maps/1622 achievements/1459 potential dailies/2207 rows. All2668
native definitions/fingerprint/revision two/registry and prior85 maps remain
unchanged. Original queue:65/220 complete,155 pending; Troll Caves next.
Active, ready accounting is mandatory; frozen recovery stays separate. No
DB/account/server operation, migration, deployment or merge. Earlier full PR
checkpoint text is preserved in the [publication archive](ZONE_STORY_PR_CHECKPOINT_HISTORY.md)
so the current PR can stay within its description size limit while retaining
every earlier repair/news and pending-plan record.

## Troll Caves completed source map — October 4, 2026

The [dossier](zone-stories/TROLL_CAVES.md) reviews all ten blocks (five M/five Q;
five addressed families/nine aliases), 82 rooms/76 prose groups/19 headers/
26 metadata groups/69 numeric exit families/119 text-keyword pairs, 28 mobiles,
37 objects and 193 resets/142 exact/152 parent-aware families. Full properties,
memberships, caps/equipment/container/followers, two foreign boundary rooms,
imported bat/guano, bounded global producers/consumers/713 teleport prototypes,
Alatorin gem dealer/full stock and computed local/shared handlers are reviewed.

The [journal](../../areas/story/troll_caves.story.json) maps one chalice-and-mace
story and four supporting paid services, eight contacts and eight optional
checks. Raw emeralds→cutting→base mace→enhanced mace is explained; ruby and
obsidian commissions stay independent. Exact supplied items skip earlier
personal production, mining or kills. Uncut/cut emeralds share an alias; both
maces display the same name but have different kinds/properties. Only the
base kind and actual chalice satisfy the blessing. Student-kill wishes and
tenfold-power prose do not become required or recorded objectives.

All four mixed fees remain unavailable with accounting active. Hraaf's
separate eleven-row forge menu is also guarded; periodic hum creates no
crafted reward. The item-only blessing retains one potential daily candidate,
without a proven fresh supply or renewal journey. Local gemstone O caps are
one; Alatorin stocks the exact raw kinds under different reset/shop authority.
Source stock and supplied custody do not prove first acquisition or payment.

Farghan demands the lost wand in dialogue, but Q148 only takes uncut emeralds
and100000 copper. Wand96930 resets loose behind the waterfall, not on the
gnome. Builder intent must choose the request versus dialogue; no recipe fix
ships. Cut-emerald/egg/Farghan wording, an unplaced foreign Graves sign and
secret switch reverse-state policy remain separate pending decisions. **No
native repair ships in this checkpoint.** Prior actual repairs and news remain
intact and clearly separated from these proposals.

Two PUSH switches dynamically bind despite zero literal assignments. For
secret targets they clear only the forward block, preserving secret/closed
and reverse blocked state. Waterfall door resets open. The sign uses PUNCH
to96937; the orb uses TOUCH to96940, both unlimited. Nineteen fall fields,
no-ground chasm and underwater sources require accepted movement/control,
arrival, protection, injury/removal and surviving-return episodes. No current
is declared despite water-flow prose. Guremgh's dynamic teacher guidance,
gnome followers, egg armor, miners and recovery flags are not extra quests.

The full custom-code scan additionally found bound/placed Ixarkon veil96402
at96524 with Troll room96909 among twenty-five random destinations. Its
ENTER check expects a leading space removed by ordinary command dispatch.
The full veil/prototype/room/reset/assignment and invocation are reviewed;
qualify and repair that comparison separately with actual selected command,
arrival, post-arrival restore/CharWait and interruption evidence. No usable
random entrance or foreign veil fix is promised. Fixed type-25 inventories
must be supplemented with custom entrance and post-travel effect review.

Plans add exact payment/material/output/source-UID/custody settlement; actual
type/flag/table dispatch census; all-stage parent views with optional supplied
routes; control state and confirmed travel/fall/escape outcomes. Guarded forge
choice12 reaches sentinel−1 for eleven rows, and inventory scan uses the NPC;
qualify bounds/player allocation and paid generation before enabling it.

Full production/source fixture, actual C++ all-map/schema/file-loader journeys,
accounting gates/daily projection, maintained build, changed/staged formatting,
whitespace and native/prior-map/link preservation are checked at publication.
Journey cases cover exact/worn/wrong kinds, optional producer history, service
exclusion, read-only readiness, spent materials, immutable owner, replay and
cold recovery. Historical paid receipts do not qualify live fees. Actual reset/
shopping/handoff, offerings/rewards, switches/doors/falls/travel/death/survival,
effects, persistence and renewal remain unqualified.

Catalog:87 maps/1618 achievements/1459 potential dailies/2207 rows. Four support
units deliberately leave the achievement denominator. All2668 native definitions,
fingerprint/revision two/registry and prior86 maps stay unchanged. Original
queue:66/220 complete,154 pending; Centaur Villages next. Active, ready accounting
is mandatory; frozen recovery stays separate. No DB/account/server operation,
migration, deployment or merge.

## Centaur Villages completed source map — October 4, 2026

The [dossier](zone-stories/CENTAUR_VILLAGES.md) reviews all eleven blocks
(seven Q/four M; four addressed families/eight aliases),100 rooms/100 prose
groups/14 headers/19 metadata/71 numeric exit families/135 text-keyword pairs,
29 mobiles,31 objects and199 resets/117 exact/125 parent-aware families.
All raw properties, memberships, source caps/containers/gear/followers and
full ordinary foreign boundary rooms are reviewed. Bounded global native
producers/consumers/resets/shops/literal assignments/713 teleport prototypes
and relevant custom/shared execution close the reviewed source dependencies.

The [journal](../../areas/story/centaur_zone.story.json) maps four outcomes and
three same-kind inspection/briefing services, eight contacts and sixteen
optional checks (eight current materials/eight producer or guidance receipts).
Heart→letter→returned unicorn horn→one half, staff→second half, then two
separate half roots→bracelet and legplates. Both halves are kind93313; a single
item cannot fill both slots. Supplied exact items skip personal kills,
briefings and earlier production. Past receipts do not restore spent roots.

Llewyn resets with letter93311cap1; the treant and Banitoor independently
reset with half93313 under shared cap2. Heart and horn/staff sources have
cap1; horn is inside hunter backpack93318, staff inside static vines93316.
P resolves a matching container by kind; it does not certify an exact source
UID. Roaming givers and the hunter require actual visible encounters.

Five dynamically bound PUSH controls clear ordinary reciprocal routes.
Four vine controls cover two grove approaches; jagged rock at93326 opens
the eastern cave93399. Controls/staff container have NOSHOW, but actual named
lookup permits that flag despite normal PC display rejecting it. This is not
proof of an inaccessible source or inert switch, and no flag repair ships.
Two mountain fall fields and four declared currents require selected control,
pre/post bits, admitted movement, nested displacement, survival and return
episodes. Level guidance, followers, fruit/pond/spring, age cure, grief,
ancestry and protector-title prose do not create extra accepted objectives.

**Actual native repair: separate fix commit[e456b3403](https://github.com/Community-Duris/Duris/commit/e456b3403).**
Tamilea's cave hint now says eastern edge; grotto west exit, both western
forest approaches, forest east intersection and dead-end eastern entrance
match their actual directions. Exactly six native clue lines change; exits,
flags, sources, caps, aliases, terms, outputs and retirement are preserved.
The focused test fails all six original clues and passes corrected topology.
News: **Centaur Villages' quest and travel clues now point in the correct
directions, including Tamilea's route to Banitoor's cave.** Source proof does
not qualify actual ASK/LOOK/traversal. Keep this fix prominent in PR/news,
apart from journal additions and pending capability/wording work.

All seven offerings are item-only. Hateeu's D1 finale consumes two distinct
halves, declares two indexed outputs and removes the recipient. Accepted
offering, output entitlements/publication, current reward custody, actual
retirement and fresh reset reappearance remain distinct qualification facts.
Do not infer two achievements, fresh supply or same-UID inspection continuity.
Plans retain current/producer/source alternatives and add richer original-
source/handoff, named-selection/learned-alias, control/travel/survival and
recipient/reward/retirement episodes only after actual semantic proof.

Production/source fixture and actual C++all88-map/schema/file-loader journeys
cover exact/worn/wrong kinds, one-versus-two halves, supplied optional history,
service exclusion, spent materials, read-only readiness, immutable zone owner,
replay and cold recovery. Native direction regression, maintained build,
changed/staged formatting, whitespace/native/catalog/prior-map/local-link
preservation are checked at publication. Local/source/projection proofs do
not qualify original/reset/handoff/GET, live offerings and both rewards,
switches/currents/falls/death/survival, Hateeu's removal/reappearance,
persistence or daily renewal. These are qualification gaps, without a claim
that the reviewed zone is impossible or its hidden controls are broken.

Catalog:88 maps/1615 achievements/1459 potential dailies/2207 rows. Three
support units deliberately leave zone completion; four outcome candidates
remain. All2668 native definitions/fingerprint/revision two/registry and
prior87 maps stay unchanged. Original queue67/220 complete,153 pending;
Enclave of the Opal Phoenix next. Active, ready accounting remains mandatory;
frozen recovery stays separate. No DB/account/server operation, migration,
deployment or merge.

## Opal Phoenix completed source map — October 4, 2026

The [dossier](zone-stories/OPAL_PHOENIX.md) reviews all6 blocks(2Q/1QA/3M),76
rooms/45 prose groups/6 headers/6 metadata families/101 numeric exit families/
4 text-keyword pairs,18 mobiles,24 objects and84 resets(M50/G13/D12/E4/O3/P2),
all76 exact/76 parent-aware families. All raw properties/memberships, shop,
computed epic/inn bindings, actual offering/recovery/item+XP/retirement,
visibility/selection/SEARCH/GET/key/door/service/rent and bounded global
dependencies/713 teleport prototypes/full ordinary Surface boundary close the
source map. Registry70353–70876 also causes the generated evidence index to
list23 foreign pirate assignments; those are not physical local specials.

The [journal](../../areas/story/opalphoenix.story.json) maps all3 outcomes,
9 contacts/8 aliases and4 optional checks(3 current materials/1 earlier receipt).
Student quill→sand+nominal20kXP; Alazia sand→tome+quill+nominal40kXP; independent
oldwoman bear remains→cloak. Frozen actor/party XP caps differ, so nominal sums
do not promise actual gain. Supplied exact items skip personal kills/source/
earlier history; spent materials remain missing. Two D1 recipients leave,
including Alazia’s remaining shop stock. A retained receipt does not prove
each indexed reward, current custody, removal, reappearance or daily renewal.

**Actual native repair: separate [ac8e2de48](https://github.com/Community-Duris/Duris/commit/ac8e2de48f3aba3915d95abb7ee1d0fefd580f14).**
Sand70823 was SECRET even after direct reward creation into player inventory;
actual visibility rejects it before the own-inventory shortcut and native
named offering selection has no SECRET exception. SEARCH has no ordinary
path to this secret carried OTHER root. Only SECRET is cleared20480→16384,
preserving NORESET and all other prototype/native contract/source bytes.
Production visibility/isname/ordinal/list functions compiled under ASan/UBSan
fail the original and pass the corrected awake-mortal fixture in both lookup
modes with negative controls. **News: Opal Phoenix’s student sand reward is now
visible and can be selected for Alazia’s delivery quest.** This repairs future
instantiations, without rewriting already-saved instances or qualifying a
played full hand-in. Keep existing-instance remediation as a separate approved
operational plan; native repair/news must stay distinct from the journal.

Intestines/key/chest retain legitimate hidden-source SEARCH paths. One roaming
bear kind/reset supplies intestines; same-name bear variants are not equivalent.
Mask chest70816 starts closed/locked with exact70821 key; SEARCH room/source
corpse, loose/HOLD key, UNLOCK,100% ordinary key break, OPEN and GET are distinct
facts. UNLOCK clears the lock before committed destruction; a busy break may
retain the key after opening access. No mask-delivery terminal is invented.
Six reciprocal doors are closed but unlocked. Rope fragility, sign factions,
exam, prayer and planned spell/ogre victory are narrative, without extra local
accepted outcomes. Asymmetric hallways are not presumed broken.

Alazia’s native Q dispatcher is independent of accounting-blocked commerce.
Azalea’s epic table binds Nature’s Sanctity at full boot despite no raw
ACT_TEACHER flag; teaching remains unavailable with accounting active. Actual
inn flags bind70851/70852/70869 to native rent; ordinary admission and terminal
DB save apply, rather than a universal accounting service disablement.
Qualification of trade/teaching before enablement and rent/save/retry remain
separate. Later optional spelling and builder effect decisions are proposals.

Plans add accepted source/handoff/current UID and same-name runtime identity,
reveal/selected alias, consumable access-cost versus world-state, frozen
indexed item+XP, merchant stock/recipient retirement/reset and service/rent
episodes. Existing optional journal/history checks do not implement those
richer semantics. No new event/API/schema/payment path ships here.

Focused source/native-selection regressions, actual C++all89-map schema/file
loader/projection journeys, maintained build, changed/staged format, whitespace,
native/catalog/prior-map/local-link preservation are checked at publication.
Source/projection proof does not qualify actual SEARCH/key/GET/source/handoff,
offerings/items+XP, recipient/stock removal/reappearance, service/rent,
persistence or daily renewal. Catalog89 maps/1615 achievements/1459 potential
dailies/2207 rows; all2668 definitions/fingerprint/revision two/registry and
prior88 maps unchanged. Original queue68/220 complete152 pending; Myrabolus
next. Accounting active and ready remains mandatory; frozen recovery separate.
No DB/account/server operation, migration, deployment or merge.

## Myrabolus completed source map — October 4, 2026

The [dossier](zone-stories/MYRABOLUS.md) reviews all17 blocks(16Q/1M),188
rooms/127 complete prose groups/27 headers/31 metadata/168 numeric exit
families/16 full text-keyword pairs,78 mobiles,62 objects,two shops and316
resets(M181/D44/E38/G22/O13/F13/P3/R2),all257 exact/272 parent-aware families.
All properties/memberships/containers/gear/positions/flags/caps,local and
imported literal procedures,computed teacher/smith/inn bindings,actual shared
offering/recovery/zero-reward/items+coins/D1/visibility/search/key/transient/
awake/wake/movement/reset/services and bounded active global dependencies/
713 teleport prototypes/eight full foreign boundary bodies close the source
review. Generated evidence alone does not establish that closure.

The [journal](../../areas/story/mira.story.json) maps13 outcomes/3 support
entries,21 contacts/4 aliases and24 optional checks(21 current materials,
3 earlier receipts). Markam’s returned note also grants a new half and is a
story stage; Balance’s same-kind referral and Roland’s blank unrewarded head
offering remain support. Andryn’s guarded paid armor is separate from the
keystone. Native definitions/daily policy are preserved;12 authored story
candidates remain,without proving available supply or renewal. Rigid’s foreign
producer receipt is optional; wrong same-name original study76068 does not
replace delivery76069. Two different half kinds and all3 token kinds are
required. Four treasure recipients and all other deliveries are independent.

Real gold-key/chest/crate, palace-key/secret Sunweaver/hatch, invisible-secret
knight half/transient assembled key, observatory-key/paired ENTER portals,
letter/drawer and secret den sources are explained. Normal100% key break is
a deliberate access cost; world unlock can precede accepted destruction.
Alexis starts asleep without a raw magical-sleep barrier; WAKE enables native
awake dispatch. Boat431 differs from fixed FLOAT life-raft container82546,
and lost-monkey item13365 differs from living local NPC82542. Same-kind returns
do not preserve original UID. No terminal rescue, launch, healing, mount-return
or world-balance effect is invented from prose. Each indexed reward/removal,
current custody, actual reset and renewal differ from one accepted receipt.

**No native zone/quest/service repair ships in this checkpoint.** Missing
treasury2267 is confirmed absent from active prototypes and its reset is
disabled by renumbering; builder chooses restoration/replacement/removal.
Unrewarded local Roland, empty houses/load-room wanderers and stub connections,
drawer/open versus closed wording and Master-set membership need intent and
qualification. The direct quiver proc has7 legacy kinds; the separate8-kind
adapter does not prove a bound longbow proc. Preserve legitimate hidden,
invisible,transient and key-cost semantics; do not globally clear flags.
Any later implemented native repair requires its own explicit fix/news record.

Andryn’s computed FORGE,Rico/Shantil commerce,epic purchases,paid locker entry
and ferry tickets remain accounting-guarded. Actual inn rent uses independent
admission/terminal save. Imported counter3097 has a locker hook; fountain72
rotates a DRINK spell; mechanism54 is a real alarm ward,not another delivery.
Crew HIRE ignores denied SUB_MONEY before changing crew/chief and queuing save,
an existing shared gap also recorded for Verspin. Plan identified admitted
wallet/ship continuations and qualification before any service completion
claim. No service is required by the item-only stories.

Expand accepted original-source/handoff/replacement UID lineage,perception/
successful reveal/selection,transient custody/access cost,awake actor,physical
affiliation versus owner,ALL-stage versus terminal supplied shortcut,competing
treasure/material allocation,indexed item+currency and recipient/reset/daily
episodes. New custom fate/launch/healing/mount endings require builder-defined
accepted effects and failure/return policy. Existing optional hints/history
do not implement those richer semantic events.

Focused source assertions,C++all90-map/schema/file-loader/projection journeys,
maintained build,changed/staged formatting,whitespace,native/catalog/prior-map
and local-link preservation are checked at publication. Local source/projection
proof does not qualify actual acquisition/handoff/search/perception/key/wake/
offer/items+coins/removal/reset,services/rent/storage/ferry,persistence or daily
renewal. Catalog90 maps/1612 achievements/1458 potential dailies/2207 rows;
all2668 native definitions/fingerprint/revision two/registry/prior89 maps and
native content/code remain unchanged. Queue69/220 comprehensive,151 pending;
The Depths of Duris next. Active-ready accounting remains mandatory,frozen
recovery separate. No DB/account/server operation,migration,deployment or merge.


## Priority 70 checkpoint: The Depths of Duris

The [comprehensive dossier](zone-stories/THE_DEPTHS_OF_DURIS.md) closes source
review for the complete 103-block/2645-room area, 75 mobiles/60 objects, 611 resets
(406 exact/530 parent-aware families),ten shops, custom/source/computed service
bindings and bounded imported/foreign dependencies. Fifteen native offerings
become nine outcomes and three support entries with 17 contacts, all 109 addressed
aliases and 15 optional checks. The generated audit and inventory are refreshed.

Mystardala's crystal-ball and dagger+trophy stages accept supplied exact inputs;
her narrated wielded-dagger kills are not native predicates. Hidden ball chest
requires its exact tiny key and resists picking. Ungalen's four brew choices
retain distinct rewards/receipts but one parent outcome; Naltem's foreign five
brews remain ALL ingredients. Tok's ordinary and stronger same-name feather
kinds are separate, and both have actual foreign sources. Fresh osquip custom
death/expiry differs from ordinary corpse, cooked steak and rotting replacement.
Glendarla's band return records no Borik rescue. Gulranor takes ten separate
kind 8 roots, rather than ten victims, and grants three indexed rewards including
two identical smoke bombs before retiring. CARVE creates inside the corpse;
first recovery, GET, handoff and mutation/expiry lineage are still untracked.

**Precise expansion:** accepted corpse/actor/generation/part/tool/UID source
transactions, intentional failed carving costs versus allocation/publication
failure, first source versus supplied custody, periodic transformation lineage,
ANY versus ALL allocation, frozen per-index item/coin/XP settlement and actual
recipient/reset/supply episodes. No new event/schema, economic adapter or runtime
API is implied by source guidance. Accounting must be active and ready for new
progression; frozen recovery remains separate.

**Native repairs:** none ships here. Six exits lack active destination rooms;
renumbering deletes them. Builder intent must choose restoration, replacement
or obsolete-row removal. The wall/stair guard note at 121257, two blue glasses'
pink descriptions and three unplaced residents receive balanced review plans.
These pending proposals do not belong in shipped-fix news. Earlier separate
native fix commits and their prominent news/proof/limits remain preserved.

**Validation:** focused exact-source checks and all 91 Python/C++ schema/file-
loader/projection tests cover nine/ten loose parts, wrong/rotting/equipped kinds,
supplied shortcuts, spent materials versus optional receipts, OR brews/supports,
owner rejection, one credit for multiple rewards, replay and cold recovery.
Full production source/catalog/inventory/audit regression, maintained server
build, changed/staged formatting, whitespace, link/original-queue checks and
exact preservation guards passed before publication. Final player-copy and
current-material assertions also pass against the refreshed catalog.
These are source/projection checks; native CARVE/death/access/GET/payment/live
settlement and actual renewal remain unqualified.

Current catalog:91 maps/1606 achievement units/1454 potential dailies/2204 rows.
All 2668 definitions, fingerprint, revision two, registry, prior90 maps and all native
world/quest/source/service bytes stay unchanged. Original roadmap is **70/220
source-comprehensive, 150 pending**. IceCrag Castle is next. The full goal remains
active. No accounting activation, DB/server operation, migration, deployment or
merge occurred.


## Priority 71 checkpoint: IceCrag Castle

The [comprehensive dossier](zone-stories/ICECRAG_CASTLE.md) closes all 67 native
blocks, 243 rooms (232 prose/30 headers/44 metadata/628 exits/241 numeric/200 text
families), 66 mobiles, 138 objects and 620 resets (374 exact/460 parent-aware).
All twenty local assignments, custom and shared source paths, four imported
objects, eleven present foreign recipe kinds and one missing kind, eighteen
foreign reset groups, six relevant shops, ten foreign native rows, all 713 portal
targets and seven boundary edges/five external room IDs were reviewed.

The journal classifies eleven native offerings as eight stories, two paid services and
one excluded incomplete recipe; 25 contacts retain all 129 distinct aliases and
16 optional checks. Seven pure-item stories are daily candidates. The sergeant's
winter-clothing help remains a meaningful story with its fee guard; readiness
of three clothes never pays it. Masha's missing 6551/pelt/parchment recipe cannot
be completed ordinarily and supplies no journal/daily unit. Exclusion does not
repair native content. No substitute ingredient or invented cookbook is added.

Three pages are different kinds despite identical names; four bottles need two
of each, with a cap-one elven source; supplied shoes can skip the artist's earlier
receipt, which cannot replenish spent shoes. Ordinary kitchen onion differs
from Masha's juicy reward; the commander's accepted hidden book differs from its
same-name unused counterpart. Two loaded hearts differ from ordinary CARVE
parts. Each accepted offering produces one outcome despite coin/XP or duplicate
rewards. The front speech door unlocks both matching sides; OPEN and arrival
remain independent. RUB pedestal/orb travels and another player's shared access
do not manufacture a quest terminal or personal key history.

**Expansion:** explicit invalid-content quarantine and builder requalification;
source/handoff/hidden selection/UID custody across wolf/vapor replacement;
successful speech-door mutation/open/arrival; Masha's attempted/accepted GET
interference, NPC defense and scoped transformation/hunting episodes; exact ALL
allocation, atomic mixed fees, per-index frozen item/coin/XP settlement and
recipient/source renewal. Existing schema carries guidance, current materials
and receipts; unsupported adapters and native campaign terminals remain planned.

**Native repairs:** none. Missing dungeon destination, incomplete recipe, elven
bottle cap, duplicate-kind copy, dormant guardian/follow code and global/failure
scope each have balanced repair/design plans. Actual future fixes require
separate fix commits and prominent PR/news trigger, before/after, failing/passing
proof, limits and a news-ready sentence. Earlier actual fixes remain preserved.

**Validation:** focused source/prototype/binding checks and all 92 Python/C++
schema/file-loader/projection journeys cover quarantine/services, exact same-name
and repeated roots, current Ready/Missing states, supplied shoes, spent optional
history, owner rejection, one credit per outcome, replay and cold recovery.
Full production source/catalog/inventory/audit regression, maintained build,
changed/staged formatting, whitespace, links, original queue, native bytes,
definitions/fingerprint/revision/registry and prior 91 journals are verified
before publication. These checks do not qualify played speech/search/GET/key/
combat/transformation/payment/settlement/retirement/reset/persistence or daily
renewal. Accounting must be active and ready; frozen recovery remains separate.

Current catalog: 92 maps/1603 achievement units/1453 potential dailies/2203 rows.
All 2668 definitions, fingerprint, revision two, registry and native world/code
remain unchanged. The original queue is **71/220 source-comprehensive, 149 pending**;
Father Tel's Holy Cloister is next. The full goal remains active. No accounting
activation, DB/server operation, migration, deployment or merge occurred.


## Priority 72 checkpoint: Father Tel's Holy Cloister

The [comprehensive dossier](zone-stories/FATHER_TELS_HOLY_CLOISTER.md) closes all
36 native blocks, 71 rooms (33 prose groups, six headers, six extra descriptions,
157 exits), 24 mobiles, 31 objects and 113 resets (97 exact/99 parent-aware).
Nineteen addressed topic families retain 28 aliases. All four automatically
bound switches, native actor assignment, shared lookup/search/movement/key/trap/
offering/reward/retirement paths and bounded foreign source closure were reviewed:
three foreign item prototypes, four reset groups, four mobile and five room bodies,
two competing foreign egg requests, all 713 portals and both valid approaches.

The journal classifies all eight offerings as seven stories and one supporting
rejection exchange; 16 contacts and 11 optional checks cover nine current materials
and two producer receipts. Tel's note refusal earns no achievement/daily unit and
does not admit a student or return the original item identity. Mahr's note can
instead yield the disciple's experience/clue. The local egg → bone key → podium
ring + poison → rib bone → helmet case route remains optional guidance between
independent offerings. Supplied notes/rings need no invented producer history;
history cannot restore spent materials. Robes differ from Mande's head, and the
General's loaded head differs from ordinary carved parts. Jade and Winterhaven
compete for the same egg without taking ownership of local completion.

SAY Khildarak and PUSH statue/boulder/tapestry are exact switch commands, despite
no literal assignments. Switches clear BLOCKED; secret passages then require
local SEARCH. Door unlock/open, key destruction, container reveal/GET and arrival
are separate operations. The egg's GET/PUT acid trap can consume its charge while
rejecting pickup; damage or attempted acquisition is not first custody. World
cap-one stock, a rare foreign ring and retiring recipients need renewed supply
and generation proof before daily availability. All seven stories are potential
item-only daily candidates, rather than a claim of qualified live renewal.

**Expansion:** semantic refusal and same-kind replacement lineage; accepted
automatic switch/search/door/trap state and exact source-versus-handoff custody;
builder-owned ALL/ANY campaign branches, supplied shortcuts and competing roots;
per-index item/nominal-versus-actual group XP settlement, recipient retirement
and source/reset renewal. Formal admission, meditation training and the tablet's
ten-minute poison mission have no accepted native terminal. Builder decisions
remain necessary; no per-keyword achievement or invented ending is added.

**Actual native repair, separate fix commit:**
[208a56840](https://github.com/Community-Duris/Duris/commit/208a56840acb100fc20dbec4b0af9a80a6cd0914)
changes Mahr's acceptance caption from letter to tablet. The exact requirement,
reward and departure remain intact. Original caption fails the focused check;
corrected caption/contract and one-word byte scope pass. Played turn-in is still
unqualified. **News:** “Brother Mahr now correctly identifies the intruder's
tablet when accepting it, making the Cloister's recommendation quest clearer.”
The source-clue disagreements, unfinished admission/deadline and orphan/stale
key-field questions have balanced proposals; they are not shipped repairs.

**Validation:** source/prototype/binding fixture and all 93 Python/C++ schema,
file-loader and projection journeys cover service exclusion, exact current
materials, worn/wrong proof, supplied notes/rings, spent optional history,
foreign owner rejection, seven outcomes, exact-event replay and cold recovery.
Full production catalog/audit/inventory regression, maintained build, changed/
staged formatting, whitespace, links and preservation checks are required before
publication. Synthetic receipts do not qualify played speech/control/search/GET/
trap/key/combat/offer/XP/reward/retirement/reset/persistence or daily renewal.
Active, ready accounting remains mandatory; frozen obligations recover separately.

Current catalog: 93 maps/1602 achievement units/1453 potential dailies/2203 rows.
All 2668 native definitions, fingerprint, revision two, registry and prior 92
journals remain intact. The one-word caption correction is the sole native
change. Original queue: **72/220 source-comprehensive, 148 pending**; Turolopolis
is next. The full goal remains active. No accounting activation, database/server
operation, migration, deployment or merge occurred.


## Priority 73 checkpoint: The Ruins of Turolopolis

The [comprehensive dossier](zone-stories/RUINS_OF_TUROLOPOLIS.md) closes all25
blocks (19M/sixQ),154 complete rooms (109 prose groups/17 headers/22 extra
descriptions/394 exits),42 mobiles,56 objects and199 resets (160 exact/164
parent-aware). All six local type25 portal kinds, shared RUB/ENTER dispatch,
selection/actual arrival, search/door/key/fall, source custody, native allocation,
indexed XP/coin/item rewards and actor retirement were reviewed. Bounded active
closure includes one complete foreign reset group, nine foreign room bodies,
all713 portals, both public reciprocal approaches and no missing required kind
or foreign local-kind recipe. Generated evidence does not replace source closure.

Six independent stories retain26 contacts/39 aliases and12 optional checks:
eleven current materials and one earlier memorial receipt. Five separate
count-one brown/green/blue/red/black steps preserve native ALL allocation;
white badges and multiple copies of one colour do not substitute. The plaza's
north–south statue row uses RUB; each mirrored room has an ENTER return portal.
Badge keys also unlock matching hatches and remain after ordinary unlock, before
the all-five offering consumes them. Current possession never proves rescue.

Five badges → lesser bloodsaber + loaded caecilia stinger → greater bloodsaber
is optional guidance between independent outcomes. Supplied blades need no
memorial history; a receipt cannot restore a spent or worn blade. Corwyck's horn
provides an iron bunker key, separate from the sentinel's crimson palace key.
The slave's named-skull outcome grants coins, without a recorded all-prisoner
liberation or spell-field collapse. Nominal XP remains subject to frozen actual
recipient policy; five D1 recipients and cap-one materials constrain renewal.

The minotaur carries the letter needed by the emissary. Ooze acceptance
retires him and can discard it; killing for the letter removes that current
ooze recipient. Source/recipient-generation branches, admitted transfer, supplied
proof and renewed availability need explicit qualification. Kurtukr follows
Sprecken from a real rare-load holding source with public-house exits and a
no-exit sink. INDOORS8 is not ROOM_NO_MOB4: public dispersal is possible; intent,
survival, sink stay and reset should be qualified before calling the route broken.

Lothrell currently loads in Surface's grassy foothills, not historical Verspin
or the empty local start. Surface discovery admits that physical encounter;
neither it nor the canonical zone71 memorial receipt fabricates local discovery.
Plan truthful cross-zone home-journal navigation while retaining actual physical
room and native ownership. Presumed-dead comrades versus living trapped heroes,
fountain cleansing and palace freedom remain builder intent questions, rather
than implemented campaign repairs. **No native Turolopolis repair ships.**

Focused source fixtures and Python/C++ projections cover exact/wrong/worn inputs,
missing badge colours, supplied upgrade, spent history, foreign discovery/owner,
six outcomes, replay and cold recovery. Full production regression, maintained
build, changed/staged formatting, source links, exact native/catalog/prior-journal
preservation and published PR verification are required before publication.
One earlier Cloister fixture now uses the correct one-based ROOM_INN bit; this
corrects the source audit/test and leaves its no-inn conclusion unchanged.

The complete earlier PR body is archived in
[checkpoint history](ZONE_STORY_PR_CHECKPOINT_HISTORY.md) before compacting
historical prose; all actual repair/news/accounting blocks remain in the current
PR verbatim. Current catalog:94 journals/1602 achievement units/1453 potential
dailies/2203 rows; all2668 definitions/fingerprint/revision/registry and prior93
journals remain intact. Queue: **73/220 source-comprehensive,147 pending;
Ixarkon next**. Source completeness and synthetic receipts do not qualify played
access/recovery/offer/settlement/reset/renewal. Active, ready accounting remains
mandatory, with frozen recovery separate. The full goal stays active; no accounting
activation, DB/server operation, migration, deployment or merge occurred.


## Priority 74 checkpoint: Ixarkon

The [comprehensive dossier](zone-stories/IXARKON.md) closes all18 blocks
(15M/threeQ),201 rooms/198 prose groups/15 headers/one extra description/468
exits,51 mobiles,44 objects,five shops and385 resets/302 exact/323 parent-aware
families. Literal and automatic bindings, imported board/counter/consumables,
shared native offer/allocation/XP/coin/item settlement/recovery and actual
access/source roles were reviewed. Bounded closure includes two Ixxillikor actor
groups, five full foreign boundary rooms/nine edges, the complete actor room,
all713 portal prototypes and every full body in the25-target custom veil list.
All25 destinations exist; separately assigned entry19890 is absent.

Existing schema2/revision1 becomes schema3/revision2 with the same three story
identities: two independent stories and one supporting paid preparation,16
contacts/22 aliases/four optional checks. Only one of six myconid placements
carries the hidden spore; only one of two same-kind elders wears the red cap.
The pacing elder is a distinct recipient beside them. Black/bone-white caps do
not substitute. Optional banker history does not restore a spent amulet;
supplied amulets need no earlier fee receipt. The native price is red cap plus
1000platinum together, but fresh mixed/coin input remains refused under active
accounting. Paid hireling and locker actions have their own active guards.

Veil room96524 has no ordinary ingress or exits despite ceremonial prose.
GithyankiCave is assigned to absent19890; real_room0 falls back to indexzero.
Builder must choose ingress/race/return intent and guarded binding before repair.
Random targets, exact argument/selected identity, actual arrival/restoration and
pre-removal failure handling need qualification. Bridge levers remove BLOCKED
but the reciprocal exits start open; the bridge is usable and no retraction is
implemented. Literal inn service works independently of absent ROOM_INN flag
and unreset receptionist. These findings remain plans, not restored content.

Actual native repair **7baa78c3c** changes12 direction words in11 rooms,
preserving every other native field/file. It has a separate fix commit and
focused original-fails/repaired-passes reciprocal-route regression. Prominent
PR/news treatment is recorded in the repair ledger and dossier. No fee, veil,
bridge, rescue, diplomatic or service mechanism repair is implied.

Focused source and Python/C++ projections cover stable identities, exact/wrong/
worn inputs, supplied amulet without banker history, historical supporting
receipt without story credit, spent custody, canonical ownership, independent
outcomes, replay and cold recovery. Full production regression, maintained build,
changed/staged formatting, source links and native/catalog/prior-journal/queue/
PR preservation checks are required before publication. Synthetic receipts do
not qualify played access/recovery/offer/settlement/reset/renewal.

Catalog:94 journals/1601 achievement units/1453 potential dailies/2203 rows;
all2668 definitions/fingerprint/content revision/registry and other93 journals
remain intact. The previous complete PR body is archived exactly with SHA-256;
all prior actual repair/news/accounting blocks remain verbatim in this PR.
Queue: **74/220 source-comprehensive,146 pending; Du'Maathe Castle next**.
Active, ready accounting remains mandatory; frozen recovery remains separate.
The full goal stays active. No accounting activation, DB/server operation,
migration, deployment or merge occurred.


## Priority 75 checkpoint: Du'Maathe Castle

The [comprehensive dossier](zone-stories/DU_MAATHE_CASTLE.md) closes all21
blocks (13M/eightQ),506 rooms/126 full prose families/20 headers/1224 exits,
92 mobiles,49 objects,one shop and419 resets/303 exact/312 parent-aware
families. Twelve addressed families have15 aliases; qc_action80 stays ambient.
Two inns and table-bound epic teacher Carmotee were distinguished from flags.
Bounded foreign review covers hermit/clothing recipe dependencies and sources,
Alatorin totem continuation, full Surface/Pit boundary rooms, tooth-key neighbour
and foreign resets, all keyed foreign exits and all713 portals. It is not a
new comprehensive Podaling-area or Pit-of-Dragons pass.

The new schema3/revision1 journal has four independent lord outcomes and four
supporting potion services,15 contacts and14 optional checks:11 present materials
and three historical receipts. Sand AND consumed foreign recipe produce three
granular potions; the lord accepts one. Supplied potion/tooth routes need no
earlier local craft/gem receipt. Foreign hermit and Alatorin exchanges retain
canonical ownership. Wrong horns/shop wand, worn or spent materials and earlier
key history do not manufacture current custody or personal source recovery.

No active exact blue-horn producer was found. Foreign clothing crafts quote9/6
platinum but accept1000/500copper, and paid input/epic purchases remain guarded
with active accounting. Loader discards high exit-state bits: waterfall and
prison portal start visible, whereas northern prison wall and catacomb gate
are secret through actual D resets. Described underwater rooms retain indoor
sector; drawbridge/collapse/ward/healing/notebook lore has no corresponding
quest endpoint. Builder selects source/price/visibility/controller/campaign intent
before separate native repairs. Accepted material/output lineage, key break,
personal source, actual quaff/spell/arrival and renewal remain qualification work.

Actual repair45bb3c948 changes exactly one direction word at the northwest
parapet, with original-fails/repaired-passes clue/title/reciprocal-route proof.
All other native bytes are preserved. Repair/news wording is prominent in the
ledger, dossier and PR; none of the proposed mechanic fixes is claimed shipped.

Focused source/Python/C++ projections, full production regression, maintained
build, changed/staged formatting, source links, original queue, definitions,
fingerprint/revision/registry/prior94 journals and exact PR preservation checks
are required before publication. Synthetic receipts do not qualify played native
source/craft/unlock/reward/foreign movement/reset/daily renewal.

Catalog:95 journals/1597 achievement units/1449 potential dailies/2203 rows;
all2668 definitions/fingerprint/content revision/registry and prior94 journals
remain intact. Queue: **75/220 source-comprehensive,145 pending; Tundra next**.
Active, ready accounting remains mandatory; frozen recovery remains separate.
The full goal remains active. No accounting activation, DB/server operation,
migration, deployment or merge occurred.


## Priority 76 checkpoint: Tundra

The [comprehensive dossier](zone-stories/TUNDRA.md) closes all20 blocks
(13M/sevenQ),175 rooms/64 complete prose families/19 headers/401 exit rows/
390 numeric families/nine exit-text families/one extra-description sign,
26 mobiles,32 objects,one shop and246 resets/197 exact/197 parent-aware
families. All12 addressed families/16 aliases are included; qc_action80 stays
ambient. Literal reception inn, absent teacher flags/bindings, loaded bandages
and automatic type29 mirror are distinguished from named scenery.

Bounded foreign closure includes148 intersecting Q/QA signatures/22 full caption
families, five required material prototypes,68 parent-aware reset groups and
full source actors/rooms. Bom's exact painting exchange produces pike, lobster
and hairy crab, without clam. Paid clam/pike merchant routes remain guarded;
Cairme's rations-plus-four-beer continuation stays foreign-owned. All19 boundary
edges and full neighbours, active versus inactive legacy map110038 and all713
portals were reviewed; no portal targets a local room. This does not claim a
new comprehensive Harrow, ship-yard or vehicle pass.

New schema3/revision1: six independent story outcomes and one paid service,
16 local contacts and14 optional checks (12 present materials/two earlier
receipts). Four distinct books produce snowy adventurer boots; supplied boots
skip that history. Head acceptance retires Eleadora, so boots-first guidance
helps preserve her other reward without forcing order. Three different seafood
kinds produce the fishbone key; Bom supplies only two. Foreign fire gland and
shell yield different XP amounts and remain independent. All six native stories,
including the D-retiring head, remain potential daily candidates. Mixed scale/
500-platinum service is unavailable with active accounting and awards no credit.

Actual loaded state matters: Onyx Stairs and workshop entrances are visible/
closed through D1 despite raw high hidden bits. Bedroom back doors use D5;
mirror bedroom north uses OPEN/BLOCKED D8 and PUSH clears BLOCKED, retaining
reverse SECRET/CLOSED. Both different village keys break on ordinary unlock.
Docks sector4 fails FISH water eligibility, while inn river sector10 is water.
The local fishmonger has selling prose but no shop. Actor loading branches can
trap giver/source; signposted routes, -1 southern destination and inactive
legacy forest target are unresolved. No native Tundra repair ships. Builder
selects intent before separate fixes with concrete before/after and news proof.

Focused source/Python/C++ projections, full production regression, maintained
build, changed/staged formatting, source links, original queue, definitions,
fingerprint/revision/registry/prior95 journals and exact PR preservation checks
are required before publication. Synthetic receipts do not qualify played catch,
source/transfer, paid order, accepted offer, rewards, switch/unlock/key break,
actor retirement, campaign, persistence or daily renewal.

Catalog:96 journals/1596 achievement units/1449 potential dailies/2203 rows;
all2668 definitions/fingerprint/content revision/registry and prior95 journals
remain intact. Queue: **76/220 source-comprehensive,144 pending; The Fields
Between next**. Active, ready accounting remains mandatory; frozen recovery
remains separate. The full goal remains active. No accounting activation,
DB/server operation, migration, deployment or merge occurred.


## Priority 77 checkpoint: The Fields Between

The [comprehensive dossier](zone-stories/FIELDS_BETWEEN.md) closes all17 native
blocks (tenM/sevenQ),153 rooms/77 complete prose families/nine headers/
428 exact exits/16 exit-text families/no extras,68 mobiles,33 objects and286
resets/273 exact/273 parent-aware families. All ten addressed families/81 aliases
are included. No local shop, inn, teacher, literal assignment or switch supplies
an omitted quest. Four unlimited ENTER portals, imported epic stone359/memory
55435, all713 active portal prototypes, six boundary edges/full neighbors and
foreign follow-on/dispatched requests were reviewed in their bounded scope.

Seven stories/22 contacts/17 optional checks preserve seven item-only outcomes
and daily candidates, including Timmy's D retirement. Optional current access
keys and materials never force personal source history. Timmy's alloy → letter
→ mother is the only local reward-to-input chain; supplied letters skip history.
Professor needs a separate alloy bar AND torture box. Five distinct hidden heads
are ALL, despite similar names; manuscript is worn as a shield and bananas are
carried by the octopus. Orders AND prepared body are another exact bundle.
Mother can emerge into Scorched Valley; receipt ownership stays Fields Between.
Seeker/Marvin requests, advisor's three heads, brewer's foreign fez continuation,
Winterhaven memory and epic TOUCH remain their own systems/owners.

**Correctiond2a64432d: portability repair05eeca928 is withdrawn.** Quest prose
was incorrectly used to justify TAKE and weight-one on unlimited ENTER portal
71030. Shared inventory selection makes that a PvP escape-capability change.
Original TAKE=0/weight1,000,000 are restored; the entire object file exactly matches
pre-fix bytes. Unsafe prototype fails the new policy regression; restored source
passes. No deployed or saved-instance rewrite occurred. **Do not announce rift
pickup/delivery as fixed.** Native source/quest conflict remains a builder decision:
separate non-teleport proof, source-bound interaction or intentional retirement.
No replacement or new portability is implemented.

Pending proposals are separate: narrated escape/transform/reunion lacks durable
world transitions; permanent chamber-rift prose has no placed portal; portable
rift slain lore differs from floor reset; troll caption says ogre, two mages
share elements aliases and foreign dispatch prose has a stale room number.
Builder selects clarification or intended source/controller behavior before
other native changes. Preserve intentional scarcity and supplied/public routes.

Focused source/schema/C++ projections, full production regression, maintained
build, changed/staged formatting, links, original queue, all2668 definitions,
fingerprint/content revision/registry/prior96 maps and exact prior PR/repair
preservation are required. Synthetic receipts do not qualify native play or
database persistence. Catalog:97 journals/1596 achievement units/1449 potential
dailies/2203 rows. Queue: **77/220 source-comprehensive,143 pending; The Town
of Moregeeth next**. The full goal remains active. Active, ready accounting is
mandatory; frozen recovery remains separate. No accounting activation, DB/server
operation, migration, deployment or merge occurred.


## Priority 78 checkpoint: The Town of Moregeeth

The [comprehensive dossier](zone-stories/THE_TOWN_OF_MOREGEETH.md) closes all20
native blocks (M8/Q6/MA1/QA5),353 rooms/148 complete prose families/26 headers/
1350 exact exits/17 exit-text families/no room extras,109 mobiles/105 full prose
families,96 objects and549 resets/442 exact/453 parent-aware families. All nine
addressed families/22 aliases, ten shops, thirteen imported prototypes, eight
unlimited type25 portals, shared inn/world-quest/bank/training/justice behavior,
all713 active portals and five boundary edges/full foreign neighbors were reviewed.
No foreign quest/reset/portal supplies an omitted local material or outcome.

Schema3/revision2 retains all seven old IDs/contracts and four exclusions, with
five story outcomes and two paid services. Fifteen optional checks explain
14 current material/access conditions and one earlier pouch receipt. Pouch →
magical key → four exact planar components is useful preparation, without
forcing personal history for supplied materials. Moreg wears Glub's requested
sword; recovery can remove another giver. Glub and Gimbatul retire after their
specific rewards. Moreg's reward called a key is actually a lockpick. Pouch is
a potion, letter is inside Ungal's desk and five skulls come from five cave bats.
STARE flame/ENTER mirrors/TOUCH crystal/SOUTH smoke use verified current commands.

**Actual native repair, separate fix [7b916b887](https://github.com/Community-Duris/Duris/commit/7b916b887):
Fix Moregeeth letter quest desk keyhole.** Before, the closed locked desk's
negative key field made PICK and KNOCK reject it before an attempt. Exactly
one field now enables attempts with keyhole zero. Lock, acid opening trap,
letter reset/text and Tala reward remain intact. Original-data regression fails;
repaired data and exact-byte scope pass. Played skill/trap/pickup/offer/settlement
remains unqualified; saved instances are not rewritten. **News-ready:**
“Moregeeth's trapped letter desk now permits normal lockpicking or knock
attempts, restoring access to the letter for Tala's quest.”

Other findings remain proposals: copied Gimbatul/Gorblog captions, distinct
same-name actors, narrated bribes/vault/rescue/ghost hunt/revenge and no durable
campaign endings. Actual admitted source/transfer, container/skill/trap/portal
movement, fee/all-root settlement, actor removal, public access and renewal
need qualification. No new kill gate, guaranteed roll, universal key, extra
achievement per keyword or campaign finale is invented.

Focused source/schema/C++ projections, full production regression, maintained
build, formatting, links, original queue, all2668 definitions, fingerprint/
revision/registry/prior96 other journals and exact prior PR/news preservation
are required. Catalog:97 journals/1594 achievement units/1449 potential dailies/
2203 rows; paid-service classification removes two inappropriate achievement
units without changing native contracts or daily count. Queue: **78/220
source-comprehensive,142 pending; Ceothia next**. Full goal remains active.
Active, ready accounting is mandatory; frozen recovery remains separate.
No accounting activation, DB/server operation, migration, deployment or merge
occurred. Synthetic receipts do not qualify played transactions or persistence.


## Priority 79 checkpoint: Ceothia

The [comprehensive dossier](zone-stories/CEOTHIA.md) closes all21 blocks
(M12/Q8/QA1),295 complete rooms/107 prose families/seven headers/619 exits/
three exit-text families/no extras,110 mobiles/88 full prose families,33 objects,
510 resets/299 exact/386 parent-aware families and the one actual shop. Full
local/shared implementations, imported agility pool62, computed epic teaching,
all713 active portal prototypes,17 foreign recipe bodies/nine foreign reset
groups and nine boundary edges/four complete foreign neighbors were reviewed.
Bounded Past/Future portal/key/source closure retains foreign zone ownership;
it does not mark either foreign zone comprehensive.

Schema3/revision1 binds nine native recipes in six cards: any one of four guild
bargains, three independent Lenbrea rewards, two-copy crate delivery and the
captain's legacy scroll. Seventeen contacts retain22 addressed aliases. Seventeen
optional checks mean14 current material/access conditions and three earlier
receipts. Supplied exact proof does not require personal kills, guild history,
door access or timeline travel. Leader retirement and replenishment, breaking
keys, secret/pickproof doors, fixed wagon/two child roots, actual portal movement,
foreign ownership and precise pool effects remain qualification work.

**No native zone or quest repair ships.** Wagon guidance correctly allows PICK/
KNOCK: bit2 is compatibility-only HARDPICK, while PICKPROOF bit16 is absent.
The pool's argument-blind DRINK handler is a pending shared target-resolution
repair. Legacy tablet supply and bound scroll learning were not identified;
builder intent must decide restoration or retirement, separate from guarded
modern epic teaching. Copied venue/door clues, unbound inn/shop descriptions,
rare-load title versus reset and narrated guild/timeline campaign remain fair
review proposals. Future repairs require isolated named fix commits and news;
the earlier shipped repair ledger is preserved verbatim.

Required validation: focused source/schema/grouped-choice C++ projections, full
production regression, all98 Python/C++ journal loading/projection journeys,
maintained build, formatting, whitespace/source links, original220 queue,
all97 prior journals/all2668 native definitions/fingerprint/revision/registry
and exact prior PR/news preservation. Catalog:98 journals/1591 achievement
units/1446 potential dailies/2200 rows; the three-unit decrease groups guild
alternatives without changing native recipes. **79/220 source-comprehensive,
141 pending; Braddistock Mansion (`brad`) next.** Full goal remains active.
Active, ready accounting is mandatory; frozen recovery is separate. Synthetic
receipts do not qualify played source/access/travel/pool/learning/crew/offer/
settlement/actor lifecycle/database persistence/daily renewal. No accounting
activation, DB/server operation, migration, deployment or merge occurred.


## Priority80 checkpoint: Braddistock Mansion (brad)

The [comprehensive dossier](zone-stories/BRADDISTOCK_MANSION_1350.md) distinguishes
this zone1350 mansion from older zone13 Braddistock. All48 complete rooms/47 full
prose families/five headers/99 exits/83 text families/26 extras in25 families,
14 mobiles/14 prose families,73 objects and175 resets/116 exact/125 parent-aware
families were read. No local Q/shop file, room F/T, object T, portal or switch was
found. All14 owned M9/Q5 Tower blocks, twelve full imported material/reward
prototypes, twelve foreign actors/placement rooms/reset groups, shared ration/
torch, all713 portals and reciprocal Tharnadia boundary were closed.

Schema3/revision2 preserves all five story IDs and bindings, with16 contacts/
32 aliases/ten optional checks (nine carried materials/one earlier Isabia
receipt). Staff, fixed head, five distinct pieces, supplied ring and bone key
remain exact independent exchanges. Local books, Ned, secret cellar/skeletons,
alchemy, static corpse, smuggler cargo and boat are exploration guidance,
without fabricated learned, source, travel, rescue or campaign completion.
Skeleton135040 is NOLOCATE rather than invisible/secret; book flags5 are closed,
unlocked; valid world door kind2 is pickable. Missing iron key135050 does not
prove a blocked non-pickproof strongbox; local maul is not older bound1372.

**Actual native repair:** separate fix5b4c4f34a sends the spirit's existing refusal
to the blocked player with CRLF. Exactly two native lines. Original isolated
production function fails; repaired recipient/exact-line and unchanged threshold,
staff/null/periodic/self/direction/action controls pass. Maintained build/format/
exact-byte checks pass; played entry remains pending. News wording is recorded
prominently in the repair ledger above and PR. Earlier repairs remain intact.

The physical story/credit owner gap is substantive: Tower giver VNUMs assign
zone1350, while encounters/placement/reset are1340. Keep frozen native ownership;
add explicit physical affiliation/referrals and discovered-area presentation,
then qualify real source/retiring-recipient renewal independently of owner mode.
Mansion clue/access/container/source provenance and actual alchemy/smuggler/
Sargon/rift/escort terminals need admitted events and builder-selected semantics.
Missing key/weapon sources and copied prose are proposals, not shipped repairs.

Required validation: focused source/schema and native entry fixtures; full
production regression; all98 Python/C++ journals/file-loader/projection journeys;
maintained build; formatting/whitespace/source links; five preserved IDs/bindings,
96 unchanged other journals plus the explicitly corrected Fields journal/all2668 definitions/fingerprint/revision/registry/original220
queue; exact previous47897-byte PR archive and nine prior repair/accounting
sections. Catalog totals unchanged:98 journals/1591 achievements/1446 potential
dailies/2200 rows. **80/220 source-comprehensive,140 pending; The Desert City of
Venan'Trut (`desert`) next.** Full goal remains active. Accounting must be active
and ready; frozen recovery is separate. Synthetic receipts do not qualify played
source/access/combat/offer/settlement/retirement/renewal/database persistence.
No accounting activation, DB/server operation, migration, deployment or merge.


## Native mechanics review rule after Fields Between correction

Do not infer authorization to enable disabled mechanics from quest prose.
Mechanical changes to portable travel/escape, combat access, charges, source
scarcity or controller activation need an explicit intended design and review
across the game. Keep existing restrictions and record conflicts as builder
questions until resolved. Native text/output corrections remain distinct from
availability and balance changes. The withdrawn rift repair and its correction
remain traceable in separate commits and historical PR archives; current docs
and news must state the final behavior, rather than preserve a false repair claim.


## Required builder follow-up confirmed by owner: Fields Between rift

The owner confirmed that non-takeable rift71030 was an old deliberate hotfix for
its game-breaking escape mechanic. Keep the restored original restrictions.
ZSQ-FIELDS-BETWEEN-RIFT-HOTFIX-REPLACEMENT is required follow-up in the
[zone dossier](zone-stories/FIELDS_BETWEEN.md) and shared plan: design an inert
non-teleport quest proof, a fixed-source interaction or deliberate retirement;
qualify PvP restrictions, source/gift rules, changed native contract/legacy
receipt compatibility and active-accounting source/settlement/recovery/renewal.
No replacement or new portal mobility is implemented. Any later fix gets its
own reviewed commit and final-behavior news; the old pickup claim is withdrawn.


## Priority 81 checkpoint: The Desert City of Venan'Trut

The [comprehensive dossier](zone-stories/DESERT_CITY_OF_VENAN_TRUT.md) closes
all16 native blocks(MA1/M7/Q8),677 complete rooms/158 full prose families/
35 headers/1710 exits/16 exit-text families/four non-exit metadata families,
245 mobiles/234 complete prose families,194 objects,1256 resets/942 exact/
964 M-parent-aware families and ten shops. Literal/type/flag/computed bindings,
full shared quest/switch/teleport/epic/ship/crew behavior, five imported reset
objects and relevant foreign source/recipe/placement closure were reviewed.
All713 active portals,13 boundary edges/three foreign neighbors and19 ordinary
foreign import reset groups retain actual ownership and follower context.

Schema3/revision1 adds eight independent cards with22 contacts/all46 aliases
and11 optional checks: ten current materials/access checks and one earlier
Goranon receipt. The remote cliff boulder opens the blocked mine; falls and
river travel lead to miner/wyrm. Distinct signets, actual worn versus floor
sources, pickproof House/palace gates and secret cellar are explained. Queen
royal garb→Goranon medallion→Eriic locket→Winterhaven fabric is a progression
story with supplied-proof shortcuts, not an enforced all-stage campaign.
Sultan's memory/epic stone, computed Eriic/Ruffus lessons and ship/crew/random
quest services stay separate. Fixed TOUCH stones differ from the type11 reward.

**No native zone or quest repair ships.** Unplaced portal templates with missing
destinations, copied clues, node lore and the taproom inn/crew handler overlap
need explicit builder intent and qualification. Do not activate disabled or
unplaced mechanics, enable TAKE or change charges to fit prose. Future repairs
need separate named fixes and prominent before/after/proof/news. Owner-confirmed
Fields hotfix follow-up and all earlier repair/withdrawal sections are retained.

Required validation: focused complete source/schema fixture, all99 Python/C++
file-loader/projection journeys, full production regression, maintained build,
formatting/whitespace/source links, all98 prior journals/all2668 definitions/
fingerprint/revision2/registry, original220 queue and exact50406-byte prior PR
archive with SHA-256/news preservation. Catalog:99 journals/1591 achievements/
1446 potential dailies/2200 rows; existing unit totals unchanged.
**81/220 source-comprehensive,139 pending; Past Ceothia (`ceopast`) next.**
The full goal remains active. Active, ready accounting is mandatory; frozen
recovery is separate. Synthetic receipts do not qualify played source/access/
falls/travel/combat/learning/group claims/ship/settlement/actor lifecycle/
database persistence or daily renewal. No accounting activation, DB/server
operation, migration, deployment or merge occurred.


## Priority 82 checkpoint: Past Ceothia

The [comprehensive dossier](zone-stories/PAST_CEOTHIA.md) closes all14 native
blocks(M8/Q6),293 complete rooms/38 full prose families/nine headers/990 exits/
46 relative exit families/13 full exit-text families/one S metadata family,
46 mobiles/39 complete prose families,25 objects and129 resets/108 exact/
108 M-parent-aware families(70 location-expanded groups). There are no local
shops/imported reset prototypes, literal assignments, inn/teacher flags or
computed epic teachers. Shared native/durable quest, search/key/portal/wandering
behavior and bounded foreign timeline context were reviewed. All713 active
portals, recipe references and relevant reset/boundary scans retain ownership;
all six touching recipes are local, and Present portal80816 is the sole
identified foreign arrival. No ordinary boundary edge exists.

Schema3/revision1 adds four cards binding all six recipes,13 contacts/all21
aliases and11 optional checks(ten current materials/access, one earlier dryad
receipt). Dryad floor blossom→hair/tube and initial carried hair are separate
sources. Majelle needs three distinct components, then her key, the shaman's
bone key and dragon's obsidian key explain the cave/vault route to Future.
All three locks are pickproof and keys break on ordinary unlocking. Jamael's
three colors form one outcome with retained individual native receipts.
Wolfspeed's skull dialogue versus actual black-pelt recipe remains explicit.

**No native zone or quest repair ships.** Proposed Wolfspeed proof/narration
repair needs intended balance and deliberate contract/version/recovery policy.
Rare staging prose does not establish a measured6% availability; two actors
can disperse independently into a dead-end sink. Mode0 source/recipient renewal
needs qualification before relying on static daily candidates. No recipe,
spawn probability, reset mode, key, TAKE, portal charges or PvP mobility is
changed. Any actual repair needs a separate named fix and prominent news.
Owner-confirmed Fields hotfix follow-up and earlier repair sections are retained.

Required validation: exact complete source/schema fixture, all100 Python/C++
loader/projection journeys, full production regression, maintained build,
formatting/whitespace/source links, all99 prior journals/all2668 definitions/
fingerprint/content revision2/registry, original220 queue and exact49555-byte
prior PR archive with SHA-256/ten repair/news/accounting sections. Catalog:
100 journals/1589 achievements/1444 potential dailies/2198 rows; grouping pelt
alternatives removes two fallback achievement/daily units without native changes.
**82/220 source-comprehensive,138 pending; The Basin Wastes (`basin_wa`) next.**
Full goal remains active. Active, ready accounting is mandatory; frozen recovery
is separate. Synthetic receipts do not qualify played source/search/access/
key destruction/combat/travel/rare wandering/effects/native offer/reward
settlement/actor lifecycle/database persistence/daily renewal. No accounting
activation, DB/server operation, migration, deployment or merge occurred.


## Priority 83 checkpoint: The Basin Wastes

The [comprehensive dossier](zone-stories/THE_BASIN_WASTES.md) closes all16
native blocks(M6/Q10),200 rooms/91 full prose families/eight headers/692 exits/
69 relative exit patterns/78 full exit-text families/one S metadata family,
15 full mobiles,32 full objects and204 resets/151 exact/191 M-parent-aware
families(83 expanded groups). Imported skull67240 and the full surface boundary
room are bounded context; all ten touching recipes are local, no foreign local
ingredient reset source or incoming portal was identified across713 prototypes.
Full shared reverse quest selection, teacher registration/epic table, spider
EAST barrier, container SEARCH, READ→LOOK and source/reset behavior were reviewed.

Schema3/revision1 adds seven cards/ten recipes/eight contacts/fourteen aliases/
fourteen optional checks(thirteen materials, one earlier red-potion receipt).
Four crafts stay distinct, four cash sales count once, and the heartstone
refusal is a zero-credit service. Giving a part selects cash; giving the elixir
selects a craft, with ambiguity if multiple part kinds are loose. Actual sources
include one elixir-bearing minotaur, ten tunnel gland holders and Aberden's
fixed corpse signet. Supplied ritual proof does not require personal craft,
kill or reading. Six hidden books and three Milton passages explain expedition
context without fabricated learned-clue credit or an onward Crystal City exit.

**No native repair ships.** Builder follow-ups cover dispatch selection, learned
book events, source/renewal, same-kind replacement refusal, clue consistency
and personal cave access. Preserve cap-one scarcity, reset mode2, wear masks,
spider barrier and mobility. Implemented repairs need separate named fixes and
prominent before/after/news. Fields owner-confirmed hotfix follow-up is retained.

Required checks: focused source/schema fixture, all101 Python/C++ loader and
projection journeys, full production regression, maintained build, formatting,
whitespace/links and publication preservation. All100 previous journals/all2668
definitions/fingerprint/content revision2/registry and original220 queue remain
intact. The exact50042-byte previous Past Ceothia PR description is archived
with SHA-256 and all ten repair/news/accounting sections. Catalog:101 journals/
1585 achievements/1441 potential dailies/2195 rows. Grouping sales removes three
fallback achievement/daily units; refusal removes one more achievement.
**83/220 source-comprehensive,137 pending; Nakral's Crypt (`crypt`) next.**
Full goal remains active. Active, ready accounting is mandatory; frozen recovery
is separate. Synthetic receipts do not qualify played recovery/gifts/SEARCH/
READ/GET/native dispatch/consumption/reward settlement/replacement identity/
combat/access/teaching/database persistence/renewal. No accounting activation,
DB/server operation, migration, deployment or merge occurred.


## Priority 84 checkpoint: Nakral's Crypt

The [comprehensive dossier](zone-stories/NAKRALS_CRYPT.md) closes all11 native
blocks(M6/Q5)/294 rooms/191 complete prose families/28 headers/653 exits/177
relative patterns/113 full exit texts/23 non-exit metadata families,59 full
mobiles(56 prose families),201 full objects and521 resets/439 exact/447
M-parent-aware families(288 expanded groups). F resets replace the current
holder: butterfly, imp, magma devil and white wolf carry statue proof, distinct
from their M leaders. Three imported prototypes and full Undead Outpost boundary
are bounded context. Five touching local recipes are local; no foreign local
material source identified across active resets, or foreign incoming portal
across713 active type25 prototypes. Local fixed TOUCH orb remains non-takeable.

Schema3/revision1 adds five cards/five recipes/nine contacts/21 aliases/14
optional checks(13 current materials, one earlier trophy receipt). Exact wood
quantities, five same-name distinct chunks, four trophies/token/bracelet and
original→enhanced same-name collar are explained. Supplied token needs no
personal prior trophy receipt; bracelet/collar each retire the statue. Real
three-note SAY magic-door and four PUSH/PULL controls are explained without
invented personal milestones. Learned pages, combat, rent/fountain/group epic
claim remain separate. Active cavern exit points to missing old surface target;
loader removes it. Do not infer an intended modern destination from old maps.

**No native repair ships.** Seven builder follow-ups cover learned words,
control/access, follower source/renewal, collar presentation, stale surface exit,
clue consistency and epic investigation. Any actual native fix needs separate
named commit, before/after proof and prominent PR/news. Preserve original
pickup/charges/mobility/reset/recipe policy and Fields owner-confirmed hotfix.

Required checks: focused source/schema, all102 Python/C++ loader/projection
journeys, full production regression, maintained build, changed/staged format,
links/whitespace and preservation/publication proof. All101 prior journals,
all2668 definitions/fingerprint/revision2/registry and original220 queue stay
unchanged. Exact50089-byte previous Basin Wastes PR description is archived with
SHA-256 and ten repair/news/accounting sections retained verbatim. Catalog:
102 journals/1585 achievements/1441 potential dailies/2195 rows; five authored
cards replace five fallback units without changing global unit counts.
**84/220 source-comprehensive,136 pending; The Valoisian Castle (`val`) next.**
Goal remains active. Synthetic receipts do not qualify played source/gifts,
F spawning, learning/SAY/switches/keys, native offer/consumption/rewards,
same-name collar identity, actual travel/combat/group claims, persistence or
renewal. Active, ready accounting mandatory; frozen recovery separate. No
accounting activation, DB/server operation, migration, deployment or merge.


## Priority 85 checkpoint: The Valoisian Castle

The [complete dossier](zone-stories/THE_VALOISIAN_CASTLE.md) closes all13
native blocks(M5/Q8),174 rooms/120 full prose families/17 headers/374 exits/
145 relative patterns/11 full exit texts/two non-exit metadata families,93 full
mobiles/57 full objects and320 resets(270 exact/280 M-parent-aware families,
166 expanded groups). F changes the chamberlain's current guard holder.
No imported local reset prototype, literal local assignment, ACT_TEACHER or
epic teacher identified; shared ROOM_INN registers Wailing Griffon rent.
Global shop scan found no matching local keeper. All eight touching recipes
are local. One bounded Verspin reset group supplies the non-quest ebony dagger
to its leader before four F smugglers. Full Surface boundary is reciprocal;
no incoming portal across713 active type25 prototypes identified.

Schema3/revision1 adds eight cards/eight recipes/18 contacts/26 aliases/15
optional checks(14 current materials, one earlier cook receipt). Family seals,
royal seals, exact four crafting models, wine→plate→queen token, two different
rose gifts and elvish note are separate accepted outcomes. A supplied plate
does not require personal cook history; prior receipt cannot replace spent
proof. Dialogue does not prove personal kills, politics or learning. All four
keyed gate pairs remain locked/pickproof; steel/ancient/vineyard keys break on
successful unlock. Secret ladder reset5 means closed/unlocked plus secret,
not open. No magical flower belt, shop, new route or portable escape inferred.

**No native repair ships.** Six builder follow-ups cover source/renewal,
access/learning, meaningful political endpoints, blank flower explanations,
isolated unfinished components and clue consistency. Fifteen Veralis rooms
have unfinished descriptions and isolated topology; banquet38440–41 is a
separate isolated component. Builder confirms reserve versus repair and
intended identity/population/access before a separate named fix/news. No
automatic spawn or gap-filling. Owner-confirmed Fields intentional PvP hotfix
and its required replacement follow-up remain intact.

Required checks: source/schema, all103 Python/C++ loader/projection journeys,
full production regression, maintained build, changed/staged formatting,
links/whitespace and preservation/publication proof. All102 prior journals,
2668 definitions/fingerprint/revision2/registry and original220 queue stay
unchanged. Exact50467-byte previous Crypt PR body is archived with SHA-256;
all ten repair/news/accounting sections retained verbatim. Catalog103 journals/
1585 achievements/1441 potential dailies/2195 rows: eight authored cards
replace eight fallback units without changing global counts.
**85/220 source-comprehensive,135 pending; Harrow -The Gnome Village (`harrow`) next.**
Goal remains active. Synthetic receipts do not qualify played source/gifts,
F spawning, keys/SEARCH/READ/GET/access, native offer/consumption/rewards,
combat/politics, actual stock/recipient renewal or database persistence.
Active, ready accounting mandatory; frozen recovery separate. No accounting
activation, database/server operation, migration, deployment or merge.


## Priority 86 checkpoint: Harrow -The Gnome Village

The [complete dossier](zone-stories/HARROW_THE_GNOME_VILLAGE.md) closes all29
blocks(20M/oneMA/fiveQ/threeQA),82 rooms/65 full prose families/21 headers/
187 exits/76 relative patterns/ten full exit texts/23 complete non-exit metadata
families,59 full mobiles/88 full objects and280 resets(230 exact/233 parent-aware
families,152 expanded groups). Nine full shop records and shared boot/keeper
dispatch, art instructor ACT_TEACHER, inn29403, complete imported paper/fish and
two reciprocal Surface boundary records reviewed. Eight recipes touching local
objects are all Harrow; imported fish link138 foreign recipes, bounded index
reviewed without moving their ownership. No foreign reset group identified.
Across713 active type25 prototypes, all five incoming destinations are local;
none enters lucky-star rooms. No local epic-teacher binding identified.

Schema3/revision1 adds eight cards/eight recipes/14 contacts/12 aliases/20 optional
checks(16 materials, four earlier ring receipts). MA/QA are room echo, not
automatic quests; seventeen ambient messages are not keyword achievements.
Ring→token supports four alternative crafts; each needs a fresh token. Supplied
token fits without personal ring history and an earlier receipt cannot restore
spent proof. Lomya's finite finished lucky-sack stock does not prove Alorka's
craft. Stylish versus shop horn, scrap versus bolt, near-finished versus gallery
painting and actual hidden doll source remain exact. Bom gives three imported
fish; goldfish feeding rewards experience; leprechaun consumes pot ITEM and
rewards clover/coins before departing. Five goldfish share one recipe.

Fixed non-takeable waterfall/observatory/bowl routes retain original values.
JUMP fishbowl names the return target; command recognition does not prove actual
arrival. Waterfall F50/C1 3 loads fall/current metadata. Lucky-star29477–81 has
no identified ordinary incoming edge; only29480 returns EAST to29476, while
29478/29479/29481 have no ordinary exits. Shared/random travel not qualified.
No automatic entrance, luck/pipe gate, magical garden belt or source kill/carve
predicate inferred. Paint set in a non-door key field is not a locked art gate.

**No native repair ships.** Six required builder follow-ups cover source/renewal,
token progression, committed travel, lucky-star access, clue consistency and
learned crafts. Sphere/staff/wand, copied dust description, mixed kitten/fish
ambient line and other clues require intended minimal text review; new entrances
need explicit PvP/terrain/scarcity design. Actual repairs remain separate named
fix commits with original-fails/repaired-passes proof and prominent news.
Owner-confirmed Fields intentional hotfix and required replacement stay intact.

Required checks: source/schema, all104 Python/C++ loader/projection journeys,
full production regression, maintained build, changed/staged formatting,
links/whitespace and preservation/publication proof. All103 prior journals,
2668 native definitions/fingerprint/revision2/registry and original220 queue
stay unchanged. Exact50691-byte previous Valois PR body is archived with SHA-256;
all ten repair/news/accounting sections retained verbatim. Catalog104 journals/
1585 achievements/1441 potential dailies/2195 rows: eight authored cards replace
eight fallback units without changing global counts.
**86/220 source-comprehensive,134 pending; Mountain Tracts of the Untamed
(`mountaintracks`) next.** Goal remains active. Synthetic receipts do not qualify
played source/gifts/stock/SEARCH/GET/worn recovery/teaching/portal dispatch/native
offer/consumption/reward settlement/combat/access/daily renewal or persistence.
Active, ready accounting mandatory; frozen recovery separate. No accounting
activation, DB/server operation, migration, deployment or merge.


## Priority 87 checkpoint: Mountain Tracts of the Untamed

The [complete dossier](zone-stories/MOUNTAIN_TRACTS_OF_THE_UNTAMED.md) closes all
eight blocks(fourM/fourQ),245 rooms/86 full prose families/35 headers/578 exits/
199 relative patterns/15 full exit texts/four complete non-exit metadata families,
98 full mobiles/89 prose families/63 full objects and259 resets(230 exact/
230 parent-aware families,144 expanded groups). One full shop, automatic shared
type29 dispatch, fixed item travel, GRAB/CLIMB, QUAFF and room renumbering reviewed.
No local literal special assignment, ACT_TEACHER, epic-teacher or ROOM_INN.
No imported reset prototype or foreign reset group identified. Five recipes
touch local items:four local plus Ohnagra's Winterhaven recipe; its complete
bounded dialogue/input/reward remains foreign. Across713 active type25 objects,
all seven fixed incoming destinations are local objects. Fifteen reciprocal
foreign neighbor pairs and their full room records reviewed; six additional raw
exits are unresolved. Exact numeric code scan found only ordinary chaos gear.

Schema3/revision1 adds four cards/four recipes/eight contacts/19 aliases/six
optional checks(five materials, one earlier Futni receipt). Miniature piece
differs from the stationary large statue; Tronglodish holds exact hidden marble,
but personal apprentice death is not required. Futni requires one exact hidden
NORENT scale and tooth together; other vampires/decorative scales differ.
Supplied potion fits Bumble without personal Futni history. The old receipt
cannot restore a spent potion. Bumble consumes one; Ohnagra's foreign recipe
needs the gloves and another fresh potion plus moonstone heart. Drinking/spilling
also consumes potion and does not complete delivery. No faith/carving/alchemy/
lost-tower endpoint or19 keyword achievements inferred. Three blank accepted
responses and two S-only headers remain honest builder explanation questions.

PUSH clears EX_BLOCKED but not EX_CLOSED:tomb/boulder routes still need OPEN.
Existing **GRAB vine** at21121 uses current CMD_GRAB65→21144; CLIMB556 is a
separate skill. No opcode repair, pickup or travel change. Two unplaced portals
and two unplaced switches targeting absent directions stay unplaced. Five exits
to absent212xx rooms and20999 SOUTH−1 are removed at boot; no live crash claimed.
21127/21128 have no identified ordinary incoming route and lose their raw exits;
21149 has no ordinary edges despite DOWN-for-Lava-Caves prose; actual connection
is21148 SOUTH. C50 current contradicts mild-current prose; F100/F10 and fog
sight rules remain native. New passages/activation require explicit builder intent.

**No native repair ships.** Six builder-required follow-ups cover source/renewal,
potion allocation, accepted control/arrival, lore endpoints, boundary intent and
clue consistency. Prefer truthful explanations before mechanics. Actual repairs
remain separate named fixes with original-fails/repaired-passes proof and
prominent before/after news. Preserve owner-confirmed Fields escape hotfix and
required replacement design; no mobile working-rift quest proof.

Required checks:source/schema, all105 Python/C++ loader/projection journeys,
full production regression, maintained build, changed/staged formatting,
links/whitespace and preservation/publication proof. All104 prior journals,
2668 native definitions/fingerprint/revision2/registry and original220 queue
stay unchanged. Exact51117-byte previous Harrow PR body is archived with SHA-256;
all ten repair/news/accounting sections retained verbatim. Catalog105 journals/
1585 achievements/1441 potential dailies/2195 rows:four authored cards replace
four fallback units without changing global counts.
**87/220 source-comprehensive,133 pending; The Orcish Slave Camp (`shortc`) next.**
Goal remains active. Synthetic receipts do not qualify played source/gifts/SEARCH/
GET/worn recovery/native offers/consumption/QUAFF/control dispatch/arrival/combat/
reward settlement/actual daily renewal or persistence. Active, ready accounting
mandatory; frozen recovery separate. No accounting activation, DB/server
operation, migration, deployment or merge.


## Priority 88 checkpoint: The Orcish Slave Camp

The [complete dossier](zone-stories/THE_ORCISH_SLAVE_CAMP.md) closes all seven
blocks (four M / three Q),14 rooms/14 full prose families/four headers/27 exits/
14 relative patterns/four full exit texts/three complete non-exit metadata
families,31 full mobiles/27 prose families/18 full objects and 64 resets(64 exact/
64 parent-aware families,54 expanded groups). All 31 mobiles are placed; three
unrelated M records have 50-percent chance. No imported reset proof, foreign
reset group or foreign recipe consumer. Only boundary is reciprocal Split
Shield 10327 EAST↔53200 WEST, with full foreign room read. Across 713 active
type 25 prototypes none targets local rooms. No shop, literal special assignment,
ACT_TEACHER, epic-teacher, ROOM_INN or exact custom numeric source reference.
Shared quest/position conversion/key unlock/break/EAT/P/D dispatch reviewed.

Schema3/revision 1 adds two cards/three classified recipes/one explicit exclusion,
five contacts/13 aliases/four optional checks (three materials, one earlier
master receipt). Hidden guard key→master coins+steak; exact steak→hero mace+
configured XP+tragic D departure. Hero's old5/5 positions mean sitting/resting,
not an added waking mission. Master also starts carrying cap1 steak stock;
supplied matching steak fits without personal master history. One fragile key
serves trapdoor access and is consumed by hand-in; history cannot restore it.
Key value 1=20 is 20-percent break roll, not 20 uses. Native has_key accepts HOLD;
loose journal check does not. Reveal/unlock/OPEN/arrival and another actor's
opening differ. Raw door kind2 supplies EX_PICKABLE, not EX_PICKPROOF; no new
mandatory key-only route is imposed. Native key break settlement can reject.

The steak is TRASH13, despite food-like values; normal EAT rejects it, while
exact item identity drives the quest scene. Ordinary FOOD19 gives no reward,
leaves hero hungry and is refused by active durable admission, which only
matches exact item roots. This branch is explicitly excluded until universal
typed offering/consumption/outcome qualification. No type conversion used as
a workaround. Narrative death and shared extraction are not combat kill,
corpse creation, rescue or prisoner liberation. Selected recipient context uses
quester/completion/room; template/room reselection lacks admitted NPC instance/
epoch. Delayed settlement versus removal/reset replacement needs actual proof
and a precise repair design. Bag scroll P selection is global prototype lookup;
authored gnome holder context does not guarantee current live custody.

**No native repair ships.** Six required follow-ups cover source/renewal,
fragile-key access/allocation, typed-food admission, recipient retirement,
steak-type intent and clue consistency. Guard key versus mud glint, dwarves
versus gnomes in southern barracks, copied descriptions and spelling require
intent review. Prefer truthful prop/clue explanation; do not create keys,
rescue followers, mines paths or new escape routes. Actual repairs remain
separate named fixes with original-fails/repaired-passes proof and prominent
before/after news. Owner-confirmed Fields escape hotfix/replacement preserved.

Required checks: source/schema, all 106 Python/C++ loader/projection journeys,
full production regression, maintained build, changed/staged formatting,
links/whitespace and preservation/publication proof. All 105 prior journals,
2668 native definitions/fingerprint/revision 2/registry and original220 queue
stay unchanged. Exact51446-byte previous Mountain Tracts PR body is archived
with SHA-256; all ten repair/news/accounting sections retained verbatim.
Catalog 106 journals/1584 achievements/1441 potential dailies/2194 rows:
explicit T19 exclusion removes one unsupported fallback achievement/row,
without removing its native definition or retained historical evidence.
**88/220 source-comprehensive,132 pending; The Underground Lava Caves (`lavcav`) next.**
Goal remains active. Synthetic receipts do not qualify played source/gifts/SEARCH/
GET/held-key use/key break/reveal/unlock/open/arrival/exact or typed offers/
poisoning/retirement/reward settlement/daily renewal or persistence. Active,
ready accounting mandatory; frozen recovery separate. No accounting activation,
DB/server operation, migration, deployment or merge.


## Priority 89 checkpoint: The Underground Lava Caves

The [complete dossier](zone-stories/THE_UNDERGROUND_LAVA_CAVES.md) closes six
blocks(four M/two Q),142 physical rooms/38 full prose families/11 full headers/
451 exits/128 relative patterns/six full exit texts/one complete non-exit
metadata family,48 full mobiles/46 full prose families/33 full objects and163
resets(87 M/48 E/20 D/five G/two F/one O;134 exact/155 parent-aware families;
111 expanded groups). All48 mobiles placed; two are F followers. Full Mountain
Tracts21148 SOUTH↔35501 NORTH boundary reviewed, plus foreign Ravenloft2 slab/
lamp/watch reset group and full room/slab/watch records. Both recipes touching
local items are local; no horns O/G/E/P producer. Across713 active type25
prototypes none targets local rooms. No shop/literal special/ACT_TEACHER/
epic-teacher/ROOM_INN binding. Dark Knight's highdrop array selector is inactive.
Shared coin refusal/reset admission/key unlock/break/PUSH/fire/wandering/
mode1 renewal dispatch reviewed.

Schema3/revision1 adds one card/two classified recipes/one explicit exclusion,
seven contacts/six aliases/three optional checks(two materials, one earlier
purchase receipt). Supplied matching horns fit lieutenant's exact return for
wrist chain/configured50000 XP without personal purchase, thief kill or rescue.
Worn horns need removal; reward possession/clues/access key do not prove the
accepted return. Historical purchase does not restore spent horns. Adventurer's
coin-only100000-base-value exchange is the sole identified horns producer and
is refused when accounting is active. Dialogue1500 platinum disagrees with
configured100 platinum. Keep excluded until actual payer debit, issuance and
selected NPC retirement are qualified; do not bypass with free stock/fee changes.

Seller spawn35635 has no incoming ordinary route; WEST can wander to lake35544,
EAST can trap him in exitless35634. Preserve deliberate trap/mobility. Current
quest context/target reselection lacks captured NPC instance/epoch; qualify
movement/removal/reset replacement/delayed settlement/disconnect/replay before
shared repairs. Prison entrance is native PICKABLE with asymmetric locked/
unlocked reset and optional iron key. Training gate is PICKPROOF but resets
open. Rare24-percent WORN onyx key can still match native has_key by VNUM;
value1=500 exceeds0–99 break roll, not500 uses. Confirm intent before type/values.
Fixed PUSH wall clears BLOCKED, not CLOSED; it stays TAKE0/weight1200. Fireplane
heat can strip protect-fire spells; journal does not grant immunity/survival.
Instructor/alchemist/library/prison/mines lore supplies no new service endpoint.
All zone item resets are refused under active accounting; declared stock is not
live availability. F followers retain followable context despite item refusal.
Mode1 renews only aged/empty; source-qualified actual daily availability remains
required, even though the exact return is one potential native candidate.

**No native repair ships.** Seven required follow-ups cover sources/renewal,
coin purchase, wandering recipient, access/fire evidence, onyx-key intent,
services and clue consistency. Prefer truthful captions after builder intent
review. Preserve source scarcity, stock, spawn trap, locks, item types and PvP;
actual repairs need separate named fix commits, original-fails/repaired-passes
proof and prominent before/after news. Owner-confirmed Fields old escape hotfix
and required replacement design remain unchanged.

Required checks:source/schema, all107 Python/C++ loader/projection journeys,
full production regression, maintained build, changed/staged formatting,
links/whitespace and preservation/publication proof. All106 prior journals,
2668 native definitions/fingerprint/revision2/registry and original220 queue
stay unchanged. Exact51403-byte prior Orcish Slave Camp PR body is archived
with SHA-256; all ten repair/news/accounting sections retained verbatim.
Catalog107 journals/1583 achievements/1441 potential dailies/2193 rows:explicit
coin-purchase exclusion removes one unsupported fallback achievement/row,
retaining the native definition and historical evidence.
**89/220 source-comprehensive,131 pending; The Cimmerian Nomad Encampment (`nomads`) next.**
Goal remains active. Synthetic receipts do not qualify played source/gifts/coins/
issuance/worn removal/fire/picking/key destruction/control/OPEN/arrival/
wandering/selected retirement/reward settlement/renewal or database persistence.
Active, ready accounting mandatory; frozen recovery separate. No accounting
activation, DB/server operation, migration, deployment or merge.


## Priority 90 checkpoint: The Cimmerian Nomad Encampment

The [complete dossier](zone-stories/THE_CIMMERIAN_NOMAD_ENCAMPMENT.md) closes all
five blocks (two M/one MA/two QA), 49 physical rooms/36 full prose families/five
headers/124 exits/31 relative patterns/two full exit texts/four complete non-exit
metadata families, 21 full mobiles/21 full prose families/23 full objects and
102 resets (55 M/22 E/16 D/four O/four G/one P;90 exact/95 parent-aware families;
60 expanded groups). All 21 mobiles are placed; all chances are100 but caps and
conditional chains matter. Four reciprocal Surface boundaries/full foreign
records reviewed; both item recipes are local, no foreign reset group/consumer
or imported proof. Across713 active type25 prototypes none targets local rooms.
No shop/literal special/ACT_TEACHER/epic-teacher/ROOM_INN binding. Direct leads:
Githzerai prime shift can choose bonfire6224; CHAOS starter profiles grant
crown6222 through durable starter issuance. Shared quest echo/ALL roots/wake/
LOOK/READ/spellbook/SEARCH/NPC corpse/transient decay/snapshot/mode2 reviewed.

Schema3/revision1 adds two cards/two recipes/seven contacts/five aliases/six
optional checks (five exact materials, one earlier evidence receipt). Wood+iron
shards→collateral ring; distinct shaman head+conjurer head+ring→crown and
Septimus departure. All complete current roots must be loose together for
native atomic batch consumption. Both heads use keyword head; duplicates of
one do not supply the other. Worn/nested ring or ordinary blue/green ring does
not fit. Supplied matching bundle fits without own evidence or kill history;
a first receipt never restores collateral consumed later. MA/QA A denotes
room echo, not an ALL-goals opcode or listener credit. D removes Septimus,
not the camp; crown can separately come from CHAOS kit, so possession is not
accepted exchange evidence.

Shards/heads/book/body are hidden; heads also TRANSIENT/NORENT. G places heads
on living strangers; no CARVE producer or death-state/personal-killer predicate
in exact hand-in. Qualify hidden source/reveal/GET/gift/steal/corpse/floor/
non-death removal/transient decay and actual snapshot/recovery rather than
invent personal death credit or strip flags. Snapshot preserves active durable
custody despite legacy no-rent omission. Blacksmith starts prone/sleeping and
quester refuses sleeping status; native WAKE has ordinary admission, not a
new mandatory stage. Rellius page1/page2 are visible extra-description clues;
READ delegates to LOOK and no page-specific story fact is recorded. Journal
is SPELLBOOK33 with108-page capacity, not spell108. Rellius body is fixed
hidden CONTAINER15/TAKE0/weight165, not normal combat corpse or recovery quest.
Preserve source/P sword roles. Instructors, infirmary, vanished weapons, bound
creatures, horses and prophecy have no extra local service/quest endpoint.
Bonfire is MAGIC_LIGHT/CITY, not elemental-fire terrain. Doors reset closed/
unlocked; OPEN and arrival differ. Plains prose versus desert Surface borders
needs intent review. Mode2 can reset while occupied; current template/room
recipient reselection lacks NPC instance/epoch. All zone item reset issuance
is refused during active accounting; actual source availability/renewal is
required before promising reliable dailies.

**No native repair ships.** Seven required follow-ups cover sources/renewal,
hidden transient proof, collateral allocation, optional investigation evidence,
recipient retirement, service/prop intent and boundary/clue consistency. Builder
chooses material-only trophies versus real death prerequisite before changes.
Any repair needs separate named fix, original-fails/repaired-passes proof and
prominent before/after news. Preserve pickup, hidden/decay flags, scarcity, types,
closed doors, existing travel and PvP; Fields old escape hotfix unchanged.

Required checks:source/schema, all108 Python/C++ loader/projection journeys,
full production regression, maintained build, changed/staged formatting,
links/whitespace and preservation/publication proof. All107 prior journals,
2668 native definitions/fingerprint/revision2/registry and original220 queue
stay unchanged. Exact52600-byte prior Lava Caves PR body is archived with SHA-256;
all ten repair/news/accounting sections retained verbatim. Catalog108 journals/
1583 achievements/1441 potential dailies/2193 rows; two native fallback outcomes
become two authored cards without changing aggregate achievement/daily/row counts.
**90/220 source-comprehensive,130 pending; The Ruins of Undermountain (`undermountain`) next.**
Goal remains active. Synthetic receipts do not qualify played wake/page reading/
SEARCH/GET/source/gifts/steal/death/looting/CARVE/decay/save/plane shift/OPEN/
exact native batch/reward settlement/selected retirement/renewal or database
persistence. Active, ready accounting mandatory; frozen recovery separate.
No accounting activation, DB/server operation, migration, deployment or merge.


## Priority 91 checkpoint: The Ruins of Undermountain

Priority 91 closes source review for zone920, registry91168–92519, area
`undermountain`, reset mode1. The [schema3/revision1 sidecar](../../areas/story/undermountain.story.json)
classifies both native hand-ins as two linked cards, three contacts, eight
addressed aliases and three optional checks: intact key, earlier Tamsil receipt,
and current note. The [generated audit](../reference/zone-story-audits/undermountain.md)
preserves exact contracts. **No native zone or quest repair ships.**

Every new discovery, encounter, journal, achievement and daily credit requires
active, ready accounting. Frozen reward recovery remains separate. Preserve
scarcity, hidden/invisible flags, key breakage, deliberate disabled controllers,
rare-monster distribution, locks, fixed portals and PvP. Source-comprehensive
mapping is not played source, transaction, retirement or renewal qualification.


## Full source and dispatch closure

| Source | Complete review and implications |
| --- | --- |
| [Quests](../../areas/qst/undermountain.qst) | All five blocks: three M responses/eight aliases, one Q and one QA. Full daughter/prison/bravery/departure messages reviewed. Both inputs are exact items; no qc_action, personal death, reading or unlock prerequisite. QA A means room echo, not listener credit. |
| [Rooms](../../areas/wld/undermountain.wld) | All441 physical records,92001–92519 with gaps;332 full prose families,16 headers,23 complete non-exit metadata families,1059 exact exits,354 relative patterns and201 full exit-text families. All destinations resolve. Lower registry bound does not invent additional physical rooms. Full pillars/messages, library, coffins, traps, blood, prison, snowy forest, dungeon occupants and concealed controls reviewed. |
| [Mobiles](../../areas/mob/undermountain.mob) | All95 prototypes92000–92094,94 full prose families and every numeric tail. Nine have no local reset:92003–06 inn family/patrons,92020–22 adventurers,92054 and92092. No imported mobile reset. Unplaced or dormant NPCs are not promised contacts. |
| [Objects](../../areas/obj/undermountain.obj) | All135 complete prototypes92000–92134, flags/values/effects/traps/descriptions. Eighteen have no local placement:92032,92035,92036,92054,92063,92064,92071,92075,92085,92089–91,92112,92114,92120,92121,92131,92134. Reward-only note/scimitar and foreign Flame placement are distinct from unreachable stock. Full imported283 sword/359 rune node/364 rations/998 wine barrel reviewed. |
| [Resets](../../areas/zon/undermountain.zon) | All731 commands:256 M/226 D/131 E/55 O/44 P/11 G/8 F;509 exact,548 M-parent-aware and274 expanded families. Chances100 retain caps/conditional chains. Essra carries cap1 grate key; note/scimitar have no reset producer. Mode1 renewal requires an empty zone; retirement and global stock matter. |
| Boundaries/global sources | Reciprocal92501 south/74045 north links Svalich.92518 down→Underworld4557 is one-way;4557 up leads54841, not back. Both complete foreign records reviewed. Across713 active type25 objects, only two local portals target local rooms. All22 touching foreign reset groups and the foreign quest consumer reviewed: Icecrag consumes two wines92048 plus two venisons90017; Brass Fingers can carry mithril picks92046; FirePlane places Flame92121; Caertannad uses20 extra92051 mobiles. These remain their owning zones' outcomes. |
| Custom dispatch | [Assignment source](../../src/specs/specs.assign.c) has the entire Undermountain NPC block inside `#if 0`. Six literal mobile assignment leads are inactive; empty-VNUM companions are inactive too. [Full dormant routines](../../src/specs/specs.undermountain.c) include nine-weapon/inn lore, following adventurers, hiring, Essra narration, dagger/corpse/death transformations and Malodine/black-pudding code. No reactivation. Eight active object bindings:92090 undead trident;92080/81/82/86 sunlight-sensitive drow gear;92065 NPC flindbar disarm;92020 shared parry;92121 Flame's combat event. These are equipment mechanics, not hand-in contracts. Generic type29 switch is assigned by the loader. |
| Shared execution | [Quest consumption/echo/retirement](../../src/world/quest.c), [key identity/unlock/break](../../src/cmd/actmove.c), [SEARCH visibility](../../src/cmd/actobj.c), [NOTE/READ](../../src/cmd/actinf.c), [WRITE](../../src/cmd/actcomm.c), [door/reset/item loader](../../src/world/db.c), [generic switches](../../src/specs/specs.object.c), [renewal](../../src/world/events.c) and [shared parry](../../src/combat/defense_resolution.c) reviewed. Avernus staff can summon prototype92076 elsewhere; [staff controller](../../src/specs/specs.avernus.c) and staff test-command lead do not establish local arrival or another quest. |


Two native independent outcomes link current material: intact key→Tamsil note,
then exact note→Convalescence scimitar. Supplied note fits without own earlier
rescue. Earlier receipt cannot supply a consumed note. Normal keyed unlock has
a100% break roll, separate accounting destruction settlement and a competing
need for intact offering. Pickable/previously accessible route is guidance, not
a fabricated unlock prerequisite. Note has no authored letter text and remains
writable; hidden reward/source need qualification. D removes Tamsil, not a
physical escort/reunion. NPC specials are compiled out; combat item bindings
remain active. Preserve unplaced lever, deliberate rare holding, fixed portals
and PvP. Seven required follow-ups:

| ID | Finding and fair plan before implementation |
| --- | --- |
| **ZSQ-UNDERMOUNTAIN-SOURCE-RENEWAL** | Qualify source/root/UID/custody, cap1 stock, gifts/loot/rewards, active item-reset refusal and actual empty-mode1 renewal. Preserve scarcity. Daily availability needs real current materials or qualified producer/restock; no free stock or accounting bypass. |
| **ZSQ-UNDERMOUNTAIN-GRATE-KEY-ALLOCATION** | One key is competing unlock/offering material, with100% break roll. Qualify exact held/loose/nested selection, break destruction admission/settlement, reciprocal door state, alternate legitimate access, fresh key, concurrency/retry/rollback and repeated exchanges. Builder decides whether current deliberate break/stock design is intended; any change separate fix/proof/news. |
| **ZSQ-UNDERMOUNTAIN-PROOF-VISIBILITY** | Hidden/invisible source and hidden directly delivered reward can be inaccessible to ordinary selectors. Qualify SEARCH/container/corpse, invisibility, source GET/gift, loose reward inspection/equip/save/recovery. Confirm builder intent before minimal flag/source/presentation repair; do not strip protection merely to satisfy a journal. |
| **ZSQ-UNDERMOUNTAIN-NOTE-PROVENANCE** | Exact note has no authored letter and is writable. Builder chooses informational text versus real signed rescue proof. Optional learned/source facts need admitted actor, itemUID, writer/content revision, selected output, source versus gift and explicit prerequisites. Changing accepted proof semantics needs contract/version/historical receipt compatibility; no fabricated read or personal rescue requirement. |
| **ZSQ-UNDERMOUNTAIN-RESCUE-RETIREMENT** | D retires Tamsil, with no escort/reunion controller; target reselection lacks captured NPC instance/epoch. Qualify original target versus reset/removal/delayed settlement/disconnect/replay, eligible actor/group versus listeners and any real rescue transition. Keep narration truthful; shared repair separate named fix/tests/news. |
| **ZSQ-UNDERMOUNTAIN-ACCESS-EVIDENCE** | PICK/KNOCK/UNLOCK/switch/portal/SEARCH and accepted arrival differ. Universal optional facts need selected door/objectUID, actor, room/dir, prior/accepted state and settlement. Qualify blocked/secret/locked modes, pre-open or other-player access, denied attempts/replay/cold recovery. Unplaced lever and deliberately rare holding rooms remain builder intent questions; preserve current controls/topology/PvP. |
| **ZSQ-UNDERMOUNTAIN-DORMANT-STORY-INTENT** | NPC bindings are compiled out; object combat procedures remain active. Builder chooses retirement/truthful lore or a newly designed controller. Before any reactivation, resolve absent IDs, allocation/event guards, per-instance state, paid hiring/pet lifecycle, source stock/rewards/accounting and balance. No blanket enablement or unqualified monster-drop/inn/weapon-collection credit. |


**91/220 source-comprehensive,129 pending; Desolate Under Fire (`desolateinv`) next.**
109 journals/1583 achievements/1441 potential dailies/2193 rows. Native
definitions/fingerprint/content revision2/registry and earlier108 journals stay
unchanged. Exact53861-byte prior Nomad Encampment PR description archived with
SHA-256; ten repair/news/accounting sections retained verbatim. No native repair,
accounting activation, DB/server operation, migration, deployment or merge.
Source/projection checks do not qualify played quests or renewal; goal active.


## Priority 92 checkpoint: Desolate Under Fire

See the [complete dossier](zone-stories/DESOLATE_UNDER_FIRE.md) and [audit](../reference/zone-story-audits/desolateinv.md). All17 requests/165 rooms/110 mobiles/96 objects/332 resets/full shop reviewed;346 exits/118 full room-prose/108 mobile-prose families. Eight rescues remain independent; Beregan accepts eight current roots without own rescue history. Jandar needs three hands without personal kills. Return monkey before hunter retirement; remove nested chain before container consumption. Three unplaced givers, absent wheel producer, blade/drink mismatch, legacy Surface source choice, inn mismatch and unplaced rock controls remain builder work. Two direction words corrected in separate fix/news above.

| ID | Finding and fair plan before implementation |
| --- | --- |
| **ZSQ-DESOLATEINV-SOURCE-RENEWAL** | Qualify source UID/root/custody, hidden stock, gifts, caps/conditions, mode2 renewal and active item-reset refusal. Repeat lines are not free stock. Keep no-exit shard-check room inaccessible. Sixteen potential dailies need legitimate supply/renewed actors. |
| **ZSQ-DESOLATEINV-CAPTIVE-BINDING-ALLOCATION** | Eight independent rescues consume one shard each; Beregan consumes eight distinct current roots, not eight types or eight own rescue records. Qualify exact count/duplicate rejection, seven-versus-eight, gifts/lost/spent bindings, concurrency/retry/rollback/output. A personal rescue campaign needs separate admitted objectives and actor/group policy. |
| **ZSQ-DESOLATEINV-CONTAINER-CONSUMPTION** | Closed lost-monkey container holds hidden neckchain. Remove chain before consuming monkey if keeping minotaur route. Qualify OPEN/SEARCH, nested destruction/custody/gift/rollback/key chest access. Container/reward does not prove escort/first recovery. No flag/type/pickup change. |
| **ZSQ-DESOLATEINV-INVASION-PHASE** | Normal Desolate random_exit22291 redirects22200 north/77302 south, closes former zone and opens773. Initial CLOSED, source20% and callback100% differ. Legacy surf217609 has two differing sources outside current registry. Verify production source choice/current entrance/status/accepted episode/route generation/return before topology repair. Discovery and hand-in narration do not settle restoration. |
| **ZSQ-DESOLATEINV-RECIPIENT-RETIREMENT** | D retires eight captives/minotaur/driver/Jandar/Beregan. Monkey return before hunter rescue preserves both. Qualify selected NPC instance/epoch, delayed settlement/reset/removal, reconnect/replay, actor/listener credit and restock. Native target reselection repair needs separate proof/fix/news. |
| **ZSQ-DESOLATEINV-UNPLACED-REPAIR-AND-DRINK** | Halfling/driver/quest merchant unplaced. Halfling consumes blade despite drink caption; repaired wheel has no producer; placed Scotson has no repair recipe. Builder chooses exact availability/drink/wheel/fee/reward semantics or retirement. Do not copy normal recipe, substitute IDs, spawn givers or enable unplaced switches. Changes need contract/receipt versioning and separate fixes. |
| **ZSQ-DESOLATEINV-TRIAL-ACCESS** | PUSH ring/secret controls/fixed portals/falls/guardians/epic Rune differ. New optional control/access/trial facts need actor/UID/door/room/direction/prior-accepted state/phase/output and accounting/recovery. No victory from loot or pre-open route. Unplaced rock targets lack exits. Preserve pending intent; only two proven direction words repaired. |
| **ZSQ-DESOLATEINV-SERVICE-AND-CLUE-INTENT** | Coin-only purchase/paid lockers guarded; shop needs live stock/payment/refund proof. Inn assignment targets tunnel77442, closed inn prose77422: builder intent before relocation/reopening. Hands/heads, medal/tankard and cloak/earring are stale captions, not authority to change item kinds. Separate exact prose repairs after intent; transaction extension only deliberately designed. |


**92/220 source-comprehensive,128 pending; Storm Port Stronghold (`spshold`) next.**
110 journals/1582 achievements/1441 potential dailies/2193 rows. Coin purchase becomes service, removing one fallback achievement; native2668 definitions/fingerprint/revision2/registry and earlier109 journals intact. Exact52154-byte previous PR archived with SHA-256; ten earlier news/accounting sections retained verbatim. No operational changes/played qualification; goal active.


## Priority 93 checkpoint: Storm Port Stronghold

See the [complete dossier](zone-stories/STORM_PORT_STRONGHOLD.md) and [audit](../reference/zone-story-audits/spshold.md). All4 Q/65 rooms/44 mobiles/34 objects/129 resets;54 full room-prose/44 mobile-prose/130 exits/98 exact-104 parent-aware-87 expanded reset families reviewed. Full master/crew procedures and foreign map/spade/chest/recipe/boundary closure. Captain bundle and ticket/helm are exchanges, not launch/travel. Hordine accepts two different maps independently; own referral/source history not required. Aphantan starts in a no-incoming hub and can roam; sea maps compete with Oberon helmsman, not Mui Pai. Burgadan returns torn map+spade; silt is keyed access, chest needs separate key, both100% break roll. Required shared defect:crew shop ignores refused debit then mutates/saves crew/chief. One direction word ships in separate fix/news above.

| ID | Finding and fair plan before implementation |
| --- | --- |
| **ZSQ-SPSHOLD-SOURCE-RENEWAL** | Qualify hidden cap1 coal/valve/maps, source UID/custody, rewards/gifts, conditional resets, roaming and mode2 renewal. Preserve active item-reset refusal and source rarity; ticket has no reset producer. Four potential dailies need legitimate current supply, actual encounter and ready accounting. |
| **ZSQ-SPSHOLD-TICKET-TRAVEL** | Coal+valve gives ticket; ticket gives helm. Neither launches/moves the Drifter or Decker, pays fare or proves arrival. Builder chooses truthful informational ticket lore versus real transport. New departure/fare/route/arrival facts need actor/vessel/route generation/admitted state/output, party/faction policy, safe return and durable accounting/recovery; no movement activation by inference. |
| **ZSQ-SPSHOLD-ROAMING-MAP-ALLOCATION** | Aphantan starts in hub22649 with no incoming route and can roam out. Sea maps compete between Hordine and Oberon helmsman; torn map is different. Qualify current NPC/room/instance/custody, visibility/loot/gifts, consuming uses, concurrency and renewal. Do not move stock/NPCs or open load-room access as convenience. |
| **ZSQ-SPSHOLD-TREASURE-ACCESS** | Burgadan returns torn map+key-type spade before Hordine consumes map for chest key. Supplied proofs fit without own referral. Silt is secret/locked keyed access, not DIG; spade/key have100% break roll. Qualify selected exact key, reciprocal state, destruction settlement, replacements, underwater return/chest loot and retries. First treasure discovery/access/loot needs separate admitted objectives, not new Stronghold hand-in credit. |
| **ZSQ-SPSHOLD-CREW-PAYMENT** | Full crew_shop_proc ignores SUB_MONEY result, mutates crew/chief and queues save; SUB_MONEY returns−1 under active accounting. Pending-wallet interpreter fence does not prevent this failure. Required shared fix: refuse active HIRE until typed payment/service settlement exists; then enforce failed/pending debit isolation, ship-owner/crew version, positive fee/frags-or-skills/faction admission, concurrency/retry/refund/reconnect/recovery. Separate named accounting fix with original-fails/repaired-passes tests and explicit PR/news. No shared repair or live exploitation test ships here. |
| **ZSQ-SPSHOLD-LEARNED-SOURCE-FACTS** | Read map/spade/ticket lore, first source versus supplied gift, personal kill, SEARCH/OPEN/UNLOCK and arrival are not completion facts. Universal optional facts need admitted actor/selected UID/NPC/door/content version/prior-accepted state/output, denied attempts, replay/cold recovery and historical compatibility. Use current-material/optional-receipt schema3 now; no invented mandatory history. |
| **ZSQ-SPSHOLD-CONTENT-MOBILITY-INTENT** | Merchant/inn/launch/ballista/slave-rescue/Hunter-Killer lore lacks corresponding local story controllers. Builder decides truthful retirement or intentional implementation; keep static portals/locked interiors/load hub/NO_GATE/faction restrictions. Outside direction is proven one-word repair; no topology/source/disabled mechanic fix inferred. |
| **ZSQ-SPSHOLD-EQUIPMENT-FOREIGN-OWNERSHIP** | Helm set effects and foreign avatar gear are separate from Decker credit. Adapter includes82559 while master count list omits it; qualify intended membership before any effect/set achievement change. Imported greaves40771 differ from unused22626; globe/boots/holy relic have other owning-zone exchanges. Keep foreign receipts/loot/source discovery separate and preserve contracts/versions. |


**93/220 source-comprehensive,127 pending; The Mountain Settlement of the Harpies (`harpyht`) next.**
111 journals/1582 achievements/1441 potential dailies/2193 rows; native2668 definitions/fingerprint/revision2/registry and earlier110 journals unchanged. Exact51634-byte previous PR archived with SHA-256; eleven earlier news/accounting sections retained verbatim. No operational/shared payment changes or played qualification; goal active.


## Priority 94 checkpoint: The Mountain Settlement of the Harpies

See the [complete dossier](zone-stories/THE_MOUNTAIN_SETTLEMENT_OF_THE_HARPIES.md) and [audit](../reference/zone-story-audits/harpyht.md). All3 Q/161 rooms/44 mobs/19 objects/176 resets/2 shops/4 bound specials reviewed;59 room prose/44 mobile prose/357 exits/149 exact-151 parent-aware-84 expanded reset families. Full nine imported objects, three foreign recipe/giver records, head/ore outputs and Surface boundary reviewed. Normal special precedence shadows khan native feather receipt; queen shackles Q is recognition, not good-path transformation. Both custom allegiance functions lack addressed-recipient/Harpy-PC admission; corpse transformation and exit gate are commented. PULL ladder and ENTER cage access retained. No native repair ships.

| ID | Evidence and fair implementation plan |
| --- | --- |
| **ZSQ-HARPYHT-DISPATCH-RECIPIENT-ACTOR** | Both queen/khan specials precede qst_func, parse only the item and accept the same feather; non-neutral GIVE is intercepted before canonical hand-ins. Neither verifies addressed recipient or Harpy/PC actor. Qualify exact addressed NPC, awake/visibility/actor/race eligibility, item UID, pending operation and intended one-time policy; unrelated-recipient/player gifts must do nothing. Decide queen shackles versus feather design explicitly rather than silently rewriting the native quest or PvP allegiance. |
| **ZSQ-HARPYHT-ATOMIC-LIFE-PATH** | Direct extract_obj plus race/racewar/alignment mutation has no accepted quest receipt or coordinated actor/item publication. Refuse unsafe active-accounting custom conversion until typed settlement is available; commit selected item retirement and exact prior→new actor state together, with actor version, failure isolation, concurrency/replay/reconnect/cold recovery and historical policy. Persist a distinct one-time branch outcome; do not count the shadowed native khan Q as a daily or grant both mutually exclusive paths. Separate named shared fix/news and original-fails/repaired-passes tests required. |
| **ZSQ-HARPYHT-UNDEAD-PATH** | ASK gargoyle undead counts two floor NPC corpses by lowercase harpy substring, while advertised unlife fails the matcher. Consumption/transformation/Bard→Spiper code is commented out; two corpses yields handled silence. Preserve the disabled transformation until intent is established. Choose truthful retired lore or an intentional atomic corpse/actor conversion; qualify exact NPC corpse identities, ownership/root children/decay, allowed supplied corpses versus personal kills, actual recipient, race/class/stat changes and recovery. Do not uncomment legacy mutation as a journal fix. |
| **ZSQ-HARPYHT-SOURCE-RENEWAL** | Hidden cap1 key O31240, cap2 feather G on two crows, dwarf D retirement and reset mode2 govern supply/recipient renewal; active item resets remain refused. Supplied exact key/shackles fit without own source/rescue. Qualify UID/custody/current visibility, retired instance, legitimate reset issuance and repeated source availability before offering two potential dailies. No stock/cap/pickup/retirement edits by convenience. |
| **ZSQ-HARPYHT-ACCESS-PRESENTATION** | ENTER cage31103→31240 with ordinary DOWN return; other cages are OTHER props despite locked prose. Ladder uses PULL340, not CLIMB556; DOWN returns from31210. Sign promises destiny gate but harpy_gate assignment is commented. Preserve commands/portals/falls/disabled exit policy; explain actual access and review mismatched prose deliberately. Dynamic journal availability needs admitted neutral actor state/recipient dispatch, not an assumed universal racewar prerequisite or a fabricated unlock/travel receipt. |
| **ZSQ-HARPYHT-LEARNED-SOURCE-FACTS** | Corpse-count ASK and LOOK hint have no learned completion; current items, source GET/gifts, own kills, reading, OPEN and entering cages are different facts. New optional semantic facts require admitted actor/NPC/UID/room/instance/content/state/output and rejected/replayed/recovered evidence. Existing schema3 material/optional receipt checks suffice now; do not make personal history mandatory from good-deed or kill narration. |
| **ZSQ-HARPYHT-FOREIGN-SOURCE-OWNERSHIP** | Egg→Wicks coins, nested plans→engineer head/retirement and gigasaur tail→two ores belong to Divine Home/Ultarium. Egg/tail are floor/G stock, not CARVE; remove plans from skeleton before offering. Golem prop and hidden artifact lack local custom quest bindings; buried prose is not BURIED metadata. Preserve foreign contract/reward/source ownership and discovery; actual golem activation, archaeology or crafting requires intentional builder design and admitted output. |
| **ZSQ-HARPYHT-SERVICES-LORE** | Merchant’s full native shop and seven imported supplies, Merv’s retired exchange redirect/empty shop/counter sign, hometown/inn flags, fauna/nesting and mining object193 are separate services/lore. Verify actual command/payment/location admission before changing advertised services or awarding deeper objectives. Preserve retirement and native home/inn placement; clarify story versus service without manufacturing bank/mining/merchant achievements. |

Catalog111 journals/1581 achievements/1440 potential dailies/2192 rows; Harpies provisional mapping replaced, other110 mappings/native definitions/fingerprint/revision2/registry intact.94 source-comprehensive/126 pending; Behemoth Herders next. Native mechanics and all earlier repair news preserved; active accounting mandatory.


## Priority 95 checkpoint: The Behemoth Herders

See the [complete dossier](zone-stories/THE_BEHEMOTH_HERDERS.md) and [audit](../reference/zone-story-audits/herders.md). All12 Q/QA,29 M/MA,347 rooms/143 prose/842 exits,144 mobiles/140 prose,97 objects,624 resets/523 exact-534 custody-aware-286 expanded families/full shop reviewed. Four imports,13 touching recipes,32 foreign reset groups,three full foreign boundary records,713 portal declarations and automatic/table-driven shared execution close the source audit. Six independent stories are potential dailies, including two XP rescues; six crafts stay services and four mixed fees remain guarded. Player copy uses named places, exact materials and commands. No native repair ships.

| ID | Source evidence and fair implementation plan |
| --- | --- |
| **ZSQ-HERDERS-SOURCE-CUSTODY-RENEWAL** | Reset mode1, shared item caps, Viscerith soul G chance30, four retiring recipients and hidden/nested material govern availability. G/E receiver is the most recently successful M/F/R mobile, not always the M master: dragonkin scales belong to F94317; horns to F94323; choker to F94379. Zugle’s skull is before F and stays on Zugle. P selects a live matching container globally, rather than necessarily the immediately preceding O. Preserve scarcity and actual parent UID; admit legitimate reset generation, instance/ownership/children and current material before daily rollout. Lifeforce94385 has NORENT; qualify durable save/reconnect/cold recovery policy before claiming it persists. Active accounting refuses item reset issuance; no convenience stock/cap/flag changes. |
| **ZSQ-HERDERS-SHARED-ACCESS** | Two PULL cranks, PUSH boulder and PUSH statue remove shared EX_BLOCKED flags; blood pool/rift use ENTER with ordinary return exits. Seven key families and search/current/container routes are real access, not personal quest history. Keys have value[1]=0, so generic UNLOCK does not run positive-probability key breakage for these prototypes; held/loose keys work, worn collar must be removed. Define exact admitted successful control/room/exit/instance/version/actor events only if builders require personal operation. Qualify paired state, resets, rejection, danger and return without requiring an own-key receipt when a route is already open. |
| **ZSQ-HERDERS-STRONGHOLD-RETURN-INTENT** | Door94623 east uses key94390 while reciprocal94634 west uses key0; both reset closed/locked. Generic UNLOCK from the keyed side clears both locks, but independent relocking/reset may leave the reverse side unusable with the expected key. Establish deliberate trap versus data error before choosing a repair. Test original/repaired admission in both directions, reset timing, alternate escape and PvP policy; any actual fix must be a separate named fix/news commit. Do not silently assign a key or remove the lock. |
| **ZSQ-HERDERS-MIXED-FEE-CRAFTS** | Argle’s sleeves/leggings/breastplate need20000 copper each, boots35000; current durable offering supports only item goals and refuses these mixed inputs. Exact repeated roots, distinct small/large bones and competing diorite consumers matter. Retain guards and six service classifications. Future selected item-root/coin settlement must consume all exact counts and fee atomically, create/recover one exact output, reject shortages/unrelated recipient/stale stock, and isolate failure/replay/recovery. No invented fee-readiness predicate or service daily. |
| **ZSQ-HERDERS-NARRATIVE-EPISODES** | Native D retires gnome/shade/Pearls/warlock; dialogue narrates restoration, cutting bonds, healing or transformation without additional admitted actions, replacement NPCs or player state. Infant is object94366 held by Persevesk, not a live baby actor; eyepatch corpse is authored container94334, not a personal NPC kill/decay receipt. Preserve exact hand-ins and permitted supplied proofs. New rescue/escort/healing/creature transitions require deliberate episode policy, actor/NPC/item instance and prior→new state, lifecycle/children/decay and coordinated accounting publication; text alone earns no separate objective. |
| **ZSQ-HERDERS-LORE-TRAINING** | All29 M/MA families are verified topic guidance; public MA messages are not durable personal knowledge. Archazel is assigned epic_teacher through epic initialization, with full-name PRACTICE/class/race/level/skill/payment guards and active-accounting refusal. Training is separate from his sword stock and public choke response. Semantic accepted-topic/skill-change adapters need admitted recipient/content/output and replay/history policy. No achievement per keyword, ambient bystander credit or inferred training success. |
| **ZSQ-HERDERS-FOREIGN-MATERIAL-OWNERSHIP** | Shaenae43156 atFenaline43268 owns ring94307→E155000+C200000/D1. Ring stock in a bone pile is distinct from stud94384 and from local rescues. Thirty-two foreign reset groups use arrows94336 or spikes94381, without foreign producers of the local quest proofs. Preserve foreign discovery/contracts/outputs and actual globally selected P parent; local holding/exploration cannot complete the foreign journal. Prove gifted versus first-source history only through admitted source/custody events. |
| **ZSQ-HERDERS-PRESENTATION-ORPHANS** | Large bone94306 LOOK calls it a small fragment; finished eyepatch94341 describes ten scales although accepted inputs/dialogue require three. Spring94359 is an OTHER prop, barkeep has no separate shop record, and four local mobiles94363/94368/94377/94378 are unplaced. Explain current functionality now; review intended text versus ornamental craft detail and intentional unused prototypes. A later caption correction must preserve IDs/counts/flags/stock and have original-fails/exact-byte proof in a separate fix/news commit. Do not turn scenery or unused monsters into quests by inference. |
| **ZSQ-HERDERS-CAMPAIGN-POLICY** | Six stories are independent item-only contracts and potential dailies; six crafts are services. Left/right halves, five separate eyes, sword+soul and diorite competing crafts need exact current roots; no larger accepted-all-stage campaign exists. Future optional campaign completion needs an explicit builder-defined policy, prerequisite versus source hint distinction, optional supplied routes, material consumption and one-time/repeat/recipient/branch rules. Existing schema3 preparation/terminal checks suffice until that policy and actual semantic evidence are qualified. |

Catalog112 journals/1575 achievements/1438 potential dailies/2192 rows; native2668 definitions/fingerprint/revision2/registry and earlier111 mappings preserved.95 source-comprehensive/125 pending; Jotunheim next. Exact51891-byte earlier PR body is archived with SHA-256, and prior repair/news/accounting sections remain verbatim. Source review/projection is not played accounting/reset/rescue/access/XP/return/persistence qualification. Goal active.


## Priority 96 checkpoint: Jotunheim

See the [complete dossier](zone-stories/JOTUNHEIM.md) and [audit](../reference/zone-story-audits/jotun.md). All15 Q/36 physical M (34 ASK families/47 aliases plus two ambient timers),296 rooms/278 prose/746 exits,77 mobiles,82 objects,392 resets/301 exact-350 actual-custody-212 expanded families and the entire custom procedure file reviewed. Four full imports,17 touching recipes,one full foreign reset group,713 portal declarations and complete relevant generic/table-driven callers close the source audit. Eleven independent item-only stories remain potential dailies including XP rewards; four rejection/craft responses are services and mixed fees remain refused. Player copy gives named places/materials/commands. No native repair ships.

| ID | Source evidence and fair implementation plan |
| --- | --- |
| **ZSQ-JOTUN-SPOKEN-ACCESS** | Gate96197S↔96198N uses key−2/last keyword kostchtchie; wall96215E↔96252W uses key−2/last keyword silverwing. Both reset closed/locked, wall also secret. Successful SAY clears lock/secret on reciprocal sides, leaves CLOSED: OPEN then move. Badge/allegiance prose creates no item/race/faction gate. Define successful actor/room/exit/instance/version/state events only if builders require personal operation. Preserve shared-open routes; qualify rejection/silence/reset/return/accounting publication. Hidden boulder/stair/tear/cobweb doors are ordinary access. |
| **ZSQ-JOTUN-RARE-SOURCE-RENEWAL** | Mode1 stages rare actors at96288–96293; trap96294 has no exits, distributor96295 has six exits to96040/96104/96088/96132/96214/96153. NPC wander uses NUM_EXITS10, no-move draw, CAN_GO/sector/master/sentinel guards and last_direction suppression. Percentages5.2/10.4/20.8/41.6/62.5/83.3 are historical labels, not qualified live probabilities. Preserve caps/positions/chances; audit actual scheduler/lifecycle distribution and recipient renewal before daily activation. Active accounting refuses item reset issuance. Admit legitimate stock generation/UID/custody; no guaranteed-spawn convenience change. |
| **ZSQ-JOTUN-COMPETING-BRANCHES** | Sword96038 is consumed by Mimir’s five-item bundle or Quelranor. Standard96060 and totem96061 differ although rivals share reward96062. Brunnhilde’s requests are independent; cloak retires her. Sarimar/trolls/Olaf/Quelranor also retire. Supplied proofs fit without own kills. Define deliberate branch/recipient/campaign policy before exclusive/aligned/full-clear objectives. Preserve independent historical receipts; shared reward/spent root cannot complete another request. |
| **ZSQ-JOTUN-MIXED-FEE-SERVICES** | Ordinary96069+C250000→96070 and ancient96071+C250000→96072 are services; durable offering refuses mixed inputs under accounting. Smith96058 is also assigned by initialize_tradeskills after boot_db, menu103–112/119; full smith selection/ore/payment/output/rollback flow has its own accounting refusal. Preserve both guards/materials/fees. Future admitted root+wallet settlement must be atomic, count-correct, owned and recoverable. Fee readiness/unrelated FORGE results earn no story credit. |
| **ZSQ-JOTUN-MIMIR-OVERRIDE-LORE** | assign_mobiles binds jotun_mimer; later normal startup epic_initialization/epic_points assigns epic_teacher96013. Old WEST level51/IS_GIANT/return-to-birth procedure is not normal active execution. PRACTICE has full summon-blizzard name/class/level/max/payment/accounting guards and falls through for ASK/GIVE/movement. Greeting/Well wisdom is lore; holy water17 heals good/harms evil, no wisdom grant. Choose combined trainer/access intent before restoring/changing a gate; test ordering/periodic registration/fallthrough/return/race-level policy.34 ASK families/47 aliases are hints; two qc_action50 timers are ambient, never player topics. |
| **ZSQ-JOTUN-NARRATIVE-RESCUES** | Silverwing96036 is Telshanar, wears cloak96042 at96286; Brunnhilde accepts proof and mourns, without freeing/healing/escort/replacement. Olaf’s mask receipt does not liberate slaves/change faction. Mimir’s bundle does not verify every mage dead/permanent extinction. Prisoners/frozen figures/illusion king96043/kitchen slaves are lore/stock. New episodes need chosen builder semantics, real actor identity/state, admitted transitions and restart/replay/party policy. |
| **ZSQ-JOTUN-PROOF-IDENTITY-CUSTODY** | Distinct jade96036/sword96038/whip96039/quartz96037/sapphire96081 are required; sapphire is ground item, not secret/container stock. Scales are G stock, not CARVE; generic CARVE creates8. Tankard96023 hand-in checks identity only, not ale/type/origin. Rejections96035/96046 create same-prototype outputs, no original-instance restoration guarantee. Fireweed96076 is NORENT. Preserve loose exact ownership/supplied routes; admit origin/transfer/liquid/content evidence before personal collection/brewing guarantees. Qualify cold save/reconnect/reset issuance. |
| **ZSQ-JOTUN-COMBAT-PROC-REVIEW** | Four mobile/six object procedures and callers reviewed. Balor death uses caster infravision for target blindness, sends some victim warnings to caster, subtracts nonfatal HP without update_pos and can call nested die; ACT_SPEC_DIE bypasses make_corpse, ordinary extraction drops surviving gear. Loki returns inside first eligible non-giant loop, overlaps fear branches and uses null TO_VICT audience. Faith checks retained original opponent trust for room-target effects across kill-capable spells; null-group targeting needs intent review. Deva cloak has similar null-group question. Build isolated original-fails/repaired-passes audience/lifetime/group/death matrices; preserve proc chances/damage150-250/durations/target policy/PvP balance. Actual repair must be a separate named fix/news commit. Proc text creates no quest receipt. |
| **ZSQ-JOTUN-KEY-RETURN-INTENT** | Thrym gate96189N uses Loki’s glowing ice key96007, reverse96190S key0/reset closed-unlocked; relocking can expose mismatch. Storeroom key96006, cells96014; all key break fields0. Determine return/trap intent before symmetric-key changes; qualify reset/return/pick/lock/scarcity. Cell96276 prose says east, actual return south. Guide correctly now; any caption fix needs separate narrow fix/news commit and exact-byte proof. |
| **ZSQ-JOTUN-UNFINISHED-ORPHANS** | Muspelheim is under construction;96073 only DOWN. Destroyed ferry96111 only SOUTH, older sign still advertises service. Registry95925–96295 contains actual96000–96295/staging;96004 DOWN−1 is NOWHERE. Mobile96066/boots96026/chest96080 have no active reset/quest/literal special producer in closure; chest key96007 cannot make it available. Imported fountain72 is TRASH scenery; node359 separate epic stone. Establish scenic/unused/unfinished intent before adding travel/stock/actors/rewards; actual fixes separate named fix/news commits. |
| **ZSQ-JOTUN-FOREIGN-OWNERSHIP** | Astral19701/O19733 ENTER→96004; local96005/O96004 ENTER→19733. Mundorno83342/M83655 native QA5707:83377+83191+C100000→leggings96021. Fearfrost131637/M131772/chance50 Q240:131647+96000+96012+96055→131648. Local Frostbite E chance40 retained. Shabo Jabulanth32829/M32866 carries outputs96077/96078; owning rewards cannot prove local hand-ins. Preserve foreign contracts/discovery and admitted source history; no guaranteed supply/ownership reassignment. |

Catalog113 journals/1571 achievements/1438 potential dailies/2192 rows; native2668 definitions/fingerprint/revision2/registry and earlier112 mappings preserved.96 source-comprehensive/124 pending; Temple of Flames next. Exact51617-byte earlier PR body archived with SHA-256; prior repair/news/accounting sections remain verbatim. Source/projection review is distinct from played source/coin/XP/reset/rescue/access/return/persistence qualification. Full220-zone goal active.
