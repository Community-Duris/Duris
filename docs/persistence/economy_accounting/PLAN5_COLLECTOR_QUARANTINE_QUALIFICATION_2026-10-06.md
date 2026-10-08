# Plan5 retained collector quarantine qualification — 2026-10-06

The collector's native held-item cancellation produces quarantined custody,
but all four independent SQL ownership projections labeled it live. The
exporter previously used only the destruction owner to infer a tombstone and
treated every other destination as live. A coherent collector quarantine could
therefore produce false stale-custody or original-plan state disagreement.

The existing producer contract supplies this specific state without a new
field: collector ledger reason 21, source owner 10, destination system owner 7,
destination ID 0/context 0. `collector_command.c` requires target state 3 for a
quarantined cancellation, and `collector_repository.c::held_authority` records
reason 21 for both expiration and cancellation. Generic item transfers reject
collector reasons, and both SQL/flat collectors share the held command rule.
The exporter now retains quarantined for that exact transition in selected
events, linked UID references, lineage history and unattributed history.
Destruction remains tombstone; unrelated system custody keeps the existing
projection. Action remains move, preserving the distinction from supply.

The actual both-engine check then established that four retained-history
consumers rejected the correctly exported state: lineage UID references,
lineage history, unreferenced events and unattributed events. The complete
fix accepts quarantined in each consumer while preserving unknown-state
refusal/orphan diagnostics. A separate negative control showed that the UID
lifetime audit missed a retired UID returning in quarantined custody. It now
treats both live and quarantined custody as occupying the retired UID and
retains the existing resurrected_item_uid finding.

This is a separate independent read-only fix. There is no mutation-code import,
schema change, automatic correction or new authentication/completeness claim.

## Source and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Branch/publication: remote `codex/accounting-plan5`; earlier branch work and
  follow-ups remain preserved there. No independent experimental-accounting push.
- Tested base `3db39b410ad4f0df799f88b224e9b2a32bdfc6a5`, including primary `d35dbaba2100f0c916e8c3680df5437c34683d44` and the
  preceding compound-action issue/primary refresh. The delivery receipt binds
  this report and exact qualified code to the separate result commit.
- Native tree `bf7a92a728ad9b5b813626462e56533f8ba39c97`; migration tree
  `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61. Earlier 0055 evidence does not qualify
  this candidate. Primary's unpublished producer composition remains outside it.
- Owned source: `scripts/economic_sql_audit_snapshot.py` and
  `scripts/reconcile_economy_accounting.py`.
- Owned regressions: `tests/async/test_economic_sql_uid_scope.py` and
  `tests/async/run_economic_sql_audit_snapshot_mysql.py` and
  `tests/async/test_reconcile_economy_accounting.py`.
- Owned documentation: this report and `AUDIT_OPERATIONS.md`.
- Shared interface/schema requests: none for this fix. It consumes five already
  selected fields: reason_type, from_owner_type, to_owner_type, to_owner_id and
  to_owner_context_id. Contracts, producer integration, registry/matrix,
  coordinator, manifests and activation owner remain untouched.

| Exact changed input | SHA256 |
| --- | --- |
| scripts/economic_sql_audit_snapshot.py | `f014c7f8aa6934608aa89064504b4ab25f5b546dc69a6e381e99ba1bbb8e409a` |
| scripts/reconcile_economy_accounting.py | `1631ef218238b674053677d8359a831b4c3db78841b13f5f06412b4d9fd0c089` |
| tests/async/run_economic_sql_audit_snapshot_mysql.py | `f9a776923740d349836770fe71b1386fd205494f23b95979e9c3052cab9fd6cf` |
| tests/async/test_economic_sql_uid_scope.py | `aec1ea04723348932cf8f281e75e71bfb7bce9c80fd9b81cfe98b390a367cdab` |
| tests/async/test_reconcile_economy_accounting.py | `7e3f5b0cbac232b7e092aebd8bf3c8669901f7c0072ecaf546651b5e63c993cd` |

Frozen archive SHA256 `7b280562e9aa0f51910b48a6a92bdcc0bd787e628ebe36b60093ac9fe4eca721` contains 3558 regular
source/helper files, no links, and 3081 maintained
code/migration/test inputs matching the final result. No live checkout,
credentials, player data or host database is mounted. The 918 regression
inventory entries and 105 integration rows remain exact; counts do not establish
executable coverage or release completion.

## Commands and observations

```text
python -B tests/async/test_economic_sql_uid_scope.py UidScopeTests.test_compound_item_actions_follow_retained_supply_endpoints -v
python -B tests/async/test_economic_sql_uid_scope.py -v
PYTHONPATH=tests/async python -B -m unittest -v test_reconcile_economy_accounting.ReconciliationTests.test_quarantined_custody_is_valid_in_every_uid_history_scope test_reconcile_economy_accounting.ReconciliationTests.test_quarantined_lineage_reference_preserves_unknown_state_diagnostics test_reconcile_economy_accounting.ReconciliationTests.test_quarantined_custody_preserves_retired_uid_lifetimes
python -B tmp/plan5/prepare-collector-quarantine-02.py
python3 -u -B -m unittest -v test_item_equipment_reconciliation test_economic_sql_uid_scope test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_reconcile_economy_accounting.ReconciliationTests test_reconcile_economy_accounting.AuditBudgetTests test_plan5_child_identity.ChildIdentityTests test_plan5_child_identity.ChildIdentityBudgetTests
python3 -u -B tmp/plan5/collector-quarantine-native.py
python3 -u -B tmp/plan5/qualify-collector-quarantine-native.py
python -B -m unittest -v test_economic_sql_uid_scope test_reconcile_economy_accounting.ReconciliationTests
python -B scripts/validate_economy_accounting.py
python -B scripts/generate_economy_writer_coverage.py --check
python -B scripts/validate_runtime_compatibility.py
python -B scripts/validate_economy_accounting.py --release
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/collector-quarantine-current61 DURIS_REGRESSION_BUILD_CACHE=off python3 -u -B tests/async/test_native_sql_baseline_audit.py
python -B tmp/plan5/seal-collector-quarantine.py
git diff --check
```

Linux selected checks inherit `PYTHONPATH=/workspace/tests/async`,
`PYTHONDONTWRITEBYTECODE=1`, `DURIS_RUN_AUDIT_BUDGET=1`,
`DURIS_PLAN5_CHILD_IDENTITY_BUDGET=1`,
`DURIS_PLAN5_CHILD_IDENTITY_BUDGET_ARTIFACTS=/workspace/bin/tests/collector-quarantine-child-budget`
and `DURIS_REGRESSION_BUILD_CACHE=off`. Windows inherits
`PYTHONDONTWRITEBYTECODE=1` and `PYTHONPATH=tests/async`.

The pinned isolated image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with Python 3.12.3/GCC 13.3/Make 4.3, MariaDB 10.11.14 and MySQL 8.0.46. Windows
uses Python 3.12.10. The container has network none, 2 CPUs/4 GiB, RAM workspace/
tmp/private cold-restore parent, and a disposable writable root. Only protected
evidence and the read-only loader are host mounts. Original flags, deadlines,
cases, native caching-off behavior and recipe markers are preserved.

| Check | Result |
| --- | --- |
| Focused pre-fix regression | Expected failure: the exact collector quarantine was live in all four projections; all controls retained their existing behavior. |
| Consumer pre-fix controls | Three methods: three history errors, one false orphan-reference finding and one missed UID-resurrection finding. All unknown-state controls retained their diagnostic behavior. |
| Repaired exporter class | 12 methods PASS, zero skips. |
| Complete focused exporter/consumer checks | 15 methods PASS, zero skips. |
| Linux selected regressions and enabled audit/child budgets | 208 methods PASS in 65.563s, zero skips. |
| Windows exporter/reconciliation classes | 136 methods PASS in 82.860s, zero skips. |
| Native collector observer | Fresh SQL/client-free C++20 builds preserve original strict ASan/UBSan flags/assertions; expiration and quarantine effects/context PASS in 80.113s. |
| Complete original native baseline/restore method | Both-engine PASS in 349.427s, zero skips, fresh original SQL/client-free binaries. |
| New SELECT-role phases | Collector quarantine and ordinary live system custody on each engine: four actual captures and 12 provenance CLI checks at limits 0/1/100 PASS. |
| Normal accounting/matrix/current61 metadata | Exit 0; release readiness remains false. |
| Release validator | Expected exit 1: writer executable evidence is incomplete. |

The expanded existing reader regression covers 15 fact patterns through all four
real reader functions. It retains previous creation/destruction compatibility,
then checks the collector quarantine and controls for unrelated system custody,
wrong source type, nonzero system ID and nonzero system context. Source rows
remain immutable, every query is SELECT-only, and personal aliases stay absent.
Committed-only revision 0 creation inference and rejected/unknown refusals remain.

Three added consumer methods cover all four retained history paths, valid
quarantined custody and unknown states (null, numeric 3, unknown text and true),
then check continuous creation-to-quarantine history and both event-retired and
baseline-retired UID resurrection. The test-only synthetic EAP1 encoder now
represents quarantined state 3 explicitly, preserving the prior live/tombstone
encodings. Native wire and production codecs are unchanged.

The native observer inserts print records after the original held-plan structural
assertion and wraps the unchanged original main. Exact inverse proof preserves
every original support/assertion byte. The original purchase, failure, expiry
and quarantine fixture paths execute, with 15 original production translation
units compiled freshly and zero reused objects. These are authentic factory
effects; the legacy reason 21 shape is taken from the actual writer source.
It does not execute the SQL writer or establish collector gameplay/publication.

The disposable SQL phases use the existing guarded private modeled schema and
SELECT-only account. A separate item-only modeled root records the history,
and its omitted original plan remains exactly one additional missing-plan
finding. Every original global partial finding remains. The creation history
and current native row are staged coherently from collector to system custody;
only reason/state differ between quarantine and ordinary live system control.
Each reader capture rolls back/closes once and leaves all 24 table hashes equal.
The new rows are removed and every changed original row plus the complete
initial snapshot is restored exactly. These are `sql_partial`, `complete:false`
captures, with no native collector admission, command-receipt or gameplay claim.

The surrounding complete method preserves two authentic native EAB2 books/
epochs per engine, 161 damaged-evidence refusals, seven constraint refusals,
ten key-hash cuts, two command-binding cuts and cold dump/import/replay. It also
retains 14 equipment/topology captures, four quarantined-coin captures, 26 supply
probes, eight compound-action captures and their original bounded CLI checks.
Both UPDATE denials remain 1142. The fresh original SQL/client-free baseline
binaries remain
`72d6d5027b5ed550921efd6b11d14b120dacc1b42d53b6ba32406f96b3f54f2c` and
`dd503bd0c1ba66557ff96a9a2004d5007d60525c5c7259bdb5224ced1072ce0d`.
There is no maintained C/C++ edit, new full server build or gameplay boot for
this Python fix; native source matches the prior full maintained build evidence.

## Evidence and remaining gates

Protected evidence root:
`D:\CodexEvidence\accounting-plan5\bin\collector-quarantine-02-20261006`.
Frozen inputs, the negative control, native/compiler commands, actual captures,
inventories and complete restore logs are retained. The seal checks
448 artifacts and 3081 maintained inputs:
`tmp/plan5/collector-quarantine-evidence.json`, SHA256 `7f6f7b7699caacd5caaa20fdccdc5cea49acc97f6e4c089801ef296af6d45fa0`.
Publication is bound by `tmp/plan5/collector-quarantine-delivery.json`.
Selected qualification has zero skips and no independent slice blocker.

The initial exporter-only attempt remains at
`D:\CodexEvidence\accounting-plan5\bin\collector-quarantine-01-20261006`.
Its frozen archive is `0da8d5a90b10a6019124289e58f4f24ddb52fdcc0819d9d76acf9b0b08988a88`. Linux 205,
Windows 133 and both original native collector builds passed, but both SQL
engines failed with invalid lineage UID history event before the complete
restore qualification. That failure is not waived or counted as a pass. Its
105 artifacts are sealed separately,
and the unchanged exporter/SQL-driver inputs are checked against the final
archive. The second attempt adds the consumer repair and freshly reruns all
selected checks and the entire original both-engine baseline/restore method.

The legacy ledger still has no general custody-state column. This exact
collector contract does not infer unknown state for other histories, qualify
complete command receipts or promote an export to complete authority. Full
authority capture, real writer/receipt coverage, remaining mechanics and
flatfile parity, lifecycle/erasure and combined release-host workload/storage
budgets remain open. Inventory and component checks do not complete R7/R8.
Accounting stays inactive, wallet-root exclusions and the declined inactive
spell path remain, and no production mutation, auto-correction, activation,
deployment or PR merge occurred.

Primary maintains the notebook locally; the user declared it nonblocking.
This owned report and sealed delivery receipt are its curator packet. Shared
notebook/coordinator files are not independently rewritten here.
