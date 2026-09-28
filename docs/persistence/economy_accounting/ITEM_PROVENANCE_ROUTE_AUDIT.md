# Item provenance route audit (Plan 3)

This records the item routes reviewed against `49af585c4` while the economic
epoch is inactive. It is a route-level supplement to `writers.json`, using its
writer/authority/backend/proof vocabulary. It is not a completed writer census
or permission to activate an epoch.

| Reachable path | Authority and source/sink | Current Plan 3 status | Proof or remaining work |
| --- | --- | --- | --- |
| `item_movement_transaction_submit` from player get, drop, put, give and trusted steal | Existing item ownership repository; custody move | Accounted intent, ordered item events and exact legacy references on SQL and flatfile; same-owner nesting admitted. SQL player materialization refuses an unexpected native child, a foreign player row or a mismatched template before detaching selected rows | `test_item_transfer_accounting.py`, `test_universal_item_transfer_accounting.py`, `test_economic_accounting_flatfile_gate.py`; SQL item harness includes each native conflict followed by a valid give. The disposable flatfile `run_npc_container_claim_journey.py --restart-only` mode verifies a nested player UID graph, unchanged custody revision, and live materialization after save/restart while the epoch is inactive |
| Locker deposit/withdraw and pet give/return through the same transaction | Existing locker/pet native rows and item ownership repository; custody move | Accounted policy admission is implemented. SQL moves nested native rows and metadata for both routes in the custody transaction; flatfile preserves the nested custody graph and exact references | Focused SQL and flatfile fixtures pass, including stale player snapshot reconciliation for pet handoff and return. Real-server SQL pet and locker journeys pass at `68af73add` with migration 0041, save and restart while the epoch is inactive. Add active-epoch and flatfile gameplay journeys |
| Corpse create/loot through item transfer | Existing corpse handoff and item ownership repository; custody move | Existing accounted path retained; coin piles excluded from ordinary item accounting | Existing corpse tests; combined death/coin effects belong to Plan 4 |
| Spell conjuration and spell component retirement | Creation grant / item movement transaction; issuance or destruction | Typed `spell_creation` and `spell_consumption` claims. Five player and room conjuration grants in `magic/spell_conjuration.c` and the three dragoon conjured-weapon grants in `magic/spell_item_lifecycle.c` generate occurrence IDs independent of candidate UIDs before submission; the grant queue retains them through command retries. Active NPC casts of these durable item spells release unpublished candidates without applying associated spell effects. The SQL item transaction removes a committed player component forest from native `player_items` while retaining destroyed UID, root and parent history. It refuses an unexpected native child before publishing retirement. Flatfile and SQL fixtures cover a multi-root retirement with a nested child and exact references. A queued stale player save remains blocked after retirement, while reconnect and restart loads leave the retired forest absent | The fixtures reject a second spell issuance with the same logical source and a new UID. `test_spell_creation_source_identity.py` and `test_conjured_weapon_source_identity.py` execute the grant source handoff, failure and NPC refusal branches. Other spell creators still need independent producer IDs; these occurrence IDs are retained by grant queues but are not persisted cast event IDs. Add live creation and consumption save/reconnect/restart journeys |
| Soulbind reload and replacement cleanup in `magic/spell_item_lifecycle.c` | Repeatable player item issuance and direct retirement of earlier bound items | Active-epoch reload refuses before `read_object`; staff clearing and replacement refuse before removing an old binding. The shared cleanup skips durable objects if an epoch becomes active before a pending callback, while transient cleanup remains available | Assign a durable entitlement generation and retire the old UID in the same accounted replacement operation before admitting reload or staff replacement. Cover the live callback and restart path |
| Key breaking, chaos pouch consumption and administrative load | Item movement transaction; intentional sink or issuance | Typed `intentional_destruction` / `administrator` claims. Flatfile retains destroyed UID and historical root/parent for read-only provenance lookup | The disposable `run_npc_container_claim_journey.py --key-break` mode unlocks a synthetic chest with a guaranteed breaking key, observes committed destruction before live extraction, and verifies the same tombstone after save/restart while the epoch is inactive. Add SQL and active-epoch command journeys and explicit source authority checks for the remaining routes |
| Staff storage establish, empty and delete in `cmd/actwiz.c` | Existing room item repository; issuance, same-room child move or destruction | Flatfile uses typed administrator/intentional claims and a bounded direct-child repair policy. Active SQL `new`, `delete` and `remove` refuse before native saved-item or live item mutation | Policy regression and `test_pa_item_admission.py` pass; qualify the full native room projection on SQL before enabling its storage commands, and add live command journeys |
| Quest item rewards in `world/quest.c` and `world/world_quest.c` | Creation grant; issuance | World quests use persisted `(player PID, quest_started)` as a logical `quest_completion` source independent of the allocated UID. The start value now advances across resets, repeat starts in one second and shared assignments. A missing source ID is refused before queuing. Active-epoch completion waits for the published item grant before quest reset, completion XP and epic reward. A terminal grant failure keeps the quest and restores the prior kill count. High-level mercenary quests with a coupled coin reward refuse before item admission. Static quests have no durable completion ID and refuse their legacy gift path before accepting an offering during an active epoch | SQL and flatfile fixtures reject a second UID with the same source; `test_world_quest_reward_policy.py` checks ordering and `test_world_quest_item_completion.py` executes the publication callback's success, refusal, changed quest and duplicate-callback cases. Restart reconciliation of a committed item whose callback did not run, an exact-once quest marker, and the coupled coin route remain open; add a durable static quest completion ID and NPC custody route |
| Random-zone quest offerings in `world/random.zone.c` and staff `randobj` in `item/randobj.c` | Direct extraction followed by unsourced random rewards; direct staff issuance | Both entry points refuse before an offering or reward is mutated in an active epoch | Add stable entitlement identities and admission for these routes if they must run after activation |
| Candidate publication in `world/handler.c` | Creation grant when an unowned candidate is handed to a player; issuance | Generic candidates are refused before active-epoch player publication. Inactive behavior retains its legacy grant path | Distinguish reset, loot and other causes with a durable producer identity before enabling active admission |
| Zone reset / room generation in `world/db.c` and other direct room placements | World reset; issuance | All seven zone item opcodes are withheld before allocation in an active epoch; other direct room placements remain unsupported | Route committed placements through a system-authorized source with persistent reset-generation identity; test SQL and flatfile |
| NPC reset equipment and random death drops in `world/db.c`, `world/random.mob.c` and `combat/fight.c` | NPC-held item issuance, later corpse/loot custody | Zone item opcodes, random mobile equipment and `die`'s unsourced reward branches are withheld in an active epoch. Other generation and death-drop paths remain unsupported as accounted first admissions | Define persistent NPC/reset or kill identity, then couple committed loot admission to the native owner and later corpse handoff |
| NPC ship treasure in `ships/ship_npc.c` | Chest, key, coin pile and materials created for a ship | Loader withholds the complete treasure bundle before allocation in an active epoch; Cyric's Revenge spawn is refused because its locked hold requires the withheld key | Couple a persistent ship-generation identity, item issuance and balanced pile value before enabling treasure in an epoch |
| `read_object` template allocation, SQL player/locker/private-chest/corpse/saved-item loads, and SQL shopkeeper catalog reload | Saved-item custody projection or derived shop stock; allocation alone is not supply evidence | SQL ground-item restore checks custody graph, UID uniqueness, handoff receipt and payload digest before publishing. Flatfile saved items share a checksummed world catalog with UID and graph validation; cross-store custody moves use its authority journal. SQL shopkeeper stock is rebuilt from `shopkeeper_items` rows without a persisted `obj_uid`, so restored items receive fresh runtime identities; migration 0044 preserves dynamic properties but not item identity or source claims. Classify this as an unqualified legacy admission and block it after activation until stable stock identity/source evidence or projection proof exists | SQL saved-item fault journeys and the flatfile world repository fixture pass. `test_shopkeeper_population.py` and `test_pa_item_dynamic_state.py` cover shopkeeper staging and property fidelity, not UID provenance or source claims. The flatfile saved collection fixture covers pre-journal refusal, post-journal recovery, and recovery with either the catalog or second store written first |
| Flatfile collector custody and saved-room collection | Collector and item ownership catalogs plus world catalog; custody move | Collector item references are staged in the same authority journal as collector state, item custody and world after-images. Reference staging creates its private bucket directory before its first read. A command interrupted after partial publication recovers and replays with exact references. Exact-ID replay refuses if the committed references have gone missing | The full-command fault fixture covers a corpse source and a saved-room forest, plus a missing-reference replay refusal. The saved-room entitlement is seeded in isolation; add a live collection and restart journey before activation |
| `extract_obj`, item scrap, zone purge and miscellaneous cleanup | Mixed temporary cleanup and real sinks | Do not treat extraction as a universal sink. Accounted command paths above are covered; the remaining reachable writers need route-specific decisions | Classify each call site and refuse unsupported active-epoch mutation before changing durable state |
| Disguise kits in `classes/disguise.c` | Carried or held kit consumed on success or a failed roll | An active epoch refuses a mortal kit-dependent disguise after kit selection and before the roll. Empty-argument disguise removal and trusted or achievement bypass remain available | Admit kit retirement through a typed source and custody event before enabling consumption |
| Soul-shard orb purchase and high-level orb summoning in `classes/drannak.c` | Three shards exchanged for an orb; orb consumed for a summoned pet | The purchase branch refuses before orb allocation or shard extraction; high-level summoning refuses before the successful pet path or orb extraction. Listing and low-level conjuring remain available | Couple the priced exchange and orb retirement to item authority; test the pet lifetime and restart path |
| Faerie dust in `classes/ethermancer.c` | Carried dust consumed to enhance or refresh faerie sight | Active-epoch casts calculate the exact dust cost across new and repeated affects, retire those carried roots with a typed `spell_consumption` source, and apply the effect only after committed publication. Zero-cost casts keep their effect without a sink; terminal retirement failure does not apply it. The callback resolves the target by runtime identity and refuses a departed target after spending the dust | `test_faerie_sight_item_retirement.py` covers new, refreshed, repeated, zero-cost, refusal and departed-target branches; `test_spell_component_exact_count.py` executes the production exact-count selection. Add a live active-epoch spell and save/reconnect/restart journey |
| Salvage and scientific tools in `item/salvage.c` | Player item and optional tool set consumed for material, essence or recipe outputs | An active epoch refuses after selecting the input and before any item allocation, grant or extraction | Couple all inputs and possible outputs to one durable crafting operation before enabling salvage |
| Repeatable divineclaim, summoned spellbook/totem, wind blade, and chaos pouch test seed | Reward or spell allocation; issuance | These entry points now refuse before reward reservation, event scheduling, or allocation in an active epoch. Summoned spellbook/totem cleanup also skips a durable old item if activation races a pending grant callback | Assign a durable generation or cast identity and coordinate its reservation with the item source claim and old-item retirement before admitting repeatable issuance |
| Divineclaim dismiss, expiry, staff revocation, and corpse cleanup | Reward retirement, child promotion and live extraction | These routes refuse active-epoch item mutation before SQL retirement or live topology changes | Admit retirement through the item event/reference adapter, including nested contents and account-wide policy changes |
| Wear/remove and direct `equip_char` / `unequip_char` | Player equipment slot; same-owner topology | Player wear and single-item remove now submit same-owner slot changes through item custody. The SQL ledger and current owner, flatfile catalog, and accounting before/after position retain the slot. Active auto-replace and wear/remove all refuse; unrelated direct equipment mutations remain unsupported | SQL and flatfile isolated fixtures cover wear, remove, stale slot, replay, references and native SQL slot. Add live player save/reconnect and classify the remaining direct callers |
| Combat fumble and critical disarm through `forced_weapon_drop` | Equipped player root to room; custody move | The weapon stays equipped until committed custody evidence exists. Typed reasons preserve the source slot; stale slot or physical tree refuses. SQL removes the departed player row tree in the custody transaction, and flatfile publishes the room tree with no player snapshot copy. Rooms that can redirect or drop the weapon further refuse before admission. The quick-step miss and non-drop disarm branches refuse their direct player unequip while an epoch is active | Focused publication harness and SQL/flatfile authority fixtures cover refusal, replay, before/after slot, references and nested tree; add a live combat journey and restart before publication acknowledgement |
| Remove curse and word of recall item drops | Worn or carried item to room; custody move | Both spells inspect their item-dropping branch at entry and refuse it before any live item mutation while an epoch is active. Remove curse can still change an explicitly targeted object's curse flag without moving custody | `test_pa_item_admission.py` checks guard ordering; admit these branches through the item transaction and test live spell journeys before activation |
| Final charge on a teleport item | Durable item retirement after a portal move | A one-charge durable teleport item refuses active-epoch use before teleportation and before `extract_obj`; an ordinary charge decrement leaves UID custody unchanged | `test_pa_item_admission.py` checks guard ordering; admit final-charge retirement through the typed destruction route and test a live portal journey |
| Beholder disintegration | Equipped item condition and possible destruction with child release | The durable item branch refuses condition damage and extraction during an active epoch; spell damage to the character continues | `test_pa_item_admission.py` checks that the guard precedes condition and custody changes; implement a sourced sink with child custody transitions before enabling the item effect |
| Moonstone, bloodstone, general portals, and avatar focus allocation | Spell room objects, old object replacement, later timed cleanup | Creation branches refuse before replacing or allocating room objects in an active epoch. Continued avatar channeling also refuses before changing the focus timer or extracting it. These prototypes and decay paths still need a durable-versus-temporary classification; no source claim is inferred from `read_object` | `test_pa_item_admission.py` checks entry ordering; prove prototype reachability, pickup eligibility, timed cleanup and source identity before admitting the routes |
| Periodic `poo` generation in `magic/affects.c` | Fresh nontransient VNUM 51 room object; issuance | The periodic route refuses before its random allocation in an active epoch. The prototype cannot be picked up by an ordinary player but lacks `ITEM_TRANSIENT`; staff can still pick it up and its room UID remains durable until decay | `test_pa_item_admission.py` checks the parsed prototype flags and refusal ordering; assign a durable occurrence identity or prove a safe transient classification before admission |
| Spring, divine font, pond and wall spell objects | Spell allocation into rooms; issuance | Active-epoch calls refuse before `read_object`; wall refusal also precedes room exit flags. The divine font prototype is takeable and nontransient, while spring, pond and wall prototypes are nontransient room objects | `test_pa_item_admission.py` checks allocation ordering and the divine font flags; assign cast identities and retirement handling or prove a safe transient classification before admission |
| Shattering iceball room debris | VNUM 50 allocated after combat damage; issuance | The damage spell still resolves but withholds its nontransient room object before allocation in an active epoch | `test_pa_item_admission.py` checks the post-damage allocation guard; classify debris lifetime and assign a source if it must be durable |
| `falling_obj` in `magic/affects.c` | Event-driven object condition and room changes | An active epoch stops a durable UID before condition, height, room or event mutation; transient unaccounted effects retain legacy physics | `test_pa_item_admission.py` checks entry ordering; route durable room transitions and condition changes through native item authority before re-enabling falling |
| `DamageOneItem` / `MakeScrap` in `world/condition.c` | Damage to equipped and carried items; possible scrap issuance and destruction | An active epoch refuses durable item damage before condition changes and refuses the scrap helper before replacement, child release or extraction | `test_pa_item_admission.py` checks both guards; account for damage, child custody and the scrap object's source before admitting this route |
| Gather, fire, throw and reload in `combat/range.c` | Ammunition and thrown weapon custody, item condition and counts | Gather refuses during an active epoch before unequipping the quiver. Fire refuses durable quiver/ammunition and skips direct durable shield damage. Throw and reload refuse durable item mutations before combat or count changes | `test_pa_item_admission.py` checks the entry and per-shot guards; admit these routes through custody and condition authority, then run a live ranged combat journey |
| Disintegrate in `magic/spell_direct_attacks.c` | Equipped item condition and destruction, with child release | The equipped durable item branch refuses its condition and custody mutation during an active epoch; character spell damage still resolves | `test_pa_item_admission.py` checks the guard before condition, unequip and extraction; add a sourced item sink and child transitions before enabling the equipment effect |
| Spellbind in `classes/salchemist.c` | Epic point purchase followed by item condition, flags and affects changes | The command refuses a durable item before the purchase request or failure damage in an active epoch. A purchase committed before epoch activation requests a refund if its pending callback finds a durable item in an active epoch | `test_pa_item_admission.py` checks both guard points; couple the priced purchase and item modification under a single authority as part of the Plan 4 boundary |
| Player foraging in `cmd/actoth.c` | Unsourced collectible items from terrain | The player command refuses during an active epoch before the cosmetic template allocation or the terrain reward path | `test_pa_item_admission.py` checks guard ordering; a stable forage occurrence and source claim are needed before admitting item rewards |
| Level achievement gift in `world/achievements.c` | VNUM 400222 player reward and level achievement marker | Level 5 and level 20 reward branches defer while an epoch is active, before allocating the item or coin and before advancing the shared level marker. Other achievement counters still update | `test_pa_item_admission.py` checks both gates; add a durable achievement claim and couple item grant with marker advancement. The level 20 coin branch needs Plan 2 balancing |
| Mine nodes, ore and gems in `economy/mining.c` | Nontransient room nodes, node depletion, and collectible ore or gem rewards | Mine VNUMs 193 and 434 are untakeable by mortals but have durable UIDs and can be moved by staff. An active epoch refuses mine starts and queued completion events before node depletion, ore allocation, pick movement or reward publication. Periodic cleanup retains depleted nodes; automatic and staff placement refuse before allocation, and staff reset/load/purge refuse before extraction | Give each node placement and collectible reward a stable source claim, then account for node depletion and retirement before enabling the route. `test_pa_item_admission.py` checks entry ordering and prototype flags |
| Kingdom harvest nodes and personal material gathering in `kingdom/kingdom_harvest.c` | Nontransient room nodes, node depletion or retirement, and material item issuance | During an active epoch, reload sweeps retain their schedule but do not reap or place nodes. Room reaping, periodic cleanup and shutdown skip unsourced extraction. Realm and personal harvest commands refuse before scheduling work; queued ticks stop before vitality, node charges, realm deposits or material allocation | Assign persistent placement and gather source identities, then account for node state, retirement and personal material custody before enabling the route. `test_pa_item_admission.py` checks the node prototypes and refusal ordering |
| `VNUM_TRACKS` extraction in `magic/spell_visibility.c` | Cleanup of generated room projection | Track objects are untakeable and represent movement traces; this lexical `extract_obj` is classified as projection cleanup | Keep this classification tied to the track generator and prototype so a pickup or persistence change triggers re-audit |

Most current lifecycle source claims use
`(lineage, kind, selected UID or first sorted UID, reason ID)`. This survives a
different command ID and epoch for the same UID. The item-transfer v8 payload
also carries an optional logical source ID for nonquest creation. When supplied,
the source claim uses `(lineage, kind, logical source ID, slot 0)` and survives
allocation of a different UID; the reason ID continues to identify the item
template. The pre-entry multi-root grant and its underlying batch transaction
retain the same ID for the entire issued forest. The executable queue fixture
checks its encoded command and rejects an ID without a valid creation source.
SQL and flatfile fixtures refuse a second world-generation UID for
the same logical source. Creation grant APIs can pass this ID, but the withheld
world reset, spell, and loot routes still need durable producer identities and
publication journeys before admission. World quest rewards use
`(lineage, quest_completion, player PID, quest_started)`, so a new UID cannot
reuse the same completion. This depends on the saved quest start timestamp;
the value is retained as a per-player watermark after quest reset and advanced
for each newly created or shared quest, including quests begun in the same
second. Static quests, resets, and loot still need their own durable cause
identities before those routes can be called complete.

The SQL duplicate quest-reward and world-generation fixtures leave the second
UID absent from current ownership, the legacy event ledger, and accounting
operations. SQL maps a duplicate source-claim insertion to terminal `EEXIST`
after rolling back the enclosing transaction; flatfile also reports a terminal
conflict. Both preserve the first reward and publish no second copy.

Spell component retirement now uses the coordinator's retained publication
callback. Before updating runtime custody, the transaction compares every
committed UID and its complete live root/parent forest against the submitted
snapshot, including duplicate and unexpected children. After the durable
destruction and runtime custody update, the spell callback checks every
selected root's player custody, prototype, and destroyed lineage before
extracting any root or continuing the spell. A missing, moved, ambiguous, or
mismatched object retains the item fence for repair. Executable forest and
publication fixtures cover refusal and the accepted handoff; a live active-epoch
spell and restart journey remains open.

The SQL locker fixture deposits and withdraws a nested forest, verifies its
native row tree, affects, descriptions, current custody, exact references and
replay, and refuses a missing chest or a duplicate native UID without publishing
a new owner. It also rejects an equipped or ambiguous source row before copying
it into locker/pet custody. The flatfile fixture checks the same nested custody
and snapshot materialization with exact references and replay. The locker snapshot worker
keeps the player's object commands fenced until its sealed snapshot commits,
so an older in-flight snapshot cannot replace a later transactional locker move.

SQL player, recursive locker, and private-chest hydration now require a strict
nonzero decimal UID and matching owner before prototype allocation. Rows with a
missing, malformed, overflowing, or mismatched UID remain in their native item
table and are not published. `test_sql_saved_item_uid_restore.py` checks the
validation order in all three loaders and sanitizer-tests UID parsing
boundaries. Unknown player-item templates can still produce partial inventories;
selected-authority, complete-graph, and active-epoch receipt proof remain open
for these recovery routes.

The flatfile pet fixture reconciles a player snapshot saved before each custody
commit. It removes the handed-off forest from the player projection, retains the
same UID tree and metadata on the pet, and restores exactly one copy to the
player after return.

The disposable-schema SQL pet journey uses a real server and player commands to
move a nested backpack, empty satchel and trinket to a raised follower and back.
It checks native player and pet rows, current custody and metadata after each
move, auto-equips the trinket on the pet, and restores the follower with all five
UIDs after a server restart. The run holds the economic epoch inactive, so it
establishes live native custody behavior rather than active-epoch references.
The journey also passed at `68af73add` with the migration 0041 runtime
contract.

The disposable flatfile container journey claims a nested room item with player
get/put commands, saves the character, and restarts the server. Its
`--restart-only` mode verifies both UIDs, parentage, one current player owner,
unchanged custody revision, and visible container contents after reconnect.
The original mode separately verifies a real NPC claim of the nested item after
player drop, with one committed room revision and no duplicate player copy.
Both modes ran while the epoch was inactive; neither proves active-epoch
accounting references for those live commands.

The flatfile key-break mode claims a key, unlocks a chest with a guaranteed
break, and checks that the key disappears only after its UID is recorded as
destroyed. A read-only lookup confirms the retired UID, higher revision, last
root/parent, and destruction owner before and after a server restart. The
isolated repository fixture also checks historical root/parent for a destroyed
container and child. This live mode ran while the epoch was inactive, so it
does not establish a live accounting source claim or reference.

The disposable-schema SQL locker journey uses a real server to deposit a nested
backpack, leave the locker, save, restart, withdraw, and save again. It checks
current custody at each move and native player or locker rows, parentage, UIDs,
affects and extra descriptions at the save boundaries. This passed with both
the earlier `4d8d0e2bd` source snapshot and `68af73add` after its migration
0041 runtime contract was measured on MySQL 8.0 and MariaDB 10.11. The economic
epoch was inactive, so this does not establish live active-epoch references.

Migration 0038 adds equipment slots to the SQL current owner, opening baseline,
and legacy transition ledger. A root slot is one-based (zero means carried);
the player equipment array remains zero-based. Flatfile catalog version 5
retains the same position and reads earlier catalog versions as carried roots.
Both player save projections reconcile stale slot and parent placement against
current custody when the current item revision has an accounting reference (or
the opening baseline has a nonzero slot). Inactive legacy wear without such
evidence retains its native equipment position. SQL journal replay after a
committed wear restores the slot without changing item revision, and the SQL
loader selects the custody slot for a committed item before publishing the
player inventory. The disposable MySQL fixture exercises that loader after the
stale save replay and confirms the inactive legacy slot remains equipped.
Flatfile baseline creation also records the opening equipped slot. Both
backends now have isolated stale-slot and inactive-legacy regressions.

SQL MySQL 8 and MariaDB 10.11 fixtures submit two first claims for the same UID
from separate connections at the same time. They retain one current owner, one
native row, one legacy event, one exact accounting reference, and one source
claim. The flatfile authority gate exercises the same race with two threads.

Ground-item SQL handoff now records the source and destination payload digests
with the receipt. A missing or changed acknowledged destination withholds both
publication and retirement. A changed or cyclic unretired source withholds the
acknowledged destination and retains the receipt for reconciliation. The
handoff changes do not make a saved-item load a new item issuance.
