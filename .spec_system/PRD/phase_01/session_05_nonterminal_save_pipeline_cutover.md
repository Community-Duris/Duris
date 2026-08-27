# Session 05: Nonterminal Save Pipeline Cutover

**Session ID**: `phase01-session05-nonterminal-save-pipeline-cutover`
**Status**: Complete
**Work Window**: The complete ordinary player checkpoint route from mutation and save
request through revisioned capture, journal handoff, worker ACK, and compatibility
reporting, excluding destructive terminal transitions.

---

## Objective

Make the revisioned coordinator authoritative for ordinary player changes, autosaves,
manual checkpoints, and existing Redis-dirty requests so normal full-player save paths
perform no database, Redis, or filesystem I/O on the simulation thread.

---

## Scope

### In Scope (MVP)

- Inventory every nonterminal `mark_player_dirty()`, `sql_save_player()`,
  `writeCharacter()`, `do_save_silent()`, deferred-save, level-checkpoint, and autosave
  caller after Phase 00 and route its intended component changes through the coordinator.
- Replace Redis dirty-set membership and debounce as durability state with in-process
  component marks plus journal-backed work; keep Redis only for reconstructible caches.
- Make autosave checkpoint only dirty components, avoid SQL for unchanged players, and
  coalesce reconnect or synchronized autosave waves within explicit capture budgets.
- Give manual save and gameplay callers explicit queued, durable, retryable, overload,
  and failure results without issuing a duplicate full snapshot.
- Preserve temporary compatibility for paths whose Phase 02 critical domain command is
  not yet implemented, while preventing them from using a stale full snapshot as an
  exactly-once substitute.
- Disable the forked player flush once parity tests show every nonterminal route reaches
  the coordinator; leave code removal for Session 08.

### Out of Scope

- Camp, rent, death, link-loss, copyover, shutdown, or other extraction gates.
- World recovery snapshots.
- Phase 02 atomic epic, bank, wallet, item-ownership, reward, or outbox commands.
- Final deletion of legacy fork and compatibility code.

---

## Prerequisites

- [x] Sessions 01 through 04 are validated, including journal failure behavior.
- [x] Phase 00 non-blocking Redis and truthful save diagnostics remain enforced.

---

## Deliverables

1. Component-specific mutation and checkpoint integration across current nonterminal
   player save callers in `src/`.
2. Autosave, manual save, deferred checkpoint, and level checkpoint routing through the
   player coordinator.
3. Redis dirty-state compatibility replacement and disabled fork-flush scheduling in
   `src/redis.c`, `src/new_events.c`, and related interfaces.
4. Focused call-site inventory, hot-path isolation, coalescing, overload, and unchanged-
   player regressions under `tests/async/`.

---

## Success Criteria

- [x] Every audited ordinary mutation and player checkpoint marks the correct component
      set and reaches the revisioned coordinator.
- [x] Autosave of an unchanged player performs no database write and creates no journal
      or worker job.
- [x] Normal mutation and checkpoint callers execute no database, Redis, or filesystem
      I/O on the simulation thread.
- [x] Redis disabled, unavailable, restarted, or missing volatile keys cannot lose or
      force a synchronous player checkpoint.
- [x] Manual and repeated save requests coalesce without losing later component changes
      or creating duplicate snapshots.
- [x] The player-save fork is disabled and all nonterminal parity tests pass before its
      final code removal.
- [x] Focused regressions, formatting checks, and `make -C src` pass.
