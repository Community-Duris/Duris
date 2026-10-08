# SHOP trade domain and authority boundary — 2026-10-08

Disposition: **source and documentation map only**. The remaining conversion is
the existing buy/sell producer's retained compound operation and native authority
boundary. R10/R12/R13/R14 already close the selected quotation and policy work.
This map adds the current ownership cut, especially keeper contention and cleanup;
it does not reserve another calculation extraction, codec closure or route repair.
Compiler, native, SQL, gameplay, persistence and recovery execution for this map
are **UNEXECUTED**. The continuing native Goal remains **BLOCKED and unfinished**.

## Frozen inputs and comparison disposition

All source anchors below refer to public primary
`fc8a8961b281c4b53bc0f7af1459dbb131781676`, not this worktree's optional code.
Its `src` tree is `833d3085815b396861ad18a77635412212381e4b`, `tests/async`
is `790f367adf805a69d53aac6460938f5c921f9136`, `migrations` is
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, and `docs/persistence` is
`fa108c6cff9aa67e01212ab3f16cff0da353636b`. The source, tests and migration
trees equal the preceding recipe-map input `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a`.
The fc8 commit changes six documentation files only.

The authenticated export of those four scopes contains 3,455 files. Its 56,320,000
byte Git archive has SHA256
`c537b79f0e39a3dd76573f469b08469f64909b9519035af023afdb2b70311cd4`.
Raw body sizes, SHA256s and Git blobs, anchor contexts, link targets and static
ordering checks are retained under
`D:\Dev\Temp\shop-trade-authority-comparison-20261008`. These checks establish
the inspected inputs and design, not executable acceptance.

The existing findings cover the component contracts. They do not together give
a current conversion decision distinguishing process-local SHOP exclusion,
ordinary SQL compare-and-update, shared keeper birth/opening authority and the
closed central admission gate. That is the nonduplicate purpose of this map.

| Existing accepted finding | Reuse and boundary |
| --- | --- |
| [R10 purchase quote](https://github.com/Community-Duris/Duris/blob/fc8a8961b281c4b53bc0f7af1459dbb131781676/docs/persistence/economy_accounting/domain-separation/R10_SHOP_PURCHASE_QUOTE_REVIEW_2026-10-07.md), code `7bbf942e3b7c12dea22e5bf03a592b48aedd690f` | Whole ordered quote, native float/int behavior and barter comparison; no new price extraction. |
| [R12 sale quote](https://github.com/Community-Duris/Duris/blob/fc8a8961b281c4b53bc0f7af1459dbb131781676/docs/persistence/economy_accounting/domain-separation/R12_SHOP_SALE_QUOTE_REVIEW_2026-10-07.md), code `72f375cbfcc7b8b5c95b7784f245b210d0b163d4` | Sale/valuation base and trophy adjustment; sale does not acquire buy's barter roll. |
| [R13 item acceptance](https://github.com/Community-Duris/Duris/blob/fc8a8961b281c4b53bc0f7af1459dbb131781676/docs/persistence/economy_accounting/domain-separation/R13_SHOP_ITEM_ACCEPTANCE_REVIEW_2026-10-07.md), code `835e70e1d9c35d56c2254ca65cbc306c7e60fc81` | Complete classifier and real keyword parser; no new acceptance fragment. |
| [R14 customer access](https://github.com/Community-Duris/Duris/blob/fc8a8961b281c4b53bc0f7af1459dbb131781676/docs/persistence/economy_accounting/domain-separation/R14_SHOP_CUSTOMER_ACCESS_REVIEW_2026-10-07.md), code `652462d8afbc3837ff5dcd6538f918e366269c85` | Complete access decision and original feedback; controlled world endpoints grant no custody authority. |
| [Live-route review](https://github.com/Community-Duris/Duris/blob/fc8a8961b281c4b53bc0f7af1459dbb131781676/docs/persistence/economy_accounting/quest-prep/SHOP_LIVE_ROUTE_REVIEW_2026-10-07.md), code `fd4563fab791b33a2905db22baa4167f3567c298` | Closed 106-check source contract, 23 definitions and 27 mutation controls. Reuse its physical-before-completion finding; do not repair the old public checker again. |
| [Codec closure review](https://github.com/Community-Duris/Duris/blob/fc8a8961b281c4b53bc0f7af1459dbb131781676/docs/persistence/economy_accounting/quest-prep/SHOP_CODEC_CLOSURE_REVIEW_2026-10-07.md), code `0b0075724457511f6a7c6599489958cf45cd828a` | Closed provider/link closure of the unchanged 46-assert component. It supplies no transaction, world or native journey proof. |
| [SHOP_ROUTE_EVIDENCE](../SHOP_ROUTE_EVIDENCE.md) | Historical inactive SQL/flat compound-route evidence and typed accounting semantics. Its earlier producer-status statements are not a description of every later body. |
| [Retained producer integration](../SHOP_RETAINED_PRODUCER_DRIVER_INTEGRATION_2026-10-05.md), [recovery bindings](../SHOP_RECOVERY_BINDING_SOURCE_2026-10-05.md), [original cold publication](../SHOP_ORIGINAL_COLD_PUBLICATION_INTEGRATION_2026-10-05.md) | Original command, full forests, one-copy continuation, cold effects and guarded cleanup. Reuse those detailed findings instead of reproducing every SHOP report. |

Those optional code commits and their reviewed executions remain accepted at
their exact inputs. They are not silently adopted into the public source tree
above. In particular, current `tests/async/test_shop_trade_live_route.py:12`
still contains the old token expectations. This map neither executes nor changes
it; the separately closed repair and its preserved failures remain distinct.

## Producer decision and frozen outcome

`src/economy/shop.c:1879` (`shopping_buy`) and `:2316` (`shopping_sell`) are the
real command bodies. Their active-epoch gates at `:1881` and `:2319` run before
selection, quotation or candidate construction. Production availability at
`src/economy/shop_trade_transaction.c:3759` requires the game thread, active
regular SQL and the central predicate. That predicate actually returns `false`
at `src/economy/economic_command_admission.c:24`: SHOP has no maintained
schema-2 admission allowlist branch. Presence of a preparation API is not
permission to bypass this gate. Flat accounted preparation/publication also
refuses through its explicit `__NO_MYSQL__` branches.

| Decision before owner transfer | Actual body and retained result |
| --- | --- |
| Buy access, selected stock and quote | `shop.c:1902`, `:1927`, `:1979`: preserve native selection and access order, one original barter roll, later cost multiplier, truncation, epic/minimum adjustments and trust-adjusted charge. |
| Sell item and quote | `shop.c:2350`, `:2386`, `:2407`, `:2415`, `:2425`: preserve potion/artifact/encrust/container refusal, base quote, roaming cash restriction, NODROP, then trophy/minimum adjustment. |
| Existing/produced purchase | `shop.c:2077`: existing stock is the selected source; produced stock remains the exemplar and `read_object` creates this copy before preparation. Freeze that actual output, not a prototype recipe for rerolling. |
| Store/destroy sale | `shop.c:2440`: selected actor item plus current duplicate-stock/TRASH decision chooses `sell_store` or `sell_destroy`; later retries consume the chosen action. |
| Retained handoff | `shop.c:976` installs the PID sequence and calls `shop_trade_preparation_owner::start`; accounted branches return at `:2108` / `:2454`, before legacy money/SQL/item mutation. |
| Inactive flat handoff | `shop.c:2110` and `:2456` build a legacy payload and submit with the physical publisher and completion callback. This is the existing inactive route, not accounted parity. |

After successful transfer, retry the retained original. Do not rerun access,
quote/RNG, stock choice, sell outcome, UID allocation or `read_object` to repair
that operation. A later produced copy is a new operation only after the prior
copy finishes publication and guarded ACK. Initial `start` returning true means
ownership was retained, including an uncertain probe; it is not committed success.

## Original preparation, identities and current observations

`shop_trade_preparation_owner::begin` at
`src/economy/shop_trade_transaction.c:3045` validates actor/keeper/selected/stock/
destination, price/action, PID exclusion, keeper exclusion and capacity. It
captures the selected literal, stock and target, complete keeper literal forest,
account/race/epoch/mapping facts, original actor and keeper runtime IDs, then
installs an operation-ID/generation entry before a possibly uncertain shell probe
(`:3092`, `:3116`, `:3137`, `:3145`). Fenced keeper UIDs include unrelated keeper
stock, not only the purchased root.

`poll` at `:3321` requires those original runtime IDs, PID/account/race, configured
shop/VNUM/roaming, expected cash and literal values. A same-PID reconnect cannot
replace the original body for preparation. It polls and holds the original full
player checkpoint at `:3397`; inventory and original level/save revision remain
separate from the selected subtree. The private native checkpoint owner at
`src/sql/sql_player.c:9362` seals complete original BEFORE/AFTER facts before DML
(`:9425`), writes literal keeper payload, updates its checkpoint revision and
resolves a lost COMMIT only against those retained facts. A later read-only
ROLLBACK cleans that proof transaction; it does not prove the earlier COMMIT failed.

`build_accounted_command` at `shop_trade_transaction.c:3417` requires the sealed
native checkpoint and held player stage. It checks prospective complete player
and keeper AFTER values and original world BEFORE, freezes four required ordered
forest bindings plus the optional paired target bindings (`:3506`, `:3540`), and
builds the exact v8 command/intent at `:3573`. The manifest types in
`src/economy/shop_trade_recovery_manifest.h:34` bind role, presence, canonical
length/digest and ordered UIDs. Present-empty differs from absent. A digest or
UID-sorted SQL lookup is neither physical order nor historical custody authority.

The real runtime builder is `src/economy/shop_trade_runtime.c:265`, delegating to
the common capture at `:101`. It uses the supplied original save/level and native
keeper revision, literal selected tree and current wallet/bank revisions; it
refuses an already-adopted produced UID (`:180`). These observations are builder
inputs, not a new authority token. `src/economy/shop_trade_command.c:765` encodes
the recovery extension while preserving the existing native command identity.

`submit_accounted` at `shop_trade_transaction.c:3615` retains that command and
callbacks. `src/player/player_save_pipeline.c:2779` installs the execution hold,
moves the actual original player body beside the frozen command, and submits
outside the pipeline mutex at `:2799`. An exception returns `journal_uncertain`
with the exact slot retained. Definite prejournal refusal releases only the
matching generation; a committed native checkpoint is not undone as compensation.
`drive` at `shop_trade_transaction.c:3841` retries a submitted original directly;
it reconciles an uncertain native checkpoint before requiring live bodies (`:3876`).

Current values are observed later under the original publication transaction.
They do not replace the immutable command, completion, manifest or original hold.
Runtime pointers from a census are transient. Reobserve after each callback and
pulse; do not promote a VNUM, stale row ID or replacement keeper pointer to durable
identity. A matching current AFTER graph alone cannot prove handler tails returned.

## One compound economic root and backend participants

Keep wallet/bank revision handling, native keeper cash, selected item custody,
creation/destruction, complete literal payload and accounting evidence under one
operation. Do not split payment and delivery into independently acknowledged
commands. The inactive legacy SQL producer's older payment/grant callbacks are
not the accounted conversion seam.

| Owner | Concrete compound boundary and limit |
| --- | --- |
| Typed accounting | `src/economy/shop_trade_accounting.c:296` validates frozen intent, native balances/revisions and exact result, then makes one plan with monetary postings and item BEFORE/AFTER references (`:406`, `:485`). For v6–v8, virtual shared sink/issuance accounts replace per-keeper monetary lifetimes; native keeper cash/stock witnesses remain necessary. This pure plan does not execute SQL or grant ACK. |
| SQL locks and apply | `src/persistence/economic_sql_shop_trade_transaction.c:1339` obtains the original authority/native/owner/custody/physical cut; v8 includes complete manifest/current custody and absent future production. `:1457` rechecks context/session and locks, derives the plan, applies native trade/item events/owner revisions, verifies full AFTER, and inserts root/source claim/plan rows at `:1546`–`:1565`. It never commits independently. |
| SQL parent receipt/commit | `src/persistence/critical_command_repository.c:3329` calls lock/apply, writes result/outbox/inbox, verifies retained SHOP root and outbox and original session, then COMMITs at `:3382`. Errors return through parent rollback; lost COMMIT yields ambiguous outcome for original-ID reconciliation. Business rejection and failed AFTER verification have different dispositions. |
| SQL replay proof | `src/persistence/economic_sql_shop_trade_transaction.h:43` and `src/economy/shop_trade_transaction.c:995` authenticate the exact historical root, postings, references, source claim and inbox/result/outbox. Publication also obtains a separate locked current native image. Current balances/revisions alone cannot authenticate historical success. |
| Inactive flat root | `src/flatfile/flatfile_shop_trade_repository.c:273` recovers under the authority lock, verifies existing operation digest before the active legacy gate (`:297`, `:307`), prepares wallet/custody/shop/materialization, and stages result catalog plus all successful afterimages and item references in one authority commit at `:485`. This schema-1 route supplies no active schema-2 accounting or cold native parity. |

Native literal sidecars, full keeper save/load and checkpoint markers must stay
coherent with ordinary persistence, not just the selected trade. Reuse
[native admission](../SHOP_NATIVE_ADMISSION_HANDOFF_2026-10-04.md),
[full player proof](../SHOP_FULL_PLAYER_NATIVE_PROOF_2026-10-04.md) and
[coherent source integration](../SHOP_COHERENT_SOURCE_INTEGRATION_2026-10-05.md).
Their source milestones are not native qualification of this frozen tree.

## Physical publication, ACK and partial produced progress

`shop_trade_native_publication_owner::publish_retained` at
`src/economy/shop_trade_transaction.c:97` enters the real private pipeline owner
at `src/player/player_save_pipeline.c:3095`. It authenticates the frozen slot,
hold generation/reservation and completion, retires only genuinely covered
ordinary save frames, censuses retained frames before and after native publication,
then calls guarded coordinator ACK (`:3152`, `:3166`, `:3174`). It does not expose
an ID-only ACK capability.

The actual guarded coordinator overload at
`src/persistence/critical_command_coordinator.c:3677` checks original command,
completion, reservation and coordinator generation. Ordinary SHOP journal
checkpoint must succeed (`:3811`, `:3862`) before fence removal/operation retirement
at `:3895`; exact hold consumption follows outside the coordinator mutex at
`:3905`. No domain callback can substitute a scalar success flag for this owner.

The live native callback at `shop_trade_transaction.c:2366` locks the complete
current SQL image and verifies the exact historical receipt within that same
session (`:2401`). Original full BEFORE world proof and retained AFTER forests
precede effects. `shop.c:197` owns physical stages: audit, notice, detach, checked
placement, nesting/destruction and cash publication have explicit started/returned
state. False/throw or a started-without-returned tail retains the original;
topology equality cannot authorize blind repetition.

After effects, the native owner reobserves complete world values, projects current
wallet/bank/cash and custody caches, assigns only representable signed current
row IDs, and proves the final graph again (`shop_trade_transaction.c:2623`,
`:2643`, `:2688`). Successful original-session ROLLBACK and idle cleanup are
required at `:2718` before this callback can return proof to guarded ACK. This is
cleanup of the read/publication cut, not rollback of the already committed trade.
The legacy physical callback likewise runs before completion in `publish` at
`:529`; its result must be true before the map node is detached at `:566`.

Produced quantity is **sequential partial progress**, not one atomic batch.
`shop.c:83` retains original shop/keeper runtime/stock/destination/price and counts.
`shop.c:1154` advances completed/remaining only in successful completion after
physical publication; accounted completion additionally follows guarded ACK.
Then `shop.c:922` rechecks that keeper/stock/destination and stages the
next copy with a fresh UID and operation. Retries of the current copy keep its
original identity. Failure of a later copy reports the earlier completed count;
do not refund or roll back prior roots. Cold replay does not reconstruct this
volatile quantity sequence or purchase extra copies. Same-account reconnect may
receive its original cancellation notification (`shop_trade_transaction.c:3795`);
it does not rebind the
preparation or manufacture a continuation.

The inactive flat route at `shop_trade_transaction.c:2825` uses ordinary
`critical_command_coordinator_submit`, not the accounted private publication
reservation. Its retained domain physical-before-completion behavior must not be
reported as the accounted guarded-ACK/cold-owner proof.

## Keeper contention, cleanup and uncertainty

The process-local keeper fence is `shop_trade_transaction_keeper_busy` at
`shop_trade_transaction.c:4157`: any retained pending entry for the same shop blocks
another preparation, including a different PID and cold registration. `begin`
checks it at `:3074`; cold registration checks it at `:2770`. Item fences cover
the prepared complete keeper forest and all cold manifest UIDs at `:4163`.
Pending uncertainty must keep these fences. Successful publication removes the
old domain node before notification so a produced next copy can acquire its own
root; notification exceptions retain a blocked original at `:578`.

SQL additionally locks the actual keeper row selected by shop identity and checks
VNUM, nonnull cash/roaming, expected cash and shop revision
(`economic_sql_shop_trade_transaction.c:250`). Its mutation compares the original
revision and requires exactly one affected row at `:1256`. The native payload
checkpoint has its own compare-and-update at `sql_player.c:9434`. These are real
existing controls. Neither the in-process map nor these ordinary trade controls
prove the separately unfinished shared keeper birth/opening historical transition,
classification/current-image authority or atomic CAS across that lifecycle.
Do not replace that missing authority with a per-PID lock, VNUM lookup or new
per-NPC monetary account. Retain the existing complete stock/cash cut through
publication and cleanup; a future lifecycle integration must preserve that cut.

| Refusal/cleanup kind | Required disposition |
| --- | --- |
| Invalid keeper stock | `shop.c:1026` / `:1039`: inactive flat submits `discard_invalid` with physical publication; inactive SQL retains the legacy path. Active epoch refuses this unported mutation. Preparation accepts only buy/sell actions, so enabling buy/sell must not silently enable cleanup. A handled selection is not proof of durable deletion. |
| Definite local prejournal cancellation | `shop_trade_transaction.c:3709` cancels only exact safe stages/holds. `shop.c:1258` removes only the uniquely found original NOWHERE produced candidate with byte-equal original literal and verifies absence; a live keeper is unnecessary after proved cancellation. False/throw keeps the original/fences and no fabricated execution completion. |
| Never-admitted retained command | `shop_trade_transaction.c:123` and `player_save_pipeline.c:3142` use the guarded original refusal owner. Reuse [guarded refusal](../SHOP_GUARDED_REFUSAL_CALLBACK_2026-10-05.md) and [BEFORE reader](../SHOP_NEVER_ADMITTED_BEFORE_READER_2026-10-05.md): execution evidence is absent, not synthesized, and cold cleanup must provide actual effects/absence proof. |
| Uncertain native SQL/journal/physical tail | Keep original command, stage, receipt and fences. `drive` reconciles native uncertainty; submitted retry uses the frozen command. Never mint a replacement operation, erase a candidate by UID lookup failure, reopen admission, or treat a new proof transaction's ROLLBACK as the old outcome. |

## Cold recovery and current unavailable dependencies

`shop_trade_transaction_restore_replayed_command` at
`src/economy/shop_trade_transaction.c:2732` requires exact schema-2 v8 original
command/manifest, installs manifest/selected UID fences and registers the real
shared player obligation. Exact retries preserve command/completion/effect state;
registration doubt remains nonpublishable. It performs no effects or ACK under
the coordinator mutex. The installed startup observer and game pulses are at
`src/net/comm.c:356`, `:2385` and `:2387`.

The SQL cold callback at `shop_trade_transaction.c:1466` distinguishes execution
from never-admitted, authenticates current full SQL forests against original
manifest order and uses the complete world census. Missing body is separate from
present-empty; duplicate/wrong-role/disconnected claims refuse as specified in
[world witness](../SHOP_WORLD_WITNESS_SOURCE_2026-10-04.md) and
[cold observation](../SHOP_COLD_WORLD_OBSERVATION_2026-10-05.md). The callable
read-only observers are `src/economy/shop_trade_world_witness.c:476` and `:943`;
they scan before establishing body/UID presence or absence. Original literal
staging and procedure/effect state precede native handlers. Never call fresh
`read_object` or RNG as cold authority, fabricate a keeper, reuse a live completion
for cold effects, resume a bulk purchase or discharge an uncertain handler from
its apparent location. Final census, current projection, transaction cleanup and
guarded ACK remain one original obligation (`shop_trade_transaction.c:2290`, `:2352`).

The [fc8 published checkpoint](https://github.com/Community-Duris/Duris/blob/fc8a8961b281c4b53bc0f7af1459dbb131781676/docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md)
reports private candidate `48570aecc6c006fe4bc030930e90b01e6a1c38a9fc358bd7f6fc7e0f5078727e`:
147 production files, 79 selected C providers, nine accepted source slices and
13 overrides. Its ordinary NMB4/NBC4/264-byte MBR4 owners and restored flat outer
retain original command/receipt/reservation/fences and the shared 32 MiB charge;
the reported delivery edge is `critical_command_coordinator_pulse`. These are
owner-reported private findings, not callable SHOP interfaces in this public tree.
Never-admitted cleanup stays closed for that private outer. Do not generalize
that private disposition into removal of the existing public SQL SHOP source.

The same checkpoint leaves published-world SQL participation, full cold
PC/keeper/pet presence-or-absence proof, native effects, boot, admission/driver,
shared keeper atomic CAS, opening correspondence, activation and combined Plan5
qualification open. Public SHOP additionally has the explicit closed admission
gate and missing accounted flat parity. Required dependent execution waits for
actual published owner contracts/composition and the original qualification
readiness; unavailable inputs block those dependent steps, not this completed map.

## Acceptance reuse and next implementation boundary

Existing focused participants are `tests/async/test_shop_trade_command.py`,
`test_shop_trade_runtime.py`, `test_shop_trade_transaction.py`,
`test_shop_trade_typed_accounting.py`, `test_shop_trade_accounting_context.py`,
`test_flatfile_shop_trade_repository.py`, `run_shop_trade_sql_lock_mysql.py`, and
`test_shop_trade_publication_retention.py`. They protect command/value codecs,
legacy route retention, pure accounting and isolated backend participants at their
actual inputs. The retained-publication fixture declares 21 owner and 16 physical
cases at `tests/async/test_shop_trade_publication_retention.py:11` / `:25`; controlled
native endpoints and `restored_success` do not establish cold native authority.
Historical link failures and the old live-route token failure remain preserved
beside the closed optional fixes. No runner is executed or repaired here.

No new future acceptance reservation is made. Calculation, source-route and codec
coverage already exist; a new controlled wrapper would duplicate them while
inventing the absent authority. The next implementation boundary is the genuine
owner's complete shared keeper/current-world/admission/backend composition. When
those real contracts are available, reuse the original SHOP/native acceptance to
prove one root under different-PID keeper contention, exact original-ID uncertain
replay, ordinary save/checkpoint races, partial produced progress, explicit cold
presence/absence, actual cleanup and guarded ACK. Those are remaining original
obligations, not a new runner or permission to start the deferred major batch.

<!-- BODY_PINS -->

All bodies below are pinned at public primary `fc8a8961b281c4b53bc0f7af1459dbb131781676`.

| Whole body | Git blob |
| --- | --- |
| `docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md` | `dc1de7c33ff4af9328c51555bab83b025d573ed4` |
| `docs/persistence/economy_accounting/SHOP_COHERENT_SOURCE_INTEGRATION_2026-10-05.md` | `4225ff02dcd5446999211ac46958c37dc3528d96` |
| `docs/persistence/economy_accounting/SHOP_COLD_WORLD_OBSERVATION_2026-10-05.md` | `a30be462bad74cdc4fd7afbd8a5ee6f2ccd550f6` |
| `docs/persistence/economy_accounting/SHOP_FULL_PLAYER_NATIVE_PROOF_2026-10-04.md` | `1b39f0db6d4690d8a3d8d0f18f940034a37f7c14` |
| `docs/persistence/economy_accounting/SHOP_GUARDED_REFUSAL_CALLBACK_2026-10-05.md` | `88fd4692ab8e2048bb0ff3b09b565d21dc19c885` |
| `docs/persistence/economy_accounting/SHOP_NATIVE_ADMISSION_HANDOFF_2026-10-04.md` | `cc260bb6a792e9bd73d678db02f12266ffeb647f` |
| `docs/persistence/economy_accounting/SHOP_NEVER_ADMITTED_BEFORE_READER_2026-10-05.md` | `0acbd930f6ae1409f0b0d877603456f417226d3b` |
| `docs/persistence/economy_accounting/SHOP_ORIGINAL_COLD_PUBLICATION_INTEGRATION_2026-10-05.md` | `5aede37139ca3fb0fadc708756d2434ffc89f51c` |
| `docs/persistence/economy_accounting/SHOP_RECOVERY_BINDING_SOURCE_2026-10-05.md` | `e62568218921102d48e01ae0865c91d7e6d5b781` |
| `docs/persistence/economy_accounting/SHOP_RETAINED_PRODUCER_DRIVER_INTEGRATION_2026-10-05.md` | `30899071967a0b6ac1563aae2940ef8ab808fa21` |
| `docs/persistence/economy_accounting/SHOP_ROUTE_EVIDENCE.md` | `b77673830f07f5d13bf1a794795c04765379b33f` |
| `docs/persistence/economy_accounting/SHOP_WORLD_WITNESS_SOURCE_2026-10-04.md` | `1dab06b297800cc413f92bbec3518d8e8cb7bd91` |
| `docs/persistence/economy_accounting/domain-separation/R10_SHOP_PURCHASE_QUOTE_REVIEW_2026-10-07.md` | `40de66f73df786b30e872807b77704bef6fae01c` |
| `docs/persistence/economy_accounting/domain-separation/R12_SHOP_SALE_QUOTE_REVIEW_2026-10-07.md` | `9196cdb3d29a84abf45b29baa2c62705a4ddf5cf` |
| `docs/persistence/economy_accounting/domain-separation/R13_SHOP_ITEM_ACCEPTANCE_REVIEW_2026-10-07.md` | `3694f86b624bb24d4b0e7910bb3ab83efab34389` |
| `docs/persistence/economy_accounting/domain-separation/R14_SHOP_CUSTOMER_ACCESS_REVIEW_2026-10-07.md` | `c86ee5d3efb2b530913e15ea6ae3b384a34364b3` |
| `docs/persistence/economy_accounting/quest-prep/SHOP_CODEC_CLOSURE_REVIEW_2026-10-07.md` | `d1c3b1dce3686297171edb52237c36a69067b661` |
| `docs/persistence/economy_accounting/quest-prep/SHOP_LIVE_ROUTE_REVIEW_2026-10-07.md` | `01cf4c2054ac935d35746e4cc73d11a5238e3f67` |
| `src/economy/economic_command_admission.c` | `a3e5244e30144a8dd0dff8485abe130e64045ffb` |
| `src/economy/shop.c` | `0e63a8b3a72c705bce5030caf6b828b57c8ccae5` |
| `src/economy/shop_trade_accounting.c` | `e487e0b59d24cdbf0933bc049fef6b7f1597dc4b` |
| `src/economy/shop_trade_command.c` | `a987e5bf67d63736468d6147eac832a3ac6b4e76` |
| `src/economy/shop_trade_recovery_manifest.h` | `be6eff78f195f9b60263b863bc0ee219e7877a24` |
| `src/economy/shop_trade_runtime.c` | `a092efcc13b29d964a7f32121daecc589d150eb1` |
| `src/economy/shop_trade_transaction.c` | `d8f70a718b65c952f2f06f9c98d569ed4ee09b84` |
| `src/economy/shop_trade_world_witness.c` | `02a40e53eaf7f7fcb6cf914ff0b08e832c8442e6` |
| `src/flatfile/flatfile_shop_trade_repository.c` | `bf0337b741fac2b6e11418e8b3a2a4f5574d3ed2` |
| `src/net/comm.c` | `8bcbc9ab95073b0d5fa71dc3f3516db183982eb1` |
| `src/persistence/critical_command_coordinator.c` | `43a5b4f15c9171703e77b89b84bbc0c6f3a49b16` |
| `src/persistence/critical_command_repository.c` | `1f69092f0b3070d665952ec8c8638c1ec5d09b00` |
| `src/persistence/economic_sql_shop_trade_transaction.c` | `ec8c8f298e01c04aeec6d8a34018a39be1e72091` |
| `src/persistence/economic_sql_shop_trade_transaction.h` | `d92baf96baa19ac990af0881d7a61381ab451c58` |
| `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| `src/sql/sql_player.c` | `57a536a3a192b5432b17fd7ac088b76f7559b41e` |
| `tests/async/run_shop_trade_sql_lock_mysql.py` | `7c0471f3271ffca1038614beefc12f44eed857bd` |
| `tests/async/test_flatfile_shop_trade_repository.py` | `efb744023773d8d922475395211c6e018fd2a078` |
| `tests/async/test_shop_trade_accounting_context.py` | `b65db8694cbbc1ca4df4acd2d2aae49e88ffb4dd` |
| `tests/async/test_shop_trade_command.py` | `51449801c538c412be86671bc4789dbdb81b6602` |
| `tests/async/test_shop_trade_live_route.py` | `95803d00847f74c16e1647c80e67dd7c433656a5` |
| `tests/async/test_shop_trade_publication_retention.py` | `f4d67053ee155abdeee24a1fd968ae53ee458631` |
| `tests/async/test_shop_trade_runtime.py` | `4539cc28bd742448e8e6abfb91675289843de114` |
| `tests/async/test_shop_trade_transaction.py` | `ad29f1d63358abbae7246dd0f15fc83b41bd0dbd` |
| `tests/async/test_shop_trade_typed_accounting.py` | `d2194aad1c2a3adcff308659b168c88e7da091cd` |
