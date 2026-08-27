# Session Specification

**Session ID**: `phase00-session01-redacted-persistence-observability`
**Phase**: 00 - Correctness and Immediate Lag Removal
**Status**: Not Started
**Created**: 2026-08-26
**Base Commit**: `0baa498df78b9d40f99247c76cc96ccc3039b5e9`
**Work Window**: One privacy and telemetry boundary covering the three MySQL execution entry points, persistence trace cleanup, bounded save-state metrics, and one trusted operator view. The window ends when redaction and metric semantics are verified together without changing durability policy.

---

## 1. Session Overview

This session makes persistence failures diagnosable without copying SQL text, bound
values, player data, or pointer values into logs. It is first because Sessions 02, 03,
05, 06, and 08 depend on trustworthy failure and timing evidence while they change hot
paths, save behavior, Redis recovery, and connection policy.

The implementation will place a bounded, thread-safe observability layer around the
existing query executors. The 508 `qry()` and `db_query()` call sites will receive
compile-time source identities through their shared wrappers, while the only three
direct `mysql_real_query()` paths in `src/sql.c`, `src/sql_persistence_raw.c`, and
`src/locker_async.c` will use the same recording contract. The same work window removes
the known ad hoc save and item traces and exposes aggregate query, queue, dirty-save,
and deferred-save health through a trusted `world persistence` view.

No schema, save ordering, retry policy, or gameplay rule changes in this session.
Failed deferred saves and Redis dirty saves are reported according to their actual
current state; Sessions 03 and 06 own their behavioral repair.

---

## 2. Objectives

1. Ensure MySQL execution failures and opt-in SQL tracing emit only stable source site,
   process-local operation ID, execution context, statement kind, duration, numeric
   error metadata, and result state.
2. Record bounded per-site query counts, failures, and latency buckets for main-thread,
   worker, and fork-child execution without storing SQL text or allocating on the query
   path.
3. Remove the unconditional `/tmp/garp-item-trace.log`, save, item-load, and pointer
   traces identified by the source review, and prevent persistence alerts from
   forwarding private owner or detail values.
4. Report queue health, active and inflight dirty-save age, deferred-save scheduling and
   failure state, and the oldest pending save through one trusted diagnostics surface.
5. Prove redaction, bounded metrics, context classification, age transitions, and
   operator output with focused runtime and source-contract regressions.

---

## 3. Prerequisites

### Required Sessions

- None. This is the first executable session in Phase 00.

### Required Tools Or Knowledge

- C++20 and the repository `g++` warning profile
- MySQL/MariaDB C API result and error semantics
- POSIX monotonic clocks and pthread synchronization
- Python 3 source-contract and standalone C++ runtime test patterns under `tests/async/`

### Environment Requirements

- Use `make -C src` and `./scripts/format.sh --check` for server verification.
- Any game-level runtime check must first confirm that `.env` selects local/development
  operation without printing its contents, then start through `scripts/start_mud.sh`.
- Do not run migrations, database write tests, fault injection, or operational scripts
  against production. This session requires no migration.

---

## 4. Scope

### In Scope (MVP)

- A server developer can identify every shared-wrapper query by a compile-time source
  site and every direct query by an explicit semantic site, without manually editing
  all 508 wrapper call sites.
- An operator can correlate a failed database operation by process-local operation ID,
  execution context, statement kind, duration, error number, and SQLSTATE without SQL
  text, server error prose, or bound values.
- An operator can inspect bounded per-site query totals and latency buckets, with an
  explicit overflow counter when the site registry is full.
- An operator can inspect item, scalar, and large-event queue counts and failures,
  Redis active/inflight dirty counts and ages, deferred-save pending/scheduled/failed
  counts and ages, and one truthful oldest-save age.
- A maintainer can enable `SQL_TRACE` without causing SQL previews, private values, or
  synchronous per-query file writes.
- Persistence paths remove the reviewed `/tmp/garp-item-trace.log`, `[TRACE]`,
  `[real-persistence-test]`, raw-query, SQL-prefix, and pointer diagnostics while
  retaining categorical failure evidence.

### Outside This Work Window

- Deferred-save retry and bounded backoff - Reason: Session 03 owns the save state
  machine and terminal safety behavior.
- Redis command deadlines, reconnect policy, dirty-set recovery repair, and child
  supervision - Reason: Session 06 owns Redis failure containment.
- Prepared-statement conversion or typed repositories for all queries - Reason: this
  session instruments the current execution boundary without redesigning 508 callers.
- Connection target, TLS, connect-timeout, and session-invariant enforcement - Reason:
  Session 08 owns the runtime connection trust boundary.
- Revision-gap, last-durable-revision, journal-age, and exact acknowledgement metrics -
  Reason: those values do not exist until the Phase 01 save pipeline.
- Conversion of the legacy raw-SQL fallback record format - Reason: this session stops
  application logs from echoing payloads; typed journal replacement is later work.
- External dashboards, production alert routing, and the 200-player load gate - Reason:
  later phases consume the safe local metrics established here.

---

## 5. Technical Approach

### Architecture

Create `src/persistence_observability.h` and
`src/persistence_observability.c` as a narrow process-local telemetry boundary. The
module will own monotonic operation IDs, monotonic duration measurement, a fixed-capacity
per-site registry, latency buckets, overflow accounting, redacted diagnostic formatting,
and immutable snapshot APIs. It will never retain a query pointer, query bytes, bound
values, MySQL error prose, player identity, or filesystem path.

Change `qry()` and `db_query()` into source-site-aware wrapper macros backed by real
functions so their existing callers acquire reproducible `file:function:line` labels
without a repository-wide call-site rewrite. Explicit `sql_trace_exec()` users retain
semantic labels. Route the direct worker executors in `src/sql_persistence_raw.c` and
`src/locker_async.c` through the same recorder with worker context while preserving
result draining, transaction ambiguity handling, pool repair, and current read/write
timeouts. Forked child connections must be distinguishable from the game-thread
context. `SQL_TRACE` remains opt-in but becomes a sanitized event stream, not a query
preview.

Add observation-only state to the deferred-save and Redis dirty-save coordinators.
Deferred slots track first scheduling time, latest request time, whether a callback is
still scheduled, attempt count, and failure count. Redis tracking maintains separate
aggregate timestamps for the active and inflight sets and moves or merges those times
at the existing rename, success, and restore boundaries. It must add no Redis command
to `mark_player_dirty()`. Existing queue counters are exposed through one locked
snapshot per queue. The trusted `world persistence` branch renders bounded,
deterministically ordered aggregates and explicit disabled, unavailable, empty, and
overflow states; it never displays entity IDs or values.

### Design Patterns

- Shared execution boundary: Instrument the three actual `mysql_real_query()` entry
  points instead of duplicating logging policy in callers.
- Compile-time source identity: Use source-site macros for wrapper calls and explicit
  semantic IDs for direct executors; never derive identity from SQL text.
- Bounded in-memory registry: Use fixed storage and an overflow counter so telemetry
  cannot become an outage amplifier.
- Snapshot under lock: Copy metrics while holding a short mutex, then sort and render
  after releasing it; never hold a telemetry lock across MySQL, Redis, file, or player
  work.
- Monotonic age tracking: Measure durations and pending age with monotonic time and
  preserve the older timestamp when failed dirty sets merge.
- Fail-closed redaction: If metadata cannot be classified safely, emit `unknown` or
  `redacted`, never the original value.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/persistence_observability.h` | Stable site, context, redacted event, metric snapshot, and save-health contracts | ~160 |
| `src/persistence_observability.c` | Bounded thread-safe registry, operation IDs, latency buckets, snapshots, and redacted formatting | ~320 |
| `tests/async/test_persistence_observability.py` | Standalone runtime coverage for redaction, concurrency, bounds, latency, and context aggregation | ~240 |
| `tests/async/test_persistence_log_hygiene.py` | Source-contract inventory preventing raw SQL, SQL prefixes, private traces, and pointer logging | ~180 |
| `tests/async/test_persistence_status_contract.py` | Source and runtime contracts for dirty, deferred, queue, and operator status snapshots | ~200 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `src/Makefile` | Compile and link the observability module | ~2 |
| `src/sql.h` | Declare observed executors and source-site wrapper macros | ~45 |
| `src/sql.c` | Record main/child query timing and failures; remove SQL previews and raw error prose | ~190 |
| `src/sql_persistence_raw.c` | Route worker raw execution through redacted observed execution while retaining connection repair | ~70 |
| `src/sql_pool.c` | Emit redacted connection failure metadata without MySQL error prose | ~25 |
| `src/locker_async.c` | Observe multi-statement worker execution and sanitize rollback/result errors | ~65 |
| `src/sql_player.h` | Replace query-bearing error declarations with site-only diagnostics | ~10 |
| `src/sql_player.c` | Remove query-bearing errors and ad hoc item/save/load traces while preserving transaction and result behavior | ~180 |
| `src/actoth.c` | Remove save traces and expose truthful deferred-save timestamps, scheduling state, attempts, and failures | ~150 |
| `src/account.c` | Remove the unconditional account load trace writer and private trace values | ~25 |
| `src/utility.c` | Apply categorical persistence-alert redaction and remove pointer-valued diagnostics | ~90 |
| `src/files.c` | Remove private owner/path values from player and locker persistence failure alerts | ~45 |
| `src/nanny.c` | Use categorical save-failure metadata instead of player identity | ~10 |
| `src/modify.c` | Use categorical rename-save failure metadata instead of player/account identity | ~15 |
| `src/ws_handlers.c` | Use categorical account persistence failure metadata instead of account identity | ~20 |
| `src/redis.h` | Declare active/inflight dirty-save metric snapshots | ~20 |
| `src/redis.c` | Track active and inflight dirty timestamps at existing state transitions without extra mark-path I/O | ~120 |
| `src/persistence_queue.h` | Declare atomic queue-health snapshot structures and getters | ~35 |
| `src/persistence_queue.c` | Copy queue counters and worker state under the owning mutexes | ~75 |
| `src/actinf.c` | Add the trusted, bounded `world persistence` diagnostics branch | ~120 |
| `docs/CONFIGURATION.md` | Document sanitized `SQL_TRACE` semantics and privacy guarantees | ~25 |
| `docs/DATABASE.md` | Document observed execution contexts, metric bounds, and retained durability behavior | ~35 |
| `docs/RUNBOOK.md` | Document `world persistence`, empty/unavailable states, and operator interpretation | ~45 |

---

## 7. Success Criteria

### Functional Requirements

- [ ] Every `qry()` and `db_query()` invocation receives a compile-time source site,
      and every direct `mysql_real_query()` path records an explicit site and execution
      context.
- [ ] Database failure events include site, process-local operation ID, context,
      statement kind, duration, numeric error code, and SQLSTATE, but no query bytes or
      MySQL error prose.
- [ ] `SQL_TRACE` emits sanitized metadata only and performs no per-query file open,
      append, close, or query-preview work.
- [ ] The reviewed `/tmp/garp-item-trace.log`, save/item `[TRACE]`,
      `[real-persistence-test]`, query-prefix, and pointer diagnostics are absent.
- [ ] `world persistence` reports query, queue, dirty-save, and deferred-save totals,
      failures, ages, disabled/unavailable states, and telemetry overflow without player,
      account, item-description, IP, path, or pointer values.
- [ ] A deferred save that has failed and has no scheduled callback is reported as such;
      this session does not falsely label it as retrying.
- [ ] Active and inflight dirty ages move, clear, or merge with the existing Redis set
      transitions and do not add an external call to the mutation path.

### Testing Requirements

- [ ] Runtime unit tests prove a canary SQL string and representative password,
      confirmation, IP, description, and maximum-length values never enter diagnostics.
- [ ] Concurrent main/worker recording, fixed-capacity overflow, empty state, failed
      operation, and latency-bucket boundaries pass.
- [ ] Source-contract tests cover all three direct MySQL query entry points and reject
      reintroduction of raw query, SQL-prefix, trace-file, and pointer logging.
- [ ] Existing SQL persistence, deferred-save, dirty-flush, boot-log, and latency-trace
      regressions pass.
- [ ] A local game run renders `world persistence` repeatedly without exposing entity
      values or changing save state.

### Non-Functional Requirements

- [ ] Successful query observation performs no filesystem or network I/O, no dynamic
      allocation, and no telemetry lock is held across a database call.
- [ ] Site storage and operator output are bounded; excess sites increment an explicit
      overflow counter and top-site output uses deterministic ordering.
- [ ] Monotonic counters saturate instead of wrapping into misleading values, and age
      calculations never become negative.
- [ ] Process-local diagnostic operation IDs are documented as correlation IDs, not
      durable idempotency IDs.
- [ ] Existing MySQL result draining, transaction failure state, pool repair, and
      worker retry behavior remain unchanged.

### Quality Gates

- [ ] All files ASCII-encoded
- [ ] Unix LF line endings
- [ ] Code follows project conventions
- [ ] Changed C/C++ lines pass `./scripts/format.sh --check`
- [ ] `make -C src` passes with the existing C++20 warning profile
- [ ] Focused tests and `make test-all` pass
- [ ] The admin diagnostics surface contains operator-facing copy only and is access
      controlled at the existing trusted `world` command boundary

---

## 8. Implementation Notes

### Working Assumptions

- `world persistence` is the narrow operator surface: `src/actinf.c` already owns
  trusted `world` diagnostics, values above `WORLD_ZONES` are privilege-gated, and
  value 8 is unused. This keeps local save health visible even when Redis is disabled.
- Compile-time `file:function:line` identities are sufficient stable query-site labels
  for a specific build. The repository has 508 wrapper calls but only three direct
  MySQL execution points, so wrapper macros provide complete coverage without a risky
  manual rewrite.
- Query operation IDs are process-local telemetry only. Durable command identity is a
  separate Phase 01/02 contract and will not be implied by logs or documentation.
- In-memory metric changes require no schema or migration because all requested Phase
  00 observability can be reconstructed after restart.

### Conflict Resolutions

- `CONSIDERATIONS.md` says Phase 00 has no session stubs and directs `phasebuild`, while
  the authoritative analyzer reports ten current-phase candidates and the Phase 00
  tracker contains all ten stubs. The analyzer and present files show that phasebuild
  has completed in the current worktree, so planning proceeds with Session 01.
- `docs/CONFIGURATION.md` describes `SQL_TRACE` as SQL trace output, while the PRD and
  P00-S01 prohibit raw SQL and bound values in diagnostics. The privacy requirement
  wins: the switch remains opt-in but its output becomes metadata-only.

### Key Considerations

- Preserve the existing main connection, pool ownership, result draining, transaction
  state, and poisoned-connection repair behavior while changing only observation and
  logging.
- Do not use raw SQL text as a metric key, fallback alert, error detail, or debug aid.
- Keep all operator metrics aggregate and bounded; do not expose PIDs or owner names to
  make age reporting easier.
- Treat Redis-disabled, Redis-unavailable, no-sample, registry-overflow, and failed
  deferred-save states as distinct observable outcomes.

### Potential Challenges

- Macro coverage can interfere with executor definitions: implement real `*_at`
  functions and confine compatibility macros to public headers so definitions remain
  unambiguous.
- MySQL error strings can echo bound values: use numeric `mysql_errno()` and fixed
  SQLSTATE only, including pool, transaction, rollback, and multi-result errors.
- Dirty-set age can become false after rename or restore: maintain separate active and
  inflight timestamps and merge with the older non-zero value on failure.
- Metrics can create contention: capture start time before execution, record only after
  execution, copy under short locks, and sort/render outside locks.

### Relevant Considerations

- [P00] **Failure logging can disclose private data**: This session removes query text,
  SQL prefixes, ad hoc traces, identities, and pointers from the reviewed persistence
  diagnostics.
- [P00] **Capacity telemetry is incomplete**: Bounded per-site timing plus oldest save
  age establishes the safe baseline used by later remediation and load work.
- [P00] **External I/O can stop the simulation**: Observation remains in memory on the
  successful path and introduces no file or network call.
- [P00] **Trace code before trusting architecture prose**: Coverage is based on the 508
  wrapper calls and three direct MySQL execution paths found in the current source.
- [P00] **Use focused source-contract regressions**: New tests lock down both runtime
  redaction and the complete execution-site inventory.

### Behavioral Quality Focus

Checklist active: Yes
Top behavioral risks for this session:
- A failure-only branch reintroduces SQL text or a bound private value even though the
  normal trace path is redacted.
- Instrumentation holds a lock or performs file work across an external call and adds
  game-thread latency during dependency slowdown.
- Dirty or deferred timestamps are cleared at the wrong transition and make an outage
  appear healthier than it is.

---

## 9. Testing Strategy

### Unit Tests

- Compile `src/persistence_observability.c` in a standalone C++20 harness and verify
  operation IDs, contexts, latency buckets, saturating counters, fixed-capacity
  overflow, concurrent recording, snapshots, and reset behavior.
- Feed the redacted formatter canary strings representing SQL, password hashes,
  confirmation tokens, IP addresses, descriptions, pointer text, control characters,
  and maximum-length input; assert only categorical metadata reaches captured logs.
- Exercise deferred and dirty metric transition helpers for initial enqueue, coalesce,
  callback attempt, failed unscheduled state, active-to-inflight rename, newer active
  work during inflight, success clear, and failure merge.

### Integration Tests

- Verify `qry()` and `db_query()` macro expansion reaches observed executors and all
  direct `mysql_real_query()` uses are confined to the shared observed boundary.
- Run `test_sql_persistence_paths.py`, `test_deferred_save_flush.py`,
  `test_dirty_flush_retry.py`, `test_boot_log_hygiene.py`, and
  `test_latency_trace_global_state.py` with the new focused tests.
- Run `make -C src`, changed-line formatting checks, and `make test-all`.

### Runtime Verification

- Confirm local/development mode without printing `.env`, start with
  `scripts/start_mud.sh`, authenticate through the configured test account, and invoke
  `world persistence` before and after ordinary save activity.
- Verify repeated reads are bounded, stable, privilege-gated, explicit when Redis or
  samples are unavailable, and contain no player/account identity or query value.
- Use the standalone failure harness for forced database-error redaction so runtime
  verification does not require destructive database or production operations.

### Edge Cases

- Database not initialized, null connection, empty query, formatting failure, and
  overlong formatted query
- No metric samples, registry full, counter saturation, zero or very long duration,
  and concurrent main/worker updates
- SQL failure followed by partial multi-result drain and connection replacement
- Redis disabled, null context, active set only, inflight set only, simultaneous active
  and inflight work, failed child restore, and successful child completion
- Deferred table empty, full, coalesced request, missing character, failed callback,
  successful flush, and failed global flush
- Operator output larger than one local buffer and deterministic top-site ties

---

## 10. Dependencies

### Other Sessions

- Depends on: None
- Depended by: `phase00-session02-in-memory-epic-bonus-hot-path`,
  `phase00-session03-save-failure-retry-and-terminal-safety`,
  `phase00-session05-combat-artifact-persistence-correctness`,
  `phase00-session06-redis-failure-and-recovery-containment`, and
  `phase00-session08-runtime-connection-trust-boundaries`

---

## Next Steps

Run the `implement` workflow step to begin implementation.
