# Plan 5 previous-owner provenance — 2026-10-08

The UID provenance view omitted captured previous owners. Its exact-projection
deduplication therefore also collapsed conflicting previous-owner evidence.
The unchanged reader already detected the bad owner history; the defect was
loss of investigation detail in the operator view, not a false-clean audit.
Three captured projections for UID81 became two rows, with neither conflicting
`from_owner` value visible. The fix preserves that optional existing field through
the existing strict validator. Exact projections still count once; differing
known previous owners and known versus absent evidence remain distinct.

## Branch, files and interface

Sole local/remote branch `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `6516e214dbb32b9f51fbf9257855fc0f6f918983`. The containing commit is this issue's result;
`delivery/result.json` records its exact SHA and verified remote after push.
All seven previously consolidated tips remain ancestors. No branch switch,
history rewrite or primary-branch push occurs.

Owned files are `scripts/reconcile_economy_accounting.py`,
`tests/async/test_item_equipment_reconciliation.py`,
`tests/async/run_economic_sql_audit_snapshot_mysql.py`,
`docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`, this report and additive
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. `change-scope.json` authenticates the sole
production-function change (`view`), the existing SQL assertion function and the
four added test methods. Production changes by one line; no new helper or
validation implementation is added.

No shared interface/schema/native change is requested. The owned provenance
output now includes existing optional `from_owner: [kind, id, context]` for a
captured event. The list has three exact integers, kind0–12 and uint64 ID/context;
absent evidence stays omitted, known `[7, 0, 0]` stays explicit and present malformed
values refuse through `item_previous_owner`. This validates representation; it
does not authenticate producer authority or infer a missing owner. The consumers
are `--view provenance --uid UID` and the existing operator/SQL assertions.
The destination `owner` and equipment transitions remain visible. Public IDs
only, bounded counts, global coverage/refusal and input immutability remain.

Shared coordinator/contracts/producers/registry/matrix/activation, migrations
and original native recipes/harnesses are untouched. Inactive behavior,
wallet-root ITEM_MONEY exclusions and the declined inactive spell-path change
are preserved. Findings never trigger adjustment or repair.

## Exact source, runtime and commands

| Input | Identity |
| --- | --- |
| Refreshed primary at freeze | `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` |
| Primary plus 24 owned overlays | `ec654e84c20a2ca4cdcae38081ab97770096b9bb` |
| Archive SHA256 | `0943b8a3c9bb93542ad4ddead6047cd7ef8bbf25f7cf365acb968b1aca958ef2` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migrations | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Canonical schema head | 64 / 0064_auction_custody_history |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

Transport authentication verifies 6, 512 Git blobs, 6, 508 regular bodies/modes and
four link targets. All three stages use the same immutable composition, with
before/after body/mode/link guards. Local shared `src` is older tree
`4abb609524a1f1682ea4c190f82d75003c4d679b` and is not the native test base.
Required AGENTS/README/Plan5/finish plan/remaining requirements/checkpoint are
retained as raw tested/refreshed bytes. Published AI_CONTEXT.md remains absent;
the user-confirmed primary-local shared notebook is nonblocking.

Host Python3.12.10; container Python3.12.3/GCC13.3/nm2.42/mysql_config10.11.14.
Tool executable hashes are in `seal/native-build-inventory.json`. Actual engines
are MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL8.0.46-0ubuntu0.22.04.4.
Containers have no network/ports, private RAM source/tmp, direct D: task build
binds, 2 CPUs and 5GiB. The original canonical test requires
`TemporaryDirectory(dir='/')`, so only that disposable root is writable.
External observers record unchanged original argv, timeouts, exits and native
lstat metadata before selected regular-only copies. Physical database bodies,
private keys and service runtime trees are excluded from Windows copies.

Commands ran from the owned worktree with D: temp/cache/build/evidence roots:

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-provenance-owner/reproduce.py
python -X utf8 -m unittest discover -s tests/async -p test_item_equipment_reconciliation.py -k ProvenancePreviousOwnerTests
python -X utf8 tests/async/test_item_equipment_reconciliation.py
python -X utf8 D:/Dev/Temp/accounting-plan5-provenance-owner/freeze.py 01
python -X utf8 D:/Dev/Temp/accounting-plan5-provenance-owner/authenticate.py
python -X utf8 D:/Dev/Temp/accounting-plan5-provenance-owner/launch.py checks01 01
python -X utf8 D:/Dev/Temp/accounting-plan5-provenance-owner/launch.py canonical01 01
python -X utf8 D:/Dev/Temp/accounting-plan5-provenance-owner/launch_budget.py historybudget01 01
python -X utf8 D:/Dev/Temp/accounting-plan5-provenance-owner/qualify.py
python -X utf8 D:/Dev/Temp/accounting-plan5-provenance-owner/seal.py
```

The checks stage loads 16 complete reader modules and executes the entire original
`run_economic_sql_audit_snapshot_mysql.py` separately on both fresh engines.
The canonical stage executes the complete original
`test_restore_economic_coin_effects` with its existing explicit integration flags,
original fixtures/providers/compiler arguments and both native modes/engines.
No C/C++ source changed; maintained `make -C src` is not rerun for this one-line
Python view change. Its prior build evidence is not relabeled as fresh.

## Executed results and scope

| Check | Result |
| --- | --- |
| Corrected unchanged-reader RED | Four methods, 47 assertion failures, zero errors |
| Full equipment/history module | 17 PASS, zero skips |
| Complete 16 reader modules | 392 loaded, 374 PASS, 18 original opt-in skips |
| Whole manual SQL snapshot runner | Both actual engines PASS |
| Previous-owner SQL controls per engine | Five captured cuts, one partial-NULL refusal, 30 actual CLI checks |
| Provenance assertions per engine | 12 known-owner, 3 unknown-owner, 15 exact-dedup checks |
| Complete original native/canonical coin-restore test | 1 PASS, zero failures/errors/skips |
| Original mapping/price budgets | All 12 actual CLI workloads PASS |
| New provenance budgets | Six actual CLI workloads PASS, 100000 events, 33553408-byte inputs |
| Checks stage | 191.311952s; 848 run commands/851 launches |
| Canonical stage | 346.587027s; 58 run commands/60 launches |
| Provenance-budget stage | 13.056630s; six run commands/launches |

The new tests cover all three captured history collections, exact deduplication,
conflicting and unknown previous owners, known unowned and uint64 boundaries,
strict malformed refusal, private-field omission, input immutability and full CLI
limits 0/1/100 with unchanged global findings. Initial RED had nine missing-field
KeyErrors; the corrected assertions make those explicit failures. Both terminal
RED logs are retained. No implementation or runtime failure is hidden or retried.

Existing SQL cuts are reused without adding writes: known source, three owner
mismatches, all-NULL legacy source and the partial-NULL refusal. Selected and
lineage projections count once and show exactly the captured tuple or omission.
Every original SQL runner control remains, including the ten historical-position
cuts/60 CLI checks and 47-table immutability checks. Setup restores its exact
baseline. These are modeled partial SQL cuts, not genuine producer journeys.

Native canonical qualification includes 3, 026 decoder cases / 1, 054 accepted;
each engine runs 109 canonical cuts / 90 refusals / 58 full-entry cuts, with original
pending-claim/native-intent/history controls. 18 original compiler commands succeed.
This establishes the named native reader/restore boundary only. No skipped
native method is claimed as executed by another test.

The 18 original opt-in skips are native stake 1, origins 3, canonical 4, child 2 and
one in each of eight custody modules. Exact names/reasons are retained in
`qualification.json`. Provenance budgets exercise healthy and owner-mismatched
history at all three limits; maximum 1.838220s/214327296B, below the
predeclared 30s/256MiB limits. Count 100, 000, optional source owners, global
exception count and input hashes are preserved. These synthetic measurements
are component evidence, not release-host/load qualification.

## Evidence, curator handoff and remaining gates

Evidence `D:/Dev/Tests/Duris/accounting-plan5/provenance-owner-20261008`;
helpers `D:/Dev/Temp/accounting-plan5-provenance-owner`; builds
`D:/Dev/Builds/Duris/accounting-plan5-provenance-owner-20261008`.
Start with `unchanged-reader-reproduction.json`, `red-corrected.log`,
`change-scope.json`, `source01.json`, `source-transport-authentication.json`,
`qualification.json`, each stage's `terminal.json`/`commands.json`/logs,
`seal/evidence.json` and post-push `delivery/result.json`.

Raw seal SHA256 `56f8c4f341914644f767f2ec7639995fdd3eb9d74f3a6d3a863cbdaba5c20714` covers 1, 025 files / 498,380,752B,
803 native build files, zero copied links/reparse points.
All three test containers and the inventory container terminate successfully,
without OOM or live recorded children. The post-push receipt independently
rehashes sealed bodies, all 24 overlay blobs, the published primary and owned remote branch tips and seven ancestors.

This report and the additive remote follow-up are curator-ready notebook input;
application/acknowledgement on the primary's local system is not claimed or a
work blocker. At qualification, primary `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` retains the
same native/migration inputs. Latest private 143-production-file d9eaa45b candidate
remains compiler/native/SQL/gameplay/recovery-unexecuted and is not this test base.

The complete backup module remains 11 PASS/one shared original fixture-link
failure at the prior sealed source (see
PLAN5_CURRENT64_FULL_BACKUP_MODULE_HANDOFF_2026-10-08.md). Genuine provider
closure, original room-seed UID-owner closure and original native SQL baseline
recipe/head qualification remain primary-owned. Genuine producer/source
installation, full opening correspondence, player/route/fault/load journeys,
combined private candidate, Plan5, original R1–R8 and release qualification
remain open. No activation, production access/write, autocorrection, deployment,
merge or experimental-accounting push occurs. This completed owned operator
slice is progress, not completion of the active Plan5 goal.
