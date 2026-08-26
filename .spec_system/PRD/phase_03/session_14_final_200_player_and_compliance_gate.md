# Session 14: Final 200-Player and Compliance Gate

**Session ID**: `phase03-session14-final-200-player-and-compliance-gate`
**Status**: Not Started
**Work Window**: One final integrated readiness boundary qualifying representative data,
running all workload ramps and fault points, reconciling every durable domain, testing
schema and data rights, repairing failures, and publishing sanitized pass evidence.

---

## Objective

Prove the complete persistence system meets the master latency, durability, ordering,
resource, login, migration, lifecycle, privacy, and recovery criteria at 200 active
players before the project makes a readiness claim.

---

## Scope

### In Scope (MVP)

- Qualify a backed-up representative development clone and non-production server ports,
  sanitize harness identities, verify production is unreachable, and capture schema,
  migration, policy, data-size, configuration, and build identities.
- Ramp 25, 50, 100, and 200 clients and hold 200 for at least 30 minutes under each
  master profile: idle/scheduled work, movement/regen, group PvE, epic/artifact rewards,
  PvP groups, two-character banking, trade/locker/auction movement, and reconnect/
  copyover with large inventories and pets.
- Inject database latency/reset/outage/deadlock/ambiguous commit, Redis latency/loss,
  worker crash, game crash at every journal/transaction/ACK edge, disk failure,
  terminal-transition failure, stale revision, duplicate critical replay, archive
  failure, migration drift, and backup restore after erasure.
- Measure p50/p95/p99/max pulse and event time, event debt, main-thread external I/O,
  query counts/latency by stable site, load query/operation complexity, queue/journal/
  inbox/outbox age and bytes, retries, locks, revisions, checkpoints, and maintenance
  cursor lag.
- Reconcile player revisions, epic balances, banks, wallets, current item owners,
  ledgers, inbox/results, outbox delivery, archive batches, migration history, and
  lifecycle actions after every workload and fault run.
- Exercise complete/failed login, export isolation and secret exclusion, erasure across
  every manifest store, tombstone restore protection, retention dry-run/mutation on
  synthetic expired rows, and boot compatibility drift.
- Repair defects discovered by the gate and rerun affected plus full acceptance suites
  without weakening thresholds, bounds, privacy, atomicity, or fail-closed behavior.
- Commit only sanitized reproducible harnesses and an evidence summary; keep raw cloned
  data, logs, plans, exports, credentials, and generated load artifacts ignored.

### Out of Scope

- Production migration, load, fault, retention, export, or erasure execution.
- Lowering acceptance thresholds or excluding a failed workload to manufacture a pass.
- Unrelated gameplay tuning, infrastructure scaling, or a Phase 04 feature program.

---

## Prerequisites

- [ ] Sessions 01 through 13 are completed and validated.
- [ ] Every earlier phase-specific gate and reconciliation suite passes.
- [ ] The representative clone, test accounts, load clients, fault controls, archive
      tables, backups, and output locations are isolated from production and disposable.

---

## Deliverables

1. Reproducible integrated load/fault/lifecycle harnesses and safe target qualification
   under `tests/async/`, `scripts/`, or focused ignored-output tooling.
2. Sanitized gate report recording build/schema/policy identity, workload, faults,
   thresholds, query/resource metrics, reconciliation, privacy, and exact outcomes.
3. Narrow repairs and regressions for every issue discovered during the gate.
4. Final operator readiness checklist and remaining evidence-backed limitation record.

---

## Success Criteria

- [ ] All eight profiles complete the 25/50/100/200 ramp and 30-minute 200-client hold
      on qualified representative data.
- [ ] p99 game pulse remains below 250 ms, p99 new-event processing remains within 25
      ms, and no sustained event debt or main-thread external I/O occurs.
- [ ] Normal oldest critical command remains below 1 second, checkpoint age meets the
      approved RPO, and every queue/journal/outbox/maintenance resource stays within
      configured byte, age, retry, and shutdown bounds.
- [ ] No epic, currency, item-owner, revision, archive, export, or erasure effect is
      lost or duplicated at any defined crash, deadlock, outage, ambiguous commit, or
      replay point.
- [ ] Every player login publishes one complete revision or fails cleanly, uses bounded
      query counts, and demonstrates linear item/pet assembly at representative size.
- [ ] Current balance/owner rows reconcile exactly with ledgers and all inbox, result,
      outbox, archive, migration, and lifecycle invariants pass after every run.
- [ ] Retention obeys approved policy, export cannot cross account scope or reveal
      secrets, and erasure remains effective after journal replay and backup restore.
- [ ] Schema checksum/history drift and incompatible boot configuration fail before any
      database write or gameplay availability.
- [ ] Raw evidence contains no committed credentials or private player/account data,
      and the sanitized report supports every readiness claim.
- [ ] Focused tests, isolated database suites, formatting checks, `make -C src`, and
      `make test-all` pass.
