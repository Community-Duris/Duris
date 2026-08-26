# Task Checklist

**Session ID**: `phase00-session01-redacted-persistence-observability`
**Total Tasks**: 21
**Work Window**: One cohesive privacy and telemetry change across shared SQL execution, persistence trace cleanup, bounded save-health snapshots, and the trusted operator view.
**Created**: 2026-08-26

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup (2 tasks)

- [x] T001 [S0001] Re-run deterministic analysis, confirm Session 01 remains the first executable candidate, record the current HEAD, and verify no older session specs require archiving (`.spec_system/scripts/analyze-project.sh`)
- [x] T002 [S0001] Reconfirm the source inventory of 508 `qry()`/`db_query()` calls, the three direct `mysql_real_query()` executors, raw-query error logs, and private trace writers before changing contracts (`src/sql.c`, `src/sql_persistence_raw.c`, `src/locker_async.c`, `src/sql_player.c`, `src/sql_pool.c`, `src/account.c`, `src/actoth.c`)

---

## Foundation (3 tasks)

- [x] T003 [S0001] Define compile-time query-site identities, execution contexts, process-local operation IDs, redacted diagnostic events, bounded latency buckets, query snapshots, and save-health snapshot types with exhaustive enum handling (`src/persistence_observability.h`)
- [x] T004 [S0001] Implement monotonic timing, thread-safe fixed-capacity per-site aggregation, saturating counters, deterministic snapshots, overflow accounting, and metadata-only formatting with no dynamic allocation or external I/O on the record path (`src/persistence_observability.c`)
- [x] T005 [S0001] Add the observability translation unit to the C++20 server build without changing generated artifact locations (`src/Makefile`)

---

## Implementation (9 tasks)

- [x] T006 [S0001] Route `qry()`, `db_query()`, `db_query_nolog()`, explicit trace execution, transaction commands, and child-context execution through source-site-aware observation while preserving result draining and existing operation timeouts, with explicit failure mapping and no SQL or `mysql_error()` prose in logs (`src/sql.h`, `src/sql.c`)
- [x] T007 [S0001] [P] Route raw event-worker and locker multi-statement execution through the shared observed boundary with worker context, preserved retry/repair and transaction ambiguity behavior, and redacted pool, rollback, and multi-result failures (`src/sql_persistence_raw.c`, `src/sql_pool.c`, `src/locker_async.c`)
- [x] T008 [S0001] Replace query-bearing player SQL error helpers with site-only metadata and remove raw batches, SQL prefixes, `/tmp/garp-item-trace.log`, item descriptions, names, PIDs, and pointer traces without changing save/load/transaction outcomes (`src/sql_player.h`, `src/sql_player.c`)
- [x] T009 [S0001] Remove `do_save_silent()` identity and pointer traces and add deferred-slot first/latest timestamps, scheduled state, attempts, failures, and a bounded aggregate snapshot that reports failed-unscheduled work truthfully without implementing Session 03 retry behavior (`src/actoth.c`)
- [x] T010 [S0001] Apply categorical persistence-alert redaction and remove private account, player, path, PID, description, and pointer values from the source-reviewed save failure callers while preserving actionable domain, action, operation, and numeric outcome metadata (`src/account.c`, `src/files.c`, `src/nanny.c`, `src/modify.c`, `src/utility.c`, `src/ws_handlers.c`)
- [x] T011 [S0001] [P] Track separate active and inflight dirty-save timestamps across existing mark, rename, child-success, and restore transitions without adding Redis I/O to the mutation path, and expose explicit disabled/unavailable/count/age snapshots (`src/redis.h`, `src/redis.c`)
- [x] T012 [S0001] [P] Add one locked snapshot per item, scalar, and large-event queue covering pending, dropped, written, failures, running state, stop state, and heartbeat age, with no queue lock held during rendering (`src/persistence_queue.h`, `src/persistence_queue.c`)
- [x] T013 [S0001] Add a trusted `world persistence` branch that renders bounded deterministic top query sites plus queue, dirty, deferred, oldest-save, overflow, disabled, unavailable, and empty states without entity values and with repeated-call state revalidation (`src/actinf.c`)
- [x] T014 [S0001] Document metadata-only `SQL_TRACE`, execution contexts, bounded metric semantics, `world persistence`, privacy guarantees, unavailable states, and the fact that diagnostic operation IDs are not durability IDs (`docs/CONFIGURATION.md`, `docs/DATABASE.md`, `docs/RUNBOOK.md`)

---

## Testing (7 tasks)

- [x] T015 [S0001] [P] Write a standalone C++20 runtime harness for concurrent recording, latency buckets, saturation, registry overflow, deterministic snapshots, and canary-secret redaction (`tests/async/test_persistence_observability.py`)
- [x] T016 [S0001] [P] Write a source-contract regression that inventories all direct MySQL execution sites and rejects raw SQL, SQL-prefix, MySQL error prose, private trace files, item-description traces, and pointer formatting in the reviewed persistence paths (`tests/async/test_persistence_log_hygiene.py`)
- [x] T017 [S0001] [P] Write status-contract coverage for deferred failed-unscheduled state, dirty active/inflight age transitions, queue snapshots, trusted access, bounded top-N output, deterministic ordering, and explicit disabled/unavailable/empty states (`tests/async/test_persistence_status_contract.py`)
- [x] T018 [S0001] Run the new tests plus the nearest SQL path, deferred-save, dirty-flush, boot-log, and latency regressions (`tests/async/test_persistence_observability.py`, `tests/async/test_persistence_log_hygiene.py`, `tests/async/test_persistence_status_contract.py`, `tests/async/test_sql_persistence_paths.py`, `tests/async/test_deferred_save_flush.py`, `tests/async/test_dirty_flush_retry.py`, `tests/async/test_boot_log_hygiene.py`, `tests/async/test_latency_trace_global_state.py`)
- [x] T019 [S0001] Format touched C/C++ lines, verify formatting, and compile the server with the zero-warning C++20 profile (`scripts/format.sh`, `src/Makefile`)
- [x] T020 [S0001] Confirm local/development mode without printing secrets, start the game, authenticate with the configured test account, and verify repeated `world persistence` output before and after ordinary save activity while using the standalone harness for forced failure paths (`scripts/start_mud.sh`, `.env`)
- [x] T021 [S0001] Run the full repository handoff gate and verify every created or modified text file is ASCII with Unix LF endings (`Makefile`, `.spec_system/specs/phase00-session01-redacted-persistence-observability/spec.md`, `.spec_system/specs/phase00-session01-redacted-persistence-observability/tasks.md`)

---

## Completion Checklist

- [x] All tasks marked `[x]`
- [x] All tests and checks passing
- [x] All files ASCII-encoded with LF line endings
- [x] implementation-notes.md updated
- [x] Ready for `creview` (next step in the implement -> creview -> validate sequence)

---

## Next Steps

Run the `implement` workflow step.
