# Implementation Notes

**Session ID**: `phase00-session01-redacted-persistence-observability`
**Started**: 2026-08-26 23:32
**Last Updated**: 2026-08-27 02:00

---

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 21 / 21 |
| Estimated Remaining | Complete |
| Blockers | 0 |

---

## Task Log

### Task T001 - Confirm deterministic session state

**Started**: 2026-08-26 23:32
**Completed**: 2026-08-26 23:35
**Duration**: 3 minutes

**Notes**:
- Confirmed Session 01 is the current session and first executable Phase 00 candidate.
- Recorded clean baseline HEAD `0600eeba358d2bb4f70e407e320502912bd27830`.
- Confirmed no older session directory exists to archive.

**Files Changed**:
- `.spec_system/specs/phase00-session01-redacted-persistence-observability/implementation-notes.md` - Recorded session baseline and evidence.
- `.spec_system/specs/phase00-session01-redacted-persistence-observability/tasks.md` - Marked T001 complete.

**Verification**:
- Command/check: `bash .spec_system/scripts/analyze-project.sh --json`
  - Result: PASS - current session is Session 01, zero completed sessions, and Session 01 is the first candidate.
- Command/check: `git status --porcelain && git rev-parse HEAD && find .spec_system/specs -mindepth 1 -maxdepth 1 -type d`
  - Result: PASS - baseline was clean and only the current session directory existed.
- UI product-surface check: N/A - setup-only task.
- UI craft check: N/A - setup-only task.

---

### Task T002 - Reconfirm persistence execution and logging inventory

**Started**: 2026-08-26 23:35
**Completed**: 2026-08-26 23:38
**Duration**: 3 minutes

**Notes**:
- Reconfirmed 508 `qry()`/`db_query()` call sites and exactly three direct `mysql_real_query()` executors.
- Located raw SQL/error prose, `/tmp/garp-item-trace.log`, save/item traces, entity identifiers, and pointer diagnostics in the scoped files before contract changes.

**Files Changed**:
- `.spec_system/specs/phase00-session01-redacted-persistence-observability/implementation-notes.md` - Recorded inventory evidence.
- `.spec_system/specs/phase00-session01-redacted-persistence-observability/tasks.md` - Marked T002 complete.

**Verification**:
- Command/check: `rg -n '\b(qry|db_query)\s*\(' src --glob '*.[ch]' | wc -l`
  - Result: PASS - returned 508.
- Command/check: `rg -n 'mysql_real_query\s*\(' src --glob '*.[ch]'`
  - Result: PASS - found only `src/sql.c`, `src/sql_persistence_raw.c`, and `src/locker_async.c`.
- Command/check: scoped `rg` inventory for `mysql_error`, SQL/query text, `/tmp`, trace markers, and `%p`
  - Result: PASS - identified every source location targeted by T006-T010 and the later hygiene regression.
- UI product-surface check: N/A - inventory-only task.
- UI craft check: N/A - inventory-only task.

---

### Task T003 - Define persistence observability contracts

**Started**: 2026-08-26 23:38
**Completed**: 2026-08-26 23:48
**Duration**: 10 minutes

**Notes**:
- Defined source sites, execution contexts, statement kinds, redacted events, bounded latency metrics, and query/save-health snapshots.
- Added exhaustive name mappings with explicit unknown handling and fixed-size storage limits.

**Files Changed**:
- `src/persistence_observability.h` - Added the complete bounded observability API and data contracts.

**Verification**:
- Command/check: `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc -c src/persistence_observability.c -o /tmp/persistence_observability.o`
  - Result: PASS - contracts compile under C++20 with warnings as errors.
- Command/check: `git diff --check -- src/persistence_observability.h src/persistence_observability.c`
  - Result: PASS - no whitespace errors.
- UI product-surface check: N/A - internal telemetry contract.
- UI craft check: N/A - internal telemetry contract.

**BQC Fixes**:
- Contract alignment: enum sentinels and unknown fallbacks prevent unchecked context or statement values.
- Error information boundaries: event contracts contain metadata only and no SQL/value field.

---

### Task T004 - Implement bounded in-memory observability

**Started**: 2026-08-26 23:40
**Completed**: 2026-08-26 23:50
**Duration**: 10 minutes

**Notes**:
- Implemented monotonic timing, a mutex-protected fixed registry, saturating aggregation, overflow accounting, latency buckets, and deterministic snapshot sorting outside the lock.
- Implemented bounded metadata-only event formatting and a reset seam for the standalone runtime harness.

**Files Changed**:
- `src/persistence_observability.c` - Added record, snapshot, timing, classification, formatting, and reset behavior.

**Verification**:
- Command/check: standalone C++20 warnings-as-errors compilation of `src/persistence_observability.c`
  - Result: PASS - translation unit compiled cleanly.
- Command/check: `rg` for allocation, filesystem, network, MySQL, and Redis calls in the new module
  - Result: PASS - no dynamic allocation or external I/O occurs in the record path or module.
- Command/check: source inspection of mutex boundaries and sort path
  - Result: PASS - recording mutates only fixed storage under lock; snapshots copy under lock and sort after unlock.
- UI product-surface check: N/A - internal telemetry implementation.
- UI craft check: N/A - internal telemetry implementation.

**BQC Fixes**:
- Concurrency safety: all shared counters and registry entries use one scoped mutex.
- Failure path completeness: clock, buffer, unknown enum, truncation, overflow, and saturation cases are explicit.

---

### Task T005 - Add observability to the server build

**Started**: 2026-08-26 23:50
**Completed**: 2026-08-26 23:55
**Duration**: 5 minutes

**Notes**:
- Added `persistence_observability.o` to the existing server object list without changing output paths.
- The strict server profile caught a Fortify truncation warning in the first build; formatting now uses a fixed intermediate buffer and fail-closed bounded copy.

**Files Changed**:
- `src/Makefile` - Added the observability object.
- `src/persistence_observability.c` - Repaired strict-profile bounded formatting.

**Verification**:
- Command/check: `make -C src /home/aiwithapex/projects/duris-db-refactor/bin/objects/server/persistence_observability.o`
  - Result: PASS - object compiled with the repository C++20 hardening and zero-warning profile under `bin/objects/server`.
- Command/check: inspected `src/Makefile` object ordering and output variables
  - Result: PASS - no artifact path changed.
- UI product-surface check: N/A - build integration only.
- UI craft check: N/A - build integration only.

**BQC Fixes**:
- Failure path completeness: undersized output buffers now return failure with an empty output rather than exposing a truncated diagnostic.

---

### Task T006 - Route shared SQL execution through redacted observation

**Started**: 2026-08-26 23:55
**Completed**: 2026-08-27 00:10
**Duration**: 15 minutes

**Notes**:
- Added call-site macros for `qry`, `db_query`, `db_query_nolog`, and explicit trace execution while retaining their existing call syntax.
- Centralized direct execution in an observed boundary that times outside the telemetry lock, classifies statements, distinguishes parent/child processes, and emits metadata-only failures.
- Preserved result draining and removed SQL previews, query bytes, MySQL error prose, and admin error disclosure from `src/sql.c`.

**Files Changed**:
- `src/sql.h` - Added source-aware executor APIs and compatibility macros.
- `src/sql.c` - Integrated observation across shared execution and redacted execution/drain failures.

**Verification**:
- Command/check: `make -C src /home/aiwithapex/projects/duris-db-refactor/bin/objects/server/sql.o`
  - Result: PASS - shared SQL module compiled under the strict server profile.
- Command/check: `rg` for `mysql_error`, SQL/query previews, and failed-query text in `src/sql.c`
  - Result: PASS - no raw MySQL prose or SQL value logging remains.
- Command/check: `rg -n 'mysql_real_query\\s*\\(' src/sql.c`
  - Result: PASS - one direct call remains inside `sql_observed_execute_at`; wrappers and explicit transaction/trace calls route through it.
- UI product-surface check: N/A - internal SQL boundary.
- UI craft check: N/A - internal SQL boundary.

**BQC Fixes**:
- Error information boundaries: database failures expose only stable site/context/kind/duration/error-code/SQLSTATE metadata.
- Concurrency safety: telemetry recording occurs only after the external call returns.
- State freshness on re-entry: result draining remains before/after callers that require it.

---

### Task T007 - Observe worker SQL and redact worker failures

**Started**: 2026-08-27 00:10
**Completed**: 2026-08-27 00:17
**Duration**: 7 minutes

**Notes**:
- Routed raw event-worker and locker multi-statement execution through the shared observed executor with distinct worker contexts.
- Preserved result draining, poisoned pool-slot replacement, rollback, and ambiguous multi-statement failure handling.
- Replaced raw query and MySQL prose with operation ID, numeric error code, and SQLSTATE metadata.

**Files Changed**:
- `src/sql_persistence_raw.c` - Observed event-worker execution and categorical drain failure.
- `src/sql_pool.c` - Redacted connection and wait failures.
- `src/locker_async.c` - Observed locker execution and redacted rollback/result errors.

**Verification**:
- Command/check: strict-profile builds of `sql_persistence_raw.o`, `sql_pool.o`, and `locker_async.o`
  - Result: PASS - all three objects compiled with warnings as errors.
- Command/check: `rg` for direct `mysql_real_query`, `mysql_error`, failed-query previews, and first-200-byte logging in the three files
  - Result: PASS - no direct query executor or private error/query prose remains.
- UI product-surface check: N/A - internal worker execution.
- UI craft check: N/A - internal worker execution.

**BQC Fixes**:
- External dependency resilience: existing bounded pool acquisition and connection repair remain intact.
- Error information boundaries: all scoped worker failures now use categorical metadata.
- Mutation safety: partially executed locker batches are still never split or retried statement-by-statement.

---

### Task T008 - Remove private player SQL diagnostics

**Started**: 2026-08-26 23:47
**Completed**: 2026-08-27 00:43
**Duration**: 26 minutes

**Notes**:
- Replaced the query-bearing player error helper with a site-only numeric diagnostic.
- Removed the `/tmp/garp-item-trace.log` writer, raw batch/query output, test traces, item descriptions, account/player/owner names, PIDs, object IDs, and pointer values from scoped player persistence diagnostics.
- Preserved transaction ownership, rollback, load/save, container assembly, and fallback behavior; removed one now-unused logging-only parameter.

**Files Changed**:
- `src/sql_player.h` - Narrowed the error helper to a stable site label.
- `src/sql_player.c` - Removed private trace surfaces and made failures categorical.
- `src/sql.c` - Updated the multi-query failure call to the site-only helper.

**Verification**:
- Command/check: strict-profile builds of `sql_player.o` and `sql.o`
  - Result: PASS - both modules compiled with warnings as errors.
- Command/check: `rg` for trace writer/markers, `mysql_error`, `sqlerr`, raw batch output, and pointer formatting in `src/sql_player.c`
  - Result: PASS - all prohibited diagnostic patterns are absent.
- Command/check: `rg -n 'sql_player_error\\s*\\(' src/sql.c src/sql_player.c src/sql_player.h`
  - Result: PASS - declaration and every caller use site-only metadata.
- UI product-surface check: N/A - internal persistence diagnostics.
- UI craft check: N/A - internal persistence diagnostics.

**BQC Fixes**:
- Error information boundaries: player/account/item values cannot enter the reviewed SQL failure logs.
- Contract alignment: helper declaration, both implementations, and all callers were updated together.
- Resource cleanup: trace-file acquisition was removed entirely; existing allocation cleanup and rollback paths remain compiled.

---

### Task T009 - Add truthful deferred-save health state

**Started**: 2026-08-27 00:43
**Completed**: 2026-08-27 00:57
**Duration**: 14 minutes

**Notes**:
- Added first/latest pending timestamps, scheduled state, attempts, and failures to each bounded deferred-save slot.
- Callback attempts now become unscheduled before execution; a failed consumed callback remains pending and is reported as failed-unscheduled without adding Session 03 retry behavior.
- Added a bounded aggregate snapshot with saturating counts and non-negative monotonic oldest age; removed identity and pointer traces from `do_save_silent`.

**Files Changed**:
- `src/actoth.c` - Added deferred state transitions, snapshot implementation, and save trace redaction.
- `src/persistence_observability.h` - Exposed the deferred snapshot getter.

**Verification**:
- Command/check: strict-profile builds of `actoth.o` and `persistence_observability.o`
  - Result: PASS - both modules compiled with warnings as errors.
- Command/check: source transition inspection for schedule/coalesce/callback/flush/global-flush paths
  - Result: PASS - scheduled callbacks, attempts, failures, success clears, and failed-unscheduled state are explicit.
- Command/check: `rg` for `do_save_silent` trace identity, PID, and pointer patterns
  - Result: PASS - prohibited traces are absent.
- UI product-surface check: N/A - internal save state; operator rendering is T013.
- UI craft check: N/A - internal save state.

**BQC Fixes**:
- State freshness on re-entry: coalesced requests refresh latest-pending time without duplicating events.
- Failure path completeness: consumed failed events remain pending with explicit unscheduled failure state.
- Mutation safety: success remains the only path that clears a valid deferred slot.

---

### Task T010 - Enforce categorical persistence alerts

**Started**: 2026-08-27 00:57
**Completed**: 2026-08-27 01:29
**Duration**: 32 minutes

**Notes**:
- Made the persistence-alert boundary discard owner/item/event values, validate bounded categorical domain/action fields, and retain details only when every format conversion is numeric.
- Removed account/player names, confirmation-code debug output, IP/account save identities, paths, PIDs, object identifiers, descriptions, and pointers from the reviewed failure callers.
- Kept actionable failure categories and numeric errno/count/size/type/attempt/failure metadata where safe.

**Files Changed**:
- `src/utility.c` - Enforced categorical alert formatting and redacted fallback/replay paths.
- `src/account.c` - Removed private trace/debug output and account-identity failure logs.
- `src/files.c` - Redacted player/locker/fallback failures and paths.
- `src/nanny.c` - Redacted post-entry and delete failures.
- `src/modify.c` - Redacted rename persistence failures.
- `src/ws_handlers.c` - Redacted account mutation logs and alerts.
- `src/actoth.c` - Redacted deferred alerts while preserving numeric state.

**Verification**:
- Command/check: strict-profile builds of all seven touched server objects
  - Result: PASS - each module compiled with warnings as errors after one unused-parameter repair.
- Command/check: scoped `rg` for trace file, pointer formatting, owner/path fields, private persistence-alert arguments, and identity-bearing failure strings
  - Result: PASS - reviewed save failure callers are categorical; remaining worker owner labels are fixed categories.
- Command/check: inspected `persistence_alert_format_is_numeric` and category validation
  - Result: PASS - string, pointer, character, and write-count conversions are rejected; numeric metadata remains bounded.
- UI product-surface check: N/A - trusted log/alert infrastructure.
- UI craft check: N/A - trusted log/alert infrastructure.

**BQC Fixes**:
- Error information boundaries: redaction is enforced centrally even if a future caller supplies a private owner or detail string.
- Trust boundary enforcement: domain/action values must contain only bounded categorical characters.
- Failure path completeness: errno and bounded aggregate counts remain available without paths or entity values.

---

### Task T011 - Track active and inflight dirty-save age

**Started**: 2026-08-27 01:29
**Completed**: 2026-08-27 01:42
**Duration**: 13 minutes

**Notes**:
- Added a fixed 512-slot in-memory dirty metric registry with separate active and inflight first-seen timestamps.
- Wired successful marks, active-to-inflight rename, child/synchronous success deletion, failure restore/merge, and explicit clearing to the existing Redis transitions.
- Added a zero-I/O snapshot that distinguishes disabled, unavailable, active, and inflight states with non-negative monotonic ages.

**Files Changed**:
- `src/redis.h` - Exposed the dirty-save health snapshot.
- `src/redis.c` - Added bounded metric state and transition integration.

**Verification**:
- Command/check: strict-profile build of `redis.o`
  - Result: PASS - Redis module compiled with warnings as errors.
- Command/check: source transition inventory for mark/move/restore/clear/snapshot helpers
  - Result: PASS - all existing state transitions update the matching in-memory timestamp state.
- Command/check: inspected `redis_dirty_save_snapshot_copy`
  - Result: PASS - snapshot performs no Redis/network call and reports explicit availability and bounded counts/ages.
- UI product-surface check: N/A - internal metrics; operator rendering is T013.
- UI craft check: N/A - internal metrics.

**BQC Fixes**:
- State freshness on re-entry: failure restore merges the older active/inflight timestamp rather than resetting age.
- Contract alignment: successful Redis transitions and metric transitions are paired.
- External dependency resilience: unavailable snapshots remain local and do not attempt reconnection.

---

### Task T012 - Add atomic queue-health snapshots

**Started**: 2026-08-27 01:42
**Completed**: 2026-08-27 01:50
**Duration**: 8 minutes

**Notes**:
- Added independent item, scalar, and large-event health getters that copy all queue and worker state under the owning mutex.
- Each snapshot includes pending, dropped, written, failures, running, stop-pending, heartbeat availability, and clamped heartbeat age.

**Files Changed**:
- `src/persistence_queue.h` - Declared the three snapshot getters.
- `src/persistence_queue.c` - Implemented locked snapshot capture with shared value assembly.

**Verification**:
- Command/check: strict-profile build of `persistence_queue.o`
  - Result: PASS - queue module compiled with warnings as errors.
- Command/check: inspected all three getter lock/copy/unlock sequences
  - Result: PASS - each owning mutex is released before returning; callers render only copied values.
- UI product-surface check: N/A - internal metrics; operator rendering is T013.
- UI craft check: N/A - internal metrics.

**BQC Fixes**:
- Concurrency safety: related queue/worker fields are captured atomically under their existing mutex.
- State freshness on re-entry: heartbeat availability distinguishes never-started workers from an age of zero.

---

### Task T013 - Add trusted persistence status rendering

**Started**: 2026-08-26 23:46
**Completed**: 2026-08-26 23:52
**Duration**: 6 minutes

**Notes**:
- Added value 8 as the trusted-only `world persistence` branch without widening the existing untrusted `world stats` and `world zones` surface.
- Captured fresh query, queue, dirty-save, and deferred-save snapshots on each invocation, then rendered only copied metadata after snapshot locks were released.
- Bounded query output to eight deterministically ranked sites and exposed empty, disabled, unavailable, pending, failed-unscheduled, overflow, heartbeat, and aggregate oldest-save states.

**Files Changed**:
- `src/actinf.c` - Added the trusted command option and bounded metadata-only renderer.

**Verification**:
- Command/check: strict-profile build of `actinf.o`
  - Result: PASS - the command module compiled with warnings as errors.
- Command/check: inspected privilege gating and snapshot placement
  - Result: PASS - persistence remains hidden from untrusted syntax/access and all getters execute anew per command call.
- Command/check: inspected rendered fields and bounds
  - Result: PASS - output is limited to eight source-site rows and contains no player, account, item, IP, SQL, path, or pointer values.
- UI product-surface check: PASS - compact line-oriented output supports repeated operational comparison.
- UI craft check: N/A - text-only trusted diagnostic command.

**BQC Fixes**:
- Authorization continuity: reused the established trusted `world` option gate.
- State freshness on re-entry: no snapshot or rendered state is cached between command calls.
- Empty and dependency states: explicit strings distinguish no work, disabled Redis, unavailable Redis metrics, and unavailable queue heartbeats.

---

### Task T014 - Document safe persistence diagnostics

**Started**: 2026-08-26 23:52
**Completed**: 2026-08-26 23:56
**Duration**: 4 minutes

**Notes**:
- Replaced the obsolete per-query file-write description with the metadata-only `SQL_TRACE` contract and its privacy boundary.
- Documented source sites, four execution contexts, fixed registry capacity, overflow semantics, lock boundaries, and process-local operation-ID limitations.
- Added an operator guide for repeated `world persistence` snapshots and every explicit empty, disabled, unavailable, failed-unscheduled, and overflow state.

**Files Changed**:
- `docs/CONFIGURATION.md` - Defined trace enablement, safe fields, and correlation-ID scope.
- `docs/DATABASE.md` - Defined the execution/aggregation architecture and bounded semantics.
- `docs/RUNBOOK.md` - Added command usage and operational interpretation.

**Verification**:
- Command/check: reviewed all three documents against T014 and the session acceptance criteria
  - Result: PASS - each required semantic and privacy guarantee is explicit.
- Command/check: searched the diagnostics documentation for the obsolete open/append/close behavior
  - Result: PASS - the stale claim was removed.
- UI product-surface check: N/A - documentation.
- UI craft check: N/A - documentation.

**BQC Fixes**:
- Documentation-code alignment: trace and status descriptions now match the implemented metadata boundary.
- Operator failure interpretation: unavailable and failed-unscheduled states cannot be mistaken for healthy retries.

---

### Tasks T015-T017 - Add runtime, hygiene, and status regressions

**Started**: 2026-08-26 23:56
**Completed**: 2026-08-27 00:07
**Duration**: 11 minutes

**Notes**:
- Added a standalone C++20 harness covering empty snapshots, statement classification, every latency-bucket boundary, eight-thread aggregation, deterministic tie ordering, fixed-capacity overflow, saturating arithmetic, and metadata-only canary redaction.
- Added a reviewed-source inventory enforcing one observed direct MySQL boundary and rejecting raw-query previews, MySQL prose, private trace files, legacy markers, item-description traces, and pointer formatting.
- Added status-contract checks for trusted access, fresh getters, eight-row bounds, explicit states, stranded deferred work, Redis timestamp transitions, queue locking, and entity-free output.
- Test-driven inspection found and removed three leftover login `[TRACE]` blocks, one ship-ID failure line, and raw immortal SQL command audit text.

**Files Changed**:
- `tests/async/test_persistence_observability.py` - Added standalone runtime coverage.
- `tests/async/test_persistence_log_hygiene.py` - Added direct-executor and redaction source contract.
- `tests/async/test_persistence_status_contract.py` - Added operator/status source contract.
- `src/nanny.c`, `src/sql_player.c`, `src/sql.c` - Removed residual private/raw diagnostics found by the new tests.

**Verification**:
- Command/check: `python3 tests/async/test_persistence_observability.py`
  - Result: PASS - compiled and ran the strict C++20/pthread harness.
- Command/check: `python3 tests/async/test_persistence_log_hygiene.py`
  - Result: PASS - all nine inventory and privacy checks passed.
- Command/check: `python3 tests/async/test_persistence_status_contract.py`
  - Result: PASS - all nine access, freshness, state, and locking checks passed.
- UI product-surface check: PASS - source contract covers the trusted text surface.
- UI craft check: N/A - text-only diagnostics.

**BQC Fixes**:
- Boundary coverage: bucket inclusivity, overflow, saturation, concurrency, and deterministic ties are executable rather than documentary assertions.
- Privacy regression resistance: representative secret canaries and known legacy leak shapes are permanently rejected.
- Failure-state truthfulness: failed-unscheduled work and unavailable dependencies have explicit source contracts.

---

### Task T018 - Run focused persistence regression suite

**Started**: 2026-08-27 00:07
**Completed**: 2026-08-27 00:14
**Duration**: 7 minutes

**Notes**:
- Ran all three new tests and the five nearest existing SQL, deferred-save, dirty-flush, boot-log, and latency contracts.
- Updated legacy source-contract assertions to recognize the new categorical redacted messages and the truthful boolean save-result gate without weakening their transaction, retry, or clear-after-success checks.

**Files Changed**:
- `tests/async/test_sql_persistence_paths.py` - Aligned expected failure categories with redacted logs.
- `tests/async/test_deferred_save_flush.py` - Anchored on the explicit saved-result gate.
- `tests/async/test_boot_log_hygiene.py` - Aligned the deferred mapping assertion with categorical output.

**Verification**:
- Command/check: all eight T018 Python tests with fail-fast execution
  - Result: PASS - new and adjacent persistence regressions completed successfully.
- UI product-surface check: PASS - status contract included in the suite.
- UI craft check: N/A - text-only diagnostics.

**BQC Fixes**:
- Regression compatibility: tests continue to prove original transaction/retry behavior while enforcing the new privacy vocabulary.
- Truthful failure gating: direct deferred saves clear only after the explicit successful result.

---

### Task T019 - Format and build the server

**Started**: 2026-08-27 00:14
**Completed**: 2026-08-27 00:17
**Duration**: 3 minutes

**Notes**:
- Applied the repository changed-line formatter and verified the resulting C/C++ diff against `.clang-format`.
- Rebuilt and linked the complete server with the repository C++20 warnings-as-errors profile.

**Files Changed**:
- `src/actinf.c` - Formatter normalized the newly added status renderer.

**Verification**:
- Command/check: `./scripts/format.sh --check`
  - Result: PASS - changed lines match `.clang-format`.
- Command/check: `make -C src`
  - Result: PASS - all translation units compiled and `bin/server/dms_new` linked with zero warnings.
- UI product-surface check: PASS - trusted command renderer compiled in the complete binary.
- UI craft check: N/A - text-only diagnostics.

**BQC Fixes**:
- Build integration: the new observability object is present in the final link, not only standalone tests.

---

### Task T020 - Verify the trusted command in a local game

**Started**: 2026-08-26 23:47
**Completed**: 2026-08-27 00:00
**Duration**: 13 minutes

**Notes**:
- Confirmed `.env` selects a non-production database target and that all three configured game-test credentials are present without printing their values.
- Found the installed `duris-mud.service` belongs to a different checkout already using ports 7777/7778/4050; `scripts/start_mud.sh` would therefore have validated the wrong binary.
- Generated this checkout's missing combined world data, launched the freshly linked binary directly on documented development port 4000, authenticated with the configured account/character, and stopped only that exact process after testing.
- Captured two consecutive complete `world persistence` reports plus a third after ordinary `save`; query totals refreshed from 1579 to 1580 to 1599 while queue, dirty, deferred, heartbeat, overflow, and oldest-save fields remained coherent and metadata-only.

**Files Changed**:
- No tracked source changes; `make world` produced ignored runtime world data and build artifacts only.

**Verification**:
- Command/check: secret-safe `.env` classification and credential-presence probe
  - Result: PASS - local/development target and complete test credentials confirmed without disclosure.
- Command/check: local socket login and three pager-aware `world persistence` captures around `save`
  - Result: PASS - three complete reports ended at `oldest_save_age_ms`, repeated calls revalidated counts, and save activity remained functional.
- Command/check: exact port-4000 listener check after SIGINT
  - Result: PASS - the test server terminated and no port-4000 listener remains.
- UI product-surface check: PASS - trusted command is readable, bounded, repeatable, and operationally complete.
- UI craft check: N/A - text-only diagnostics.

**BQC Fixes**:
- Environment isolation: avoided invoking the unrelated checkout's configured user service or touching its running process.
- Pagination correctness: runtime automation consumed the MUD pager so each report was verified through its final oldest-save line.
- State freshness: query totals advanced independently on every status invocation and after save activity.

---

### Task T021 - Run the complete handoff gate

**Started**: 2026-08-27 00:00
**Completed**: 2026-08-27 00:08
**Duration**: 8 minutes

**Notes**:
- Normalized legacy non-ASCII punctuation in session-touched text files and verified ASCII bytes, Unix LF endings, and final newlines across all 34 session-owned changed files.
- The first full gate exposed fourteen dormant capacity claims in the now-touched `actinf.c`; converted them to the array-capacity-aware `APPENDF` helper.
- Updated the locker async source contract to require the shared observed executor and forbid a direct `mysql_real_query` bypass.
- Preserved an unrelated concurrent change in `docs/ongoing-projects/ongoing/todo.md`; it is not part of this session or its encoding assertion.

**Files Changed**:
- `src/actinf.c` - Replaced unsafe legacy append capacity claims in the touched command module.
- `tests/async/test_locker_async_pipeline.py` - Aligned the worker boundary assertion with the observed executor.
- Session-touched docs and C/C++ sources - Mechanically normalized non-ASCII punctuation to ASCII equivalents.

**Verification**:
- Command/check: `make test-all`
  - Result: PASS - complete build plus all 166 Python regressions and signal-handler checks passed.
- Command/check: `./scripts/format.sh --check` and `git diff --check`
  - Result: PASS - formatting and whitespace gates are clean.
- Command/check: byte-level session file scan
  - Result: PASS - all 34 session-owned text files are ASCII, LF-only, and final-newline terminated.
- UI product-surface check: PASS - runtime and source contracts cover access, bounds, pagination, and repeated rendering.
- UI craft check: N/A - text-only diagnostics.

**BQC Fixes**:
- Dormant memory safety: fixed all undersized `line` buffer append claims activated by the full repository scanner.
- Executor boundary enforcement: legacy locker coverage now verifies central observation rather than requiring the removed direct database call.
- Handoff completeness: full build, full regression suite, formatting, whitespace, encoding, and line endings all pass.

---
