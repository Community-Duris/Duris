# Plan 5: evidence index format validation

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Frozen tested base is `d0d570113491af896ead216fb2b5f958b717a499`.
Refreshed primary is `14dff224b63b7b3fcd7846734ae40b9de601293b`; its incoming
native fixes and canonical 0056 are already present in this base. The later
primary advance adds documentation and the already-present count repair;
no additional merge was needed to select the source for this slice.
Native tree remains `e018587932abef86ef3bcab9d61a6f3be3afe933`, migration tree
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
The base is published on the Plan 5 branch. Result commit and verified remote
SHA are supplied in delivery.

A delivery-time refresh finds primary
`0723e10a5a81615465e0727d18e9f837d9b93873`, which imports the prior result-code
repair and adds shop/NPC native observation components. Its native tree is
`6313ff3d7a340788606e0ba7bc5015cdc72dc773`; migrations remain unchanged.
That incoming source is not mixed into this live test run. This slice's evidence
qualifies the frozen base above, not the newer combined native candidate.

## Established defect and complete owned repair

The audit indexed evidence rows and compared their numeric indexes using
Python equality without first validating their representation and native
bounds. Twenty-four serialized synthetic cuts across effects, postings,
item references and ownership events returned zero findings when integer
indexes were replaced by equivalent floats or booleans. Four list-valued
key indexes caused uncaught `TypeError` exceptions. Input objects and
serialized files stayed unchanged during these reproductions.

The reader now validates all ten numeric index fields on every row in five
evidence tables before indexing, including duplicate rows that would otherwise
be discarded. It requires `type(value) is int` and the following inclusive
ranges, interpreted independently from the existing contracts:

| Table | Field | Minimum | Maximum |
| --- | --- | ---: | ---: |
| effects | account_index | 0 | 3071 |
| postings | line_index | 0 | 6143 |
| postings | account_index | 0 | 3071 |
| postings | child_index | 0 | 64 |
| children | child_index | 1 | 64 |
| children | parent_index | 0 | 63 |
| item_references | event_index | 0 | 2999 |
| item_references | child_index | 0 | 64 |
| item_references | legacy_event_index | 0 | 65535 |
| ownership_events | event_index | 0 | 65535 |

These bounds follow `migrations/economy_accounting.sql` (effect/posting/child/
item constraints), `migrations/item_ownership_ledger.sql` (unsigned 16-bit
ownership event index), and the existing native DTO/normalization contracts.
The normalized posting omits `event_index`, which the schema requires to equal
`line_index`; the normalized item reference omits `line_index`, which the
schema requires to equal `event_index`. No new field is required.

Malformed, missing or out-of-range values raise
`SnapshotError("invalid <table> <field>")` before dictionary lookup. The CLI
returns 2 with empty stdout and only the bounded public table/field label on
stderr. Private values and tracebacks are absent. Valid integer disagreements
retain semantic audit findings: for example, a child whose parent is itself
still produces `invalid_child_link`. The repair adds 15 reader lines and
does not coerce evidence or import mutation logic.

Owned files are `scripts/reconcile_economy_accounting.py`,
`tests/async/test_reconcile_economy_accounting.py` and this report. No shared
interface or schema change is requested. Final tested SHA-256 values are:

- Reader: `b64686ac080878871b85cccb5555ae292753cdc1058855c4fae328bea527a51b`.
- Tests: `74a5aaa493d5451275e89616d8d0ff025916bc2af3272ee73566c6771339c325`.

## Exact tests and retained reproductions

All invocations use Docker image `duris-plan5-origin-sql-tools:local`, immutable
ID `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with Ubuntu 24.04.4, GCC 13.3, Python 3.12.3, OpenSSL 3.0.13,
MariaDB 10.11.14 and MySQL 8.0.46. Network is disabled, `/workspace` is read-only
and `/workspace/bin` is writable. SQL evidence uses fresh private datadirs and
TCP-disabled sockets. No production environment or data is read.

The pre-fix focused command sets `PYTHONPATH=/workspace/tests/async` and
`PYTHONDONTWRITEBYTECODE=1`:

```sh
python3 -m unittest -v \
  test_reconcile_economy_accounting.ReconciliationTests.test_evidence_index_types_missing_fields_and_native_bounds_refuse \
  test_reconcile_economy_accounting.ReconciliationTests.test_evidence_index_boundaries_keep_semantic_audit_and_input \
  test_reconcile_economy_accounting.ReconciliationTests.test_duplicate_evidence_cannot_hide_malformed_key_index \
  test_reconcile_economy_accounting.ReconciliationTests.test_evidence_index_cli_refusal_is_bounded_private_and_global
```

RED exits 1: four tests, 473 failures and 42 errors, 16.690 unittest seconds.
The separate 24-cut zero-finding reproduction is `unit-red-proof.json`, with
exact input hashes. `parser-red-proof.json` records the four uncaught list-key
errors. The failed assertions, source snapshots and all altered inputs are
retained. `py_compile` succeeded before the native reproduction was launched.

Both native invocations run:

```sh
python3 -u -m unittest -v tests.async.test_reconcile_economy_accounting.NativeStakeSQLTests
```

They set `DURIS_RUN_STAKE_SQL_INTEGRATION=1`. RED additionally sets
`DURIS_PLAN5_INDEX_TYPES_RED=1`, `DURIS_PLAN5_INDEX_TYPES_ONLY=1` and
`DURIS_PLAN5_PRICE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-price-view-sql/index-types-red`.
It exits 0 after 364.0201724 outer seconds (358.111 unittest seconds), proving
32 falsely accepted exports from actual canonical0056 SQL evidence through
96 API and 192 CLI checks on both engines. These native cuts cover the effect
account index and the three posting indexes on both native money rows, using
equivalent float/boolean values at limits 0, 1 and 100. Item and child index
representation/boundary proof is provided by the unit tests; no native item or
child producer journey is attributed to the money fixture.

GREEN uses artifact directory
`/workspace/bin/tests/plan5-price-view-sql/index-types-green` without either
INDEX RED/ONLY flag. It exits 0 after 361.6844784 outer seconds
(357.400 unittest seconds), with zero skips. All 32 malformed index exports
now refuse through 96 API and 192 CLI checks. The prior count and result-code
proofs stay green: 12 count cuts through 36 API/72 CLI checks and four
result-code cuts through 12 API/24 CLI checks.

The complete existing matrix passes: 90 read-only captures and rollbacks,
72 fault captures, two UPDATE permission denials, 104 native source grammar
cases, 1,107 policy decisions, six original-link cases, four price roots across
two epochs, 24 price CLI cases and 16 shared price-projection comparisons.
All seven authority tables and all inputs remain unchanged after every
capture and altered export; `active_epoch` remains NULL on both engines.

The native probes are freshly built from these eight maintained inputs:
`src/economy/economic_accounting_plan.c`,
`src/economy/economic_accounting_types.c`,
`src/economy/economic_accounting_intent.c`,
`src/persistence/critical_command.c`, `src/item/item_transfer_command.c`,
`src/item/craft_pouch_mutation.c`, `src/combat/chaos_pouch_ledger.c` and
`src/player/player_snapshot_codec.c`. Both SQL and `__NO_MYSQL__` modes use
C++20, `-Wall -Wextra -Wpedantic -Werror -O1 -g`, ASan/UBSan, frame pointers,
`-fno-pie -no-pie`, `-Isrc` and `-lcrypto`. Leak/error stopping is enabled.
Generated C++ and executables are retained. Both modes agree within each run:

- RED executable: `73a2c5a3559ffd2a694cb57b5406d0b3c4daf45010aeb70856821b6f45d68e87`.
- GREEN executable: `9fb11ad30d3a8ea5ed3e1bbe040499b0cbf8d524d21a02e170d10ccc88a78fa1`.
- Encoded native output, all four executions:
  `1ed10a94436e14271b6bc07d8f17ebf364b10dcd770e1da1c6738df1d02c7281`.

The complete pure unit command passes with zero skips:
`python3 -u -m unittest -v test_reconcile_economy_accounting.ReconciliationTests`:
110 tests, 66.176 unittest seconds, 67.9521746 outer seconds, exit 0. It covers
all ten fields, missing/null values, floats/booleans, containers/private strings,
negative/overflow and nonfinite values, native range boundaries, duplicate
rows, preserved semantic findings, global CLI refusal despite operation filters,
bounded output at limits 0/1/100 and immutable input objects/files.

`python3 -m py_compile scripts/reconcile_economy_accounting.py
tests/async/test_reconcile_economy_accounting.py` and `git diff --check` pass.
`python3 scripts/validate_economy_accounting.py` passes: 14 fixtures,
886 writer routes, 2,843 candidate sites, `release_ready=False`.
`python3 scripts/generate_economy_writer_coverage.py --check` passes:
`coverage_complete=False`, release `BLOCKED`.

## Evidence and remaining qualification

Logs, command JSON, source snapshots, reproduction/recorder scripts, source
trees and runtime metadata are in `bin/tests/plan5-evidence-index-types/`.
Native evidence and altered exports are in
`bin/tests/plan5-price-view-sql/index-types-red/` and `index-types-green/`,
including `index-type-results.json`, `count-type-results.json` and
`result-type-results.json`. Artifacts stay ignored; generated data, executables,
logs, datadirs, environment files and archives are not committed.

Sealed manifest: `tmp/plan5/evidence-index-types-evidence.json`, SHA-256
`a4f09a354d7923bf2c86279fd0ca78b6f3600d884db8f63d1aad176afc86df51`.
It records 577 fresh artifacts and verifies all 1,490 native/migration inputs
(1,254 native plus 236 migration files), unowned Python/test inputs and all
36,254 prior artifacts unchanged. The reproduction driver is
`python3 tmp/plan5/probe-evidence-index-types.py`; the evidence recorder runs
`python3 tmp/plan5/record-evidence-index-types-evidence.py start`, `preserve`
and `finish` as separate invocations. The sealed artifact directories are not
changed after the manifest is written.

The native harness combines real encoded plans and canonical0056 SQL evidence
with modeled holdings/origins. This proves the reader's structural validation
and read-only discipline. Full runtime producer publication, complete native
capture, gameplay and activation remain unqualified by this fixture. This
slice validates the stated index fields and ranges; it does not establish
complete identity validation, dense ordinal sequences or every relationship.
No unchanged maintained server build or cold-restore run is repeated for this
Python-only repair; prior build/restore reports remain limited to their exact
tested inputs.

There is no blocker to this owned repair. Full baseline CCM1 authentication
still needs the primary-owned original admission-time field
`economic_baseline_witness.command_accepted_at_usec`; it must be captured from
the original admission, remain immutable and be compared on replay. Historical
NULL values must not be replaced with invented SQL timestamps. No independent
DDL or producer change is made here. Native capture/mapping coverage,
Plans 2–4 producer qualification, combined-candidate acceptance, applicable
retention/replica gates and R1–R8 requirements remain open. Accounting remains
inactive; wallet-root item exclusions and the declined inactive spell path
are preserved. No experimental-accounting push, activation, merge, deployment,
production write or automatic correction of audit findings is performed.

This report is a handoff for the primary's local notebook curator. The shared
notebook is maintained on that system and does not block this work; no remote
notebook update is claimed.
