# Plan 5 exact restore projection representations

The independent restore verifier accepted float and boolean SQL JSON values
equal to original native integers. Rejected roots also accepted noninteger
zero counts, including null values. The verifier now requires exact native
types and values for root controls and every normalized projection family.
It reports a controlled refusal without modifying authority or original bytes.

## Source and ownership

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `64d5fca99a375ee56118727f721126e43fa44e94`, preserving both the preceding
  Plan 5 baseline-origin fix and primary `9665df324ab9751408d2cb9b735797f2d13b3ef8`.
- Native tree: `139556ad49b39d006588572015dc5006e8186a59`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical 0056.
- Owned files: `scripts/economic_restore_evidence.py`,
  `tests/async/test_economic_sql_canonical_audit.py`, and this report.
- Tested verifier SHA256:
  `376e5c767e5ce41825170b945612292a5cf267800b3f0e61f657d39d51f5659e`.
- Tested test-module SHA256:
  `0a66167e3ff0b2c59040bb3b8a45708f2adac46cec917b19ff6c351fea6cb9b9`.
- Evidence manifest: `tmp/plan5/restore-type-evidence.json`; result commit and
  remote verification: `tmp/plan5/restore-type-delivery.json`.

Full `src/`, `migrations/`, `scripts/` and `tests/` hash maps are captured before
each run and checked afterward. Copied preimages, final owned sources, exact
commands, environment gates, compiler metadata, binaries, database cuts and
logs are retained. This report is committed with the fix; the delivery receipt
binds that result commit without rewriting the report after qualification.

No shared coordinator, producer, accounting contract, migration, registry,
matrix or activation file changes. All earlier Plan 5 slices and original six
branch heads remain preserved on the requested remote branch. Follow-up work
continues there. The primary maintains the shared notebook through its curator;
notebook locality is not a blocker.

## Established defect and fix

The unmodified verifier preimage SHA256 is
`1a96b178a4ae7baf6efe10e4f8ab691823a4649e99b95c6483cb3ddc863a4bf3`.
An initial model probe covers 131 float/boolean aliases and observes 128 false
clean results; the existing capsule-length validation already refuses three.
These are explicit reference models, not native producer or release evidence.
The six new regression methods then run against that preimage: 127 failing
subtests, zero errors and zero skips. Both red runs and original sources remain
in evidence.

The fix reuses the reconciler's existing `same_projection` helper. Exact
recursive comparisons cover original root metadata, committed plan counts,
account effects, coin postings, children, item references, custody ledger
projections and baseline reservations. Root outcome and result code require
exact integers; result code must fit its existing unsigned 32-bit SQL column.
Rejected roots require six exact integer zero counts. Existing positive controls
include ordinary and baseline roots, rejected roots and result codes 1 and
UINT32_MAX. Refusal names and capsule decoding remain unchanged.

The helper has no dependency on mutation logic. Audit inputs are compared with
independently decoded original capsules, never coerced, resealed or corrected.
The existing standalone canonical audit and full SQL restore qualifier both
consume this verifier.

## Commands and evidence

All new generated artifacts use the physical bin root
`D:/CodexEvidence/accounting-plan5/bin/restore-types-20261005`, mounted as
`/workspace/bin`. The checkout is mounted read-only at `/workspace`; Docker
network access is disabled. No checkout database credentials are consumed.
The pinned image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with GCC 13.3.0 and Python 3.12.3.

The orchestration command for each mode is:

```powershell
docker run --rm --network none `
  --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max,target=/workspace,readonly' `
  --mount 'type=bind,source=D:\CodexEvidence\accounting-plan5\bin\restore-types-20261005,target=/workspace/bin' `
  --workdir /workspace `
  sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a `
  python3 -u -B tmp/plan5/run-restore-type-checks.py MODE
```

The final pure child command is:

```text
python3 -u -B -m unittest -v
  test_economic_sql_canonical_audit.RestoreProjectionTests
  test_economic_sql_canonical_audit.CanonicalAuditTests
  test_plan5_child_identity.ChildIdentityTests
  test_reconcile_economy_accounting.ReconciliationTests
  test_economic_sql_audit_origins.OriginTests
  test_economic_sql_audit_origins.ItemRevisionTests
  test_economic_sql_audit_origins.BaselineVersionTests
  test_economic_restore_mobile_grammar.MobilePositionTests
```

`pure-complete` passes 193 methods, zero skips, exit 0, in 86.722 process
seconds (78.819 unittest seconds). Earlier 193-method passes are retained as
separate attempts; they do not multiply distinct coverage. Each final mode has
`inputs.json`, `results.json`, `checks.log` and copied sources below
`bin/tests/p5-restore-types-20261005/`.

`native-complete` runs
`python3 -u -B tmp/plan5/compare-native-restore-types.py`, selecting
`test_economic_sql_canonical_audit.NativeCanonicalAuditTests`. It enables
`DURIS_PLAN5_CANONICAL_NATIVE=1` and selects its fresh `native` artifact directory.
The comparison helper reruns the old verifier only after a new API refusal,
using the same executor and consistent read snapshot, then propagates the new
refusal. It adds read-only observations without changing CLI behavior,
transaction rollback or cursor closure. Exact gates and paths are in the receipt.

The original probe is compiled during `native-first` for SQL and flatfile with
C++20, strict warnings as errors, ASan/UBSan, leak detection and immediate
sanitizer failure. Both compile and execution exits are 0, with empty stderr.
Both binaries have SHA256
`a6384b663fb2f2f837bc543f0ecfda427f500f78a3ad5c6fb887e81457e54c0e`.
The final run reuses those preserved binaries only after checking original probe
text, every native input hash, exact flags, recorded successful exits and binary
hashes. This is verified reuse of this slice's compilation, not a fresh final
compilation. Complete commands remain in `native-builds.json`.

Both databases are freshly bootstrapped and migrated through canonical 0056.
Versions are MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL
`8.0.46-0ubuntu0.22.04.4`. The native probe supplies original EAI1/EAP1 capsules;
the test imports their projections into disposable databases. This establishes
reader behavior, not an integrated gameplay producer journey.

| Engine | API/CLI cases | Accepted controls | Expected refusals | New float refusals |
| --- | ---: | ---: | ---: | ---: |
| MariaDB | 39 | 25 | 14 | 5 |
| MySQL | 39 | 20 | 19 | 10 |

The final native method passes, zero skips, exit 0, in 242.713 process seconds
(236.285 unittest seconds). Each engine retains all 19 original cuts and adds
10 storage cuts plus their 10 restored controls. The separate owner changes one
column in each of five families to DOUBLE or DECIMAL(20,1): account
`after_silver`, posting `delta_silver`, child `domain_id`, item `before_revision`,
and custody `to_owner_context_id`. Original values and native bytes remain exact;
child values include UINT32_MAX. The actual SQL JSON type/value is saved for
every cut. All 15 float-valued projections are refused by the fixed verifier and
accepted by the old verifier in the same snapshot.

MariaDB normalizes all five tested DOUBLE values to JSON integers and accepts
those exact projections. MySQL preserves floats for those cuts. This fix checks
the projected representation; it does not authenticate physical SQL column
types when JSON normalizes them. The five MariaDB results are normalization
controls, not qualification of the altered schemas. Schema fingerprint and
release checks remain separate gates.

After each cut the fixture restores the complete original column definition,
including defaults, and requires byte-identical `SHOW CREATE TABLE` output and
unchanged database rows. Each API call rolls back once and closes its cursor.
API queries are SELECT or read-only transaction setup; CLI exits are 0 for
accepted controls and 2 for controlled refusals, with no capsule/alias disclosure.
Existing 24 damaged saved projections remain checked across limits 0/1/100 and
exception/operation/provenance views: 216 API and 216 CLI observations, with
unchanged saved input and authority.

Two failed native fixture attempts are preserved: `native-first` incorrectly
required MariaDB DOUBLE to produce JSON floats; `native-final` completed all
cases but expected the old total 38 instead of 78. Neither is reported as a
passing run. The first failure left a private negative schema cut in its retained
datadir; only `native-complete` verifies all schema and row restorations. An
earlier `red-native` storage experiment shows eight old canonical accepts during
the origin-reader's driver-type refusals; SQL JSON could normalize those values,
so that experiment is not counted as eight proven JSON float false-cleans.

## Full restore consumer and remaining gates

The final `baseline-restore` mode selects the existing
`test_native_sql_baseline_audit.NativeBaselineAuditTests` through the retained
temporary-directory helper. It enables `DURIS_RUN_NATIVE_BASELINE_AUDIT=1` and
uses `bin/tests/plan5-baseline-sql-restore/restore-types-20261005`. Its actual
child command is `python3 -u -B tmp/plan5/retain-restore-type-baseline.py`; the
unmodified test invokes `python3 -u tests/async/run_native_sql_baseline_audit.py`
for each engine. The method passes, zero skips, exit 0, in 638.848 process
seconds (633.346 unittest seconds), with both engine subprocesses exiting 0.

Both baseline fixtures compile fresh with zero reused objects, C++20, strict
warnings as errors and ASan/UBSan. SQL and client-free binary hashes are:

```text
SQL          9ff48b67a9e4cff9b71e5001e9783eaa0f51f82b3efe278248ddc81ad423f7bf
client-free  4bd40802a3f1ef04eb10c7d7ff7e066d298af36a52ebdc011b924bee98439a3a
```

The existing test retains all original cases on both freshly migrated canonical
engines: 157 corruption cuts and their independent restore refusals, seven
constraint refusals, two inactive epochs/books, original command-binding and
keys-hash checks, orphan reservations, exact operator views and the SQL partial
exporter's original damage checks. The full SQL qualifier is exercised directly,
on its declared corruption cases and on a cold imported clone. Each clone
preserves all 18 captured tables, qualifies complete migration history and
economic evidence, and reproduces the exact original native baseline output
without changing rows. The SELECT-only role is denied UPDATE with code 1142.

| Cold clone | Dump bytes | Dump SHA256 |
| --- | ---: | --- |
| MariaDB | 332977 | `263bab6009d0c666e66de67e722507ada0382a0889c85ea61842f3512dc566a1` |
| MySQL | 341737 | `5dcf770b239dce411780673185710c9996cd01eb4bd2eedd9f379963543cc878` |

Native capsules, both dumps, migration histories, logs, build metadata and the
stopped original private datadirs are retained. `evidence.json` records both
engine outcomes, consumed hashes and zero skips. The clone datadirs use the
test's normal temporary cleanup; retained dumps and cold-clone receipts bind
their exact successful import and replay. Original partial captures retain
their five declared findings: one `evidence_loss`, two `missing_native_holding`
and two `missing_native_item`. The test explicitly records incomplete world
capture and release status; it does not establish an entire clean game world.

The final three test selections execute 195 distinct methods (193 pure, one
canonical native and one baseline native), zero skips. Repeated green attempts
and negative storage/model observations are not additional distinct methods.
No external blocker prevents this completed verifier fix.

The primary advanced during these checks to
`bb25935985af38181080b681b083cdd84774bd52`, importing the preceding baseline-origin
fix without changing its executable inputs and adding native SHOP work. Its
native tree is `b00968beadaa72d2e126c11d41a92231017e6d27`; it is not the source
qualified by this slice. Integrate and test that combined source separately.
Native EAB2/schema61 remains unpublished here and is not qualified by reference
models or historical 0056 fixtures.

The normal validator exits 0 (12.104 seconds), the matrix `--check` exits 0
(13.552 seconds), and the release validator exits 1 (0.145 seconds), with
`writer has no executable evidence`. Exact commands and source maps are retained
under `bin/tests/p5-restore-types-20261005/inventory/`. The expected release
refusal is an open gate, not a completed acceptance result.

No interface or schema change is required. Narrow primary-owned registration
request: include these six methods in the existing offline canonical-audit
entry's `required_cases` and `arguments`, or an equivalent dedicated entry in
`tests/integration_manifest.json`, while preserving all existing cases:

```text
RestoreProjectionTests.test_intact_ordinary_baseline_and_rejected_controls
RestoreProjectionTests.test_all_normalized_projection_families_require_exact_integers
RestoreProjectionTests.test_root_metadata_requires_exact_integers
RestoreProjectionTests.test_committed_counts_and_status_require_exact_integers
RestoreProjectionTests.test_rejected_controls_require_unsigned_integer_status_and_zero_counts
RestoreProjectionTests.test_baseline_reservations_require_exact_integers
```

They consume `economic_restore_evidence.require_integrity`, preserving exact
integer projections, original capsule comparisons and read-only refusals. This
registration does not promote writer-route coverage or require a schema change.
Import this report through the primary's notebook curator and register only its
recorded source and scope; the tested combined candidate remains primary owned.

Full gameplay/persistence, original writers, workload/fault budgets, schema61
installation and engine sealing, complete backup/restore/retention/erasure
journeys and all R1–R8 release gates remain open. The native canonical mobile
and source-event variants, full flatfile authority matrix, and maintained
726-unit server builds were not repeated for this Python-only slice. Their
earlier evidence keeps its original exact source scope; this slice does not
extend it to the primary's newer native SHOP tree. No accounting activation,
production mutation, deployment, PR merge or audit autocorrection occurred.
Inactive behavior, wallet-root item exclusions and the declined inactive spell
change remain preserved.
