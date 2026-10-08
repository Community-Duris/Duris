# The Village of New Hope: mithral services and the guarded vault

Priority107 of the original220-zone queue. This is a **source-comprehensive map**;
played accounting/source/access/persistence qualification remains pending. The
[schema3/revision1 journal](../../../areas/story/newhope.story.json) maps four
independent crafting services, three contacts/21 verified aliases and eight
optional exact material checks. All four native accepted receipt identities
remain. Services give zero zone-story achievements or dailies; discovery remains
separate. New credit requires active, ready accounting; frozen recovery is separate.

## Source closure

| Source | Complete review and implication |
| --- | --- |
| [Quests](../../../areas/qst/newhope.qst) | All4 Q and15 addressed M, no ambient; exact offerings, rewards, dialogue and D0 flags. Five smith, seven janitor and three scout topic groups;21 distinct aliases. No accepted lore finale. |
| [Rooms](../../../areas/wld/newhope.wld) | All228 complete rooms89000–89227;210 full prose families,20 headers,6 non-exit metadata families,550 exact exits/177 relative patterns/36 complete exit-text families. Registry88970–89227/mode2. Village/inn/prison/trainers/fields/maljer huts/keep/royal court/bedroom/tunnel/underwater lair/vault fully reviewed. |
| [Mobiles](../../../areas/mob/newhope.mob) | All201 complete records89000–89200, full prose/numeric data. Villagers, guards, contacts, prisoners, trainers, Hammerhead, tentacler and apprentices; no reset-imported mobiles. |
| [Objects](../../../areas/obj/newhope.obj) | All190 complete records89000–89189: full text, flags, values, applies and extra descriptions. Ordinary mithral inputs/shining outputs, raw mithral, three access keys, vault equipment and scenery; no local type25 teleport or type29 control. All4 reset imports203/204/424/835 reviewed. |
| [Resets](../../../areas/zon/newhope.zon) | All1033 commands:D184/O37/P32/M571/G120/E69/F20.861 exact/880 parent-aware signatures/439 complete expanded argument/location/parent families. Header/caps/chances and mode2 retained; no R. No ten-shop stock substitutes for accepted ordinary mithral weapons. |
| Shops and compiled/shared execution | All10 full [shop records](../../../areas/shp/newhope.shp), stock/keeper/prices/policies; sole literal89181→[tentacler_death](../../../src/specs/specs.newhope.c) read completely. Shared read_mobile ACT_SPEC/teacher binding, static Q encounter/admission/refusal/accepted receipt, die/custom death/extract_char inventory release, setup_dir/reset door decoding, search/open/key/unlock/pick/movement and imported mindstone travel/anti-banishment reviewed. |
| Foreign closure | All4 local recipes plus the one touching imported-object recipe;4 complete Alatorin table-reuse reset groups,4 boundary edges/two active surface room records, all713 active type25 declarations scanned with zero incoming New Hope targets. Foreign recipe recipient/prototypes/target-room records read. Reuse/imports/boundaries remain separately owned. |

## Four independent native services

All are Vitrius89102@89083, D0, with exact native bindings retained:

| Service | Exact offering → reward | Fee |
| --- | --- | --- |
| Q35: shining short sword | I89117+I89141+C45000 → I89118 |45platinum |
| Q43: shining long sword | I89117+I89142+C100000 → I89119 |100platinum |
| Q51: shining two-handed sword | I89117+I89142+C250000 → I89120 |250platinum |
| Q60: shining dagger | I89117+I89140+C10000 → I89143 |10platinum |

These are paid support crafting, not a sequential campaign. Each requires fresh
raw mithral, the exact ordinary weapon and its fee. Reward89118 is not input89141;
reward89119 is not input89142. The long/two-handed services compete for independent
copies of the same ordinary longsword. One loose copy can show current preparation
for alternatives without proving both were settled. Output possession, coins,
equipped/nested/consumed inputs and unrelated shop weapons cannot prove acceptance.
All4 have native Unsupported durable offering exclusions. Active quest.c refuses
coin offerings; item admission rejects a coin goal. Do not bypass this guard.

Raw mithral89117 has Ocap2/probability100 at Zarix's cell89098 and maljer hut89162.
Court guards wield the three ordinary weapon types at89201/2/3, each cap3/E16.
Exact supplied material fits without personal source, theft, kill or prior ASK
history. Readiness is current stock; accepted receipts are history. Ordinary
mithral dagger89140, short sword89141 and long sword89142 are distinct from their
shining outputs, cracked vault junk and the black-handled shop weapons.

## Keep, access and custom reward closure

Janitor89165@89179 and centaur scout89166@89195 narrate Hammerhead's rule, a vault
and hoped-for relief. Sir Gamadir/Lady Lysaly, Zarix, the inn observer, Feralis and
Beth provide prisoner/trainer/surveillance/necromancer lore. No compiled or Q
endpoint records rescuing them, reporting to the scout, releasing souls or
liberating New Hope. The inn's mirror exists in room extra-description text;
no local teleport/control prototype makes looking at it travel or quest success.

Hammerhead89156@89189 carries petrified heart89019/Gcap1. It fits89188D→89218,
the hidden locked trapdoor beneath the bedroom rug. Native file6 masks to a
pickable door; reset6 adds secret/closed/locked. Reverse89218U file5/reset5 is
secret/closed/unlocked and has key0. SEARCH has a chance to reveal a hidden exit;
OPEN requires an accessible unlocked door, and held/loose keys fit UNLOCK while
nested ones do not. Shared-open routes do not prove a personal solve. Keep entry
89085N→89164 is closed/unlocked; no fabricated mandatory heart gate there.

Tunnel89219 has submerged prose but native sector26/swamp; lair89220 is native
underwater9. Preserve actual movement/breathing/combat/access policy rather than
changing a sector from prose. Tentacler89181 starts89220, carries eye89188/G and
wields lash89144/E16. Eye opens89220N→89227; cracked dagger89152 fits the reverse
89227S→89220. File3 makes both vault doors EX_PICKPROOF; reset2 closes/locks them.
Native reciprocal unlock/open settles both matching sides despite different key
IDs. Heart/eye/dagger break chances are0/10/1 percent. Two cracked dagger floor
placements have cap2. This return key may be intentional; no lock repair inferred.

ACT_SPEC_DIE is set on the tentacler, and read_mobile adds ACT_SPEC from its
assigned procedure. CMD_DEATH selects exactly one of89145–49 with number(0,4)
and attempts read_object then obj_to_room89227. This path has no Q receipt or
player participant attribution. The custom branch bypasses make_corpse; ordinary
NPC extraction releases its non-transient eye and lash on the lair floor, not
inside a corpse. The vault also has five independent Ocap1/20% reset rolls for
the same possible items, plus rusty plate89150/cracked sword89151 and two dagger
placements. A found item may predate the death. Runtime create/pickup/ownership
observations need exact event lineage before first-recovery or personal kill
credit; no current possession shortcut or guaranteed five-item drop is claimed.

Foreign Alatorin tables89008, imported spellbook/quill/mindstone/chillum and two
reciprocal surface edges do not create additional local quest branches. The
mindstone's own journey and worn anti-banishment behavior retain native guards.
Imported chillum835 is also a Newbie Red reward; that foreign quest's description
mentions only its hefty bag. Its full native reward includes835+5000XP+29310.
Keep foreign classification and source/campaign ownership explicit.

## Capability, builder and repair follow-ups

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-NEWHOPE-SOURCE-PROOF | Mithral89117 Ocap2 at89098/89162; court guards89170/1/2 at89201/2/3 wield ordinary weapons89140/2/1 respectively, cap3/E16. Add settled actor/source-root/item UID/generation/custody first-acquisition evidence distinguishing recovery, theft/loot and gifts. Loose supplied materials remain valid preparation; no forced personal kill or source chain. Chaos equipment arrays also include local89169/89136/89143; avoid exclusive-source claims. |
| ZSQ-NEWHOPE-MIXED-PAYMENT | All4 Q consume mithral plus an exact ordinary weapon plus45000/100000/250000/10000 copper-value coins. Active quest.c rejects coin offerings and durable item admission rejects non-item goals. Qualify atomic multi-root plus coin payment/accepted output/receipt, actor/recipient epoch, frozen price, failure preservation and replay/restart recovery before enabling. No coin-possession readiness or current payment claim. |
| ZSQ-NEWHOPE-RECIPE-CAPTIONS | Q35 actually requires89141 shortsword although its M asks a dagger; Q43 requires89142 longsword although M asks a shortsword; Q60 requires89140 dagger omitted by M. Q51 correctly requires89142. Ordinary dagger89140 ground caption says shining; shortsword89141 inspection says longsword. Proposed separate fix/news commit should reconcile text to accepted IDs after builder recipe-intent review; do not silently change recipes or invent an upgrade chain. |
| ZSQ-NEWHOPE-COLLATERAL-ALLOCATION | Shining89118/89119 are not ordinary inputs89141/89142. Longsword and two-handed recipes compete for separate copies of89142 and fresh mithral/fees. Qualify material reservation and consume-once allocation; one carried copy may prepare alternatives but cannot settle both. Vault keys, shining outputs and shop black-handled weapons cannot substitute. |
| ZSQ-NEWHOPE-ACCESS-AND-RETURN | Hammerhead89156@89189 carries heart89019;89188D→89218 uses it (file6/reset6, secret/closed/locked but pickable). Return89218U file5/reset5 is secret/closed/unlocked. Lair89220N→89227 file3/reset2 uses eye89188; reverse file3/reset2 uses cracked dagger89152 (two Ocap2 vault placements). File3 is EX_PICKPROOF after setup_dir, not a runtime closed bit. Key breaks0/10/1 percent respectively; has_key accepts HOLD/loose inventory, not nested. Capture successful settled search/unlock/open and matched reverse-door generation separately from shared-open access; preserve breathing/movement/PvP restrictions. Tunnel89219 is native swamp26 despite submerged prose; lair89220 is underwater9. Changing sectors or return-key design needs builder intent, no repair inferred. |
| ZSQ-NEWHOPE-CUSTOM-DEATH-EPISODES | Sole literal mob89181→tentacler_death, native ACT_SPEC_DIE plus read_mobile ACT_SPEC binding. CMD_DEATH attempts one random89145–49 read_object then obj_to_room89227; reset has five independent Ocap1/20% floor rolls. Custom die path bypasses make_corpse; ordinary extract_char drops Geye89188/E16lash89144 in lair89220 when non-transient. Add committed actor/participant/kill attempt, recipient/source epoch and exact created-object lineage; do not treat reset stock, floor pickup, key ownership or proc invocation alone as personal kill/first-recovery evidence. No Q receipt emitted here. |
| ZSQ-NEWHOPE-RECIPIENT-EPISODES | Vitrius89102 starts89083, Hammerhead89156@89189 and tentacler89181@89220; cap1/probability100/mode2, no local Q retirement. Keys, mithral, guard weapons and vault stock have separate caps/chances. Qualify actual availability, NPC/source generation and renewal before promising repeat visits or daily stock; all4 authored crafts remain services with no daily credit. |
| ZSQ-NEWHOPE-NARRATIVE-ENDPOINTS | Janitor89165@89179 and scout89166@89195 explain lord/vault/dead/keep lore; no accepted scout report, prisoner release, Hammerhead soul liberation, village transformation or necromancer repair endpoint. Observer89083@89036 uses a descriptive mirror, with no local type25 control. Future builder-authored campaign needs explicit accepted tasks, branches and settled world-state facts; distinguish prose, training, conversation and actual outcomes. |
| ZSQ-NEWHOPE-ONWARD-OWNERSHIP | All4 local recipes only; imports203/204/424/835 have one touching foreign recipe, Newbie Red29233@29234 Q529 produces835+5000XP+heftybag29310 from bullfrog eyes29241/herb826/blood825/dust824/root822. Imported mindstone424 has guarded lucrot_mindstone journey and worn anti-banishment behavior. Four Alatorin O groups reuse table89008 at nexus56-declared83513/14/16/18. Two active surface boundary rooms618434/618833 reciprocate89217; no incoming type25 target. No automatic campaign, source or discovery linkage. Preserve foreign journal ownership and existing travel guards. |
| ZSQ-NEWHOPE-REWARD-AND-RENEWAL | Preserve native4 receipt identities through raw4-achievement→authored4-service projection; earlier acceptance remains evidence while authored achievement total becomes0. Qualify actual mixed payment, consumed roots, selected reward/recipient custody and accounting recovery independently from projection. Custom vault production has no current receipt or frozen recipient attribution. Source proof, service acceptance and discovery remain distinct; new credit needs active, ready accounting. |

## Qualification and repair boundaries

Focused source/schema and compiled journeys cover exact independent contracts,
current loose materials, wrong/worn/held/reward-only stock, paid-service refusal,
contact visibility, service receipts/replay/cold recovery and raw4→authored4
reclassification. Synthetic accepted receipts qualify projection only; actual
mixed payment, source recovery, death production, key use, breathing, movement
and accounting persistence still require committed gameplay journeys.

The concrete proposed repair is truthful smith/ordinary-weapon text after builder
recipe-intent review. Changing recipe IDs, sectors, source caps, key design,
mobility or inventing a liberation finale is a separate content decision. No
native repair, accounting activation or gameplay qualification ships here. Any
selected native repair requires its own named fix/news commit and before/after
validation, clearly separated from this journal authoring checkpoint.
