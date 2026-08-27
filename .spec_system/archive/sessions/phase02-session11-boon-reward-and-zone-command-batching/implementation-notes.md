# Implementation Notes

- 2026-08-27: Session opened from `c07765fa` after Session 10 passed and published.
- Inventory found `check_boon_completion` synchronously selecting active definitions,
  loading each definition and progress row, then issuing per-boon progress/shop writes
  before reward publication.
- Epic-stone completion directly updates zone current state, touch history, reset state,
  in-memory alignment, and Redis after separately issuing participant epic awards.
- Account-bound reward grants already use exact templates, unique grant/character summon
  rows, cooldown/recovery state, guarded transactions, and dedicated concurrency/schema
  regressions. This session preserves that explicit command boundary while removing
  synchronous I/O from simulation-triggered boon and zone paths.
- Boon triggers now capture bounded scalar event facts and enqueue one player-keyed
  command. The repository locks eligible definitions and progress, applies completion
  and shop effects, and records a bounded result and outbox atomically.
- Epic-stone touch captures the exact bounded participant PID set before submission.
  Zone current state, alignment, legacy history, outcome, participants, inbox result,
  and cache outbox commit in one worker transaction.
- Exact completions publish gameplay effects through existing epic, currency, item,
  and player-state adapters; Redis invalidation is restricted to durable outbox delivery.
- Local schema verification and reconciliation, the real MySQL replay harness, format,
  warning-as-error build, security baseline, and all 195 repository tests pass.
