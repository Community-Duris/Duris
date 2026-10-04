# Plan 5 fresh-world SQL audit and release handoff

Owner boundary: independent audit/lifecycle/qualification work in this chat;
Plans 1–4 are being implemented by a separate agent on another system.
Delivery branch: `experimental-accounting`. This isolated development branch
starts at published `9d0ea2a1dd49cdb3fa132e5d38035c931a080b14`.
Production repair under #664 is excluded. Release and activation remain BLOCKED.

## Acceptance contract and ownership

Owned slices are #490 sections 4, 6 and 8 / R7–R8: independent native/evidence
reconciliation, bounded ID-based operator views, restore continuity, deterministic
fault/reference-model/load tooling, and this report. Candidate files are
`scripts/reconcile_economy_accounting.py`, `scripts/economic_sql_audit_snapshot.py`,
`scripts/economic_sql_audit_origins.py`, `scripts/audit_accounting_invariants.py`,
`scripts/persistence_backup.py`, `scripts/persistence_restore.py`,
`scripts/qualify_database_restore.py`, and their focused tests. Before each slice,
inspect current ownership and limit edits to its necessary files.

Shared types, reason/source registry, `writers.json`, generated coverage matrix,
migrations, transaction owners, gameplay producers, activation and central test
registration remain with the producer/integration owner. Supply evidence and
narrow interface requests through this file; do not allocate migrations here.

## Retained evidence and current baseline

The current implementation already contains opening/source/receipt and mapping
checks, UID history/lifetime/topology checks, pending-claim attribution, realized
prices, persisted ship/guild treasury diagnostics and currency revision/value
restore verification. Preserve the scoped evidence in
[October 3 review status](REVIEW_STATUS_2026-10-03.md), including delivered active
alchemy and native COMMIT-reply-loss qualification; it is pinned to its original
inputs and is not a current whole-candidate release pass.

At `9d0ea2a1d`, direct Windows Python execution passes:

- `python tests/async/test_reconcile_economy_accounting.py`: 61 tests.
- `python tests/async/test_economic_sql_audit_origins.py`: 10 tests.
- `python tests/async/test_audit_accounting_invariants.py`: 16 tests.

These are pure/component tests, not native SQL or actual player journeys.

## Demonstrated gaps and implementation sequence

1. SQL detail export uses inner root joins, hiding detail rows whose root is
   absent. Add bounded database-wide orphan collection in the same read-only cut
   and independent specific exceptions. A missing root has no trustworthy epoch
   or lineage, so do not silently discard it using the requested scope.
2. Operator views lack an exact operation lookup and account-key filtering;
   provenance currently reads only selected-epoch events. Extend existing views
   with safe ID filters and retained-history evidence, with explicit coverage.
3. SQL restore checks migration history, epics and wallet/bank value chains, but
   do not independently qualify all economic roots, custody, sources, epochs,
   pending publication/recovery and allocator authority at one boundary. Inspect
   the existing backup/restore fixtures before selecting the next coherent slice.
4. The exporter bounds rows/bytes and refuses oversized cuts, but does not yet
   provide durable fair resumable historical sweeps. Preserve this limitation;
   operation IDs/timestamps are not commit watermarks. Interrupted or partial
   work must never be an all-clear.
5. Integrated deterministic reference/fault/load tooling and predeclared budgets
   still need a pinned combined candidate and exact producer interfaces.

## Producer interfaces and release dependencies

- Supply native account lifetime/revision/source authority for every supported
  holding. Ships and guilds are captured diagnostically; missing durable money
  revisions remain specific exceptions, including zero/unknown holdings.
- Supply complete native UID payload/topology and creation/retirement evidence,
  original root/child/receipt/source bindings and publication ACK/recovery proof.
- Supply exact fresh-character initialization, grant, lifecycle, reset and
  correction native receipts. Typed correction application remains producer-owned;
  no direct SQL repair is provided.
- Supply current writer evidence and central registration against the actual
  integrated revision. Synthetic manifests qualify disposable components only.
- Select one combined revision/schema before broad qualification. Every supported
  route needs executable and applicable actual-player proof, zero unexplained
  discrepancies, and measured predeclared budgets before release certification.

Completion for this assignment requires implemented/tested independent modules
and qualification tooling, plus reproducible handoffs for producer dependencies.
Flatfile parity, existing-data upgrades, archival/retention policy decisions and
full supported-route journeys remain required when deferred, not removed from
the feature contract. Do not claim final qualification from pure fixtures or
inactive gameplay, enable destructive policies, or perform production operations.

## Slice evidence

Update here after each coherent slice with commit, files, exact commands/results,
backend/version, original failures, remaining dependencies and integration action.

### Database-wide orphan evidence

Files: exporter, reconciler, their two existing unit suites, and
`tests/async/run_economic_sql_audit_snapshot_mysql.py`. R7 / #490 section 4.
The cut now captures orphan account effects, postings, children and item
references across the database with per-table counts. Missing roots cannot
reliably identify lineage/epoch. The independent auditor recomputes coverage,
requires it for partial SQL exports and emits four specific discrepancy codes.
No authority mutation, migration, adjustment, activation or source policy changed.

Validation on the slice:

- `python tests/async/test_reconcile_economy_accounting.py`: 63 PASS.
- `python tests/async/test_economic_sql_audit_origins.py`: 11 PASS.
- `python tests/async/run_economic_sql_audit_snapshot_mysql.py`: PASS on native
  MySQL 8.0.46 and MariaDB 10.11.14, using fresh disposable namespaces and a
  SELECT-only audit user. Existing native checks remain intact. Added tests seed
  all four orphan families, compare exact pre/post rows, commit a lower-ID row
  after the first read view and detect it in the next cut, and interrupt/retry
  a capture. These are native database fixtures, not actual player journeys.
- `python scripts/validate_economy_accounting.py`: PASS, `release_ready=False`.
- `python scripts/generate_economy_writer_coverage.py --check`: see next slice
  evidence when its pending check completes.
- `git diff --check`: PASS.

Native fixture invocation sets `ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1`,
`DB_HOST=127.0.0.1`, `DB_PORT=3306`, `DB_USER=root`, and a disposable-only
`DB_PASSWORD`, and runs the command inside the database container's network
namespace. No project `.env`, player data or production database was used.
Initial fixture setup failures are retained in the chat tool evidence: MySQL
native-AIO resource exhaustion and audit-user denial through the Docker bridge.
The successful MySQL fixture uses `--innodb-use-native-aio=0`; this is functional
qualification, not a performance claim.

Bounds are unchanged: 100,000 rows per collection, 32 MiB encoded snapshot,
100 output details, with an additional 100,000 total orphan-row cap. SQL reads
retain the existing 30-second socket timeout. Oversized and interrupted cuts
refuse; no resumable-history or SQL query-plan/resource budget pass is claimed.
The exporter still says `complete=false`. Producer/interface and broader release
dependencies above remain open. Integration action: carry this tested commit to
`experimental-accounting`, then continue ID views and restore qualification.
