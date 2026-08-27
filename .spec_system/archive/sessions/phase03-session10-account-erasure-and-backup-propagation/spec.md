# Session Specification

**Session ID**: `phase03-session10-account-erasure-and-backup-propagation`
**Phase**: 03 - Load Path, Schema, and Retention
**Base Commit**: `3c0ab8b3e477a3c73594673d3e43a4785dd6bec6`
**Created**: 2026-08-27

## Objective

Define the authenticated, idempotent account-erasure and restore-tombstone boundary
for every lifecycle store, while keeping canonical mutation unavailable until exact
store actions and controller decisions are externally approved.

## Architecture

- Reuse Session 09 reauthentication, stable account scope, rate, and ownership rules.
- Add request/store/evidence/tombstone schema with stable identities, resumable state,
  exact policy checksum, fence/drain/reconciliation gates, and no raw subject values.
- Derive store ordering and pending actions from the lifecycle dependency graph; an
  unknown, protected, retained-exception, or unapproved action fails closed.
- Model fence, drain, disposition, verify, credential-finalize, and exact-completion
  transitions without providing a canonical destructive adapter.
- Require backup/import/replay preflight to apply non-reversible tombstones before any
  restored identity becomes loadable or externally published.

## Success Criteria

- [ ] Authentication, stable identity, ownership, cancellation, and retry are bounded.
- [ ] All 180 stores receive one ordered action/evidence state or an explicit policy block.
- [ ] Mutation cannot begin before fence/drain and cannot finalize before reconciliation.
- [ ] Protected/value-bearing domains cannot be ad-hoc deleted or orphaned.
- [ ] Tombstones prevent synthetic backup, pfile, journal, cache, and export resurrection.
- [ ] Schema replay, crash points, partial failure, duplicate, and restore tests pass.
- [ ] Canonical erasure remains blocked while controller actions are pending.

## Safety

No production connection, configured database, real account, credential, deletion,
pseudonymization, backup rewrite, or operational script. Dynamic tests are synthetic
and use only disposable databases/directories.

## Next Steps

Run `implement`.
