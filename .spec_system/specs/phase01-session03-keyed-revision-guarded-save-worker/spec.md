# Session Specification

**Session ID**: `phase01-session03-keyed-revision-guarded-save-worker`
**Phase**: 01 - Replace Forked Full Saves
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `0e8e2bf0`
**Work Window**: One bounded PID-keyed queue/apply/completion boundary; no trigger cutover or durable journal.

---

## 1. Session Overview

Sessions 01 and 02 provide exact revision/component state and immutable payloads. This
session moves those payloads through a bounded worker without allowing same-PID overlap,
applies selected repositories inside a revision-guarded transaction, and returns exact
value-only completions for main-thread acknowledgement.

## 2. Objectives

1. Add a byte/job/age-bounded PID-keyed coordinator with queued/inflight coalescing.
2. Permit worker parallelism across PIDs while enforcing one active apply per PID.
3. Apply every checkpoint component from typed DTO values in one database transaction.
4. Lock and compare durable `save_revision`, reject stale work, and advance it only with the component transaction.
5. Classify retryable, terminal, stale, and ambiguous outcomes and reconcile commit ambiguity by revision identity.
6. Apply exact completions to Session 01 state and expose redacted queue/latency/retry metrics.

## 3. Scope

### In Scope

- Coordinator lifecycle and deterministic test hooks.
- Typed MySQL repository for status/replacement/skill/affect/item/pet/shape/trophy rows.
- Main-thread completion drain and Phase 00 persistence-health rendering.
- Concurrency, order, coalescing, capacity, failure, and SQL contract tests.

### Outside This Work Window

- Active mutation/save trigger routing, restart durability, journal spill/replay, and terminal drain policy.

## 4. Technical Approach

The coordinator owns at most one queued and one inflight value snapshot per PID. A newer
queued snapshot may replace an older one only when its mask covers the cumulative
unacknowledged mask. Workers remove a ready PID under the queue lock, release the lock,
borrow a pool connection, apply, and publish a bounded completion. Repository apply
starts a transaction, locks `player_data`, compares the durable unsigned revision,
applies only selected component tables, advances the revision, and commits. Commit
transport failures are reconciled on a fresh connection before retry classification.
The game thread drains completions and calls only exact Session 01 ACK/failure APIs.

## 5. Success Criteria

- [x] Same-PID work never overlaps and increasing revisions are applied in order.
- [x] Newer pending values coalesce without losing cumulative component identity.
- [x] Durable revision N or newer rejects snapshot N before component mutation.
- [x] All 14 component bits route to typed repository logic in the guarded transaction.
- [x] Exact ACK clears only acknowledged state; stale/failed results retain newer work.
- [x] Queue, byte, age, latency, retry, revision-gap, and lifecycle values are bounded/redacted.
- [x] Focused tests, formatting, warning-as-error build, security scan, and full suite pass.

## Next Steps

Implement the coordinator, repository, diagnostics integration, and regressions.
