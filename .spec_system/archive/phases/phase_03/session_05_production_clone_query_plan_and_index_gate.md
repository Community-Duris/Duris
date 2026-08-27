# Session 05: Production-Clone Query Plan and Index Gate

**Session ID**: `phase03-session05-production-clone-query-plan-and-index-gate`
**Status**: Not Started
**Work Window**: One evidence-to-migration boundary covering representative data
qualification, stable query manifests, before/after plans, read latency, write cost,
candidate selection, guarded schema changes, and replay verification.

---

## Objective

Apply only query and index changes that demonstrably improve the implemented workload
on a representative non-production clone without unacceptable write amplification,
locking, or migration risk.

---

## Scope

### In Scope (MVP)

- Define a sanitized query-plan manifest with stable site ID, exact parameter shape,
  expected cardinality, required result ordering, owning feature, and acceptance metric.
- Require representative cloned row distributions for login components, Phase 02
  ledgers/current rows/inbox/outbox, PvP, epic history, progress, item history,
  statistics, trophies, zone touches, accounts, and soft-deleted leaderboard rows.
- Capture compatible MySQL/MariaDB `EXPLAIN ANALYZE` or best available measured-plan
  evidence, execution timing, rows examined, temporary/filesort use, lock waits, and
  cache-state notes before and after each candidate.
- Evaluate the master candidates against final query shapes: direct case-insensitive
  name equality, recent PvP, epic/task membership, zone touch time, active race frag,
  sargable trophy windows, player/pet load joins, and lifecycle cursors.
- Measure representative critical-command, snapshot, login, archive, and maintenance
  insert/update throughput plus index size and lock behavior before accepting a change.
- Add only approved composite indexes, bounded type/predicate repairs, and sargable
  query changes through guarded migrations synchronized with the fresh bootstrap and
  schema verifier.
- Produce a redacted committed summary and keep clone data and raw generated plans in
  ignored local output paths.

### Out of Scope

- Applying speculative indexes because they appeared in the original review.
- Query measurement or migration against production.
- Database server tuning, hardware procurement, sharding, or unrelated schema cleanup.

---

## Prerequisites

- [ ] Sessions 01 through 04 final load and gameplay read query shapes are validated.
- [ ] Phase 02 final transactional schema and workload generators are available.
- [ ] A backed-up representative development clone is positively identified and no
      production target is reachable by the harness.

---

## Deliverables

1. Reproducible sanitized query-plan and representative-data qualification harness under
   `tests/async/`, `scripts/`, or another focused repository location.
2. Before/after read, write, lock, and storage evidence for every accepted or rejected
   candidate, with raw outputs ignored.
3. Guarded additive migration, fresh-bootstrap, verification, and query changes for
   candidates that pass the gate.
4. Focused plan-shape, migration replay, result-order, write-amplification, and
   MySQL/MariaDB compatibility regressions.

---

## Success Criteria

- [ ] Every tested data set satisfies documented representative size/distribution
      criteria or the candidate remains unapplied without a readiness claim.
- [ ] Every accepted index or query change has a stable query ID and reproducible
      before/after plan, timing, rows-worked, write-cost, and lock-impact evidence.
- [ ] No rejected or unmeasured candidate appears in the authoritative migration or
      fresh bootstrap.
- [ ] Result contents and ordering remain equivalent for all changed queries.
- [ ] Guarded migration rerun, fresh bootstrap, schema verification, and rollback or
      recovery procedures pass on isolated databases.
- [ ] No clone rows, player/account values, credentials, raw plans with bound values, or
      generated database artifacts are committed.
- [ ] Focused regressions and isolated schema/query tests pass.
