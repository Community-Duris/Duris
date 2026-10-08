# Currency native acceptance blueprint — 2026-10-08

This blueprint specifies the next genuine native acceptance work for ATM/shared
bank publication and ordinary-room coin drop/pickup. It adds setup, phase cuts,
expected assertions and runner ownership to the accepted
[currency boundary map](CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md), rather
than repeating its implementation inventory. **No case below was executed for
this delivery.** A source boundary is not an installed observation or fault hook.

## Source, review and proposed first slice

The authorization and independent map review are published at
`6f5d208de6dbf91d57e40b17e03b21f328337518` in
[PREPARATION_BUNDLES_REVIEW_2026-10-08.md](https://github.com/Community-Duris/Duris/blob/6f5d208de6dbf91d57e40b17e03b21f328337518/docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md).
All maintained references in this document use that full commit. Its `src` tree
is `833d3085815b396861ad18a77635412212381e4b` and `tests/async` tree is
`790f367adf805a69d53aac6460938f5c921f9136`, identical to the map's maintained
`a6aec4c4a058a5174e5b679708103762fa05e05a` source/test trees. The earlier read-only
export under `D:\Dev\Temp\currency-authority-20261008\source` is reused only
after individual body authentication. Additional inspected world data uses the
new maintained pin. New scratch belongs in
`D:\Dev\Temp\currency-native-acceptance-20261008`.

The accepted map is the exact body published at
`6da1fbd836e4e46b5f65630035f639c4c30ad792`, blob
`d3d1caab73fe706bdd7fc0e912fd7a2f83ede65e`, SHA256
`5b2ba114bd7543926642ba9eb9ccacb4662357d1270ccc840e52bc4b71fc0c6d`.
The existing [operation inventory](OPERATION_INVENTORY.md), R0–R14, optional R2
and closed operator qualification remain preserved. No code adoption, facade,
RAM authority switch, private candidate import or native qualification follows
from this blueprint. The continuing native Goal remains BLOCKED and unfinished.

**First native slice: C01, one active SQL `deposit 1 copper` through the real
registered command, followed by normal save/logout/relogin and cold reload.**
Use one already authenticated, funded PC in an actually loaded ATM room. Assert
the exact one-root durable transfer, native publication, original-ID ACK and
reload agreement. This is smaller than a two-PC/morph/crash slice and reaches a
real producer omitted by the existing ATM component. It does not substitute an
inactive or component result for active acceptance. Its production route exists;
its genuine initialized-world/active-authority fixture and phase observer are
not supplied here, so execution remains conditional on the input table below.

An optional native inactive control may establish parser/baseline behavior
earlier, but must be reported as inactive and cannot close C01. Do not seed an
active epoch or use the component's synthetic character/mapping bootstrap to
turn C01 green. The first code reservation should be one future runner,
`tests/async/run_currency_native_acceptance_journey.py`, with local assertions
and C01 only. That path is a proposal, not a file created or authorized to edit
by this delivery. Extend that runner for later cases rather than creating a
parallel runner framework or an accounting verifier.

## Authentic initial state and commands

Every case starts from observed identity and values, not expected constants
written into authority. Use a fresh isolated disposable runtime, journals and
backend roots supplied by their owners; never reuse player/production data.
Record exact binary/source/provider manifest, engine version, migrations and
runtime fingerprint, generated-world input hashes, configuration, accounting
mode and original initialization/activation evidence. The fixture must prove
the intended mode; a process starting or an active-epoch row alone is not that
proof. Do not install or activate anything as part of this preparation.

| Required initial observation | Actual input / native setup | Refuse the fixture if missing |
| --- | --- | --- |
| PC A | Real account/login and durable PID, racewar, current account association, runtime body ID, native wallet/bank vectors and revisions; matching actual durable row/domain image and authenticated wallet/bank lifetime. | Unknown account, missing baseline/mapping, identity mismatch, stale native opening or unresolved pending command. Never assign a PID, mapping ID or balance in the assertion helper. |
| Funding | C01 requires observed copper `W0[0] >= 1` and bank headroom. Retain the opening/source receipt or prior supported funding operation and its completed publication/ACK. | An arbitrary wallet UPDATE, `ADD_MONEY`, seeded opening epoch or staff coin grant is not a supported active funding witness. If only bank funds exist, a separately witnessed native withdrawal can fund C01 once its own prerequisites are met. |
| ATM room | Actual visible object accepted by `test_atm_present`, `src/cmd/actoth.c:2433`: native prototype 3097, 132581 or guild bank counter 48010 (`src/guild/guildhall.h:51`), placed in the PC's actual room and visible to it. | Prototype existence, source reset instruction or a fake `test_atm_present=true` is insufficient. Preserve actual runtime room VNUM, object UID/prototype, visibility and position. |
| Concrete world candidate | `areas/obj/verz1.obj:1199` defines 3097 as the bank counter; `areas/zon/newbie.zon:193` places it in room 29266; `areas/wld/newbie.wld:1351` defines that bank. | These are source-grounded candidates, not proof that a generated world loaded/reset them or that a character can reach them. Owner supplies genuine generated full-world artifacts and legal movement/setup chronology; do not issue an invented teleport/setup command. Active ordinary reset O remains a separate primary dependency. |
| PC B for shared bank | A second real PID, distinct runtime body, same case-insensitive account and racewar, authenticated shared bank row/lifetime, its own wallet/revision. Record a different-account and opposite-racewar observer if available. | Two synthetic bodies are not real login coverage. Respect actual simultaneous-login policy: `src/account/nanny.c:2430` has trusted/whitelisted host rules; require supported fixture eligibility, not a removed guard. No account or whitelist mutation is selected here. |
| Morph variant | A genuinely entered native morph with retained original PC/descriptor association and observable runtime IDs, using the actual supported morph route supplied by its owner. | The component's manually populated `descriptor.original` or replaced `IS_MORPH` is not a native morph journey. Morph capability, resource and command details remain required inputs. |
| Coin room/pile | An ordinary ground room and actual money prototype, with no water/fall/no-ground conditions; literal, UID and runtime custody independently captured. | `coin_physical_publication_room_safe`, `src/economy/coin_physical_publication.c:195`, rejects unsupported pile events/affects/traps/artifact/transient/lit state. Do not clear fields to force eligibility. |

Send actual native commands through the existing game connection/dispatcher:

- `deposit 1 copper`, `withdraw 1 copper`, `deposit all`, `balance`, `score`.
  `coin_type` accepts denomination abbreviations (`src/core/utility.c:2980`),
  but the first case uses full names. `do_deposit`/`do_withdraw` are registered
  at `src/cmd/interp.c:3129` and `:3248`. Preserve the current unusual
  `strstr("all", argument)` behavior (`actoth.c:2549`); an empty deposit argument
  is not a safe assumed syntax-refusal control because it enters that branch.
- `drop 7 copper` and `get coins`, using an isolated room with exactly one
  selectable matching pile whose UID was observed after the real drop. Native
  drop is `src/cmd/actobj.c:5063`; get parsing is
  `src/item/item_command_parser.c:21` and its money branch reaches
  `submit_coin_get` at `actobj.c:1165`. A partial-pickup quantity command is not
  provided by this parser. Do not invent `get 3 copper` as a partial-pile control.
- `save`, normal logout and reconnect, and a controlled process restart when
  its case is selected. Existing native connection/save patterns are in
  `tests/async/test_flatfile_combat_journey.py:79` and
  `tests/async/pa_copyover_fixture.py:426`; their fixture credentials and seeded
  miniature worlds do not grant active accounting authority.

For coin setup, use a successfully acknowledged native drop to create the pile.
`src/world/handler.c:6635` uses the actual money prototype and replaces prototype
amounts with the requested literal; the detached staging object's UID is the
original UID. Capture it from original command/native owner evidence, not by
assuming a counter value. A preexisting untracked pile is a separate C09 branch:
its absent-item enrollment (`actobj.c:4497`) precedes a recaptured coin command.
Do not relabel enrollment and pickup as one transaction.

## Observation contract for the future runner

Use `(copper, silver, gold, platinum)` throughout and value weights `(1,10,100,1000)`.
Let `W0/B0` be observed authoritative opening vectors, `rw0/rb0` their revisions,
and `A` the actual account/racewar plus native bank row/lifetime. Define expected
changes from those observations and the exact frozen command. Reject an
incomplete or mixed cut; never fill missing fields with the expected value.

| Cut | Required observations | Assertions and limits |
| --- | --- | --- |
| T0 — ready | Native A/body/room/ATM or pile; durable wallet/bank/current custody; authenticated epoch/lifetimes; prior funding/origin receipts; no unresolved predecessor. | Native and durable opening agree in every denomination/revision. Historical receipt authentication and current identity are separate checks. Preserve all original findings from supplied owners. |
| T1 — admitted/journal durable | Original operation ID, full canonical command bytes/hash, schema/reason/source/deadline, expected revisions, publication-required bit, pending owner and matching player/account/item fence keys; journal durability. | A successful submission is not final success. No pre-debit or pile placement/removal is permitted. Native output may say pending/busy; silence or elapsed time proves no phase. |
| T2 — backend terminal result before dispatch | Actual locked-row/domain outcome, result bytes/code, ledger and root accounting effects, outbox where applicable; same original command/journal. | Applied state must match the command exactly. Known business rejection may durably add receipt metadata without changing money/custody. Lost reply is uncertain even when an independent observer sees the commit. Never require globally unchanged SQL across admission or retained rejection. |
| T3 — live monetary/physical projection before ACK | Correct current body identity, all wallet/bank values/revisions, shared descriptors; for coin the original UID/literal/current runtime custody and projection completion. Retained original pending owner/receipt. | A scalar balance message is insufficient. Coin monetary and physical effects may be temporarily at different internal publication phases under the retained obligation. No external success notification or fence retirement may certify an incomplete projection. |
| T4 — durable ACK | Exact original checkpoint/typed owner handoff, pending/fence retirement and staging release; producer notification ordering. Coin cold cases also require authentic save slot/generation/frozen bytes and hold consumption. | An outbox status or journal absence alone does not prove publication. Failed ACK retains the original owner; notification failure after ACK cannot retry effects. Ordinary ATM has no invented coin save hold. |
| T5 — reload/cold | Normal authoritative load into a fresh body; cold room graph/current custody and original retained receipts; repeated stable cuts after recovery. | No new transfer/refund/UID on replay. Native projection agrees with current durable authority, which may legitimately be newer than an old receipt under the existing coverage rules. Historical success does not freeze all future balances. |

The native phase witness must come from the actual owning code on the game
thread. Proposed witness fields are an **evidence requirement**, not a new API or
wire format: operation/command binding, body/account/room and raw native vectors,
runtime custody/literal, owner phase and checkpoint/hold state. Current coordinator
health/durability/journal APIs exist, but no complete external native cut protocol
for this runner was found. A future owner-approved read-only fixture observer or
existing genuine observer artifact must expose the missing fields. Never infer
private pending state from a file name, output prompt or a sleep.

SQL capture uses the existing actual native tables and retained receipts: player
wallet/revision in `player_data`, shared bank/revision and actual row ID in
`account_banks`, exact original `critical_operation_inbox` and `currency_ledger`,
`critical_outbox`, and the economic operation/account-effect/coin-posting rows.
Use a separate bounded read-only consistent transaction coordinated with a
stable native phase; it must not own or alter the producer's transaction. Capture
raw values/bytes and compare the actual result; no decoded cache replaces raw
evidence. The existing ATM harness's exact successful root count assertions are
at `tests/async/pa_atm_publication_harness.cpp:325`. Coin also needs original
child IDs, `item_current_owner`, owner revisions, literal/coin payload, custody
ledger, economic item references and retained pile/source proof, using the
existing owner codecs/verifiers rather than rewriting their authentication.

Flat capture needs the actual identity and authoritative canonical player/bank/
item images plus retained record/result/plan/source proof under the existing
identity-before-authority cut. A sequence of unlocked file reads is not a
coherent authority cut. Preserve original transaction-journal recovery and
domain capacities; do not parse private bytes with an invented alternative
codec. The current headers/owners supply component access, not a finished native
observer process. Native snapshots must also include other same-account bodies
and any affected room so a missing projection cannot pass by omission.

## Expected monetary and identity projections

For C01 require both observed revisions below `UINT64_MAX`, adequate bank
denomination headroom and no unresolved predecessor. For deposit of one copper,
one new root `O` must have
`W1=W0-(1,0,0,0)`, `B1=B0+(1,0,0,0)`, `rw1=rw0+1`, `rb1=rb0+1`.
Its frozen expected revisions are `rw0/rb0`, account/racewar exactly A. Typed ATM
success has one native currency ledger and one success outbox in SQL, one
accounting operation, two account effects and two posting vectors; exact bytes
must match the result. Repeat receipt verification is read/replay of O, not a
second submitted `deposit` command. A subsequent native withdrawal is a separate
root `O2` with the inverse denomination vector and another pair of increments.

For `deposit all`, with nonnegative W0, `W1=(0,0,0,0)` and `B1=B0+W0` in one
command/root, including mixed denominations. There are no four separately
committed denomination operations. Check all bounds and no revision wrap.
Withdrawal uses the durable bank's funds; do not turn the absence of a native
bank precheck into an expectation of success. A normal success returns a balance
message only if the actor still has an ATM (`actoth.c:2502`); removal/movement
after admission can suppress that message without undoing the committed effect.

**Coin conservation is by value, not by a forced denomination round trip.**
`wallet_value_delta`, `src/economy/currency_transaction.c:1037`, canonicalizes the
total remaining wallet for a debit. Positive pickup adds the canonical vector
for the credited value. For an observed `W0=(23,4,2,1)`, a real `drop 7 copper`
therefore expects `W1=(6,5,2,1)` and a pile literal `(7,0,0,0)`; full pickup into
that wallet expects `W2=(13,5,2,1)`, not W0. This is a calculation example to
derive assertions, not an instruction to seed those balances. Each single-wallet
coin root advances its wallet and bank revision once under existing legacy
policy even though bank denominations do not change. Compare B exactly; do not
ignore the bank revision. Child/owner/item/pile revisions come from the actual
prepared payload and result, not an assumed global starting revision of zero.

Drop creates the captured original UID in the original room with exact literal,
root UID, parent zero, active custody and result owner/item revisions. Full pickup
credits the wallet and consumes that same pile; it must be absent from the
native room and all current durable active custody, with authenticated destroyed
history/current state as the owner contract requires. Partial pickup retains the
same UID with the exact nonzero remainder, advances the appropriate pile/head/
owner clocks and credits only the taken value. Count room graph matches, not
just a single successful lookup; duplicate UID or competing active custody must
refuse current proof.

A precise conditional partial example is an authentically observed recipient
wallet `(INT_MAX-1,INT_MAX,INT_MAX,INT_MAX)` and a genuine pile `(7,0,0,0)`:
only one copper fits, so the result wallet is `(INT_MAX,INT_MAX,INT_MAX,INT_MAX)`
and the same pile UID retains `(6,0,0,0)`. A subsequent retry takes zero and
admits no coin root. This supplies an independent expected vector and boundary
control, not a reachable funding claim; no genuine setup for that opening is
currently provided.

## Case specifications, fault cuts and coverage reuse

Each row lists actual available inputs and a missing dependency. Future runner
edits mean proposed extensions to the single currency runner named above,
subject to a later reviewed reservation. Tests cited here were inspected only;
their original assertions/controls and limits stay in force.

| Case / real command and setup | Required assertions | Existing meaningful coverage / available inputs | Missing input and future runner slice |
| --- | --- | --- | --- |
| **C01 first — native SQL deposit/reload.** Real funded A at observed ATM; `deposit 1 copper`; normal save/logout/relogin, then cold reload. | T0→T4 vectors/revisions and root counts above; one original ID/checkpoint, no early native debit, post-ACK output ordering, T5 native/durable agreement, no bulk-save overwrite. | Actual parser/producer, repository/coordinator and load/save paths; `pa_atm_publication_harness.cpp:372`/`:405`/`:454` checks typed SQL completion/offline replay but bypasses real command/location and uses synthetic bootstrap/publisher. | Genuine initialized-world/active fixture, funded lifetime/source proof, exact binary/backend and native cut witness. Future one-runner C01 implementation is the first reservation candidate; no fault mechanism needed for initial happy path. |
| **C02 — denomination/all and refusal controls.** Separate roots: `withdraw 1 copper`, mixed `deposit all`; no-ATM room; `deposit 0 copper`, `withdraw 0 copper`, invalid denomination; amount exceeding observed carried copper; withdraw `B0[c]+1` within native integer range. | Exact inverse/all vectors and one all root. Native validation refusals admit no operation and have no monetary effects; durable insufficient withdrawal retains terminal `ENOSPC`/before-state result, no success ledger/outbox or debit. Capacity/revision overflow refuse without partial effects. Do not demand empty inbox after a retained business rejection. | `actoth.c:2534`/`:2632`; pure mutation/adapter tests; `economic_currency_adapter_test.cpp:258`; typed bank root at `critical_command_repository.c:2530`. | Mixed/near-bound authentic openings and reachable no-ATM room. Extend C01 runner with input/output and selective durable assertions; no unbounded/overflowing text-to-int oracle or weakened error ordering. |
| **C03 — two-PC shared bank and command fence.** A/B genuine concurrent sessions with same A; submit A deposit, then B withdrawal while O is retained, then allow publication/ACK. | Before O finishes, B cannot execute a stale nonrebasable bank mutation. Queue gating can defer input; it need not emit an immediate refusal. After release, B captures current bank revision and a separately admitted root. Both same-account bank projections agree, B wallet changes only for B's operation; other account/racewar remains unaffected. | Real busy scan `currency_transaction.c:1139`/`:1183`; native dequeue `src/net/comm.c:1520`; real publisher `utility.c:3240`; flat contention component `flatfile_accounting_bank_test.cpp:985`. | Eligible simultaneous sessions, deterministic retained phase observation/control and current bank mapping/lifetime. Extend runner after C01; do not disable login policy or call direct submit to manufacture a native race. A second mutation forcibly bypassing native fences is an owner component race, separately labeled. |
| **C04 — offline/morph/native newer body.** Disconnect A after admission without changing O; reconnect through actual load/readiness. Separately place B or A in a genuinely established morph. | Offline ordinary ATM retains pending original command and no fabricated callback success; real return publishes/ACKs once. Shared publisher targets descriptor original/MORPH_ORIG with exact account/racewar; morph shell is not assigned PC bank. Older completion cannot overwrite a genuinely later revision. Never force the actor to morph after admission through a gated command. | `pa_atm_publication_harness.cpp:420`/`:438`/`:454`; `test_morph_bank_publication.py:88`/`:96`/`:103`; current game-thread publication and readiness bodies. | Authentic morph/reentry chronology, offline detach and observer cut. Sequential reload of B can cover shared account before the simultaneous fixture exists, but cannot close C03. Extend C01 runner with genuine session transitions, keeping component-only synthetic bodies labeled. |
| **C05 — original-ID lost reply / retained rejection.** Hide the backend COMMIT reply after actual successful commit, or lose process after journal durability and before ACK. Replay the original command/journal; separately retain insufficient-funds rejection. | Commit ambiguity produces no terminal failed-payment notification or refund. Reconciliation of O returns exact original result with no second ledger/effects/revision increments. Known rejection replays its original code/result and changes no money/pile. Conflicting same-ID body refuses; a second player command is not a retry of O. | SQL link wrapper `economic_sql_bank_transaction_mysql_harness.cpp:44` hides a real COMMIT reply and supplies error 2013; real-pool controls at `:522`. `test_currency_completion_retention.py` covers ambiguous/malformed/offline receipts with doubles. | Wrapper is test-only, not installed in a native server. Need owner-approved genuine reply-loss fixture/binary and deterministic pre-ACK stop evidence. Extend future runner's fault orchestration only after that owner publishes it; do not add a network proxy or production fault switch here. |
| **C06 — full ordinary room drop/pickup.** A has genuine funds and a safe room; `drop 7 copper`, wait for complete root/ACK, observe its UID, then `get coins` targeting only that pile. | Two distinct coin roots and their derived child bindings; value and all vectors/revisions as above; exact original pile identity/literal/room; no durable wallet-only or pile-only success; physical placement/extraction before ACK/notification. Failed admission destroys detached staging without monetary/custody mutation; its allocated staging UID need not roll back. | Native drop `actobj.c:4809`/`:5063`, pickup `:4479`; actual SQL compound root `critical_command_repository.c:2170`; physical publisher `coin_physical_publication.c:206`; physical tests and typed flat coin root components. | SQL qualified boot/current-world owner and complete native/retained/custody observer. Current flat central coin admission is closed (table below), so a flat component pass cannot close live flat C06. Add this to the same runner after C01, preserving original item/save/coin owners. |
| **C07 — partial pickup and zero capacity.** A real recipient with an authenticated near-capacity wallet tries `get coins` on a real acknowledged pile; choose expected whole-coin take from captured W0/pile. | `submit_coin_get` (`actobj.c:4559`) chooses fitting whole coins; wallet credit is canonical(taken value), exact per-denomination remainder preserves the original UID. Zero take admits no coin mutation. No implicit lower-denomination pile conversion; compare retained result and current head on replay. | Actual capacity algorithm and owner literal projection; `flatfile_accounting_coin_test.cpp:1249` marks its genuine component partial-pickup case; physical and recovery component scenarios. | Authentic supported near-bound funding/opening is not provided. No direct assignment of INT_MAX cash or invented quantity syntax. Until an owner supplies reachable capacity state, retain this as a precise setup dependency, not a successful native test. |
| **C08 — committed coin, physical/ACK interruption and cold retry.** Preserve original room-coin command and sealed execution receipt after COMMIT; interrupt before physical publish or after it before durable ACK; cold boot/retry. | Monetary effects occur once; original UID/literal/current custody restored or consumed exactly; false/throwing publisher and failed ACK retain original obligation without schema-2 retry exhaustion. Cold save owner proves exact held slot/generation, current wallet/pile/bank cut and census, then typed ACK consumes hold. Conflicting literal, UID, body, changed receipt/census or lost reservation refuses and retains evidence. | `test_coin_publication_ack_retention.py:552`/`:591`; actual recovery `src/economy/coin_physical_recovery.c:1260`; save handoff `src/player/player_save_pipeline.c:3183`; existing cold SQL room runner describes full-world pinned observer discipline. | Actual original pre-ACK journal/receipt, cold room catalog, qualified native observer/save reservation and owner-approved fault cut. Existing room graph observer protocol is not automatically a coin ACK observer. Extend runner only when these arrive; no generic ACK call or regenerated command. |
| **C09 — preexisting untracked pile enrollment.** Use a genuinely created existing money pile with original custody/source evidence, currently absent only from the runtime ownership registry under an owner-supported cold/enrollment path. | Record enrollment ID and later coin ID separately; wait for genuine enrollment and recapture; no money credit from enrollment alone, and failed enrollment leaves no pickup. Preserve actual source/UID and original command receipts on retry. | `actobj.c:4497`/`:4532` plus its recapturing completion; item owner source. | Genuine origin/current absence and supported admission owner. Reset-created piles and private reset reports do not supply this setup by themselves. Do not delete a registry row or seed an untracked object and call it a native journey. |

## Fault locations are conditional, not available hooks

| Boundary in actual code | Existing mechanism and what it proves | Requirement for future native execution |
| --- | --- | --- |
| After root COMMIT, reply reported lost — `critical_command_repository.c:2598` (ATM) / `:2402` (coin) | SQL component link wrapper calls real query first and hides the result; actual root reconciliation and real pool replacement can be checked. | A separately reviewed native fixture build/control must apply the fault to the one intended original operation without changing authority. External SQL observation alone cannot say the game thread received an ambiguous outcome. |
| Flat authority journal/image publication | `src/flatfile/flatfile_authority_transaction.c:479` has compile-guarded `DURIS_FLATFILE_AUTHORITY_FAULT_TEST` controls after journal/operation/image. `flatfile_accounting_coin_test.cpp:1582` uses the image interruption, expects ambiguous EIO, then same-ID already-applied recovery. | This is an error-return component interruption, not proof of a real process crash. Preserve compile guard/recipe and recovery journal; native coin admission must first exist. Never enable ambient fault variables in an ordinary server. |
| Completion delivery before live publish — `currency_transaction.c:1649`; coin monetary→physical — `:559`/`:446` | Existing harness withholds completion, supplies publisher/ACK failure doubles, or reconstructs a component owner. | A genuine native held-phase observer/control is missing. Arbitrary sleeping, TCP output blocking or stopping after a prompt does not identify this phase. Do not change production dispatch or replace the actual physical publisher. |
| Journal checkpoint after projection — `src/persistence/critical_command_coordinator.c:3126` | Real checkpoint result gates generic ACK/fence release. Cold coin uses a distinct typed save owner. | Owner-reviewed deterministic checkpoint failure or crash observation; preserve original journal and any save hold. Post-ACK notification exception is a separate control and must never redo economic/physical work. |
| Cold current proof → guarded ACK — `player_save_pipeline.c:3183`/`:3280` | Current code rechecks exact save reservation, census and sealed receipt around physical recovery. | Actual installed slot/generation and native/current backend evidence. A historical receipt, classification helper or matching ID cannot manufacture this capability. |

For every fault run retain the failed original attempt, exact command/result and
phase evidence, then its same-ID recovery. A retryable error or exhausted
automatic execution is not a terminal business rejection. Compare economic
state at equivalent stable cuts; after a real commit the ledger/inbox legitimately
differs from T0. Do not weaken the oracle to make those cuts appear unchanged.

## Backend availability and original controls

| Route at the maintained pin | Current source capability | Native qualification boundary |
| --- | --- | --- |
| SQL ATM | Typed bank producer, locked SQL writer, retained verification and publication/ACK wired through actual server coordinator. | Needs genuine initialized active fixture and observer; selected restricted qualification scopes can refuse ordinary ATM. No successful current full native run is asserted. |
| Flat ATM | `economic_flatfile_command_admission_supported`, `src/economy/economic_command_admission.c:163`, admits supported account-bank reasons; `src/net/comm.c:1130` selects flat dispatcher. Typed flat bank root and publication code exist. | Flat identity/baseline/current-world installation and native restart/save observation still required. Preserve different legacy compatibility policy and canonical image/journal behavior. |
| SQL room coin | General schema-2 coin intent/compound root, real room publisher and SQL cold restore/current proof exist. | Complete source support is not initialized-world, runtime or recovery qualification. Keep original lineage/room catalog and save-owner proof. |
| Flat room coin | Typed flat compound owner/components exist, but central flat allowlist returns only account-bank/item-transfer (`economic_command_admission.c:181`); `src/economy/coin_physical_recovery.h:20` explicitly retains the central admission closure. | Expected current live typed-coin refusal; no allowlist bypass to obtain a pass. Flat owner/admission/hold/replay integration and qualification must be published before live C06–C08. |
| Inactive/legacy coin and snapshots | Real native routes and older native copyover/SQL tests exist; SQL `test_pa_coin_sql.py:12` explicitly invokes the legacy storage matrix. | Useful compatibility controls, with different composite drop/refund behavior. They do not prove active typed compound atomicity or flat active parity. |

Reuse original controls without reopening passed bundles just to fill time:
pure mutation/adapter and numeric controls, typed SQL/flat bank roots, original
coin accounting/pile/refusal/cold cases, completion/ACK retention, morph/shared
publication, queue gating, opening/load and snapshot non-overwrite. The accepted
map already enumerates their scoped assertions and pins. The native runner adds
what they omit: real command dispatch, actual world/identity/funding, true native
projection and authoritative cold replay. When code or dependencies change,
select only affected existing controls plus the new native case; retain original
flags, provider lists, budgets and failure/skip distinctions.

The future runner must report unavailable required inputs as BLOCKED, not PASS
or a successful skipped native case. PASS for C01 requires its actual registered
command and all required phase/reload assertions on the pinned inputs. A partial
case set cannot claim native currency qualification, backend parity or full
Plans 1–5/R1–R8 completion. Retain original failures, fault attempts, replay
receipts and observed phase timestamps alongside the final outcome; do not
replace them with a green aggregate count.

The existing SQL ATM wrapper is `tests/async/run_pa_atm_publication_sql.sh:8`:
assigned resource/heavy locks, disposable loopback schema and exact base-build
artifact are prerequisites. Its harness also hard-checks journal path layout
(`pa_atm_publication_harness.cpp:357`); do not redirect it blindly to D: or alter
its assertions. For a later approved runner, use task-specific D: runtime,
scratch/evidence and `BIN_ROOT` build output, `/mnt/d` in WSL, direct D: Docker
mounts where applicable. Record actual paths and toolchain/binary hashes.
Preserve live jobs, existing volumes and closed qualification packets.

## Precise missing inputs and next reservations

| Priority / proposed output | Available inputs now | Exact dependency / ownership limit |
| --- | --- | --- |
| 1 — Review C01 runner reservation, one proposed `tests/async/run_currency_native_acceptance_journey.py` | This blueprint, accepted map, real command/repository/publication/load sources, existing native transport and read-only SQL/component recipes. | Primary supplies exact integrated source/binary/schema/world initialization and legitimate funded PC/account/lifetimes, mode/admission scope, native phase witness and scoped disposable runtime. Reservation must choose concrete observer/read inputs before implementation. No production observer is invented here. |
| 2 — Implement C01 when those inputs are available; then C02/C03/C04 in the same runner | Exact first-case vectors/counts and shared bank/body/queue assertions above. | Shared login/morph/fault chronology remains owner-provided. No duplicate helper/framework, quest path, primary verifier/normalizer/producer, Plan5 audit or backup edit. Keep reader assertions local to this runner until another real consumer justifies extraction. |
| 3 — Native SQL ordinary coin C06, then C08 cold proof | Actual SQL root, native publisher/current-proof/save-owner code, original control sources and real drop-derived UID setup. | Exact original command/receipt, initialized current room/custody and save owner/observer/fault evidence. Use owner verifier and Plan5 outputs as authenticated inputs, not a new auditor. |
| 4 — Partial-pile C07 and enrollment C09 setup feasibility | Real capacity/enrollment source and scoped component scenarios. | Authentic reachable near-capacity opening/funding or current registry absence/producer source. An explicit future setup-feasibility reservation can resolve chronology; no seeded cash/pile or registry deletion. |
| 5 — Flat native parity C01 then C06–C08 | Flat ATM source is present; flat coin components and private reported integration plans exist. | Flat ATM genuine installation/observer first; coin central admission and shared hold/replay support must be actually published and qualified. A private report is not callable source or native acceptance. |

No next runner, assertion helper, hook, source edit or new task starts from this
document. It is one finite preparation delivery for independent review. Task-
specific unknowns above do not prevent writing the case design; they prevent
claiming an executable authentic setup or run that was not supplied. Primary
never waits for adoption of this optional blueprint. Quest owns all quest-prep
paths and QP02/QP03 work; this document neither edits nor duplicates them.

## Validation and exact file bodies

Executed validation is limited to this document: authenticate cited maintained
and accepted-map/inventory bodies, verify line anchors and links, check the
sole-path diff and whitespace, then commit/push. No native, DB, server, migration,
Docker, build or test was run. The appendix uses full maintained revision
`6f5d208de6dbf91d57e40b17e03b21f328337518` except owned map/inventory, which use
`6da1fbd836e4e46b5f65630035f639c4c30ad792`.

<!-- BLOB_PINS -->

| Revision | Inspected file | Exact Git blob |
| --- | --- | --- |
| maintained | `areas/obj/verz1.obj` | `66a3f57de4169d7c2f89fad61772a6e1f8910718` |
| maintained | `areas/wld/newbie.wld` | `9b35bddb8f5ac554f83385cfba48cef01ad9961a` |
| maintained | `areas/zon/newbie.zon` | `485137c57f56746a5eb61ef4f87d08203e330712` |
| owned | `docs/persistence/economy_accounting/domain-separation/CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `d3d1caab73fe706bdd7fc0e912fd7a2f83ede65e` |
| owned | `docs/persistence/economy_accounting/domain-separation/OPERATION_INVENTORY.md` | `e1761f0dde821d281a9fd24618d932234d192f90` |
| maintained | `docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md` | `b302ffa2d26570c7fa1ea280b1fac9f4bb3d0d2b` |
| maintained | `src/account/nanny.c` | `05ff831e396bc5027228db0a086a4ef58320775f` |
| maintained | `src/cmd/actobj.c` | `a2114fddb6816f1534488ff11457a1001d47a14c` |
| maintained | `src/cmd/actoth.c` | `53554534c4c411cbcddcdac7844f1456657d9cd3` |
| maintained | `src/cmd/interp.c` | `74281b6f20ccd88a588d63cdda143da275258e85` |
| maintained | `src/core/utility.c` | `92bdffd57f8979d9531251d1b049887505d7715d` |
| maintained | `src/economy/coin_physical_publication.c` | `932a7631a5be6163c3074bbada3d774014eb8d3d` |
| maintained | `src/economy/coin_physical_recovery.c` | `19027a42cfc993deeeb7b2b6c08e0f83b43337e1` |
| maintained | `src/economy/coin_physical_recovery.h` | `0a73049e165db8e62d9e3c02c9b3a0f9eb1e21d9` |
| maintained | `src/economy/currency_transaction.c` | `7366935cbf829554395f739f7af7b32ba303558f` |
| maintained | `src/economy/economic_command_admission.c` | `a3e5244e30144a8dd0dff8485abe130e64045ffb` |
| maintained | `src/flatfile/flatfile_authority_transaction.c` | `4ef5a310e7565f41ddaf3bcce780219a7aad7029` |
| maintained | `src/guild/guildhall.h` | `254b7d50dbbf34054d232d9ca21471db692919a8` |
| maintained | `src/item/item_command_parser.c` | `e1b052a89fe729aef62e85c06576721302dabce2` |
| maintained | `src/net/comm.c` | `8bcbc9ab95073b0d5fa71dc3f3516db183982eb1` |
| maintained | `src/persistence/critical_command_coordinator.c` | `43a5b4f15c9171703e77b89b84bbc0c6f3a49b16` |
| maintained | `src/persistence/critical_command_repository.c` | `1f69092f0b3070d665952ec8c8638c1ec5d09b00` |
| maintained | `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| maintained | `src/world/handler.c` | `0054c7db2f7cb990dfde047e6de09250d54670e3` |
| maintained | `tests/async/economic_currency_adapter_test.cpp` | `88c66a7490a954c90a6ddbaf94a161fc44e40df2` |
| maintained | `tests/async/economic_sql_bank_transaction_mysql_harness.cpp` | `df57e85e386c2aded5a057645d071818b60bf60b` |
| maintained | `tests/async/flatfile_accounting_bank_test.cpp` | `d0b1584def10500029e1239b00fc483ad3c1a861` |
| maintained | `tests/async/flatfile_accounting_coin_test.cpp` | `24351b6f2504ee3a3c4c2510ec49171941fee1f9` |
| maintained | `tests/async/pa_atm_publication_harness.cpp` | `553d20ca939e431fd9c6bf41e06bdc02a0e15357` |
| maintained | `tests/async/pa_copyover_fixture.py` | `7c674aa5acfa0a022b85fd28b887f5e0e9672efe` |
| maintained | `tests/async/run_pa_atm_publication_sql.sh` | `77d07af771538dd19a0f5a82822ac4976c3eac32` |
| maintained | `tests/async/test_coin_publication_ack_retention.py` | `d2aa15e20e8a8362d73a83b357998892f1bd2c2c` |
| maintained | `tests/async/test_currency_completion_retention.py` | `3b92ac83a65ccb60841f1c55287bfa79603de127` |
| maintained | `tests/async/test_flatfile_combat_journey.py` | `403157ea76702ec1ebe8eb61d1eb9675cc132235` |
| maintained | `tests/async/test_morph_bank_publication.py` | `6c338c7ae51e304bc82b74b512afcb056a5ab2b1` |
| maintained | `tests/async/test_pa_coin_sql.py` | `bebd8af936fbf5a49699dcbef5c5e9b3ac822298` |
