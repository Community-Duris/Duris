# Harrow -The Gnome Village: comprehensive story mapping

Priority86 closes the source review for zone294, registry29318–29482,
source area `harrow`, reset mode2. The [schema3/revision1 sidecar](../../../areas/story/harrow.story.json)
maps eight cards to all eight native recipes, with14 contacts, all12 addressed
aliases and20 optional checks(16 current materials and four earlier ring receipts).
The [generated audit](../../reference/zone-story-audits/harrow.md) records binding
identity. **No native zone or quest repair ships.** Original item flags, scarcity,
shop admission, passages, teaching and PvP mechanics remain unchanged.

All new discovery, encounter, journal, achievement and daily credit requires
active, ready accounting. Frozen reward recovery remains separate. This is source
and journal-projection qualification; no played source/transaction/renewal or
database qualification is claimed.

## Complete source closure

| Source | Reviewed closure and dispatch implications |
| --- | --- |
| [Native quest file](../../../areas/qst/harrow.qst) | All29 blocks:20M/oneMA/fiveQ/threeQA. Four addressed response families/twelve aliases; seventeen `qc_action` ambient messages. All eight exact input/reward/departure identities preserved. |
| [Rooms](../../../areas/wld/harrow.wld) | All82 physical rooms29400–29482 except29458;65 full title/prose families,21 headers,187 exact exits/76 relative patterns,ten full exit-text families and23 full non-exit metadata families. Extra descriptions, altar writing, workshop/attic scenery and waterfall F50/C1 3 read. |
| [Mobiles](../../../areas/mob/harrow.mob) | All59 full prototypes29400–29458, including numeric tails and hamster ASCII art. Literal inline tildes in art must not be mistaken for line-ending string boundaries by review helpers. Keywords/names do not establish quest controller or source proof. |
| [Objects](../../../areas/obj/harrow.obj) | All88 full prototypes29400–29487: exact materials, alternate finished reward stock, five fixed travel objects, scenery, negative-weight garden trousers, trap data and extra descriptions. Unplaced spyglass remains unplaced. |
| [Resets](../../../areas/zon/harrow.zon) | All280 commands:161M/52G/24D/22O/20E/oneP;230 exact/233 M-parent-aware families and152 expanded groups. No F reset. The P command places hidden anti-poison in moss; it does not create another quest recipe. Current holders, caps, worn and hidden proof distinguished. |
| [Nine shop records](../../../areas/shp/harrow.shp) | Lomya, Womo, Torker, Gipie, Vetrix, Klemo, Henna, Fiona and Mandar. Full producing lists, keeper/room/open/race/other admission fields read. Shared [shop boot and dispatch](../../../src/economy/shop.c) prefers generated world.shp, then directory fallback; actual selected room/keeper/stock and trade admission still matter. |
| Imported source closure | Full [limbo paper5](../../../areas/obj/limbo.obj), stocked/produced by Torker, and full [heavens fish318/319/330](../../../areas/obj/heavens.obj), rewarded by Bom. Fish are food prototypes with native food values; rewards do not prove fishing or eating. |
| Shared procedures | [Quest loader/handler](../../../src/world/quest.c): MA/QA set room echo, not automatic acceptance. Addressed ASK/TELL and accepted GIVE are separate. `qc_action` is a timer room message. [Database loader](../../../src/world/db.c) registers ACT_TEACHER on instructor29428 when no other function; [literal assignment](../../../src/specs/specs.assign.c) binds inn at29403. No local epic-teacher table binding found. |
| Travel and boundary | Full [item teleport selector](../../../src/magic/spell_travel.c), interpreter dispatch and room fall/current load semantics checked. Two reciprocal Surface neighbors547275/547676 have full boundary records read. Across713 active type25 prototypes, all five identified incoming destinations are Harrow’s own; no foreign fixed portal enters lucky-star rooms. |
| Foreign consumers | Exactly eight recipes touching local88 objects, all Harrow. Including imported fish gives146 touching recipes:138 foreign recipes in tundra/newbie/shipy/vehicles/connectorzones/jade/alatorin/surface. Their bounded input/reward/giver index was read; they remain foreign outcomes. No foreign reset group referencing local mobile/item proofs identified. |

The review includes supporting streets/shops, garments and craft displays,
gardens and wildlife, atelier, secret tunnel, lake/fisherman, waterfall/altar,
observatory/telescope, home/attic/fishbowl and lucky-star component. Source
coverage does not create achievements for scenery or incomplete implied scenes.

## Player progression and exact accepted outcomes

| Card / native binding line | Exact inputs | Outcome and explanation |
| --- | --- | --- |
| Young girl /20 QA | Sapphire ring29405 | Token29406; girl departs. Starts in29431; ring is hidden floor29428. Returning supplied ring is valid; receipt does not prove personal garden search or father encounter. |
| Alorka glass horn /46 QA | Stylish horn29407 + token29406 + glass29414 | Rose glass horn29416. Horn starts held by lonely bard29422 in29418; hidden glass is carried by Brwra29434 in29423. Mandar’s dragonslayer horn29480 differs. |
| Alorka colorful robe /55 Q | Token29406 + silk29410 + simple robe29415 | Robe29417. Hidden silk is floor29451, not a spider drop. Instructor29428 wears robe in29438; worn proof must be removed before hand-in. No art lessons prerequisite. |
| Alorka light wand /64 Q | Token29406 + scrap cloth29413 + pretty doll29419 | Wand29418. Small children29406 carry scrap in29416/29442; sleeping artist29438 carries hidden doll in29446. Cloth bolt, other toys and attic toy prose differ. Native sphere/staff claims are stale; actual reward unchanged. |
| Alorka lucky sack /73 Q | Token29406 + leather sack29400 + dust29402 | Lucky sack29404. Lomya produces leather input; Torker produces dust. Lomya also has cap1 finite finished lucky-sack stock, outside her producing list. Its purchase/possession cannot complete the craft. |
| Bom painting /114 Q | Near-finished painting29440 | Pike318 + lobster319 + crab330. Bom starts29461. Sleeping/working artists in29446 carry matching painting under shared cap2; decorative floor painting29433 at29445 differs. No personal fishing/eating predicate. |
| Goldfish feeding /141 QA | Fish food29460 | Native4000 experience reward. Food floors29473/29475 share cap2. Five goldfish29449 resets(29472, twice29473,29474,29475) share one recipe; neither bubbles nor feeding another prototype records this exchange. |
| Leprechaun /162 Q | Pot-of-gold item29444 | Clover29453 +77777 native coins; recipient departs. Whale29450 carries pot in29474; leprechaun starts29477. Coin quantity is a reward, not a payment requirement or substitute for the pot item. |

One ring→token receipt is a helpful optional earlier step on each craft.
Four crafts consume four tokens; the earlier receipt is not a reusable material.
A matching supplied token fits without earlier personal history. One token can
appear ready on several cards, but this is a current inventory view, not a
reservation or proof of four funded outcomes. Live native acceptance selects and
consumes exact materials; journal rendering cannot settle them or infer sources.

Ask the girl `lost`, `ring` or `sad`; Alorka `treasure`, `craft` or `list`;
Bom `fish`, `artsy`, `items` or `trade`, or `hi`/`hello`. This is four response
families, not twelve achievements. Goldfish and leprechaun have ambient clues
and accepted recipes without addressed topic families. Periodic kitten,
girl, Alorka, cook, stag, Bom, watching trees, goldfish, watery kitten,
leprechaun and hamster scenes are supporting lore. Mob29451’s goldfish-style
line alongside kitten text deserves copy review, not invented feeding credit.

## Access and mechanic boundaries

| Native feature | What the source establishes; what the journal must not infer |
| --- | --- |
| Gardens and flower stalls | Ring floor29428 is hidden. Leather belt29412 is ordinary armor stocked by Lomya; Merill’s garden trousers29485 are an existing container with negative weight. No magical garden-belt gate or accepted flower recipe identified here. Preserve unusual existing item mechanics; do not normalize them while mapping quests. |
| Cave29448–51 | Native narrow tunnel/visibility metadata applies. Silk is hidden floor proof, not a corpse/carve reward. The spider’s presence does not require personal defeat or manufacture silk. |
| Workshop/observatory | Ordinary stairs connect workshop29440 through29439. Fixed ENTER portal29430 goes to29471; portal29431 returns to29440. Workshop NO_TELEPORT flag alone is not proof the shared item-teleport return is rejected. Telescope29461 is scenery and spyglass29434 is unplaced; no star-learning controller identified. |
| Waterfall and altar | Floor portal29425 at29469 uses ENTER7→29470, charges−1; altar WEST returns to29469. F50 loads fall chance, C1 3 loads current speed/direction. Engraved altar text is lore, not a worship/reading achievement. Actual committed movement/reading would need authoritative events. |
| Fishbowl | Fixed portal29441 at29443 ENTER7→29472; return29442 at29472 uses JUMP264→29443, charges−1. Shared selector uses argument to find an inventory/equipment/room object for non-direction commands; `JUMP fishbowl` names the existing target. Do not claim bare JUMP, JUMP out, every bowl-room return, or player arrival from command recognition alone. |
| Lucky-star component |29477 links29478–81;29480 EAST returns to29476. No local ordinary edge enters this component and no fixed incoming portal identified. Rooms29478/29479/29481 have no ordinary exits; only29480 has the identified return. This does not prove all shared/random travel impossible. Native entrance intent is unresolved; do not infer clover/luck/pipe/coin unlocks or add a passage to make the recipe reachable. |
| Shops/teacher/inn | Nine actual shop records differ from room-name assumptions. Produced stock is renewable only through native admission; finite finished lucky-sack stock can be depleted. ACT_TEACHER registers the art instructor; literal inn29403 is distinct. Existing teaching is not a story endpoint. |
| Paint-set exit field |29407 NORTH→29421 has door kind0 with key29420(the OTHER paint set). A populated key field on a non-door is not a locked magical art gate. Builder decides intended clue/metadata; no new key behavior inferred. |

All five fixed travel prototypes remain non-takeable with original values.
Existing command/arena/charge/ownership/teleport behavior remains shared native
code. Preserve the [Fields Between deliberate escape hotfix and required
replacement design](FIELDS_BETWEEN.md); quest prose cannot authorize portable
travel, new activation or altered scarcity.

## Builder-required follow-ups

| ID | Fair finding and proposed work; qualification before credit or repair |
| --- | --- |
| **ZSQ-HARROW-SOURCE-RENEWAL** | Exact hidden/worn materials, shared caps, produced supplies, finite alternate finished reward stock, girl/leprechaun departures and mode2 govern availability. Preserve source/root/UID/reset/custody and distinguish first personal acquisition from gifts, shops, player hand-offs and reward receipts. Qualify consumed/worn/nested inputs, actual recipient renewal, concurrency, retry/replay and frozen recovery. No daily availability claim from static candidate status. |
| **ZSQ-HARROW-TOKEN-PROGRESSION** | One optional ring receipt supplies history for four alternative token-consuming crafts. Keep current token readiness separate from history and reservation. Any richer choice/budget UI must use live admitted inventory and exact selected recipe, explain spent tokens and supplied-token acceptance, and never credit all crafts from one token/reward. No mandatory earlier personal ring gate without builder design and versioned compatibility. |
| **ZSQ-HARROW-TRAVEL-ARRIVAL** | Fixed waterfall, observatory and bowl routes plus water/current/fall/visibility metadata affect access. Add selected-target/command/actual committed arrival evidence before access milestones; distinguish rejected commands, wrong objects, other actors and repeated arrival. Qualify JUMP fishbowl target and access from every intended bowl room without changing pickup/charges/mobility. A future text correction must reflect existing dispatch. |
| **ZSQ-HARROW-LUCKY-STAR-ACCESS** | Ordinary incoming route into29477–81 was not identified; the existing EAST return29480→29476 works in source topology. Builder confirms intentionally reserved/shared-travel access, missing clue, retired exchange or intended entrance. Prefer accurate explanation first. Any new controller/passage requires explicit PvP/terrain/scarcity review, reciprocal/rejected/played traversal proof and a separate named fix/news. Never connect isolated rooms automatically. |
| **ZSQ-HARROW-CLUE-CONSISTENCY** | Alorka says sphere/staff but rewards wand; faerie-dust extra description reads like scrap cloth; attic toys and gallery painting do not match proofs;29451 has mixed goldfish/kitten ambient wording; lake spelling varies; paint-set key field is on a non-door. Confirm intent and propose minimal accurate native text/metadata correction, preserving recipes/flags/types/caps/routes. Actual repairs must be separate named fix commits, original-fails/repaired-passes proof and prominent before/after news. No repair ships here. |
| **ZSQ-HARROW-LEARNED-CRAFTS** | Instructor’s shared teaching, shop craft displays, altar inscription, telescope and ambient scenes lack authored story-learning endpoints. Define a meaningful lesson/selected response/read passage and authoritative delivered/result evidence before adding learning milestones. Ambient audience, addressed actor, taught skill, copied lore and accepted craft receipt are different events. Keep existing keyword aliases grouped by actual response family; no achievement for each alias/timer. |

Builder follow-ups describe design obligations and limitations; they enable no
controller, proof item, credit or native repair. Update this dossier and the
[integration plan](../ZONE_STORY_INTEGRATION_PLAN.md) as intent and played evidence
become available. Preserve native receipt identity when only explanation changes;
version changed input/reward/departure deliberately with legacy/frozen recovery.

## Qualification and checkpoint

Existing focused Python source/schema tests and C++ loader/projection journeys
cover all eight exact bindings, source-holder identities, supplied token without
ring history, wrong horn/painting/cloth and worn material, alternate reward
possession without crafting credit, one shared token across cards without
fabricated outcomes, eight distinct receipts, spent-token history, replay and
cold recovery. Source contracts protect fixed non-takeable routes, isolated
lucky-star entrance, native shop producing lists, QA/MA echo semantics and
teacher/inn dispatch. Synthetic receipts qualify projection only.

The maintained build, production regression, changed/staged format, source links,
whitespace and preservation/publication checks are required before publication.
All103 prior journals,2668 native definitions/fingerprint/content revision2/
registry, original220 queue and earlier repair disclosures remain unchanged.
Catalog104 journals/1585 achievements/1441 potential dailies/2195 story rows:
eight authored cards replace eight fallback units. Exact50691-byte previous
Valois PR description is archived with SHA-256 and all ten repair/news/accounting
sections retained verbatim.

The [Checkpoint 46 audit supplement](../quest-story-followups/HARROW_AUDIT_REVIEW.md)
records the broader connectivity, typed-reference, and code-reference review.
**195/220 source-comprehensive, 25 pending. Priority 196, Arcium (`arcium`) is next. The full goal remains active.** No native repair,
accounting activation, database/server operation, migration, deployment or merge.

## Direct area-owner questions and conservative recommendations

These questions make the remaining route, recipe, and story-credit choices explicit. Recommendations keep the eight native contracts authoritative and treat optional scenery, source history, and travel as separate facts.

| Question | Source evidence | Conservative recommendation |
| --- | --- | --- |
| **HARROW-Q01.** Is the absence of an ordinary route into lucky-star rooms #29477–#29481 intentional, or should the area provide a named entrance? | [Lucky-star access boundary](#access-and-mechanic-boundaries), [travel follow-up](#builder-required-follow-ups) | Preserve current topology and describe the access as unresolved; add no entrance until its source, destinations, and safe return are selected. |
| **HARROW-Q02.** Should any of the eight accepted exchanges require the player to personally use the waterfall, observatory, or fishbowl routes, or do supplied exact items satisfy the existing contracts? | [Accepted outcomes](#player-progression-and-exact-accepted-outcomes), [travel mechanics](#access-and-mechanic-boundaries) | Keep exact supplied materials valid; add personal arrival credit only for an explicitly selected route and a committed destination event. |
| **HARROW-Q03.** Is the girl's ring receipt optional preparation, or should the player have to earn it personally before crafting with token #29406? | [Ring-to-token progression](#player-progression-and-exact-accepted-outcomes), [sidecar checks](../../../areas/story/harrow.story.json) | Keep the earlier receipt optional and accept the exact supplied token without inventing a personal garden-search prerequisite. |
| **HARROW-Q04.** Should the four Alorka crafts each consume one separate token, making the player choose among them until another token is found? | [Four token-consuming recipes](#player-progression-and-exact-accepted-outcomes), [token follow-up](#builder-required-follow-ups) | Preserve one token per accepted craft and keep readiness separate from history or reservation; do not let one receipt fund all four. |
| **HARROW-Q05.** Is Lomya's one finished lucky sack intended as ordinary shop stock alongside the separate token-based craft, or should the player-facing copy distinguish those routes? | [Sack craft and finite stock](#player-progression-and-exact-accepted-outcomes), [shop inventory](../../../areas/shp/harrow.shp) | Keep purchased or possessed stock distinct from an accepted craft receipt and avoid implying that the finite sack completes Alorka's recipe. |
| **HARROW-Q06.** Should Alorka's clues about a sphere and staff be corrected to name the actual wand reward, or are different rewards intended? | [Alorka's four crafts](#player-progression-and-exact-accepted-outcomes), [native dialogue and recipes](../../../areas/qst/harrow.qst) | Preserve the exact wand prototype and recipe; align dialogue only after the owner confirms that the wording is stale. |
| **HARROW-Q07.** Is the faerie-dust extra description meant to describe scrap cloth, or should it identify the actual dust item used by the recipe? | [Clue-consistency findings](#builder-required-follow-ups), [dust prototype](../../../areas/obj/harrow.obj) | Keep the recipe's exact dust input; correct the extra description after confirming the intended item identity. |
| **HARROW-Q08.** Should Bom's painting exchange remain an accepted delivery of the three named fish, with no fishing or eating requirement? | [Painting exchange](#player-progression-and-exact-accepted-outcomes), [native contract](../../../areas/qst/harrow.qst) | Preserve the three distinct reward kinds and credit the accepted exchange only; do not infer fishing or consumption from the returned fish. |
| **HARROW-Q09.** Is giving fish food to the goldfish intended as the complete local feeding outcome, or should it represent a further fish-care result? | [Goldfish exchange and ambient scenes](#player-progression-and-exact-accepted-outcomes), [goldfish prose](../../../areas/mob/harrow.mob) | Keep the single exact-item exchange and its stated experience reward; do not add feeding, growth, or repeat-credit stages from ambient behavior. |
| **HARROW-Q10.** Should the altar inscription, telescope, art instruction, or periodic scenes produce story-learning credit, or remain lore and services? | [Learning and ambient boundaries](#access-and-mechanic-boundaries), [learning follow-up](#builder-required-follow-ups) | Keep those elements outside story credit until the owner selects a lesson or discovery with a specific accepted result. |
| **HARROW-Q11.** Is the key field on the paint-set exit intentional metadata, or should the exit become an actual keyed door or a clue-only route? | [Paint-set exit field](#access-and-mechanic-boundaries), [exit and key prototypes](../../../areas/wld/harrow.wld) | Preserve the non-door behavior as written; do not promise that the key field unlocks a passage unless the owner selects a tested door design. |
| **HARROW-Q12.** Should the mixed goldfish/kitten line and the other named clue mismatches be corrected as copy, or are they deliberate cross-references? | [Clue-consistency findings](#builder-required-follow-ups), [mobiles](../../../areas/mob/harrow.mob), [quest responses](../../../areas/qst/harrow.qst) | Keep current native bindings and outcomes; correct only the confirmed copy, without adding new aliases, rewards, or achievements. |
