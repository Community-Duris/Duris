# PRD Phase 01: Replace Forked Full Saves

**Status**: In Progress
**Sessions**: 8 (initial estimate)
**Estimated Duration**: Adaptive; each session continues through its verification boundary

**Progress**: 0/8 sessions (0%)

---

## Overview

Phase 01 replaces stale forked full-player saves with one revisioned persistence
pipeline. The game thread remains the sole owner of mutable player and object state,
captures immutable component snapshots, and submits bounded work ordered by player.
Workers apply only newer revisions and return exact acknowledgements; dirty components
remain pending whenever a newer mutation or failed apply supersedes an acknowledgement.

The phase also introduces a typed, checksummed, idempotent local journal, moves normal
and terminal save triggers onto the new coordinator, and replaces forked world recovery
with a long-lived worker that receives immutable sequence-numbered snapshots. Critical
epic, wallet, bank, and ownership transactions remain Phase 02 work, so revisioned
checkpoints must not be presented as exactly-once domain commands.

Phase 00 is complete and audited. Its correctness, observability,
failure-containment, and security contracts are prerequisites for every session here.

---

## Progress Tracker

| Session | Name | Status | Work Window | Validated |
|---------|------|--------|-------------|-----------|
| 01 | Player Revision and Component State Foundation | Not Started | Durable revision contract, component taxonomy, and game-thread dirty state | - |
| 02 | Immutable Player Snapshot Capture | Not Started | Bounded typed DTO capture for every checkpoint component without live-state mutation | - |
| 03 | Keyed Revision-Guarded Save Worker | Not Started | Ordered coalescing jobs, transactional apply, exact ACKs, and stale-result handling | - |
| 04 | Typed Persistence Journal and Replay | Not Started | Checksummed append, spill, replay, corruption handling, and idempotent checkpoints | - |
| 05 | Nonterminal Save Pipeline Cutover | Not Started | Ordinary mutation, autosave, manual, and Redis-dirty routes moved to the coordinator | - |
| 06 | Terminal Drain and Shutdown Safety | Not Started | Disconnect, rent, death, copyover, and shutdown promotion, drain, and durable spill | - |
| 07 | Immutable World Recovery Worker | Not Started | Sequence-numbered world snapshots, background publication, ACKs, and floor-delta retention | - |
| 08 | Legacy Fork Removal and Recovery Gate | Not Started | Dead-path removal, end-to-end fault/load verification, and operational documentation | - |

---

## Completed Sessions

None yet.

---

## Upcoming Sessions

- Session 01: Player Revision and Component State Foundation

---

## Objectives

1. Give every player checkpoint a monotonic durable revision and explicit dirty
   component identity owned by the game thread.
2. Capture bounded immutable player snapshots without unequipping, removing affects,
   or allowing a worker to traverse live `P_char` or `P_obj` graphs.
3. Apply player snapshots through keyed, coalescing, revision-guarded transactions and
   clear state only after an exact main-thread acknowledgement.
4. Retain all unacknowledged work in a typed, checksummed, versioned, idempotent local
   journal with bounded replay and overload behavior.
5. Move ordinary, manual, autosave, disconnect, rent, death, copyover, and shutdown
   checkpoints onto the new pipeline without duplicate full saves.
6. Remove Redis dirty membership as a durability dependency and delete the forked
   player-save path after the coordinator is authoritative.
7. Replace forked world serialization with a long-lived worker receiving immutable,
   sequence-numbered snapshots and acknowledging exact completion.
8. Prove revision ordering, failure recovery, resource bounds, and simulation-thread
   isolation under representative non-production save and recovery workloads.

---

## Prerequisites

- All Phase 00 sessions are completed, validated, and reconciled by the Phase 00 audit.
- Phase 00 redacted persistence metrics and truthful dirty/save-age reporting are
  available for worker and journal validation.
- Phase 00 failed-save, terminal-transition, Redis-containment, connection, and schema
  safety contracts are preserved.
- Database migrations and fault/load work use only a backed-up development clone and
  non-production ports.
- C/C++ changes use the repository C++20 build, changed-line formatting checks, and
  focused regressions under `tests/async/`.

---

## Planning Assumptions And Resolutions

### Working Assumptions

- The current player save component boundaries are a starting inventory, not the final
  persistence API: `sql_save_player()` already groups status, skills, affects, items,
  pets, and shapechanges, while `writeCharacter()` adds money, playtime, epics,
  trophies, and equipment mutation. This evidence is sufficient to define foundation,
  capture, apply, and cutover windows without freezing the final component enum before
  Session 01 inspects the post-Phase-00 source.
- A long-lived in-process world recovery worker is the initial topology. The repository
  already has the locker worker pattern for main-thread immutable capture and later
  completion, while no sidecar IPC or deployment contract exists. The worker must use
  bounded incremental capture so this choice does not move serialization cost back to
  the simulation thread.
- The typed journal becomes the write target for new unacknowledged player work, but
  existing legacy pfiles are preserved and inventoried rather than deleted. Read-only
  compatibility or an explicit operator report remains until replay evidence supports
  a later retirement decision.
- These session boundaries are stable feature and verification seams. When Phase 01
  becomes active, `plansession` will re-run repository analysis and incorporate actual
  Phase 00 changes and carryforward lessons without changing the phase objective.

### Conflict Resolutions

- Normal `phasebuild` advances `current_phase`, but the user explicitly requested
  advance planning while Phase 00 has not started. The analyzer reports Phase 00 as
  active, ten Phase 00 candidates, zero completed sessions, and Session 00.01 already
  planned. Phase 01 is therefore added as future tracked work while `current_phase` and
  `current_session` remain on Phase 00; this prevents the workflow from skipping ten
  unfinished sessions.
- `CONSIDERATIONS.md` said Phase 00 had no session stubs and no active session, while
  the analyzer and filesystem show ten stubs plus the Session 00.01 spec and checklist.
  The analyzer and present artifacts are authoritative, and the stale consideration is
  corrected during this run.
- The master PRD requires revisioned snapshots in Phase 01 but reserves atomic epic,
  wallet, bank, and item-ownership outcomes for Phase 02. Phase 01 provides ordered
  checkpoint durability only; it does not claim that snapshot revision guards replace
  Phase 02 operation IDs, ledgers, or transactional outboxes.

---

## Technical Considerations

### Architecture

The game thread owns mutable player and object graphs, creates revisions, captures
immutable component values, and applies completions. A player-keyed coordinator retains
dirty, queued, inflight, and acknowledged revisions; coalescing must carry every
unacknowledged component forward so a later partial snapshot cannot make an earlier
dirty component disappear.

The worker owns only immutable typed jobs and borrowed database connections. Its
transaction first establishes that the incoming player revision is newer, applies the
included component set, commits, and returns a result containing player identity,
revision, component mask, and classified outcome. Ambiguous commits are reconciled by
durable revision identity rather than blind retry.

The journal contains typed values, schema version, revision identity, length bounds,
and checksum; it never stores unrestricted raw SQL. World recovery is a separate
sequence and acknowledgement domain so optional report-cache health cannot clear or
validate recovery state.

### Technologies

- C++20 server sources with legacy `.c` filenames and pthread worker patterns
- MySQL or MariaDB with InnoDB and additive guarded migrations
- Typed immutable snapshot structures and bounded player-keyed queues
- Checksummed local journal records with atomic checkpoints and idempotent replay
- hiredis for reconstructible caches and the separately monitored world recovery store
- Python and shell regressions under `tests/async/`
- Non-production fault injection and 25-to-200-client save/recovery workloads

### Risks

- Incomplete dirty coverage can silently omit state: inventory every save and mutation
  entry point, keep unacknowledged bits cumulative, and verify each component round trip.
- An older or partial job can suppress newer data: order work per player, guard the
  transaction by revision, and ignore stale acknowledgements on the game thread.
- Snapshot capture can replace database stalls with CPU stalls: use explicit byte and
  object limits, bounded per-pulse capture, and telemetry around capture age and cost.
- Journal failure can become data loss or an outage amplifier: define atomic append,
  sync, corruption, disk-full, quota, replay, and backpressure behavior before cutover.
- Terminal cutover can extract state too early: promote the existing latest revision,
  require DB acknowledgement or an explicitly durable journal handoff, and retain live
  state whenever neither succeeds.
- Removing forks can expose hidden dependencies: keep both legacy paths disabled but
  available for comparison until replacement fault tests pass, then delete them in the
  final phase session.

### Relevant Considerations

- [P00] **The game thread owns mutable objects**: Snapshot DTOs are sealed on that
  thread, and workers never receive live pointers.
- [P00] **Revision and acknowledgement identity are mandatory**: Sessions 01 and 03
  establish exact revision, component, and completion semantics.
- [P00] **Queues and recovery must be typed and bounded**: Sessions 03 and 04 define
  queue, spill, journal, replay, and overload limits before any trigger cutover.
- [P00] **Both forked snapshot paths are transitional hazards**: Sessions 05, 07, and
  08 replace and then remove player and world persistence forks.
- [P00] **Do not clear or destroy before an exact durable ACK**: Dirty bits, live
  characters, inventory, floor deltas, and journal records survive failures and stale
  completions.
- [P00] **Follow the locker worker boundary**: Its immutable snapshot, generation
  coalescing, and main-thread completion pattern is reused without copying its raw SQL
  payload or synchronous fallback weaknesses.
- [P00] **Filesystem fallback is not a recovery protocol**: Session 04 replaces new
  player fallback work with typed journal records, and Session 06 preserves legacy
  files until an evidence-backed retirement path exists.

---

## Success Criteria

Phase complete when:
- [ ] All 8 sessions completed and validated
- [ ] Every player checkpoint carries a monotonic revision and explicit component mask
- [ ] Workers receive no live `P_char` or `P_obj` pointers and snapshot capture does not
      unequip items or remove/reapply affects
- [ ] An older revision cannot replace a newer durable player revision
- [ ] Dirty components clear only for the exact acknowledged revision and remain set
      when a newer mutation exists
- [ ] Unacknowledged work survives restart through bounded typed journal replay without
      loss, duplicate application, raw SQL records, or silent corruption
- [ ] Ordinary player mutation, autosave, and manual checkpoint paths perform no
      steady-state database, Redis, or filesystem I/O on the simulation thread
- [ ] Terminal transitions promote the existing latest job, avoid duplicate snapshots,
      and retain live state when neither DB acknowledgement nor durable spill succeeds
- [ ] Redis dirty membership and both persistence `fork()` paths are removed
- [ ] World snapshots publish sequence, checksum, timestamp, and validity atomically,
      and floor deltas clear only after the matching acknowledgement
- [ ] Queue bytes, oldest age, retries, journal bytes/age, revision gap, capture/apply
      latency, and worker health are bounded and observable
- [ ] Phase-specific stale-revision, duplicate-replay, outage, worker-crash, disk, Redis,
      shutdown, and 25-to-200-client save/recovery tests pass on non-production systems
- [ ] Relevant focused tests, formatting checks, `make -C src`, and the repository gate
      pass

---

## Dependencies

### Depends On

- Phase 00: Correctness and Immediate Lag Removal
- Phase 00 carryforward, documentation, and phase-transition evidence before execution

### Enables

- Phase 02: Transactional Gameplay Domains
- Ordered checkpoint and journal foundations reusable by typed economy and ownership
  commands
- Reliable revision and queue telemetry for the final 200-player capacity gate
