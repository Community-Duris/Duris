# Plan 5 historical item-position validation — 2026-10-08

The independent reconciler returned zero exceptions for an impossible
intermediate custody position when the retained opening and final positions were
valid. In the unchanged-reader reproduction, UID81 revision4 had root99 with no
parent, then revision5 returned to root81. Both selected and lineage checks and
the full CLI at limits0/1/100 accepted that history unchanged. The existing
independent position grammar and published native `item_position_valid` both
reject the intermediate state. This slice closes that false-clean path.

## Branch, owned files and interface

Sole local/remote branch `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `194e31e7ce57d7217c9a001e1964f1aa4b0435db`. The containing commit is this
issue's result; post-push `delivery/result.json` records its exact SHA and the
verified remote. All seven previously consolidated branch tips remain ancestors.
There is no branch/worktree switch, history rewrite or primary-branch push.

Owned files:

- `scripts/reconcile_economy_accounting.py`
- `tests/async/test_item_equipment_reconciliation.py`
- `tests/async/run_economic_sql_audit_snapshot_mysql.py`
- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`
- This report and additive `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.

The reader reuses its existing independent native position grammar on every
selected, lineage and unattributed event before any missing-origin skip. It
uses the event's `to_equipment_slot`, never a stale `equipment_slot`. Missing
historical equipment remains unknown; malformed present resulting slots retain
the position finding and, where the existing anchored transition check applies,
the existing slot/missing-evidence findings.
`invalid_item_history_position` contains only UID and operation ID and is counted
once per legacy `(operation_id,event_index,uid)` across projections. All other
continuity, original-plan, coverage and unattributed findings remain independent.
No mutation or repair follows from any finding.

No shared interface or schema change is requested. Existing event UID, revision,
root, parent, owner, state and resulting-slot fields are sufficient. The new
exception name is consumed by the independent report/operator views; five new
pure methods, 21 full CLI calls and both-engine SQL controls cover it. Shared
native/coordinator/producers/registry/matrix/activation and original native
recipes/harnesses remain unchanged. Wallet-root ITEM_MONEY exclusions, inactive
behavior and the declined inactive spell change are preserved. Scope is recorded
position grammar, not reconstruction of an absent historical multi-UID forest.

## Exact tested source and commands

| Source | Identity |
| --- | --- |
| Refreshed published primary at freeze | `e8d03ff50fe3ace010b56a27c1fc4c2a840511f2` |
| Primary plus 24 exact owned overlays | `3268bd291451c2d3ae563321ef6ce0bc617e4a0e` |
| Archive SHA256 | `3e36a3b9adbfdc3fbe5437fee025992ba6bb0d5a11376e3855be143bfa300b94` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Canonical schema head | 64 / 0064_auction_custody_history |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

All three stages execute the same immutable composition. Transport authentication
checks 6512 Git blobs, 6508 regular bodies/modes and four link targets. Guards
verify all before/after source bodies, modes and links; all 24 overlay blobs are
rechecked against the published result. Local shared `src` is the older tree
`4abb609524a1f1682ea4c190f82d75003c4d679b` and is not the native test base.
Required AGENTS, README, Plan5, finish plan, remaining requirements and checkpoint
are retained as raw tested/refreshed bytes. Published AI_CONTEXT.md remains
absent; the user-confirmed primary-local shared notebook is nonblocking.

Host commands run with explicit workdir above, D: temp/cache/build/evidence roots:

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-history-position/reproduce.py
python -X utf8 D:/Dev/Temp/accounting-plan5-history-position/freeze.py 01
python -X utf8 D:/Dev/Temp/accounting-plan5-history-position/authenticate.py
python -X utf8 D:/Dev/Temp/accounting-plan5-history-position/launch.py checks01 01
python -X utf8 D:/Dev/Temp/accounting-plan5-history-position/launch.py canonical01 01
python -X utf8 D:/Dev/Temp/accounting-plan5-history-position/launch_budget.py historybudget01 01
python -X utf8 D:/Dev/Temp/accounting-plan5-history-position/qualify.py
python -X utf8 D:/Dev/Temp/accounting-plan5-history-position/seal.py
```

Containers have no network/ports, private RAM source/tmp, direct D: build binds,
2 CPUs and 5GiB memory. Reader/budget roots are read-only. The unchanged original
canonical test requires `TemporaryDirectory(dir='/')`, so its disposable root is
writable. The external observer records original argv/timeouts/exits, preserves
native lstat metadata before selected regular-only copies and does not alter
compiler arguments, fixtures, assertions or native recipes. Physical database
bodies, private keys and service runtime trees are excluded from Windows copies.
Existing services/volumes and production data are untouched.

## Executed results and preserved failures

| Check | Result |
| --- | --- |
| Corrected RED against unchanged reader | Five new methods, 18 assertion failures, zero errors |
| Complete equipment module after fix | 13 PASS, zero skips |
| Complete 16-module reader suite | 388 loaded, 370 PASS, 18 original opt-in skips |
| Complete original manual SQL snapshot runner | PASS on both fresh actual private engines |
| New history-position controls per engine | 10 cuts: two healthy, eight invalid; 60 actual CLI checks |
| Complete original canonical coin/restore test | PASS, zero errors/failures/skips; both native modes and both engines |
| Original synthetic mapping/price budgets | All 12 actual CLI workloads PASS |
| New near-32MiB history budgets | Six actual CLI workloads PASS, limits0/1/100, 100000 events each |
| Snapshot stage | 185.713007s, 844 recorded run commands, 847 launches |
| Canonical stage | 340.659080s, 58 recorded run commands, 60 launches, 18 successful compiler commands |
| New history-budget stage | 9.863916s, six recorded run commands/launches |

The 16 complete modules cover reconciler, UID scope, origins, equipment,
canonical audit, child identity, ship/guild and eight SQL custody readers.
The 18 original opt-in skips are native stake1, origins3, canonical4, child2
and each custody module1. Exact names/reasons remain in `qualification.json`;
this slice does not claim those skipped methods ran through another test.

Actual engines are MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and
MySQL8.0.46-0ubuntu0.22.04.4. The complete manual runner preserves every original
control. Added cuts exercise wrong intermediate root, self-parent, missing owner
ID, invalid shop/pet context, native-mobile slot44, known unowned custody,
selected/lineage overlap and orphan unattributed history. Both exception and
provenance CLI views run at limits0/1/100. SELECT-only captures check rollback and
cursor close; all47 application tables are hashed before/after each audit. The
setup restores its exact baseline and re-captures it. These are modeled partial
SQL projections, not authentic producer/admission or complete-opening evidence.

The complete original native/canonical test runs unchanged SQL and client-free
ASan/UBSan recipes. The independent decoder agrees on3026 native cases,1054
accepted. Each actual canonical engine passes109 cuts,58 full-entry cuts and90
expected refusals, with authority unchanged. No new C/C++ is changed; a full
maintained Make rebuild is not required or claimed for this Python slice.

The new budget workload preserves either100000 valid positions or99999 invalid
intermediate positions followed by a valid final one. Each minified input is
33,553,408 bytes; all six hashes remain unchanged. Exact global totals stay zero
or99999 at all three display limits. Maximum elapsed1.322869s, peak186,015,744
bytes, within original30s/256MiB component bounds. Original mapping/price maxima
are1.118260s and158,179,328 bytes. Padding and histories are synthetic; these
measure this component/host and do not establish real-root release throughput.

Initial RED had one assertion KeyError, corrected before the retained18-failure
RED. The first full equipment run then exposed12 expected-result failures for
malformed present `to_equipment_slot`: the new position finding accompanies the
original invalid-slot/missing-evidence findings. Only those expectations were
extended; their prior assertions remain. All failure logs are preserved, and no
failed run is relabelled green. All three frozen native/container stages reach
terminal exit0; no observation timeout restart or source substitution occurs.

## Evidence, curator input and remaining gates

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/history-position-20261008`.
Builds: `D:/Dev/Builds/Duris/accounting-plan5-history-position-20261008`.
Helpers: `D:/Dev/Temp/accounting-plan5-history-position`.
Key records: unchanged-reader reproduction/snapshot, corrected RED and initial
equipment failure, `source01.json`, transport authentication, each terminal,
commands/launches, SQL snapshots/views/queries/authority inventories,
`qualification.json`, `seal/evidence.json` and post-push `delivery/result.json`.

Raw seal SHA256 `a687aa0723a3ebd9cf38dec623d3474cdc3f3501e5a1c9518a219e0e46991b90`:
1008 retained regular files,497,166,930 bytes,803 native build files; zero copied
links/reparse points. All four containers are stopped with OOM=false and no live
recorded children. Native lstat precedes regular-body Windows rehash. Post-push
delivery rechecks every sealed file, all24 overlay blobs, all seven ancestors,
clean worktree and exact remote result. Report and additive follow-up are
curator-ready notebook input; application or acknowledgement is not claimed.

Primary refresh `f679ee312baccbe077267aedd36fceaa2f97b14f` changes six documentation
files only; actual test inputs/native/migrations stay exact. Its latest private
140-file live-flat-shop candidate has SHA256
`87c253f48e89a1b3656c787c7a630331da35a3c43fdd549bc099b6e1a0d7265b`.
Compiler/native/gameplay/SQL/persistence/recovery remain UNEXECUTED for that
private candidate. This slice qualifies published source plus owned overlays,
not the private candidate or a primary-published combined release.

Shared handoffs remain open: original flat authority recipe/provider closure
(auction decoder/publication dependencies), original room union UID-owner
collision, and original SQL-baseline missing providers/head62 pin. Those require
primary changes to original shared recipes/harnesses. Complete12-case backup
module, combined native candidate, full genuine producers/opening/treasury/player
journeys, fault/load/route coverage, cold registration/admission and Plan5/R1–R8
release acceptance remain unqualified. Inventory and isolated passing checks
cannot close them. No accounting activation, automatic correction, production
access, primary push, deployment or PR merge occurred.
