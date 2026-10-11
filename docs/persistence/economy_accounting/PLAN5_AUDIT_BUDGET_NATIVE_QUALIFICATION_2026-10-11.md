# Plan 5 current-primary original Linux audit-budget qualification — 2026-10-11

Both original AuditBudgetTests pass on the frozen current-primary reader
composition: **2 tests, 0 failures/errors/skips**, twelve original budget records
and both original oversized-input checks. The largest measured CLI is
1.262167412 seconds and the largest child peak is151457792bytes (144.44MiB),
below the unchanged30-second/256MiB limits. This is a synthetic offline JSON
component; release_host_qualified remains false. It does not complete Plan5,
native authority, SQL/flatfile durability, the release host or full accounting.

## Source, publication and ownership

- Branch and only publication destination: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Producer base: `24ae553d9cb732182993baba18b36c3e1fc63d5f`.
- Exact tested component source:
  `0555741d740cf028552fcf63973265bfbcda052c`, tree
  `36ff31fdd214ca80c69b5edf2c4aabcc1f69b2ef`.
- This is the four-file selected reader/test/decoder/registry composition from
  that source, not execution of the full checkout or broad unadopted Plan5 code.
- Maintained changes: this report and additive
  `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md` only. Shared interface/schema requests:none.
  No source/test/registry/matrix/migration/producer/contract/activation edit.
- Result/verified remote/report/evidence identities are bound after publication
  in `delivery/result.json`. All seven historical Plan5 tips remain ancestors.

The exact inputs, decoder correction, actual missing-module failure and prior
preparation preservation are recorded in
[the corrected preparation](PLAN5_AUDIT_BUDGET_DEPENDENCY_CORRECTION_2026-10-11.md).
All four native inputs remain original Git bytes:

| Input | SHA-256 |
| --- | --- |
| `scripts/reconcile_economy_accounting.py` | `f8235cab0f79e785d8a27183ac0076a44e334a46afffb181c5e83958f28b1dd5` |
| `tests/async/test_reconcile_economy_accounting.py` | `ba7cb643673bf445c625c243313ec4e498213d3a52e73d909cbad6ebbca3c9c9` |
| `docs/persistence/economy_accounting/registry.json` | `fbe53c9a037fe3bcdcb27cfed4110ed39e01c6acaa1917c26c34ce957bf0e8df` |
| `scripts/economic_restore_evidence.py` | `bb927db1a509d377e9930b264534665f38bd88ebbbee5672075db7267ad6f0bc` |

Source inventory SHA85161510b291142e388ccdb33c7466a9a63fac8432bb769add79bd1358024267;
archive SHA1a206ab72fd37ddff471981b627a911eb36202bab4d904f030e972b6a66a04ab.
No local environment, credentials, player data or live checkout is mounted.
The independent reader/decoder does not import mutation logic. The deferred SQL
reader module is outside the tested paths. Earlier unqualified preparation and
helper failures remain preserved; this pass does not relabel them.

## Exact commands, execution and results

Host command, executed once on native process session57445:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01/helpers/run_audit_budget.py --preparation D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01/preparation-receipt.json --grant D:/Dev/Temp/accounting-acceleration-bootstrap-20261011/ACC-05-AUDIT-BUDGET-01-resource-grant.json
```

Inside the pinned Linux QA container, the original command is:

```text
python3 -u -B tests/async/test_reconcile_economy_accounting.py AuditBudgetTests.test_near_limit_price_view_cli_budget_and_limit_invariance AuditBudgetTests.test_near_limit_mapping_snapshot_cli_budget_and_limit_invariance -v
```

`DURIS_RUN_AUDIT_BUDGET=1`, `PYTHONDONTWRITEBYTECODE=1` and
`TMPDIR=TEMP=TMP=/scratch` satisfy the original explicit Linux guard. Original
unittest entry point, methods, asserts, fixtures, CLI subprocess arguments,
fresh-child timing/RUSAGE and35-second child timeouts are unchanged. There is no
observer, monkey patch, waived case, skip, retry or broader suite invocation.

Actual original terminal: `Ran 2 tests in 17.318s`, `OK`. Native, Docker
foreground, host transport and process session all terminate0. Recorded Docker
foreground duration18.625seconds; job receipt interval
2026-10-11T10:51:01.466234Z–10:51:23.905279Z includes preflight and cleanup.

| Original method/control | Actual input and cases | Result |
| --- | --- | --- |
| Near-limit price view |100000 price roots;33552384bytes; clean and conflicting fixtures at limits0/1/100. |Six real CLI measurements; clean global count100000/exception0; conflict count100001/exception1. Exact requested output-row counts and truncation, global coverage and input identity pass. Maximum0.892705917s/151457792bytes child peak. |
| Near-limit mapping audit |9948 retained-plan mapping roots;33551795bytes of structured JSON; clean and corrupt fixtures at limits0/1/100. |Six real CLI measurements; clean exceptions{}; corrupt exactly `invalid_mapping_creation_root:1`. Full operation/effect counts, bounded details and input identity pass. Maximum1.262167412s/130363392bytes child peak. |
| Oversized refusal within the original mapping method |33554433bytes; limits0/100. |Both unchanged assertions complete: CLI exit2, empty stdout, `snapshot or output limit exceeded`, unchanged input. No separate fabricated oversized measurement is emitted. |

Every measured call satisfies the original near32MiB byte range,100000-row and
100-output limits,30seconds and256MiB child peak. The price fixture reaches its
row bound and uses original valid trailing whitespace to approach its byte bound;
it is not32MiB of structured price records or a complete mixed native snapshot.
Each clean/damaged input has the same original hash across all three detail
limits, with input_unchanged true and synthetic_component true.

The original TemporaryDirectory cleanup removes generated large inputs. Their
hashes, bounds, invariance and immutability assertions remain in the unchanged
source and actual logs; this report does not claim retained raw input files.
Raw stdout contains all twelve original markers; raw stderr contains both named
method outcomes and real zero-skip terminal. The seal parser independently
verifies that complete case/limit matrix and exact terminal.

## Reviewed resource receipt, seal and cleanup

Coordinator grant SHA
c73937ca62c93a10d917164296cfadcf1d8d43462e79ead032049223f794fc7a;
initial exact GRANTED global lock SHA
7db8edcfa4b6e3ec3b5011ccb4b27d588e4e88716b639d332872ce721d0dae0f.
Preparation review/grant SHA
ebf6a93a41aa6eb239f4a97532ab4eef797d6e5401dac4e2a9a137908f41eefe.
Final preparation6f5189db.../transportb3303bd.../seal helper414b71eb... are the
full exact hashes in the corrected report and sealed receipt. No helper or
source edit occurred after grant or during execution.

Actual QA image:
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
Actual container `plan5-acc05-audit-budget01-a425dcb8ac-qa`, full ID
`6019e35cd2e6b2f2bd91f0eafe6d0cd5614970f6fd2ef89eaa8e2a6b1b362ce4`.
One foreground `--rm`/restartno container,2GiB/CPU2/networknone/read-onlyroot;
fresh D: source read-only and task-specific scratch/evidence writable. Full
owner/job/source labels, mounts and actual HostConfig are retained. No DB,
server, compiler, build cache, database volume or production connection is used.

Sealing command, after actual native terminal and transport cleanup:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01/helpers/seal_audit_budget.py --preparation D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01/preparation-receipt.json
```

Seal SHA-256:
`c2f946d03dde30c0158ef7f86d9891f28939d2b4b9dd6b623d4f84efea6cd245`;
76 verified members/2668643bytes. Qualification-result SHA
64deeebe8bc7ae13a64f4ffa928bc5ce0593eca2918cd126592f3a0203068722,
status `PASS_SYNTHETIC_COMPONENT_ONLY`. The seal includes four exact source
files, source archive, helpers, raw logs, grants/review, session receipt,
original failed preparation/history and successful terminal inventories.
Later publication metadata lives separately under `delivery`.

The QA container removed itself after its successful foreground exit. Transport
and independent seal checks both confirm successful empty exact owner/job
inventory, all four passive database volumes unchanged with consumers0, and
all seven unrelated running service full IDs preserved. All four source files
and helper bytes remain unchanged. The matching heavy lease is RELEASED after
seal verification; `delivery/released-lease.json` records its sealed release and
the global lock is absent. No owned running resource remains. Existing unrelated
services, passive volumes, stopped recovery containers and all evidence remain.

## Remaining qualification and curator packet

Evidence root:
`D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01`.
The coordinator independently verifies all76 sealed members, four Git/source
inputs, original native controls, exact container configuration, fresh owned0,
four passive volumes/consumers0, seven preserved live IDs and matching released
lease. Terminal review SHA
5be8e7f7c122146e5bfcf0622e07c894ba7371fb92ddf905c7cea2c44e0c10ba at
`D:/Dev/Tests/Duris/accounting-accel-integration/ACC-05-AUDIT-BUDGET-01-review/terminal-review.json`.
The coordinator preserves and repairs one host-review mount-comparison guard:
actual Docker metadata retained Windows D: strings rather than its expected
Linux /d/ spelling. Exact resolved D: comparison passes; no native rerun,
evidence, source, result or seal change. This is a review-helper correction,
separate from the intact original native pass.

This exact terminal slice is ready for primary handoff. The report/follow-up is
additive curator input for the locally
maintained shared notebook; no notebook import or full candidate adoption is
claimed. No additional cohort is started without a fresh reviewed ticket.

Remaining gates include complete current two-engine65 matrix/startup/tamper and
measured contract integration; current backup/restore, native retention and
durable erasure propagation/replay; wallet/room authority; complete mixed32MiB,
release-host/player/gameplay/fault/load/recovery and R1–R8 release acceptance.
No disposable database, SQL/flatfile authority, C++ build or full audit suite ran
in this bounded ticket. Those gates cannot be inferred from this synthetic pass
or route inventory. No selector was skipped and no resource blocker affected
the completed component.

Full goal remains ACTIVE; accounting inactive, admission CLOSED, coverage
incomplete, release BLOCKED. Preserve wallet-root item exclusions and the declined
inactive spell-path change. No activation, deployment, merge, production data
mutation or automatic audit correction occurs.
