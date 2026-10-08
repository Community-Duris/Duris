# Currency and banking authority boundary — 2026-10-08

This is a maintained-source boundary map and a design proposal. It prepares a
replaceable currency domain without introducing another implementation, command
format, authority, framework, migration or execution claim. The useful next work
is acceptance preparation and an adoption comparison; native money, recovery and
publication ownership still prevent treating numeric extraction as a completed
currency domain.

## Frozen scope and evidence

All maintained-source references below use commit
`a6aec4c4a058a5174e5b679708103762fa05e05a` on `experimental-accounting`.
The read-only source archive SHA256 is
`38faf6c8ddf050de9008af2b5315ea040ab5d6fe763d5f63c4066f012c79ef51`;
its extraction is `D:\Dev\Temp\currency-authority-20261008\source`.
Line numbers refer to that frozen body, not to a moving branch. The final appendix
pins every cited repository file to its Git blob.

The owned preparation baseline is
`c84786260d3eb7e2ea041f461bbb5450c91542ec` on
`codex/accounting-domain-separation`. Its [R2 reservation](R2_RESERVATION.md)
and [operation inventory](OPERATION_INVENTORY.md) are separately pinned in the
appendix. R2's `24fa551ae` numeric preparation and its previously reported numeric,
build and 84 selected SQL/flat completion checks remain their original scoped
evidence. The maintained snapshot mapped here still contains the local
`wallet_value_delta` and `bank_payment_deltas` bodies and has no
`currency_value_plan.h`. This document does not imply that R2 has been adopted
into this maintained snapshot or that its component checks qualify the authority
boundary described here.

The operator provider recipe repair has already been adopted by primary at
`bf1aaad37`, with coordinator record `c8c89c29a`. Its closed qualification is not
reopened. No build, test, server, database, Docker, migration, gameplay or recovery
execution was performed for this document. Tests below were inspected as source.
The continuing Goal remains **BLOCKED and unfinished**; this finite document
neither resumes it nor satisfies Plans 1–5 or applicable original R1–R8.

## Existing types and the boundary they actually provide

| Existing surface | What it owns | What it does not establish |
| --- | --- | --- |
| `src/economy/currency_command.h`: `currency_vector`, `currency_command_payload`, `currency_command_result`, `currency_prepared_mutation` | Four signed 64-bit denomination values; bounded PID/account/racewar/reason data; fixed payload/result; owned before/after state and revisions after validated preparation. | A live character, a lock, active mapping, a transaction, a physical pile or publication permission. |
| `src/economy/currency_command.c:285`: `currency_prepare_mutation` | Pure checked application to captured state, existing revision policies and refusal codes; output assigned only on success. | Reading native or durable current state, writing a ledger, or proving a receipt current. |
| `src/economy/economic_currency_adapter.h`: `economic_currency_authority`, prepared currency/accounting plan | Existing epoch, account identities and fence keys paired with owned state; exact agreement between the mutation and the accounting plan. | Holding the authority whose values were copied. The SQL or flat owner must authenticate and retain it. |
| `src/economy/currency_transaction.h:70`: game-thread admission, submit, completion and publication entry points | Native capture, pending continuations, original operation identity, live wallet/shared-bank publication and callback lifetime. | Durable commit from a successful Boolean submission, or a generic physical-recovery capability. |
| `src/economy/currency_repository.h`: `currency_repository_execute` | Legacy currency mutation on an already open SQL transaction. | Beginning or committing a separate transaction. |
| `src/economy/currency_sql_mutation_writer.h` and `src/flatfile/currency_flatfile_mutation_writer.h` | Private mutation access for the existing typed bank transaction owners. | A public writer or an accounting receipt. |
| `src/economy/currency_publication.h` | Awaiting, ready, offline, callback-retry and blocked-receipt states. | Equivalence between durable completion and completed live publication. |

`currency_account_key` (`currency_command.c:114`) case-folds account identity and
includes racewar in a coordinator fence key. It is not the SQL bank row ID, an
accounting mapping ID or a native lifetime identity. Keeping these identities
distinct is necessary even when their numeric representations happen to agree.

The pure mutation policy is already separated. It rejects invalid policies and
`INT64_MIN` deltas; validates every opening count in `0..INT_MAX` before stale
revision checks; refuses insufficient funds with `ENOSPC`, overflow with `ERANGE`,
and incompatible expected revisions with `ESTALE`. `bank_only` requires a
nonzero bank change and no wallet change, and advances only the bank revision.
SQL and flat legacy policies normally advance both revisions even if one vector
has zero delta. Existing SQL and flat policies deliberately preserve different
denomination error ordering. Rebasable positive wallet rewards and chaos starter
bank rewards have specific rules (`currency_command.c:134`); a generic bank
reward is not automatically rebasable.

The adapter's ATM preparation (`economic_currency_adapter.c:188`) authenticates
its complete frozen binding against supplied authority and requires opposite
wallet/bank denomination movements with conservation. Its chaos starter and
quest wallet reward paths (`:299`, `:416`) have separate issuance/source proof.
This is not a general spend/payment counterparty adapter. Moving arithmetic does
not authorize any additional active-mode producer.

The real gate is `economic_gameplay_authority::prepare_currency`
(`src/economy/economic_gameplay_authority.c:543`). With selected ordinary active
authority, a fresh schema-1 command can become schema 2 only for ATM
deposit/withdraw, chaos starter reward or the specifically identified recovery
quest-wallet reward (`:583`), and only with mapped wallet and bank coverage.
Fresh generic wallet spend, bank payment and bank reward return incomplete
coverage. Already frozen schema-2 commands retain their original epoch/lifetimes;
previously accepted schema-1 commands cannot be retagged. SQL recovery and
wallet-root qualification scopes have additional refusal rules (`:561`, `:567`).
Consequently the prepared locker payment path described below is useful legacy
and future admission preparation, not a currently supported new active payment.
Coin roots use the separate `prepare_coin_transfer` (`:628`) gate and existing
endpoint intent; currency preparation cannot stand in for that owner.

## Native entry points and captured state

| Real entry and source pin | Capture and command effect | Admission, visible result and refusal |
| --- | --- | --- |
| ATM `do_deposit`, `src/cmd/actoth.c:2534`; registered in `src/cmd/interp.c:3129` | Reads PC identity, ATM presence and carried denomination counts. The `all` route freezes all positive counts into one wallet-debit/bank-credit vector and submits one command (`:2570`); explicit denomination uses one command (`:2621`). | No native pre-debit. Parser and ATM checks remain native. Existing `strstr("all", argument)` parsing is part of the current behavior, not redesigned here. Completion is `atm_transaction_complete` (`:2502`); its later ATM check controls balance presentation, not durable commit. |
| ATM `do_withdraw`, `actoth.c:2632`; registered in `interp.c:3248` | Parses a positive denomination amount, freezes wallet credit and bank debit, submits at `:2677`. | The authoritative repository decides bank sufficiency; a stale native bank view is not a durable funds check. Terminal failure is reported through the same completion. |
| `ADD_MONEY`, `src/core/utility.c:3116` | For persistent PCs, submits positive wallet value with reward reason (`:3145`); value conversion is in `currency_transaction.c:1037`. | Active unsupported cash paths refuse (`utility.c:3128`). Admission failure can preserve the reward through the existing auction pending-money route; it is not proof of a paid wallet. NPC/local fallback still assigns native cash (`:3164`). |
| `SUB_MONEY`, `utility.c:3301` | Checks native money and mode, then persistent PCs submit a wallet spend (`:3314`); NPC/local paths still mutate cash directly. | Active unsupported spend paths refuse. A zero return from the wrapper follows accepted submission, not acknowledged durability. A caller must not interpret it as permission to irreversibly deliver a purchased effect. |
| `SUB_BALANCE`, `utility.c:3289` | Positive mode-zero payment goes through `currency_transaction_submit_bank_payment` (`currency_transaction.c:1634`). `bank_payment_deltas` (`:1558`) captures bank counts, consumes denominations in existing order and returns change to the wallet. | This wrapper also reports admission, not final payment. Preserve its change, insufficient-funds and revision behavior. It cannot make an arbitrary service delivery atomic. |
| Locker identification, `src/item/locker_identify.c:396` and `currency_transaction_prepare_identify`, `currency_transaction.c:1595` | Chooses wallet payment if native wallet value covers the cost; otherwise prepares the whole bank-funded payment, rather than blending wallet and bank. Freezes one identified command and text receipt before submission. | Durable receipt first, exact `submit_prepared` second (`locker_identify.c:354`). `paid` (`:103`) records paid/failed only for the matching original operation; showing a paid receipt and then recording delivered occurs at `:321`. Failed receipt I/O retries without inventing a new charge. |
| Wallet producer families | Actual calls include quest (`src/world/quest.c:1507`), chaos (`src/combat/chaos.c:228`), epic (`src/world/epic.c:286`), auction pickup (`src/economy/auction_houses.c:2979`) and shop (`src/economy/shop.c:870`). Existing bank rewards also have callers. | A call site is not proof its active-mode reason is admitted or its enclosing service is atomic. Quest producer work remains with the quest owner; this map does not reserve it. |

`currency_transaction_can_submit_nonrebasable` (`currency_transaction.c:1133`)
adds absence of pending publication to basic admission (`:1110`): valid persistent
PC/account, capacity, established flat baseline and clear player/account fences.
`currency_transaction_player_busy` (`:1183`), through `pending_affects_character`
(`:1139`), includes the shared account/racewar,
not just PID, and coin wallet endpoints. Thus another character on the account
can remain blocked by an unpublished bank predecessor even after another layer
has released a fence. A blocked rebasable obligation also prevents accepting
another reward behind it. Item UID busy checks remain separate (`:1198`).

`currency_transaction_submit_identified` (`:1368`) uses the original operation ID,
captures native wallet/bank revisions for ordinary commands and uses the existing
wildcard revision policy only for the recognized rebasable cases. Preparation
occurs before submission. `currency_transaction_submit_prepared` (`:1420`)
rechecks identity and exact command binding, retains a pending owner before
coordinator submission, attaches exact duplicates, and removes it if admission
does not keep the operation. A producer timestamp cannot admit a new schema-1
command while accounting is active. Pending capacity and allocation failure are
admission failures, not debit or credit outcomes.

## Durable owners: common policy, different storage contracts

The coordinator retains the canonical original command and publication-required
bit before journal submission (`src/persistence/critical_command_coordinator.c:1698`).
Schema-2 retry does not normalize or allocate a replacement command. Exact ID/body
replay attaches; conflicting content refuses. Coordinator durability, repository
durability and game-thread publication are separate handoffs.

### SQL legacy currency and typed bank roots

`execute_currency_state` (`src/persistence/critical_command_repository.c:1523`)
locks the actual player and account-bank rows, checks current account/racewar,
captures both denomination vectors and revisions, and applies the SQL policy.
`write_currency_state` (`:1334`) writes guarded wallet/bank revisions and the
currency ledger, including opening baselines where needed. The actual
implementation is in this repository file; the repository header is not a second
writer.

For typed bank operations, `economic_sql_bank_transaction::prepare` in
`src/persistence/economic_sql_bank_transaction.c:523` borrows the root's original MYSQL
session. It authenticates the pending inbox and canonical hash, resolves the
bank mapping hint to the actual native bank ID, locks lineage/epoch/mappings,
locks current native rows, and prepares the existing adapter on that state.
The original session must remain in the original transaction with reconnect
disabled (`:247`). Only the supported business refusals become typed rejected
results; corrupt or unauthenticated current state is not converted into a
successful terminal business decision.

`apply` (`:627`) checks its transaction/savepoint marker, pending inbox, exact
before state and absence of conflicting ledger/evidence before invoking the
private writer. `finalize` (`:668`) revalidates current authority, exact written
balances/revisions and ledger, appends accounting evidence, then rereads and
revalidates. These borrowed operations do not COMMIT. The outer account-bank /
currency root (`critical_command_repository.c:2530`) owns result encoding,
outbox, typed finalization, inbox completion, root verification and COMMIT
(`:2598`). A lost COMMIT reply leaves the original operation ambiguous; replay
must resolve that ID. It cannot safely compensate or submit a second payment.

Historical receipt verification is intentionally different. In
`economic_sql_bank_transaction.c:796`, retained verification authenticates inbox,
original command/result, reconstructed before/after plan, ledger and retained
mapping history. It does not require today's active mapping, epoch, name or
balances to equal the historical operation (`:922`). Fresh root verification
(`:738`) additionally checks current authority and the current after-state.
A valid old receipt alone cannot authorize current native publication or a new
mutation.

### Flat typed bank and legacy currency paths

`src/flatfile/flatfile_accounting_dispatch.c:19` routes schema 1 to the legacy
dispatcher and schema 2 only to its selected typed owner. Unsupported typed
commands return `ENOTSUP`; there is no legacy fallback.

`flatfile_accounting_bank_transaction::apply`
(`src/flatfile/flatfile_accounting_bank_transaction.c:273`) acquires identity
before accounting authority, recovers interrupted journals, then checks the
original retained receipt before fresh identity/current-epoch admission. New
work verifies actual identity/account/racewar, authoritative domain state,
mapping/lifetime and current metadata. Existing starter-grant snapshot/source
requirements remain specific to that producer; they do not license other bank
rewards.

The private writer (`src/flatfile/flatfile_player_domain_repository.c:889`) verifies
the borrowed lock, frozen command and prepared mutation; reloads authoritative
player/bank images; checks exact before state; regenerates the flat policy; and
stages exactly the resulting native images. Its preservation check (`:1025`)
restores currency fields and reproduces the original canonical bytes, protecting
unrelated fields and legacy receipts. It does not commit. The typed owner adds
the result, plan, evidence and source claim to the same operations and commits
one authority publication (`flatfile_accounting_bank_transaction.c:398`). It
then checks retained receipt, domain bytes and current balances. Failure after
publication begins is ambiguous; retry uses the same ID and recovers the journal.

The legacy `apply_currency_command`
(`flatfile_player_domain_repository.c:1816`) remains materially different. It
loads authoritative player/bank state before its embedded receipt lookup,
enforces active-mode legacy-write fences and its 512-operation capacity, and
still performs inline arithmetic rather than calling `currency_prepare_mutation`.
It preserves wallet-then-bank checking, wildcard revision behavior, both revision
increments, retained rejected results, and the existing two-image journal for
success. The typed path's receipt-first ordering and authority-evidence model
must not be silently substituted for this compatibility route.

## Native publication, ACK and persistence boundaries

The server's game-thread completion dispatcher calls the currency owner and
locker service (`src/net/comm.c:293`); journal replay restoration is separately
wired (`:362`). Login/account readiness invokes retained publication retry rather
than issuing a new economic command. Worker repository code does not mutate a
live `P_char`.

`currency_transaction_handle_completions` (`currency_transaction.c:1649`) stages
the sealed completion and retries ready original owners. `publish` (`:859`)
requires a valid disposition and an exact bounded result for committed work.
Ambiguous commits, exhausted repository retries, changed/malformed receipts and
invalid committed balances retain the original continuation; they are not sent
to the producer as an ordinary failed purchase. Known terminal rejection can
notify failure under its existing result rules. Offline ordinary players remain
waiting until the correct body returns.

`currency_transaction_publish_wallet` (`:1089`) checks all native count bounds
and suppresses older revision overwrite. Bank publication (`:136`,
`src/core/utility.c:3240`) updates the retained actor and matching account/racewar
descriptors, including morph originals, with revision monotonicity. Wallet and
bank publication are distinct effects even when the command changed only one.
Native balances are projections of the durable result, not the next write's
unconditionally authoritative opening state.

For schema 2, publication ACK precedes extraction of the pending owner and the
producer callback (`currency_transaction.c:859`). A failed ACK retains the
operation/context for retry, without repeating the debit. Generic coordinator
ACK (`critical_command_coordinator.c:3126`) checks the original pending owner,
refuses specialized owner/save-hold cases, and checkpoints the original journal
before removing fences and pending state (`:3176`). A journal checkpoint is not
itself proof that native publication occurred; the caller must satisfy its
publication contract first.

Ordinary persistence must not become a second currency writer. SQL snapshot
application skips cash (`src/player/player_snapshot_repository.c:260`);
`sql_save_player` updates existing cash columns to themselves
(`src/sql/sql_player.c:1617`) and establishes the wallet opening baseline for a
new player (`:1801`). SQL load captures wallet/bank revisions and values through
`src/player/player_load_repository.c:363`, `:446`, `:628`; materialization
publishes native cash/revisions (`src/player/player_load_materialize.c:558`).
Flat snapshots establish an initial domain only (`src/flatfile/flatfile_player_repository.c:806`),
preserving an existing shared bank; authoritative domain load overlays balances
for load (`:1004`). Moving to a domain facade must retain these opening/load
exceptions without allowing later bulk snapshots to overwrite durable money.

## Physical coin pickup and drop: one monetary leg inside a compound owner

The native `do_get` (`src/cmd/actobj.c:2593`) reaches `submit_coin_get` (`:4479`)
through the real pickup paths (`:1165`, `:1532`). Active ordinary-room pickup
requires the supported room shape; container/source-move paths are not silently
admitted. An untracked pile first uses the existing absent-item admission flow
(`:4497`), whose completion resolves the object again and reattempts pickup. That
enrollment is a separate operation, not an atomic part of the later coin root.

For an enrolled pile, native capture freezes wallet counts, eligible whole-coin
pickup/carry limit, original pile UID/literal/custody/revisions and remainder.
`prepare_coin_pile` and `currency_transaction_coin_wallet` construct the pile
and wallet endpoints of the same root (`actobj.c:4596`;
`currency_transaction.c:1256`, using `coin_wallet_delta` at `:1223`). Active pickup submits verified physical publisher,
notification and release callbacks (`actobj.c:4602`). Numeric wallet preparation
cannot substitute a new pile identity or claim the item leg.

`do_drop` (`actobj.c:5063`) parses and checks native denomination counts before
`submit_coin_debit` (`:4903`). Active ordinary-room drop goes through
`submit_accounted_coin_drop` (`:4809`): it creates a detached money object,
captures its actual literal/UID and original room, combines wallet debit with
system-to-room pile creation, and submits the one typed coin root (`:4833`).
Failed admission releases the detached object (`:4842`). Active give has its own
supported PC route; it is not permission for arbitrary containers or NPC cash.

Inactive/legacy drop still has a different composite path: wallet spend then
`coin_debit_completion` / `publish_coin_drop` (`:4725`) materializes the pile;
failure can require a separate refund. NPC/local paths retain direct native
money changes. This compatibility behavior must not be described as the atomic
schema-2 drop guarantee.

`currency_transaction_submit_coin` (`currency_transaction.c:1274`) requires
nonrebasable admission, bounds context, freezes one original root, checks all
endpoint fences/live wallet actors and pending state, and requires a verified
publisher for schema-2 physical endpoints before accepting. The pending entry
exists before coordinator submission. Busy UID checks and coordinator fences
are present; this layer does not establish a generic native save-hold capability.

SQL coin execution stays in the existing root/session
(`critical_command_repository.c:2170`). A `coin_endpoints` savepoint (`:2190`)
covers ordered wallet/item endpoints, child inbox/ledger/outbox and shared-bank
revision rebasing. Wallet state is locked and checked; the item owner proves
literal/custody/before state (`:2265`). Business failure rolls back all endpoint
effects (`:2310`), then retains the parent rejection and any authenticated stale
repair witness. Success records coin accounting, child references, pile head and
source effect (`:2343`), verifies the root, and COMMITs once (`:2402`). Child
operation IDs do not create independently committed wallet and pile payments.

Flat coin execution (`src/flatfile/flatfile_accounting_coin_transaction.c:754`)
uses identity-before-authority locking and retained lookup before fresh work.
It prepares wallet and pile mutations under the same locked authority, clears
staged images on rejection, and publishes native images, result, plan,
accounting references, source claim and pile head in one commit (`:956`). A
zero bank delta can still advance the bank revision; two wallets sharing a bank
need the existing ordered second revision, not two stale independent openings.

Live schema-2 `publish_coin` (`currency_transaction.c:559`) publishes the monetary
result before `publish_accounted_coin` (`:446`) requires the producer's exact
physical projection, verifies the same body/receipt, then ACKs. Only afterward does it extract the pending
owner, release staging and notify. A false/throwing publisher retains the same
obligation without a lifetime retry cap. A post-ACK notification failure cannot
repeat monetary or physical effects. The legacy eight-attempt cleanup policy
does not apply to schema-2 obligations (`currency_transaction.h:51`).

The actual room publisher (`src/economy/coin_physical_publication.c:206`) resolves
the UID freshly and checks original wallet/result, pile literal, owner, revision
and runtime custody. It guards projection reentry, materializes the original
drop identity when legitimately absent, applies partial remainder or extraction,
and verifies the resulting unique object/literal (`:309`, `:343`, `:362`). A
domain extraction cannot replace it with an unverified callback or allocate a
new UID on retry.

## Cold recovery: historical receipt plus current authority plus save owner

`currency_transaction_restore_replayed_command` (`currency_transaction.c:1695`)
restores only the recognized schema-2 publication-required obligations. Its
ordinary account-bank recovery accepts ATM deposit/withdraw; that restriction
must not be generalized to every bank reward or payment. Ordinary single-room
coin recovery validates the original envelope/shape and installs the existing
SQL coin save obligation (`src/player/player_save_pipeline.c:2621`). Missing
producer recovery support remains missing; replay never fabricates a callback.

The sealed receipt is a necessary historical input, not current authority.
`coin_physical_recovery_publish` (`src/economy/coin_physical_recovery.c:1260`)
authenticates that receipt and original command and obtains a fresh current
wallet/bank/pile cut. SQL uses its borrowed original session, transaction and
native proofs and confirms rollback/idle before returning success (`:1331`,
`:1351`); flat takes the identity/authority locks (`:1363`). Current wallet/pile
identity, cash/revisions and exact literal/custody must be proven. A newer bank
revision can be covered by current authoritative bank state under the existing
coverage rule (`:448`). Actual cold-loadable room items are checked (`:1212`);
the coin payload is not a replacement for the room/item catalog. Retry acquires
a fresh proof cut rather than reusing a previous observation.

`player_save_restored_publication_owner::publish` (`player_save_pipeline.c:3183`) is the
save owner's handoff: exact held slot/generation/original command, pipeline
health, terminal/login/append/snapshot fences, reservation epoch, worker and
revision acknowledgements, journal/census capture and current receipt all remain
checked. It invokes the primary physical recovery and rechecks the census before
typed ACK (`:3280`). Hold consumption (`:3316`) authenticates the acknowledged
slot and frozen bytes, clears its checkpoint and resumes only the exact deferred
save. The currency layer's `restored_coin_receipt_current` observation
(`currency_transaction.c:1811`) is not an ACK capability. This compound save,
publication and recovery responsibility remains with its current owner.

## Smallest future currency-domain sequence

This is a review proposal, not a reservation to edit code. Reuse the command,
state, result, private prepared mutation and existing adapters above. Keep one
native currency owner for native identity/cash/revision capture, admission,
pending continuations and publication; keep one durable currency responsibility
inside each existing SQL/flat root. Physical coin ownership stays with the item
and save owners, with the currency domain supplying only its existing monetary
endpoint. No generic domain registry, replacement codec, new ledger or parallel
writer is needed.

1. **Reconcile existing preparation before another extraction.** Compare the
   exact R2 patch with the selected maintained/candidate source, including callers,
   reason gates, bank-change order, error order and prepared locker receipts.
   Review adoption or conflict against the actual selected primary publication.
   Reuse or deliberately defer R2; do not add a second numeric helper beside it.
2. **Write acceptance fixtures and ownership checks first.** Reserve a currency
   fixture blueprint against the pins here. Specify observable admission,
   durable result, live publication, ACK and reload outcomes for the cases below.
   Existing tests can be extended after a reviewed code/test reservation and the
   required native fixtures exist. A blueprint itself needs no database.
3. **Introduce the smallest replaceable boundary with durable storage still
   authoritative.** If reviewed and reserved, consolidate only existing owned
   preparation behind the existing command/state/result interfaces and connect
   actual native producers and existing SQL/flat transaction owners. Remove the
   replaced helper in that adoption. Preserve legacy compatibility branches where
   they are still required, with explicit tests for their differing semantics.
   SQL locked rows or flat locked canonical images still decide current balances,
   mapping/lifetime, revisions and durable acceptance. RAM is the captured view
   and published projection. Ordinary snapshots remain unable to write money.
4. **Qualify that intermediate state before discussing RAM authority.** Run the
   original focused tests with the exact integrated source/providers, genuine
   native ATM/payment/coin journeys, shared-account/morph/login, fault injection,
   original-ID replay, save/copyover/restart and current-proof refusal for both
   backends. Publish source, inputs, schema/runtime fingerprints and actual
   observations. A successful component test or historical audit is insufficient.
5. **Make a later RAM-authority decision only with its prerequisites.** Primary
   must define complete initialized world/cash/pile capture, authenticated active
   mapping/lifetime ownership, exclusive write admission, reconciliation and
   persistence/recovery across crash and copyover, including NPC/reset/collector
   and external writer coverage. The switch requires an explicit reviewed
   authority cut and proof that no storage writer or stale save can compete.
   None is supplied by this document. Until then the intermediate database /
   flat-domain authority remains the implemented contract.

There is no useful additional production extraction reserved here. The remaining
currency seam crosses identity, transaction, retained receipt and publication
owners. The most useful available step is specifying their observable contract
against real fixtures, then comparing a selected primary publication to it.

## Existing tests and the exact claim each can support

These are **source-inspected assertions/scenarios, not results from this task**.
Wrappers, doubles and synthetic setup remain part of each test's scope. Existing
passing evidence is not silently transplanted to this frozen tree or a private
candidate. Test paths are under `tests/async/` and individually pinned below.

| Test / concrete inspected evidence | Invariant it exercises | Limit / remaining native case |
| --- | --- | --- |
| `test_currency_transaction_contract.py:49`, `:77`, `:111`, `:209` | Source checks require both-state/ledger/outbox commit, one-command deposit-all and completion wiring, snapshot non-overwrite, game-thread dispatch. | Token/source contracts do not execute those behaviors. Need real registered ATM command, location/parser and persistence journey. |
| `test_economic_currency_adapter.py`; `economic_currency_adapter_test.cpp:60`, `:258`, `:376` | Actual pure preparation/agreement, revision policies, randomized denomination changes, bank-only starter supply and existing typed plan. | Supplied authority is a fixture; no live identity, lock, SQL or physical publication proof. |
| `test_currency_completion_retention.py:415`, `:590`, `:891` | Harness scenarios cover prepared wallet/bank payment and pending payment, typed bank ACK retry/invalid result/restart, terminal failures, callback chains and reentry, malformed/ambiguous/offline/newer-revision results. | Real currency owner with doubles does not prove native service delivery or a real repository COMMIT/recovery. Preserve original full scenario manifest. |
| `run_economic_sql_bank_transaction_mysql.py`; `economic_sql_bank_transaction_mysql_harness.cpp:904`, `:961`, `:1113`, `:1181` | Actual typed SQL root rejects tamper/rollback-marker misuse, verifies retained history after retirement, refuses changed command/evidence and checks real-pool acquisition/replacement accounting. | Requires guarded disposable loopback schema, real pool and exact provider link. Not a real server command/location/world journey. |
| `test_flatfile_accounting_bank.py`; `flatfile_accounting_bank_test.cpp:842`, `:880`, `:955`, `:985`, `:1086` | Allocation/ambiguity retry preserves original ID, five forged receipt classes refuse, legacy capacity remains distinct, shared-account contention gives one commit/one rejection with conservation, restart waits for publication ACK. | Native domain/codec components use synthetic lifecycle and characters. Need full initialized world and native publication/reload. |
| `test_pa_atm_publication_sql.py`; `pa_atm_publication_harness.cpp:325`, `:405`, `:432`, `:454` | Exactly one inbox/ledger/outbox/accounting root; no native change before completion; replay of original payload; offline retention; return publishes wallet/shared bank then one checkpoint and clears fences. | Uses identified ATM vector, lifecycle bootstrap seam and descriptor publisher double. Does not invoke real `do_deposit`/`do_withdraw`, parser, ATM presence or network login. |
| `test_morph_bank_publication.py:88`, `:96`, `:103` | Extracted real publisher updates direct/original bodies, preserves morph bank, ignores old revision and publishes newer revision/GMCP. | Native-shaped fixture, no durable transaction. Need real morph/account/login around a pending bank operation. |
| `test_new_player_bank_hydration.py:122`, `:143`, `:185`, `:194` | Actual flat hydration preserves opening wallet and existing shared/opposite-racewar bank; invalid identity/item authority refuses without changing wallet. | Synthetic character/domain fixture; need whole first-session save/load and concurrent account-bank history. |
| `test_locker_receipt_recovery.py:40`; `locker_receipt_harness.cpp`; `test_locker_identify.py:59` | Real service/receipt process boundaries before payment, after payment and after receipt; actual lore capture matches normal rendering for five item types. | Service injection/payment fixture and optional disposable SQL setup are not full active payment admission plus native item/service journey. Active unsupported payment must remain a refusal. |
| `test_coin_transfer_shared_bank_accounting.py`; `coin_transfer_shared_bank_accounting_test.cpp:71`, `:85`, `:107`, `:116` | Requires the second shared-bank increment; rejects wrong destination revision, mismatched original fences and overflow; distinct banks retain original fencing. | Plan validation alone does not execute two native PCs or their publications. |
| `test_flatfile_accounting_coin.py`; `flatfile_accounting_coin_test.cpp:160`, `:358`, `:921`, `:1383`, `:1583`, `:1637` | Native domain/pile fixtures exercise ordered shared-bank children, preexisting pile pickup, cold reader, retained replay, ambiguous interrupted publication resolved by same-ID replay, drop/split/merge/pickup and denomination change. | Actual root/components, synthetic world/lifecycle. Not real gameplay or the integrated native save-owner recovery. |
| `test_coin_publication_ack_retention.py` | Named scenarios include missing publisher/admission, absent actor, partial projection retry/conflict, changed receipt after physical/wallet projection, ACK retry, throwing publisher/notification and exhaustion. | Real currency/coordinator with physical-publisher doubles; must also prove actual unique object/custody and save hold. |
| `test_coin_physical_publication.py` | Actual physical source scenarios include fresh body, invalid wallet/revisions/bank range/pile count/root/owner, projection reentry and identity changes. | Fixture world; need real registered pickup/drop, actual room catalog and both-backend restart. |
| `test_coin_recovery_owner_revisions.py` | Scenarios include stale counters, conflicting owner/literal, partial pickup, consumed cold cache, uncertain effect and lost authority. | Selected actual recovery owner functions with deliberately unreachable SQL boundary. Does not qualify the real SQL current-proof session or integrated cold ACK. |

## Missing acceptance work and required inputs

| Required observation | Concrete fixture / input still needed | Owner prerequisite and completion evidence |
| --- | --- | --- |
| Real wallet reward and spend outcome across admission failure, explicit rejection, lost COMMIT and offline completion | Exact admitted producer/reason and real character; before/after durable vectors/revisions, result, pending state, live output and reload. Include unsupported active producer refusal. | Currency producer + primary integrated candidate. Original-ID replay must cause at most one mutation; neither admission nor timeout may report final paid delivery. |
| Actual ATM all/explicit denomination deposit and withdraw | Real registered command/parser and ATM room; invalid/empty/insufficient/overflow cases, account sibling, morph original, opposite racewar; source schema/runtime pins. | Native command owner plus both storage engines. Observe one root for deposit-all, exact vector/revisions, final output, shared publication and post-restart persistence. |
| Bank-funded payment/change and prepared locker recovery | Costs covered by wallet, only bank, neither; denomination rounding/change; exact persisted prepared command/text; crashes before payment, after commit, before receipt/display/ACK. | Locker/service owner and primary admitted payment capability. No duplicate debit; no success before final result; replay delivers original receipt/text. Unsupported active paths still refuse before service delivery. |
| Simultaneous shared-account commands | Two real PCs on one account/racewar, live morph/original and queued command; controlled lock/admission overlap and disconnect/reconnect. | Native admission + SQL/flat owner. Current shared bank revisions determine winner; pending publication blocks stale next input; no lost or duplicate bank update. |
| Real room drop and partial/full pickup | Enrolled and initially untracked piles; actual UID/origin/literal/room catalog; carry limit, remainder, stale owner, conflicting literal/UID and extraction. | Item/coin owner + currency endpoint. Both legs commit or reject together after enrollment; current physical projection occurs once, before ACK; retry preserves root and UID. |
| Crash after durable coin commit but before physical publication or ACK | Original publication-required journal and sealed receipt, actual fresh backend current cut, cold room items, authentic held save slot/generation and deferred save. | Primary save/recovery owner. Current wallet/pile and permitted newer bank coverage authenticate; reentry/conflict/changed census refuses and retains hold; exact typed ACK consumes it only after proof. |
| Save/load, copyover and recovery cannot overwrite money | Genuine ordinary snapshots before/after currency result, first-player baseline, account-bank hydration, initialized live world and complete producer set. | Primary persistence/Plans 2 and 5 + storage owners. Reload agrees with durable result; stale snapshot cannot roll cash/revisions back; recovery retains unresolved original obligations. |
| Error ordering and compatibility after any extraction | Original backend policy/error precedence, legacy capacity/replay/fences, exact reason/source gates and complete existing scenarios. | Reviewed R2 adoption comparison and focused existing tests; changes in status/error/result bytes must be deliberate and separately reviewed. |

Execution needs primary's exact integrated publication, source/provider manifest,
genuine schema/runtime fingerprints, disposable backend roots, initialized world
and the real native fixture/owner handoffs. Staff account credentials remain in
the authorized local environment and are not copied here. Absence of these
inputs does not prevent source-grounded fixture design, but it does prevent
calling that design executed gameplay or current-authority proof.

## Private interfaces and collision boundaries

The maintained [review checkpoint](../EXPERIMENTAL_REVIEW_CHECKPOINT.md) at the
frozen commit reports private Plan 2/cache/NPC/initialized-world/reset work. These
are **reported contracts, not imported implementation or executed evidence**.
The private NPC successor reports complete raw/native comparison, distinct
mapping ID/native UID, all four current cash denominations and revision, original
birth authentication before native locks, and an owned authenticated lifetime
vector retained through the synchronous verifier callback (`:17`, `:28`). A
currency facade must consume the eventual authenticated result at its existing
owner boundary; it must not reimplement mapping verification, reduce raw evidence
or infer authority from a cached decoded view.

The same checkpoint's reset and cache reports (`:90`, `:114`) and the maintained
[persisted-provider union handoff](https://github.com/Community-Duris/Duris/blob/a6aec4c4a058a5174e5b679708103762fa05e05a/docs/persistence/economy_accounting/PERSISTED_PROVIDER_UNION_PRIMARY_HANDOFF_2026-10-07.md)
and [reset/Plan 5 interface](https://github.com/Community-Duris/Duris/blob/a6aec4c4a058a5174e5b679708103762fa05e05a/docs/persistence/economy_accounting/ZONE_RESET_ORIGIN_PLAN5_INTERFACE_2026-10-07.md)
retain actorless money, physical literal/custody, room current effects,
constructor/source lineage, original retained-session/census and cleanup
responsibilities. Fresh-cache or owned RR-cut preparation is not itself an
initialized-world acceptance result. Currency arithmetic neither issues an NPC
birth nor validates collector/reset/auction custody. Their current-clock, raw
census, publication and ACK interfaces must be reconciled when primary actually
publishes the code.

Primary still owns normalizer/verifier/producer/audit/backup and Plan 5 execution;
this document reserves none of those paths. The quest preparation owner owns
QP02/QP03 native journey blueprints and all quest-preparation paths. Currency
acceptance preparation may refer to that owner's eventual output without
duplicating or editing it. Closed operator repair/evidence, prior R0/R1/R2
handoffs and all active worktrees/jobs remain unchanged.

## Prioritized next actions for coordinator review

No action below starts automatically. Each new document, fixture/test edit,
adoption or extraction needs its own reviewed reservation and exact file scope.

| Priority / next action | Concrete output and available inputs | Missing prerequisite / collision risk |
| --- | --- | --- |
| 1 — Next preparation task: currency native acceptance blueprint | One reserved currency-only scenario/observation matrix derived from the real ATM, shared-bank, prepared payment and ordinary-room coin paths above; expected refusal/result/publication/ACK/reload records, fixture construction and fault cuts. All maintained source and test bodies are available now. | Native execution can wait for primary's integrated publication and real fixtures. Coordinate SQL/flat/save/item owners; exclude quest QP02/QP03 and all quest-prep paths, primary verifier/producer and closed operator packets. |
| 2 — Dependency reduction: R2 applicability review | Exact patch/caller/error-policy comparison with an explicit adopt/conflict/defer disposition, using owned `24fa551ae`, the maintained pin and actual selected primary candidate when available. | Do not assume private patch contents or adoption. A document-only comparison is available; any code import needs primary publication, reviewed reservation and proof requirements. Avoid duplicate numeric helpers. |
| 3 — Source-grounded shared-bank and payment review | Enumerate existing wrapper callers whose success means accepted submission, identify actual irreversible delivery continuations, and propose the smallest owner-specific receipt/ACK repair only where source proves one is needed. Existing utility, locker and producer sources are available. | No generic payment framework. Review/admit the particular producer with its owner before code; private source/reason gates and real service fixtures may be missing. Do not take quest, collector or reset producer ownership. |
| 4 — Code only after publication: adopt the selected existing preparation | A narrowly reserved patch connecting existing native capture to owned preparation and existing durable writers, removing the replaced helper; proportionate original tests and native journeys with immutable proof pins. | Requires selected integrated primary source, resolved authority/compatibility review, provider/build/schema/native inputs. High collision risk in `currency_transaction.c`, repository roots and save/publication owners; no concurrent speculative edits. |
| 5 — Authority redesign only after integrated qualification | Explicit owner-reviewed RAM authority cut, full writer inventory and current-world/recovery acceptance evidence. | Depends on complete Plans 2/5 and all native money/item producers plus crash/copyover persistence. Not an idle-time implementation task and not reserved here. |

## Document validation

Validation for this handoff authenticates the frozen source files against Git
blobs, verifies cited line anchors exist, checks local document links and confirms
that only this document changed. `git diff --check` is the whitespace check. This
establishes traceable preparation, not successful behavior. The maintained pin
and owned preparation pin remain separate; later adoption must repeat the
applicability comparison against its own exact source.

## Exact Git blob pins

The following appendix is generated from the two frozen Git trees and checked
against the inspected source bodies. The revision column identifies which tree
supplies each file. All maintained entries use the full `a6aec4c4a058a5174e5b679708103762fa05e05a`
pin; owned entries use full `c84786260d3eb7e2ea041f461bbb5450c91542ec`.

<!-- BLOB_PINS -->

| Revision | File | Exact Git blob |
| --- | --- | --- |
| maintained | `docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md` | `ee138c7b9edf73769a1a321b8d73234916a81498` |
| maintained | `docs/persistence/economy_accounting/PERSISTED_PROVIDER_UNION_PRIMARY_HANDOFF_2026-10-07.md` | `0f9db291694dcdfc7ed9c803c9ad72f79e65e548` |
| maintained | `docs/persistence/economy_accounting/ZONE_RESET_ORIGIN_PLAN5_INTERFACE_2026-10-07.md` | `ec633a3d78205bea6275ac5b4b51fd1c9ae34cbf` |
| owned | `docs/persistence/economy_accounting/domain-separation/OPERATION_INVENTORY.md` | `e1761f0dde821d281a9fd24618d932234d192f90` |
| owned | `docs/persistence/economy_accounting/domain-separation/R2_RESERVATION.md` | `57ffc720ee8ade963017284ece3c0709daa04f82` |
| maintained | `src/cmd/actobj.c` | `a2114fddb6816f1534488ff11457a1001d47a14c` |
| maintained | `src/cmd/actoth.c` | `53554534c4c411cbcddcdac7844f1456657d9cd3` |
| maintained | `src/cmd/interp.c` | `74281b6f20ccd88a588d63cdda143da275258e85` |
| maintained | `src/combat/chaos.c` | `b9eb112486f42ca2df4d9a338a77c5ecb7e757b0` |
| maintained | `src/core/utility.c` | `92bdffd57f8979d9531251d1b049887505d7715d` |
| maintained | `src/economy/auction_houses.c` | `f032b614e58e3e43caeac255be3c6d3f8c6d35f7` |
| maintained | `src/economy/coin_physical_publication.c` | `932a7631a5be6163c3074bbada3d774014eb8d3d` |
| maintained | `src/economy/coin_physical_recovery.c` | `19027a42cfc993deeeb7b2b6c08e0f83b43337e1` |
| maintained | `src/economy/currency_command.c` | `29ba7ccbadf7d8a19a3a009b02c514fea9a11a14` |
| maintained | `src/economy/currency_command.h` | `6797b440784b060d8ccb40826172fb12aa8967a6` |
| maintained | `src/economy/currency_publication.h` | `9075511956a83dfb59b52741c32ebe3d26e1dbdd` |
| maintained | `src/economy/currency_repository.h` | `e1e0607926edf2aa10a0c59867b39e3de1f4a130` |
| maintained | `src/economy/currency_sql_mutation_writer.h` | `bdeabef3686f9f12fbc0a57d9ba6144761597bc1` |
| maintained | `src/economy/currency_transaction.c` | `7366935cbf829554395f739f7af7b32ba303558f` |
| maintained | `src/economy/currency_transaction.h` | `d5918c572d086e0f56094f7bb4aa5790770dffcf` |
| maintained | `src/economy/economic_currency_adapter.c` | `00ba35d6a6b65a2941796609f2e5f911f951b9b0` |
| maintained | `src/economy/economic_currency_adapter.h` | `e720658ce7f880aa5ed40b6b8ed80bce443a3e84` |
| maintained | `src/economy/economic_gameplay_authority.c` | `4afa41bdb718068e4862f422b701fa5ad6953d56` |
| maintained | `src/economy/shop.c` | `0e63a8b3a72c705bce5030caf6b828b57c8ccae5` |
| maintained | `src/flatfile/currency_flatfile_mutation_writer.h` | `5e4c86de282588f2eb065f8e4e2f6a5b602a5e46` |
| maintained | `src/flatfile/flatfile_accounting_bank_transaction.c` | `278f5a59a5fd236783b8c228e0370a3be3af904f` |
| maintained | `src/flatfile/flatfile_accounting_coin_transaction.c` | `0247628992ebe091221c3dad8401b19422cba70d` |
| maintained | `src/flatfile/flatfile_accounting_dispatch.c` | `323386ecfb4739996ba40158abfa524522a0ee38` |
| maintained | `src/flatfile/flatfile_player_domain_repository.c` | `8583e9969963e7bd5bff8802879eefe37f728e09` |
| maintained | `src/flatfile/flatfile_player_repository.c` | `83c3690d486493b98b0e0da2e39f514fdff8422a` |
| maintained | `src/item/locker_identify.c` | `247988fda718fb2c06315f7375ca335bc26d371a` |
| maintained | `src/net/comm.c` | `8bcbc9ab95073b0d5fa71dc3f3516db183982eb1` |
| maintained | `src/persistence/critical_command_coordinator.c` | `43a5b4f15c9171703e77b89b84bbc0c6f3a49b16` |
| maintained | `src/persistence/critical_command_repository.c` | `1f69092f0b3070d665952ec8c8638c1ec5d09b00` |
| maintained | `src/persistence/economic_sql_bank_transaction.c` | `0aee72d5c5473834509ee44f6bcc5ea3f78b9658` |
| maintained | `src/player/player_load_materialize.c` | `2ec4341c3b6ab4e768f7435958d2884f57306ffc` |
| maintained | `src/player/player_load_repository.c` | `9763bfabcd8dc92af814ed894d3e1d63429e3e5b` |
| maintained | `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| maintained | `src/player/player_snapshot_repository.c` | `21c7108c041613b2b76d1b305b1fe5d842bb360d` |
| maintained | `src/sql/sql_player.c` | `57a536a3a192b5432b17fd7ac088b76f7559b41e` |
| maintained | `src/world/epic.c` | `9a51d8702e1c1a10b0e4e15a9bd6ce07bc41c0aa` |
| maintained | `src/world/quest.c` | `4ee99c6c1020250395cba9bdfa3e47f42a642cfe` |
| maintained | `tests/async/coin_transfer_shared_bank_accounting_test.cpp` | `61cc02b24235cc01ba34818ecaaa58ffd4cfa98e` |
| maintained | `tests/async/economic_currency_adapter_test.cpp` | `88c66a7490a954c90a6ddbaf94a161fc44e40df2` |
| maintained | `tests/async/economic_sql_bank_transaction_mysql_harness.cpp` | `df57e85e386c2aded5a057645d071818b60bf60b` |
| maintained | `tests/async/flatfile_accounting_bank_test.cpp` | `d0b1584def10500029e1239b00fc483ad3c1a861` |
| maintained | `tests/async/flatfile_accounting_coin_test.cpp` | `24351b6f2504ee3a3c4c2510ec49171941fee1f9` |
| maintained | `tests/async/locker_receipt_harness.cpp` | `0992e98c6e4ce9551b5f8e84a0bc6fd10d098c74` |
| maintained | `tests/async/pa_atm_publication_harness.cpp` | `553d20ca939e431fd9c6bf41e06bdc02a0e15357` |
| maintained | `tests/async/run_economic_sql_bank_transaction_mysql.py` | `a1e35649ffacd06bd0c21216c7cb3f157b3d5f02` |
| maintained | `tests/async/test_coin_physical_publication.py` | `ecc201cefe9cf727cb23838ade73a4953b3fc854` |
| maintained | `tests/async/test_coin_publication_ack_retention.py` | `d2aa15e20e8a8362d73a83b357998892f1bd2c2c` |
| maintained | `tests/async/test_coin_recovery_owner_revisions.py` | `b15b21c96a5e9fb2489012c6c7e39e15c762dfb6` |
| maintained | `tests/async/test_coin_transfer_shared_bank_accounting.py` | `cd8dd2ee1e8540e88fcf9729034deab9bce0f8e4` |
| maintained | `tests/async/test_currency_completion_retention.py` | `3b92ac83a65ccb60841f1c55287bfa79603de127` |
| maintained | `tests/async/test_currency_transaction_contract.py` | `93b7173a7541646506c232a297809133eaf2b209` |
| maintained | `tests/async/test_economic_currency_adapter.py` | `3a19a9ff0ee011edca410134d8e4ab57a5c55ad7` |
| maintained | `tests/async/test_flatfile_accounting_bank.py` | `b930100cf123f612a1813b6eb3354c3b02b215de` |
| maintained | `tests/async/test_flatfile_accounting_coin.py` | `e1417a83b2d263b60854f83ede8d57904f626c9f` |
| maintained | `tests/async/test_locker_identify.py` | `7ca5b21ebf07d1fa9bc8085f5dbf42386f95444f` |
| maintained | `tests/async/test_locker_receipt_recovery.py` | `90d04361cfb6b4403b2a01c080b272d9a67deb57` |
| maintained | `tests/async/test_morph_bank_publication.py` | `6c338c7ae51e304bc82b74b512afcb056a5ab2b1` |
| maintained | `tests/async/test_new_player_bank_hydration.py` | `e0872a79aa1115144e7e39baaf58a291b91c777b` |
| maintained | `tests/async/test_pa_atm_publication_sql.py` | `86be17e3630ae1eaf411cc97a5eced621e97fdd8` |
