# Money and supply policy for Plan 2

This records the policy for money and supply accounting. It does not grant
admission to a writer. A route is admitted only when its selected backend can
commit its native change, accounting evidence, source claim when needed, and
receipt under one root, and an executable journey proves that boundary.

## Holdings and ordinary transfers

Wallets use the durable player wallet lifetime, shared banks use the durable
account/racewar bank lifetime, and coin piles use their item UID. A pile's
custody-only movement does not change its money holding. A value change records
the exact four-denomination before and after vectors and native revisions, with
balanced copper-value postings. Making change is a denomination effect even
when total copper value does not change. Live room publication keeps each
accounted coin pile's UID separate; the old automatic room merge changes two
pile values without an accounting root, so it is disabled in an active epoch.

An ATM operation moves value between one wallet and one shared bank. A peer
give moves value between two player wallets; the recipient must resolve to a
durable player wallet before submission. A split is one child transfer per
eligible recipient under the existing command boundaries: the splitter keeps
the integer remainder. Group membership, room, visibility, and morph-to-player
identity are checked before each child's admission. A morph and its original
player count as one wallet recipient. Active `do_split` waits for
each final child receipt before admitting the next transfer. A later refusal
leaves earlier shares transferred and the remaining shares with the splitter;
there is no all-or-nothing group receipt. The command fixture checks a morph
recipient, integer remainder, and later-child refusal. A full server journey
through each backend is still needed. After a process restart, an admitted
child is recovered from its own receipt; later unsubmitted shares are not
resumed from process-local split state.

NPC cash has no specified durable holding lifetime or reset/death/loot policy.
NPC balances and keeper cash therefore cannot be used as transfer
counterparties in an active epoch. The existing VNUM exceptions do not create
an accounting identity. The generic `ADD_MONEY` and `SUB_MONEY` helpers refuse
active-epoch calls, including NPC calls; direct assignment sites still require
separate source review before activation. Numeric `give` admits a resolved
player-to-player wallet transfer, including a morph whose original player wallet
is live. The named denomination is debited and credited exactly. It refuses
ordinary NPC recipients and staff grants without a sender debit before
submitting any active-epoch coin command.

Ship coffer claims have no unified holding-to-wallet root. The captain's
`get money` command therefore refuses in an active epoch before crediting the
wallet or clearing the ship coffer. An extracted-function regression verifies
that refusal and the unchanged inactive claim. Coffer reward and insurance
sources still need their own accounting policy and admission evidence.

## Supply events

An issuance, expense, or restitution uses a versioned reason and one durable
logical source event. A command's generated operation ID is a retry identity,
not a substitute for the source event. The source claim is unique across
retries, restarts, and epoch changes. The counterparty and sign are fixed by
the reason: issuance credits an ordinary holding, expense debits one, and
restitution reverses a named prior loss. Opening equity is restricted to a
witnessed baseline and cannot fund gameplay.

The existing SQL ATM adapter owns a wallet and bank transfer only. The legacy
`currency_prepare_mutation` advances both wallet and bank revisions even for a
wallet-only delta, and its command has no durable source event. A SQL supply
route therefore needs its own wallet native-revision preparation and locked
authority binding, a source claim unique for the logical event, balanced
wallet/issuance-or-sink postings, and the native write, evidence, and receipt
in one root. Reusing an ATM writer ID or a fresh operation ID alone does not
qualify a grant or expense.

An inactive Chaos starter bank adapter now freezes the existing deterministic
`CHAOSEED` player identity as a `starter_grant` source, requires the exact
1,000,000-platinum bank credit, and prepares a balanced bank/issuance plan. It
rejects a changed operation ID, source, amount, or authority witness. The SQL
bank owner checks the persisted pending bit in `player_data.act3` under its
native lock, refuses an already claimed source, and commits the native ledger,
accounting plan, unique source claim, receipt, and outbox in one root. SQL
advances the unchanged wallet's revision along with the bank revision because
the existing currency ledger requires a unique wallet revision for every row;
the plan records that exact native effect. Retained replay verifies the source
claim without requiring the pending bit to remain set. The flatfile owner reads
the atomically saved player snapshot's `act3` status under its accounting
authority lock and commits its native wallet and bank revisions, balanced plan,
unique source claim, and receipt in one recoverable root. Its replay verifies
the claim without rereading the pending bit. Both owners resolve current native
revisions when this sourced reward commits, so an intervening ATM operation
does not strand its deterministic ID. The gameplay currency producer now freezes
this grant through the selected wallet and bank projection and submits the
typed command for publication; the producer fixture covers both backend builds.
The flatfile native fixture also commits that gameplay-frozen command with its
wildcard bank revision, then verifies the source claim, intervening ATM rebase,
retained replay, and crash recovery. The pooled SQL fixture takes the same
gameplay-frozen command through the coordinator and verifies its native bank
credit, balanced postings, source claim, receipt, and retained replay on an
isolated MySQL 8 schema. A live player journey through each backend remains
pending.

The versioned reason registry requires a source event for service, training,
locker, shipping, insurance, guild, crafting, gambling-stake, shop-buy,
collector-purchase, auction, death-transfer, item-reward, and quest-cost
composites. It checks
the source kind for the named reward, service, crafting, gambling, auction,
baseline, and lifecycle families. Restitution also names an original operation.
These structural checks reject a missing or mismatched event; the domain
adapter still must prove that the event belongs to this player and has not
already been claimed under another operation ID.

`item_reward` names one item action that can retire an item or coin pile and
issue an exact wallet award; its source kind is `item_action`. `quest_cost`
names a quest action and requirement slot; its source kind is `quest_action`.
These are registry contracts for presently refused routes. Their native item,
quest, NPC, and wallet effects still need one transactional root.

| Route | Required source identity and money effect | Active-epoch disposition |
| --- | --- | --- |
| Quest, chaos, epic, achievement, boon, and loot grants | Durable completion or generation ID and slot; issuance to the resolved player holding | Refuse until the domain publishes that identity and an atomic adapter |
| Legacy quester coin requirement or reward | Quest completion plus exact requirement/reward slot; NPC cash needs a finite holding policy | Refuse a coin gift or any money-bearing completion before the NPC receives an item or coins in an active epoch |
| Mercenary world quest cash | Quest completion ID and reward slot, retained with item and epic rewards | Refuse the level 25+ mercenary reward before item grant, kill progress, or completion state changes in an active epoch |
| Ship-owner Sailor's Tattoo cash | Achievement completion ID and cash alternative slot | Leave the award pending before the completion marker and cash message in an active epoch |
| Administrator grant | Durable operator action ID and target; issuance or documented correction | Refuse until the action and authorization are retained with the root |
| Staff junk reward | Item UID and staff action ID; item destruction with the exact wallet issuance | Refuse junk before object extraction or cash award in an active epoch |
| Llym's altar offering | Treasure UID and offering ID; item retirement plus the chosen blessing, cash issuance, or other reward | Refuse the offering before treasure extraction in an active epoch |
| Dump room reward | Dropped item or pile UID and action ID; exact destruction and wallet issuance | Skip implicit cleanup and refuse drops before extraction in an active epoch |
| Service, training, guild, crafting, shipping, insurance, and tax costs | Durable purchase/action ID; expense or transfer to a real treasury if one exists | Refuse until goods and cash share the existing critical boundary |
| Pending claim after a failed reward | The original reward event ID; transfer from a durable claim holding at collection | Never mint again at collection; refuse until the reward and claim share one source claim |
| Non-auction pending claim | Durable claim lifetime and the originating operation | Refuse until claim creation and collection are accounted |
| NPC or keeper cash | Stable treasury or NPC lifetime, generation and retirement policy | Refuse until a finite durable holding policy exists |
| Alternative quest cash action | Durable quest completion/reward slot for issuance; finite NPC holding or expense event for required cash | Skip the entire action before item, tag, NPC cash, or wallet mutation in an active epoch |
| Coin steal | Two durable player wallets and exact stolen denominations; NPC target needs a finite holding lifetime | Refuse before victim resolution, live victim decrement, or thief credit in an active epoch |
| Smelter coin/ore give | Durable service event, NPC cash policy, and atomic ore/cash result | Refuse the special procedure at `CMD_GIVE` admission before either coin or ore mutation in an active epoch |
| Permanent stat potion | Durable purchase ID, exact wallet expense, and permanent stat result | Refuse `stat_shops` buy before the debit or stat spell; the legacy path ignores a refused `SUB_MONEY` result |
| Patrol hiring | Durable hire ID, exact wallet expense, and spawned patrol identity | Refuse `patrol_shops` buy before mob creation, debit, or patrol publication; listing remains available |
| Pet purchase and stable rental | Durable purchase or rental ID, exact wallet expense, and pet or ticket state | Refuse `pet_shops` buy, rent, and ticket return before mob creation, pet storage, ticket movement, or debit; listing and rental quotes remain available |
| Locker entry and private chest creation | Durable visit or chest purchase ID, exact wallet or bank expense, and selected locker or chest state | Refuse paid personal locker entry before locker loading and room movement; refuse private chest creation before password work or SQL insertion, and recheck when its asynchronous callback runs |
| Item enhancement | Durable action ID, exact wallet expense, donor/material consumption, and resulting item state | Refuse `do_enhance` at command admission before either enhancement implementation runs |
| Guild founding | Durable confirmation ID, exact wallet expense, and guild creation | Consume and refuse active founding confirmation before `found_asc` creates the guild |
| Guildhall construction | Durable construction ID, exact wallet expense, construction-point cost, and resulting guildhall state | Refuse all six recognized `construct` actions at the shared dispatcher before a cost or structural change; keep command help available |
| Epic task clearing | Durable task-clear ID, exact wallet expense, and removed epic-task affect | Refuse the paid `clear_epic_task_spec` prayer after task lookup but before affect removal or wallet debit; free task information remains available |
| Item prayer collection | Durable stash item UID, exact wallet expense, and custody movement | Refuse `pray_for_items` only when a matching stash item exists, before item movement or debit; unmatched prayers remain available to other handlers |
| Home relocation | Durable home-change ID, exact wallet expense, and saved birthplace result | Refuse `do_home` after eligibility checks but before saving home fields or debiting the wallet |
| Epic skill training | Durable training ID, epic-point and wallet costs, and learned skill result | Refuse `epic_teacher` purchase before submitting the epic-point debit; a rejected callback coin submission requests an epic-point refund before the skill grant |
| Ordinary skill practice | Durable lesson ID, exact wallet expense, and learned skill increment | Refuse a priced `do_practice` lesson before debit or learning in an active epoch; require a successful debit in inactive mode and keep zero-cost spell copying and price listings available |
| Paid mail send | Durable message submission ID and exact wallet stamp expense | Refuse `postmaster_send_mail` before debit submission or message editor state; mail receipt remains available |
| Monk remort | Durable training ID, exact wallet expense, class and spell transition | Refuse the `remort` ask branch before debit submission or class mutation |
| Paid character rename | Durable rename request ID, exact wallet expense, character identity and ship-owner change | Refuse `mob_do_rename_hook` at named ask admission before identity changes or wallet debit |
| Artifact location service | Durable request ID and exact wallet expense; the NPC has no durable cash lifetime | Refuse `llyren` at paid listing admission before payment, NPC cash clear, or artifact disclosure |
| Rented cleric spell | Durable service ID, exact wallet expense, and spell outcome; resurrection also changes corpse custody | Refuse named `rentacleric` buys before payment, NPC cash clear, or spell while leaving free price listings available |
| Witch doctor elixir | Durable purchase ID, exact wallet expense, and persistent affect result | Refuse `witch_doctor` buys before payment or affect while leaving the free listing available |
| Flight and ferry tickets | Durable purchase ID, exact wallet expense, ticket UID creation, and refund outcome | Refuse ticket purchase before debit submission or ticket creation, including free ticket variants lacking a source-backed item admission |

The boon result is selected inside its durable repository and can contain cash.
New boon completions therefore refuse at `boon_reward_transaction_submit` in an
active epoch, before the completion command is queued or progress is changed.
The executable refusal regression checks that no operation ID, command, or
submission is created. Previously admitted boon results still require the
activation drain before this boundary can be relied upon.

Shops, auctions, collector, death, and crafting composites are handed to Plan 4
for their native commit integration. Their cash legs still obey this source and
counterparty policy. The inactive collector purchase component currently omits
a source event; it must bind the listing's finite source before activation.

## Blackjack

The two blackjack procedures use the same economic rule. A wager moves the
stake from the wallet to a held-stake account. A push returns only that stake.
A win returns the stake and issues winnings equal to that stake. A loss, player bust,
or fold expenses the held stake. The player-visible coin denomination of each
leg must be retained, even if an equivalent copper value could be paid another
way.

Before a round may be admitted, the table needs a durable item UID and each
wager needs a durable round ID tied to that table and player. The round ID is
the source event for its single terminal outcome. If play is interrupted after
the stake commits and before a terminal result commits, recovery refunds the
held stake as the same round's terminal restitution; it does not draw another
hand or issue winnings. Activation must verify that no legacy stake is in
flight. Both current blackjack entry points refuse active-epoch play until
the held stake, round recovery, and selected backend commit are implemented.
The versioned contract now reserves account kind `gambling_stake` for a finite
table UID and round sequence. A stake moves the named denomination from wallet
to that holding under source slot 0. A terminal push or win returns the stake
to the wallet, with issuance only for net winnings; a loss expenses it, and an
interruption refunds it. Terminal roots use source slot 1 and name the original
stake operation. Structural tests require the held account, one denomination,
balanced postings, source stage, round sequence, and an original-operation
reference. The backend must verify that reference against the committed stake.
No backend yet owns or recovers this round holding, so these contracts grant no
gameplay admission.
The live `magic_deck` dealer-bust path settles synchronously and credits two
stakes: the returned wager and an equal win. Its unused periodic dealer branch
was removed: the function-local `game_on` is initialized to `BJ_PREBID` and
assigned only `BJ_PREBID`, `BJ_POSTBID`, `BJ_POSTDEAL`, or `BJ_POSTHIT`, never
`BJ_DEALERSTURN`. The typed payout contract rejects an issuance larger or
smaller than the held stake.

## Current executable boundary

The SQL wallet-to-pile drop, full pile pickup, pile-to-pile split and merge,
change making, and peer-wallet fixtures exercise schema-2 roots through the
connection pool. They check native revisions, exact denomination effects,
balanced postings, custody references where applicable, retained replay, and
reconcile on disposable MariaDB 10.11 and MySQL 8.0 databases. An injected
item-endpoint failure during pickup rolls back native state and accounting
evidence, then succeeds with the same operation ID. The SQL split-child fixture
commits two successive wallet transfers for an 11-copper split, verifies each
root's native revisions, balanced postings, receipt, and replay, and retains
the splitter's five-coin remainder. Morph resolution and a rejected later child
are exercised by the extracted command fixture; full gameplay remains pending.
The flatfile dispatcher supports typed ATM and wallet-to-wallet coin roots. The
flatfile peer fixture checks two native wallet images, the shared bank's
per-leg revision advances, balanced evidence, exact replay, restart, and a
retained stale-revision rejection. It also interrupts an authority commit after
the first image and verifies that journal recovery exposes one retained root
without a second debit or credit. A separate three-wallet split fixture commits
two sequential roots, checks wallet identities, balances, revisions, and
balanced postings, retains the integer remainder, and rejects a stale later
child without undoing the first. It replays both receipts after recovery. The
flatfile fixture also covers wallet-to-pile creation, full pile pickup, and
pile-to-pile split and merge with the native items, wallets where applicable,
UID-keyed pile heads, item references, and an accounting receipt in the same
authority commit. A retired pile head prevents
UID reuse. The fixture checks stale pile refusal, detects a damaged head on
replay, and checks an exact wallet denomination change during a pile drop.
Each split transfer has its own root receipt; flatfile does not claim separate
receipts for the native images within one root.
An isolated pre-existing pile journey reads the locked native ownership row and
exact legacy player snapshot for a nonstandard money VNUM without a cached coin
payload, commits its holding/item baseline witness and UID-keyed head with one
receipt, then spends the pile through a schema-2 pickup. The flatfile
inventory checks every active ownership entry against its native snapshot when
it has no retained coin payload. It reads player and pet inventory from player
snapshots, and room, corpse, and locker inventory from their domain stores. A
player snapshot match must belong to the recorded player or pet; it includes
legacy money objects even with a nonstandard VNUM and skips ordinary items only
after matching their UID and VNUM. Missing native evidence or an owner mismatch
fails enumeration. The native fixture covers a legacy player money item, an
ordinary item, a pet money item, an owner mismatch, and a missing snapshot.
Other owner stores still fail closed. The native lifecycle capture now
enumerates every player wallet and shared bank under the authority lock,
matches wallets to retained identities, rejects active identities missing a
wallet or bank, and binds each captured balance to its native revision and
source digest. The native coin fixture verifies that those exact revisions and
digests survive pure baseline preparation and command construction. Complete
UID coverage across those stores and lifecycle installation remain open before
activation.
A direct-assignment review found the reachable alternative
quest cash path in `src/cmd/nq.c` and the victim-decrement path in
`src/cmd/actoth.c`; both are now listed in the writer registry and refuse
active-epoch cash actions before native mutation. The smelter's direct cash
transfer and ore/cost sequence also refuse at their shared command entry.
Other direct cash assignments in the current lexical review include character
initialization, NPC prototype generation/clearing, durable-state hydration,
and committed balance publication. Their remaining call paths and the broader
writer coverage matrix are not yet complete, so activation remains blocked.
The executable stat-shop fixture checks all nine choices in active and inactive
mode. Source-order checks pin the enhancement, guild confirmation, and epic
teacher guards before their separate native mutation or transaction submission.
These refusals do not supply the purchase source identities or atomic adapters;
the routes remain unavailable in an active epoch.
The extracted patrol and pet shop fixture exercises active refusal for patrol
hiring, pet purchase, stable rental, and ticket return before a debit or native
service effect. It also checks that listings and rental quotes remain available
and that all four legacy actions still execute while accounting is inactive.
These services still need typed source events and atomic money and gameplay
commits before activation.
The paid locker fixture checks refusal before personal locker loading or room
movement and before a private chest hash job or SQL insertion. The chest
callback checks the epoch again before insertion, including when activation
occurs after the password job was queued. Identification receipt reads and
chest listings remain available. Paid locker services still need one source
event and atomic fee and native state per operation.
The ordinary skill practice fixture checks the extracted fee guard against
active refusal, a failed inactive debit, and a successful inactive debit. The
production command places that guard before the learned-skill increment while
retaining zero-cost spell copying and price listings. Paid lessons still need
a durable source and atomic wallet and skill state.
The mail-send fixture exercises active refusal before the stamp debit and editor
state, plus the inactive path. Source-order checks cover monk remort and both
ticket procedures. Their separate debit, item, and class/message outcomes are
still not atomic and are intentionally unavailable in an active epoch.
An extracted production-function harness executes the artifact-location active
refusal and inactive payment branch. Source-order checks pin its guard and the
rented-cleric and witch-doctor refusals before their payment and service effects.
These three routes still need full gameplay refusal journeys and typed service
roots on each backend before their activation policy can be qualified.
An extracted production-function harness executes paid rename refusal before
character or ship identity changes and wallet debit, and verifies the inactive
payment path. A typed rename source and backend gameplay journeys remain pending.
The guild construction dispatcher harness executes active refusal for guildhall,
room, golem, upgrade, rename, and overmax, while retaining help and inactive
dispatch. Their wallet, construction-point, and structure changes still need
one durable source per action and backend gameplay journeys.
Extracted production-function harnesses execute active refusal and inactive
behavior for epic task clearing, item prayer collection, and home relocation.
Their affect, item, or saved-home changes still need the same durable root as
their wallet expense and full backend gameplay journeys.
