# Implementation Notes

**Result**: COMPLETE

Implemented one typed eleven-job maintenance registry with stable instance offsets,
priority, fixed row/time budgets, one bounded worker, pooled database access, exact
cursors, bounded backoff, overlap suppression, and redacted health. Immutable requests
and pending completions are checksummed and atomically persisted so restart re-executes
or republishes the same identity rather than losing auction, catalog, boon, CTF, or
notification completion work.

The game loop now only captures bounded primitive state and publishes pure results.
Database work for auction, poll, epic catalog/balance/modifiers, level cap, trophies,
boons, cargo, and operational statistics runs on the worker. Web output is atomically
replaced by the worker. Pure ship, ferry, random-map, short-affect, and newcomer work
remains on the game thread with deterministic offsets. Copyover quiesces and drains the
scheduler and resumes all workers on any failed copyover path.

The complete classification is recorded in `maintenance-inventory.md`. No migration,
new index, production operation, credential change, or Phase 04 artifact was created.
