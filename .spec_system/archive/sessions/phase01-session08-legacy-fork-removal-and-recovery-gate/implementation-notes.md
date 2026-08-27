# Implementation Notes

## Cutover

- Deleted the persistence child state machine, timeout/kill/reap code, synchronous world
  fallback, and legacy JSON world save/restore codec from `redis.c`.
- Removed Redis dirty-player key initialization, pwipe clearing, public clearing API,
  and the misleading immortal command that claimed it could discard the real queue.
- World status now calls the validated immutable-generation predicate instead of trusting
  a separately mutable `valid` flag.
- Increased player pipeline and worker PID capacity from 128 to 256 so a 200-player
  checkpoint wave fits while retaining the existing 32 MiB byte bound.

## Gate

`test_phase01_recovery_gate.py` drives the production revision-state and keyed-worker
implementations with 25, 50, 100, and 200 logical clients. Workers are held until each
wave is admitted, making the queue high-water assertion deterministic. Selected PIDs
receive one ambiguous-commit result and must retry with the same revision; others cover
already-applied reconciliation. Every completion must leave exact revision 1 ACKed with
no unacknowledged components.

The same gate inventories persistence sources for retired child/dirty/legacy tokens,
worker pointer ownership, player routes, world publisher ownership, and copyover/shutdown
drains. Existing journal, terminal, worker, Redis outage, and world framing regressions
remain part of the focused fault matrix.

## Load Evidence

The local dependency-free gate admitted all cohorts. The 200-client wave retained
819,200 bytes at peak, below the 32 MiB limit, and completed within milliseconds on the
validation host. This is a deterministic coordinator/worker readiness result, not a
production network or database benchmark.
