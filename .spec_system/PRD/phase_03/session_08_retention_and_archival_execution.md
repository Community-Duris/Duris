# Session 08: Retention and Archival Execution

**Session ID**: `phase03-session08-retention-and-archival-execution`
**Status**: Not Started
**Work Window**: One lifecycle execution boundary from due-row selection and dry-run
planning through protected InnoDB archival, manifest/checksum verification, bounded
purge, exact cursors, reconciliation, retry, and operator evidence.

---

## Objective

Enforce approved season/time lifecycle rules in bounded idempotent batches without
losing protected history, breaking current-row reconciliation, or blocking gameplay.

---

## Scope

### In Scope (MVP)

- Add guarded archive-job, batch, cursor, count/checksum, policy-version, approval,
  status, and error metadata plus restricted InnoDB archive tables for manifest entries
  whose approved action requires archival.
- Select due rows with sargable time/season cursors and the Session 05 validated indexes;
  process fixed row/time/byte budgets without offset scans or unbounded transactions.
- Copy active history to archive with stable source identity, verify counts and
  deterministic checksums, record a durable batch completion, then delete or
  pseudonymize active rows only when the exact policy and verification permit it.
- Order parent/child, ledger/current-row, inbox/result/outbox, and cross-table work from
  the lifecycle manifest and run domain reconciliation before and after affected
  batches.
- Keep dry-run as the default, require an explicit non-production-safe target and policy
  version for mutation, and make retry/restart resume the same batch and cursor.
- Integrate archive work with the Session 06 scheduler, operation deadlines,
  backpressure, shutdown, redacted metrics, and operator pause/resume/inspect controls.
- Cover retention boundaries, season boundaries, late rows, duplicate execution,
  failure between copy/verify/delete, policy change, archive corruption, and restore.

### Out of Scope

- External cold-storage or cloud-object-store integration not defined by this
  repository or deployment.
- Personal data request packaging and account erasure, owned by Sessions 09 and 10.
- Production lifecycle execution during development or validation.

---

## Prerequisites

- [ ] Sessions 05 through 07 query paths, maintenance scheduler, and approved lifecycle
      manifest are validated.
- [ ] Phase 02 ledgers/current rows reconcile exactly before any history batch runs.
- [ ] Archive and purge integration tests use isolated databases or backed-up clones.

---

## Deliverables

1. Guarded archive schema, lifecycle job/batch state, fresh-bootstrap synchronization,
   and schema verification under `migrations/`.
2. Typed dry-run, archive, verify, purge/pseudonymize, retry, cursor, and reconciliation
   executors integrated with bounded maintenance scheduling.
3. Operator inspect/pause/resume/report tooling with target and policy safety checks and
   redacted evidence.
4. Focused threshold, season, dependency, protected-ledger, crash-point, duplicate,
   checksum, policy-change, reconciliation, and restore regressions.

---

## Success Criteria

- [ ] Due-row selection is sargable and every run respects configured row, byte,
      transaction-age, and wall-time bounds.
- [ ] Retry or restart resumes the same stable batch and cannot create duplicate archive
      rows or skip active rows.
- [ ] No active row is removed before exact archive count/checksum verification and all
      required domain reconciliation checks succeed.
- [ ] Protected financial, ownership, moderation, and audit history follows only its
      approved retain/archive/pseudonymize rule and never an ordinary cleanup default.
- [ ] Dry-run reports exact intended actions without mutation, and mutation fails closed
      on target, policy, approval, schema, archive, or reconciliation mismatch.
- [ ] Active query paths exclude archived rows and measured working sets improve without
      unacceptable write, lock, or pulse impact.
- [ ] Focused regressions and isolated schema/lifecycle tests pass.
