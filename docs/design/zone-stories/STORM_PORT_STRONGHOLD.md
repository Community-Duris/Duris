# Storm Port Stronghold: comprehensive story mapping

Priority93 closes source review for zone226, registry22547–22664, area
`spshold`, reset mode2. The [schema3/revision1 sidecar](../../../areas/story/spshold.story.json)
covers all four native exchanges as four independent cards, six contacts,
zero dialogue aliases and eight optional checks (six current-material/two
earlier-receipt rows). The [audit](../../reference/zone-story-audits/spshold.md)
keeps exact native identities. All new discovery, encounter, journal,
achievement and daily credit requires active, ready accounting. Frozen recovery
is separate. Source-comprehensive review does not qualify played transactions.

## Actual repair and news

Separate [fix 4b25c5f17](https://github.com/Community-Duris/Duris/commit/4b25c5f177ebb11d4a2eabab286b5c2a2e5a55f9) changes exactly one direction word in
[room22607](../../../areas/wld/spshold.wld): the outpost lies **west**, replacing
east. The forest still lies east. Existing reciprocal22607 west/22606 east
and22607 east/22630 west routes stay unchanged. No topology, pickup, sources,
payments, guardians, rewards or travel mechanics change.

Player-facing news: **Storm Port Stronghold’s outside-path description now
correctly points west toward the outpost.** The extended
[production source regression](../../../tests/async/test_zone_story_quest_production_catalog.py)
rejects the original caption and passes the corrected caption plus all four
existing reciprocal exits. Exact native bytes qualified; live LOOK/traversal
is not claimed. Journal authoring is a separate feature commit.

## Full source and dispatch closure

| Source | Complete review and implications |
| --- | --- |
| [Quests](../../../areas/qst/spshold.qst) | All four Q blocks, no M/MA/QA or addressed aliases. Captain coal+valve→ticket, Decker ticket→helm, Hordine sea maps→three equipment outputs, Hordine torn map→key. All responses reviewed. Every giver remains; no qc_action, personal kill, escort, launch or arrival predicate. |
| [Rooms](../../../areas/wld/spshold.wld) | All65 physical records22600–22664;54 full prose families/18 headers/two complete non-exit metadata families/130 exact exits/55 relative patterns/seven full exit texts. Pier/defenses/training/watchtowers/Hunter-Killers, static Drifter/juggernaught interiors, furnace, source load hub, unfinished crew structure and slave lore reviewed. F10 at22621 is fall risk; six unnamed rooms and registry lower bound do not invent stories. |
| [Mobiles](../../../areas/mob/spshold.mob) | All44 complete prototypes22600–22643,44 full prose families and every numeric tail. All have local reset placement; no imported mobile. Native contact starts: Decker22634/captain22643/Hordine22659. Aphantan22623 is SCAVENGER with no SENTINEL or STAY_ZONE; global wandering is separate from a guaranteed source route. |
| [Objects](../../../areas/obj/spshold.obj) | All34 complete prototypes22600–22633, flags/values/effects/descriptions. Eight have no local reset:22613,22620,22621,22624–27,22633; distinguish reward-only ticket/helm/equipment/key from unused prototype stock. Hidden coal/valve/sea maps and hidden NORENT bone/iron keys remain. Treasure key22633 has100% ordinary keyed-unlock break roll. No flag/weight/type repair. |
| [Resets](../../../areas/zon/spshold.zon) | All129 commands:M76/E20/D14/O7/G7/F4/P1;98 exact/104 parent-aware/87 expanded families.128 chance100 plus eye22643 M80%. Preserve caps/conditional chains; child G100 does not cancel parent80. Coal is cap1 P inside fixed furnace22609, valve cap1 G on ifrit22618, sea maps cap1 G on Aphantan22623. Ticket has no reset producer. Mode2 and active item-reset refusal do not prove renewed supply. |
| Shops and specials | No local `.shp` file despite rakshasa merchant prose and declared training sword/quiver stock. Only local literal bindings in [assignments](../../../src/specs/specs.assign.c):helm22621→master_set and room22648→crew_shop_proc. Makeshift inn22610 lacks ROOM_INN and a local inn assignment. Generic type25 portals supply access; no local moving Drifter/juggernaught controller is identified. |
| Complete foreign sources | Full imported training sword5308/holy relic76712/reward greaves40771/torn map77209/spade38037/chest77210 and Burgadan38037 mobile reviewed. Three touching reset groups:two vehicle avatars at47197 with M25%/gear G20%, plus pirate77219 at77248 carrying torn map cap1. Full treasure chest reset family at77262 includes coins/flippers/scepter/gauntlets. Holy relic on eye belongs to Jade’s four-relic ritual, not a Stronghold quest. |
| Complete foreign recipe closure | Ten touching local/quest-prototype recipes:four local/six foreign. Burgadan in Thetis returns torn map with spade; Oberon helmsman77218 consumes sea maps with no authored reward; Divine Home has separate globe/greaves rewards; Myrabolus rewards maelstrom boots; Winterhaven13-item set consumes those boots. Imported holy relic76712 adds Jade’s four-relic exchange. All seven foreign block texts/terms reviewed; ownership stays with their zones. Local greaves22626 differ from actual reward40771. |
| Complete boundaries | Three Surface approaches/returns:636281 south↔22600 north,636680 east↔22600 west,636682 west↔22634 east. Load room22649 points north22255/east22113/south82540/west40717/down22423/northwest76616, plus up22650; no static incoming22649 edge identified anywhere in the active world. All nine full foreign boundary records reviewed. Four incoming local-room portals are all local across713 active type25 objects. |
| Shared execution | [Quest exact-root settlement](../../../src/world/quest.c), [door/key/break/portal travel](../../../src/cmd/actmove.c), [SEARCH](../../../src/cmd/actobj.c), [DIG](../../../src/cmd/actoth.c), [loader/door resets/item issuance](../../../src/world/db.c) and [NPC scavenging/wandering](../../../src/mob/mobact.c) reviewed. [Master set](../../../src/specs/specs.set.c) and [set adapter](../../../src/specs/specs.set_equipment.c) are equipment effects, not quest receipts. Full [crew shop](../../../src/ships/ship_shop.c) and Stronghold crew entry in [ship variables](../../../src/ships/ship_variables.c) reviewed against [SUB_MONEY](../../../src/core/utility.c) and interpreter pending-wallet admission. |

## Exact independent progression

| Native contract | Result | Boundary |
| --- | --- | --- |
| Captain22636 /Q31, helm22643 | I22610+I22612→I22613; stays | Two actual loose roots together. Narration says the ship can take off, but no launch/fare/transport/escort state is changed. |
| Decker22617 /Q2, forest22634 | I22613→I22621; stays | Supplied ticket fits without own captain receipt or a travelled ride. History never restores a spent ticket. Helm possession/set bonuses are not this exchange. |
| Hordine22626 /Q10, bridge22659 | I22622→I22627+I22625+I40771; stays | Hidden sea maps from roaming Aphantan. Torn map is different. Another consuming sea-map hand-in belongs to helmsman77218, not Mui Pai. |
| Hordine22626 /Q18,22659 | I77209→I22633; stays | Torn map is foreign pirate stock. Optional Burgadan referral first returns map+spade. Supplied map fits without personal referral. Treasure access/loot remains foreign and unrecorded as local story. |

No personal kill is imposed by item names or source holders. Readiness/history
rows are optional; accepted native completion remains the terminal evidence.

Drifter access uses fixed UP ladder22617 at22631→22640 and DOWN ladder22618
at22642→22631. Furnace22604 at22641 ENTERs22647; return22605 there ENTERs
22641. Both ladders/furnaces are non-takeable type25, unlimited−1, commands
5/6/7 respectively. Furnace22647 has no ordinary exits. Return presence and
existing travel admission need actual qualification; the ticket is not consumed
by these portals.

Coal22610 is hidden P in juggernaught container22609 at22663. Its values
1000/29/key22611/1000 describe fixed closed/locked/pickproof container stock.
Slave master22624 at22662 carries hidden NORENT iron key22611. Hordine carries
hidden NORENT bone key22602 for raw3/D2 locked Hunter-Killer doors22611 east/
22635 west. These access items are different from the quest’s ticket or treasure
key. Valve22612 is hidden G on ifrit22618 inside22647. No source kill, SEARCH,
UNLOCK or combat outcome is silently added to the quest contract.

Sea maps22622 start on Aphantan at22649. The hub has outbound links but no
static entrance; ordinary NPC wandering can leave it. Do not teleport players
into a load hub or promise where the ghost will be. Current source/visibility,
gifts/loot and roaming custody need qualification. Aphantan’s scavenging lore
does not prove personal map recovery. Hordine and Oberon helmsman compete for
sea-map material; one receipt does not produce a replacement.

## Torn map, spade, silt and chest

Torn map77209 is hidden G on pirate77219 at77248. Burgadan38037 starts at
Thetis38013; his exact foreign Q37 consumes/returns map77209 and gives key-type
spade38037. Hordine consumes a matching map for key22633. Doing Burgadan first
can retain a map for Hordine, but own referral history is not required when exact
map/spade are supplied. The two exchanges remain separately owned.

Silt77261 down→77262/raw6 and77262 up→77261/raw7 both use key38037;
D6 resets set secret/closed/locked. Loader masks raw kind to2 pickable from
above and3 pickproof from below; reset state is separate. Spade is ITEM_KEY18,
not a shovel/hoe/pick DIG tool. DIG rejects underwater terrain and only reveals
BURIED floor items. Do not tell players to DIG this route or equate wielding,
reading, ordinary OPEN, pre-open access and settled personal unlocking.

Chest77210 is fixed closed/locked/pickproof container29, key22633, cap1 O at
77262. It holds two cap2 coin piles, flippers, scepter and gauntlets. Existing
native treasure access exists, but claiming first discovery/loot needs its own
admitted outcome. The chest is not BURIED. Both spade38037 and treasure
key22633 have value[1]=100:100% break roll on ordinary successful keyed UNLOCK,
not100 uses. Accounting destruction settlement differs from door state changes;
source renewal or supplied replacements matter. No breakage/lock/pickup repair.
Invalid foreign77262 south→−1 is removed by the loader, not a return route.

## Services, equipment and unscripted story intent

Crew shop22648 is listed as evil-aligned. Full handler requires owning a ship,
valid numbered crew/chief, frags or skills, positive cost unless trusted and
sufficient money. It calls SUB_MONEY then immediately changes crew/chief,
updates status and queues a ship save. **It ignores the debit return value.**
SUB_MONEY returns−1 when accounting is active; the handler has no matching
active-accounting refusal. The interpreter’s pending-wallet command fence
includes HIRE but only blocks unpublished balances, not this failed debit.
Static source therefore exposes an unpaid mutation path under active accounting.
This is a required shared accounting fix; no live exploitation test or repair
is claimed here. Refuse active HIRE until typed payment and crew publication can
settle together, then qualify failure/retry/refund/ship-owner recovery before
enabling. Do not award a story for hiring.

Helm22621’s master_set periodically applies equipment bonuses. The adapter’s
registry includes82559 while the underlying master_set count list omits it;
qualify intended set membership before adding completion or changing effects.
Shoes/globe carry PROC flags without a local literal binding. Slave rowing,
Hunter-Killer revenge, stranded passengers, mounts, ballistae and naval invasions
remain combat/lore, without authored rescue/launch/campaign objectives.

## Required builder and capability work

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


Use existing schema3 material and optional accepted history. New learned/source/
travel/access/phase facts need actor/selected UID/instance/state/output and
accounting/recovery/version proof. Preserve scarcity, hidden/locked states,
portal restrictions, faction admission and deliberately isolated load content.

## Validation and limits

Existing source/schema and Python/C++ projection checks cover all four bindings,
both exact bundle roots, wrong/held/nested material, supplied ticket without own
repair, history without ticket, sea versus torn maps, optional foreign referral
without current map/spade, independent Hordine outcomes, replay/cold recovery
and no foreign discovery fabricated by projection. The original outpost caption
fails and repaired source passes with all four unchanged reciprocal routes.
Full production regression/all111 journeys/build/changed-staged formatting and
preservation required before publication.

Catalog111 journals/1582 achievements/1441 potential dailies/2193 rows.
Native2668 definitions/fingerprint/content revision2/registry and earlier110
journals remain unchanged. Original220 queue:93 comprehensive/127 pending;
The Mountain Settlement of the Harpies next. Exact51634-byte prior PR archived
with SHA-256; prior repair/news/accounting sections stay verbatim. No accounting
activation/DB/server/migration/deployment/merge. Projection does not qualify
played hidden recovery, native offers, key destruction, child/root custody,
roaming, travel/arrival, treasure access, paid crew mutation or source renewal.
