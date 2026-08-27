# Session 05: Combat and Artifact Persistence Correctness

**Session ID**: `phase00-session05-combat-artifact-persistence-correctness`
**Status**: Complete
**Work Window**: Two tightly scoped deterministic-output defects in combat persistence,
verified at their mutation-to-durable-publication boundary and failure paths.

---

## Objective

Publish victim frag state only after applying the in-memory loss and make artifact-bind
lookups initialize caller outputs deterministically for success, no-row, and query
failure outcomes.

---

## Scope

### In Scope (MVP)

- Apply the victim frag loss before the durable leaderboard or progress update reads the
  player value.
- Preserve existing frag calculations, messaging, boon checks, and gameplay semantics.
- Initialize artifact-bind owner and timer outputs before any query and retain safe
  defaults on query, result-allocation, no-row, or malformed-row failure.
- Audit direct callers for assumptions about uninitialized bind outputs.
- Add focused regressions for victim publication order and every bind lookup result.

### Out of Scope

- Phase 02 batching of PvP, artifact, boon, reward, or zone-touch persistence.
- Phase 03 recent-death N+1 query optimization.
- Redesign of frag gain formulas or artifact gameplay rules.

---

## Prerequisites

- [x] Session 01 redacted query failure diagnostics are validated.

---

## Deliverables

1. Correct mutation and persistence ordering in `src/fight.c`.
2. Deterministic artifact-bind output handling in `src/sql.c` and affected callers.
3. Focused source-contract or runtime regressions under `tests/async/`.

---

## Success Criteria

- [x] Durable victim frag and leaderboard state reflects the post-loss value.
- [x] Artifact-bind outputs are always initialized, including database failure and
      missing-row paths.
- [x] Existing gain/loss formulas and player-visible messages remain unchanged.
- [x] Focused regressions, formatting checks, and `make -C src` pass.

---

## Completion Summary

Victim frag loss now mutates in-memory state before the durable progress and
leaderboard update reads it. Artifact-bind lookup now initializes every provided
output, reports status explicitly, treats a genuine missing row as successful unbound
state, and rejects database, result, fetch, null-column, malformed, and out-of-range
failures without publishing partial values.

All three artifact callers fail closed before ownership decisions. Focused and nearest
regressions, the warning-as-error C++20 build, formatting, review, and the full 172-test
suite plus signal-handler checks pass.
