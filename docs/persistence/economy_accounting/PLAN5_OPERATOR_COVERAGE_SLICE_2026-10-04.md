# Plan 5: preserve whole-audit status in every bounded operator view

Delivery branch: `codex/accounting-plan5`. Worktree:
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base: `dbc5038c213a8f0b3427c203c1c3c096625110d3`. The result is the commit
containing this report, recorded after commit in
`tmp/plan5/operator-coverage-evidence.json` and separately published from
`experimental-accounting`.

Canonical remote refreshed before choosing the base:
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native source remains tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, all 1,232 native inputs,
with canonical migration 0056. The primary owner's unpublished combined
integration is not qualified by this delivery branch.

## Established output defect and independent fix

Unfiltered holdings, supply, prices and routes returned only `count`, `rows`
and `truncated`, while provenance coverage omitted the global exception count.
The CLI correctly exited nonzero for discrepancies, but the JSON itself lost
the whole-audit status for consumers that saved or forwarded only that output.
An empty price/supply view or limit-zero result could therefore omit every
visible indication of an incomplete cut or an unrelated native discrepancy.

Two focused regressions establish the defect on the unchanged base reader:
four clean/corrupt and complete/incomplete combinations, six operator views,
and limits 0/1/100. The direct-view and actual CLI checks produce 120 missing
coverage/count errors. CLI exit status remains correct in the RED run.
`operator-coverage-unit-red.log`, the base reader and RED test source are
preserved under `tmp/plan5/`.

Every non-exception view now includes the same existing audit context:
`coverage.lineage`, `selected_epoch`, `complete`, `quiescent` and
`exception_count`. The context is assembled once within the owned view function.
Filtered holdings, operation and provenance retain their additional scope
fields. Row projection, selection, order, counts, truncation, aliases and CLI
exit semantics remain unchanged. Exception view still returns the full report.

The independent `Reconciler` class is AST-identical to the base. There is no
change to conservation, source policy, origin/lifetime checks, native authority,
mutation integration, schema, accounting intent/plan or snapshot input format.
Coverage reflects the supplied snapshot and report; it is not a writer-matrix
or release attestation. Existing direct provenance calls that supply no audit
report retain an unknown (`null`) count instead of fabricating zero. The actual
CLI always supplies the full report and emits an integer global count.

Owned files:

- `scripts/reconcile_economy_accounting.py`
- `tests/async/test_reconcile_economy_accounting.py`
- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`
- this report

Shared coordinator, producer, registry/matrix, accounting contracts, migrations
and activation-owner files remain untouched. Accounting stays inactive.
Wallet-root item exclusions, the declined inactive spell change and active
blackjack refusal remain preserved.

## Regression and native/database qualification

The focused tests pass all 72 direct-view combinations and all 72 real CLI
invocations. Clean complete inputs retain zero exceptions; unrelated native
balance corruption retains one; incomplete/unfenced inputs retain two; combined
inputs retain three. Every view preserves these values, its declared flags,
lineage and selected epoch at all limits. Detail rows remain bounded, counts and
truncation remain invariant, aliases remain absent and every input is unchanged.

The maintained native fixture now uses
`bin/tests/plan5-operator-coverage-sql`, preserving earlier binaries and outputs.
It compiles eight unchanged production sources in SQL and flatfile modes with
C++20, strict warnings as errors, ASan/UBSan and no PIE. Both agree on native
EAI1/EAP1 bytes, 104 source grammar decisions, 1,107 source-policy decisions and
six original-link decisions. This is structural/component proof, not an active
writer or player journey.

Fresh private MySQL 8.0.46 and MariaDB 10.11.14 datadirs use Unix sockets with
TCP disabled, bootstrap/adoption and canonical migrations through
`0056_spell_ward_durability`. FK/CHECK constraints remain enabled. Each engine
passes 41 canonical captures. All seven compared authority/evidence tables
remain exact; the SELECT-only user is denied UPDATE (1142), and each capture
rolls back and closes its cursor exactly once. On valid incomplete native cuts,
all six views retain the mandatory global exception, lineage/epoch and flags at
limits 0/1/100. No capture is relabeled complete by selecting a view.

The maintained opt-in budget also passes the same six actual CLI workloads:
15,245 synthetic mapping creators, 33,551,932 input bytes, clean/corrupt inputs
and limits 0/1/100. Bounds remain 30 seconds and 256 MiB; measured ranges are
recorded exactly in the evidence manifest. These remain container component
measurements. They do not qualify every operator shape, mixed history, database
query plans, real writer latency, growth/checkpoint budgets or the release host.

## Commands and pinned evidence

Native Linux qualification uses `duris-plan5-origin-sql-tools:local`, immutable
image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Toolchain: Ubuntu 24.04, Python 3.12.3, GCC 13.3.0, PyMySQL package
1.0.2-2ubuntu1.1, MySQL 8.0.46-0ubuntu0.22.04.4 and
MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1. Checkout is read-only and only
`bin/` writable. No ports are published or capabilities added; no production
data or checkout credentials are used.

| Command | Result | Evidence under `tmp/plan5/` |
| --- | --- | --- |
| Host focused two operator regressions at the base | RED, missing JSON coverage/count fields | `operator-coverage-unit-red.log` |
| Host focused two operator regressions after fix | PASS two tests, including 72 actual CLI invocations | `operator-coverage-focused-green.log` |
| Host `python tests/async/test_reconcile_economy_accounting.py -v` | 92 tests PASS, two explicit Linux-only skips, 11.268 seconds | `operator-coverage-fast-green.log` |
| `DURIS_RUN_STAKE_SQL_INTEGRATION=1 DURIS_RUN_AUDIT_BUDGET=1 python3 -u tests/async/test_reconcile_economy_accounting.py -v` in the isolated container | 92 tests PASS, zero skips; both native modes, both canonical engines through 0056 and six budget cases | `operator-coverage-qualified-green.log` |
| Host `python -m py_compile scripts/reconcile_economy_accounting.py tests/async/test_reconcile_economy_accounting.py` | PASS | Command output |
| `git diff --check` | PASS | Command output |
| Post-commit evidence recorder | PASS owned/native blobs, frozen source, helpers, artifacts, logs and preserved prior evidence | `operator-coverage-evidence.json` |

The exact Docker invocation mounts `<worktree>` at `/workspace` read-only,
`<worktree>/bin` at `/workspace/bin` writable, sets workdir `/workspace`, and
passes both environment flags with `--env` to `docker run --rm` using the image
above. The source SHA-256 values frozen before native qualification are:

- Reader: `68bf3bdc742916d492b8abc0095e7d04970eee5d80fc9924946ccc9395d4a9d5`
- Maintained tests: `67ec0c068a6d4e158c0bfcd77445f559718b8be8f5e250229ad577e5cb151e64`

The manifest pins all 1,232 native inputs, migration files, helper inputs,
image/toolchain, native fixture binaries/output and exact budget measurements.
Every earlier Plan 5 artifact remains preserved. Logs, binaries, fixture/player
data and credentials are not committed. Post-commit checks compare tested owned
and native bytes directly with Git blobs. No production C/C++ changes occurred;
server builds, full recovery and native deletion journeys are not repeated.

## Narrow operator-interface handoff and remaining gates

The additive owned operator-output handoff is:

- Fields: `coverage.lineage` and `selected_epoch` are the audited snapshot IDs;
  `complete` and `quiescent` reflect its exact-boolean flags;
  `exception_count` is the whole report's count, independent of view/filter/limit.
- Invariants: preserve global status for clean, corrupt and incomplete inputs,
  including empty and zero-detail results; never upgrade completeness or hide an
  unrelated discrepancy. Existing row fields/counts/scope fields remain intact.
- Consumers: reconciler CLI JSON users; the maintained reconciler/native tests;
  the existing SQL snapshot and Plan 5 retention runners. Exporters importing
  only audit constants/helpers are unaffected. No producer/storage input changes
  or shared accounting-schema change is requested.
- Tests: both focused tests above, current canonical MySQL/MariaDB captures and
  existing filtered-query/provenance tests. Primary integration should retain the
  existing Linux native and budget flags and accept the additive coverage object.

The separate missing retained-price history in the price view is not changed in
this slice. Full independent capture, protected metadata-header and retained
baseline-marker handoffs, writer census/executable evidence, real supported
producer/UID journeys, populated upgrades, governance/retention/export decisions,
mixed workloads, release-host budgets and the primary owner's tested combined
candidate remain required. Component counts do not establish release completion.

Notebook reconciliation remains pending because `AI_CONTEXT.md`, the notebook
reference and required curator workflow are unavailable. The existing request
has no answer and no curator capability is exposed. This report is an evidence
handoff, not a notebook update. No accounting activation, auto-correction,
production mutation, merge or deployment is performed.
