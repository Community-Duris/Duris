# Code Review: Nonterminal Save Pipeline Cutover

**Reviewed**: 2026-08-27
**Base commit**: `896f0aee`
**Result**: RESOLVED

## Scope

Reviewed coordinator ownership, revision transitions, append/apply ordering, allocation
and overload behavior, worker threading, production lifecycle, ordinary save branches,
mutation inventory, Redis retirement, transaction compatibility, diagnostics, and tests.

## Findings

### High - resolved

1. Installing the Session 04 append hook directly would run `fdatasync` on the game
   thread. A dedicated bounded dispatcher now performs journal append before making a
   snapshot worker-eligible; the game pulse contains no journal call.
2. Initial pulse dispatch copied up to a 4 MiB snapshot on the game thread and consumed
   it even when the worker could not accept it. The retained-submit API now moves only
   on successful admission and keeps durable-ready ownership on capacity/unavailability.
3. A failed capture or admission left queued components with `dirty_components == 0`,
   so later autosave could classify them as unchanged forever. Checkpoint retry now
   detects queued identity without a retained/inflight snapshot and recaptures it.
4. A synchronous critical compatibility save could be overwritten by an older worker
   snapshot. The legacy transaction now advances a newly marked durable revision and
   requires exactly one affected revision row, fencing all older jobs.

### Medium - resolved

1. Worker and replay threads did not initialize MySQL thread-local state. Each now pairs
   `mysql_thread_init()` and `mysql_thread_end()` around repository work.
2. Dispatcher and enqueue container allocation failures could escape or discard retry
   intent. They now become overload/durable-spill health while unacknowledged revision
   state remains available for recapture or journal replay.
3. `do_save_silent` initially skipped its ship side effect after player cutover. It now
   uses the existing nonblocking ship-save queue before returning.
4. Conservative all-component marks were narrowed across audited money, quest, object,
   pet, skillbook, experience, and timer mutation sites.
5. New characters without a durable PID could enter the pipeline before revision
   hydration. Initial creation remains on the guarded legacy insert route.

## Behavioral Review

- Unchanged autosave returns before queue, capture, journal, worker, Redis, or SQL work.
- Journal sync precedes worker admission, and exact durable ACK precedes checkpoint.
- Newer PID work remains cumulative across pending append, durable ready, active worker,
  and one newest worker-pending snapshot.
- Redis outages and volatile key loss cannot remove local dirty or journal state.
- The dirty-save fork implementation and conditional Redis scheduling are removed.
- Terminal and locker paths remain outside this work window and are not silently routed.

## Verification

- Revision state runtime and coordinator/call-site contracts: PASS.
- Redis failure, dirty flush, status, worker, journal, deferred, and terminal tests: PASS.
- Warning-as-error build, security scan, formatting, and whitespace checks: PASS.
- Full suite: PASS, 182/182 plus signal-handler checks.

## Conclusion

All findings are resolved. The implementation is ready for validation.
