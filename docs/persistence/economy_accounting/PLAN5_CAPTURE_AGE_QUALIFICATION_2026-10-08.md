# Plan 5 nonnegative backup capture age — 2026-10-08

The backup health command returned `ok` with a negative capture age after a
one-second or five-minute clock rollback. The unchanged reader reproduces this
in both backup modes. Generation verification intentionally accepts up to 300
seconds of clock skew; status then checked only the upper RPO bound. The existing
backup/finalization guard already requires a nonnegative age.

One condition in `status` now requires `0 <= age <= rpo_seconds`, preserving the
existing `rpo_exceeded` refusal code. The verifier's clock-skew tolerance, manifest
format, capture, restore, retention and completion logic remain unchanged. Zero
age and the inclusive RPO boundary pass. Negative ages and RPO+1 refuse. Status
preserves the generation and completion receipt; it performs no adjustment.

## Branch and exact owned scope

Sole remote/local `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Owned base `d8c606042da403e5332c380801ebe1e4fa7a4cfc`. The containing commit is the solved issue's result;
`D:/Dev/Tests/Duris/accounting-plan5/capture-age-20261008/delivery/result.json` records the exact result/remote, clean tree,
ancestor preservation and post-push seal rehash. All seven consolidated historical
tips and earlier owned fixes remain ancestors. No branch switch or primary push.

Four owned files: `scripts/persistence_backup.py`, existing
`tests/async/test_persistence_backup.py`, this report and additive
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. The sole production change is the status
condition; one test method is added. Every prior function/test method and the
entire restore/remediation/integration modules remain exact. `change-scope.json`
binds the AST/body evidence and the two-file source-composition diff. No shared
interface/schema change is requested. Shared coordinator/contracts/producers/
registry/matrix/activation, migrations and original native recipes are untouched.
Inactive behavior, wallet-root ITEM_MONEY exclusions and the declined inactive
spell path remain. Earlier tombstone, generation-manifest and drill-receipt fixes
are preserved in the 27-overlay composition; primary integration must retain
those prerequisites rather than reverting their blobs.

## Tested source and commands

| Input | Identity |
| --- | --- |
| Refreshed/tested primary | `e62269e48aebcab906256908cf97a65dc4ffb5a4` |
| Fixed primary plus 27 owned overlays | `a19c4d6505cd56a6c1034dd6d95ca2ffa7316541` |
| Fixed archive SHA256 | `cacef231181c3a8a77f5b3c2853f2c0e65ffa74eda58a51e515a27467d0af6da` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree, canonical head 64 | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Backup script blob | `2e3b1d74996b8981bb13bb8aa423c15cecad5224` |
| Backup test blob | `0427809552302859992e1bb8b961c2b52b18ae42` |
| Preserved restore blob | `064e40729da3e5371a86e898753ea6c657952ff7` |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

Source00 `df4205391a1dc67d795c2cd7b4347613f270711a` establishes the
unchanged defect; source01 `c7ce9a9cd8dfd7e5ba5f9dda065e6cfd90bc9cd6`
adds only the regression. All passing stages use source02. The three archives
authenticate 6517 Git blobs, 6513 regular bodies/modes and four link targets each.
Before/after guards verify every source body, mode and link. Local historical
native files are not the native test base. Raw AGENTS/README/Plan5/finish plan/
requirements/checkpoint/backup guidance is retained. Published AI_CONTEXT.md is
absent; the primary-local notebook is user-confirmed nonblocking.

Exact host Python executable is
`C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe` (Python3.12.10,
SHA256 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a).
The following commands run from the owned worktree. Full Docker argv and every
native/SQL/qualifier invocation are retained in stage `docker-command.json`,
`commands.json` and `launches.json`. All writable builds/scratch are under D:;
Docker mounts D: directly, with network disabled for every stage.

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/freeze.py 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/launch.py reproduce00 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/freeze.py 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/launch.py red01 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/freeze.py 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/authenticate.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/bind_reuse.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/launch.py units02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/launch.py disposable02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/launch.py cli02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/bind_scope.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/primary_refresh.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/qualify.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-capture-age/seal.py
```

## Results and limits

- Unchanged reproduction: four false healthy results, at ages−300/−1 in both
  backup modes; source generations unchanged.
- New regression RED: eight failed subtests, no errors/skips. Final unit run:
  46 backup tests plus 17 recovery-remediation tests, **63 PASS/zero skips**.
  The new method checks six ages×two modes×both plain/required-drill status,
  direct API and CLI dispatch, exact refusal/age fields and immutable generation/
  completion receipt. Existing drill and retention controls remain exact.
- Actual CLI: six calls across both backup modes; future captures 60/300 seconds
  produce four `rpo_exceeded` refusals, two current controls pass. Generations and
  receipts remain unchanged. This filesystem stage explicitly mocks SQL dump
  bytes and is not a database restore claim.
- Real SQL integration: both original SQL backup methods run unchanged and PASS,
  zero skips. Their native/full-dump/schema/history/value/claim controls and
  three corrupt-import refusals per engine remain. Two actual server boots reach
  `Entering game loop.`. The appended twelve status checks use these real SQL
  generations: six healthy zero/positive/RPO-boundary results and six negative/
  stale refusals. Every generation and completion receipt remains unchanged;
  status adds no service boot or database qualification call.
- Exact engines: MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and
  MySQL8.0.46-0ubuntu0.22.04.4. Native/database qualification calls:
  **46**, including **22 expected refusals**.
  Fresh native fixture GCC driver calls: **20**, comprising
  **4 compile/link calls** and
  **16 toolchain queries**. Original arguments/providers
  remain; nested compiler-internal counts are not inferred. Passing native stage
  contains 9 regular build files, not that many compiler jobs.
- Maintained SQL/flatfile servers are authenticated reused binaries, not fresh
  builds. `reuse/*/reuse-binding.json` verifies native/schema dependencies,
  SQL 1318 repository+28 image dependencies and flatfile 1320 repository+20 image
  dependencies. Exact binary hashes remain 1a7e305e8be3f516ae6778d6eb03fadcb14859e0876df6d279fc1d4bc75d937b
  and 9462829d443f5e138feac60c30bb27c8ec81b921fadfb2862be4a5386e3b54be.
  Linux Python3.12.3/GCC13.3/nm2.42/mysql_config10.11.14 versions and executable
  hashes are retained in the native seal inventory. No C/C++ source changes.

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/capture-age-20261008`; build root
`D:/Dev/Builds/Duris/accounting-plan5-capture-age-20261008`.
`qualification.json`, stage terminals/logs, `cli-results.json`,
`whole-results.json`, `age-observations.json`, service logs, source authentication,
scope/dependency/native inventories and original assertions establish this scope.
Raw seal **03f77533ea2c5ee39cd09397636ba5f49a5bd1fa7ec2bb65624fcafb78381a6c** covers 1465 files/1,670,193,875 bytes,
13 native build regular files and
6 stopped non-OOM network-isolated containers. RED remains
retained; no copied native link/reparse target is followed. Delivery rehashes all
sealed files and verifies all 27 overlay blobs against the result commit.

This does not rerun the full original 12-case backup module: its original flatfile
fixture recipe still lacks seven established genuine-provider symbols before
nine missing-record controls. The last full 11 PASS/1 link-error result retains
its own scope. No stub, relaxed warning flag or shared recipe edit is introduced.
Actual flatfile service health/drill, native keeper/producer/room UID ownership,
complete opening/treasury/player/routes, fault/load/ACK/recovery, private combined
candidate, activation and full Plan5/R1–R8/release remain open. A future schedule
receipt is a separate investigation, not fixed or qualified here. No new blocker
to publishing this solved issue; no production write, activation, promotion,
deploy, merge, autocorrection, primary push or cross-chat message occurs.

Report and additive remote follow-up are curator-ready notebook input. The
primary-local notebook remains nonblocking. Curator application, primary import
or acknowledgement is not claimed. Private source-only primary milestones do not
receive execution qualification from this component result.
