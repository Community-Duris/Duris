# Session Specification

**Session ID**: `phase02-session02-transactional-inbox-outbox-and-reconciliation`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `c941faba`

## Objectives

1. Add guarded InnoDB inbox, test-state, outbox, and delivery-dedupe schema contracts.
2. Apply one immutable critical command once through prepared statements and canonical
   row locking, returning the stored result for an identical duplicate.
3. Reconcile ambiguous commits by stable operation ID and retain the same command for
   retry when no committed inbox row exists.
4. Deliver bounded typed outbox records at least once with consumer dedupe, retry,
   dead-letter, lifecycle, and redacted health contracts.
5. Provide read-only discrepancy scans and narrowly typed repair actions.

## Design Boundary

The generic repository accepts only the Session 01 typed envelope. It hashes canonical
command bytes and keys in-process, binds values through fixed prepared statements, and
never executes payload bytes as SQL. The test mutation proves atomic inbox/state/outbox
behavior before real gameplay domains use the adapter. An inbox row is authoritative
for duplicate and ambiguous-commit resolution.

The outbox is at-least-once. A consumer transaction records `(consumer_id, outbox_id)`
before acknowledging delivery, and duplicate acknowledgement is success. Delivery
failure never rolls back the already committed gameplay transaction.

## Success Criteria

- [x] Identical operation replay returns one stored result; mismatched reuse fails closed.
- [x] Test mutation, inbox result, and logical outbox rows commit atomically.
- [x] Canonical locking, deadlock retry, and ambiguous-commit lookup retain the same ID.
- [x] Outbox delivery, dedupe, retry, dead-letter, restart, and bounds are deterministic.
- [x] Reconciliation output and diagnostics are bounded and metadata-only.
- [x] Migration/source, focused runtime, isolated schema, format, build, security, and full gates pass.
