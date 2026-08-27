# Code Review: Redacted Persistence Observability

**Reviewed**: 2026-08-27  
**Base commit**: `0baa498df78b9d40f99247c76cc96ccc3039b5e9`  
**Result**: RESOLVED

## Scope

The review covered every change from the session's recorded base commit, including the three planning commits made after that base and all untracked implementation files. The pre-report surface contained 83 tracked files (49 added, 34 modified) and six untracked files. This report is the only additional file created by the review itself.

Reviewed groups:

- `.spec_system/CONSIDERATIONS.md`, `.spec_system/CONVENTIONS.md`, `.spec_system/SECURITY-COMPLIANCE.md`, `.spec_system/state.json`, the root PRD, four phase PRDs, and all 44 phase 00-03 session plans.
- Current-session `spec.md`, `tasks.md`, and untracked `implementation-notes.md`.
- `README.md`, `docs/CONFIGURATION.md`, `docs/DATABASE.md`, `docs/RUNBOOK.md`, `docs/VERSIONING.md`, and `docs/ongoing-projects/ongoing/todo.md`.
- `src/Makefile`, `account.c`, `actinf.c`, `actoth.c`, `files.c`, `locker_async.c`, `modify.c`, `nanny.c`, `persistence_queue.{c,h}`, `redis.{c,h}`, `sql.{c,h}`, `sql_persistence_raw.c`, `sql_player.{c,h}`, `sql_pool.c`, `utility.c`, and `ws_handlers.c`.
- Untracked `src/persistence_observability.{c,h}`.
- Existing touched async contracts: `test_boot_log_hygiene.py`, `test_deferred_save_flush.py`, `test_locker_async_pipeline.py`, and `test_sql_persistence_paths.py`.
- Untracked async contracts: `test_persistence_log_hygiene.py`, `test_persistence_observability.py`, and `test_persistence_status_contract.py`.

## Findings

### Critical

None.

### High

None outstanding.

### Medium - resolved

1. Query snapshot ordering was not a total order when records shared a source site. Concurrent first insertion could therefore change equal-count output. The comparator now breaks ties by total calls, source site, execution context, and query kind; the runtime harness covers the tie case.
2. SQL process context used a function-local PID initialized on first execution. If a forked child executed first, it could be labeled as the main process. The main PID is now captured at process load and checked by the centralized executor; a source contract protects it.
3. `sql_trace_exec` discarded its caller-provided semantic label. The executor now constructs the observed source site from the source file, semantic label, and line number; a source contract protects the mapping.
4. The trusted `world persistence` report omitted latency buckets even though the session requires bounded latency visibility. Each top-site row now includes all eight fixed bucket counts, and the command contract verifies them.
5. Dirty-registry disabled and unavailable states hid locally known active/inflight counts and ages. The report now keeps the explicit state while still rendering bounded local measurements.

### Low - resolved

1. A deferred-save comment claimed the slot was cleared before the save, while the implementation correctly clears only on success. The comment now describes the actual retry-preserving behavior.
2. The T020 implementation-note start time contradicted the recorded runtime chronology. It was corrected without changing task evidence.
3. The concurrent `todo.md` addition lacked its final newline. Its content was preserved and only the text-file terminator was restored.

## Deliberate Decisions

- The planning-only commits introduce no runtime behavior. Their 44 session files were checked for the required headings, session identity, uniqueness, and phase coverage; no speculative implementation was added during this session.
- The unrelated `todo.md` content is user-owned concurrent work. Only its missing final newline was repaired.
- A websocket bind conflict on port 4050 belongs to the separately running `/home/aiwithapex/projects/duris` checkout. Runtime verification for this checkout used its game listener on port 4000 and did not stop or alter the other service.
- The fixed-capacity registries intentionally avoid allocation in observation paths. Query overflow is reported explicitly; the dirty registry capacity exceeds the 200-player target established by the phase plan.

## Behavioral Review

- Query text, SQL errors, player names, account names, email addresses, tokens, and object descriptions do not cross the persistence logging boundary.
- All direct SQL execution routes through the source-aware observed executor; the only `mysql_real_query` call is the centralized call in `src/sql.c`.
- Operation IDs, fixed latency buckets, execution context, categorical outcomes, queue state, deferred-save state, and dirty-registry state remain bounded and deterministic.
- The trusted command remains behind the existing privileged `world` command gate and was exercised twice consecutively and again after a save.
- Failed deferred saves remain scheduled, terminal paths expose failure, and Redis dirty bookkeeping does not falsely claim success.

## Verification Evidence

- `bash .spec_system/scripts/analyze-project.sh --json` - PASS; phase inventory resolves to 10 + 8 + 12 + 14 sessions.
- Planning structure audit - PASS; 44 session plans, 44 unique session IDs, zero missing required sections.
- `./scripts/format.sh --check` - PASS.
- `make -C src` - PASS with warnings treated as errors by the repository build.
- Focused persistence observability, log hygiene, status contract, and deferred-save regression tests - PASS.
- `make test-all` - PASS; 166/166 Python async/source-contract tests plus signal-handler checks.
- `git diff --check` - PASS.
- Byte-level review-surface scan - PASS; all 89 pre-report files are ASCII with LF endings and final newlines.
- Local game verification using the configured test account - PASS; consecutive complete reports and the post-save report rendered without sensitive data.

The repository has no separately configured linter or static type-checker beyond its compiler/build, formatting check, source-contract tests, and Python regression suite.

## Conclusion

All review findings are resolved. The implementation matches the session specification and is ready for the validation stage.
