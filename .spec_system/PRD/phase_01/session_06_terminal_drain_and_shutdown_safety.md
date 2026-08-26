# Session 06: Terminal Drain and Shutdown Safety

**Session ID**: `phase01-session06-terminal-drain-and-shutdown-safety`
**Status**: Not Started
**Work Window**: One terminal persistence contract covering player extraction, bounded
promotion and wait, durable spill, copyover/shutdown drain, and legacy fallback
transition.

---

## Objective

Move every destructive or process-terminal player save onto the latest revisioned job,
avoid duplicate snapshots, and retain live state whenever neither exact database
acknowledgement nor the permitted durable journal handoff succeeds.

---

## Scope

### In Scope (MVP)

- Route camp, inn, death, link loss, idle rent, artifact transitions, forced extraction,
  copyover, and shutdown through one terminal coordinator contract.
- Promote the existing latest dirty or queued revision instead of flushing it and then
  building a second full snapshot; gate relevant mutations while terminal completion is
  pending.
- Define bounded wait, cancellation, retry, journal spill, and operator-visible outcome
  semantics for ordinary disconnects, copyover, and full shutdown.
- Extract characters and inventory only after the transition's required database ACK
  or explicitly allowed synced journal handoff; retain the live retryable state when
  neither durable boundary succeeds.
- Stop new mutations, drain workers to a deadline, persist remaining jobs, and verify
  restart replay before copyover or shutdown reports success.
- Stop creating new legacy player pfiles after journal cutover while preserving existing
  files for read-only inventory, compatibility, or operator recovery reporting.

### Out of Scope

- Deleting preexisting pfiles or player/account runtime data.
- World recovery worker shutdown and restart, owned by Session 07.
- Phase 02 critical economy or ownership completion gates.
- Final removal of disabled legacy player-fork code.

---

## Prerequisites

- [ ] Session 05 nonterminal cutover is validated.
- [ ] Phase 00 terminal failure paths retain live character and inventory state.
- [ ] Journal replay has passed every Session 04 crash-point test.

---

## Deliverables

1. Terminal save request, promotion, completion, durable-spill, and failure APIs in the
   player coordinator and all extraction callers.
2. Bounded copyover and shutdown drain integration in `src/copyover.c`, `src/comm.c`,
   worker lifecycle code, and related terminal paths.
3. Legacy pfile write retirement plus non-destructive inventory or compatibility
   reporting for existing records.
4. Focused camp, rent, death, link-loss, artifact, forced-disconnect, copyover,
   shutdown, timeout, journal, and restart regressions under `tests/async/`.

---

## Success Criteria

- [ ] A terminal request promotes one latest player revision and never issues a second
      snapshot for the same state.
- [ ] Failed or timed-out DB and journal handoff leaves the character and inventory live,
      fenced, and retryable instead of extracting them.
- [ ] Copyover and shutdown stop new mutations, drain or durably spill all accepted
      jobs by a bounded deadline, and replay every spill after restart.
- [ ] Stale completions cannot release a newer terminal fence or clear newer dirty state.
- [ ] No new player state is written to the unreconciled legacy pfile format, and no
      existing pfile is deleted.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
