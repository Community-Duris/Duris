# Session Specification

**Session ID**: `phase00-session03-save-failure-retry-and-terminal-safety`
**Phase**: 00 - Correctness and Immediate Lag Removal
**Status**: Not Started
**Created**: 2026-08-27
**Base Commit**: `4f49a11f`
**Work Window**: One durable-save failure contract from deferred scheduling through bounded retry, explicit flush results, inventory restoration, and every player or locker extraction that depends on a terminal save. The window ends when failed camp, rent, death, idle/link-loss cleanup, ghost extraction, copyover, shutdown, and offline artifact/locker transitions retain live recoverable state.

---

## 1. Session Overview

The existing deferred-save table records failures but clears its scheduled bit before attempting the save and never queues another callback. A later request finds the occupied slot, updates it, and returns without rescheduling, leaving work permanently pending but inert. Synchronous flushes also return no result, so terminal callers cannot distinguish a completed prerequisite from a retained failure.

`writeCharacter()` compounds the problem for terminal rent types: after SQL failure, including after a flat fallback record is attempted, it still extracts equipped and carried objects before returning false. Several callers then extract the character or locker anyway. The resulting alert may accurately say SQL failed while the live recovery source has already been destroyed.

This session establishes a narrow fail-closed contract. It does not introduce Phase 01 revisions or claim that the binary fallback is replayable database durability. It keeps failed state live, retries ordinary deferred saves with bounded backoff, and permits terminal destruction only after the required save result succeeds.

---

## 2. Objectives

1. Keep the newest coalesced deferred-save request scheduled until success, with bounded exponential backoff and saturating telemetry.
2. Return truthful success from direct and global deferred flushes and prevent redundant full saves after a successful flush.
3. Restore equipment, affects, and carried inventory on every failed `writeCharacter()` terminal path, regardless of flat-fallback outcome.
4. Gate every destructive player and locker terminal transition on its save prerequisite, including copyover and shutdown preparation.
5. Distinguish durable SQL success, retryable failure, fallback-record success, and fallback-record failure in redacted operator alerts.

---

## 3. Prerequisites

- [x] `phase00-session01-redacted-persistence-observability` - supplies redacted alerts and deferred-save snapshots.
- [x] `phase00-session02-in-memory-epic-bonus-hot-path` - confirms the active phase baseline and current build/test count.

### Environment Requirements

- Use only local/development runtime paths selected by `.env`; do not print credentials.
- Do not run migrations, wipes, or production operational scripts; this session changes no schema.
- Compile touched `.c` files as C++20 and verify changed lines with `.clang-format`.

---

## 4. Scope

### In Scope

- Extend each fixed deferred slot with explicit scheduled/retry timing and retain the latest type, level-dirty request, reason, first/latest age, attempts, and failures.
- Queue one retry event after failure using a documented bounded backoff; a new request updates semantics and ensures an unscheduled slot becomes scheduled.
- Make one-character and all-character flush APIs return success; clear only successful slots and retain failed live slots.
- Make `writeCharacter()` fail closed: a failed terminal database save re-equips the exact saved equipment, leaves inventory attached, reapplies affects, and returns false even if a separate fallback record was written.
- Gate destructive paths in quit/camp/rent/death/idle rent, ghost extraction, link-loss cleanup, copyover/shutdown preparation, offline artifact dummy handling, and legacy terminal locker handling.
- Preserve successful path ordering and eliminate a second full save when a pending flush already saved equivalent terminal state.
- Add runtime/source-contract failure tests and update operator documentation.

### Outside This Work Window

- Monotonic revisions, component dirty masks, immutable save DTOs, typed journals, and worker acknowledgements - Phase 01.
- Treating the legacy binary pfile as an automatically reconciled or authoritative recovery log.
- Transactional artifact ownership, corpse, epic, wallet, or item ledgers - Phase 02.
- Unrelated gameplay changes to rent, death, camps, artifacts, or lockers.
- Database schema or index changes.

---

## 5. Technical Approach

### Deferred State Machine

Keep the existing fixed 512-slot game-thread-owned table. Add a small scheduling helper that owns `scheduled`, retry delay, and event creation. The callback copies request semantics, clears only the event-in-flight marker, increments attempts, and either clears on success or increments failures and queues exactly one retry. Backoff is exponential in pulses with a short initial delay and a fixed maximum; arithmetic saturates. A later request coalesces type and dirty-level intent, updates the latest timestamp/reason, and schedules the slot if no event is active.

Flush functions return `bool`. A successful explicit flush clears the slot, making the already queued event a harmless no-op. Failure retains or re-arms the slot. Global flush reports false if any live pending character fails; missing/extracted characters are reported categorically rather than counted as durable success.

### Terminal Save Contract

Refactor the end of `writeCharacter()` around `result`: only a successful terminal database save may extract serialized inventory. On failure, use the existing `save_equip` array to re-equip the original objects, leave carried objects in place, reapply affects, run the existing locker post-hook, and return false. Record fallback success separately; it is recovery evidence, not SQL durability.

Callers must check the returned result before irreversible extraction, object nuking, racewar/offline publication, or success copy. Where a pending deferred save is being superseded by a terminal save, consume its latest semantics without performing two full player saves. Link-loss retains an in-memory character and schedules retry. Copyover must abort before process replacement when pending or final saves fail. Normal shutdown/reboot must complete the player-save gate before destructive extraction or process exit; a failure cancels the terminal transition and leaves the server/live characters available for retry.

### Behavioral Decisions

- Existing active game objects are the only complete retry source until Phase 01 adds a journal.
- A flat fallback record is reported as `fallback_recorded` but does not authorize terminal extraction because automatic reconciliation is explicitly out of scope.
- Player-facing failure copy is concise and actionable; raw database errors, names in persistence alerts, SQL, and fallback paths remain excluded.
- All retry state and event scheduling stay on the game thread and perform no new worker traversal of `P_char`.

---

## 6. Deliverables

### Files To Create

| File | Purpose |
|------|---------|
| `src/deferred_save_policy.h` | Publish dependency-free bounded retry and terminal-inventory extraction policy contracts |
| `src/deferred_save_policy.c` | Implement deterministic capped backoff and the durable-success extraction decision for runtime harness coverage |
| `tests/async/test_deferred_save_retry.py` | Runtime/source contracts for coalescing, retry scheduling, backoff, flush results, and no stranded slots |
| `tests/async/test_terminal_save_safety.py` | Inventory restoration and destructive-caller gate contracts across player, artifact, locker, copyover, and shutdown paths |

### Files To Modify

| File | Changes |
|------|---------|
| `src/Makefile` | Link the dependency-free deferred-save policy module into the C++20 server |
| `src/actoth.c` | Retryable deferred state, scheduling helper, boolean flushes, terminal quit gates, and truthful alerts |
| `src/actwiz.c` | Gate ghost character extraction on terminal persistence success |
| `src/prototypes.h` | Publish boolean flush and terminal-preparation contracts |
| `src/files.c` | Preserve equipment/inventory on failed terminal saves and distinguish fallback outcome |
| `src/affects.c` | Refuse camp extraction on failed terminal save |
| `src/fight.c` | Refuse player death extraction on failed death save |
| `src/specs.room.c` | Gate inn rent and heaven/death extraction |
| `src/limits.c` | Gate idle-rent extraction and avoid redundant link-loss saves |
| `src/comm.c` | Make link-loss alerts truthful and abort shutdown/reboot destruction on save failure |
| `src/copyover.c` | Abort copyover on pending-flush failure and preserve active process state |
| `src/copyover.h` | Expose truthful copyover success/failure to the live game loop |
| `src/artifact.c` | Preserve offline dummy owner state when artifact terminal saves fail |
| `src/storage_lockers.c` | Keep legacy deferred terminal locker characters live on save or commit failure |
| `tests/async/test_deferred_save_flush.py` | Update existing flush contracts for boolean, retryable behavior |
| `tests/async/test_copyover_save_guards.py` | Require pending flush and process-replacement failure gates |
| `tests/async/test_locker_terminal_fallback_contract.py` | Cover legacy deferred terminal locker failure retention |
| `docs/DATABASE.md` | Document retry timing, terminal durability gate, and fallback truthfulness |
| `docs/RUNBOOK.md` | Document alerts and operator recovery actions for retained terminal failures |

---

## 7. Success Criteria

### Functional

- [ ] Failed deferred saves remain pending and scheduled with bounded retry; newer requests cannot become stranded.
- [ ] Retry delay grows to a fixed ceiling, counters saturate, and successful retry clears exactly one slot.
- [ ] Direct/global flush APIs report failure truthfully, retain failed live work, and do not duplicate a successful full save.
- [ ] Failed terminal `writeCharacter()` calls restore equipment and affects and leave all inventory reachable.
- [ ] Flat-fallback success is visible but never mislabeled as database durability or used to authorize extraction.
- [ ] Camp, rent, death, idle/link-loss cleanup, ghost extraction, artifact dummy, locker, copyover, shutdown, and reboot paths refuse irreversible completion after save failure.
- [ ] Successful terminal behavior, rent types, room selection, gameplay messages, and save order remain compatible.

### Testing And Quality

- [ ] Focused tests exercise initial failure, repeated backoff, coalesced updates, retry success, explicit flush success/failure, missing-character state, and capacity behavior.
- [ ] A standalone failure harness proves terminal equipment/inventory restoration after SQL and fallback failure.
- [ ] Source contracts inventory every destructive terminal `writeCharacter()` caller and require a success gate before extraction or nuke.
- [ ] Existing deferred, copyover, locker, auction, epic, ship, persistence-observability, and save-guard regressions pass.
- [ ] All created or modified files are ASCII with Unix LF endings; formatting, C++20 build, and `make test-all` pass.

### Non-Functional

- [ ] Deferred storage remains fixed-capacity, game-thread-owned, allocation-free, and bounded per event.
- [ ] Retry scheduling adds no new external I/O beyond the existing save attempt.
- [ ] No raw SQL, database error prose, credentials, player/account data, or fallback path enters new diagnostics.
- [ ] No schema, migration, dependency, or stronger recovery guarantee is introduced.

---

## 8. Risks And Resolutions

- **Shutdown resume risk**: cleanup currently begins only after the main loop exits. Establish the player durability gate before destructive subsystem shutdown and reset the requested terminal mode when the gate fails, so the live process resumes instead of exiting.
- **Copyover failure risk**: `copyover_save()` currently runs after the loop and process-worker teardown. Move or wrap the gate so failed character persistence returns to the running process before `exec`/exit; do not close preserved descriptors until all prerequisite saves pass.
- **Global save buffer risk**: `writeCharacter()` uses the global `save_equip` array. Keep the game-thread-only invariant and restore/clear every slot on all exits.
- **Commit ambiguity**: do not label a failed commit or binary fallback as SQL success. Retain state and require a later explicit retry.
- **Scope breadth**: distinguish destructive terminal callers from ordinary best-effort checkpoints. Only the former must block gameplay transition in this session.

### Behavioral Quality Focus

Checklist active: Yes

Highest-risk behaviors are inventory destruction after a false result, duplicate saves that overwrite newer terminal semantics, process exit after a failed shutdown/copyover gate, and dummy/locker extraction after failed persistence.

---

## 9. Testing Strategy

- Compile a focused harness around the deferred scheduling state or expose dependency-free delay helpers for deterministic saturation/backoff tests.
- Use source contracts to verify event rescheduling, coalescing, boolean propagation, and every save-before-extract order.
- Exercise `writeCharacter()` failure restoration with controlled save/fallback seams where practical; supplement with structural contracts for legacy integration.
- Run the nearest existing guards first, then formatter, server build, and all repository regressions.
- Perform local non-production runtime smoke coverage for ordinary save/camp cancellation when a safe failure seam is available; do not corrupt the development database to induce failure.

---

## 10. Dependencies

- Depends on Sessions 01 and 02.
- Enables Phase 01 revisioned save workers and journal recovery by making current failure state truthful and non-destructive.

---

## Next Steps

Run the `implement` workflow step.
