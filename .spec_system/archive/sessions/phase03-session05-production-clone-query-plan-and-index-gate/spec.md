# Session Specification

**Session ID**: `phase03-session05-production-clone-query-plan-and-index-gate`
**Phase**: 03 - Load Path, Schema, and Retention
**Status**: Not Started
**Created**: 2026-08-27
**Base Commit**: `abeba9865a378ac2fba6d7bf6b90cc89d797fcbe`

## Overview

Create a reproducible, redacted, fail-closed plan gate for the final Phase 02/03 query
shapes. The configured target must be non-production and loopback, and its row-count and
distribution qualification must pass before any candidate index can be accepted. If it
is too small, the correct result is a committed rejection report, no migration, and no
readiness claim.

## Objectives

1. Inventory stable query IDs, parameter shapes, ordering, owning features, and candidate
   indexes for player load, PvP/epic history, zone touch, leaderboards, and trophies.
2. Qualify representative cardinality and skew without emitting bound values or rows.
3. Capture sanitized plan shape and timing using the best supported MySQL/MariaDB method.
4. Measure candidate storage, write, and lock impact only on isolated temporary clones.
5. Add guarded migration/bootstrap changes only for a candidate that passes every gate.

## Scope

### In Scope

- A versioned JSON manifest and Python/C++ guarded local harness.
- Stable aggregate-only evidence written under ignored `tmp/query-plan-gate/`.
- Current-index and query-shape comparison against authoritative bootstrap DDL.
- Explicit qualification thresholds and per-candidate accepted/rejected/unmeasured state.
- Source/migration tests proving no rejected candidate leaks into schema.

### Out of Scope

- Production access, operational DDL, server tuning, fabricated representative data,
  speculative indexes, or committing raw plans/bound values.

## Success Criteria

- [x] Target classification is non-production and loopback before database inspection.
- [x] Every query has a stable ID, parameter type, ordering contract, and acceptance metric.
- [x] Aggregate qualification is reproducible and redacted.
- [x] Unqualified data rejects every candidate without DDL or readiness language.
- [x] Any accepted candidate has before/after read, write, lock, and storage evidence.
- [x] Migration/bootstrap/schema verification change only for accepted candidates.
- [x] Raw evidence is ignored and no credential, row value, or clone artifact is committed.
- [x] Focused tests, local harness, build, formatting, and `make test-all` pass.

## Technical Approach

The manifest is authoritative and contains no bound player/account values. The harness
reads `.env` only to classify `ENVIRONMENT` and `DB_HOST`, then obtains aggregate table
statistics and sanitized plan operators. Qualification thresholds are conservative and
feature-specific. Candidate status is `accepted`, `rejected`, or `unmeasured`; only
`accepted` permits an additive guarded migration. Raw evidence stays in ignored output.

## Planned Deliverables

- `tests/async/query_plan_manifest.json`
- `tests/async/query_plan_gate.py`
- `tests/async/test_query_plan_gate.py`
- `.gitignore` raw-evidence exclusion
- `query-plan-gate-report.md` aggregate committed result
- Migration/bootstrap changes only if a candidate passes

## Safety Boundary

No production migration, wipe, or operational script may run. The harness is read-only
except for connection-local temporary tables used for isolated write-cost measurement.

## Next Steps

Run the `implement` workflow step.
