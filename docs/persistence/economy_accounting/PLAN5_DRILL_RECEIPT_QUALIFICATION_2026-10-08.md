# Plan 5 restore-drill receipt qualification — 2026-10-08

A recent failed drill receipt caused `drill` to report `not_due`. Future and
fractional completion timestamps also deferred the actual restore. The unchanged
predecessor reproduces all three cases. Malformed receipt roots reached `.get()`
and escaped the CLI's controlled refusal handling. Required-drill health checks
accepted fractional completion times as current evidence.

The existing scheduler now defers only for a qualified receipt with an exact
integer completion timestamp between zero and the current integer epoch time,
and an age strictly below `drill_seconds`. The health command uses the same
completion-age check; `--require-drill` retains its existing qualified-result
requirement and inclusive maximum-age boundary. Invalid completion evidence
reports `restore_drill_missing_or_overdue` there. Optional health reports a null
age for invalid completion data. A syntactically readable but invalid drill
receipt cannot defer a fresh restore. Unreadable JSON still refuses through the
existing reader. Receipts are written only after actual qualification; no input
receipt or authority is auto-corrected. No receipt field or version is added.

## Branch and owned scope

Sole local/remote branch `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `66a6bc1dd5f4f4bdaf1666df2419a51a10937ad6`. The containing commit is this slice's result;
`D:/Dev/Tests/Duris/accounting-plan5/drill-receipt-20261008/delivery/result.json` records the exact result, verified remote,
clean tree, ancestor preservation and post-push evidence rehash. All seven
consolidated historical tips and every intervening owned issue remain ancestors.
No branch switch or primary push occurs.

Owned files are `scripts/persistence_backup.py`, `scripts/persistence_restore.py`,
`tests/async/test_persistence_backup.py`, this report and additive
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. One small helper serves the two existing
receipt consumers. Only `status` and `restore` change; every other production
function and prior test method remains exact. Two focused methods are added to
the existing test file. The integration module and remediation module are byte
identical. `change-scope.json` binds these AST/body comparisons. No shared
interface/schema request, native recipe/harness edit, coordinator/producer/
registry/matrix/activation edit, dependency or framework is introduced.
Inactive behavior, wallet-root ITEM_MONEY exclusions and the declined inactive
spell path remain. Earlier owned tombstone and generation-manifest fixes are
preserved in both tested scripts. Primary integration must preserve their
prerequisites; this slice does not reimplement them.

## Exact tested source

| Input | Identity |
| --- | --- |
| Tested published primary | `ba319959eec7305a7e68a60dce3d24accbf29408` |
| Fixed primary plus 27 owned overlays | `51f484cf3fd2d1460cc3cdc95cf30ba22604e606` |
| Fixed archive SHA256 | `5d3239e797cb757e9b24c3d353cb683fde909e0f1190e0c4babf226672d6a166` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Canonical head | 64 / 0064_auction_custody_history |
| Backup script blob | `a38e053e9ed1380e3f6fa23132d3211830ac9d1c` |
| Restore script blob | `064e40729da3e5371a86e898753ea6c657952ff7` |
| Backup test blob | `be1daf9e9b15c8e1d691913861bb77c07e0d4699` |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

Source00 `23feca74ceb468597d10f21dfc0e84c19eccd074` reproduces the
unchanged predecessor. Source01 `a1e5ccdff9505da11214c1a4bc07a8f7df3b4e93`
adds only the regressions. Final source02 is used by all passing stages. Its
published-primary advance since source00 is documentation only. All three Git
archives authenticate every body, mode and link target; before/after stage
checks retain those bindings. Raw AGENTS/README/Plan5/finish plan/requirements/
checkpoint/backup policy guidance is retained. Published AI_CONTEXT.md is absent;
the user-confirmed primary-local notebook is nonblocking.

Delivery refresh `0e67e71624e7e547d3a13eac692efcc4c33681f5` changes only the three documented
paths in `primary-refresh/result.json`; native/schema trees remain exact. New
private refining/craft/warm-room source acceptance has no transferred execution
qualification. The owned branch's historical native tree is not the test base.
This composition records exact executed inputs and does not qualify every
included overlay or the primary's private candidate.

## Commands and results

Run from the owned worktree with Windows Python3.12.10, D: temp/cache/builds and
distinct evidence directories. Every Docker stage has network disabled and
writable build output directly mounted from D:. Commands are retained as exact
argv in each stage's `docker-command.json`, `commands.json` and `launches.json`.

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/freeze.py 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/launch.py reproduce00 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/freeze.py 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/launch.py red01 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/freeze.py 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/authenticate.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/bind_reuse.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/launch.py units02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/launch.py disposable03 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/launch.py cli02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/bind_scope.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/primary_refresh.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/qualify.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-drill-receipt/seal.py
```

- Reproduction: three unchanged false `not_due` results, receipts unchanged.
- RED: the two new methods produce 18 failed subtests and 18 errors, no skips.
  This is retained separately; it is not a passing stage.
- Linux filesystem modules: 45 backup tests plus 17 recovery remediations,
  **62 PASS, zero failures/errors/skips**. Existing exact-boundary controls remain;
  new controls cover malformed roots, missing/wrong result, bool/float/string/
  container/negative/future/nonfinite/missing completion and interval boundaries.
  The restore test exercises 27 invalid/due inputs through the direct API and
  CLI dispatcher before the generation gate, preserving input evidence and
  excluding candidate/service/native work. Qualified current and last-second
  receipts still defer; exact due boundary proceeds.
- Actual status CLI: **38 calls** across both backup modes, comprising 36 fixed
  refusals plus two current-qualified controls. Filesystem capture is synthetic;
  SQL dump bytes are explicitly mocked in this stage. Receipts and generations
  remain unchanged; no uncontrolled traceback or private alias escapes.
- Real disposable integration: both original SQL backup methods run unchanged,
  plus three real drill restores per engine. **Two methods PASS, zero skips**;
  **six fresh drills**, six repeat `not_due` controls, **eight real server boots**
  all reach `Entering game loop.`. The bad prior receipt classes are recent
  failed, qualified future, and qualified fractional. Each fresh drill imports
  the full dump, qualifies before and after service, removes its isolated
  candidate, writes the qualified receipt and passes required-drill health.
  Generation and journals remain unchanged; the source-code guard passes.
- Original SQL controls retain full schema/history/value checks, read-only
  pending-claim capture, distinct socket checks and three corrupt import refusals
  per engine before boot. Exact versions are retained in `whole-results.json`:
  MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL8.0.46-0ubuntu0.22.04.4.
  Total database qualifier calls: **58**, including
  **22 expected refusals**.
- Original native fixture compiler arguments and genuine providers are unchanged.
  The passing native stage observed GCC driver calls: **20**, comprising
  **4 compile/link calls** and
  **16 toolchain queries**. Nested compiler internals are
  not inferred. The passing native stage contains 9 regular files,
  which is not a compiler-job count.
- SQL and flatfile maintained servers are authenticated reused binaries from the
  prior published62/schema64 build, not fresh rebuilds. Exact source/dependency/
  binary bindings are in `reuse/*/reuse-binding.json`; SQL1318 repository+28 image
  and flatfile1320 repository+20 image dependencies match. Fresh fixtures and
  actual restore/server execution are separately recorded. Linux Python3.12.3,
  GCC13.3, nm2.42 and mysql_config10.11.14 executable versions/hashes are retained
  in the native seal inventory.

## Evidence and remaining gates

Raw evidence: `D:/Dev/Tests/Duris/accounting-plan5/drill-receipt-20261008`. Builds:
`D:/Dev/Builds/Duris/accounting-plan5-drill-receipt-20261008`.
`qualification.json`, stage terminals, test logs, `cli-results.json`,
`whole-results.json`, `drill-observations.json`, service logs, source transports,
AST/body scope and compiler/dependency inventories establish the reported scope.
Raw seal: **2f800f18e05c0f39b84502a63fa3ba247c3e47dd245838fe01f289c208ff1e10**, 1660 files,
2,136,241,430 bytes, 22 build regular files,
7 stopped non-OOM network-isolated containers. No copied
native link or Windows reparse target is followed. Failed RED and the initial disposable02 observation run remain retained.
That first observation tried to treat a disposable SQL datadir as a file source,
correctly hit writable_ancestor, and failed before the new drill cases. The
corrected disposable03 observer compares generation/journals and retains the
original SQL-level value controls. Production source02 is unchanged.
Post-push `delivery/result.json` rehashes all sealed files and verifies all 27
source overlay blobs against the result commit.

The whole original 12-case backup module is not rerun: its unchanged flatfile
native recipe still lacks the seven previously established genuine-provider
symbols, before nine missing-record controls. Its last full run remains 11 PASS/
1 unchanged link error, with its own source and evidence. This slice neither
stubs those providers nor edits the shared recipe. Native keeper/producer/room
UID ownership and baseline recipe handoffs remain with primary. No production
backup, host timer installation, activation, promotion, deploy, merge, external
message or audit correction occurs. Actual flatfile service drill, complete
opening/treasury/player/routes, fault/load/ACK/recovery, private combined native
candidate and full Plan5/R1–R8/release remain open. There is no new blocker to
publishing this solved slice.

This report and the additive remote follow-up are curator-ready notebook input.
The primary-local notebook remains nonblocking; curator application, primary
import and acknowledgement are not claimed.
