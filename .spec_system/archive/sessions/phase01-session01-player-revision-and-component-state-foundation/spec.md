# Session Specification

**Session ID**: `phase01-session01-player-revision-and-component-state-foundation`
**Phase**: 01 - Replace Forked Full Saves
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `64f249ec`
**Work Window**: Durable player revision schema, checkpoint component taxonomy, PID-keyed game-thread state, hydration, and lifecycle contracts without save-route cutover.

---

## 1. Session Overview

Phase 00 made existing save failure truthful but the active player save has no durable
monotonic identity. Dirty work is represented by Redis membership, container flags,
and deferred slots rather than one component-aware state machine. Phase 01 needs that
identity before immutable capture or worker execution can be safe.

## 2. Objectives

1. Add an additive `player_data.save_revision` contract with deterministic legacy/new-row behavior.
2. Define the full checkpoint component mask and explicit Phase 02 command-domain boundary.
3. Add PID-keyed main-thread dirty, queued, inflight, and acknowledged revision state.
4. Prove cumulative coalescing, exact acknowledgements, redirty safety, and overflow failure.
5. Hydrate state at required player status load, initialize PID assignment, preserve it across rename/reconnect, and forget it only after successful deletion.

## 3. Scope

### In Scope

- A standalone C++20 player revision-state module with no character pointers or I/O.
- One unsigned 64-bit durable revision column in guarded migration and bootstrap definitions.
- Required load hydration and narrow creation/deletion lifecycle hooks.
- Source, runtime, schema, and current-route non-cutover regressions.

### Outside This Work Window

- Snapshot DTO payloads, workers, journals, ACK polling, active dirty-call routing, or
  Phase 02 exactly-once economy/ownership behavior.

## 4. Technical Approach

Use a process-local map keyed only by positive PID. Each component records its latest
mutation revision so an ACK for N clears a component only when it was not dirtied after
N. Queue preparation always copies the cumulative unacknowledged mask; inflight
promotion and ACK require exact revision/mask identity. Revision allocation fails and
latches an overflow state at `UINT64_MAX` rather than wrapping. Hydration accepts the
durable unsigned value and fails the load if required state cannot be established.

Add `save_revision BIGINT UNSIGNED NOT NULL DEFAULT 0` through a re-runnable migration
and every maintained fresh schema. Append it to status load, initialize new PID state,
retain PID identity through rename/reconnect, and discard state only after successful
row deletion. Do not call component marking from production mutation sites yet.

## 5. Deliverables

| File | Change |
|------|--------|
| `src/player_revision_state.h`, `src/player_revision_state.c`, `src/Makefile` | Taxonomy and PID-keyed state machine |
| `src/sql_player.c`, `src/db.c` or nearest lifecycle hooks | Required hydration, creation, release, rename/delete behavior |
| `migrations/player_save_revision.sql` and maintained schema sources | Additive revision column |
| `tests/async/test_player_revision_state.py` | Runtime, source, schema, and non-cutover contracts |

## 6. Success Criteria

- [x] Revisions are monotonic and overflow fails without pointer/time identity.
- [x] ACK N cannot clear a component whose latest mutation is newer than N.
- [x] Every queued revision carries the cumulative unacknowledged component mask.
- [x] Legacy/new rows initialize at revision zero and required hydration fails closed.
- [x] PID identity survives reconnect and rename; successful deletion forgets state.
- [x] Migrations are additive/guarded and no production database is touched.
- [x] The active save route and current mutation callers remain unchanged.
- [x] Focused tests, formatting, warning-as-error build, and full regressions pass.

## 7. Risks And Resolutions

- **Premature cutover**: tests reject production callers of the marking API in this session.
- **Stale ACK data loss**: latest-per-component revision gates every bit clear.
- **Coalescing omission**: queue snapshots use the cumulative unacknowledged mask, not only newly dirty bits.
- **Revision exhaustion**: overflow is explicit and sticky until operator/recovery intervention.
- **Transient player loads**: state is keyed by durable PID rather than name or character pointer.

## Next Steps

Session complete. Continue with Phase 01 Session 02 immutable snapshot capture.
