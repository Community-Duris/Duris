# Plan 5: independent audit, lifecycle, and release qualification

Start from add-double-entry HEAD 49af585c4. The reconciler and route inventory
can be developed against current native rows and synthetic committed evidence
while Plans 1-4 proceed. Final release certification consumes their results,
but this plan's code and fixtures do not require their implementation branches.
See [R6-R8](../REMAINING_REQUIREMENTS.md).

## Result

Operators can read an immutable, bounded provenance and money-flow account of
the economy, detect disagreement with native authority, and safely pause
affected writers. Full-feature completion has reproducible evidence rather
than a count of green source-contract tests.

## Starting files and first checks

Inspect scripts/validate_economy_accounting.py,
scripts/generate_economy_writer_coverage.py,
scripts/audit_accounting_invariants.py, and the registry/matrix in this
documentation directory. Start with
python3 tests/async/test_economy_writer_coverage_contract.py and
python3 tests/async/test_audit_accounting_invariants.py. Run
python3 scripts/validate_economy_accounting.py to reproduce the known
writer-census drift before changing the census.

## Work

1. Finish the semantic writer census from reachable call sites across commands,
   special procedures, world resets, SQL/direct writes, admin tools, rewards,
   and flatfile. Record backend, authority owner, source/sink class, root type,
   status, refusal, and executable proof for each route. Resolve current
   validator drift and refresh the machine-readable registry/matrix while
   preserving unrelated worktree changes.
2. Build a read-only reconciler separate from mutation code. Compare native
   wallet/bank/pile/escrow/claim/treasury balances with account effects and
   postings; compare each admitted live/tombstoned UID with current-owner and
   ownership/reference history. Detect unbalanced roots, unlinked children,
   missing/extra events, duplicate source events, unknown legacy origins, and
   evidence loss. Never auto-post an adjustment to clear an exception.
3. Add bounded staff/operator queries and export views for holdings, issuance
   and sinks by policy, item provenance, realized prices, route status, and
   reconciliation exceptions. Restrict personal aliases and access; preserve
   non-personal IDs after erasure. Guard corrections and restitution behind
   expected-state checks, original-operation linkage, and operator authority.
4. Register accounting evidence in fresh/upgrade migration, runtime schema
   checks, backup/restore, retention, deletion/erasure, and any enabled export
   path. An old binary must not silently write an active epoch. Restore must
   preserve lineage, source dedupe, exact receipt replay, and pause authority
   when evidence is missing.
5. Define measurable operation size, latency, storage growth, checkpoint, and
   reconciliation budgets. Run fault/restart/replay and real gameplay journeys
   for both backends. Advance a route from legacy to observed to enforced only
   with current executable evidence; make unsupported routes refuse in an
   active epoch.

## Independent acceptance

- Feed the reconciler deliberately corrupted disposable fixtures: missing
  posting, extra coin effect, duplicate UID/source event, orphan item reference,
  stale native balance, unknown opening, and erased alias. Each produces a
  specific exception without changing authority.
- Writer census and release validator pass with zero unclassified real writers.
  Nonwriters and unsupported routes have reviewed reachability/refusal proof.
  The current validator failure is a task input, not a pass to waive.
- SQL fresh/upgrade and replay checks run on MySQL and MariaDB. Flatfile
  journal interruption, backup/restore, retention, and UID/source dedupe pass.
  Both server builds and the focused gameplay/fault matrix pass.
- Release report identifies exact tested commit, backend, test command, result,
  sampled workload, route coverage, and any intentionally unsupported path.
  A synthetic fixture suite is recorded separately from real player journeys.

## Boundary and handoff

This plan owns audit and proof, not domain mutation. Plans 1-4 own repairs when
audit finds a missing writer or atomicity defect. Final activation requires the
combined release report and a separately authorized maintenance/deployment
decision; an incomplete backend or unknown writer keeps full completion blocked.
