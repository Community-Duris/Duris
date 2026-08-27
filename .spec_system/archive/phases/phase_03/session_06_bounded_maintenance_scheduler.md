# Session 06: Bounded Maintenance Scheduler

**Session ID**: `phase03-session06-bounded-maintenance-scheduler`
**Status**: Complete (validated 2026-08-27)
**Work Window**: One recurring-work boundary from cadence registration and deterministic
staggering through bounded job submission, row/time cursors, completion, retry,
shutdown, and pulse-latency verification.

---

## Objective

Prevent exact-modulus maintenance waves and unbounded recurring queries or file work
from pausing the simulation while preserving each maintenance task's cadence and result.

---

## Scope

### In Scope (MVP)

- Re-inventory post-Phase-02 timers, auctions, random-map spawns, web statistics,
  approval/expiration polls, epic-zone balance, level cap, boon maintenance, artifact
  checks, operational statistics, cache refreshes, and other recurring external work.
- Define one typed maintenance registry with stable job IDs, nominal cadence,
  deterministic per-instance offset, priority, deadline, row/time budget, continuation
  cursor, overlap rule, and observable completion state.
- Replace aligned `pulse % interval` external work with staggered bounded submissions
  while keeping lightweight pure game-thread work at its intended cadence.
- Route database, Redis, filesystem, and large serialization work through the applicable
  Phase 01/02 typed workers or a bounded maintenance worker; never hold live pointers or
  game locks across that work.
- Make each scan process at most its row/time budget, persist or retain an exact cursor,
  resume without skipping or duplicating effects, and prevent a slow prior run from
  creating overlapping copies.
- Move operational statistics database/file output off the pulse callback or into the
  approved metrics path with bounded buffering, retention classification, and truthful
  drop/spill behavior.
- Add virtual-pulse/common-multiple, backlog, outage, retry, cursor, restart, copyover,
  shutdown, and 200-player scheduled-work tests.

### Out of Scope

- Gameplay rule or reward changes inside auction, boon, artifact, epic-zone, or level-
  cap calculations.
- Database indexes not approved by Session 05.
- Retention archive execution, owned by Session 08.

---

## Prerequisites

- [x] Phase 01 bounded worker/journal/shutdown behavior is authoritative.
- [x] Phase 02 scheduled domain mutations use typed idempotent commands.
- [x] Session 05 has established the accepted access paths for maintenance queries.

---

## Deliverables

1. Typed maintenance registry, cadence/offset policy, bounded job/result contracts, and
   lifecycle integration in focused `src/` modules.
2. Cutover of audited recurring external work from aligned game-loop callbacks to
   bounded jobs with exact cursors and overlap prevention.
3. Operational metrics/statistics path with explicit buffering, failure, and retention
   behavior and no synchronous pulse file/database output.
4. Focused timing, common-multiple, row/time-budget, cursor, overlap, outage, restart,
   shutdown, and scheduled-load regressions under `tests/async/`.

---

## Success Criteria

- [x] Recurring external jobs have stable IDs and deterministic offsets that prevent
      the 60/120-second maintenance wave within one server and across configured peers.
- [x] Every scan respects its row or wall-time budget and resumes from an exact cursor
      without skipping or applying an idempotent effect twice.
- [x] A still-running job cannot overlap itself, and retry uses the same work identity
      and cursor under bounded backoff.
- [x] No scheduled callback performs database, Redis, filesystem, or large serialization
      work on the simulation thread.
- [x] Queue age, run age, cursor lag, rows, retries, overlap suppression, and failures
      are bounded and redacted.
- [x] The scheduled-work workload stays within pulse/event budgets without sustained
      debt at the defined 25-to-200-client levels.
- [x] Focused regressions, formatting checks, and `make -C src` pass.
