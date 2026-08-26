# Session 05: Combat and Artifact Persistence Correctness

**Session ID**: `phase00-session05-combat-artifact-persistence-correctness`
**Status**: Not Started
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

- [ ] Session 01 redacted query failure diagnostics are validated.

---

## Deliverables

1. Correct mutation and persistence ordering in `src/fight.c`.
2. Deterministic artifact-bind output handling in `src/sql.c` and affected callers.
3. Focused source-contract or runtime regressions under `tests/async/`.

---

## Success Criteria

- [ ] Durable victim frag and leaderboard state reflects the post-loss value.
- [ ] Artifact-bind outputs are always initialized, including database failure and
      missing-row paths.
- [ ] Existing gain/loss formulas and player-visible messages remain unchanged.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
