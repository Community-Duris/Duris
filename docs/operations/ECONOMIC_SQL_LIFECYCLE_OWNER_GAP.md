# SQL double-entry lifecycle owner: delivery status

**Status: wallet/shared-bank mappings and UID-keyed coin-pile opening balances implemented; runtime activation and complete writer authority remain unfinished.**

The original evidence described the pre-owner gap and its RED test. The component now supplies cooperating boot/maintenance and writer guards, native SQL capture including `economic_account_mapping` and `item_current_owner.coin_payload`, wallet/bank lifetime mappings, UID-keyed opening balances for every active coin pile, collision-safe wallet/bank mapping IDs, baseline receipt replay/read-back, and a staged lifecycle receipt. An active coin without a valid payload or a coin in unresolved custody blocks installation. Those guards are not yet installed at the production admission/writer call sites. See [`../persistence/economy_accounting/SQL_LIFECYCLE_OWNER.md`](../persistence/economy_accounting/SQL_LIFECYCLE_OWNER.md) for the API and remaining integration limits.

## Preserved safety decisions

- No caller-provided boolean or digest is treated as authority. Lifecycle requires a live `economic_sql_lifecycle_guard` acquired on a reconnect-disabled, autocommit control connection. A process-wide MySQL advisory lock excludes runtime admission while maintenance owns the cutover boundary; a second named lock drains/serializes currency writers.
- Native wallet (`player_data.pid`), shared-bank (`account_banks.id`, including `racewar`), and active money-item (`item_current_owner.item_uid`) balances are enumerated from a repeatable SQL source snapshot. Wallet/bank mapping checks and exact active coin-payload coverage reject missing, duplicate, negative, unsupported, or capacity-exceeding holdings before durable installation. Piles use item UIDs directly and create no account-mapping row.
- The lifecycle baseline operation is applied and reconciled against its retained witness/critical receipt before the separate installation row advances from phase 1 to phase 2. A SQL trigger in the behavioral harness rejects selection if either receipt is absent at the update boundary.
- The SQL lifecycle row records a staged maintenance cutover only. It does **not** update `economic_lineage_state.active_epoch`. The runtime/legacy-writer guard APIs refuse admission while any staged installation exists; production callers still need to adopt them.
- The private gameplay-cache API in the parent commit is not copied, called, or modified here. Future activation must run inside the trusted friend owner while maintenance is held and after all source/writer authority is verified. No broad gameplay coverage is claimed.

## Owner and verification

- SQL owner: `src/persistence/economic_sql_accounting_lifecycle_transaction.{h,c}`.
- Admission/writer gate: `src/persistence/economic_sql_lifecycle_guard.{h,c}`.
- Additive schema: `migrations/immutable/0033_economic_sql_lifecycle_owner.sql`, mirrored by `migrations/economic_sql_lifecycle_owner.sql` and fresh bootstrap.
- Behavioral fixture: `tests/async/economic_sql_lifecycle_owner_mysql_harness.cpp`, invoked by `tests/async/run_economic_sql_lifecycle_owner_mysql.sh` and `tests/async/test_economic_sql_lifecycle_owner_contract.py`.
- The harness imports fresh bootstrap, drops/replays migration 0033, enumerates native rows, verifies invalid input leaves no owner/mapping rows, and uses a receipt-checking trigger, exact retry, conflicting retry, and a concurrent guarded currency writer.

## Integration still required

- Parent must acquire/retain `economic_sql_lifecycle_guard::acquire_runtime()` before gameplay admission and release it only after shutdown. Runtime acquisition refuses a database containing a staged installation.
- Every SQL wallet/shared-bank currency writer must acquire `economic_sql_currency_writer_guard` before starting its transaction and retain it through commit/rollback. This slice does not edit `critical_command_repository.c`, economy producer sources, or game-loop boot files.
- Complete pending-intent, evidence-loss/partial-restore, and global activation recovery before installing gameplay authority. The exported mappings alone are not authorization to activate.
- Owner and guard source files are now in server build wiring. This does not wire production admission or activate accounting; full integrated builds and acceptance remain separate gates.

## Historical pre-owner RED evidence

The previous source-contract failure and its original log remain represented in repository history at the pre-owner baseline. The test path now invokes the disposable SQL behavioral suite instead of asserting source-name presence; the old RED checks are not retained as a substitute for runtime evidence.
