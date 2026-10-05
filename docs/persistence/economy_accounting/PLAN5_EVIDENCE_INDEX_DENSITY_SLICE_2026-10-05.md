# Plan 5: contiguous evidence indexes

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Frozen tested base is `a9f51ef4f59bee41a67bdfc8ebe9c161eff7d2e7`, the ordinary
merge of refreshed primary `523c1c7ee3bec57844e6b312f7e9cba495198f84`.
The primary has imported the preceding index-type repair. Native tree is
`d6cc4e5e6f16ebaee18af9902a1ac57e02e44d37`; migration tree is
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
Result commit and verified remote SHA are supplied in delivery.

Delivery refresh finds primary `c2b98c3e4935279ecf051c1be4e14dcdc82db759`,
native tree `a5d8b2580bd794a0efa9a97032de4d1d4ff38ddd`, migrations unchanged.
Its compiler include repair, coverage/provenance repair and keeper insertion
ordering arrived during this frozen test run. This slice does not qualify that
newer native candidate. Fresh maintained builds and current-binary persistence
checks are the next independent qualification work.

## Established defect and owned repair

The reader validated index types, global bounds and row counts but did not
check that retained positions form the native sequence for each root.
Eight synthetic gap/offset cases across effects, postings, children and item
references returned zero findings despite coherent balances, revisions, header
counts and dependent links. There were six distinct serialized payloads:
the initial single-child and single-item gap/offset payloads coincided.
Four focused regressions produced 72 assertion failures before the repair.

On both MariaDB and MySQL, fixture-owner SQL then demonstrated eight actual
schema-valid corruptions: effect account indexes and their dependent postings
were renumbered together, or posting line/event indexes were renumbered
together. Constraints stayed enabled, and values, revisions and counts stayed
unchanged. Eight real SELECT-only snapshot captures admitted these cuts;
24 API and 48 CLI checks confirmed the false clean results before the fix.
This reproduction is not limited to changing an exported JSON document.

The reader now requires effects, postings and current-root item references to
cover exactly `0..count-1`, and children to cover exactly `1..count`. It checks
the sequence only after cardinalities match and after the existing type/bounds
validation. Work is bounded by actual retained rows and the native count caps.
Export order may vary. Ownership-event and legacy-reference indexes retain
their original positions in filtered ownership history and may remain sparse.
Existing duplicate and cardinality findings keep their meanings.

The new diagnostic is `evidence_index_mismatch`, using the existing public
root-ID/table envelope. It is an audit finding (CLI exit 1). Global failure
status survives result limits 0, 1 and 100 and unrelated operation filters;
private aliases and source values remain absent from output. No mutation code
is imported and no audit finding is corrected automatically.

The independent interpretation follows native vector contracts in
`src/economy/economic_accounting_types.c` (posting positions, child parent
positions and item positions), and `insert_plan_rows` in
`src/persistence/economic_sql_shop_trade_transaction.c` (effect and posting
loop indexes). The generated native probe also checks sparse posting,
out-of-range account and absent-child references in both SQL and flat modes:
six assertions pass without changing the encoded native result.

Owned files are `scripts/reconcile_economy_accounting.py`,
`tests/async/test_reconcile_economy_accounting.py`,
`docs/persistence/economy_accounting/AUDIT_OPERATIONS.md` and this report.
The reader change adds eight lines and removes two; the operations document
adds seven lines. No shared interface, field, schema or activation change is
requested. Final tested SHA-256 values are:

- Reader: `5ab01420abdb9e7e6cd766cec7ae58373bf360cbae5bf08705e1e76b849b4d8d`.
- Tests: `d58b28429fa7c4123324c1871e04a133c0957ad0b7fb4cb907d5ff4b62f2cc95`.
- Operations document: `78107ddbe18e53b181ba1e21abce2ddad1ce70f49fc133494709ac34f579867a`.

## Exact source, commands and results

All selected checks ran in pinned Docker image
`duris-plan5-origin-sql-tools:local`, immutable image ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Environment: Ubuntu 24.04.4, GCC 13.3, Python 3.12.3 and OpenSSL 3.0.13;
MariaDB 10.11.14 and MySQL 8.0.46. Containers used `--network none`, a
read-only source mount, writable private `bin` artifacts, separate caches,
private database datadirs and Unix sockets. Project `.env` and production
credentials/data were not consumed.

The RED unit command names the four new methods under
`test_reconcile_economy_accounting.ReconciliationTests`:
`test_evidence_index_sequences_detect_balanced_renumbering`,
`test_dense_sequences_ignore_export_order_and_preserve_legacy_positions`,
`test_sequence_check_retains_existing_cardinality_and_duplicate_findings`, and
`test_index_sequence_cli_failure_is_global_bounded_private_and_read_only`.
Its exact invocation is retained in `unit-red-command.json`.

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
  python3 -u -m unittest -v test_reconcile_economy_accounting.ReconciliationTests

DURIS_RUN_STAKE_SQL_INTEGRATION=1 \
DURIS_PLAN5_PRICE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-price-view-sql/density-green \
  python3 -u -m unittest -v tests.async.test_reconcile_economy_accounting.NativeStakeSQLTests

DURIS_RUN_AUDIT_BUDGET=1 PYTHONPATH=/workspace/tests/async \
  python3 -u -m unittest -v test_reconcile_economy_accounting.AuditBudgetTests

python3 -m py_compile scripts/reconcile_economy_accounting.py tests/async/test_reconcile_economy_accounting.py
python3 scripts/validate_economy_accounting.py
python3 scripts/generate_economy_writer_coverage.py --check
git diff --check
```

Native RED uses the same class with `DURIS_PLAN5_INDEX_DENSITY_RED=1`,
`DURIS_PLAN5_INDEX_DENSITY_ONLY=1` and the `density-red` artifact directory.
The GREEN invocation has neither density RED/ONLY flag. Command JSON and
logs retain exact invocations and outer elapsed times.

| Check | Result | Unittest seconds | Outer seconds |
| --- | --- | ---: | ---: |
| Four unit regressions, RED | Exit 1; 72 failures, zero errors | 7.704 | See retained command/log |
| Native density reproduction, RED | Exit 0; eight false-clean cuts established | 262.140 | 264.9193328 |
| Full reconciliation class, GREEN | Exit 0; 114 tests, zero skips | 51.932 | 53.1552833 |
| Full native/SQL class, GREEN | Exit 0; one test, zero skips | 262.474 | 264.9358027 |
| Audit budget class, GREEN | Exit 0; two tests, zero skips | 14.219 | 15.9980559 |

These are 117 selected passing tests, not the entire repository suite.
Compilation checks, validator, matrix check and diff check all returned zero.
The validator covers 14 fixtures, 887 routes and 2,843 sites but reports
`release_ready=false`. The writer matrix retains `coverage_complete=false`
and release `BLOCKED`; inventory checks do not establish release completion.

The native probe compiles these eight source files: economic accounting plan,
types and intent; critical command; item transfer command; craft pouch
mutation; chaos pouch ledger; and player snapshot codec. Exact paths and
all 1,498 frozen source hashes (1,262 native plus 236 migration inputs) are
retained. Both builds use C++20, `-Wall -Wextra -Wpedantic -Werror`, `-O1 -g`,
ASan/UBSan, frame pointers, `-fno-pie -no-pie`, `-Isrc`, and `-lcrypto`; the
flat build additionally uses `__NO_MYSQL__`.

RED SQL/flat probe SHA-256 is
`54774698e7f82e136969c5ff774e9eae118acb68560d84efcfd98f6f4208ff22`.
GREEN SQL/flat probe SHA-256 is
`788b7d1c0456b91e3a66cf4648c55aad3740d60ca6dfcc96e5ed0613cc50755b`.
All four encoded outputs retain SHA-256
`1ed10a94436e14271b6bc07d8f17ebf364b10dcd770e1da1c6738df1d02c7281`.

GREEN detects all eight SQL density cuts: 24 API and 48 CLI checks pass.
The normal matrix still passes 90 read-only captures/rollbacks and 72 fault
cuts; density adds eight captures and eight cuts, totaling 98 and 80.
The seven audited source tables stay unchanged during every capture; rollback
and connection closure are checked. Fixture-owner writes deliberately seed
the corruption and finally restore the exact original rows. That fixture
restoration is separate from the SELECT-only operator/auditor. Activation
remains NULL. Two UPDATE-denial checks also pass.

Previous count/result/index validations remain green, as do 104 grammar cases,
1,107 policy cases, six original-link cases and the price projection checks
(four roots, two epochs, 24 price CLI checks and 16 projection comparisons).
Child/item sequence coverage is synthetic unit evidence; the actual SQL
density reproduction uses the native money fixture's effects and postings.
Modeled holdings/origins and selected native components do not qualify new
NPC/shop producers or full gameplay journeys.

The two budget tests retain 12 CLI samples. Mapping audit inputs have 15,245
roots and 33,551,932 bytes; maximum runtime is 0.9948186348192394 seconds and
peak memory 139,206,656 bytes. Price inputs have 100,000 roots and 33,552,384
bytes; maxima are 0.6324736999813467 seconds and 138,002,432 bytes. All meet
the existing 30-second/256-MiB component limits, preserve input bytes and
exercise oversize refusal. Samples explicitly retain
`synthetic_component=true` and `release_host_qualified=false`.

## Evidence, curator handoff and remaining gates

Sealed local artifact roots are `bin/tests/plan5-evidence-index-density/` and
`bin/tests/plan5-price-view-sql/density-red/` plus `density-green/`.
The manifest `tmp/plan5/evidence-index-density-evidence.json` has SHA-256
`f418db7c854e24de6a94dab6d55d2562ab57e0f774ab80eba7bb7602ba7efc3e`.
It seals 589 fresh artifacts and preservation of all 39,749 prior artifacts,
all frozen native/migration inputs and unowned reader/test dependencies.
Logs, commands, binaries, captured cuts, source copies, budget metrics and
delivery-time source refresh are retained. Artifacts are ignored local
evidence; the report and owned fix are published on the Plan 5 branch.

The initial recorder selected the saved baseline test blob correctly but
hashed the edited test file for its owned preimage entry. The correction
hashes the selected data and verifies baseline test SHA-256
`74a5aaa493d5451275e89616d8d0ff025916bc2af3272ee73566c6771339c325`.
Original recorder/state and `preimage-record-correction.json` are retained.
Unowned input/preservation checks and actual test sources/results were
unaffected; the final sealed manifest contains the corrected preimage.

The primary repaired the prior compiler include finding in `3a4841d8b` and
writer/provenance findings in `ec24501e5`. Those historical failures remain
evidence for their old source pins, not current blockers. The newer combined
source still needs fresh maintained SQL/flat builds and current-binary managed
restore checks. This slice did not repeat a known failing whole-server build
on its unchanged frozen source, and it does not substitute selected probe
builds for maintained server qualification.

Other remaining gates include primary-owned original CCM1 admission time
(`economic_baseline_witness.command_accepted_at_usec`, proposed 0058), a
coherent 0057/0058/0059 migration/producer candidate, actual SQL/flat producer
parity and gameplay journeys, native capture/mapping authenticity, combined
source publication and guarded acknowledgement, release-host operation
latency/storage/retention/replica evidence, and complete R1-R8 qualification.
The reader does not invent the missing command-admission timestamp.

The primary maintains the shared notebook locally through its curator
workflow. This report is the integration/curator handoff; notebook location
does not block independent work and no remote notebook update is claimed.
Inactive behavior, wallet-root item exclusions and the declined inactive
spell-path change are preserved. There is no activation, experimental-branch
push, PR merge, deployment, production-data change or audit autocorrection.
