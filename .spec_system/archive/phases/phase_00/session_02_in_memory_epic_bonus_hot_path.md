# Session 02: In-Memory Epic Bonus Hot Path

**Session ID**: `phase00-session02-in-memory-epic-bonus-hot-path`
**Status**: Not Started
**Work Window**: The complete epic-bonus read lifecycle from login hydration through
award and selection mutation to expiry, verified at every regeneration and XP caller.

---

## Objective

Remove MySQL and Redis operations from active epic-bonus calculations while preserving
the selected bonus, rolling contribution window, modifier cap, and gameplay-visible
results.

---

## Scope

### In Scope (MVP)

- Add an in-memory epic-bonus state owned by the player object with selected type,
  selection time, qualifying contribution state, computed modifier, and next expiry.
- Hydrate the state during login using bounded set-based reads and fail login or disable
  the bonus explicitly when required data cannot be loaded.
- Update the state after successful bonus selection and epic award paths, including
  expiry of contributions outside the configured rolling window.
- Make hit regeneration, movement regeneration, XP, and other active-player bonus reads
  pure in-memory operations.
- Add focused regressions for hydration, selection changes, award updates, expiry, caps,
  and zero database calls from the hot callbacks.

### Out of Scope

- The Phase 02 atomic epic ledger and spendable-balance transaction.
- Redis caching of active-player bonus state.
- Unrelated epic task, artifact, or guild persistence redesign.

---

## Prerequisites

- [ ] Session 01 diagnostics are validated so hot-path query regressions are visible
      without logging private data.

---

## Deliverables

1. Player-owned epic-bonus state in the appropriate `src/` structures and lifecycle
   helpers.
2. Login hydration plus mutation and expiry integration in `src/epic_bonus.c`,
   `src/epic.c`, and their callers.
3. In-memory reads in `src/limits.c` and all other active-player hot paths.
4. Focused regressions under `tests/async/` that enforce both result correctness and
   external-I/O isolation.

---

## Success Criteria

- [ ] Hit and movement regeneration and XP calculation call no database or Redis API.
- [ ] Login, bonus selection, epic gain, rolling expiry, and configured caps produce
      the same intended modifier as the durable data.
- [ ] Missing or failed hydration has a bounded, explicit behavior and never performs
      a lazy hot-path query.
- [ ] Worker code does not traverse or mutate live player objects.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
