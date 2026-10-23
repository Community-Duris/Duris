# Githzerai Stronghold: outpost, chaos relics and royal tombs

Priority102 of the original220-zone queue. This is a **source-comprehensive map**;
played accounting/source/access/persistence qualification remains pending. The
[schema3/revision1 journal](../../../areas/story/githzer.story.json) maps13 cards:11
independent stories/potential dailies and2 support services,8 contacts/35 addressable
aliases and20 optional current-material rows, without exclusions. The cash-only key
route is a support service and currently unavailable with accounting active. The
[Checkpoint 62 repository-wide audit
supplement](../quest-story-followups/GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md) records three
additional source-only WLD approaches into room #44400 and the typed external-reference
closure. New discovery/encounter/journal/achievement/daily credit requires active, ready
accounting; frozen recovery is separate.

## Source closure

| Source | Complete review and implication |
| --- | --- |
| [Quest source](../../../areas/qst/githzer.qst) | All13 Q/QA and22 raw M:20 addressed blocks/35 aliases plus ambient qc_action30(spirit) and25(king). Exact repeated inputs, alternative requests, empty reward, three D1 departures and all dialogue reviewed. Ambient timers are not ASK topics. |
| [Rooms](../../../areas/wld/githzer.wld) | All381 full physical rooms44400–44785 except44582/44750/44751/44756/44757:175 full prose families/43 headers/18 metadata/919 exact exits/289 relative patterns/62 full exit texts. Registry44335–44785/mode1. Outpost, Limbo/Pandemonium, fortress, towers/church, Rrakkma, hidden throne, royal crypts, mirrors/time and void reviewed. The all-95 scan also found three source-only approaches into room #44400; they are not active ingress. |
| [Mobiles](../../../areas/mob/githzer.mob) | All124 full records44400–44523/121 full prose families plus ID-paired numeric metadata. Three sergeants44402@44404/10/16; Zangzk44422@44462; hunter44431@44541; spirit44437@44519; king44461@44589; inspector44465@44612; stranded44494@44665; prophet44509@44729. Source placements and follower/mount guards are declarations, not played stock availability. |
| [Objects](../../../areas/obj/githzer.obj) | All182 full records44400–44581, including names/values/flags/inscriptions/traps/applies. Similar hide/ring names resolved by exact vnum;22 local type25 routes and ten type29 switches reviewed. Imported360(heavens) full record and prior owner procedure review retained. |
| [Resets](../../../areas/zon/githzer.zon) | All590 argument/location/cap/chance declarations via345 expanded families;466 exact/495 parent-aware reset signatures. D108/O56/P17/M207/G61/F30/E110/R1;100×589 and80×1. Every current item parent, equipment slot, source mobile/follower, exact door/control location and mount relationship reviewed. R mounts mobile44488 on rider44489@44565, not object removal. |
| Shared/custom execution | Entire [lucky_weapon](../../../src/specs/specs.githzer.c) and literal44499 assignment; full addressed/ambient quest dispatch and durable whole-bundle selection in [quest.c](../../../src/world/quest.c); password/SAY, item_switch, type25 command dispatch, complete falling.c, relevant command/reset/factory/custody/no-corpse paths reviewed. Earlier unchanged native key/successful-arrival/runtime qualification retained. `_spec2_` sets class specialization in mobconv.c. No new source/provenance/access endpoint inferred. |
| Foreign closure | Global active recipe/reset/exit/object/713-portal scan:17 touching recipes (13local+4Alatorin);16 foreign reset groups with all nine full source rooms/owner mobiles and imported barovia91053. Full boundaries realm14210,wh55634,surface538611;23 incoming travel declarations (22local+full ravenloft2 object59079/source59169). Incoming and imported ownership remain separate from local discovery/acceptance. |

## Independent accepted contracts and material progression

| Request | Exact accepted input → outcome | Source and larger story |
| --- | --- | --- |
| Sergeant44402/Q9 | I44401 → I44515;D0 | Parchment is G from warden44401@44400. Three sergeants can receive the same native request. Zangzk consumes the same map in another request. |
| Zangzk44422/Q41 | I44422+I44428+I44429 → I44431+I44516;D0 | Assassin44420@44458 carries heart; spy44425@44468 scalp; escort44426@44469 skull. Exact three-item bundle; Krazakan katana and master key, no separate sheath reward. |
| Zangzk44422/Q55 | C500000 → I44516;D0 | 500platinum wallet fee. Support service, no achievement/daily unit; legacy currency refused with accounting active. Requires deliberate payment integration, not bypass. |
| Zangzk44422/Q60 | I44512+I44401+I44470 → I44516;D0 | Master warlock44412@44448 carries potion; warden supplies map; master rogue44405@44423 carries azure poison bottle. Independent alternative; information hand-in not required. |
| Zangzk44422/QA69 | I44509 → no reward item;D0 | Adamantite coin item P into locker44403@44431/cap1, one of three guild lockers. Native accepted consumption yields information; support service without achievement/daily credit. Foreign Alatorin K'tharz83236/QA3427 offers E230000+I83501/D1 for the same item. |
| Prazyz44431/Q87 | I44436+I44440+I44440 → I44439;D0 | Mixed slaad44430@44483 supplies large red hide; two red slaadi44432@44490 supply small scraps. Helmet request requires three distinct physical items together. |
| Prazyz44431/Q94 | I44437+I44441+I44441 → I44438;D0 | Same mixed slaad supplies large green hide; two green slaadi44433@44490 supply small scraps. Separate leggings request; red stock does not substitute. |
| Spirit44437/QA107 | I44434+I44444+I44448 → I44456;D0 | Mixed slaad supplies first relic; huge shadow44436@44513 second; watch leader44439@44540 third. Long key opens fortress approach44540N. Spontaneous spirit speech is not a topic prerequisite. |
| King44461/QA139 | I44446 → I44501;D1 | Spirit carries heartstone. Eternal shadow is also exact key for44589UP/44590DOWN. Acceptance retires the king; independently reset arcane44462@44590 is not spawned by this receipt. |
| Inspector44465/Q189 | I44427 → I44519;D0 | Four Zerths44424@44467 carry necklace. Mindstone bracelet; outpost inspection/army plan remains narrative. |
| Inspector44465/Q198 | I44550 → I44577;D0 | Two Zerths44424@44680/81 wear rings. Mindstone ear clasp; remove from equipment for loose hand-in. Royal signet44563 is different. |
| Stranded44494/Q212 | I44580 → I44581;D1 | Torturer44495 at the same44665 carries implement. Rosary receipt retires recipient; no player escort or rescue destination. Empty greeting reply still belongs to an addressed topic family. |
| Prophet44509/QA283 | Five distinct I44563 → I44572;D1 | Five P rings into O sarcophagi44561@44752/53/54/55/58/cap5. Four kings and father44516 guard the tomb branches. Sarcophagi are closeable/closed, not locked; remains44562/64 are separate scenery. Bright marble key opens44583N rogue chamber, not the forcefield by itself. |

The same accepted contract preserves native identity even when several sergeants
share a prototype. A source declaration is not a personal first-acquisition
receipt. Current loose material checks remain optional and change with custody;
the sole completion step for each story requires its exact accepted transaction.
Paid key and information remain visible support services, outside achievements
and dailies. Historical accepted receipts survive this13-to11 projection change.
All-or-nothing durable selection requires the complete item bundle together and
uses different object pointers/UIDs for repeated ingredients, with a maximum14.
There is no new incremental NPC-held collection record. Exact supplied proof can
fit without asking topics, visiting source rooms, defeating guardians or solving gates.

## Access and exploration stories

Outpost intelligence connects to Zangzk's independently chosen key requests.
Guild master key44516, golem chipped key44425 and cracked key44412 govern different
doors. Source containers, potion use and competing map/coin consumers impose real
custody choices. Locker traps and actual closed/locked flags remain native policy.
Foreign Brino83302 also purchases local44404/44407/44410 equipment for coins;
these three foreign recipes do not become local story receipts.

Limbo supplies the hunter and chaos relic paths. The spirit's long key leads into
the fortress; shadow essence links the hidden throne and independently placed
arcane king. Tower key44462, caretaker key44531, steward key44532, rider key44472,
necrolyte key44559, war-Rrakkma chest/key44545/44546/44557, church keys44539/40,
captain key44544, shrine key44547, prophet key44572 and vortex key44575 are distinct
routes. A supplied key, picking where native policy permits or a shared-open door
does not require all earlier quest receipts. Narrative strength/flight suggestions
do not override the actual flags, hazards and equipped abilities.

Four key−2 speech doors use rrakkma at44626N, kraange at44678S, raylen at44679S
and tayr-dryn at44715N. check_magic_doors compares the last keyword token through
isname after allowed SAY and clears LOCKED/SECRET on the real reciprocal pair.
It does not clear CLOSED/BLOCKED. Reading an inscription is a clue; native code
does not require a personal learned-password state or record an individual solve.

Ten type29 switches:44471@44559→44587DOWN;44479@44572→44586DOWN;
44500@44585→44546DOWN;44502@44589→44589UP uses BLEED179;44551 carried by
Raylen44454@44687→44704DOWN;44553@44693→44705DOWN;four44568–71 carried by
undead guardians44517@44759→44760–63DOWN. All others use PULL340. Generic switch
dispatch can target a remote exit when the control is on a room floor; carried/
worn controls require the actor at the target room. It clears BLOCKED only.
Undead no-corpse death can put fixed ghost controls onto the actual floor.
Their absent TAKE flag alone is not proof of a broken route; no flag repair ships.
Creation/destruction guards and exact floor/access state still need qualification.

All22 local travel objects retain unlimited charges (value2=−1) and exact commands:
44426 ENTER→44473/44432 ENTER→44467;44477 BOW→44726/44486 ENTER→44720;
44488 UP→44768;44493 STARE→44768/44573 STARE→44767;44498 TOUCH→44585;
44506 ENTER→44473;44517 WEST→44613;44522 EAST→44495;44523–26 four cardinal
commands→44500;44527 EAST→44617;44528 ENTER→44514;44529 NORTH→44552;
44535 PRAY→44648;44541 TOUCH→44650;44554 ENTER→44706/44555 ENTER→44569.
Objects44517/22/29 lack an identified current stock producer; this is an intent
follow-up, not proof that unused prototypes must be spawned. Ghost chosen44469
and huge shadow inventories can also fall to the floor through no-corpse policy.
Command recognition/charge accounting is separate from successful teleport arrival.
F80 at44586/87 andF35 at44624 retain native fall/climb/flight/mount/blocked-exit guards.

The prophet's tomb story links statue bow, five exact rings, father's spirit,
forcefield, rogue Rrakkma and King Tayr-Dryn. Only five-ring acceptance is a
current personal terminal receipt. No mapped prerequisite insists on personal
bowing, defeating the father, operating four controls, awakening a Rrakkma or
overthrowing the king. Builders must choose evidence-producing endpoints for those
larger episodes. The prince/king/father historical wording does not establish a
campaign state. Mirrors, time and void retain their exploration and PvP consequences.

## Builder and capability follow-ups

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-GITHZER-CURRENCY-KEY | Q55 C500000 means500 platinum, not an item44509 payment. Accounting-active GIVE refuses legacy currency offering; durable quest submission currently selects item roots only. Integrate an authorized coin debit, frozen request/recipient/reward and idempotent recovery before enabling this route. Preserve fee, scarcity and guarded refusal; supplied master key and independent item requests are separate. |
| ZSQ-GITHZER-WHOLE-BUNDLES | Hunter requires one large hide plus two distinct small scraps; prophet requires five distinct44563 signets. Durable submit selects different loose object roots for each occurrence, maximum14, and requires the whole bundle together. Qualify incomplete counts, duplicate UID prevention, mixed colors/wrong ring, retained stock, all-or-nothing consumption and cold replay. Do not fabricate an incremental NPC-held collection state. |
| ZSQ-GITHZER-SOURCE-CUSTODY | Resolve exact item UID/generation/source/parent and admitted recovery separately from current loose preparation. P44509 uses locker44403; P44563 uses sarcophagus44561 at44752/53/54/55/58. Fresh successful O makes the normal P parent; global parent lookup and caps still need retained-world qualification. Sarcophagus flags5 mean closeable+closed, not locked. Remove worn44550 before offering; exact supplied proof fits without personal combat/tomb history. |
| ZSQ-GITHZER-ALTERNATIVES | Parchment44401 is consumed by sergeant or Zangzk; trophies and potion/map/poison are independent master-key requests. Item44509 is consumed by Zangzk information or foreign Alatorin83236/QA3427; no local reward item is issued. Preserve separate request identities, empty-reward settlement and competing inventory uses. Information is not a mandatory predecessor. |
| ZSQ-GITHZER-SPEECH-ACCESS | Four native key−2 doors at44626N/44678S/44679S/44715N use final keyword rrakkma/kraange/raylen/tayr-dryn. SAY clears LOCKED+SECRET only, with native speech guards. Qualify actual actor success, remaining CLOSED/BLOCKED, reciprocal state, supplied passwords and already-shared-open alternatives before personal clue/solve credit. |
| ZSQ-GITHZER-FLOOR-CONTROLS | Four fixed levers44568–71 are G on undead guardians44517@44759. Native no-corpse death can drop their inventory onto the floor; generic floor controls can remotely clear44760–63DOWN BLOCKED. Carried/worn controls instead require actor at target room. Do not change TAKE flags based on inventory appearance alone. Qualify death/factory publication, actual floor placement, exact receiver, remaining SECRET/CLOSED, repeated/shared-open state and actor evidence before any repair or achievement. |
| ZSQ-GITHZER-TRAVEL-FALLS | All22 local type25 declarations use exact commands/destinations/unlimited charges: BOW statue, STARE mirror pair, PRAY idol, TOUCH skull/spell, ENTER portals and cardinal floor routes differ. Objects44517/22/29 have no identified current O/P/G/E source or maintained literal producer; confirm intended unused prototypes before proposing supply. F80 at44586/87 andF35 at44624 go through flight/climb/mount/native fall guards. Qualify successful arrival independently from command acceptance. |
| ZSQ-GITHZER-ROYAL-ENDPOINTS | King44461 heartstone receipt retires him; arcane44462@44590 and followers already have independent resets. Prophet D text explains father/forcefield/rogue but does not grant personal clearance. Supply builder-authored actor/state/generation endpoints for feud, transform, awakened Rrakkma, father interaction, forcefield and king overthrow; do not infer them from reward or shared passage. |
| ZSQ-GITHZER-NARRATIVE-WORDING | Zangzk promises a sheath but Q41 rewards only katana44431+key44516; stranded44494 has empty greeting/completion reply and a narrated departure, not an escort. BLEED179, not SACRIFICE409, invokes shadow switch44502 despite the sacrifice inscription. Decide intended prose/command/endpoint changes with builders; any selected native repair belongs in a named fix/news commit. Current journal explains actual behavior. |
| ZSQ-GITHZER-RENEWAL-ACCOUNTING | Mode1 does not guarantee available daily stock: caps, consumed map/coins/hides/rings, wandering contacts and three retiring givers constrain renewal. Accounting-active O/P/G/E refusal lacks durable reset generation identity; guarded no-corpse and other source factories require admission. Qualify legitimate stock, bundle settlement, empty/multiple rewards, reset renewal and cold recovery without activating unsafe issuance or granting free materials. |
| ZSQ-GITHZER-FOREIGN-OWNERSHIP | Four foreign Alatorin recipes and16 foreign reset groups use local equipment/materials; incoming realm14210/wh55634/surface538611 and ravenloft2 portal59079 have separate ownership. Imported360 monolith and follower91053 remain distinct systems. R44488 mounts a mobile; object44488 is a different portal. Keep namespaces, source area/discovery and local accepted identity distinct; `_spec2_` is class specialization metadata, not a custom quest procedure. |


No native repair ships. Any selected repair needs its own named fix commit,
focused before/after evidence and prominent PR/news treatment. Preserve native
identity, scarcity, hazard/PvP/access policy and historical receipts. This source
map does not establish played availability under active accounting.

## Direct area-owner questions and conservative recommendations

The seven ZSQ rows above retain their technical scope and identifiers. These questions ask the associated owner decisions directly without changing the mapped native contracts.

| Question | Source evidence | Conservative recommendation |
| --- | --- | --- |
| **GITHZER-Q01.** Will the schema-3 journal keep its 13-card scope of 11 independent story/daily candidates and two support services, and which cards should create durable player credit? | The mapped sidecar has 13 cards, 11 stories/potential dailies, two services, and no exclusions. See [source closure](#source-closure) and [journal](../../../areas/story/githzer.story.json). | Treat the counts as source scope, not proof that all candidates are eligible or implemented; retain the two services outside achievements/dailies unless explicitly designed otherwise. |
| **GITHZER-Q02.** What renewal, recurrence, and replay policy should apply to daily candidates when the accepting givers or required source items depart or are depleted? | Three accepted contracts have D1 giver departure; material placement and reset-when-empty remain separate from calendar time. See [accepted contracts](#independent-accepted-contracts-and-material-progression), [resets](../../../areas/zon/githzer.zon), and [daily/service classification](../../../areas/story/githzer.story.json). | Require an actual eligible giver and replenished source under the authored policy; do not let day rollover create stock, repeatability, or a new durable award. |
| **GITHZER-Q03.** Will the 500-platinum Q55 route remain a support service until an authorized debit, committed result, and idempotent recovery contract are defined? | Q55 requests C500000 for a key and is not an item reward or achievement/daily unit. See [accepted contracts](#independent-accepted-contracts-and-material-progression) and [currency capability follow-up](#builder-and-capability-follow-ups). | Preserve the fee and its separate support role; do not enable it by treating coins as an item or bypassing accounting refusal/recovery rules. |
| **GITHZER-Q04.** Will each native offering preserve its exact item multiset, count, giver, reward, and departure, including repeated but distinct hide and signet instances? | The accepted contracts define exact alternate recipes and whole bundles; two rings or scraps require distinct physical items. See [accepted contracts](#independent-accepted-contracts-and-material-progression) and [whole-bundle follow-up](#builder-and-capability-follow-ups). | Keep exact identities and all-or-nothing submission; do not substitute a similar item, split a bundle into implied NPC-held progress, or invent a kill/cast prerequisite. |
| **GITHZER-Q05.** What evidence should qualify a player's personal source acquisition separately from current loose-material readiness and another actor's recovery? | Current accepted-item checks and source placements differ; SECRET materials and roaming/follower stock do not prove a personal source episode. See [source availability](#source-closure), [source-custody follow-up](#builder-and-capability-follow-ups), and [reset instances](../../../areas/zon/githzer.zon). | Use an admitted actor/source/item-instance receipt after actual recovery; do not award source credit from prototype existence, stock, current possession, or defeat alone. |
| **GITHZER-Q06.** Should valid access remain a set of alternatives, without requiring one historical entrance, key chain, discovery order, or shared-open door to prove the native return? | The map has multiple keys, entrance routes, Realm arrival, supplied-proof contracts, and shared-open paths with separate conditions. See [access and exploration](#access-and-exploration-stories) and [entrance boundaries](#source-closure). | Preserve the source alternatives and distinguish access from quest history; do not impose one route or prerequisite unless a separate owner decision defines it. |
| **GITHZER-Q07.** Will the four keyword doors be qualified by the actual final SAY keyword and resulting lock/secret state, while leaving CLOSED/BLOCKED and crossing separate? | The native `rrakkma`, `kraange`, `raylen`, and `tayr-dryn` branches clear LOCKED/SECRET only. See [password-door review](#access-and-exploration-stories) and [shared special](../../../src/specs/specs.githzer.c). | Keep password recognition, flag mutation, ordinary opening, and traversal as distinct outcomes; do not invent a learned-password achievement or clear extra flags. |
| **GITHZER-Q08.** What actor/control/exit evidence should qualify the ten type-29 switches, including fixed controls that can drop from no-corpse guardians? | The switches use exact commands and targets; generic handling varies between room-floor and carried/worn objects and clears BLOCKED only. See [switch review](#access-and-exploration-stories), [objects](../../../areas/obj/githzer.obj), and [switch handler](../../../src/specs/specs.object.c). | Preserve native command, owner, target, and remaining door flags; credit only a successful authorized state change on the real control, not a command attempt or guessed floor location. |
| **GITHZER-Q09.** Should the 22 type-25 routes retain their exact commands, targets, and unlimited-charge declarations, with source availability reviewed for #44517, #44522, and #44529? | The route inventory distinguishes BOW, STARE, ENTER, PRAY, TOUCH, cardinal, and other commands; three prototypes have no identified current source. See [travel inventory](#access-and-exploration-stories) and [travel/fall follow-up](#builder-and-capability-follow-ups). | Keep exact route declarations and conditions; do not manufacture object stock for the three unsourced prototypes or treat command recognition as successful arrival. |
| **GITHZER-Q10.** What movement, capability, arrival, and survival evidence should qualify the F80/F35 fall routes and water/visibility conditions? | The source records fall chances at #44586/#44587 and #44624; authored sectors and movement guards do not guarantee a safe destination. See [fall and travel review](#access-and-exploration-stories), [rooms](../../../areas/wld/githzer.wld), and [falling handler](../../../src/world/falling.c). | Preserve the recorded probabilities and source graph; do not promise safe travel, water capability, or survival from an attempted command. |
| **GITHZER-Q11.** Is the five-signets-to-prophet acceptance intended to remain the current terminal tomb receipt without requiring personal bowing, defeating the father, operating every control, or overthrowing the king? | The exact QA contract consumes five distinct #44563 rings; the broader tomb story describes additional scenes without those as native prerequisites. See [accepted contracts](#independent-accepted-contracts-and-material-progression) and [tomb exploration](#access-and-exploration-stories). | Preserve the exact accepted receipt; keep optional exploration and narrative scenes distinct unless the story owner defines separate evidence-producing objectives. |
| **GITHZER-Q12.** Which larger outpost, royal-tomb, rescue, or Rrakkma episodes should produce a lasting outcome beyond the current accepted item returns? | Several descriptions imply broader military and royal stories, but source mapping identifies only the listed contracts as current personal terminal receipts. See [environment and progression](#access-and-exploration-stories) and [accepted contracts](#independent-accepted-contracts-and-material-progression). | If a larger outcome is intended, define its actor, target, authority, duration, reset, and replay semantics before calling a room visit, kill, or disappearance completion. |
| **GITHZER-Q13.** How should D1 giver departures, retained accepted receipts, and any live reappearance or duplicate stock be represented across reset/restart? | The source documents depart commands, reset placements, roaming/mobile behavior, and historical receipt preservation as different mechanisms. See [accepted returns](#independent-accepted-contracts-and-material-progression) and [reset inventory](#source-closure). | Keep accepted history durable and live stock separate; qualify actual eligibility/reappearance before promising a repeat attempt or daily refresh. |
| **GITHZER-Q14.** Will outside Alatorin, Barovia, Air Plane, and Brino consumers of Githzer items remain separate from local acceptance, ownership, and achievement credit? | The bounded global scan found 17 touching recipes, outside shop/quest consumers, and imported records. See [foreign closure](#source-closure), [contract dependencies](#independent-accepted-contracts-and-material-progression), and [repository-wide audit](../quest-story-followups/GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md). | Preserve each exact foreign recipe, provider, reward direction, and active owner; do not infer local completion or an item owner from the consumer's story. |
| **GITHZER-Q15.** Are the imported Heavens objects and active epic-stone procedure intentional at their recorded placements, with each prototype owner kept distinct? | The native map imports active Heavens object #360, and the source review retains the prior owner/procedure findings for epic stone #359. See [object/dependency review](#source-closure), [Heavens records](../../../areas/obj/heavens.obj), and [special assignment](../../../src/specs/specs.assign.c). | Preserve cross-area ownership and the authorized callback; do not migrate, clone, or award story credit from object presence or touch messaging alone. |
| **GITHZER-Q16.** Should the three approaches from unregistered WLD sources remain historical records rather than active ingress into #44400? | Checkpoint 62 records three source-only exits; current active boundaries and candidate ownership are separate. See [repository-wide audit](../quest-story-followups/GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md) and [source closure](#source-closure). | Keep all three under their source-map owner; do not activate them or include them as player entrances without registration, map-intent, route, and return review. |
