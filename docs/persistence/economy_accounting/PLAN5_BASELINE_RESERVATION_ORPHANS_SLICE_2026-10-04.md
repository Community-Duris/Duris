# Plan 5: inventory baseline reservations without a matching witness

This slice starts at `e694798f63caeda6cf778d27f1376cb7dba45737` on
`codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. The result
commit, committed input hashes and preserved prior evidence are bound by
`tmp/plan5/baseline-reservation-orphans-evidence.json`. Publish this lane's
branch for primary-owner integration. Do not push it to experimental-accounting.

The refreshed integration remote is
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native canonical base is
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`, native tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`. All 1,232 native source inputs
and 236 migration inputs remain unchanged. Every fresh database uses canonical
migrations through `0056_spell_ward_durability`. These tests do not qualify the
primary owner's unpublished combined candidate.

## Established defect and independent fix

The database-wide orphan collector covered effects, postings, children and item
references. Baseline reservations were absent. Existing per-book projection
checks cannot identify a reservation whose claimed book and operation are both
unknown. The global projection bound includes those rows but provides no
inventory or specific finding.

The private native fixture attempted to insert such a reservation. Both
canonical engines rejected it with composite-FK error 1452. A separately marked
damaged-import fixture temporarily disabled the private owner's foreign keys,
inserted the row and restored foreign keys before invoking the SELECT-only
reader. Both engines missed the reservation with unchanged initial findings.
The RED ran one guarded test with two engine failures, zero skips, in 169.897
seconds. Frozen source and complete artifacts are preserved.

The independent collector now includes `economic_baseline_reservation`. Its
database-wide anti-join requires an exact witness match on `operation_id`,
`lineage` and `epoch`. It has no selected-book predicate. Rows without that
composite match produce `orphan_baseline_reservation`. Claimed lineage and epoch
are explicitly untrusted and are never assigned to the selected book.

The existing orphan collection, bound, coverage map, exception report and
operation view are extended. There is no separate correction path. The
reservation's full unsigned identity ID is preserved separately from its kind;
several natural keys under one operation remain separate. Per-family and
aggregate row limits remain enforced. The composite witness join also catches
known operation IDs with a foreign claimed lineage or epoch.

Selected-book projection disagreement still refuses capture before a snapshot
is available. Retained-book disagreement still emits `baseline_source_claim`;
foreign reservation scope now additionally emits its specific orphan finding.
No finding is suppressed. Missing/invalid claimed IDs, identity kinds or unsigned
identity IDs refuse malformed snapshots. An older four-family coverage map
refuses rather than inventing zero reservation anomalies.

## Narrow shared interface handoff

No canonical schema, native accounting contract, mutation owner, shared
coordinator, producer, registry/matrix or activation file changes in this lane.
The primary owner must adopt this owned audit-output extension coherently with
its exporter, snapshots and combined report. The exact audit-format addition is:

- `orphan_evidence_coverage.table_counts.baseline_reservations`: mandatory
  nonnegative integer, exactly equal to this family's exported row count. The
  coverage `scope` remains `database`. Existing four counts remain mandatory.
- `orphan_evidence[].table`: additional value `baseline_reservations`.
  `operation_id` remains a nonzero lowercase 32-digit hex ID. For this family,
  `row_index` is identity kind 1 (account lifetime) or 2 (item UID).
- Reservation rows add `identity_id`, an actual integer in `[1, 2**64 - 1]`, and
  `claimed_lineage` / `claimed_epoch`, nonzero lowercase 32-digit hex IDs. These
  are claims from the reservation row, not authoritative book assignments.
- Exception code `orphan_baseline_reservation` includes `operation_id`, `table`,
  `identity_kind`, `identity_id`, `claimed_lineage` and `claimed_epoch`.
  The operation view retains `row_index` and adds the same identity fields.
  Its existing `record_counts.orphan_evidence`, count, truncation, coverage and
  nonzero CLI exit status remain global and limit-invariant.

Consumers are `economic_sql_audit_snapshot.py`,
`reconcile_economy_accounting.py`, operator JSON readers, stored audit snapshots
and the primary owner's release reporter. No change to `schema_version` or the
ten required accounting evidence collections is proposed. An existing SQL
snapshot with only four orphan coverage counts must be recaptured with the
updated exporter; consumers must not default the fifth count to zero.

Acceptance tests are
`test_orphan_baseline_reservation_identity_coverage_and_operator_lookup`,
`test_rootless_reservation_export_preserves_untrusted_scope_and_uint64`, the
updated bounded orphan-export test, and the native reservation cuts/operator
CLI probe on both engines. Shared test registration remains with the primary
owner: register `DURIS_RUN_NATIVE_BASELINE_AUDIT=1` for both engines and zero
skips, plus the explicit native-origin pair. Fields are `path`, `arguments`,
`environment`, `required_cases`, `provider`, `engines` and timeout; consumers
are the integration runner and combined release report.

## Exact validation and evidence

The immutable tool image is `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
It contains Ubuntu 24.04, Python 3.12.3, GCC 13.3 and PyMySQL 1.0.2-2ubuntu1.1.
Canonical engines are MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL
`8.0.46-0ubuntu0.22.04.4`. The checkout is mounted read-only at `/workspace`
with only `bin/` separately writable. Fresh disposable daemons use private Unix
sockets, no TCP, and a clean test environment without the checkout `.env`.

The native fixture still has 13 C++20 inputs with strict warnings, ASan and
UBSan. SQL binary SHA-256 remains
`7985d95effd41456d445a5924e466a5ae5ad25fa9506d141d2ff1dd1168c4ee2`;
client-free remains
`efaee7f33a1ebc38fe9b6afe9baaa32b66b8b3db1746679dc58c05cf39a9b027`.
The client-free owner retains initialize/apply/reconcile `ENOTSUP` refusals.
Native publication, exact replay and reconciliation cover two nonempty books
with `active_epoch` NULL.

The passing matrix has 145 cuts per engine: 65 capture refusals and 80 exact
diagnostic cuts, plus seven canonical constraint refusals. Seventeen cuts are
explicitly marked damaged imports. The six new damaged-import cuts per engine
cover unknown book/operation,
known-lineage unknown-epoch/operation, unknown-lineage known-epoch/operation,
known book with unknown operation, a receipt without a witness, and four
distinct natural keys under one missing operation. IDs include 1, `2**63` and
`2**64 - 1`; both identity kinds and differing claimed books remain visible.
Existing retained foreign-lineage/epoch reservation cuts now require the extra
orphan diagnostic. Canonical FK/CHECK/UNIQUE constraints remain unchanged;
foreign keys are enabled for every audit and native replay. Private fixture
repair happens only after proving the reader did not alter authority.

Each audit and operator CLI invocation preserves 18 captured authority/evidence
tables. SELECT-only credentials reject UPDATE with 1142. Exact CLI lookups at
limits 0, 1 and 100 preserve all counts, global exception status, a nonzero exit,
safe natural identities and unchanged snapshot bytes. The independent
native-origin pair preserves 15 tables, flatfile bytes, permissions and link
counts; every successful or refused capture rolls back and closes its cursor.

The intact native fixture still reports one evidence-loss, two missing-native-
holding and two missing-native-item findings because no complete world/native
capture is supplied. This remains partial evidence with accounting inactive.

Results and elapsed times are recorded in the protected manifest and logs:

| Exact command | Evidence |
| --- | --- |
| `DURIS_RUN_NATIVE_BASELINE_AUDIT=1 python3 -u tests/async/test_native_sql_baseline_audit.py -v` at `/workspace` | Pass: one guarded test, both engines, zero skips, 324.470 seconds. `tmp/plan5/reservation-orphans-green.log`, `bin/tests/plan5-baseline-reservation-orphans/{mariadb,mysql}.log`; 145 cuts/seven constraints per engine, native replay and sibling exporter/operator matrix. |
| `python3 -m unittest test_reconcile_economy_accounting test_economic_sql_audit_origins -v` at `/workspace/tests/async` | 124 collected, 119 passes, five explicit skips, 12.327 seconds. `tmp/plan5/reservation-orphans-components.log`. |
| `DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1 python3 -m unittest test_economic_sql_audit_origins.NativeSQLOriginTests -v` at `/workspace/tests/async` | Two passes, zero skips, 244.478 seconds. `tmp/plan5/reservation-orphans-origins.log`; four authentic native EAB1 batches across two epochs, three captures/three refusals/six rollbacks per engine. |
| `python tests/async/test_economic_sql_audit_origins.py -v` on Windows | 25 collected, 23 passes, two explicit native-origin skips, 0.039 seconds. `tmp/plan5/reservation-orphans-host.log`. The explicit native pair executes those skipped cases. |
| Seven changed Python files: `python -m py_compile`; unstaged/staged `git diff --check` | `tmp/plan5/reservation-orphans-final-checks.log`. No native input changed; no server rebuild or changed-line C++ formatting is required for this slice. |

The first Windows unit run failed because a pre-existing assertion forbade any
`lineage=` text, including the required composite join. Its corrected assertion
forbids a selected-lineage parameter predicate. That failed log is preserved as
`tmp/plan5/reservation-orphans-host-attempt.log`. The audit did not change to
satisfy the assertion.

Exact syntax command:

```sh
python -m py_compile scripts/economic_sql_audit_snapshot.py scripts/reconcile_economy_accounting.py tests/async/test_economic_sql_audit_origins.py tests/async/test_reconcile_economy_accounting.py tests/async/run_economic_sql_audit_snapshot_mysql.py tests/async/test_native_sql_baseline_audit.py tests/async/run_native_sql_baseline_audit.py
```

Generated binaries, logs, JSON operator views and the protected manifest remain
uncommitted under `bin/` and `tmp/plan5/`. Previous slice evidence is preserved
byte-for-byte. The manifest binds all ten frozen executable inputs, unchanged
native/migration maps, exact result commit, toolchain and commands.

## Ownership and remaining gates

Eight owned files are the exporter, reconciler, their two unit test files,
`run_economic_sql_audit_snapshot_mysql.py`, `run_native_sql_baseline_audit.py`,
`test_native_sql_baseline_audit.py` and this report. The independent canonical
decoder and origin verifier remain unchanged. Primary-owner shared files are
outside the diff.

Complete native capture, namespace authority, real writer/player journeys,
fault/restart/lost-reply matrices, populated upgrades, full restore/clone
qualification, retention/erasure/export policy, live sweeps and measured mixed
workload budgets remain open. The two near-limit budget cases and native-stake
SQL case are skipped here and not requalified. Previous backup/restore and
retention results remain pinned to their own source. Inventory, synthetic
fixtures and passing components do not establish release completion.

`AI_CONTEXT.md` remains absent from both checkouts. No curator capability or
notebook/workflow reference is available, and the earlier request is unanswered.
Bounded accessible Pages searches returned no target and do not prove absence.
This report and manifest supply a curator handoff; no notebook update is
claimed. Wallet-root item exclusions, declined inactive spell behavior and
active blackjack refusal are preserved. No activation, production access,
merge, deployment or audit auto-correction occurred. The combined candidate
and release remain unqualified.
