# SQL wallet, shared-bank and coin-pile lifecycle owner

The owner creates a durable maintenance receipt and a complete baseline for SQL-native wallets, shared banks, and active coin piles in `item_current_owner`. Coin-pile accounts use the never-reused item UID directly; they do not receive a second mapping row. Wallet and bank mapping IDs are allocated around active pile UIDs so the baseline's account identity reservations remain distinct. Installation leaves `economic_lineage_state.active_epoch` NULL. A separate guarded activation API exists for complete route evidence; production has no registered verifier and cannot select an epoch yet.

## Trusted boundary and call order

The caller must use a dedicated, reconnect-disabled, autocommit MySQL control connection and follow this order:

1. At runtime boot, acquire `economic_sql_lifecycle_guard::acquire_runtime(control_connection, &runtime_guard)` **before admitting gameplay**. Keep the guard/control connection alive until shutdown. It refuses a staged installation or inconsistent decision. With a matching durable active decision, `recover_runtime` reads the baseline and mappings back into the process admission cache before gameplay.
2. For one-time maintenance cutover, first quiesce the MUD using the trusted boot/maintenance process, then acquire `economic_sql_lifecycle_guard::acquire_maintenance(control_connection, &maintenance_guard)`. The owner uses MySQL named locks, not a caller boolean/digest: the runtime boot lock must be free and the currency-writer lock must drain. Both controls are held for the maintenance guard lifetime.
3. Build a stable `economic_sql_lifecycle_request`: operation ID, lineage, epoch, actor ID, and accepted timestamp must be reused exactly on retry. Call `economic_sql_accounting_lifecycle_transaction::install(control_connection, maintenance_guard, request, &receipt)` while maintenance authority remains alive.
4. The owner captures/normalizes native source rows under the guard; verifies every SQL wallet PID and shared-bank row has one valid native locator; verifies every active `VOBJ_COINS` item has one valid UID-keyed coin payload and rejects unresolved coin custody; rejects unsupported, negative, incomplete, or over-capacity source sets; creates one durable `economic_account_mapping` lifetime per wallet/bank; persists all wallet, bank, and active coin-pile balances in the opening baseline; and reconciles/read-backs the exact critical baseline receipt/witness before moving the lifecycle receipt from phase 1 to phase 2.
5. The receipt exports wallet and bank mapping data, not permission to activate gameplay. Pile identities remain their native item UIDs and are retained in baseline account effects/reservations. The private gameplay cache is admission metadata, not storage authority.
6. For activation, a trusted maintenance caller must hold an `economic_sql_cutover_transaction_owner` on the exact control session and call `activate_verified` with the same request, complete route counts, nonzero manifest and independent audit digests, and a Plan 5 verifier. The owner captures all sources in that transaction, checks pending inbox/outbox and quarantine state, runs the verifier in a savepoint, reads back baseline and mappings, and atomically records a global decision and `active_epoch`. The caller owns commit, rollback, and publication; a failed call must be rolled back. An exact committed retry returns the existing decision without recapturing holdings. `pause` clears the pointer and records a paused decision in one owner transaction; resumption requires fresh verification.
7. Do not start gameplay from the staged phase. The runtime guard refuses admission until durable decision, pointer, and baseline agree. Full gameplay activation requires the remaining producer/writer coverage and independent audit from Plans 2-5.

The `economic_sql_lifecycle_guard` token is tied to the exact MySQL connection/thread session that acquired the named locks. Do not reconnect, change MySQL thread, enter with an open transaction, or destroy the control connection before the guard.

## Parent integration wiring

The parent commit `30c9a6590` defines the private `economic_gameplay_authority::install` method and grants friendship to the `economic_sql_accounting_lifecycle_transaction` class. The cache accepts wallet `{pid, account}` and bank `{name, racewar, account}` spans. This owner exports compatible mapping data; preparing those spans does not itself authorize installation:

```cpp
std::vector<economic_gameplay_wallet_mapping> wallets;
std::vector<economic_gameplay_bank_mapping> banks;
for (const auto &mapping : receipt.wallets)
    wallets.push_back({ mapping.pid, mapping.account });
for (const auto &mapping : receipt.banks)
    banks.push_back({ mapping.name, mapping.racewar, mapping.account });
// No gameplay authority is installed by this staged-baseline component.
```

The trusted friend class installs the cache during guarded runtime recovery after it verifies the durable active decision, baseline, and mappings. The parent must not activate online gameplay based solely on this wallet/shared-bank receipt.

For actual native currency serialization, every legacy SQL writer needs this sequence around its own database transaction:

```cpp
economic_sql_currency_writer_guard writer_guard;
if (economic_sql_currency_writer_guard::acquire(connection, &writer_guard))
    return fail_closed;
// BEGIN/updates/COMMIT or ROLLBACK; writer_guard remains in scope throughout.
```

The owner and guard modules are included in `src/Makefile`; client-free builds expose refusing `ENOTSUP` implementations. Production boot holds the runtime guard through admission and recovers an active cache only from durable evidence. Source-complete legacy-writer coverage remains unfinished. The SQL critical dispatcher holds the legacy writer guard for schema-v1 item, coin, auction, collector, corpse and restitution commands, from before `START TRANSACTION` through commit, rollback or replay return. It refuses staged phases 1/2, any non-NULL active epoch, and missing/unreadable lifecycle schema. It does not fabricate an accounting root for a legacy inbox ID. Every affected writer must participate before these locks establish a complete runtime exclusion boundary. An available advisory lock is not proof that an older, uninstrumented MUD is stopped.

## Durable state and replay behavior

`economic_sql_lifecycle_installation` has one row per lineage and binds a request digest, source-capture digest, native-boundary digest, source counts, baseline operation ID, phase, selected epoch, and revision. Phase 1 represents the durable staged owner and mapping transaction. Phase 2 is written only after the baseline command returns an applied/already-applied result and a separate reconcile confirms the same durable baseline revision. `active_epoch` remains NULL in both states.

An exact retry verifies the same operation/request/lineage/epoch, native boundary, lifetime mappings, baseline operation ID, and retained baseline witness; it does not advance baseline revision. Reusing the operation ID with changed request fields is rejected and leaves the output object unchanged. An interrupted phase-1 operation remains fail-closed and can resume only with the same IDs/source boundary; absence of a row never authorizes overwriting existing lineage/mapping state.

`economic_sql_currency_writer_guard` acquires a process-local shared lock and SQL named lock before the legacy writer begins; it holds through writer scope exit and refuses participating old paths if a staged installation or active epoch exists. `acquire_maintenance()` takes the process-local exclusive lock, then the runtime boot lock and currency-writer lock, so it waits for a participating writer's commit/rollback. Every production legacy writer must use this helper for the serialization guarantee to apply.

## Scope limits

This boundary enumerates `player_data` wallet PIDs, `account_banks` row lifetimes, and active money-item values with their item UIDs. A coin item in unresolved custody blocks this cutover instead of disappearing from the opening balance. It is **not** complete economic-state authority for non-coin inventory custody, auction escrow, corpses/claims, death restitution, NPCs, quest/reward sources, or other game state. Parent tests for ATM/flatfile admission do not prove those producers are integrated into a full SQL epoch.

The activation API refuses pending/unpublished SQL work and quarantined or incomplete sources. It cannot supply the independent game-wide route census and audit itself. The writer/boot gate detects retained staged installation rows; do not interpret absent installation evidence as permission to resume legacy writes after an evidence-loss incident.

## Coordinator and flatfile accounting roots

The coordinator's SQL pooled apply and reconcile paths admit supported schema-2
bank, coin, and item commands. Each root checks its retained native and EAI1/EAP1
evidence and its exact root outbox event before reporting a durable completion.
The coordinator fixture holds publication across restart, checks same-ID replay
and conflicting-ID refusal, and simulates a lost result after the pooled commit.

Flatfile admits schema-2 bank and item commands through their native authority
journals. A flatfile coin root is refused before journal admission until its
typed owner is qualified. Bank and item fixtures exercise interruption,
recovery, held publication, and acknowledgement under the original ID.

These routes do not widen immutable migration 0036's wallet-root qualification
scope or authorize a game-wide active epoch. The staged runtime gate remains in
force until the remaining writer coverage and independent audit are qualified.

## Disposable validation

Run the behavior test on a disposable MariaDB/MySQL container only (the runner never reads `.env`):

```sh
make test TEST_MATCH=economic_sql_lifecycle_no_mysql TEST_JOBS=1
make test TEST_MATCH=economic_sql_lifecycle_owner_contract TEST_JOBS=1
ECONOMIC_SQL_LIFECYCLE_DB_IMAGE=mysql:8.0 \
  make test TEST_MATCH=economic_sql_lifecycle_owner_contract TEST_JOBS=1
```

The SQL runner imports fresh bootstrap, removes/replays the lifecycle schema and global decision migration, then runs a C++ ASan/UBSan harness. It exercises complete wallet/bank/item-row enumeration, mapping ID separation from colliding pile UIDs, UID-keyed coin-pile balances and postings for every active pile, refusal of invalid or unresolved coin custody, fail-without-partial-writes, baseline receipt read-back, writer serialization, exact and conflicting replay, and staged runtime refusal. Its synthetic route manifest tests incomplete-evidence refusal, activation, active recovery, exact retry, pause, and paused-runtime refusal only in a disposable schema. The separate client-free test checks that unsupported-backend calls cannot issue authority or mappings. These are component contracts, not game-wide producer coverage or live activation approval.
