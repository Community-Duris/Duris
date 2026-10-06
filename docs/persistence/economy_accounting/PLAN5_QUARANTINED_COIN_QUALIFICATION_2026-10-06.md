# Plan5 quarantined coin reader qualification — 2026-10-06

The SQL exporter accepts native item state 3 and exports a quarantined coin row,
but the reconciler previously accepted only live/tombstone coin-pile states.
Every such captured row aborted the complete reader with CLI exit 2 and
`invalid coin-pile row`, before operators could inspect its findings. The
canonical current-owner schema permits states 1 through 3; the original native
position validator also accepts quarantine. This was an audit input mismatch.

The reader now accepts the existing quarantined projection and emits
`quarantined_coin_pile` with the UID. No new input/output field, database schema
or mutation capability is added. It keeps quarantined coins outside active
holdings and live-pile totals; available denominations remain captured and a
missing payload stays unknown. Existing missing-holding, dangling-mapping,
stale-state and partial-capture findings remain. Unknown states and negative
denominations still refuse. No audit finding is automatically corrected.

## Source, branch and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Branch and publication destination: `codex/accounting-plan5`.
- Exact qualification base: `e42a50cfd979da5ddcc1768339f9e7edefadac52`; refreshed primary
  `7b00306d4abfd76698f3614e067f59b85c6d32d7` is included. All seven earlier branch tips and the
  preceding quarantined topology fix remain preserved.
- Result: the separate solved-issue commit containing this report. The delivery
  receipt binds its result SHA, exact remote tip and any later primary merge.
- Native tree `bf7a92a728ad9b5b813626462e56533f8ba39c97`; migration tree
  `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61. Earlier 0055 results do not qualify
  this source. Primary's ongoing private birth/quest/native12 implementation
  remains outside this maintained native composition and qualification.
- Owned implementation: `scripts/reconcile_economy_accounting.py`.
- Owned tests: `tests/async/test_reconcile_economy_accounting.py` and
  `tests/async/run_economic_sql_audit_snapshot_mysql.py`.
- Owned documentation: this report and `AUDIT_OPERATIONS.md`. No independent
  shared coordinator, producer, contract, schema, registry/matrix, manifest or
  activation edits. No shared interface/schema/registration request.

The fixed diagnostic code uses the existing bounded finding/UID contract.
All operator views retain whole-snapshot exception counts and status. The
existing reconciliation owner collects the new method, and the original
native baseline recipe runs both new SQL captures before its qualification
marker. All 918 inventory entries and 105 integration rows remain exact; those
counts are inventory, not release coverage.

| Exact changed input | SHA256 |
| --- | --- |
| scripts/reconcile_economy_accounting.py | `b51e43eaa6c3d7acb4960430e141cdaa8053666c91db33f5ee73ff0dfa528e30` |
| tests/async/run_economic_sql_audit_snapshot_mysql.py | `e98222046e6c42880aa32689aff20320943f6855562a5109186977464025ab4e` |
| tests/async/test_reconcile_economy_accounting.py | `bf5ce59557d60ce289531a3088c5d010192887ddaadb7ef3ce2a3f1de33db7d3` |

Frozen archive SHA256 `f9c3722ef4bdc5de2e5d8cb39b0adfba40dd04be31949bea9fdac87fb556b41a` contains 3552 regular
source/helper files and no links. All 3081 code/migration/test
inputs match the final tested composition. Archive inputs are verified before
and after execution. No live checkout, credentials, player data or host database
is mounted. Source capture is component scope, not complete world authority.

## Exact commands and results

```text
PYTHONPATH=tests/async python -m unittest test_reconcile_economy_accounting.ReconciliationTests.test_quarantined_coin_piles_report_without_becoming_active_holdings
python tmp/plan5/prepare-quarantined-coins.py
python3 -u -B tmp/plan5/qualify-quarantined-coins.py
python3 -u -B -m unittest -v test_item_equipment_reconciliation test_economic_sql_uid_scope test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_reconcile_economy_accounting.ReconciliationTests test_reconcile_economy_accounting.AuditBudgetTests test_plan5_child_identity.ChildIdentityTests test_plan5_child_identity.ChildIdentityBudgetTests
python3 -u -B tmp/plan5/quarantined-coin-native.py
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/quarantined-coins-current61 DURIS_REGRESSION_BUILD_CACHE=off python3 -u -B tests/async/test_native_sql_baseline_audit.py
python -B tests/async/test_reconcile_economy_accounting.py ReconciliationTests -v
python -B scripts/validate_economy_accounting.py
python -B scripts/generate_economy_writer_coverage.py --check
python -B scripts/validate_runtime_compatibility.py
python -B scripts/validate_economy_accounting.py --release
python tmp/plan5/reproduce-quarantined-coins-original-sql.py
python tmp/plan5/seal-quarantined-coins.py
git diff --check
```

Frozen Linux image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
Python 3.12.3/GCC 13.3/Make 4.3, MariaDB 10.11.14/MySQL 8.0.46. Windows is
Python 3.12.10. The container uses network none, 2 CPUs/4GiB and separate 2GiB
RAM workspace/tmp/private restore-parent filesystems. Its disposable root is
writable for the original cold-clone layout. There is no live service or DB
contact, and native build caching is disabled. Exact compiler/process commands,
deadlines, source maps, sanitizer results and binary hashes are retained.

| Check | Result |
| --- | --- |
| Original synthetic CLI cases | 18 runs return exit 2 for six valid quarantined projections at limits 0/1/100; inputs unchanged. |
| New method before correction | Expected exit 1, 18 error subtests from the original reader refusal. |
| Linux selected regressions and enabled audit/child budgets | 203 methods PASS in 56.146s, zero skips. |
| Windows complete reconciliation class | 120 methods PASS in 57.875s, zero skips. |
| Repaired pure cases | 18 CLI observations/platform preserve fixed finding counts, ID-only output and bytes. Positive/zero/unknown payload and mapped/unmapped cases retain zero active coin holdings/piles. Unknown state and negative denomination controls refuse. |
| Fresh native SQL/client-free observer | Both native quarantine positions accepted; positive and zero native coin payloads decode to exact denomination vectors; ASan/UBSan pass, no DB contact. |
| Original native baseline recipe on both engines | 1 complete method PASS in 334.591s, zero skips. |
| Original reader against actual new SQL capture files | 12 original CLI runs return exit 2 at limits 0/1/100 across both engines and payload cases, preserving input bytes. |
| Normal accounting, matrix and canonical 61 runtime metadata | Exit 0; coverage/release readiness remain false. |
| Release validator | Expected exit 1: executable writer evidence remains incomplete. |

The native observer compiles the original 14 production units plus a generated
fixture, with the original strict C++20/Werror/O1/debug/ASan/UBSan/frame-pointer/
non-PIE flags. It invokes production `economic_item_effects_validate` and
`player_item_snapshot_list_encode`; the independent Python payload reader
compares their actual bytes. Original fixture support has a byte-verified
inverse after renaming its unused main and adding an explicit return. That
support main is not invoked by this observer, and no cached object is reused.

| Fresh native observer | Compile time | Binary SHA256 |
| --- | --- | --- |
| sql | 40.866 s | `5e0e97b3c4aab20b5af2f41606feaf13a3212bf1247918b9771fd8d3728bde3c` |
| client-free | 36.057 s | `8417d64cecfa885cbde3d36e588f06cfdc06ccf4c2ec2899ac61acbc4f499005` |

## SQL evidence and boundaries

Four new actual SELECT-role captures run in the existing modeled SQL fixture:
quarantined UID 82 with its positive payload, and with no payload, on each
engine. Each capture executes 60 read-only queries, rolls back/closes
once, and preserves all 24 application-table hashes. Twelve repaired
CLI observations retain exact counts and bytes; both readers also deny UPDATE
with error 1142. No active pile balance is invented from the retained payload.

Both SQL cases preserve exactly the fixture's original `evidence_loss:1`,
`missing_item_equipment_evidence:3`, `unmapped_native_wallet:1`,
`unauthorized_mapping_creation:1`, and `missing_original_plan:3`. Quarantine
adds one each of `quarantined_coin_pile`, `dangling_pile_mapping`,
`dangling_coin_pile_mapping`, `missing_native_holding` and `stale_native_item`.
The whole fixture is restored exactly after observations. Its books/current
rows remain explicitly modeled EAB1 diagnostic evidence; they are not promoted
to authentic native monetary admission or complete authority.

The surrounding complete recipe independently preserves two actual native EAB2
books/epochs per engine, 161 damaged-evidence refusals, seven constraint
refusals, ten key-hash cuts, two command-binding cuts and original cold
dump/import/replay. Its fresh original SQL/client-free binaries remain
`72d6d5027b5ed550921efd6b11d14b120dacc1b42d53b6ba32406f96b3f54f2c` and
`dd503bd0c1ba66557ff96a9a2004d5007d60525c5c7259bdb5224ced1072ce0d`.
All previous equipment/topology and supply observations remain in the original
recipe. Native flags, cases, policies and deadlines are preserved.

## Evidence, curator packet and remaining gates

Protected evidence root:
`D:\CodexEvidence\accounting-plan5\bin\quarantined-coins-01-20261006`.
It retains frozen source/maps, original negative controls, all new SQL capture
files, commands, source/binary pins, table inventories and native/restore logs.
The seal verifies 318 artifacts and 3081 code
inputs: `tmp/plan5/quarantined-coins-evidence.json`, SHA256
`ca25a4f1b79509b28545d380b8cc0ad6802e20efbb7b0f430c65ebddf35df1c1`. Publication is bound by
`tmp/plan5/quarantined-coins-delivery.json`. Selected qualification has zero
skips. Two auxiliary shell edit commands had quoting errors; literal patches
corrected those helper edits. The failed commands did not modify maintained
source or qualify a test result.

No independent blocker remains for this slice. No maintained C/C++ change,
full server build or gameplay boot is claimed here. Full authority capture,
real writer/typed command-receipt coverage, remaining mechanics, flatfile
parity, lifecycle/erasure and combined release-host workload budgets remain
open. Synthetic fixtures, inventory and isolated tests do not complete R7/R8
or release. Accounting stays inactive; wallet-root exclusions and the declined
inactive spell path remain. No production mutation, audit autocorrect,
experimental-accounting push, PR merge, activation or deployment occurred.

All work and earlier-branch followups remain on remote `codex/accounting-plan5`.
Primary maintains the notebook locally and the user declared it nonblocking.
This report and sealed delivery receipt provide its curator packet; shared
coordinator/notebook documents are not independently rewritten here.
