# PRD Phase 03: Load Path, Schema, and Retention

**Status**: Complete
**Sessions**: 14 (initial estimate)
**Estimated Duration**: Adaptive; each session continues through its verification boundary

**Progress**: 14/14 sessions (100%)

---

## Overview

Phase 03 completes the persistence scalability program by making login one bounded,
consistent, fail-closed read workflow; removing the remaining read-side query fan-out;
and validating schema access paths on representative cloned data. The game thread
materializes typed load results in linear time, while database workers read immutable
row data from one consistent snapshot and never construct or traverse live game objects.

The phase also turns schema and data lifecycle behavior into enforceable contracts. It
adds a complete immutable migration ledger, verifies compatibility before boot writes
lookup data, staggers and bounds scheduled maintenance, and defines retention and
archival for every growing history. The Phase 03 security scope adds a complete personal
data inventory, authenticated access/export, account erasure, and restore-safe deletion
propagation without inventing legal conclusions or deleting protected economy,
ownership, moderation, or audit evidence outside an approved policy.

The final sessions reconcile documentation to the implemented system and provide a
fail-closed 25-to-200-client workload and fault gate. All 14 sessions are complete and
validated. The user explicitly postponed the representative 200-account/four-hour
execution, so Phase 03 makes no 200-player capacity-readiness claim.

---

## Progress Tracker

| Session | Name | Status | Work Window | Validated |
|---------|------|--------|-------------|-----------|
| 01 | Consistent Player Load Transaction | Complete | Async typed login read, one snapshot, required components, and fail-closed publication | 2026-08-27 |
| 02 | Batched Item Ownership and Linear Assembly | Complete | Set-based item/owner metadata and O(N) object graph materialization | 2026-08-27 |
| 03 | Batched Pet Graph Hydration | Complete | Set-based pet items/metadata, explicit bounds, staged publication, and recovery semantics | 2026-08-27 |
| 04 | Set-Based PvP and Epic Task Reads | Complete | Recent-death aggregation, in-memory task selection, and remaining read fan-out removal | 2026-08-27 |
| 05 | Production-Clone Query Plan and Index Gate | Complete | Fail-closed qualification harness; local fixture unqualified and no candidate applied | 2026-08-27 |
| 06 | Bounded Maintenance Scheduler | Complete | Staggered cadences, async jobs, row/time budgets, durable cursors/completions, and pulse isolation | 2026-08-27 |
| 07 | Data Processing and Retention Contract | Complete | Complete technical inventory, pending decision markers, subject mapping, and fail-closed policy | 2026-08-27 |
| 08 | Retention and Archival Execution | Complete | Idempotent archive state machine, schema, verification, reconciliation gates, dry-run controls, and policy-disabled scheduler slot | 2026-08-27 |
| 09 | Authenticated Personal Data Export | Complete | Guarded schema, exact manifest mapping, reauthentication/package/spool contract; canonical activation blocked by pending disclosure policy | 2026-08-27 |
| 10 | Account Erasure and Backup Propagation | Complete | Guarded request/store/tombstone schema and restore preflight; canonical mutation blocked by pending policy | 2026-08-27 |
| 11 | Immutable Migration Ledger and Runner | Complete | Honest 170-table baseline, immutable manifest, success-last history, chain-head tamper evidence, and exact resume | 2026-08-27 |
| 12 | Boot Schema and Lookup Compatibility | Complete | Exact pre-write schema/history/connection gate and transactional checksummed lookup publication | 2026-08-27 |
| 13 | Documentation and Operator Contract | Complete | Source-traced architecture, database, configuration, lifecycle, testing, and recovery guidance | 2026-08-27 |
| 14 | Final 200-Player and Compliance Gate | Complete | Fail-closed integrated gate, local migration/runtime proof, dual-engine verification, sanitized non-claim | 2026-08-27 |

---

## Completed Sessions

- Session 01: Consistent Player Load Transaction (completed 2026-08-27)
- Session 02: Batched Item Ownership and Linear Assembly (completed 2026-08-27)
- Session 03: Batched Pet Graph Hydration (completed 2026-08-27)
- Session 04: Set-Based PvP and Epic Task Reads (completed 2026-08-27)
- Session 05: Production-Clone Query Plan and Index Gate (completed 2026-08-27; local target unqualified)
- Session 06: Bounded Maintenance Scheduler (completed 2026-08-27)
- Session 07: Data Processing and Retention Contract (completed 2026-08-27; destructive rules disabled)
- Session 08: Retention and Archival Execution (completed 2026-08-27; canonical mutation disabled)
- Session 09: Authenticated Personal Data Export (completed 2026-08-27; canonical activation disabled)
- Session 10: Account Erasure and Backup Propagation (completed 2026-08-27; canonical mutation disabled)
- Session 11: Immutable Migration Ledger and Runner (completed 2026-08-27)
- Session 12: Boot Schema and Lookup Compatibility (completed 2026-08-27)
- Session 13: Documentation and Operator Contract (completed 2026-08-27)
- Session 14: Final 200-Player and Compliance Gate (completed 2026-08-27; representative live run deferred, no capacity claim)

---

## Upcoming Sessions

None. Phase 03 is complete; no Phase 04 is defined or started.

---

## Objectives

1. Load every required player component through one consistent read transaction and
   publish either one complete revision or a clean login failure.
2. Batch item ownership, item metadata, pet metadata, affects, and descriptions and
   assemble player and pet object graphs in O(N) time with explicit validated bounds.
3. Replace recent-PvP-death, random epic task, and other audited read fan-out with
   set-based or hydrated in-memory access that preserves gameplay semantics.
4. Capture representative production-clone row counts and `EXPLAIN ANALYZE` evidence,
   then apply only indexes and query-shape changes whose read benefit and write cost
   pass a documented gate.
5. Stagger scheduled work deterministically and execute database, Redis, filesystem,
   and large CPU work through bounded jobs with row/time budgets and continuation
   cursors.
6. Define an enforceable data processing, season, retention, archival, erasure, and
   audit-exception contract for every database table and non-database store.
7. Provide authenticated, account-scoped personal data export and idempotent erasure
   workflows that prevent cross-account disclosure and post-restore resurrection.
8. Record every new migration with an immutable ID and checksum, adopt existing schemas
   honestly, and fail closed on missing, reordered, or modified history.
9. Verify the complete required schema and connection contract before any boot write,
   then publish versioned race/class lookup data atomically.
10. Reconcile all operator and developer documentation and pass the integrated
    25-to-200-client load, fault, migration, lifecycle, and privacy gate.

---

## Prerequisites

- All Phase 00, Phase 01, and Phase 02 sessions are completed and validated.
- Phase 01 provides revisioned typed workers, exact acknowledgements, bounded journals,
  and fork-free player/world recovery.
- Phase 02 provides authoritative epic, bank, wallet, and item-owner rows plus immutable
  ledgers, operation inbox/results, transactional outbox, and reconciliation tooling.
- Carryforward, documentation, schema, and gate evidence from every earlier phase is
  reconciled before Session 01 is planned.
- Query, migration, retention, archive, load, and fault work uses only isolated
  databases or backed-up representative development clones on non-production ports.
- C/C++ changes use the repository C++20 build, changed-line formatting checks, and
  focused regressions under `tests/async/`.

---

## Planning Assumptions And Resolutions

### Working Assumptions

- Phase 02 current-owner, ledger, inbox, outbox, and domain-revision tables are the
  authoritative load and lifecycle inputs after their gate. The current source still
  queries `persistence_item_events` once per item, but Phase 03 plans against the
  explicitly defined Phase 02 replacement rather than optimizing that transitional
  history query.
- Login database work uses a bounded read-worker path. A 200-player reconnect storm
  currently serializes status, ancillary rows, inventory, ownership checks, and pets on
  the main connection; returning typed rows from one consistent transaction preserves
  game-thread ownership while removing that whole-loop stall risk.
- The first archive implementation uses restricted InnoDB archive tables in the same
  protected database because the repository defines no external object-store,
  encryption-key, or archive-service contract. Active queries exclude archive tables,
  archive batches carry checksums and policy identity, and a later deployment may add a
  separately approved cold-storage exporter without changing retention semantics.
- Engineering records only controller-approved purposes, lawful bases, retention
  windows, and exceptions. Missing approval never becomes an invented legal claim:
  destructive lifecycle actions fail closed while inventory, export, dry-run, and
  evidence collection remain usable.
- Existing deployments receive one explicit verified baseline-adoption record rather
  than fabricated per-step history. The present runner has over one hundred operations
  but records only selected data-copy markers, so claiming every historical step ran
  would create false evidence.
- Session boundaries remain feature and verification seams. When Phase 03 becomes
  active, `plansession` re-runs source and schema analysis against the implemented prior
  phases and may refine file-level tasks without weakening these outcomes or gates.

### Conflict Resolutions

- Phase 03 became active only after Phases 00 through 02 and their transition evidence
  completed. The user boundary remains explicit: complete Phase 03 and stop before any
  Phase 04 planning or implementation.
- The master Phase 03 outline names login, read queries, indexes, retention, migration
  compatibility, and documentation, while `SECURITY-COMPLIANCE.md` assigns personal
  data inventory, access/export, erasure, and backup propagation to P03. Both are
  normative: Sessions 07 through 10 include the security scope and do not claim GDPR
  compliance until approved policy and end-to-end tests provide evidence.
- Scheduled maintenance is a deferred master requirement but is not repeated in the
  six-line Phase 03 outline. No Phase 04 is defined, and exact-modulus maintenance is a
  confirmed whole-game latency risk, so Session 06 owns the remaining scheduler and
  bounded-work requirement.
- Phase 01 and Phase 02 gates update documentation for their intermediate systems, but
  the master PRD still requires a final Phase 03 correction. Session 13 traces the
  completed integrated implementation and removes stale legacy claims rather than
  duplicating earlier transition notes.
- Earlier phase load gates prove their own persistence boundaries. Session 14 reruns
  the complete eight-profile workload and fault matrix after login, indexes,
  maintenance, lifecycle, and schema contracts are present; it complements rather than
  replaces the earlier gates.

---

## Technical Considerations

### Architecture

Login has a read-side analogue to the write architecture. A bounded worker owns one
database connection and one consistent read transaction, returns only typed rows and
classified errors, and never calls object or character constructors. The game thread
validates revision and bounds, creates the character and object graphs with ID maps,
and publishes them only when every required component is complete. Partial results are
discarded safely, and transient failure never enters the world with missing skills,
affects, items, ownership, or pets.

Read optimization follows query shape, not guesswork. Set-based rewrites land before
index selection; a reproducible manifest records row counts, plans, timing, buffer or
row work where available, and write amplification on a representative clone. Migrations
apply only the candidates that pass that gate and keep fresh bootstrap, upgrade, and
boot verification synchronized.

Lifecycle policy is data, not scattered maintenance SQL. A machine-readable manifest
maps each active, history, ledger, inbox, outbox, journal, cache, log, pfile, export, and
backup record to purpose, subject key, season, retention, archive, erasure, and exception
rules. Archive and purge jobs are resumable, idempotent, bounded, redacted, dry-run by
default, and reconciled before deletion. Restores apply durable erasure tombstones
before service opens so old backups cannot recreate deleted identities.

### Technologies

- C++20 typed read DTOs, bounded worker jobs, game-thread materialization, and ID maps
- MySQL or MariaDB consistent reads, InnoDB transactions, archive tables, and measured
  composite indexes
- Additive guarded migrations with immutable IDs, SHA-256 checksums, schema manifests,
  and fresh-bootstrap synchronization
- Existing Phase 01 journal/worker and Phase 02 current-row/ledger/inbox/outbox contracts
- Python and shell source-contract, isolated-MySQL, query-plan, lifecycle, and load tests
- Repository pulse, query-site, queue, revision, operation, and reconciliation telemetry
- Versioned JSON or similarly typed export bundles in ignored permission-restricted paths

### Risks

- A long consistent read can block purge or transactional commands: keep the transaction
  bounded, use one documented isolation level, measure age, and reject oversized loads
  before materialization.
- Row batching can move N+1 work into quadratic CPU assembly: index every row by stable
  ID once, validate graph cycles and parents, and assert linear operation counts.
- Indexes can improve reads while harming critical writes: measure representative
  insert/update cost and lock behavior before migration and retain before/after evidence.
- Retention can break reconciliation or legal/audit duties: let the approved manifest
  distinguish purge, archive, pseudonymize, and retain actions and verify ledgers before
  removing active rows.
- Export can disclose another player's or account's data: derive scope from the
  authenticated account, exclude secrets, classify shared records, and test isolation
  and one-time delivery.
- Erasure can resurrect from journals, Redis, pfiles, or backups: fence new operations,
  drain or cancel by domain rules, propagate tombstones to every recovery path, and
  test a restore before reporting completion.
- Migration history can falsely bless drift: record verified baseline adoption
  explicitly, reject checksum mismatch, and validate the required schema independently.
- A final load report can expose real player data: use cloned or synthetic identifiers,
  redact evidence, store generated output only in ignored locations, and commit only
  sanitized summaries.

### Relevant Considerations

- [P00] **Player save and load are over-broad and inconsistent**: Sessions 01 through
  03 provide one complete revision, set-based metadata, explicit bounds, and linear
  assembly.
- [P00] **Runtime and schema contracts have drifted**: Sessions 11 through 13 align the
  migration ledger, boot manifest, lookup publication, and documentation.
- [P00] **Capacity evidence is not representative**: Session 05 and Session 14 require
  backed-up representative clones and non-production ports.
- [P00] **Operational choices remain open**: Session 07 turns retention and audit
  decisions into an explicit approved manifest rather than embedding assumptions in SQL.
- [P00] **MySQL/MariaDB is the durable authority**: Login snapshots, archive records,
  migration state, and erasure outcomes remain transactional InnoDB data.
- [P00] **Do not tune from tiny local plans**: Candidate query and index changes cannot
  pass without clone row counts, plans, timing, and write-cost evidence.
- [P00] **Do not accept partial or silently truncated loads**: Required components,
  inventory, and pets publish together or login fails cleanly with a bounded reason.
- [P00-S08] **Retention and data-subject rights are not implemented end to end**:
  Sessions 07 through 10 are the implementation and evidence boundary for that finding.

---

## Success Criteria

Phase complete when:
- [x] All 14 sessions completed and validated
- [x] Login reads one consistent durable revision and either publishes every required
      component or fails cleanly without a partial character
- [x] Item ownership, item metadata, pet metadata, affects, descriptions, and container
      graphs are fetched in bounded set-based queries and assembled in O(N) time
- [x] No fixed silent pet or inventory truncation remains; configured bounds produce an
      explicit safe error and no partial publication
- [x] Recent-PvP-death and epic task selection perform no N+1 or `ORDER BY RAND()` work
      in gameplay callbacks and preserve their documented results
- [x] Candidate indexes remain gated on representative evidence, and bootstrap,
      migration, and verification contracts agree
- [x] Scheduled maintenance is deterministically staggered, row/time bounded,
      cursor-resumable, and performs no external I/O on the simulation thread
- [x] Every growing table and non-database store has a versioned purpose, subject,
      season, retention, archive, erasure, and audit-exception classification; pending
      controller decisions fail closed
- [x] Archive and purge runs are idempotent, resumable, reconciled, dry-run safe, and
      incapable of deleting protected ledger or ownership history outside policy
- [x] Authenticated export includes the correct account scope, excludes credentials and
      other subjects' protected data, and uses expiring auditable delivery
- [x] Account erasure covers database rows, caches, journals, local files, exports, and
      restore-time tombstones without reviving deleted identity or losing retained audit
      integrity
- [x] Every post-baseline migration has one immutable ID and checksum; rerun is exact,
      partial failure is recoverable, and checksum or schema drift fails closed
- [x] Boot validates required migration, table, column, index, engine, collation, and
      connection invariants before writing versioned lookup data atomically
- [x] README and database, architecture, configuration, testing, runbook, lifecycle,
      migration, export, and erasure guidance match traced implementation
- [x] The complete eight-profile 25-to-200-client workload and fault matrix is encoded
      in a strict gate that cannot issue a readiness pass without complete evidence; its
      representative live execution is explicitly deferred and no capacity claim is made
- [x] Relevant focused tests, isolated schema tests, formatting checks, `make -C src`,
      and the full repository gate pass

---

## Dependencies

### Depends On

- Phase 00: Correctness and Immediate Lag Removal
- Phase 01: Replace Forked Full Saves
- Phase 02: Transactional Gameplay Domains
- Phase 02 carryforward, documentation, domain-gate, reconciliation, and schema evidence
  before execution

### Enables

- A defensible 200-player readiness claim backed by integrated load and fault evidence
- Enforceable schema, retention, archive, export, and erasure operations
- Final workflow audit, pipeline, infrastructure, carryforward, and documentation review
- Project completion; no Phase 04 is currently defined
