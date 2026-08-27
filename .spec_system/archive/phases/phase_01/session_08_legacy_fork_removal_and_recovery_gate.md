# Session 08: Legacy Fork Removal and Recovery Gate

**Session ID**: `phase01-session08-legacy-fork-removal-and-recovery-gate`
**Status**: Complete
**Work Window**: One final cutover boundary removing disabled persistence forks and
proving the player and world pipelines together under bounded load, crash, dependency,
and restart scenarios.

---

## Objective

Delete the replaced player and world fork paths, remove obsolete durability
coordination, and prove the revisioned pipelines meet ordering, recovery, resource, and
simulation-thread isolation criteria before Phase 02 builds on them.

---

## Scope

### In Scope (MVP)

- Remove persistence `fork()`, child MySQL/Redis connection, `waitpid`, child timeout,
  Redis dirty-set, synchronous full-save fallback, and dead compatibility code after
  source and runtime inventories prove no active route depends on them.
- Verify every player save and world recovery entry point reaches the new coordinator,
  journal, worker, completion, and diagnostics contracts with no live-pointer escape.
- Add deterministic fault harnesses for stale revisions, duplicate replay, DB latency
  and outage, ambiguous commit, worker crash, game-process crash, journal disk failure,
  Redis outage, stale world ACK, copyover, shutdown, and restart convergence.
- Run representative non-production 25, 50, 100, and 200-client checkpoint and world-
  recovery workloads, including reconnect and save-wave cases, with queue and pulse
  metrics captured from redacted telemetry.
- Repair defects found by the gate without weakening revision, journal, bound, or
  fail-closed contracts.
- Update architecture, database, configuration, testing, and runbook documentation to
  the implemented execution, recovery, migration, and operator behavior.

### Out of Scope

- Phase 02 transactional gameplay-domain ledgers, outboxes, and command batching.
- Phase 03 login N+1, query/index, migration-ledger, retention, and data-rights work.
- Production load, destructive production tests, or deletion of legacy player data.

---

## Prerequisites

- [x] Sessions 01 through 07 are completed and validated.
- [x] A backed-up representative development clone and non-production game ports are
      available for database and load/fault work.

---

## Deliverables

1. Removal of obsolete player/world fork, Redis dirty-authority, child connection, and
   synchronous snapshot fallback code from `src/`.
2. Phase 01 source-contract, integration, crash-point, recovery, and bounded load
   harnesses under `tests/async/` and ignored generated-output locations.
3. Repaired issues found by the complete player/world recovery gate.
4. Updated `docs/ARCHITECTURE.md`, `docs/DATABASE.md`, `docs/CONFIGURATION.md`,
   `docs/TESTING.md`, `docs/RUNBOOK.md`, and relevant README guidance.

---

## Success Criteria

- [x] No player or world persistence route calls `fork()` or lets a worker traverse live
      mutable game state.
- [x] An older player or world revision never replaces a newer acknowledged generation
      across every ordering and duplicate-replay test.
- [x] Every defined crash point converges without lost accepted work, duplicate durable
      application, premature dirty clearing, or premature character/floor destruction.
- [x] Normal player mutation and snapshot paths perform no database, Redis, or
      filesystem I/O on the simulation thread.
- [x] Queue and journal bytes/age remain within configured limits, overload is explicit,
      and dependency outages do not OOM or strand shutdown.
- [x] The 25-to-200-client Phase 01 workload stays within the PRD pulse/event budgets or
      records a concrete failed gate without claiming readiness.
- [x] Operator documentation and diagnostics distinguish cache, player queue, journal,
      database worker, and world recovery health accurately.
- [x] Focused tests, formatting checks, `make -C src`, and the full repository gate pass.
