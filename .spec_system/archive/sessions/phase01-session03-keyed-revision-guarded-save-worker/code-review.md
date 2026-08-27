# Code Review: Keyed Revision-Guarded Save Worker

**Reviewed**: 2026-08-27
**Base commit**: `0e8e2bf0`
**Result**: RESOLVED

## Scope

Reviewed the complete Session 03 diff: PID-keyed queue state, worker concurrency,
coalescing, exact completion transitions, retry/age/capacity bounds, repository SQL
ordering, all component mappings, connection repair, ambiguity reconciliation,
diagnostics, and regressions.

## Findings

### High - resolved

1. An older exact ACK can remove a redundantly captured bit from a newer queued mask.
   Pending promotion initially required the pre-ACK mask and stalled. Promotion now
   narrows only the pending DTO's selection mask to authoritative Session 01 queued
   identity; copied values remain sealed and unselected rows are ignored.
2. A newer snapshot arriving before an older active snapshot was dispatched initially
   created a second job. Undispatched work now transitions exact state and is replaced
   by the newer cumulative snapshot, preventing redundant database application.
3. Legacy spellbook extra descriptions contain a binary bitset, not a C string. Capture
   now converts the marker to bounded typed spell IDs and the repository emits the
   existing JSON representation without worker access to live objects.

### Medium - resolved

1. An exception escaping the repository callback could terminate a worker thread and
   process. Allocation failures are now retryable completions and unexpected exceptions
   become terminal value-only completions.
2. A failed pooled-connection replacement could return without releasing the borrowed
   handle in a closing-state path. The failure branch now always releases safely.
3. Queue age was initially clamped in diagnostics. It now reports truthful age and a
   separate configured-limit-exceeded flag.

## Behavioral Review

- One active apply exists per PID; independent PIDs use up to the configured workers.
- Pending snapshots replace older pending values only with a newer cumulative mask.
- The 128-PID, 256-result, 32 MiB, five-minute age, and eight-retry bounds are explicit.
- Database work occurs after releasing the queue lock and receives only snapshot values.
- `SELECT ... FOR UPDATE` and comparison precede every component mutation.
- Equal durable revision is idempotent; newer durable revision is stale and mutates none.
- Commit transport ambiguity is checked on a repaired pooled connection before retry.
- Main-thread ACK/failure calls require exact PID/revision/component identity.
- No production save trigger initializes or submits to the new worker in this session.

## Verification

- Deterministic worker concurrency/coalescing/retry/capacity runtime regression: PASS.
- Repository transaction/component/ambiguity source contracts: PASS.
- Snapshot regression, security scan, warning-as-error build, and formatting: PASS.
- Full suite: PASS, 180/180 plus signal-handler checks.

## Conclusion

All findings are resolved. The implementation is ready for validation.
