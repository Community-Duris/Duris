# Considerations

> Institutional memory for AI assistants. Updated between phases via carryforward.
> **Line budget**: 600 max | **Last updated**: Phase 00 (2026-08-26)

---

## Active Concerns

Items requiring attention in upcoming phases. Review before each session.

### Technical Debt
<!-- Max 5 items -->

- [P00] **Remediation has not started**: Phase 00 has 0 completed sessions, ten session
  stubs, and a planned Session 01 spec/checklist. Phases 01 and 02 are preplanned but
  not active; the next executable workflow step remains `implement` for Phase 00
  Session 01.
- [P00] **Save failures can become data loss**: Deferred-save failures strand occupied
  slots, while terminal paths can extract characters or inventory after failed SQL;
  the legacy pfile fallback is not automatically reconciled (DB-004, DB-005).
- [P00] **Removed or failed state can reappear**: Timers, undead slots, forged-item
  knowledge, and granted commands lack replacement deletes; victim frag publication
  and artifact-bind outputs also have ordering or initialization defects (DB-008, DB-012).
- [P00] **Player save and load are over-broad and inconsistent**: Full saves rewrite
  mostly unchanged state and mutate equipment to snapshot it; login uses N+1 queries,
  mixed revisions, partial-success semantics, and fixed pet-item truncation (DB-009 to DB-011).
- [P00] **Runtime and schema contracts have drifted**: Unsafe DB defaults, pre-validation
  boot writes, an incomplete migration ledger, and outdated database documentation mean
  compatibility cannot currently be inferred from a successful boot (DB-020 to DB-022).

### External Dependencies
<!-- Max 5 items -->

- [P00] **MySQL/MariaDB is the durable authority**: Preserve InnoDB and validate all
  migrations, query plans, and write amplification only on a backed-up development
  clone; never use production for exploratory schema or load work.
- [P00] **Redis has mixed reliability roles**: Cache loss may degrade performance, but
  dirty-player and world-recovery loss affects durability. Treat Redis as optional and
  reconstructible until those domains are separated and independently monitored.
- [P00] **Filesystem fallback is not a recovery protocol**: Current event logs and
  binary pfiles have different replay guarantees. Do not promise recovery until typed,
  checksummed, versioned, idempotent journal and reconciliation paths exist.
- [P00] **Capacity evidence is not representative**: The reviewed local DB has 4 players,
  108 player items, and no usable Performance Schema histograms. Index selection and the
  200-player claim require production-sized cloned histories and non-production ports.
- [P00] **Operational choices remain open**: Gameplay and operations owners still need
  to finalize the checkpoint RPO, queue limits, epic authority, world-worker topology,
  retention policy, pfile retirement, and non-local DB transport mechanism.

### Performance / Security
<!-- Max 5 items -->

- [P00] **Per-pulse epic lookups exceed the event budget**: At 200 damaged players, hit
  regeneration alone can issue about 800 main-thread reads per second; movement regen
  and XP add more work against a 25 ms per-pulse event budget (DB-001).
- [P00] **External I/O can stop the simulation**: Most of roughly 500 direct SQL call
  sites use the main connection, and primary Redis commands have no command timeout;
  aligned maintenance callbacks add predictable whole-game latency spikes (DB-003).
- [P00] **Queue capacity hides outages**: Maximum fixed payload storage is roughly
  512 MiB before allocation overhead, growth occurs under producer locks, and worker
  health checks treat a blocked database write as healthy (DB-013, DB-014).
- [P00] **Capacity telemetry is incomplete**: Query-site latency, main-thread external
  I/O time, oldest work age, revision lag, journal age, retries, circuit state,
  deadlocks, and lock waits are missing, so current counters cannot prove readiness.
- [P00] **Failure logging can disclose private data**: Raw SQL, bounded SQL prefixes,
  PIDs, pointer values, and unconditional `/tmp/garp-item-trace.log` writes combine
  privacy exposure with synchronous filesystem work (DB-019).

### Architecture
<!-- Max 5 items -->

- [P00] **The game thread owns mutable objects**: Build immutable typed snapshots on
  that thread; workers must never traverse live `P_char` or `P_obj` graphs or hold game
  locks across database, Redis, filesystem, or allocator-heavy work.
- [P00] **Revision and acknowledgement identity are mandatory**: Order work per entity,
  apply only newer revisions, acknowledge the exact revision, and clear a dirty
  component only when no newer mutation supersedes that acknowledgement.
- [P00] **Critical domains need one durability boundary**: Epic, wallet/bank, item
  ownership, ledger, audit, and outbox changes require unique operation IDs and one
  idempotent transaction before final gameplay success is reported (DB-006, DB-007).
- [P00] **Queues and recovery must be typed and bounded**: Use byte and age limits,
  backpressure or durable spill, checksums, schema versions, retry classification, and
  idempotent replay; unrestricted raw SQL is not a durable message format.
- [P00] **Both forked snapshot paths are transitional hazards**: Player saves can apply
  stale copies, and player/world children can deadlock after forking a multithreaded
  process. Replace them with long-lived workers or an independently started sidecar.

---

## Lessons Learned

Proven patterns and anti-patterns. Reference during implementation.

### What Worked
<!-- Max 15 items -->

- [P00] **Trace code before trusting architecture prose**: Static tracing exposed that
  full player saves and most gameplay SQL bypass the documented worker pipeline; use
  source behavior as evidence and update documentation with the implementation.
- [P00] **Preserve the InnoDB baseline**: All 124 inspected local base tables use
  InnoDB, and the core `sql_save_player()` path already groups principal components in
  a transaction.
- [P00] **Reuse enforced dedupe contracts**: Persistence item/scalar event tables have
  uniqueness and index contracts checked at boot, and player item child tables already
  provide useful foreign keys and indexes.
- [P00] **Retain failed queue heads**: Existing event workers keep the head until the
  writer reports durable success. Extend this acknowledgement discipline to typed
  player and critical-domain commands.
- [P00] **Follow the locker worker boundary**: `locker_async.c` snapshots immutable
  values on the game thread, coalesces generations, and applies completion later; it is
  the closest existing model for the new persistence pipeline.
- [P00] **Keep connection ownership explicit**: Pool connections are individually owned
  while borrowed, select `utf8mb4`, and have bounded read/write calls. Add async healing,
  operation deadlines, and bounded shutdown without weakening those properties.
- [P00] **Keep payload bounds visible**: Query truncation checks and 1 MiB item
  sub-batches already avoid some fixed-buffer and packet-size failures; preserve
  explicit limits and turn silent truncation into a validated error.
- [P00] **Use focused source-contract regressions**: `tests/async/` already checks save
  failures, queue generations, fallback preservation, schema contracts, and locker
  terminal behavior; extend the nearest test with each behavior change.
- [P00] **Fail closed around destructive operations**: The migration runner and newer
  locker terminal paths demonstrate the desired bias: retain retryable state and do not
  publish destructive completion without durable success.

### What to Avoid
<!-- Max 10 items -->

- [P00] **Do not fork the running server for persistence**: Threads, allocators, client
  libraries, mutable snapshots, and unbounded child lifetimes make both player and
  world fork paths unsafe.
- [P00] **Do not put external I/O in mutation or pulse callbacks**: A Redis or database
  failure must retain dirty work for retry, never trigger a synchronous full save in
  the caller.
- [P00] **Do not clear or destroy before an exact durable ACK**: Dirty flags, floor
  deltas, inventory, characters, and journal records must survive failure and stale
  acknowledgements.
- [P00] **Do not absolute-save shared balances**: Cached account-bank values from one
  character can overwrite another character's update; use checked delta commands and
  publish the authoritative committed result.
- [P00] **Do not split balance, ownership, ledger, and audit writes**: Relative timing
  across independent saves or queues cannot provide atomicity or exactly-once effects.
- [P00] **Do not treat raw SQL as a queue or journal contract**: It is hard to version,
  redact, classify, and deduplicate after an ambiguous commit or replay.
- [P00] **Do not equate large buffers with resilience**: Unbounded recovery time and
  producer-side allocation merely postpone visible failure while increasing memory and
  latency risk.
- [P00] **Do not accept partial or silently truncated loads**: Required player
  components must come from one consistent revision or login must fail cleanly with an
  explicit bounded error.
- [P00] **Do not tune from tiny local plans**: Candidate indexes, partitioning, and
  retention require representative clone measurements plus reconciliation and audit
  ownership before migration.
- [P00] **Do not broaden fixes into legacy modernization**: Keep each session to one
  persistence objective, change nearby code narrowly, and preserve gameplay semantics.

### Tool/Library Notes
<!-- Max 5 items -->

- [P00] **C++20 build despite `.c` suffixes**: Compile server changes with
  `make -C src`; format touched C/C++ lines with `./scripts/format.sh` and verify with
  `./scripts/format.sh --check`.
- [P00] **Spec state is script-derived**: Use
  `.spec_system/scripts/analyze-project.sh --json` instead of manually interpreting
  `state.json`; it currently reports Phase 00 as not started with Session 01 planned and
  no completed sessions.
- [P00] **Database tests require isolation**: Start with the smallest relevant Python or
  shell regression, use `make test-db` only for isolated DB suites, and reserve
  `make test-all` for the complete handoff gate.
- [P00] **Runtime configuration is local and sensitive**: Use `.env` and
  `scripts/start_mud.sh`, never print or commit credentials, and keep development away
  from production data and ports.
- [P00] **MySQL and hiredis defaults are insufficient**: New connections must verify
  charset, time zone, isolation level, SQL mode, TLS or protected transport, connect
  and operation deadlines, and null/error context handling.

---

## Resolved

Recently closed items (buffer - rotates out after 2 phases).

| Phase | Item | Resolution |
|-------|------|------------|
| - | *No resolved items yet* | Phase 00 implementation has not started. |

---

*Initial baseline populated from the PRD and current repository state; future phases update it via carryforward.*
