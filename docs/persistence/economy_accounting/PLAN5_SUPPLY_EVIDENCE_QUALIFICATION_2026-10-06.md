# Plan5 unique supply evidence qualification — 2026-10-06

The independent supply view summed every raw posting and selected an account
kind from the last captured effect at an operation/account index. Repeating a
three-copper sink line displayed six copper; a conflicting six-copper line
displayed nine. Conflicting effect rows could switch the displayed account kind
between issuance and sink when export order changed. The reconciler already
reported duplicate identities, but those ambiguous rows still affected totals.

Supply now requires exactly one committed root, one posting at its
operation/line identity, and one effect at the referenced operation/account
index. Duplicate identities, including exact repeats, contribute no aggregate
amount. Distinct posting lines can legitimately aggregate against one account.
All original global findings and CLI status remain; unaffected committed
evidence stays visible, and displayed totals do not certify an audited root.
This changes only the independent read-only projection, with no mutation,
automatic adjustment, schema field or shared interface change.

## Source, branch and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Branch/publication: `codex/accounting-plan5`; all earlier branch work and
  follow-ups remain reachable there. No push to `experimental-accounting`.
- Exact tested base `9d326b6bb70b34503dc2398b566c6ede844695e1`, containing primary
  `92ca96795ccb0da64b56e1f3a367b496f0178a58`. The separate solved-issue commit containing this report
  is bound to its exact remote result by the delivery receipt.
- Native tree `bf7a92a728ad9b5b813626462e56533f8ba39c97`; migrations
  `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61. Earlier 0055 evidence does not
  qualify this composition. Primary's private ongoing birth/quest/native12
  work remains outside the maintained native source qualified here.
- Owned code: `scripts/reconcile_economy_accounting.py`.
- Owned regressions: `tests/async/test_reconcile_economy_accounting.py` and
  `tests/async/run_economic_sql_audit_snapshot_mysql.py`.
- Owned documentation: this report and `AUDIT_OPERATIONS.md`.
- Shared schema/interface/registration requests: none. Coordinator, producers,
  contracts, registry/matrix, manifests and activation owner remain untouched.

| Exact changed input | SHA256 |
| --- | --- |
| scripts/reconcile_economy_accounting.py | `16577ce28ee1289e001a45afd95ea2ff2b21e8d0ad5f5d66fe80eb514784a81f` |
| tests/async/run_economic_sql_audit_snapshot_mysql.py | `7a50d3bc94d4edba41051d49b54ca8ca1bb56db4f690a8470a8ecb2d63c6500e` |
| tests/async/test_reconcile_economy_accounting.py | `e7edad15cc102696e809871c32825b010bc7f1346ebc167ad93e104899b83fb6` |

Frozen source SHA256 `8ba55f8e3ed5867665af42be8de6522d98b3241e859a2a7070e6c40893e76355` contains 3552 regular
source/helper files with no links. All 3081 code/migration/test
inputs match the final result. Source transport is verified before and after
execution; no live checkout, credentials, player data or host database is mounted.
The 918 regression inventory entries and 105 integration rows remain exact;
inventory counts are not executable release coverage.

## Commands and results

```text
python tmp/plan5/probe-supply-evidence.py
PYTHONPATH=tests/async python -B -m unittest test_reconcile_economy_accounting.ReconciliationTests.test_supply_requires_unique_effect_and_posting_identities -v
python tmp/plan5/prepare-supply-evidence.py
python3 -u -B -m unittest -v test_item_equipment_reconciliation test_economic_sql_uid_scope test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_reconcile_economy_accounting.ReconciliationTests test_reconcile_economy_accounting.AuditBudgetTests test_plan5_child_identity.ChildIdentityTests test_plan5_child_identity.ChildIdentityBudgetTests
python3 -u -B tmp/plan5/qualify-supply-evidence-native.py
python -B tests/async/test_reconcile_economy_accounting.py ReconciliationTests -v
python -B scripts/validate_economy_accounting.py
python -B scripts/generate_economy_writer_coverage.py --check
python -B scripts/validate_runtime_compatibility.py
python -B scripts/validate_economy_accounting.py --release
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/supply-evidence-current61 DURIS_REGRESSION_BUILD_CACHE=off python3 -u -B tests/async/test_native_sql_baseline_audit.py
python tmp/plan5/reproduce-supply-evidence-original-sql.py
docker exec plan5-supply-evidence-01-20261006 python3 -u -B /evidence/measure-supply-evidence-budget.py
python tmp/plan5/seal-supply-evidence.py
git diff --check
```

Linux image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`
uses Python 3.12.3/GCC 13.3/Make 4.3, MariaDB 10.11.14 and MySQL 8.0.46.
Windows uses Python 3.12.10. The isolated container has network none,
2 CPUs/4GiB, RAM workspace/tmp/private restore-parent filesystems and a writable
disposable root for the original cold-clone layout. Native caching is disabled.
Original command deadlines, flags, cases and markers are preserved in raw logs.

| Check | Result |
| --- | --- |
| Original synthetic CLI probe | 24 exit-1 runs reproduce incorrect aggregates/order selection; duplicate findings remain, inputs unchanged. |
| New regression before fix | Expected failure: 96 subcases display ambiguous supply. |
| Linux selected regressions and enabled audit/child budgets | 204 methods PASS in 65.068s, zero skips. |
| Windows reconciliation class | 121 methods PASS in 68.563s, zero skips. |
| New corrected CLI regression | 96 observations/platform cover four system kinds, exact/conflicting effect/line duplicates, both export orders and limits 0/1/100. Output stays bounded and ID-only; inputs are immutable. |
| Valid repeated-account control | Three distinct balanced lines, with retained original EAP1 and reversed export order, remain clean and total three copper. |
| Supply CLI component budget | Six 32 MiB runs over 500 modeled roots/32,000 postings, including a duplicate-line case, pass at limits 0/1/100: maximum 0.751s and 111620096 peak bytes, within 30s/256MiB. |
| Original native baseline recipe | Complete both-engine method PASS in 352.246s, zero skips; fresh original SQL/client-free builds and ASan/UBSan remain. |
| SQL-derived saved projection probes | Eight added damaged projections/engine, 48 repaired CLI runs, no ambiguous supply. |
| Original reader on the same SQL-derived files | 48 exit-1 runs reproduce the incorrect aggregate with identical global coverage and unchanged bytes. |
| Normal accounting, matrix, runtime metadata | Exit 0; release readiness remains false. |
| Release validator | Expected exit 1: executable writer evidence is incomplete. |

The pure fixtures author synthetic EAP1 before deliberately damaging projected
identities. Only the original complete sink control establishes a clean modeled
policy root; the all-system-kind cases exercise projection behavior while
retaining their policy findings. They are not authentic gameplay admission.

## Disposable SQL and native restore evidence

The existing modeled SQL supply fixture retains its three SELECT-only captures
per engine and original committed/rejected/unknown/duplicate-root probes. Eight
additional saved projections per engine repeat or conflict the system effect
or posting line in both export orders. SQL primary keys cannot contain those
duplicates: corruption is applied only to captured JSON. There is no new SQL
authority mutation or promotion to complete/native admitted system evidence.
All 13 probes per engine run at three detail limits: 78 CLI observations total,
including the original 30. Global finding counts and exit status remain, while
duplicate identities contribute no supply rows. Table inventories prove the
SELECT captures leave authority unchanged, and the original fixture is restored
exactly after its existing modeled outcome changes.

The complete surrounding recipe retains two authentic native EAB2 books/epochs
per engine, 161 damaged-evidence refusals, seven constraint refusals, ten key-hash
cuts, two command-binding cuts and cold dump/import/replay. It also retains
the previous 14 equipment/topology captures and four quarantined-coin captures,
including SELECT-only rollback/close checks and both UPDATE denials with error
1142. Those are component observations, not a full active gameplay journey.

The original SQL/client-free native binaries remain
`72d6d5027b5ed550921efd6b11d14b120dacc1b42d53b6ba32406f96b3f54f2c` and
`dd503bd0c1ba66557ff96a9a2004d5007d60525c5c7259bdb5224ced1072ce0d`.
No maintained C/C++ change, full server rebuild or gameplay boot is claimed for
this Python view fix. Native source matches the prior maintained build evidence.

## Evidence and remaining gates

Protected evidence root:
`D:\CodexEvidence\accounting-plan5\bin\supply-evidence-01-20261006`.
Frozen inputs, original negative controls, actual captures and damaged saved
projections, table inventories, process/compiler commands and native/restore
logs are retained. The seal verifies 438 artifacts and
3081 code inputs:
`tmp/plan5/supply-evidence-evidence.json`, SHA256 `b78bb98d3625d71bccb787a6296a571f8fc755287939b198ba53cfcd4b91d19e`.
Publication is bound by `tmp/plan5/supply-evidence-delivery.json`.
Selected qualification has zero skips and no independent slice blocker.
The first auxiliary budget helper assumed the existing fixture was below the
byte limit; it was already exactly 32 MiB through its test padding field.
That preparation assertion failed before invoking the CLI. The retained helper
was corrected to allow the exact bound and reduce only non-authority padding
before adding the duplicate. Both versions are preserved; all six actual CLI
budget observations then passed. This is synthetic component evidence, not a
release-host budget result.

Full authority capture, real writer/typed command receipt coverage, remaining
mechanics and flatfile parity, lifecycle/erasure and combined release-host
workload budgets remain open. Passing these synthetic/component checks does
not complete R7/R8 or release. Accounting stays inactive; wallet-root exclusions
and the declined inactive spell path remain. No audit auto-correction,
production mutation, activation, deployment or PR merge occurred.

Primary maintains the notebook locally; the user declared it nonblocking.
This owned report and sealed delivery receipt are its curator packet. Shared
coordinator/notebook files are not independently rewritten here.
