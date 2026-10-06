# Plan5 quarantined containment qualification — 2026-10-06

Matching captured openings and current item positions could produce a false
all-clear for a quarantined forest whose child and parent disagreed on owner
identity or root. The topology pass computed each disagreement and propagated
it through ancestors, but emitted `inconsistent_native_topology` only for live
items. Seven exact static synthetic cases returned zero findings and CLI exit0
at detail limits0,1,100. They disagree on owner ID, owner type, owner context,
root, ancestor owner, ancestor root or a destroyed parent. The independent
retained-evidence forest reader and actual native forest validator reject them.

The one-line correction applies that existing finding to both live and
quarantined items. The bounded topology pass, diagnostic schema, mutation
separation, lifetime checks, existing tombstone diagnostics and historical
equipment handling remain. No authority row is changed or automatically
corrected. Valid forests can mix live and quarantined nodes; exact global
counts and CLI status survive detail limit0. Owner aliases are omitted.

## Branch, source and ownership

- Branch/worktree: `codex/accounting-plan5`,
  `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Refreshed primary: `e015beb7fb288c61553b91a1c0f2c0c20accd510`. Normal merge base
  `77222742cd225b11c0cd1a9052ff41b292dc86b7` preserves all seven earlier branch tips and the published
  independent flatfile semantics fix `9e72cd35fc8175fc4f669669aee25419c67481a9`.
- Result: the separate solved-issue commit containing this report. Its protected
  post-publication delivery receipt records the exact result, remote tip, any
  later primary merge and ancestry checks. Publication goes only to
  `origin/codex/accounting-plan5`.
- Native tree `bf7a92a728ad9b5b813626462e56533f8ba39c97` and migration tree
  `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`: canonical61. Earlier0055 evidence does not qualify
  this composition. Primary's private quest/constructor/native12 source remains
  outside maintained native installation and this proof.
- Owned implementation: `scripts/reconcile_economy_accounting.py`.
- Owned tests: `tests/async/test_reconcile_economy_accounting.py` and
  `tests/async/_plan5_equipment_restore.py`.
- Owned documentation: this report and `AUDIT_OPERATIONS.md`. Shared coordinator,
  contracts, producers, migrations, registry/matrix, manifests and activation
  stay primary-owned. No shared interface, schema or registration request.

The existing reconciliation class discovers the new method. Both new SQL
captures execute in the original native baseline method before its mandatory
qualification marker. All918 regression entries and105 integration rows remain
unchanged. Inventory membership does not establish writer or release coverage.

| Exact qualified changed input | SHA256 |
| --- | --- |
| scripts/reconcile_economy_accounting.py | `793333dfa65f447619bc078d83740c4b98837101d39fc7a508fa5b6a54229544` |
| tests/async/_plan5_equipment_restore.py | `fd7f8b274b54f4282b3bfc3333981cb1fe5e32d7e994856c2cfe9d8631d21e0c` |
| tests/async/test_reconcile_economy_accounting.py | `1f1381ee83954d48eba1a800ca7cf58a6a840908c0c217d05dd48ccf93819067` |

The frozen source archive contains3550 regular source/helper files,
no links, and SHA256 `11b33cc28439ad55419607ed19f40c1ab0e4ab0475ad3970298dedfdcd84504a`. All3081 code,
migration and test inputs match the working composition; all archive inputs
are verified before/after execution. No live checkout, environment credentials,
player-data directory or host database is mounted. This captures component
source rather than complete world/runtime authority.

## Commands, backends and results

```text
python tmp/plan5/probe-quarantined-topology.py
PYTHONPATH=tests/async python -m unittest test_reconcile_economy_accounting.ReconciliationTests.test_quarantined_containment_mismatches_cannot_share_a_clean_opening
python tmp/plan5/prepare-quarantined-topology.py
python3 -u -B tmp/plan5/qualify-quarantined-topology.py
python3 -u -B -m unittest -v test_item_equipment_reconciliation test_economic_sql_uid_scope test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_reconcile_economy_accounting.ReconciliationTests test_reconcile_economy_accounting.AuditBudgetTests test_plan5_child_identity.ChildIdentityTests test_plan5_child_identity.ChildIdentityBudgetTests
python3 -u -B tmp/plan5/quarantined-topology-native-grammar.py
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/quarantined-topology-current61 DURIS_REGRESSION_BUILD_CACHE=off python3 -u -B tests/async/test_native_sql_baseline_audit.py
python -B tests/async/test_reconcile_economy_accounting.py ReconciliationTests -v
python -B scripts/validate_economy_accounting.py
python -B scripts/generate_economy_writer_coverage.py --check
python -B scripts/validate_runtime_compatibility.py
python -B scripts/validate_economy_accounting.py --release
python tmp/plan5/seal-quarantined-topology.py
git diff --check
```

Linux runs use pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
Python3.12.3, GCC13.3, Make4.3, MariaDB10.11.14 and MySQL8.0.46. Windows uses
Python3.12.10. Network is disabled, with2 CPUs/4GiB and separate2GiB RAM
workspace, temporary and private restore-parent filesystems. The ephemeral
container root is writable for the original isolated clone layout. There is no
external daemon or live service contact. Exact process/compiler commands,
deadlines, output hashes and container metadata are in the seal.

| Check | Result |
| --- | --- |
| Original actual CLI,10 synthetic static snapshots | Seven false all-clear inputs at each of0/1/100; clean control passes; cycle/orphan controls already refuse. All input bytes unchanged. |
| New method before correction | Expected exit1 with21 failed subtests, exposing the seven omissions at all three limits. |
| Linux selected pure regressions and enabled audit/child budgets |202 methods PASS in54.786s, zero skips. |
| Windows complete reconciliation class |119 methods PASS in56.438s, zero skips. |
| New method after correction |30 actual CLI cases/platform, full counts, bounded detail, ID-only output and unchanged input. Four valid all-live/all-quarantined/mixed forest controls pass. |
| Actual native static forests, fresh SQL/client-free builds |14 cases/context:5 accepted,9 refused, exact expected outcomes, no sanitizer diagnostics or DB contact. |
| Original native baseline method, both canonical engines |1 complete method PASS in332.841s, zero skips. |
| Normal accounting, writer-matrix and canonical61 runtime metadata | Exit0; writer coverage and release readiness remain false. |
| Release validator | Expected exit1: missing executable writer evidence remains refused. |

The native classifier calls production `economic_item_effects_validate` on
identical before/after forests, including quarantined, live, mixed, conflicting,
retired-parent, cycle and orphan cuts. All individual positions use native-valid
identities; the nine refusals exercise containment. The original fixture support
is retained with an exact inverse after renaming its unused main and adding an
explicit return. The original SQL support main is not invoked in that observer.
Original14 production units plus one generated fixture compile with C++20,
Wall/Wextra/Wpedantic/Werror, O1/debug, ASan/UBSan, frame pointers and non-PIE.
No cached native object is reused.

| Fresh native classifier | Compile time | Binary SHA256 |
| --- | --- | --- |
| sql | 39.803 s | `8e39cd9aeb9b0b9c878b4a4326bc1d0ab81ccd6297f14627890107ae14aad581` |
| client-free | 35.047 s | `e1772fe952f6c2fd00832aaee7ea59383546e7e70bc7eb317675ae14fc3eff98` |

## Canonical SQL and restore boundaries

The complete original native baseline fixture runs with fresh SQL and client-free
builds on both private engines. Original native code, flags, cases and deadlines
remain exact. Its SQL binary SHA256 is
`72d6d5027b5ed550921efd6b11d14b120dacc1b42d53b6ba32406f96b3f54f2c`;
client-free binary is
`dd503bd0c1ba66557ff96a9a2004d5007d60525c5c7259bdb5224ced1072ce0d`.
Each engine retains two actual native EAB2 books/epochs,161 independent damaged
evidence refusals, seven constraint refusals, ten key-hash cuts, two command
binding cuts and the original cold dump/import/replay checks.

The owned capture helper retains its five previous phases and adds matching
and mismatching quarantined current positions on each engine. Each new pair
keeps the original native EAB2 books untouched; these positions are modeled
current authority, not an authentic corrupt native opening. Matching quarantine
reports `evidence_loss:1`, `missing_native_holding:2` and `stale_native_item:2`.
The mismatched child adds `inconsistent_native_topology:1` to exactly those
findings. It never replaces the existing partial-capture or stale-state gates.

Across both engines all14 captures execute64 fixed read-only queries each,
roll back and close their cursor exactly once, hash all228 application tables
before/after and retain exact snapshot bytes at limits0/1/100:42 actual CLI
observations. Four original cold equipment dump/import phases retain exact
application-data hashes, immutable native replay and SELECT-role denial1142.
The original ten supply probes/six capture transactions/30 CLI checks also pass
with their modeled-root scope and unchanged native books. Private fixture
mutations are bounded to disposable current rows and cleaned back to the exact
initial fixture. Audit code never mutates authority.

## Evidence, curator packet and remaining gates

Protected artifacts:
`D:\CodexEvidence\accounting-plan5\bin\quarantined-topology-01-20261006`.
Its original-probe directory retains all10 inputs,30 original CLI outputs and
the expected failing regression. Final logs, source archive, source manifest,
native binaries, generated observer and canonical database/cold-restore
observations are retained. The seal contains293 immutable
artifact hashes and3081 exact code-input hashes:
`tmp/plan5/quarantined-topology-evidence.json`, SHA256 `ec76308e9214152b221d9e9225c114ef8ccd1544ceefa807906a41703cc1d758`.
The post-publication receipt is `tmp/plan5/quarantined-topology-delivery.json`.
Qualification selected zero skips. An auxiliary version inventory initially
used a nonexistent guessed MySQL executable path; the corrected PATH-resolved
inventory identifies `/usr/local/bin/mysqld` and8.0.46. This did not alter or
qualify a native test result.

This narrow slice has no independent blocker and makes no full Plan/release
claim. No full server build or gameplay boot was rerun here: no maintained
C/C++ input changed. Full native source/tree pins remain exact, and prior full
build/operational records retain their original scopes. New flatfile restore
semantics remain separately qualified by their exact report. Complete authority
capture, real command/typed receipt and writer coverage, remaining mechanics,
flatfile custody parity, recovery/publication/erasure and final combined
qualification/latency/storage budgets remain open. Synthetic controls, inventory
coverage and isolated passing tests do not establish release completion.

All work and earlier-branch followups remain on remote `codex/accounting-plan5`.
Primary maintains the shared notebook locally and the user declared it
nonblocking. This owned report and sealed delivery receipt form the curator
packet for that notebook; shared coordinator documents are not independently
rewritten. Accounting stays inactive, wallet-root item exclusions and the
declined inactive spell-path change remain. No experimental-accounting push,
activation, PR merge, deployment, production-data mutation or audit autocorrect.
