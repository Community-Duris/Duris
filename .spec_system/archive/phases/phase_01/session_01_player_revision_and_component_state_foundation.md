# Session 01: Player Revision and Component State Foundation

**Session ID**: `phase01-session01-player-revision-and-component-state-foundation`
**Status**: Complete
**Work Window**: One persistence identity boundary spanning durable player revisions,
component taxonomy, game-thread dirty state, login hydration, and schema contracts.

---

## Objective

Establish the monotonic revision and component-state contract that every later player
snapshot, worker transaction, journal record, and acknowledgement can share without
changing the active save route yet.

---

## Scope

### In Scope (MVP)

- Define the complete checkpoint component taxonomy from the post-Phase-00 save and
  mutation inventory, including explicit compatibility boundaries for Phase 02
  economy and ownership commands.
- Add guarded, re-runnable schema and fresh-bootstrap support for the durable player
  save revision and any component metadata required by revision-guarded apply.
- Add game-thread-owned dirty, queued, inflight, and acknowledged revision state keyed
  by PID, with monotonic allocation and cumulative unacknowledged component masks.
- Hydrate or initialize revision state during player load and define safe behavior for
  legacy rows, failed hydration, PID assignment, rename, deletion, and reconnect.
- Provide narrow component-marking and state-snapshot APIs with bounded metrics and
  source-contract tests; do not route production save callers yet.

### Out of Scope

- Immutable snapshot payload capture or database worker execution.
- Journal persistence and replay.
- Replacing current autosave, terminal, Redis-dirty, or fork behavior.
- Phase 02 exactly-once epic, bank, wallet, or item-ownership transactions.

---

## Prerequisites

- [x] All Phase 00 sessions and phase-transition checks are complete.
- [x] The post-Phase-00 schema and player save/load inventories are revalidated.
- [x] Schema validation used static/runtime contracts only; no configured database was touched.

---

## Deliverables

1. Player revision and component-state contracts in new focused `src/` modules and the
   nearest player structures or headers.
2. Additive migration, fresh-bootstrap schema update, and schema verification under
   `migrations/`.
3. Login, creation, reconnect, rename, and deletion lifecycle integration for revision
   state without enabling the new save pipeline.
4. Focused runtime, source-contract, and isolated schema regressions under
   `tests/async/`.

---

## Success Criteria

- [x] Revisions are monotonic, never represented by pointer or wall-clock identity,
      and have explicit overflow behavior.
- [x] Acknowledging revision N cannot clear components dirtied again after N.
- [x] Coalescing carries every unacknowledged component into the next revision.
- [x] Existing and new player rows initialize revision state deterministically, and
      failed required hydration fails closed.
- [x] The migration is additive, guarded, re-runnable where practical, and never run
      against production.
- [x] Current save behavior remains unchanged until later cutover sessions.
- [x] Focused regressions, formatting checks, and `make -C src` pass.

---

## Completion

Validated 2026-08-27. The warning-as-error build, focused state/schema/lifecycle
contracts, formatting, and all 178 Python regressions plus signal-handler checks pass.
No migration was executed.
