# DurisMUD - Product Requirements Document

## Source Review Record

- **Requirements baseline:** Database Integration and 200-Player Scalability Review
- **Date:** August 26, 2026
- **Status:** Phases 00-03 complete; representative 200-account/four-hour capacity run deferred
- **Scope:** MySQL/MariaDB, Redis, player load/save, event persistence, epic points,
  combat-triggered queries, shared currency, lockers, and operational failure behavior
- **Canonicality:** This PRD incorporates the complete review, including its method,
  limits, evidence, demand model, DB-001 through DB-023 findings, recommendations,
  target design, phase plan, validation plan, acceptance criteria, and positive
  findings. The detailed baseline is retained below with ASCII punctuation so this
  file remains self-contained if the original review file is later removed.

## Overview

DurisMUD needs a persistence system that can safely support 200 active players while
its single game thread continues to process commands, combat, world ticks, and events.
The reviewed system is not ready for that load. The primary constraints are synchronous
MySQL and Redis operations on the simulation thread and stale, forked full-player
snapshots that can overwrite newer durable state.

The product outcome is a revisioned, immutable, idempotent persistence architecture.
Critical economy and ownership actions must commit exactly once at a defined
transaction boundary; ordinary player changes must be coalesced by dirty component;
and workers must acknowledge exact revisions before the game clears dirty state.
Failure must retain retryable state without converting an external-service outage into
blocking game-thread work.

The delivery scope includes immediate correctness and lag fixes, replacement of forked
full saves, transactional gameplay domains, login/schema/retention improvements,
observability, and a 200-player load and fault-injection gate. Existing transactional,
dedupe, connection-pool, and immutable locker-worker foundations are retained and
generalized.

## Goals

1. Prevent stale saves, partial terminal transitions, and failure paths from losing,
   duplicating, reviving, or overwriting player-critical state.
2. Remove steady-state database, Redis, and filesystem I/O from per-pulse callbacks and
   normal mutation paths on the simulation thread.
3. Establish ordered, revision-guarded, immutable, idempotent persistence with bounded
   queues, explicit completion, retry, spill, and replay behavior.
4. Give epic points, account-bank money, player wallets, item ownership, and their
   ledgers or audit events atomic and reconcilable durability boundaries.
5. Load each player from one complete durable revision or fail cleanly, without N+1
   amplification, fixed silent truncation, or partial-success semantics.
6. Bound maintenance, queue, connection, cache, and recovery work so the game remains
   responsive through the defined 200-player workloads and injected failures.
7. Make capacity, revision freshness, queue age, external-I/O latency, fallback state,
   and schema compatibility observable and operationally actionable.

## Non-Goals

- Increasing database connection count alone and treating that as 200-player readiness.
- Using Redis as the authoritative dirty-player registry or adding Redis round trips to
  hot player callbacks.
- Treating the four-player local database, its 108 player items, `Slow_queries=0`, or
  non-production `EXPLAIN` estimates as proof of production capacity.
- Running migrations, write tests, destructive operations, or load tests against
  production; all such validation uses a backed-up development clone and non-production
  ports.
- Adding every candidate index without production-clone row counts, `EXPLAIN ANALYZE`,
  and write-amplification measurement.
- Deleting financial, ownership, or other append-only history without a documented
  retention, reconciliation, and audit policy.
- Letting persistence workers traverse mutable live `P_char` or `P_obj` graphs or
  forking the running multithreaded server as the target architecture.
- Redesigning unrelated gameplay; persistence changes preserve gameplay semantics while
  making completion, retry, and failure behavior explicit.

## Users and Use Cases

### Primary Users

- **Active player:** Expects combat, regeneration, rewards, banking, inventory, login,
  disconnect, death, rent, and recovery to remain responsive and durable.
- **Gameplay owner:** Defines acceptable recovery windows and verifies that persistence
  changes preserve economic and gameplay semantics.
- **Server developer:** Implements and tests persistence commands, snapshots, schema,
  workers, caches, and failure handling without touching production data.
- **Operator:** Deploys compatible schemas, observes health and revision freshness,
  handles degraded dependencies, and verifies recovery without reading private values.

### Key Use Cases

1. A player regenerates, gains XP or epics, moves items, banks money, and continues
   playing without synchronous external I/O stalling the simulation loop.
2. Multiple characters on one account update a shared bank without duplication, loss,
   or stale absolute-balance overwrite.
3. A player camps, rents, dies, disconnects, enters copyover, or shuts down while the
   latest revision is promoted, acknowledged, or safely retained for retry.
4. A reconnecting player with a large inventory and pets loads one internally
   consistent revision, or login fails closed with no partial character.
5. A worker, database, Redis instance, filesystem, or game process fails at any defined
   crash point and idempotent replay converges without loss or duplication.
6. An operator can distinguish cache health, queue health, journal durability, recovery
   snapshot health, connection state, and each player's latest durable revision.
7. A developer can validate fixes with focused regressions, production-sized cloned
   histories, a 25-to-200-client ramp, and the complete fault-injection matrix.

## Requirements

### MVP Requirements

- A player can receive hit regeneration, movement regeneration, and XP epic bonuses
  from in-memory state without a database or Redis operation in the callback.
- The save coordinator can retain and retry a failed deferred save with bounded backoff,
  truthful state, and no permanently stranded player slot.
- A player can remain live and fully retryable when a terminal save fails; inventory and
  character extraction occur only after the required durable completion.
- A player can clear timers, undead spell slots, forged-item knowledge, and granted
  commands and observe those removals after save and relog.
- PvP and artifact processing can persist post-mutation frag values and deterministic
  artifact-bind outputs on every success and failure path.
- The game runtime can tolerate Redis connection or command failure within bounded
  deadlines without a synchronous full-player-save fallback in the mutation caller.
- The recovery coordinator can recover or merge an existing dirty inflight set, bound
  temporary child runtime, and retain floor deltas until the matching world snapshot
  succeeds.
- An account holder can deposit and withdraw through checked delta-only bank operations
  without any character overwriting the shared balance from a stale cached copy.
- An operator can observe redacted query timing by stable call site plus dirty-state and
  save age before the system is evaluated at scale.

### Deferred Requirements

- A player can persist ordinary mutations through monotonic player revisions and dirty
  component masks rather than full-state rewrites.
- A persistence worker can apply an immutable player snapshot only when its revision is
  newer, then return an exact completion that the game thread applies safely.
- The runtime can coalesce keyed player work, recover unacknowledged work from a typed,
  checksummed, idempotent journal, and delete the forked player-save path.
- A disconnect, copyover, or shutdown can promote and drain the existing latest job to
  a bounded deadline, spill the remainder durably, and avoid duplicate snapshots.
- World recovery can use a separately started worker or sidecar with immutable,
  sequence-numbered snapshots, checksums, completion acknowledgements, and bounded
  runtime.
- A player can earn or spend epics exactly once while balance, immutable ledger, and
  required downstream notification state commit atomically and reconcile exactly.
- Two characters on one account can transfer value between wallet and bank in one
  idempotent transaction and receive the committed shared balance.
- Players can trade, store, loot, auction, recover, or move an item across owners while
  current ownership, both inventory revisions, and the audit outbox commit atomically.
- Combat, artifact, boon, reward, and zone-touch outcomes can persist as typed, batched,
  idempotent commands without per-recipient raw-query fan-out.
- Event fallback and replay can process typed, schema-versioned records with mandatory
  unique event IDs and no unrestricted raw SQL durable messages.
- A player can log in through one consistent read transaction with batched ownership,
  item, pet, affect, and description data assembled in O(N) time.
- Gameplay and maintenance queries can use measured, production-clone-validated access
  paths without N+1 recent-death, random-zone, trophy, pet, or ownership queries.
- Operators can apply an immutable, checksummed migration ledger and verify the complete
  required schema and connection configuration before boot writes any lookup data.
- Operators can enforce documented season/time retention and archival for every growing
  history table without violating reconciliation or audit requirements.
- Scheduled maintenance can run with deterministic staggering, bounded rows or time,
  continuation cursors, and no recurring exact-modulus whole-game latency spike.
- Developers and operators can rely on corrected database, architecture, configuration,
  and runbook documentation that matches the implemented persistence routes.

## Non-Functional Requirements

- **Scale workload:** Ramp 25, 50, 100, and 200 clients, then hold 200 for at least 30
  minutes under each of the eight defined workload profiles using representative cloned
  history sizes.
- **Simulation latency:** Keep p99 game-pulse duration below 250 ms with no sustained
  new-event debt under the complete 200-player workload mix.
- **Event latency:** Keep p99 new-event processing within the existing 25 ms per-pulse
  budget under the complete 200-player workload mix.
- **Hot-path isolation:** Execute zero steady-state database or Redis calls from
  per-pulse player callbacks and zero main-thread external I/O in normal mutation paths.
- **Command freshness:** Keep normal oldest critical-command age below 1 second and
  checkpoint age at or below the documented recovery-point objective.
- **Durability:** Produce zero lost or duplicate epic, currency, or item-ownership
  outcomes across every defined crash point and duplicate-replay case.
- **Revision ordering:** Permit zero cases in which an older player or world revision
  replaces a newer committed revision.
- **Terminal safety:** Preserve the live character and inventory in 100% of injected
  terminal-save failures so the transition remains retryable.
- **Reconciliation:** Reconcile database balance and current-owner rows exactly with
  their ledgers after every workload and fault run.
- **Login integrity:** Load exactly one complete player revision or fail login cleanly;
  zero partial-character successes are permitted.
- **Resource bounds:** Stay below configured queue byte and age hard limits, apply
  explicit backpressure or durable spill, and avoid process OOM during the defined
  30-second dependency outage and worker-failure tests.
- **Security and privacy:** Emit zero raw SQL statements, bound private values, password
  hashes, confirmation values, player descriptions, or IP addresses in failure logs;
  all connection sessions must verify charset, time zone, isolation level, and SQL mode.
- **Transport security:** Use MySQL TLS or a protected local socket/tunnel for 100% of
  non-local database connections.
- **Compatibility:** Pass the smallest relevant focused regressions after each behavior
  change, `make -C src` after each C/C++ change, and the full repository gate before
  phase completion.

## Constraints and Dependencies

- The server runs four 250 ms pulses per second, and its new-event scheduler normally
  budgets 25 ms of callback work per pulse.
- The game thread remains the sole owner of mutable `P_char` and `P_obj` state.
- Existing `.c` server sources are compiled as C++20 with `g++`; changes must remain
  narrow and follow nearby legacy style and `.clang-format`.
- MySQL or MariaDB with InnoDB remains the durable system of record. Redis is optional
  and may hold only reconstructible cache or explicitly bounded recovery-domain data.
- The local database evidence covers 4 players, 108 player items, 124 InnoDB tables,
  and no production-like latency histograms because Performance Schema was unavailable.
- Migrations live in `migrations/`, must be additive, guarded, and re-runnable where
  practical, and must never be exercised against production.
- Schema and load work requires a backed-up development clone, representative history
  sizes, and non-production game ports.
- Critical event commands require unique operation IDs; snapshot domains require
  monotonic revisions; audit and cross-system effects require transactional outbox or
  inbox semantics.
- Queue bytes, oldest age, operation deadlines, retry classification, circuit state,
  and journal replay must be bounded and observable.
- Session planning must keep one coherent outcome or tightly related objective cluster,
  sized from the working set, dependencies, risk, verification boundary, and practical
  model capacity. No fixed duration or task-count cap applies; `phasebuild` owns phase
  trackers and session stubs.

## Phases

This system delivers the remediation via phases. Each phase is implemented through
adaptively sized sessions whose specs define coherent implementation and verification
boundaries.

| Phase | Name | Sessions | Status |
|-------|------|----------|--------|
| 00 | Correctness and Immediate Lag Removal | 10 | Complete |
| 01 | Replace Forked Full Saves | 8 | Complete |
| 02 | Transactional Gameplay Domains | 12 | Complete |
| 03 | Load Path, Schema, and Retention | 14 | Complete |

## Phase 00: Correctness and Immediate Lag Removal

### Objectives

1. Cache epic-bonus state in memory and remove SQL from hit regeneration, movement
   regeneration, and XP calculation.
2. Repair deferred-save retry, rescheduling, state reporting, and alert truthfulness.
3. Require durable success before terminal extraction or live-inventory destruction.
4. Delete obsolete replacement-subtable rows before inserting current player values.
5. Correct victim frag update order and initialize artifact-bind outputs on every path.
6. Bound Redis connections and commands, guard every context, and remove synchronous
   full-save failure fallback.
7. Recover dirty inflight state at boot, temporarily bound both fork paths, and retain
   floor deltas until matching world-snapshot acknowledgement.
8. Make account-bank operations checked and delta-only while the transactional ledger
   is prepared.
9. Add redacted call-site query timing plus dirty-state and save-age metrics.

### Sessions

| Session | Name | Status |
|---------|------|--------|
| 01 | Redacted Persistence Observability | Complete |
| 02 | In-Memory Epic Bonus Hot Path | Complete |
| 03 | Save Failure Retry and Terminal Safety | Complete |
| 04 | Player Replacement State Cleanup | Complete |
| 05 | Combat and Artifact Persistence Correctness | Complete |
| 06 | Redis Failure and Recovery Containment | Complete |
| 07 | Account Bank Delta Safety | Complete |
| 08 | Runtime Connection Trust Boundaries | Complete |
| 09 | Private Chest Password Hardening | Complete |
| 10 | Security Policy and Dependency Baseline | Complete |

Detailed scopes and verification boundaries are tracked in the Phase 00 PRD and its
`session_NN_name.md` stubs under `.spec_system/PRD/phase_00/`.

## Phase 01: Replace Forked Full Saves

Phase 01 is complete and archived. Its revisioned save and recovery contracts remain
the foundation for later transactional and load work.

### Objectives

1. Add monotonic player revisions and explicit dirty component state.
2. Capture immutable player snapshot DTOs on the game thread without unequipping or
   exposing live game objects to workers.
3. Apply keyed, coalescing player jobs through revision-guarded transactions and exact
   main-thread acknowledgements.
4. Retain unacknowledged work in a typed, checksummed, idempotent local journal.
5. Move nonterminal and terminal player-save triggers to the revisioned pipeline.
6. Replace forked world recovery with an immutable sequence-numbered worker pipeline.
7. Delete both persistence fork paths and pass the Phase 01 recovery and load gate.

### Sessions

| Session | Name | Status |
|---------|------|--------|
| 01 | Player Revision and Component State Foundation | Complete |
| 02 | Immutable Player Snapshot Capture | Complete |
| 03 | Keyed Revision-Guarded Save Worker | Complete |
| 04 | Typed Persistence Journal and Replay | Complete |
| 05 | Nonterminal Save Pipeline Cutover | Complete |
| 06 | Terminal Drain and Shutdown Safety | Complete |
| 07 | Immutable World Recovery Worker | Complete |
| 08 | Legacy Fork Removal and Recovery Gate | Complete |

Detailed scopes and verification boundaries are tracked in the Phase 01 PRD and its
`session_NN_name.md` stubs under `.spec_system/PRD/phase_01/`.

## Phase 02: Transactional Gameplay Domains

Phase 02 is complete and archived. Its operation-keyed transactional domains and
reconciliation contracts are the authority used by Phase 03.

### Objectives

1. Extend the Phase 01 typed journal and coordinator with stable non-coalescing critical
   operation IDs, deterministic multi-key ordering, gameplay fences, and exact results.
2. Add a generic transactional inbox/outbox contract with duplicate-result lookup,
   ambiguous-commit reconciliation, and bounded at-least-once delivery.
3. Make epic awards/spends and account-bank/player-wallet deltas atomic, idempotent,
   and exactly reconcilable with immutable ledgers.
4. Add authoritative current item ownership and an immutable ownership ledger, then
   cut over live movement, corpses, lockers, and auctions.
5. Batch PvP, artifact, guild, boon, reward, and zone outcomes as immutable typed
   commands with set-based transactions and post-commit publication.
6. Remove unrestricted raw non-idempotent SQL from durable queues and pass the Phase 02
   crash, replay, reconciliation, and 25-to-200-client gate.

### Sessions

| Session | Name | Status |
|---------|------|--------|
| 01 | Critical Operation Identity and Coordinator | Complete |
| 02 | Transactional Inbox, Outbox, and Reconciliation | Complete |
| 03 | Epic Ledger and Balance Transactions | Complete |
| 04 | Account Bank and Wallet Transactions | Complete |
| 05 | Item Ownership Ledger and Transfer Primitive | Complete |
| 06 | Live Item Movement and Corpse Cutover | Complete |
| 07 | Locker Ownership Cutover | Complete |
| 08 | Auction Settlement and Claim Cutover | Complete |
| 09 | PvP and Combat Outcome Batching | Complete |
| 10 | Artifact and Guild Outcome Batching | Complete |
| 11 | Boon, Reward, and Zone Command Batching | Complete |
| 12 | Raw Event Queue Retirement and Domain Gate | Complete |

Detailed scopes and verification boundaries are tracked in the Phase 02 PRD and its
`session_NN_name.md` stubs under `.spec_system/PRD/phase_02/`.

## Phase 03: Load Path, Schema, and Retention

Phase 03 is complete and archived. It is the final phase defined by this remediation
plan; no later phase is defined or active.

### Objectives

1. Load players and pets from one bounded consistent read transaction, batch item
   ownership and metadata, and assemble object graphs in O(N) time without silent
   truncation or partial publication.
2. Replace recent-PvP-death and epic-task read fan-out with set-based or in-memory paths
   that preserve gameplay semantics.
3. Measure final query shapes and write costs on a representative development clone,
   then apply only evidence-backed indexes and sargable query changes.
4. Stagger and bound scheduled maintenance with typed jobs, row/time budgets,
   continuation cursors, and no simulation-thread external I/O.
5. Define and enforce approved retention, archival, export, erasure, and restore-
   tombstone rules for every database and non-database record class.
6. Establish an immutable checksummed migration ledger and verify the complete schema,
   connection, and versioned lookup contract before boot writes data.
7. Reconcile README and operator/developer documentation to the implemented system and
   pass the integrated eight-profile 25-to-200-client workload and fault matrix.

### Sessions

| Session | Name | Status |
|---------|------|--------|
| 01 | Consistent Player Load Transaction | Complete |
| 02 | Batched Item Ownership and Linear Assembly | Complete |
| 03 | Batched Pet Graph Hydration | Complete |
| 04 | Set-Based PvP and Epic Task Reads | Complete |
| 05 | Production-Clone Query Plan and Index Gate | Complete - local fixture unqualified |
| 06 | Bounded Maintenance Scheduler | Complete |
| 07 | Data Processing and Retention Contract | Complete - policy pending |
| 08 | Retention and Archival Execution | Complete - canonical mutation disabled |
| 09 | Authenticated Personal Data Export | Complete - canonical activation disabled |
| 10 | Account Erasure and Backup Propagation | Complete - canonical mutation disabled |
| 11 | Immutable Migration Ledger and Runner | Complete |
| 12 | Boot Schema and Lookup Compatibility | Complete |
| 13 | Documentation and Operator Contract | Complete |
| 14 | Final 200-Player and Compliance Gate | Complete - representative run deferred |

Detailed scopes and verification boundaries are tracked in the Phase 03 PRD and its
`session_NN_name.md` stubs under `.spec_system/PRD/phase_03/`.

## Technical Stack

- **C++20 server:** Legacy C-style `.c` files compiled with `g++` under C++20.
- **Single-threaded simulation loop:** `select()`-driven commands, combat, world ticks,
  and four 250 ms pulses per second.
- **Persistence workers:** Existing item, scalar, large-payload, and locker worker
  patterns with a bounded MySQL/MariaDB connection pool.
- **MySQL/MariaDB and InnoDB:** Durable player, account, economy, ownership, event,
  world, and operational data.
- **Redis via hiredis:** Optional derived caches and currently mixed dirty/recovery
  roles that this project separates and bounds.
- **cJSON and local filesystem records:** Current world-snapshot serialization and
  fallback mechanisms to replace or constrain through typed journals and checksums.
- **SQL and Bash migrations:** Authoritative schema material and guarded runner under
  `migrations/`.
- **Make, `g++`, and clang-format:** Build and formatting toolchain; server output is
  `bin/server/dms_new`.
- **Python, Bash, and isolated MySQL tests:** Focused regression/source-contract tests
  under `tests/async/`, with Docker/MySQL suites when database behavior is exercised.

## Success Criteria

- [ ] Zero steady-state database or Redis calls execute from per-pulse player callbacks.
- [ ] Normal mutation paths perform no external I/O on the main game thread.
- [ ] The 200-player workload keeps p99 pulse duration below 250 ms with no sustained
      new-event debt.
- [ ] The 200-player workload keeps p99 new-event processing within the 25 ms budget.
- [ ] Normal oldest critical-command age remains below 1 second and checkpoint age
      remains within the documented recovery-point objective.
- [ ] Queues remain within explicit memory and age bounds, apply backpressure or durable
      spill, and do not OOM during outage tests.
- [ ] Every crash and replay test produces zero lost or duplicate epic, currency, or
      item-ownership outcomes.
- [ ] A failed terminal save leaves the live character and inventory intact and
      retryable.
- [ ] Balance and current-owner rows reconcile exactly with their ledgers.
- [ ] A stale revision never replaces a newer revision.
- [ ] Login loads one complete revision or fails cleanly, never a partial character.

## Risks

- **Stale snapshot overwrite:** Revision-guard every apply and clear dirty state only
  for the exact acknowledged revision.
- **Simulation-thread stalls:** Move steady-state external work behind completion-based
  jobs with short deadlines, retry classification, and circuit breakers.
- **Ambiguous critical commits:** Assign mandatory operation IDs and make ledger,
  balance, ownership, and outbox writes idempotent and transactional.
- **Failure-induced data loss:** Retain live state and dirty revisions, journal
  unacknowledged work, and fail terminal transitions closed.
- **Queue outage amplification:** Bound queue bytes and age, preallocate compact storage,
  batch compatible work, and spill idempotently before hard limits.
- **Post-fork deadlock and memory pressure:** Remove player and world fork paths after
  temporary deadlines and inflight recovery guards are in place.
- **Login inconsistency and growth:** Use one consistent read, batched metadata, O(N)
  assembly, validated limits, and production-sized histories.
- **Unsafe schema tuning:** Measure clone plans and write amplification before applying
  candidate indexes or retention changes.
- **Private-data disclosure:** Replace raw-query logs and ad hoc traces with redacted
  stable site IDs, durations, error codes, sampling, bounds, and rotation.
- **Documentation drift:** Treat traced code behavior as current evidence and update the
  database, architecture, configuration, and runbook documentation in Phase 3.

## Assumptions

- **The August 26, 2026 review is the authoritative remediation baseline:** It records
  completed static tracing and read-only local inspection, while explicitly marking
  remediation as not started. The PRD can proceed because every proposed requirement
  maps to that evidence or the repository's documented build and safety constraints.
- **The initial acceptance gate is binding for planning but can be tightened by gameplay
  owners:** The source calls it a reasonable initial gate and explicitly reserves final
  target ownership. Planning can proceed because the stated figures are measurable and
  conservative enough to expose the confirmed bottlenecks.
- **Production capacity must be established on representative cloned data:** The local
  database is too small and lacks Performance Schema histograms. This does not block the
  PRD because the source supplies an explicit clone-only workload and fault plan.
- **The detailed baseline below is retained as normative engineering context:** The user
  requires the source document to become deletable without information loss. Its full
  substance is embedded rather than summarized away, with only ASCII punctuation and
  link-path normalization.

### Conflict Resolutions

- **Documentation versus traced implementation:** `docs/DATABASE.md` describes broader
  worker coverage than the code provides. Traced code and this review govern planning;
  documentation correction is a Phase 3 deliverable.
- **Redis as optional cache versus durability coordinator:** The target interpretation
  keeps Redis for reconstructible cache data, keeps player dirty state in process plus a
  durable journal, and treats world recovery as an independently monitored domain.
- **Legacy pfile fallback versus durable journal:** The source identifies two honest
  policies and explicitly prefers a versioned write-ahead journal. The PRD adopts that
  preferred target; until it exists, pfile alerts must state that recovery is not
  automatic and terminal failures must retain live state.
- **Connection count versus game-loop exposure:** The source concludes connection count
  is not the limiting issue. Work prioritizes main-thread isolation, ordering, and
  bounded recovery before pool expansion or database scaling claims.

## Open Decisions

1. Final checkpoint recovery-point objective and any tighter gameplay-specific latency
   thresholds, to be owned by gameplay stakeholders before the final load gate.
2. Exact queue byte limits, age high/low watermarks, spill thresholds, and overload
   policy for each typed command and snapshot class.
3. Whether the epic ledger plus snapshots or the current balance row is the canonical
   epic authority, provided reconciliation proves exact agreement.
4. Whether world recovery runs in a long-lived in-process worker or an independently
   started sidecar; either choice must preserve immutable sequence/ACK semantics.
5. Which candidate composite indexes survive production-clone `EXPLAIN ANALYZE` and
   measured write-amplification review.
6. Per-table season/time retention, archival, and audit obligations for every
   append-only history table.
7. The transition and retirement policy for legacy pfiles after the typed journal and
   boot reconciliation path are proven.
8. The deployment-specific mechanism for non-local database transport security: native
   MySQL TLS or a protected local socket/tunnel.

## Detailed Engineering Baseline and Remediation Record

The remainder of this PRD preserves the complete review record. Terms such as
"Confirmed" and "Risk," code evidence, measured constants, caveats, recommendations,
and candidate changes retain their original meaning.

### Executive conclusion

The current persistence system should **not** be considered ready for 200 active
players without changes. The limiting issue is not database server connection count. It
is synchronous database and Redis work on the single game thread, combined with a
player-save mechanism that can commit stale forked snapshots.

The most urgent findings are:

1. **While a player is regenerating hit points, the active regeneration event performs
   a synchronous epic-bonus lookup every 250 ms.** If 200 players are simultaneously
   damaged, that is approximately **800 main-thread reads per second** before movement
   regeneration, combat XP, epic awards, commands, or saves. Players whose selected
   bonus is health regeneration add an aggregate over `epic_gain` to every callback.
2. **The Redis dirty-player flush forks a multithreaded server and writes old, full
   player snapshots without a revision check.** A child can commit after a newer save
   and revert currency, epics, inventory, affects, or other status. This is a confirmed
   lost-update design, not merely a performance concern.
3. **Most durable game state is still written synchronously.** The three persistence
   queues cover event/audit inserts, not full character saves or the majority of the
   509 direct `qry()`/`db_query()` call sites. A slow main MySQL query can pause the
   whole game for the configured 10-second timeout; the primary Redis context has no
   timeout at all.
4. **Failure paths amplify load and do not consistently preserve data.** Redis failures
   can invoke a full synchronous player save from an object or XP mutation. A failed
   deferred save is left registered but is never rescheduled. The character flat-file
   fallback is not read when that character still exists in SQL, while terminal rent
   paths can destroy live items and extract the character after the SQL save failed.
5. **Epic balances, account-bank money, item ownership, and their audit records do not
   share one atomic durability boundary.** A process crash can leave the balance and
   ledger disagreeing, or duplicate/lose bank money across the DB update and later
   player save.
6. **Several player subtables retain deleted state.** Timers, undead spell slots,
   forged-item knowledge, and granted commands are `REPLACE`d when present but are not
   deleted when removed or reset, so stale rows can return at the next login.

There are good foundations to preserve: all inspected local tables use InnoDB; the core
player save is transactional; item-event dedupe keys and foreign keys exist; the event
workers use a bounded connection pool; and the locker worker already demonstrates the
right basic boundary of building an immutable snapshot on the game thread and applying
it without touching live game objects. Those pieces are not yet composed into a safe
general player-persistence system.

### Method and limits

This review used:

- static tracing of database and Redis call sites through the game loop, login,
  autosave, disconnect, combat, epic, artifact, bank, locker, and shutdown paths;
- read-only inspection of the configured **local development database**;
- read-only `EXPLAIN` plans for representative hot queries;
- the event-loop hotspot measurements from August 2026, whose durable outcome is
  recorded in [ARCHITECTURE.md](../../docs/ARCHITECTURE.md#event-wheel).

No migration, write test, production query, destructive operation, or 200-client load
test was run. The local database contains only 4 players and 108 player items, so its
row estimates and `Slow_queries=0` are not evidence of production capacity. Performance
Schema was disabled/inaccessible, so there are no production-like query latency
histograms in this report.

Severity means:

| Priority | Meaning |
|---|---|
| **P0** | Fix before claiming 200-player readiness; data-loss, duplication, or game-loop outage risk |
| **P1** | Fix as part of the scalability project; serious amplification, recovery, or growth risk |
| **P2** | Hardening, maintainability, and operational clarity |

"Confirmed" means the behavior follows directly from the current code. "Risk" means a
failure requires timing, production data volume, configuration drift, or another
condition that was not reproduced here.

### Current architecture

| Path | Execution model | What it actually persists | Main-loop exposure |
|---|---|---|---|
| Global `DB` connection | Synchronous, single connection | Login/load, full player saves, combat/epic/artifact work, accounts, auctions, reports, and most gameplay SQL | Direct; 10-second read/write timeouts, no reconnect path |
| Item/scalar/large queues | Three worker threads, pool size 4 | Primarily item ownership events, login/logout audit lines, bottle epic audit, and oversized versions of those lines | Enqueue is cheap normally; queue growth and fallback can block |
| Redis dirty-player set | Main-thread Redis commands plus a fork every 30 seconds | Coalesces selected dirty PIDs, then the child runs full SQL player saves serially | Redis calls are synchronous; fork fallback is synchronous |
| Locker async worker | Main-thread immutable SQL snapshot, one worker job at a time | Public locker inventory; private-chest work remains synchronous | Better isolation, but synchronous fallbacks remain |
| Redis caches | Synchronous cache get/set/delete | Frag list, epic-zone report, artifact reports, named report, and ship snapshots | Every cache operation can block the game thread |
| Redis world recovery | Forked child, every 10 seconds by default | NPCs, floor objects, doors, and zone timers serialized as one JSON value | Parent counts the world and manipulates floor-drop keys; child has no timeout or reliable completion check |
| Flat fallback log/pfile | Synchronous local file writes | Event SQL lines or a binary character snapshot | Event fallback calls `fsync`; character fallback is not automatically reconciled |

The description in [docs/DATABASE.md](../../docs/DATABASE.md) is materially out of date. Full
player/object/ship persistence does not all flow through the three workers, and the
large queue is not the normal `pkill_info` path. The code contains about 500
direct `qry()`/`db_query()` call sites; the largest concentrations are `sql.c`,
`sql_player.c`, `artifact.c`, `auction_houses.c`, `account_reward.c`, `boon.c`,
`epic.c`, and `outposts.c`.

### When player state is saved today

There is no single save policy. The durable timing of a mutation depends on which
helper its caller happens to use:

| Trigger | Timing | Current path and durability window |
|---|---|---|
| Per-player autosave | First scheduled 1,200 pulses (five minutes) after login, then every five minutes | Flushes at most 64 item events, schedules a coalesced character save two pulses (about 500 ms) later, then runs synchronous `do_save_silent()`/`writeCharacter()`. Each timer is relative to login, so a reconnect wave creates a later save wave. |
| Manual `save` | Two pulses after the command | Uses the same 512-slot deferred-save table as autosave. Multiple requests for a PID coalesce, but a failed callback leaves the slot stranded. |
| Level advance | Two pulses after the level change | Sets the deferred slot's `level_dirty` flag and then performs the same full save. |
| XP, carried money, inventory/container, and selected quest mutations | Nominally within the next Redis dirty flush | `mark_player_dirty()` adds the PID to Redis. The global flush first runs five seconds after event initialization and then every 30 seconds; it forks and calls `sql_save_player()` for each currently online PID. The actual window is 0-30 seconds plus serial child time, and longer whenever a previous child is still running. If Redis is disabled, the mark is discarded and only another explicit/autosave trigger protects the change. |
| Epic gain | Ledger write occurs during `gain_epic()`; spendable balance waits for another character save | Most gains synchronously insert `epic_gain`; bottle gains enqueue the insert. The in-memory balance is neither part of that transaction nor marked dirty, so its recovery point is whichever unrelated player save happens next. |
| Selected rewards and state-changing commands | Immediately in the command/callback | Tradeskills, account rewards, epic skills, auctions, artifacts, guild operations, and several admin paths call `do_save_silent()` or `writeCharacter()` directly. These full saves block the game thread and do not use one consistent error policy. |
| Camp/inn, death, link loss, idle rent, copyover, or shutdown | At the terminal transition | These paths synchronously save, often after first flushing a deferred save. Link loss and shutdown can therefore perform two consecutive full saves for the same state. Normal mortal exit is primarily camp/inn rather than `do_quit()`. |
| Redis command/reconnect failure during a dirty mark | Immediately in the mutation caller | The failure fallback invokes synchronous `sql_save_player()`, converting an optional coordination failure into game-loop database work. |

Evidence for these cadences and routes:

- [`nanny.c:2273`](../../src/nanny.c#L2273) and
  [`actoth.c:1710`](../../src/actoth.c#L1710) for the five-minute autosave;
- [`actoth.c:2086`](../../src/actoth.c#L2086) and
  [`limits.c:708`](../../src/limits.c#L708) for two-pulse manual/level checkpoints;
- [`new_events.c:1568`](../../src/new_events.c#L1568),
  [`redis.c:59`](../../src/redis.c#L59), and
  [`redis.c:921`](../../src/redis.c#L921) for dirty-save startup and cadence;
- [`comm.c:2157`](../../src/comm.c#L2157),
  [`copyover.c:472`](../../src/copyover.c#L472), and
  [`comm.c:1618`](../../src/comm.c#L1618) for terminal saves; and
- representative immediate action saves at
  [`tradeskill.c:1319`](../../src/tradeskill.c#L1319),
  [`account_reward.c:733`](../../src/account_reward.c#L733), and
  [`epic.c:2259`](../../src/epic.c#L2259).

This mixed trigger model is itself a correctness and capacity problem. Two mutations
of the same player can enter different paths, reach SQL in a different order, and save
different component sets. A central revisioned mutation/checkpoint policy is required
before any cadence can be treated as a reliable recovery-point objective.

### Epic award and combat persistence flow

All inspected epic sources converge on `gain_epic(ch, type, type_id, amount)`:

| Gameplay source | Epic types / fan-out |
|---|---|
| Epic-zone stones, nexus stones, and random-zone completion | Zone and nexus awards can fan out to every eligible group member in the room; random-zone completion has both solo and group paths. |
| PvE combat | An elite level-50+ mob calls `group_gain_epic()` for the killer's in-room group. Ordinary qualifying kills can independently award one random-mob epic to each eligible participant. |
| PvP combat | Ground PvP reaches `epic_frag()` and can add a 500-point task bonus before `gain_epic()`; ship PvP awards the captain based on frags. |
| Quests, boons, achievement, and consumable | World quests scale the award by quest level; boons carry a configured amount; the Strahd achievement awards 1,000; an epic bottle awards 75. |

Evidence: [`epic.c:335`](../../src/epic.c#L335),
[`epic.c:558`](../../src/epic.c#L558),
[`fight.c:2395`](../../src/fight.c#L2395),
[`fight.c:3013`](../../src/fight.c#L3013),
[`random.zone.c:1212`](../../src/random.zone.c#L1212),
[`world_quest.c:137`](../../src/world_quest.c#L137),
[`nexus_stones.c:633`](../../src/nexus_stones.c#L633), and
[`ships/ship_combat.c:285`](../../src/ships/ship_combat.c#L285).

For each non-bottle award, the current order is:

1. calculate task/nexus/race modifiers and call the DB-backed epic-point bonus;
2. synchronously save qualifying guild prestige;
3. increment the spendable epic balance in memory;
4. synchronously insert the `epic_gain` ledger row;
5. feed every equipped artifact, update the in-memory `TAG_EPICS_GAINED` affect, and
   possibly choose a new task or grant a level.

Bottle awards skip bonus/artifact/task processing and enqueue their ledger insert.
Neither path marks the player dirty, and none of the balance, ledger, guild, artifact,
affect, task, or level side effects share one transaction. The guild helper also saves
the new prestige before adding any threshold-earned construction points, leaving those
construction points dependent on a later guild save. Group rewards multiply this
sequence by recipient, which is why an epic stone or elite kill is both a database
burst and an ambiguous-crash consistency boundary.

PvE XP is separately applied through `gain_exp()`, which marks the player dirty; that
incidental mark may eventually checkpoint an epic balance from the same kill, but it
does not make the XP, epic ledger, balance, artifacts, and guild update atomic. PvP adds
the wider death-log, leaderboard, recent-death, artifact, and victim-save sequence
detailed in DB-012.

### 200-player demand model

Duris runs four pulses per second (`OPT_USEC=250000`, `WAIT_SEC=4`). The new-event
scheduler normally allows 25 ms of callback work per pulse. A blocking callback can
still overrun that budget because the budget is checked between callbacks. These
constants are defined at [`config.h:82`](../../src/config.h#L82),
[`config.h:105`](../../src/config.h#L105), and
[`new_events.c:41`](../../src/new_events.c#L41).

#### Scale-driving hot work

| Source | Intended frequency at 200 active players | SQL work |
|---|---:|---|
| Hit regeneration while 200 players are below maximum HP | 200 callbacks/pulse x 4 pulses/s = **800 callbacks/s** | At least one `epic_bonus` read/callback; a matching health bonus adds one rolling `SUM(epics)` |
| Movement regeneration while below max | Up to another **800 callbacks/s** | Same pattern for movement bonus |
| Damage XP | Activity-dependent; potentially many calls per combat round | Epic-bonus selection read and, for an EXP bonus, rolling `SUM(epics)` |
| Five-minute autosave | 200 / 300 s = **0.67 saves/s**, or 40/minute average | At least about 14 round trips per established crash save before current languages, intros, skills, affects, dirty items, pets, trophies, or extra descriptions |
| 30-second Redis dirty flush | Up to 200 / 30 s = **6.67 full player saves/s** in one child | At least about 11 round trips/player, serially on one child connection; row-dependent work can be far larger |

In the 200-player sustained-regeneration case, the hit path alone gives the 25 ms event
budget only **125 microseconds per player for the entire callback**, including SQL and
game logic. If all 200 select the health bonus and therefore run two queries, the
average query allowance is below 62.5 microseconds. Deferring callbacks does not remove
the work; it creates event debt and visible regeneration/combat timing lag. Full-health
players with non-negative regeneration return before this query and stop the event, so
800 reads/s is a stress-case rate rather than the idle baseline.

The autosave lower bound comes from the duplicate shapechange transaction in
`writeCharacter()`, plus the master transaction, name lookup, crash-room lookup, status
update, replacement-table deletes, and commit. Real characters add multi-row inserts,
inventory replacement, per-item affects/descriptions, pets, and one or two trophy
queries per historical zone. Average rates also hide synchronized bursts after login,
copyover, or reconnect.

#### Burst examples

- A group epic-stone touch runs `gain_epic()` for every eligible member, and each gain
  can query the bonus, insert the epic ledger row, query and update every equipped
  artifact, invalidate Redis reports, and update zone metadata.
- A PvP death queries the latest 20 victim rows and then can issue one `pkill_event`
  query for each row, inserts a full log/equipment record for killer and group members,
  updates progress and leaderboard state for participants, calculates race totals,
  awards epics/artifact time, and finally performs a full victim save.
- A login reads status plus at least seven ancillary status tables, skills, affects,
  items, item metadata, and one ownership event per item. A 200-player reconnect storm
  serializes most of this through the one main connection.

#### Background scheduled work

Player-driven rates are not the whole load. Several exact-modulus callbacks align on
the same game-loop pulses, so average query rates hide recurring spikes:

| Scheduled work | Steady cadence | Main-thread or recovery risk |
|---|---:|---|
| Redis donation polling | 1 second | Touches a separate Redis subscriber context from the event loop. |
| Redis world recovery snapshot | 10 seconds by default | Counts live world entities on the parent, then forks a child that traverses all NPCs, floor objects, doors, and zones and writes one JSON value. |
| Expired-artifact check | 12 seconds | Queries expired artifacts and can load/save or remove owners and objects. |
| Operational statistics | 75 seconds | Scans descriptors, opens/appends a local file, and synchronously inserts a SQL row. |
| Auction finalization, level-cap check, boon maintenance | 60 seconds, on the same pulse | Auction work loops over every expired auction; boon maintenance loads active IDs and then fetches/checks each boon; cap work aggregates leaderboard state. |
| Epic-zone balancing | 120 seconds | Loads all epic zones and can issue one or more updates per zone requiring adjustment. |
| Artifact wars / bind reconciliation | 30 minutes / 7 minutes after their initial early run | Load artifact ownership sets and can cascade into owner, timer, and item changes. |

At 60-second common multiples, auction, cap, boon, timer, and other activities execute
in one `activities` slice. At 120-second multiples, epic-zone work joins them. These
callbacks are not isolated by the new-event 25 ms budget and can therefore create
predictable whole-game latency spikes. Stagger them with deterministic jitter, put SQL
work behind bounded jobs, and cap each maintenance pass by rows/time with a continuation
cursor. Evidence: [`comm.c:1354`](../../src/comm.c#L1354),
[`new_events.c:1547`](../../src/new_events.c#L1547),
[`statistics.c:35`](../../src/statistics.c#L35), and
[`redis.c:1468`](../../src/redis.c#L1468).

### P0 findings

#### DB-001: Database reads in per-pulse regeneration

- **Status:** Confirmed
- **Impact:** Event backlog and game-wide lag during sustained regeneration at scale

`event_hit_regen()` calls `hit_regen()` and, while regeneration remains active,
reschedules itself every pulse. `hit_regen()` returns before the database lookup when a
player is already at maximum HP with non-negative regeneration. Otherwise it calls
`get_epic_bonus()`. That function first selects `epic_bonus`; if the selected type
matches, it runs a rolling aggregate over `epic_gain`.

Evidence:

- [`events.c:441`](../../src/events.c#L441)
- [`limits.c:269`](../../src/limits.c#L269)
- [`epic_bonus.c:132`](../../src/epic_bonus.c#L132)
- [`epic_bonus.c:167`](../../src/epic_bonus.c#L167)

Movement regeneration and XP calculation repeat the same DB-backed lookup at
[`limits.c:443`](../../src/limits.c#L443) and
[`limits.c:1134`](../../src/limits.c#L1134).

**Recommendation:** Hydrate an `EpicBonusState` into `pc_only_data` during login:
selected type, selection timestamp, qualifying rolling gain, computed modifier, and
next expiry boundary. Increment it in memory when `gain_epic()` succeeds, reset it when
`epic_bonus_set()` succeeds, and expire old contributions using a small per-player
timestamp bucket/deque or a scheduled refresh. Regeneration and XP must be pure
in-memory reads. Redis is not appropriate for this hot path because it is another
network round trip.

#### DB-002: Forked player saves can overwrite newer state

- **Status:** Confirmed design race
- **Impact:** Lost currency, XP, epic points, items, flags, affects, skills, pets, or
  location; long DB stalls can make the stale window large

Every 30 seconds, `flush_dirty_players()` renames a Redis set, obtains online PIDs, and
forks. The child walks its copied `P_char` graph and writes full player snapshots. The
parent continues accepting commands. There is no `save_revision`, compare-and-swap,
per-player lock shared with the child, or other ordering check in SQL. Therefore:

1. child snapshots revision A at `fork()`;
2. the live player reaches revision B and a main-thread save commits B;
3. the child reaches that PID later and commits A;
4. A becomes durable even though it is older.

Evidence: [`redis.c:921`](../../src/redis.c#L921),
[`redis.c:1020`](../../src/redis.c#L1020), and
[`sql_player.c:951`](../../src/sql_player.c#L951).

Additional problems in this path:

- the server already has persistence and locker threads, so the child calls allocation,
  logging, MySQL, and game code after `fork()` in a multithreaded process; inherited
  library or allocator locks can deadlock;
- the child connection has no connect/read/write timeout
  ([`sql_player.c:798`](../../src/sql_player.c#L798));
- while a child is hung, every later flush is skipped and there is no child deadline or
  kill/recovery policy;
- the parent clears container dirty flags immediately, before child success; if the
  child fails, restoring the PID does not restore those flags, so a retry can omit the
  unsaved item changes;
- if `fork()` fails, the main thread synchronously saves every PID and then deletes the
  inflight Redis set regardless of individual save results;
- an inflight `mud:dirty_players:flushing` set left by a process crash is not recovered
  at boot, and a later `RENAME` can overwrite it.

The world-recovery system repeats the fork pattern every 10 seconds by default. The
parent first counts the live world, then the child allocates a cJSON tree while walking
all NPCs, floor objects, doors, and zones and uses a fresh hiredis context with no
timeout. This has the same post-fork allocator/library-lock hazard and can create
copy-on-write memory pressure. The parent has no child deadline and ignores a completed
child's exit status. The serializer also ignores errors when writing the timestamp and
`valid=1` marker and treats non-null Redis error replies for the main snapshot as
success, while the parent clears the floor-drop set immediately after forking regardless
of child success. A failed snapshot can therefore silently degrade the next crash
recovery. Evidence: [`redis.c:1468`](../../src/redis.c#L1468) and
[`redis.c:2150`](../../src/redis.c#L2150).

**Recommendation:** Remove both fork paths. Use a long-lived worker (or sidecar process
started independently) that accepts immutable snapshots. Assign every player mutation a
monotonic revision. A worker transaction may apply revision N only if the stored
revision is older; the game thread clears dirty state only after an ACK for the exact
revision, and only if no newer revision exists. Never let a worker traverse `P_char` or
`P_obj`. Give world recovery its own immutable, sequence-numbered snapshot job and keep
floor-delta records until that exact snapshot is acknowledged.

#### DB-003: Blocking MySQL and Redis are on the game thread

- **Status:** Confirmed
- **Impact:** Whole-game pauses, command lag, missed event budgets, and failure cascades

The global MySQL connection is used synchronously by most gameplay code. It has
10-second read and write timeouts but no connect timeout, ping/reconnect loop, or circuit
breaker. One query can therefore consume 40 game pulses, and a transient connection
loss can leave subsequent calls failing.

The primary hiredis connection is also shared by main-thread cache, online-player,
dirty-save, floor-item, and world-state code. It is created with `redisConnect()` and no
command timeout. A stalled Redis server or network can hang the game indefinitely.
`mark_player_dirty()` can call `SISMEMBER` before checking that `redis_ctx` is non-null,
which is also a potential null-context crash.

Evidence:

- [`sql.c:470`](../../src/sql.c#L470)
- [`sql.c:1819`](../../src/sql.c#L1819)
- [`redis.c:133`](../../src/redis.c#L133)
- [`redis.c:174`](../../src/redis.c#L174)
- [`redis.c:854`](../../src/redis.c#L854)

On Redis reconnect or `SADD` failure, `mark_player_dirty()` performs a full SQL player
save synchronously in the caller. At scale, a cache/coordinator outage therefore turns
loot, money, XP, quest, and inventory mutations into the most expensive possible main-
thread operation.

**Recommendation:** No steady-state database or Redis command should execute in the
simulation thread. Route DB work through completion-based jobs. Give every external
operation a short connect and operation deadline, classify retryable errors, and use a
circuit breaker. When Redis is unavailable, retain in-memory dirty state and a durable
local journal; do not synchronously full-save from the mutation call.

#### DB-004: Deferred save failure strands the player

- **Status:** Confirmed correctness bug
- **Impact:** A failed autosave/checkpoint can suppress every later scheduled save for
  that player until a disconnect or global flush happens

`event_deferred_character_save()` clears a slot only on success. On failure it neither
clears nor reschedules the event. Later calls find the existing slot, update its flags,
and return without adding a new event. The slot is permanently pending but has no
callback.

`persistence_flush_character_saves()` also emits `deferred_save_flushed` even when the
save failed. Disconnect and shutdown paths often flush a pending save and then perform
another full save, creating duplicate work in the successful case.

Evidence: [`actoth.c:1766`](../../src/actoth.c#L1766),
[`actoth.c:1814`](../../src/actoth.c#L1814), and
[`actoth.c:1877`](../../src/actoth.c#L1877).

**Recommendation:** Implement an explicit state machine with `dirty_revision`,
`queued_revision`, `inflight_revision`, retry count, next retry time, and last error.
Failure must retain dirty state and enqueue bounded exponential backoff. Success may
clear only the acknowledged revision. Disconnect should promote/wait for the one
existing job rather than issue a second snapshot.

#### DB-005: Terminal save failure can discard live state, and fallback is not recovered

- **Status:** Confirmed
- **Impact:** Item/player-state loss on terminal save failure; normal login can ignore
  the newer fallback that operators may expect to recover

When SQL player save fails, `writeCharacter()` writes a legacy binary pfile. But
`restoreCharOnly()` checks SQL first: if the PID exists and SQL component loading fails,
it returns `-2`; it does not compare or load the pfile. If SQL succeeds, it always wins
even when the fallback is newer. There is no import/reconciliation queue. The pfile
write also closes without an explicit file or parent-directory `fsync`.

For `RENT_INN`, `RENT_LINKDEAD`, `RENT_CAMPED`, `RENT_DEATH`, and artifact terminal
types, `writeCharacter()` extracts every saved equipped/carried object after the save
attempt based only on the rent type-not on `result`. Camp and afterlife/death paths then
ignore the false return and extract the character. Thus a transient SQL failure can
leave the only new snapshot in the non-reconciled pfile while destroying the in-process
objects that could otherwise have been retried.

Evidence: [`files.c:1315`](../../src/files.c#L1315),
[`files.c:1667`](../../src/files.c#L1667),
[`files.c:1684`](../../src/files.c#L1684),
[`files.c:2691`](../../src/files.c#L2691), and
[`affects.c:3571`](../../src/affects.c#L3571).

**Recommendation:** Choose one honest policy:

- preferred: use a durable, versioned write-ahead journal that the new persistence
  worker replays idempotently; or
- make pfiles first-class recovery records with revision/timestamp, checksum, atomic
  write+rename+directory sync, boot reconciliation, and an explicit operator report.

Until then, a failed terminal save must abort the terminal transition and retain the
live character/items for retry; callers must check the result. Alerts should say
"fallback written but not automatically recoverable."

#### DB-006: Shared bank money and player money are not atomic

- **Status:** Confirmed
- **Impact:** Currency duplication/loss on crash and stale overwrite across multiple
  characters on one account

A deposit mutates carried money in memory and separately increments `account_banks`.
The caller ignores DB failure. The lower carried-money value is persisted later in
`player_data`. A crash between the two durability points can restore the old carried
money while retaining the bank deposit, duplicating money. Withdrawal has the inverse
loss window.

The atomic SQL delta helpers are undermined by `sql_save_account_bank()`, which writes
an absolute balance from one character's cached copy. Another online character on the
same account can update the bank, after which a stale absolute save overwrites it. The
withdraw helper also returns `current - amount` from a pre-update `SELECT`, so concurrent
updates can make the returned in-memory balance stale even when the guarded decrement
succeeds.

Evidence:

- [`actoth.c:2148`](../../src/actoth.c#L2148)
- [`sql_player.c:10698`](../../src/sql_player.c#L10698)
- [`sql_player.c:10722`](../../src/sql_player.c#L10722)
- [`sql_player.c:10780`](../../src/sql_player.c#L10780)
- [`utility.c:2959`](../../src/utility.c#L2959)

**Recommendation:** Treat account bank as a DB-authoritative ledger, never as an
absolute cached field saved from a character. Use one idempotent transaction to apply
the bank delta and the corresponding player-wallet delta, keyed by a unique command
ID. Return the committed balance (`UPDATE ... RETURNING` where supported, or a locked
transaction), then publish it to every online character on that account. Commands can
wait asynchronously for completion while that player's economy actions are gated.

#### DB-007: Epic balance and epic ledger can disagree

- **Status:** Confirmed
- **Impact:** Lost balances, duplicate audit entries, incorrect bonus totals

`gain_epic()` updates `ch->only.pc->epics` in memory, then writes `epic_gain`. Most epic
types use a direct synchronous insert; bottle gains use the scalar queue. Neither path
atomically updates `player_data.epics`, and `gain_epic()` does not mark the player dirty.
The ledger can therefore contain an award that a crash removes from the spendable
balance. The reverse is also possible on insert failure.

The queued bottle helper accepts an `event_key` but ignores it and inserts a raw row
without a unique idempotency key. Fallback replay or an ambiguous connection failure
can duplicate that award in the ledger and in the rolling bonus calculation.

Evidence: [`epic.c:353`](../../src/epic.c#L353),
[`epic.c:414`](../../src/epic.c#L414), and
[`sql.c:2089`](../../src/sql.c#L2089).

**Recommendation:** Give every award/spend a unique event ID. In one transaction,
insert the immutable epic ledger event with `INSERT ... ON DUPLICATE KEY`, update the
balance, and write an outbox row if other systems need notification. Make either the
ledger plus snapshots or the balance authoritative, and provide a reconciliation query
that proves they agree. Update the in-memory epic-bonus accumulator only after the
durable command ACK.

#### DB-008: Removed player subtable values reappear

- **Status:** Confirmed correctness bug
- **Impact:** Expired/reset timers, undead slots, forged knowledge, or revoked commands
  can return after relog

Languages and introductions are deleted before being reinserted. Timers, undead spell
slots, forged items, and granted commands are only `REPLACE`d for current non-zero
entries; their "batch delete then insert" comments are not implemented. A value changed
to zero or a command removed from the array leaves the old DB row behind, and the login
loader restores it.

Evidence: [`sql_player.c:1420`](../../src/sql_player.c#L1420) through
[`sql_player.c:1523`](../../src/sql_player.c#L1523), and the corresponding loaders at
[`sql_player.c:4070`](../../src/sql_player.c#L4070) through
[`sql_player.c:4116`](../../src/sql_player.c#L4116).

**Recommendation:** Delete all replacement-style subtables in the same player-save
transaction before inserting current rows, or issue precise deletes for cleared dirty
entries. Add regressions that save a non-zero value, clear/revoke it, save again, and
reload.

### P1 findings

#### DB-009: A "full character save" is large, redundant, and not fully atomic

`sql_save_player()` transactionally saves status, skills, affects, items, pets, and
shapechanges, which is a good core. `writeCharacter()` nevertheless:

- saves shapechanges once before the master save and again inside it;
- scans every historical zone-trophy entry and performs a `SELECT` plus optional
  `UPDATE`/`INSERT` per entry, without dirty tracking;
- unequips every item, removes affects, saves, then reapplies them;
- rewrites all skills and affects with delete+insert even when unchanged;
- deletes and recreates every crash-saved pet and recursively writes pet items;
- writes each item affect and extra description separately after batching item rows;
- can perform the trophy/first-shape save outside the master transaction, so the
  operation called "save character" is not one durability unit.

Inventory dirty flags help only some cases. Ordinary transfers frequently mark whole
inventory/equipment sets dirty. The clean-inventory path still saves every other player
component.

Evidence: [`files.c:1560`](../../src/files.c#L1560),
[`sql_player.c:951`](../../src/sql_player.c#L951),
[`sql_player.c:2798`](../../src/sql_player.c#L2798), and
[`trophy.c:377`](../../src/trophy.c#L377).

**Recommendation:** Track dirty component groups and snapshot only changed groups.
Use set-based multi-row upserts/deletes, and make trophy changes write-through or dirty-
set based. Snapshot equipment without mutating live wear/affect state. Preserve one
transactional revision across every component included in a player checkpoint.

#### DB-010: Login has N+1 queries, partial-success semantics, and no consistent snapshot

Status load is one wide player query followed by separate language, intro, timer,
undead-slot, forged-item, and two granted-command queries. Skills and affects add two
more. Item loading performs one item query, batched affect and description queries, but
also queries `persistence_item_events` once per item to validate its latest owner.
Container and metadata matching use repeated linear scans, producing O(N^2) CPU work for
large inventories.

Pet loading performs an item query per pet and two extra queries per pet item. It stores
only 256 pet items in a fixed local array and silently stops reading beyond that limit.

Component failures after status are often logged and treated as empty/continue. The
queries are not in a consistent read transaction, so a login can combine components
from different save revisions or enter with missing skills, affects, or items during a
transient DB failure.

Evidence:

- [`sql_player.c:3813`](../../src/sql_player.c#L3813)
- [`sql_player.c:4241`](../../src/sql_player.c#L4241)
- [`sql.c:3891`](../../src/sql.c#L3891)
- [`sql_player.c:3257`](../../src/sql_player.c#L3257)
- [`sql_player.c:4649`](../../src/sql_player.c#L4649)

**Recommendation:** Load a player in one consistent read transaction and fail the login
closed if required components fail. Batch current ownership for all item UIDs in one
query, or maintain a unique `current_item_owner` table updated in the ownership
transaction. Use hash maps from item ID to object for O(N) assembly. Batch pet affects
and descriptions exactly as player items are batched, and replace fixed truncation with
an explicit validated limit/error.

#### DB-011: Item audit durability is not ordered with character snapshots

The ownership event queue is asynchronous. Autosave calls
`persistence_flush_item_events(64)`, but that function immediately returns while the
normal worker is running; the save is merely scheduled two pulses later. There is no
barrier proving all ownership events up to the snapshot revision reached SQL first.

The owner validator also deliberately keeps an item when its query fails or no event is
found. That favors availability, but during an outage or queue lag it cannot prevent a
duplicate/stale snapshot from loading.

Evidence: [`actoth.c:1710`](../../src/actoth.c#L1710),
[`utility.c:1009`](../../src/utility.c#L1009), and
[`sql.c:3902`](../../src/sql.c#L3902).

**Recommendation:** Put ownership transfer and both owners' inventory revisions in one
idempotent transaction. If audit is an outbox side effect, commit it in that transaction
and deliver it later. Do not depend on relative timing between independent queues.

#### DB-012: Combat and epic actions create multi-system query fan-out

Confirmed examples include:

- `setHeavenTime()` selects the last 20 death event IDs, then executes up to 20 more
  queries to determine which were within an hour;
- `sql_save_pkill()` directly inserts a potentially large event and one large
  `pkill_info` row per killer/victim group member; it does not normally use the
  large-payload worker described in the docs;
- `AddFrags()` writes progress, updates the leaderboard, sums a race leaderboard,
  potentially updates the level cap, invalidates Redis, and awards epics/artifact time
  for each eligible participant;
- the victim leaderboard is updated before the in-memory frag loss is applied, so the
  stored `total_frags` is the old pre-loss value;
- each epic award can run approximately four DB operations per equipped artifact:
  artifact-bind read, artifact read, existence/read inside update, and update/insert;
- a qualifying guild award calls `Guild::save()` synchronously for each recipient, and
  threshold-earned construction points are added only after that save.

Evidence: [`fight.c:769`](../../src/fight.c#L769),
[`sql.c:1258`](../../src/sql.c#L1258),
[`fight.c:839`](../../src/fight.c#L839), and
[`artifact.c:1432`](../../src/artifact.c#L1432), plus the guild sequence at
[`assocs.c:121`](../../src/assocs.c#L121).

`sql_get_bind_data()` also returns on query failure without initializing its output
arguments, so artifact decisions can use indeterminate values during an outage.

**Recommendation:** Convert one gameplay outcome into one typed persistence command.
For PvP, batch group rows and calculate recent-death count with one join/aggregate.
Update frag state after applying the in-memory delta, or preferably make the transaction
return the authoritative value. Keep active artifact state in memory and persist one
set-based delta per award. Apply guild prestige/construction as a single ordered delta
rather than saving between the two mutations.

#### DB-013: Queue capacity masks outages and can consume about 512 MiB

Each small queue can grow from 4,096 to 131,072 fixed 1 KiB slots; the large queue can
grow from 64 to 2,048 fixed 128 KiB slots. At maximum, the three payload arrays alone
are roughly **512 MiB**. Growth allocates every slot separately, copies queued payloads,
and frees the old array while holding the queue mutex in the producer call. The game
thread can therefore experience allocator pauses and transient memory substantially
above the steady-state size. Scalar growth failure increments the dropped counter twice.

Workers execute one SQL string at a time. There is no batching, rate adaptation,
oldest-event-age SLO, retry classification, or circuit breaker. A large capacity makes
an outage less visible while recovery time and memory grow.

Evidence: [`persistence_queue.h:4`](../../src/persistence_queue.h#L4) and
[`persistence_queue.c:133`](../../src/persistence_queue.c#L133).

**Recommendation:** Use bounded typed queues with byte limits, high/low watermarks, age
metrics, and explicit overload policy. Preallocate compact ring storage outside the hot
producer path. Batch compatible inserts. Spill durably before memory limits, with
idempotent record IDs, rather than growing toward hundreds of MiB.

#### DB-014: Worker health checks do not detect a blocked DB write

The workers set `in_write=1` around the writer. Both `worker_running()` and
`worker_stuck()` explicitly treat that state as healthy/non-stuck, regardless of
heartbeat age. The main heartbeat checker only attempts restart when
`worker_running()` is false. A DB call blocked in the client library is therefore not
detected by the watchdog.

The pool limits read/write calls to 10 seconds and acquisition to 2 seconds, but a
repeatedly failing connection can still cause long queue age. If connection replacement
fails, that pool slot is set to null and never healed, permanently shrinking the pool.
`sql_pool_shutdown()` waits indefinitely for borrowers and currently has no call site.

Evidence: [`persistence_queue.c:604`](../../src/persistence_queue.c#L604),
[`persistence_queue.c:1497`](../../src/persistence_queue.c#L1497),
[`utility.c:1294`](../../src/utility.c#L1294),
[`sql_pool.c:292`](../../src/sql_pool.c#L292), and
[`sql_pool.c:130`](../../src/sql_pool.c#L130).

**Recommendation:** Enforce deadlines at the operation layer, expose worker state as
`idle/acquiring/executing/retrying`, and alert on both execution age and oldest queue
age. Heal pool slots in a background reconnect loop. Shutdown needs a bounded drain,
durable spill of remaining jobs, and then bounded worker termination.

#### DB-015: Event fallback replay is not uniformly idempotent

On queue failure or DB failure, event persistence appends a line, calls `fflush` and
`fsync`, and closes the file. This is useful durability, but if used from a producer
fallback it puts disk latency directly in the game thread. Boot replay executes records
one at a time synchronously before workers start.

Replay writes SQL first and rewrites/replaces the log afterward. If the rewrite, hard
link, or rename fails after SQL succeeds, the original remains and is replayed at the
next boot. Item events use a dedupe key, but arbitrary scalar/large raw SQL-such as
bottle epic inserts and login log rows-does not consistently have one.

Evidence: [`utility.c:888`](../../src/utility.c#L888) and
[`utility.c:1416`](../../src/utility.c#L1416).

**Recommendation:** Persist typed journal records with a mandatory unique event ID and
schema version. The destination table or an inbox table must atomically record that ID.
Replay in batches and checkpoint only after destination commit. Never use unrestricted
raw SQL as the durable message format.

#### DB-016: Redis is mixing cache, dirty-state coordination, and recovery authority

Redis currently serves three different reliability roles:

1. optional derived report cache;
2. required dirty-PID coordination for SQL player saves;
3. crash-recovery world/floor snapshots.

Those roles need different failure policies. Cache loss should only reduce performance;
dirty-state loss must not lose durable player changes; recovery snapshot loss needs an
explicit recovery downgrade. Today, Redis failure can either disable saves, force a
synchronous full save, or make recovery data unavailable depending on the call site.

The 64-entry dirty debounce also scales poorly: the first 64 PIDs still issue
`SISMEMBER` during the one-second debounce to cover a rename race, while players beyond
64 are never retained in the debounce table and issue `SADD` for every mutation.

Report caches are inconsistent: epic zones have a 15-minute TTL, while frag, artifact,
and named-report keys generally rely on perfect invalidation and can remain stale
indefinitely. A miss rebuilds synchronously on the main SQL connection. If Redis remains
unavailable or the cache `SET` fails, each later caller repeats that work; multiple game
processes would also have no single-flight protection.

**Recommendation:** Keep player dirty state in the game process and its durable journal.
Use Redis only for reconstructible cross-process/cache data. Add bounded timeouts,
namespaced/versioned keys, TTL jitter, and single-flight rebuilds. Track each recovery
domain independently in health output.

#### DB-017: Schema indexes do not match several hot access patterns

Read-only local schema inspection found all 124 tables on InnoDB, which is positive.
Representative gaps are:

| Query/path | Current relevant index | Local plan / risk | Candidate change to validate on a production clone |
|---|---|---|---|
| Player lookup `LOWER(name)=LOWER(?)` | unique `name`, plus redundant non-unique `name` | Function causes a full index scan; direct equality is a constant/index lookup under the current case-insensitive collation | Use `name=?`; remove redundant index after checking dependencies |
| Rolling epic bonus | `epic_gain(pid)` | Reads every historical row for the PID, then filters time/type/positive amount | `(pid,time)` or a covering variant; maintain rolling state so this is not a hot query |
| Random epic task | `epic_gain(pid)`; no useful `zones.task_zone` path | Scans all 351 local zones and uses temporary/filesort for `ORDER BY RAND()` | In-memory eligible list/reservoir; `(pid,type,type_id)` for completion membership |
| Recent PvP deaths | `pkill_info(pid)`; `pk_type` is `MEDIUMTEXT` | Filters type and orders after PID lookup; then N+1 event queries | Change type to bounded enum/varchar; `(pid,pk_type,id)` and one join |
| Latest zone touch | `zone_touches(zone_number)` | Local plan reports `Using filesort` | `(zone_number,touched_at)` |
| Race frag sum | index begins `(deleted_at,racewar,...)` | Query omits `deleted_at`, producing an index scan | Add intended active/deleted predicate or an index matching actual semantics |
| Quest/shop time windows | entity-only indexes | `TO_DAYS(NOW())-TO_DAYS(timestamp)` prevents a timestamp range lookup | Sargable `timestamp >= NOW()-INTERVAL ...`; composite entity/timestamp indexes |

These are candidates, not migrations to apply blindly. Capture production-clone row
counts and `EXPLAIN ANALYZE`, then measure write amplification before adding them.

#### DB-018: Append-only tables have no visible retention strategy

`epic_gain`, `progress`, `log_entries`, `statistics`, `pkill_event`, `pkill_info`,
`zone_touches`, trophy history, and persistence events grow for the life of the game or
season. Several lack a time index suitable for retention/export. This increases working
sets, backup/restore time, migration time, and every poorly indexed history query.

`statistics` also inserts from the main event callback every `PULSES_IN_TICK` (75 real
seconds), even though it is operational analytics rather than gameplay state.

**Recommendation:** Define per-table retention and season ownership. Archive/partition
large ledgers by time or season only after testing on a clone. Move operational metrics
to the metrics pipeline or an async batch. Never delete financial/ownership history
without a documented reconciliation and audit requirement.

#### DB-019: Query construction, escaping, and error logs expose a broad risk surface

The codebase formats SQL strings manually across hundreds of call sites. Many strings
are escaped correctly, but guarantees vary between `mysql_real_escape_string`, helper
buffers, custom queue escaping, and internal assumptions. The event queue's custom
quote/backslash escaping depends on SQL mode not containing `NO_BACKSLASH_ESCAPES`; the
local mode is compatible, but boot does not enforce this invariant.

On main-query failure, `sql_trace_exec()` logs the complete SQL statement. Player
descriptions, IPs, account password hashes, confirmation values, logs, and other private
content can therefore reach application logs. Pool workers log the first 200 characters
of failed raw SQL. The main connection does not explicitly select `utf8mb4`, while pool
and child connections do.

There are also ungated development traces in the normal persistence path. Both
`sql_player.c` and `account.c` unconditionally open, append to, and close
`/tmp/garp-item-trace.log`; player save/load invokes this several times. Every
`do_save_silent()` emits begin/result records, and every item save emits a
`[real-persistence-test]` debug record containing PIDs and pointer values. At 200-player
cadence this adds synchronous filesystem work, unbounded log growth, and unnecessary
identity/address disclosure. Evidence: [`sql_player.c:45`](../../src/sql_player.c#L45),
[`account.c:41`](../../src/account.c#L41),
[`actoth.c:1959`](../../src/actoth.c#L1959), and
[`sql_player.c:2828`](../../src/sql_player.c#L2828).

**Recommendation:** Introduce prepared statements/typed repositories for hot and
sensitive domains. Log query site, error code, duration, and a generated operation ID,
not bound values or raw SQL. Set and verify charset, time zone, isolation level, and SQL
mode on every connection. Remove leftover trace files; make any diagnostic tracing
explicitly enabled, sampled, redacted, non-blocking, size-bounded, and rotated. Add tests
with quotes, backslashes, Unicode, maximum lengths, and `NO_BACKSLASH_ESCAPES` either
prohibited or supported deliberately.

### P2 findings

#### DB-020: Configuration can silently select unsafe defaults

If `.env` is missing, non-test builds silently default to database `duris` with username
and password `duris`. `ENVIRONMENT=local` is not consulted by the code; the actual
selection comes from `DB_NAME`, with a port guard only for the exact names `duris` and
`duris_prod`. The `.env` parser does not trim whitespace or implement shell quoting.
There is no MySQL TLS configuration, so a remote DB connection relies on external
transport security.

The local `.env` is ignored by Git, but a Phase 00 read-only check found mode 0644.
Restrict it to 0600 without exposing or committing its contents, and enforce that
expectation in the runtime configuration preflight.

Evidence: [`sql.h:7`](../../src/sql.h#L7) and [`sql.c:410`](../../src/sql.c#L410).

**Recommendation:** Fail closed when required DB settings are absent, require an
explicit environment role and allow-listed database name, and refuse production-like
targets in test mode. Load secrets from process environment/secret storage rather than
weak compiled defaults. Require TLS or a local protected socket/tunnel for non-local DB
hosts. Update the README and database guide to the actual selection rules.

#### DB-021: Boot writes lookup tables before validating schema

Boot deletes and individually reinserts every race and class before the general schema
probe. The replacement is not one transaction, errors are ignored, and readers can see
empty/partial lookup tables if another service uses the DB during boot.

Evidence: [`sql.c:323`](../../src/sql.c#L323) and [`sql.c:570`](../../src/sql.c#L570).

**Recommendation:** Validate first, then upsert changed rows in one transaction. Better,
version the static lookup dataset and update only when the compiled version changes.

#### DB-022: Migration state and boot validation do not prove schema compatibility

The migration runner contains 111 ordered operations but `mud_schema_migrations` is
used only for selected one-time data-copy markers, not as a complete immutable migration
ledger. The local table has zero marker rows despite a populated modern schema. Boot
validates accounts readability, two event-table contracts, and auction engines, but not
the player/epic/frag/locker schema, collation, SQL mode, or migration version.

The runner is fail-closed, which is good, but its many DDL/data operations cannot be one
transaction and a failure can leave a partially migrated schema.

Evidence: [`migrations/run_migration.sh`](../../migrations/run_migration.sh) and
[`sql.c:570`](../../src/sql.c#L570).

**Recommendation:** Give every migration a unique immutable ID and checksum, record it
only after success, and expose expected/current schema versions. Run a read-only
preflight at boot that covers all required columns/indexes/configuration. Continue to
apply migrations only to backed-up clones, then promote the tested schema through the
deployment process.

#### DB-023: Current observability cannot establish a capacity limit

Available counters report queue pending/dropped/written/failures and pool use, but do
not report:

- query count and latency by call site;
- main-thread DB/Redis time per pulse;
- oldest queue/dirty-player age and high-water mark;
- snapshot build, DB apply, ACK, retry, and end-to-end save latency;
- player revision gap or last durable revision;
- fallback journal bytes/age and replay duplicates;
- connection timeout/reconnect/circuit state;
- transaction deadlocks and lock-wait time.

The latency trace writer uses a hard-coded `/durismud/logs/latency_trace.log` and
silently returns if it cannot open it, which makes local/test telemetry easy to miss.

**Recommendation:** Add stable site IDs around every DB/Redis call and export counts,
latency histograms, and errors without values. Correlate these with pulse/event metrics.
Create an operator command/dashboard that distinguishes cache health, queue health,
journal durability, and latest player revision.

### Recommended target design

#### Ownership and flow

The game thread should remain the sole owner of mutable `P_char`/`P_obj` state. Database
workers should receive only immutable typed values:

```text
game mutation
    -> increment player/account/entity revision
    -> update in-memory state and dirty component mask
    -> append critical idempotent command or create coalesced snapshot
    -> worker transaction applies only a newer revision
    -> completion arrives on game thread
    -> clear acknowledged components only when current revision == ACK revision
```

Required properties:

- one ordered queue per entity key, with parallelism across different players;
- monotonic revisions persisted in the parent row;
- no traversal of live game pointers by workers;
- no `fork()` from the running multithreaded server;
- mandatory operation IDs for currency, epic, item-transfer, auction, reward, and other
  non-idempotent events;
- transactional outbox/inbox for audit and cross-system notifications;
- bounded queue bytes and age with durable spill/replay;
- explicit completion and retry states rather than synchronous fallback.

The current locker worker is a useful partial model because it snapshots on the main
thread, gives the worker sealed data, coalesces generations, and handles completion on a
later pulse ([`locker_async.c:1`](../../src/locker_async.c#L1)). General player
persistence still needs durable queuing, DB-side revision guards, per-component dirty
masks, and no synchronous fallback.

#### What should be cached

| Data | Recommended location | Invalidation/durability rule |
|---|---|---|
| Active player's epic bonus | In process on `pc_only_data` | Update after durable epic award/bonus selection; expire rolling contributions by timestamp |
| Active player status, skills, affects, inventory | Authoritative in game memory; revisioned SQL checkpoints | Never use Redis dirty membership as the only save trigger |
| Account bank | SQL ledger/current-balance row | Atomic deltas with command IDs; publish committed balance to online alts; do not absolute-save cached copies |
| Item current owner | SQL current-owner row plus immutable ownership ledger | Update both atomically; batch-prefetch at login |
| Static races/classes/zones/properties | In-process boot snapshot | Versioned reload; no per-command SQL |
| Frag/artifact/epic-zone/named reports | Redis | Versioned keys, finite TTL+jitter, explicit invalidation, single-flight rebuild |
| World recovery snapshot | Dedicated recovery domain | Sequence/checksum, completion marker, age SLO; independent from optional cache health |

#### Save policy

- **Critical economy/ownership action:** durable idempotent transaction before the game
  reports final success. The command can be asynchronous while relevant player actions
  are briefly gated.
- **Ordinary scalar mutation:** coalesce a revisioned status snapshot; no full inventory
  rewrite.
- **Inventory-local mutation:** mark only affected inventory/containers; cross-owner
  movement is a critical ownership transaction.
- **Autosave:** checkpoint only dirty component groups. Five minutes can remain a safety
  sweep, but unchanged players should generate no SQL.
- **Disconnect:** promote and wait for the existing latest revision with a bounded
  deadline; do not flush and then save a duplicate snapshot.
- **Shutdown/copyover:** stop accepting state-changing commands, drain to a documented
  deadline, persist remaining typed jobs to the journal, then exit. Recovery proves the
  journal and DB revisions converge.

### Phased remediation plan

#### Phase 0 - correctness and immediate lag removal

1. Cache epic-bonus state in memory and remove SQL from hit/move regeneration and XP.
2. Fix deferred-save retry/rescheduling and truthful alerts.
3. Make every terminal caller check durability; never destroy live inventory or extract
   a character after a failed save.
4. Delete stale replacement-subtable rows before inserting current values.
5. Fix victim frag update ordering and initialize artifact bind outputs on every path.
6. Add Redis connect/command timeouts and guard context before every command; remove
   synchronous full-save failure fallback.
7. Recover/merge any existing dirty inflight set at boot and add a child deadline as a
   temporary guard for both fork paths; retain floor deltas until world-snapshot success.
8. Make account bank operations delta-only and check every result; stop absolute bank
   overwrites while the transactional ledger is built.
9. Add redacted query timing by call site and dirty/save age metrics.

#### Phase 1 - replace forked full saves

1. Add player save revision and component dirty mask.
2. Build immutable player snapshot DTOs on the main thread without unequipping.
3. Add keyed/coalescing worker jobs with revision-guarded transactions and main-thread
   ACK application.
4. Add a typed, checksummed, idempotent local journal for unacknowledged work.
5. Move all autosave/disconnect/Redis-dirty behavior to this pipeline, then delete the
   fork path.
6. Move world-recovery serialization to a separately started worker/sidecar with
   immutable snapshots, sequence checks, completion ACKs, and bounded runtime.

#### Phase 2 - transactional gameplay domains

1. Implement epic award/spend ledger plus balance transaction.
2. Implement account-bank/wallet transaction and online-alt synchronization.
3. Implement atomic current item ownership plus audit outbox for trades, lockers,
   corpses, auction settlement, and pickups.
4. Batch PvP, artifact, boon, reward, and zone-touch commands.
5. Remove raw non-idempotent SQL messages from event queues.

#### Phase 3 - load path, schema, and retention

1. Batch login ownership and pet metadata; use O(N) ID maps and a consistent snapshot.
2. Rewrite the N+1 PvP and random-zone queries.
3. Measure production-clone plans, then add the validated composite indexes.
4. Define season/time retention and archival per append-only table.
5. Establish complete migration ledger/checksum and boot compatibility contract.
6. Correct `docs/DATABASE.md`, `docs/ARCHITECTURE.md`, and README configuration guidance.

### 200-player validation plan

Run only against a backed-up development clone and non-production ports.

#### Workload profiles

Ramp 25 -> 50 -> 100 -> 200 clients, then hold 200 for at least 30 minutes under each mix:

1. idle/full-health players with all scheduled events;
2. movement and regeneration;
3. group PvE with damage/healing XP, loot, containers, and periodic deaths;
4. epic-zone and elite-mob rewards with equipped artifacts;
5. PvP group kills;
6. banking from two characters on the same account;
7. trade/locker/auction item movement;
8. reconnect/copyover login storm with large inventories and pets.

Use representative cloned history sizes for `epic_gain`, progress, PvP, item events,
statistics, and trophies. Tiny synthetic tables will hide the main problems.

#### Fault injections

- DB latency at 50/200/1,000 ms, connection reset, 30-second outage, deadlock, and
  ambiguous commit response;
- Redis latency, disconnect, restart, and lost volatile keys;
- worker crash while queued and while committing;
- game process kill before enqueue, after journal append, during DB commit, and before
  ACK application;
- disk full/read-only fallback path;
- camp, inn, death, and idle-rent transitions while the DB/fallback is failing;
- simultaneous newer manual save and older queued snapshot;
- duplicate replay of every critical event ID.

#### Acceptance criteria

Targets should be finalized with gameplay owners, but a reasonable initial gate is:

- zero steady-state DB or Redis calls from per-pulse player callbacks;
- no main-thread external I/O in normal mutation paths;
- p99 game pulse below 250 ms and no sustained new-event debt;
- p99 new-event processing within its 25 ms budget under the 200-player mix;
- normal oldest critical-command age below 1 second and checkpoint age below the agreed
  recovery-point objective;
- bounded queue memory under outage, with explicit backpressure and no process OOM;
- no lost or duplicate epic, currency, or item ownership after every crash point;
- a failed terminal save leaves the live character and inventory intact and retryable;
- DB balance/current-owner rows reconcile exactly with their ledgers;
- a stale revision can never replace a newer revision;
- login either loads one complete revision or fails cleanly-never a partial character.

### Positive findings to retain

- All 124 local base tables use InnoDB.
- Core `sql_save_player()` groups its principal components in a transaction.
- Player item child tables have useful foreign-key/index structure.
- Persistence item/scalar event tables have explicit dedupe/index contracts checked at
  boot.
- Pool connections set `utf8mb4`, have read/write timeouts, and are individually owned
  while borrowed.
- The queue preserves a failed head until its writer reports durable success.
- Locker async jobs do not let their worker traverse live `P_char`/`P_obj` state and use
  generation-aware coalescing.
- Query truncation checks and 1 MiB item sub-batches avoid some legacy fixed-buffer and
  packet-size failures; the local server's `max_allowed_packet` is 16 MiB.
- The local `.env` is ignored by version control; its current mode 0644 is a Phase 00
  security finding and must be restricted without exposing or committing its contents.

These are useful building blocks. The central change is to extend their transactional,
immutable, and idempotent properties to every player-critical path while removing all
external I/O from the simulation loop.
