# Session 02: Transactional Inbox, Outbox, and Reconciliation

**Session ID**: `phase02-session02-transactional-inbox-outbox-and-reconciliation`
**Status**: Not Started
**Work Window**: One generic database command boundary from inbox dedupe and canonical
row locking through atomic apply, typed outbox delivery, ambiguous-commit lookup, and
operator reconciliation, before gameplay-domain cutover.

---

## Objective

Provide a reusable InnoDB transaction and outbox contract that applies one operation ID
once, exposes its committed result, and converges safely after duplicate or ambiguous
execution.

---

## Scope

### In Scope (MVP)

- Add guarded, re-runnable migration and fresh-bootstrap contracts for operation inbox,
  typed result metadata, transactional outbox, delivery state, and required indexes.
- Implement a typed repository/worker adapter that locks affected rows in canonical
  order, claims or reads the operation ID, validates command and payload hashes, applies
  a test domain mutation, stores the result and outbox rows, and commits once.
- Return the previously committed result for an identical duplicate and fail closed on
  an operation-ID collision with different command type, keys, version, or payload.
- Resolve timeout, connection reset, deadlock, lock wait, and ambiguous commit outcomes
  by operation-ID lookup and retry the original immutable command only when safe.
- Deliver typed bounded outbox records at least once, dedupe consumers by outbox ID,
  retain undelivered rows, and expose retry, age, dead-letter, and destination health.
- Add redacted read-only reconciliation and narrowly guarded repair interfaces for
  inbox/result/outbox discrepancies without executing raw SQL journal payloads.

### Out of Scope

- Production epic, currency, ownership, combat, artifact, guild, boon, reward, or zone
  mutations.
- Historical ledger backfill or current-owner construction.
- Phase 03 migration ledger and retention policy.

---

## Prerequisites

- [ ] Session 01 critical identity, journal, multi-key coordinator, and fences are
      validated.
- [ ] Database work targets only an isolated development database or backed-up clone.
- [ ] Phase 00 connection invariants and redacted error logging remain enforced.

---

## Deliverables

1. Additive inbox/outbox schema, fresh-bootstrap synchronization, and verification under
   `migrations/`.
2. Generic typed transaction, duplicate-result, ambiguous-commit, and reconciliation
   repositories in focused `src/` modules.
3. Bounded outbox dispatcher, consumer-dedupe contract, lifecycle integration, and
   operator diagnostics.
4. Focused isolated-MySQL tests for duplicate IDs, mismatched payloads, row-lock order,
   deadlocks, commit ambiguity, outbox retry, restart, and reconciliation.

---

## Success Criteria

- [ ] An operation ID can commit one logical mutation and one logical outbox set only.
- [ ] An identical replay returns the original committed result without applying again.
- [ ] Reusing an operation ID for a different payload fails closed and emits a redacted
      integrity alert.
- [ ] Ambiguous commit and deadlock tests converge using the original operation ID.
- [ ] Outbox delivery failure cannot roll back a committed gameplay transaction or lose
      the retained notification record.
- [ ] No database worker executes SQL supplied by a command or journal payload.
- [ ] Inbox, result, outbox, retry, dead-letter, and reconciliation state is bounded,
      queryable, and redacted.
- [ ] Focused regressions, isolated schema tests, formatting checks, and `make -C src`
      pass.
