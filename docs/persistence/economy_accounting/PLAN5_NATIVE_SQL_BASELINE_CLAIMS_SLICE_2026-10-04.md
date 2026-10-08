# Plan 5: independently reconcile native SQL baseline claims

This slice is based on `57d4d99b296f9f1ac7b8e7360a6220c9e944d8e3` in
`codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. The result SHA
and committed blobs are bound by `tmp/plan5/native-baseline-evidence.json`.
Publish this lane's branch; the primary owner integrates it into
`experimental-accounting`.

The refreshed canonical remote is
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. All 1,232 native server inputs and
236 migration inputs remain unchanged. Native tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b` comes from canonical base
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`. Every SQL integration below creates
fresh private databases through canonical `0056_spell_ward_durability`.
Earlier 0055 results do not qualify this slice or the combined candidate.

## Established defect

The native SQL baseline persistence owner stores one source claim for each
committed baseline operation. The independent reconciler nevertheless reported
`baseline_source_claim` for every reason-38 claim. The SQL exporter also excluded
reason 38 from lineage-wide missing-claim, duplicate-source and required-source
coverage. Valid native publication produced false findings while damaged
baseline source coverage could be omitted.

The controlled RED test uses the actual native SQL owner to initialize two
inactive epoch books, apply two nonzero baseline batches, replay both exact
commands and reconcile their durable receipts. Each batch includes wallet and
item origins. Both engines then report two erroneous `baseline_source_claim`
findings. The RED run took 244.120 seconds, with two engine failures and no
skips. Frozen source, native binaries and engine logs remain preserved.

The fixture does not capture a real world boundary. It creates inactive epoch
metadata with a private fixture owner and calls the existing native test access
interface. The native owner has no production caller at this source. Actual
holding/UID publication and namespace activation are not established.

## Scoped independent fix

After verifying an existing EAB1 book's digests, envelope, record order,
holdings/items, dense revisions, terminal control and committed roots/receipts,
the independent origin reader derives each baseline source identity from
preparation ID, epoch, batch index, kind 10, version 1 and slot zero. It imports
no native mutation codec or coordinator.

Within the same SELECT-only consistent read, the exporter binds selected and
retained epoch baseline claims to these independently read identities. Retained
books are decoded once per epoch; the cache retains only source maps. A whole
lineage preflight refuses more than 100,000 witness rows or 32 MiB of witness
bytes before decoding another epoch's book. These are input bounds, not proof
of a fair live sweep or historical coverage watermark.

The reconciler admits a SQL baseline claim only when its exact witness identity,
joined lineage/epoch/root/source, committed outcome and durable inbox receipt
agree. Missing or damaged retained books remain findings; damage to the selected
origin book refuses capture. Global source coverage now includes reason 38.
Ordinary claims retain their existing checks, and flatfile baseline claim
rejection remains unchanged.

These are additions to owned audit output, with no shared schema or producer
change:

| Field | Exact value and consumer |
| --- | --- |
| `economic_sql_audit_origins_v1.baseline_source_events` | Map from operation ID hex to derived 48-byte source identity hex; consumed by the SQL snapshot exporter. |
| `source_claims[].baseline_witness` | Four keys exactly: `lineage`, `epoch`, `operation_id`, `source_event`; canonical hex identities from a verified book, or null when unavailable. Consumed by the independent reconciler for reason 38. |
| Existing source coverage counters | Include baseline roots alongside ordinary roots throughout the lineage; consumed by reconciliation and operator views. |
| Exporter CLI `--socket` | Explicit optional Unix socket transport. Existing host/port invocation remains supported. Both exporter CLI journeys execute against the private socket databases. |

Tests verify exact proof fields, selected/retained books, foreign identities,
receipt/source disagreement, global bounds and per-epoch cache behavior. No
Boolean fixture attestation admits a claim. No shared interface request is
required for this fix.

## Native and disposable-database qualification

Environment: image `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
Ubuntu 24.04, Python 3.12.3, GCC 13.3, PyMySQL 1.0.2-2ubuntu1.1.
MariaDB is `10.11.14-MariaDB-0ubuntu0.24.04.1`; MySQL is
`8.0.46-0ubuntu0.22.04.4`.

The checkout is mounted read-only and `bin/` is a separate writable mount.
Fresh private Unix sockets disable TCP. Database credentials are private fixture
constants supplied in a clean environment; checkout `.env` is never loaded.
The audit account has SELECT only, and UPDATE is denied with error 1142.

The native fixture compiles 13 source inputs in SQL and `__NO_MYSQL__` modes
with strict C++20 warnings and ASan/UBSan. SQL binary SHA-256 is
`7985d95effd41456d445a5924e466a5ae5ad25fa9506d141d2ff1dd1168c4ee2`;
client-free binary SHA-256 is
`efaee7f33a1ebc38fe9b6afe9baaa32b66b8b3db1746679dc58c05cf39a9b027`.
The client-free initialize/apply/reconcile calls all refuse with `ENOTSUP`.

For each canonical engine, the intact independent audit accepts both native
baseline claims. It still reports `evidence_loss: 1`, `missing_native_holding: 2`
and `missing_native_item: 2`: the fixture has no native holding/UID capture.
These component limitations are deliberately retained, and the snapshot remains
partial with accounting inactive.

Each engine executes 19 damage cuts: missing selected/retained claims, claim
identity or root disagreement, duplicate/missing required sources, damaged
receipts, book controls, witness digests/revisions, rebound preparation/batch/
epoch identities, malformed claim source and a lost retained book. Seventeen
cuts produce the exact expected additional diagnostics; two selected-book
cuts refuse capture. Each read leaves all 15 captured authority/evidence tables
unchanged. Fixture repairs are performed only by the private test owner, and
the final native replay sees the exact original state.

Two additional cases verify canonical constraints without altering schema:
the composite source/root foreign key refuses with 1452, and operation-claim
uniqueness refuses with 1062. Five damaged-import cuts temporarily disable
foreign keys in the private fixture owner's session to inject otherwise
unreachable damage. Foreign keys are re-enabled before every audit and every
native replay. CHECK and UNIQUE constraints remain enforced. These test-only
owner writes are not audit correction behavior.

The existing broader SQL exporter/operator matrix also runs in a separate
synthetic sibling schema on each private daemon, including both exporter CLI
journeys. Its baseline fixture now uses the correct EAB1-derived source and
matching claim. Synthetic results remain component evidence.

| Exact command inside the image | Result and local evidence |
| --- | --- |
| `DURIS_RUN_NATIVE_BASELINE_AUDIT=1 python3 -u tests/async/test_native_sql_baseline_audit.py -v` | Pass: one guarded test, both engines, zero skips, 261.855 seconds; 19 cuts and two constraint refusals each, native exact replay and full sibling exporter matrix. `tmp/plan5/baseline-native-green4.log` and `bin/tests/plan5-native-baseline-audit/{mariadb,mysql}.log`. |
| `python3 -m unittest test_reconcile_economy_accounting test_economic_sql_audit_origins -v` from `tests/async` | 116 collected tests: 111 passes and five explicit skips, 17.104 seconds. `tmp/plan5/baseline-native-components-final.log`. |
| `DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1 python3 -m unittest test_economic_sql_audit_origins.NativeSQLOriginTests -v` | Two native origin tests pass in 207.711 seconds, zero skips. Each engine performs three captures, three refusals and six rollbacks using native EAB1-rich bytes in canonical 0056 schemas. `tmp/plan5/baseline-native-origins.log`. |
| `make -C src -j2` | Pass, no-op; existing server SHA-256 `ac7646a258aa5ebb5a05a95c51f93797ad95e97d74716b3b5a05042910425ace`. `tmp/plan5/baseline-native-build.log`. |
| `scripts/format.sh --check --file tests/async/plan5_sql_baseline_audit_fixture.cpp` | Pass with WSL clang-format 14. `tmp/plan5/baseline-native-format.log`. |
| `python -m py_compile` for the eight changed Python files; `git diff --check` | Pass; staged and final checks bound by the manifest. |
| `python tests/async/test_native_sql_baseline_audit.py -v` on Windows | One explicit Linux/opt-in guard skip. `tmp/plan5/baseline-native-host-skip.log`. |

The exact Python syntax-check command is:

```sh
python -m py_compile scripts/economic_sql_audit_origins.py scripts/economic_sql_audit_snapshot.py scripts/reconcile_economy_accounting.py tests/async/test_native_sql_baseline_audit.py tests/async/run_native_sql_baseline_audit.py tests/async/run_economic_sql_audit_snapshot_mysql.py tests/async/test_economic_sql_audit_origins.py tests/async/test_reconcile_economy_accounting.py
```

The unit invocation's two native-origin skips are covered by the separate
explicit native-origin run. The two near-limit budget cases and native-stake
SQL integration case are not rerun by this slice; their prior source-pinned
evidence is preserved. They are not counted as passes here. Native-origin
dependencies did not change after that run; the subsequent CLI socket transport
is exercised by the full sibling exporter runs.

The first fixed attempt passed native claim admission but failed when canonical
foreign keys prevented a corruption injection (319.141 seconds). The next
attempt passed all 19 cuts and both constraints on both engines, then exposed
the sibling test's TCP-only reader grant (165.123 seconds). A third attempt
reached the broader matrix's historical provenance assertion (250.235 seconds):
the old fixture omitted the operator view's existing global exception count.
Its expected view now includes the real audit report, including the unchanged
CLI refusal at each output limit. All failed attempts, their exact frozen source
and engine logs are preserved and are not passes.
The final manifest verifies all preceding slice artifacts remain unchanged.

## Owned files and integration gates

Owned files are the three independent scripts (`economic_sql_audit_origins.py`,
`economic_sql_audit_snapshot.py`, `reconcile_economy_accounting.py`); the two
existing unit files; the existing SQL exporter matrix; the new
`plan5_sql_baseline_audit_fixture.cpp`, `run_native_sql_baseline_audit.py` and
`test_native_sql_baseline_audit.py`; and this report. Shared coordinators,
producer integration, accounting contracts, migrations, writer registry/matrix,
release registration and activation owner are unchanged.

The primary owner should register the new native test with
`DURIS_RUN_NATIVE_BASELINE_AUDIT=1`, both canonical engines and the documented
compiler/client prerequisites, with
`NativeBaselineAuditTests.test_native_baseline_claims_both_canonical_engines`
executed and zero skips. Register the already requested native-origin pair separately. Shared
registration fields are `path`, `arguments`, `environment`, `required_cases`,
`provider`, `engines` and timeout; consumers are the integration runner and
combined release report. This is an integration handoff, not a shared contract
or schema change.

Complete native capture, authority-bound initialization/activation, executable
writer journeys, failure/reply-loss/restart matrices, populated upgrades,
retention/erasure/export, fair live sweeps and measured mixed-workload budgets
remain open. The primary owner's final published combined candidate must be
qualified at its exact source; this component result does not establish release
completion. Prior backup/restore qualification remains source-pinned and is not
rerun or transferred to the combined candidate by this audit-only slice.

`AI_CONTEXT.md` and the required curator workflow/notebook reference remain
unavailable. The earlier request is unanswered; bounded accessible Pages
searches returned no target and do not prove the notebook is absent. This
report and protected manifest are a curator-ready handoff, not a claimed
notebook update. Accounting remains inactive; wallet-root item exclusions,
the declined inactive spell-path change and active blackjack refusal are
preserved. No production access, activation, merge, deployment or audit
auto-correction occurred.
