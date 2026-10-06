# Plan5 compound item action qualification — 2026-10-06

The SQL exporter classified every ownership event solely by its generic reason:
reason 2 was creation, reason 3 destruction, and everything else movement.
Actual compound item producers retain their domain reason. Craft ledgers keep
reason 34 for both consumed inputs and created outputs; quest retirement keeps
reason 33, and collector expiry reason 21. Native craft outputs have a system
source and UID revision 1, while retirements end at the destruction owner.
These events were exported as moves in all four ownership projections.
Craft creation could therefore lose its committed revision 0 origin and produce
false unknown/unanchored-origin findings; provenance hid its supply action.

One independent projection helper now keeps explicit generic creation and
destruction reasons, preserving existing diagnostics for contradictory rows.
For other reasons it uses the retained destruction endpoint, or system-source
revision 1 creation, and otherwise reports movement. All four SELECT projections
use the same rule: selected events, linked UID references, lineage history and
unattributed history. The three narrower queries now also select the existing
source-owner type. Retained domain reasons and authority remain unchanged.
Creation-origin inference still requires a committed revision 0-to1 creation;
rejected and unknown outcomes cannot establish an origin.

## Source, branch and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Branch/publication: `codex/accounting-plan5`. Earlier branch work and its
  follow-ups remain reachable there; there is no independent push to
  `experimental-accounting`.
- Exact tested base `62ad380b60d0f10a1e610dd396ad56ee10a81a72`, containing primary
  `92ca96795ccb0da64b56e1f3a367b496f0178a58`. The solved-issue commit containing this report is bound
  to its exact remote result by the delivery receipt.
- Native tree `bf7a92a728ad9b5b813626462e56533f8ba39c97`; migrations
  `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61. Earlier 0055 evidence does not
  qualify this composition. Primary's private ongoing producer/mechanics work
  is outside the maintained native source qualified here.
- Owned source: `scripts/economic_sql_audit_snapshot.py`.
- Owned tests: `tests/async/test_economic_sql_uid_scope.py` and
  `tests/async/run_economic_sql_audit_snapshot_mysql.py`.
- Owned documentation: this report and `AUDIT_OPERATIONS.md`.
- Shared fields/schema/interface requests: none for this action fix. The rule
  consumes existing reason, source/destination owner type and UID revision.
  Producers, contracts, registry/matrix, manifests, coordinator and activation
  owner are unchanged.

| Exact changed input | SHA256 |
| --- | --- |
| scripts/economic_sql_audit_snapshot.py | `c2ae23f568046bbb525e59b9af7cddd662fa404bffc04c2800bcff95eceba51c` |
| tests/async/run_economic_sql_audit_snapshot_mysql.py | `a0cd35e9916315d971e552f5b723baf368a82b55cf0461e6983f70127808007f` |
| tests/async/test_economic_sql_uid_scope.py | `a342cee7fed6e125bac55297d119e7e71822a7bfd0b31a154f9572c593ef9520` |

The successful recovery archive SHA256 is `2652ba730a28fa1a70ab3995c3ea9fa3f1cbd60e30c4b55b4e094c8baaf65175`;
the initial archive was `ba984b7810dffc262569dd2160a84bf106ca28c97b4e34305a5c8ee4c23ca779`. The final archive
pins all 3081 maintained code/migration/test inputs. Exporter,
Python test subjects/dependencies and native factory inputs are identical across
attempts. The only changed maintained input since the initial passing Python
and native observer checks is the added disposable SQL fixture driver; its
complete final recipe is rerun. Ignored observer/orchestration helpers also differ.
There are 3554 regular source/helper files, no links, no live checkout
mount and no credentials, player data or host database. Archive members and final
maintained inputs are checked byte-for-byte. The 918 regression inventory entries
and 105 integration rows remain exact; inventory is not executable coverage.

## Commands and observations

```text
python -B tests/async/test_economic_sql_uid_scope.py UidScopeTests.test_compound_item_actions_follow_retained_supply_endpoints -v
python -B tests/async/test_economic_sql_uid_scope.py -v
python -B tmp/plan5/prepare-compound-actions.py
python -B tmp/plan5/recover-compound-actions.py
python -B tmp/plan5/recover-compound-actions-sql.py
python3 -u -B -m unittest -v test_item_equipment_reconciliation test_economic_sql_uid_scope test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_reconcile_economy_accounting.ReconciliationTests test_reconcile_economy_accounting.AuditBudgetTests test_plan5_child_identity.ChildIdentityTests test_plan5_child_identity.ChildIdentityBudgetTests
python3 -u -B tmp/plan5/compound-actions-native.py
python3 -u -B tmp/plan5/qualify-compound-actions-native.py
python -B -m unittest -v test_economic_sql_uid_scope test_reconcile_economy_accounting.ReconciliationTests
python -B scripts/validate_economy_accounting.py
python -B scripts/generate_economy_writer_coverage.py --check
python -B scripts/validate_runtime_compatibility.py
python -B scripts/validate_economy_accounting.py --release
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/compound-actions-current61 DURIS_REGRESSION_BUILD_CACHE=off python3 -u -B tests/async/test_native_sql_baseline_audit.py
python -B tmp/plan5/seal-compound-actions.py
git diff --check
```

Pinned Linux image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`
uses Python 3.12.3/GCC 13.3/Make 4.3, MariaDB 10.11.14 and MySQL 8.0.46.
Windows uses Python 3.12.10. Each container has network none, 2 CPUs/4 GiB,
RAM workspace/tmp/private cold-restore parent and a disposable writable root.
Only protected evidence and the read-only loader are host mounts; there is no
contact with a live database. Original native caching remains disabled and
recipe flags, cases, markers and deadlines are unchanged.

Linux selected checks inherit `PYTHONPATH=/workspace/tests/async`,
`PYTHONDONTWRITEBYTECODE=1`, `DURIS_RUN_AUDIT_BUDGET=1`,
`DURIS_PLAN5_CHILD_IDENTITY_BUDGET=1`,
`DURIS_PLAN5_CHILD_IDENTITY_BUDGET_ARTIFACTS=/workspace/bin/tests/compound-actions-child-budget`
and `DURIS_REGRESSION_BUILD_CACHE=off`. Windows checks inherit
`PYTHONDONTWRITEBYTECODE=1` and `PYTHONPATH=tests/async`. The preserved execution
and source-transport helpers record the actual environment and Docker arguments.

| Check | Result |
| --- | --- |
| Focused regression before fix | Expected failure: ten subcases. Five reveal wrong compound supply actions, five require the missing retained source field in every narrow SELECT. |
| Focused repaired exporter class | 12 methods PASS, zero skips. |
| Linux selected regressions and enabled audit/child budgets | 205 methods PASS in 66.768s, zero skips. |
| Windows exporter/reconciliation classes | 133 methods PASS in 73.172s, zero skips. |
| Native craft observer | Fresh SQL/client-free C++20 builds, all original fixture functions and flags preserved, ASan/UBSan: exit 0 in 73.946s. Four authentic factory events/context now project two destroys and two creates while retaining reason 34. |
| Complete original native baseline/restore recipe | Both-engine method PASS in 385.973s, zero skips, fresh original SQL/client-free binaries. |
| Added SELECT-role compound captures | Four phases/engine, eight captures and 24 actual provenance CLI runs at limits 0/1/100. Correct actions, committed absent origins, original partial findings and immutable inputs remain. |
| Normal accounting, matrix, runtime metadata | Exit0; coverage/release remain incomplete. |
| Release validator | Expected exit 1: executable writer evidence is incomplete. |

The focused test exercises the real four reader functions through modeled
query rows for craft creation/retirement, corpse creation, quest/collector
retirement, ordinary moves, system moves at revision2, and contradictory generic
creation/destruction reasons. Every query is SELECT-only; source rows remain
unchanged and personal aliases are absent from output. Each supported creation
projection also checks committed-only origin inference and refusal for rejected
or unknown outcomes. Generic reason compatibility is intentionally retained so
existing original-plan/custody mismatch findings cannot be hidden by an endpoint
rewrite.

## Native and disposable SQL scope

The native observer adds four print records to the original craft factory test
and changes its entrypoint only to invoke the same original main. An exact
inverse proves every original assertion/support byte remains, and all original
fixture functions execute. Both builds use the original production units and
strict sanitizer flags with zero reused objects. These are authentic native
factory EAP1 item effects; the legacy-ledger shape is projected using the actual
`insert_craft_ledger` source rule, which maps an absent source to system owner 7.
The observer does not execute a SQL craft writer or establish a gameplay journey.

The added SQL phases use the existing explicitly guarded private modeled schema
and SELECT-only reader role. They modify only that fixture to reproduce retained
craft creation and craft/quest/collector retirement reason/endpoint pairs.
Each capture rolls back and closes once, and full hashes of all 24 application
tables remain equal before/after the reader. All phases recover UID 84's creation
origin and preserve the fixture's original global partial-evidence findings.
The modeled retirement has its own item-only root using the existing
item-destruction reason and item-action source. It adds exactly one
`missing_original_plan` finding while preserving every original finding; no
model-authored plan is supplied to clear it. The new rows are removed and the
initial fixture/captured snapshot are restored exactly afterward. This is not
a native craft/quest/collector root or receipt.
This fixture is explicitly `sql_partial`, `complete: false`, with no native
compound gameplay or admission claim. Its modeled legacy rows are not authentic
command receipts or full producer proof.

The surrounding original method retains two authentic EAB2 books/epochs per
engine, 161 damaged-evidence refusals, seven constraint refusals, ten key-hash
cuts, two command-binding cuts and cold dump/import/replay. It also preserves
the original 14 equipment/topology captures, four quarantined-coin captures,
26 supply probes and their existing bounded CLI checks, read-only transaction
checks and both UPDATE denials with 1142. The freshly compiled original baseline
SQL/client-free binaries remain
`72d6d5027b5ed550921efd6b11d14b120dacc1b42d53b6ba32406f96b3f54f2c` and
`dd503bd0c1ba66557ff96a9a2004d5007d60525c5c7259bdb5224ced1072ce0d`.
There is no maintained C/C++ edit, new full server rebuild or gameplay boot for
this Python fix; the native source matches the prior full maintained build.

## Evidence, attempts and remaining gates

Protected evidence roots:
`D:\CodexEvidence\accounting-plan5\bin\compound-actions-01-20261006`,
`compound-actions-02-20261006`, `compound-actions-03-20261006`,
`compound-actions-04-20261006`, `compound-actions-05-20261006`,
`compound-actions-06-20261006` and `compound-actions-07-20261006` in the same
parent. The first contains successful Python checks, original negative controls
and the observer failure. The second retains a failed recovery preparation.
The third contains the passing corrected observer and failed first SQL recipe.
The fourth retains a failed orchestration tuple. The fifth retains the failed
money-effect/root-policy fixture. The sixth retains the duplicate-source fixture
failure. The seventh contains the complete corrected both-engine qualification,
actual captures, table inventories,
native/compiler commands and restore logs.
The seal checks 566 artifacts and 3081 maintained
inputs: `tmp/plan5/compound-actions-evidence.json`,
SHA256 `57bd4a0c0b53e04d8cb940adde640cb4f5407439635057da7b882b4c177e42bf`. Remote publication is bound by
`tmp/plan5/compound-actions-delivery.json`.

The initial modeled cursor fixture omitted three existing aggregate bounds and
failed with ten errors before testing the defect. It was corrected before the
recorded ten-failure negative control. The first native observer print loop then
failed compilation under `-Werror=misleading-indentation`; braces and indentation
were corrected only in the ignored helper. The first recovery archive helper
wrongly required directory entries to be regular files and stopped before any
container launch. The first SQL fixture attached a retirement to a coin-transfer
root and correctly triggered the unrelated coin-lifecycle-source finding; the
fixture now uses the existing item-destruction root reason and restores it.
The SQL retry orchestration then omitted a one-element tuple comma and stopped
before tests; that ignored helper is corrected. The reused root's money effects
also correctly triggered two counterparty-policy findings under item-destruction
policy. Retirement now has a separate item-only modeled root; the exact assertion
retains all original findings plus its missing-plan finding. Its first source ID
duplicated the existing creation ID and correctly produced duplicate-source and
claim findings. The ID is corrected and preconditions now require both new root
and source identities to be absent before setup. All attempts remain preserved.
Recovery proves passing Python subjects/dependencies and native observer inputs
are unchanged, so those results retain their exact source attribution without
an unnecessary repeat. The corrected complete native/SQL recipe passes;
there are zero selected qualification skips and no independent slice blocker.

This action projection does not infer a custody state missing from a legacy
ledger or establish complete receipt authentication. Full authority capture,
real writer/typed command receipts, remaining mechanics and flatfile parity,
lifecycle/erasure and combined release-host workload/storage budgets remain open.
Native inventory, modeled captures and isolated passing tests do not complete
R7/R8 or release. Accounting stays inactive; wallet-root exclusions and the
declined inactive spell path remain. No audit correction, production mutation,
activation, deployment or PR merge occurred.

Primary maintains the notebook locally; the user declared it nonblocking. This
owned report and the sealed delivery receipt are its curator packet. Shared
notebook/coordinator files are not independently rewritten here.
