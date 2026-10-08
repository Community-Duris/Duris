# Plan 5 schedule-receipt validation — 2026-10-08

Future and nonfinite schedule completion times caused `schedule` to skip capture
and return healthy status. Fractional completion also deferred capture; non-object
JSON roots raised uncontrolled AttributeError. The unchanged reader reproduces
four false healthy deferrals and five uncontrolled errors. Other malformed times
could trigger capture and replace the untrusted schedule receipt.

The scheduler now samples the current integer epoch once, requires a receipt
object with an exact integer `completed` in `[0, now]`, and only then chooses the
due-capture or recent-health path. Invalid readable metadata returns exit 1 with
`event:schedule`, `result:failed`, `code:invalid_schedule_receipt`, preserving the
receipt and refusing before capture, rotation/pruning or health fallback. Missing
receipts still take the first scheduled capture; valid zero/old/due/current times
retain their existing cadence. Capture failure still cannot advance the deadline.
No receipt version, storage field, retention tier or accounting contract changes.

## Branch, owned files and consumer handoff

Sole remote/local `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Owned base `5e6a63ab847163852359351ca683152a4bbff992`. The containing commit is this solved issue's result;
`D:/Dev/Tests/Duris/accounting-plan5/schedule-receipt-20261008/delivery/result.json` records exact result/remote, clean tree,
preserved ancestry and post-push evidence rehash. All seven consolidated branch
tips and earlier owned fixes remain ancestors. No branch switch or primary push.

Four owned files: `scripts/persistence_backup.py`, existing
`tests/async/test_persistence_backup.py`, this report and additive
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. Only the existing `main` schedule branch
changes; two methods are added to the existing test file. Every prior function/
test method and the entire restore/remediation/integration module remain exact.
`change-scope.json` binds that AST/body scope and the two-file composition diff.
No helper, dependency, framework, shared native recipe or harness is added.
Shared coordinator/contracts/producers/registry/matrix/activation and migrations
remain. Inactive behavior, wallet-root ITEM_MONEY exclusions and the declined
inactive spell path stay. Earlier tombstone/manifest/drill/capture-age fixes
remain in the tested 27-overlay composition and must be retained during import.

Existing schedule field: `completed`. Invariant: exact integer, nonnegative and
not after the sampled current epoch. The owned diagnostic above adds one fixed
refusal code; stderr shape and nonzero exit contract remain. The sole reader/
writer is the schedule branch; `backup_pfiles.sh` execs the CLI and forwards its
exit status. The sample systemd backup oneshot consumes that status and has no
code allowlist. `operator-consumers/` retains their exact unchanged source and
contract. No shared accounting interface/schema request is needed. Installed
host timers, permissions and alert delivery are not qualified by source review;
operators must inspect invalid evidence and clock state. This tool does not
repair or delete the receipt to manufacture a successful schedule.

## Exact tested source

| Input | Identity |
| --- | --- |
| Refreshed/tested primary | `e62269e48aebcab906256908cf97a65dc4ffb5a4` |
| Fixed primary plus 27 owned overlays | `79b7d17ba7e508d7d691f7e8daffd582b1c05230` |
| Fixed archive SHA256 | `1706a6d154f1f9c8f0a4dc980a7e6ec7dd45e5f71a4b42b1edb182cb9eda1e47` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree, canonical head 64 | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Backup script blob | `0975d0f3adf58843c9f3131cba6e6cad1fb6bc13` |
| Backup test blob | `b2ca933ab3e386885cb4d92a17803b7c7706e63b` |
| Preserved restore blob | `064e40729da3e5371a86e898753ea6c657952ff7` |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

Source00 `a19c4d6505cd56a6c1034dd6d95ca2ffa7316541` establishes the
unchanged defect; source01 `9776ba7c642cb2f7670dee7de01674355ebf30d7`
adds only the regressions. All passing stages use source02. All three transports
authenticate 6517 Git blobs, 6513 regular bodies/modes and four links each.
Before/after guards bind every source body/mode/link. Raw AGENTS/README/Plan5/
finish plan/requirements/checkpoint/backup guidance is retained. Published
AI_CONTEXT.md is absent; the primary-local notebook is user-confirmed nonblocking.
Historical local native source is not the published native test base. This exact
composition does not qualify every included overlay or private primary candidate.

## Commands and results

Host executable: `C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe`,
Python3.12.10/SHA256 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a.
Commands run from the owned worktree. Full Docker/compiler/SQL/qualifier argv is
retained in stage `docker-command.json`, `commands.json` and `launches.json`.
Every Docker stage disables network and mounts D: outputs directly.

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/freeze.py 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/launch.py reproduce00 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/freeze.py 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/launch.py red01 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/freeze.py 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/authenticate.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/bind_reuse.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/launch.py units02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/launch.py disposable02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/launch.py cli02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/bind_scope.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/bind_consumers.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/primary_refresh.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/qualify.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-schedule-receipt/seal.py
```

- RED: new invalid-receipt method has 13 failures/6 errors across 19 malformed
  cases; cadence/failure-deadline control method already passes. Retained as RED.
- Full filesystem modules: 48 backup tests plus 17 recovery-remediation tests,
  **65 PASS, zero errors/failures/skips**. New cases cover non-object/missing
  completion, bool/float/string/container/negative/future/nonfinite values;
  refusal excludes both capture and health calls and preserves generation,
  schedule and completion receipts. Valid first/zero/due/current and exact last-
  second cadence controls perform real synthetic filesystem captures in both
  modes; a failed capture leaves the deadline unchanged. Existing post-publication
  retry/replication/retention/failure controls remain exact.
- Actual CLI: **40 calls** across both backup modes: 38 fixed refusals and two
  recent exact-integer controls. No private alias or traceback escapes; all
  malformed receipts/generations/completion receipts remain. SQL dump bytes in
  this filesystem stage are explicitly mocked, not a real SQL capture claim.
- Disposable SQL: both original SQL backup methods run unchanged and PASS,
  zero skips, with original full schema/history/value/claim and corrupt-import
  controls. Two actual server boots reach `Entering game loop.`. Each engine then
  imports the verified generation into a fresh socket-only source, copies its
  original journals and qualifies it. Four bad receipts per engine refuse with
  no new generation or receipt rewrite. A valid due receipt performs an actual
  scheduled full SQL capture and publishes a verified new generation; recent
  receipts before and after that capture take health without capture/advancing
  the receipt. Thus **two actual due captures, eight invalid-receipt refusals,
  four recent controls** across MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and
  MySQL8.0.46-0ubuntu0.22.04.4. Prior generations and journals remain exact.
  The original `claim_capture` proves ten selected accounting/native claim tables
  plus the four selected critical inbox roots unchanged; this is bounded model
  evidence, not complete source-wide producer/opening proof.
- Database qualifier calls: **50**, including
  **22 expected original refusals**. Fresh native fixture
  GCC driver calls: **20**, comprising
  **4 compile/link calls** and
  **16 toolchain queries**. Original genuine providers and
  arguments remain; nested compiler-internal counts are not inferred. Passing
  native stage contains 9 regular build files, not that many
  compiler jobs. Maintained SQL/flatfile servers are authenticated reused binaries,
  not fresh server builds. `reuse/*/reuse-binding.json` binds SQL 1318 repository+
  28 image and flatfile 1320 repository+20 image dependencies, exact native/schema
  and binary hashes. Linux Python3.12.3/GCC13.3/nm2.42/mysql_config10.11.14
  versions/executable hashes are in the native seal inventory. No C/C++ edits.

## Evidence and remaining gates

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/schedule-receipt-20261008`. Builds:
`D:/Dev/Builds/Duris/accounting-plan5-schedule-receipt-20261008`.
`qualification.json`, stage logs/terminals, `cli-results.json`,
`whole-results.json`, `schedule-observations.json`, source transport, AST/body/
consumer/dependency/native inventories and original assertions bind these claims.
Raw seal **e18f9d20cbdca091d919dd63585a4f3fac63d9ce2cc157aef02cbb499acc4354** covers 1708 files/1,670,995,992 bytes,
13 native build regular files and
6 stopped non-OOM network-isolated containers. RED remains;
no copied native link/reparse target is followed. Post-push delivery rehashes all
sealed files and verifies all 27 overlay blobs against the result commit.

Full original 12-case backup module is not rerun: its original flatfile fixture
recipe still lacks seven established genuine-provider symbols before nine
missing-record controls; the last 11 PASS/1 link-error result keeps its own scope.
No stub, relaxed warning flag or shared recipe edit is introduced. Native keeper/
producer/room UID ownership, actual flatfile scheduled service journeys, complete
opening/treasury/player/routes, fault/load/ACK/recovery, private combined candidate,
activation and full Plan5/R1–R8/release remain open. No new publishing blocker;
no production write, activation, promotion, deploy, merge, audit autocorrection,
primary push, host timer installation or cross-chat message occurs.

Report and additive remote follow-up are curator-ready notebook input. Primary-
local notebook remains nonblocking; curator application/import/ack is unclaimed.
Private source-only primary milestones receive no execution qualification here.
