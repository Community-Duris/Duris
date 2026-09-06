# PR #158 review resolution

This audit covers both [the original review](https://github.com/Community-Duris/Duris/pull/158#issuecomment-5559846352) and [the follow-up review](https://github.com/Community-Duris/Duris/pull/158#issuecomment-5559982837). Findings were checked against the actual sources; several locations, symbols, and conclusions in the follow-up do not exist at the reviewed head.

## Blockers

| Finding | Resolution and evidence |
| --- | --- |
| B1: 65th corpse dispute is dropped | Replaced the fixed PID table with a runtime field on each player. Recording a refusal needs no allocation and has no separate capacity limit. The compiled production tracker test holds 1,024 simultaneous disputes and clears them independently. |
| B2: terminal fence leaks on capture/queue refusal | An unqueued-fence guard releases the slot on all death capture/queue/enqueue failures. Failed revision marking also releases its reservation. The compiled terminal coordinator test forces capture failures for 299 distinct players, queue mismatch, enqueue refusal, ordinary terminal save afterward, and a timed-out backend. |
| B3: scopeguard process change | Removed the scopeguard clarification from the PR diff. This review remediation is explicitly authorized by the PR owner's instruction to fully address the reviews. No policy change is needed to justify it. |

## Major findings

| Finding | Resolution and evidence |
| --- | --- |
| M1: successful corpse transfers are quarantined | Refuted. Both backends quarantine only remaining active custody owned by the dying player. Successfully transferred rows have corpse ownership. The SQL death harness explicitly retains an active corpse-owned row and another player's row while quarantining the player's disputed root and durable-only descendant. Narrowing to live observations would miss that descendant, which caused the original refusal. |
| M2: admission continuation trusts stale topology | The continuation verifies its original outer object UID and location kind, actor/room accessibility, admitted source identity, and active container custody before submitting pickup. Compiled tests move the container to the actor, another player, and another room during admission and verify no pickup submission. Existing SQL/flat-file admission tests reject durable UID collisions. |
| M3: live pile merge bypasses durable amount checks | Refuted. SQL loads the custody row under `FOR UPDATE`; flat-file reads it under the authority lock. Both compare the stored payload's amounts with `endpoint.before` and reject `ESTALE` before commit. Existing backend tests verify unchanged wallets, pile values, revisions and ledgers on stale input. Runtime item-busy checks also prevent overlapping pile publication. |
| M4: orphan flat-file death evidence | Death bytes are now an after-image in the same authority transaction as quarantine and the player snapshot. The fault harness verifies no death file exists after a pre-commit failure, interrupts after the first after-image, and verifies recovery/replay completes all three effects exactly once. |
| M5: death migration verifier accepts damaged tables | Verifies exact ordered columns, data types, unsignedness, nullability, defaults, binary width, timestamp precision, engine, collation, and primary/secondary index signatures. Both-engine tests damage wallet types/nullability, add a column, remove indexes/primary key, and change engine; reapplying `CREATE IF NOT EXISTS` cannot make verification pass. |
| M6: offline death-schema blind spot | Added an explicit offline column/index/engine definition contract with mutation tests. Post-baseline tables are discovered from immutable migrations instead of hardcoded exclusions. The sealed 170-table baseline remains intact; the current runtime inventory now contains all 177 tables, including kingdom garrison. Both engine fingerprints were measured again and runtime drift tests pass. |
| M7: wrong existing coin column blessed | Both guarded SQL entry points now signal on incompatible type, nullability, default or ordinal position. The coin verifier pins ordinal 10 and is tested against BLOB NOT NULL, TEXT and misplaced MEDIUMBLOB columns on both engines. Standalone fresh bootstrap, legacy ledger installation and immutable upgrade remain supported entry points; all enforce the same `MEDIUMBLOB NULL AFTER state` contract. |
| M8: flat-file death store absent from lifecycle | Registered `file:player-deaths` as protected recovery evidence. SQL and file copies have explicit indefinite retention until recovery is resolved and the controller approves a purge horizon. No automatic purge is appropriate while these may be the only payloads proving owed assets. Export/disclosure decisions remain pending; technical retention does not invent controller approval. The archive scanner now includes migrations 0009 and 0011. |
| M9: behavior/durability coverage | Existing compiled capture, codec, currency coordinator, flat-file repository and real SQL suites already exercise backend behavior beyond source contracts. Added real SQL connection termination during the pile update after wallet debit, rollback verification, and same-operation replay; flat-file after-image interruption; fence-capacity failures; event-admission failure; and bounded publication abandonment. The journey inspects stored evidence at the account menu, verifies no unfinished authority transaction, asserts refused-death coin-value conservation, and compares evidence after restart/re-entry. Death and full currency/loader suites run on both engines. |
| M10: fixture caveats and skipped death branch | The reviewed head already ran both `run_journey(binary)` and `run_journey(binary, reset_coins=True)`; the follow-up's claim that only the latter ran is false. Both reconnect and boon caveats were already in the PR body. Expanded the reset-coin leg to include ordinary recovery and disputed death too. This exposed and fixed `create_money` adding nonempty prototype amounts to wallet conversions; conversions now use exactly the requested amounts, with sanitizer-backed regression coverage. Acceptance remains **a character reconnected after creation, with optional boons disabled**. Fresh-character bank-revision initialization and boon-enabled combat interactions are outside this acceptance claim. |

## Original minor findings

| Finding | Resolution and evidence |
| --- | --- |
| N1: item schema column count | Includes `coin_payload`, expects 56 matching columns, and independently requires exactly 56 total columns across the six tables, rejecting extras. |
| N2: new rejection of negative `add_coins` deltas | Refuted: master already rejects negative arguments. Audited all callers: zero-delta rerendering, nonnegative pile merges, validated put amounts, and pile creation. The compiled arithmetic test covers negative input without mutation. `LOG_EXIT` is a log filename, not an unconditional process-exit function. |
| N3: large pile display truncation | Refuted: `%d` is reached only for totals 2–5. Larger amounts select named descriptions first. The arithmetic harness creates four `INT_MAX` denominations and verifies the mountain description and correct composition. |
| N4: global object scan | Confirmed O(number of live objects). UID resolution must also find unattached conversion piles; a container-only search would miss them. This runs on the game thread, as do live-object mutations, so no cross-thread lock is needed. Retained as a bounded-frequency death-conversion cost rather than adding a second object index. |
| N5: process-local dispute state | The flag is runtime-only. A crash before a death snapshot reaches the journal/authority is outside the durable-disposition guarantee; recovery starts from the last durable player/custody state. After journal admission, existing replay applies the immutable death record. The flag itself does not survive restart and is not represented as durable evidence. |
| N6: NPC conversion result ignored | Both special replacement-death paths now abort the replacement and extract the newly allocated replacement mob when `money_to_inventory` fails, preserving the original NPC and wallet for normal death handling. NPC conversion is synchronous and bypasses player permission/busy checks; malformed wallet values or unavailable coin prototype can still refuse it. |
| N7: MySQL loader subset | Removed the wrapper's MySQL-only `PLAYER_LOAD_REAL_ONLY` shortcut. The disposable schema uses real fixture tables so MySQL can reopen them in batched UNION queries. Ordinary configured-database harness usage retains connection-local temporary tables; regular fixtures require the explicit disposable flag and exact synthetic schema name. Both engines pass the complete matrix. |
| N8: load query ceiling by construction | Replaced the new ceiling-literal assertion with an owner-scoped, single batched coin-payload-query assertion outside the item loop. The real loader matrix verifies constant query counts with empty, item and pet inventories. |
| N9: 0.992-second sample | Historical measurement is labeled `n=1, isolated flat-file`, not an SLA. The journey reports each new observation with the same scope. The compiled terminal coordinator deterministically withholds ACKs, reaches `timed_out`, and verifies no false durability result; caller retention and the event fallback are separately tested. |
| N10: kingdom garrison omitted | Added migration 0009 to lifecycle/archive schema scanning, registered `kingdom_garrison`, and added it to current runtime metadata and missing-table rejection. No modification to the sealed baseline. |

## Follow-up additions and corrections

- **New gap 1, publication retry:** the claim that every callback failure permanently locks the player is incorrect: `currency_transaction_player_busy` already ignores a coin entry after wallet publication. The pile/context could nevertheless retry forever and consume capacity. Publication callbacks now get at most eight attempts, followed by an `EOWNERDEAD` cleanup notification, an operation-ID alert and retirement from pending. Bulk-get context is cleared. Already committed money is never reported as a rejected debit or refunded; custody and the durable command remain recovery evidence. Tests prove temporary recovery, permanent abandonment, unchanged committed wallet, cleared busy state and no further callbacks over 100 pulses.
- **New gap 2, missing parent outbox:** added a parent audit receipt containing both child operation IDs in the same SQL transaction. Children retain their existing publication responsibilities. Reconciliation now sees an outbox row for every committed coin parent; operation replay does not duplicate it. Receipt delivery and malformed receipt rejection are tested.
- **New finding 3, event scheduling failure:** retain fallback corpse UID, due time and delay directly on the live player when event admission fails. The game pulse invokes the recovery callback once due; rescheduling success clears fallback state. Compiled tests force repeated admission failure followed by success while the character remains dead and retained. The event allocator itself does not return an allocation-failure status; the fallback still covers explicit scheduling refusals.
- **Follow-up minor 5, health counter race:** unsubstantiated. Currency pending state and health are owned by the game-thread coordinator; no worker calls the health accessor. Adding atomics to just counters would not make concurrent access to `pending` safe and is not needed for the actual ownership model.
- **Follow-up minors 6 and 7:** `corpse_transfer_dispute_count` and `coin_command_status` do not exist. There is no demonstrated counter sanitization or redundant-status-mapping defect to repair.
- **Follow-up minor 8, death-file log rotation:** death files are immutable recovery records, not rotating logs. Their deliberate retention is now registered as described under M8.
- **Follow-up minor 9, fallback error propagation:** inspected `read_legacy_coin` and coin apply. Player/room/corpse/locker routes consistently distinguish I/O (`EIO`), absence (`ENOENT`), duplicate/conflicting rows (`EMSGSIZE`) and malformed payload (`EBADMSG`). Decode failures stop the transaction. No specific contradictory branch was supplied or found.
- **Follow-up minor 10, unused debug format strings:** no debug/printf/TRACE strings exist in `critical_command_repository.c` at the reviewed or remediated head. No removal is warranted.

## Validation

The database repository suites, both runtime compatibility engines, the full isolated database gate, and focused compiled regressions passed locally. The PR check list reports the final complete-build/regression result. Reproduce the validation with:

```bash
make -C src -j4
make test-all -j4 TEST_JOBS=4
make test-db
./scripts/format.sh --check
git diff --check
python3 tests/async/test_coin_custody_lifecycle.py
python3 tests/async/test_currency_input_queue.py
python3 tests/async/test_death_item_custody_contract.py
python3 tests/async/test_player_save_pipeline.py
python3 tests/async/test_player_snapshot_capture.py
python3 tests/async/test_flatfile_player_repository.py
python3 tests/async/test_flatfile_combat_journey.py
bash tests/async/run_critical_command_schema_mysql.sh
CURRENCY_DB_IMAGE=mariadb:10.11 bash tests/async/run_currency_transaction_schema_mysql.sh
CURRENCY_DB_IMAGE=mysql:8.0 bash tests/async/run_currency_transaction_schema_mysql.sh
DEATH_DISPOSITION_DB_IMAGE=mariadb:10.11 bash tests/async/run_player_death_disposition_mysql.sh
DEATH_DISPOSITION_DB_IMAGE=mysql:8.0 bash tests/async/run_player_death_disposition_mysql.sh
RUNTIME_DB_IMAGE=mariadb:10.11 bash tests/async/run_runtime_compatibility_mysql.sh
RUNTIME_DB_IMAGE=mysql:8.0 bash tests/async/run_runtime_compatibility_mysql.sh
```

No production migration, deployment, or production gameplay test was performed. Existing deployments of this unmerged branch need a disposable rebuild of their migration test databases when testing the revised 0010/0011 checksums; do not edit a live migration ledger to bypass checksum validation.
