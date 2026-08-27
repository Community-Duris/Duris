# Session Specification

**Session ID**: `phase00-session05-combat-artifact-persistence-correctness`
**Phase**: 00 - Correctness and Immediate Lag Removal
**Status**: Not Started
**Created**: 2026-08-27
**Base Commit**: `20e13a1a6916102914c945647b0d211af25ad9b1`
**Work Window**: Two deterministic persistence-boundary fixes: publish post-loss victim frags and make artifact-bind lookups explicit and fail-closed.

---

## 1. Session Overview

`AddFrags()` persists a victim loss before subtracting it from the in-memory total, while `sql_modify_frags()` publishes that in-memory total to the leaderboard. Artifact-bind lookup also returns without initializing outputs on several database failure paths, allowing stack values to drive ownership decisions.

## 2. Objectives

1. Apply victim loss before durable publication reads the victim total.
2. Give artifact-bind lookups deterministic defaults and an explicit success/failure result.
3. Preserve no-row as a successful unbound lookup and make database/malformed-row failures fail closed.
4. Preserve frag formulas, gameplay messages, and normal artifact ownership behavior.

## 3. Scope

### In Scope

- Reorder the existing victim mutation and persistence calls.
- Initialize bind outputs before querying, check result allocation, and strictly parse both selected columns.
- Return `bool` from bind lookup and audit all three direct callers.
- Add focused source-contract regressions for ordering and every lookup outcome.

### Outside This Work Window

- Persistence batching, frag formula redesign, artifact schema changes, or broad SQL parsing cleanup.
- Production database operations or migrations.

## 4. Technical Approach

Change `sql_get_bind_data()` to return `bool`. Valid output pointers are set to owner/timer zero before the query. Query failure, missing result objects, missing rows after a reported row count, null columns, and invalid or out-of-range integers return false with defaults intact. A genuine zero-row result returns true with those defaults. Callers return or skip on false so outages cannot bind, feed, or repair ownership using fabricated unbound state.

## 5. Deliverables

| File | Change |
|------|--------|
| `src/fight.c` | Apply victim loss before durable publication |
| `src/sql.c`, `src/sql.h` | Deterministic, status-bearing bind lookup |
| `src/artifact.c` | Fail-closed handling at all direct callers |
| `tests/async/test_combat_artifact_persistence.py` | Focused source contracts for both defects |

## 6. Success Criteria

- [ ] `sql_modify_frags(victim, ...)` observes the post-loss in-memory total.
- [ ] Bind outputs are zeroed for every valid call before database work.
- [ ] Success, no-row, query failure, allocation failure, and malformed-row outcomes are explicit and deterministic.
- [ ] Every direct caller checks failure before ownership decisions or updates.
- [ ] Focused tests, formatting, C++20 build, and full regression suite pass.

## 7. Risks And Resolutions

- **Outage interpreted as unbound**: distinguish failure from a successful zero-row result and fail closed in callers.
- **Partial malformed publication**: parse into local temporaries and publish both values only after both validate.
- **Formula drift**: reorder only the existing mutation and write statements; leave calculation and messages byte-for-byte unchanged.

## Next Steps

Run the `implement` workflow step.
