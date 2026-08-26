# Session 01: Consistent Player Load Transaction

**Session ID**: `phase03-session01-consistent-player-load-transaction`
**Status**: Not Started
**Work Window**: One login read boundary from authenticated player selection through a
bounded worker-owned consistent snapshot, typed result staging, exact revision checks,
and all-or-nothing game-thread publication.

---

## Objective

Load each player from one complete durable revision without blocking the simulation
thread or allowing an optional-looking query failure to create a partial character.

---

## Scope

### In Scope (MVP)

- Re-inventory the implemented Phase 01/02 login, copyover, account selection, disguise,
  revision, bank, epic, ownership, and recovery paths before defining the final required
  component set.
- Define a schema-versioned immutable player-load request and typed result containing
  status, ancillary arrays, skills, affects, shapechange, authoritative domain values,
  snapshot revision, and classified component errors without live pointers.
- Execute required reads on one worker-owned connection in one documented consistent
  read transaction with a bounded deadline, cancellation, row/byte limits, and exact
  connection/session invariants.
- Stage every result away from the live world and publish on the game thread only after
  all required components, expected revisions, and account/player identities validate.
- Make required component failure, transaction timeout, disconnect, duplicate request,
  stale completion, and shutdown discard the staged result and return a clean bounded
  login outcome.
- Convert login/nanny and copyover state transitions to exact request/completion
  identity without synchronous database work or a second independent component load.
- Expose redacted request age, query count, snapshot age, row/byte size, cancellation,
  failure class, and publication latency.

### Out of Scope

- Final item-owner batching and O(N) object assembly, owned by Session 02.
- Final pet item/metadata batching and graph publication, owned by Session 03.
- Query-index migrations, retention, export, or erasure.

---

## Prerequisites

- [ ] Phases 00 through 02 and their transition evidence are complete.
- [ ] Phase 01 worker, completion, revision, journal, and shutdown contracts are
      authoritative.
- [ ] Phase 02 domain-owned values and revisions cannot be overwritten by player
      snapshots.

---

## Deliverables

1. Typed load request, row DTO, component result, error, and exact completion contracts
   in focused `src/` modules.
2. Bounded consistent-read worker/repository integrated with account selection,
   `restoreCharOnly()`, nanny, copyover, and shutdown lifecycle.
3. Game-thread staged materialization and all-or-nothing publication without partial
   live character state.
4. Focused consistent-revision, component-failure, timeout, disconnect, duplicate,
   stale-completion, copyover, and shutdown regressions under `tests/async/`.

---

## Success Criteria

- [ ] All required player rows come from one documented consistent database snapshot.
- [ ] Status, skills, affects, ancillary arrays, shapechange, and domain-owned values
      publish together or none of them enter the live character.
- [ ] A missing or failed required component returns a clean login error instead of an
      empty default and never leaves a partially initialized character or descriptor.
- [ ] Duplicate, cancelled, disconnected, or stale requests cannot publish another
      request's data or consume a newer completion.
- [ ] Login and copyover database reads do not run on the simulation thread.
- [ ] Request time, transaction age, rows, bytes, and retries remain within configured
      bounds and diagnostics expose no account, IP, description, or credential values.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
