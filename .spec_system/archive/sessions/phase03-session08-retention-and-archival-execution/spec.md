# Session Specification

**Session ID**: `phase03-session08-retention-and-archival-execution`
**Phase**: 03 - Load Path, Schema, and Retention
**Base Commit**: `77346e731a7ec76b61336c5bd1514a0b704890ab`
**Created**: 2026-08-27

## Objective

Implement a bounded, resumable lifecycle execution boundary that can plan, archive,
verify, and conditionally finalize approved rules while the checked-in pending policy
remains dry-run-only and incapable of mutation.

## Architecture

- Additive guarded lifecycle job/batch schema with stable identity, policy version,
  cursors, counts/checksums, deadlines, approval evidence, status, and redacted errors.
- Typed planner/executor with dependency ordering, fixed row/byte/time bounds, stable
  retry identity, copy/verify/finalize crash recovery, and reconciliation hooks.
- Dry-run default plus exact target, environment, role, manifest, schema, approval, and
  policy gates before any mutation connection or SQL can be reached.
- Session 06 scheduler registration remains disabled/blocked when the canonical policy
  has no approved destructive rule; operator tooling supports inspect/pause/resume and
  sanitized evidence without changing data.

## Success Criteria

- [ ] Plans and batches have stable IDs/cursors and fixed row, byte, transaction-age,
      and wall-time budgets.
- [ ] Retry/restart resumes copy, verification, or finalization without duplicates or
      skipped source identity.
- [ ] Finalization is unreachable until exact counts/checksums and required domain
      reconciliation succeed under the same policy/approval identity.
- [ ] Protected stores and the checked-in pending policy cannot reach mutation SQL.
- [ ] Dry-run, inspect, pause, resume, and report paths are redacted and non-mutating.
- [ ] Schema/source/model regressions cover bounds, dependency order, crash points,
      duplicate retry, corruption, policy change, restore, and safety gates.
- [ ] Formatting, build, full regression, review, security/BQC, and validation pass.

## Safety

No production lifecycle run and no active-data archive, purge, pseudonymization, or
deletion. Tests use synthetic records and, if available, isolated disposable schemas.
The missing controller approval is a deliberate blocked execution state, never an
inferred decision.

## Next Steps

Run `implement`.
