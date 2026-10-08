# Plan 5 completion-receipt validation — 2026-10-08

Matching-generation completion receipts previously bypassed their writer's version,
completion time and result/replica grammar. The unchanged reader reports healthy
status and reaches the next capture boundary for boolean version, fractional time,
completion before capture, future time and an unknown replica alias. The previous-
generation recovery reader applied stricter checks, but could still accept future
completion within its existing relative-generation tolerance.

The three readers now reuse a small private predicate for existing fields. A receipt
must be an object, version must be exact integer 1, completed must be an exact
integer between its generation's created time and the current integer epoch, and
generation must bind that generation. Successful result ok requires string replica
not_configured or transport_and_readback_verified; replication_pending requires
string replica pending. The previous-generation reader retains its existing relative
generation bound, result ok requirement and configured-replica policy check.

Invalid current receipts refuse before capture or completion retry. Status uses
existing backup_or_rotation_incomplete; backup uses existing
prior_backup_requires_finalization. The CLI keeps exit 1 and its fixed redacted JSON
error contract. Protected refusal may persist the existing backoff marker, but does
not rewrite the invalid receipt, create a generation or auto-finalize. Valid pending
replication still retries; valid successful completion still permits new capture.
No new field, format, error code, dependency, operational switch or accounting
contract is introduced. Optional replica_error and replica policy changes retain
their existing interpretation; this predicate does not add new policy enforcement.

## Branch, scope and handoff

Sole local/remote branch codex/accounting-plan5; worktree
C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max.
Base 6a1c564b6a8d57273a4115a3f1c03b32a6713f9b. This containing commit is the result;
D:/Dev/Tests/Duris/accounting-plan5/completion-receipt-20261008/delivery/result.json records its exact SHA, remote SHA, clean tree,
preserved ancestry and post-push evidence rehash. All seven consolidated historical
branch tips and prior owned fixes remain ancestors. No branch switch or primary push.

Four owned files: scripts/persistence_backup.py, the existing
tests/async/test_persistence_backup.py, this report and additive
PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. Exactly backup, status and
previous_completion_matches change; _completion_receipt_matches is added and used
by all three. One focused method is added to the existing test file. All prior
other functions/tests and the entire restore, remediation and original integration
modules remain exact. change-scope.json binds the AST/body scope and the two-file
source composition diff. Shared coordinator/contracts/producers, native recipes/
harnesses, registry/matrix, activation and migrations are not edited. Inactive
behavior, wallet-root ITEM_MONEY exclusions and the declined inactive spell path
remain. Prior owned fixes, including tombstone/manifest/drill/capture-age/schedule
receipt fixes, remain prerequisites in the tested 27-overlay composition.

Shared interface/schema requests: none. Existing fields version, completed,
generation, result and replica are authenticated consistently by the three named
consumers. Existing protected codes and retry-marker behavior remain. Operators
must inspect invalid evidence and clock state; this tool does not repair receipts
to manufacture a successful health or backup result. Installed host timers and
alert delivery are outside this slice's checks.

## Exact tested source

| Input | Identity |
| --- | --- |
| Refreshed/tested primary | 73e441704fe30a907d53746838f154533b55b292 |
| Fixed primary plus 27 owned overlays | 2f4057ebf4455c0c8f57e5419dceec63b1c5f2e7 |
| Fixed archive SHA256 | 4fd04480c03db0f3b3d4bbf5d5f4c08e87b9e3429f67153fea53f9663edfae87 |
| Native tree | 833d3085815b396861ad18a77635412212381e4b |
| Migration tree, canonical head 64 | 7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2 |
| Backup script blob | e20544ad8b0b327088f7319e57890c96eeec566a |
| Backup script SHA256 | 7c07ac3858a8eec796d387ad13f4a46b11b54b97eb367aac91dbaad5bc279e09 |
| Backup test blob | da1799db2744ec0025ceaa87ead19981b316d778 |
| Backup test SHA256 | f803f58123bcf613a652b242b632b5d889622b52ffa6851a81f86212d9645ce5 |
| Preserved restore blob | 064e40729da3e5371a86e898753ea6c657952ff7 |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

Source00 12f8efb8365d74d9b394dd340074508cd28d89f2 establishes the
unchanged defect; source01 f6e4a779c1aa399e403fca6bb14b477c0d282c5e
adds only the new regression. All passing stages use source02. Each transport
independently authenticates 6518 Git blobs, 6514 regular bodies/modes and four
links. Before/after guards bind source bodies, modes and link targets. Historical
local native source is not the published native test source. This composition
qualifies the bounded change here, not every included overlay or private candidate.

Raw AGENTS, README, Plan 5, finish plan, remaining requirements, latest checkpoint,
backup guidance and Craft/Forge source report are retained. AI_CONTEXT.md is absent
at the published revision; the primary-local notebook is user-confirmed nonblocking.
Refreshed primary 138c379e42940cdfdc0d077054b35b76f5bd2016 differs only in the retained nine
documentation paths; native/schema are exact. The new cold-restoration/Smith handoff
requires authentic cold vector order, UIDs, containment and frozen warm decisions,
without requiring historical global construction chronology. The private 9a7faa28
Craft/Forge/warm-census composition remains source-only. Smith compound ownership,
reset retry/constructor witness/cold restoration and special routes, major-plan
execution, activation and full release remain open. No private execution/import
qualification is inferred from documentation.

## Commands and results

Host Python3.12.10:
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe,
SHA256 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a.
Commands run from the owned worktree. Exact Docker, compiler, SQL and qualifier argv
are retained in stage docker-command.json, commands.json and launches.json. All
Docker stages disable network and mount D: outputs directly. Every command below
uses that host executable with -X utf8; launch.py takes stage and source label.

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/freeze.py 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/launch.py reproduce00 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/freeze.py 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/launch.py red01 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/freeze.py 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/launch.py units02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/authenticate.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/bind_reuse.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/launch.py cli02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/launch.py disposable02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/bind_scope.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/primary_refresh.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/qualify.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-completion-receipt/seal.py
```

- Unchanged reproduction: five malformed receipts in each of both modes, ten false
  healthy results and ten reaches of the real before_capture seam. The deliberate
  synthetic_capture_started exception stops before authority reads/mutation.
  Receipts and generations remain exact.
- RED: the new method has 122 failing subtests and two errors, zero skips. Its 31
  malformed receipt cases cover version, missing fields, boolean/float/string/
  container/negative/before-capture/future/nonfinite completion, invalid replica
  and mismatched pending result. Three consumers across both modes give 186
  subtests. Some previous-reader cases already refused correctly; no blanket claim
  that every new subtest failed on unchanged code.
- Full filesystem modules: 49 backup tests and 17 recovery-remediation tests,
  **66 PASS, zero errors/failures/skips**. New checks prove refusal before both
  capture and complete_generation, unchanged receipt/generation, and no staging.
  Only remember_blocked is mocked within the semantic matrix so its optimization
  cannot mask the next consumer. The CLI checks below use real protected markers.
  Canonical completion controls pass; existing interrupted publication, retention,
  replica failure/retry and subsequent fresh capture tests remain unchanged and pass.
- Actual CLI: **126 subprocess calls** across both modes: 124 fixed refusals and
  two valid status controls. Each action uses its own verified generation root so
  one refusal's marker cannot determine the next action's code. Exit, stdout,
  exact JSON stderr, receipt/generation preservation and no new staging/generation
  are asserted. Neither private replica alias nor traceback escapes. SQL dump
  bytes in this filesystem stage are explicitly mocked; this is not real SQL
  capture evidence.
- Disposable SQL: both original SQL backup methods run unchanged and **PASS,
  zero skips**, retaining original full schema/history/value/claim and corrupt-
  import controls. Two actual server boots reach Entering game loop. Each engine
  imports its verified generation into a fresh socket-only source, copies original
  journals and qualifies it. Six invalid receipt classes times both status/backup
  refuse on each engine: **24 refusals**, preserving receipts, generations, journals
  and selected SQL claims. Valid completion permits **two actual full SQL captures**
  and four valid status controls. Prior generation/journals remain exact. The
  original claim_capture bounds the unchanged source proof to ten selected tables
  and four critical inbox roots, not all producer or opening behavior. Engines:
  MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL8.0.46-0ubuntu0.22.04.4.
- Database qualifier calls: **50**, including
  **22 expected original refusals**. Fresh genuine native
  fixture GCC driver calls: **20**, comprising
  **4 compile/link calls** and
  **16 toolchain queries**. Original providers/compiler
  arguments remain exact. Nested compiler-internal counts are not inferred.
  Passing native stage has 9 regular build files; this is
  not a compiler job count. Maintained servers are authenticated reused binaries,
  not fresh builds: SQL SHA256
  1a7e305e8be3f516ae6778d6eb03fadcb14859e0876df6d279fc1d4bc75d937b;
  flatfile SHA256
  9462829d443f5e138feac60c30bb27c8ec81b921fadfb2862be4a5386e3b54be.
  Reuse bindings prove SQL 1318 repository/28 image and flatfile 1320 repository/20
  image inputs exact. Linux Python3.12.3, GCC13.3, nm2.42, mysql_config10.11.14
  executable hashes/versions are in the native seal inventory. No C/C++ edit.

## Evidence and remaining gates

Evidence: D:/Dev/Tests/Duris/accounting-plan5/completion-receipt-20261008.
Builds: D:/Dev/Builds/Duris/accounting-plan5-completion-receipt-20261008.
qualification.json, stage terminals/logs, cli-results.json, whole-results.json,
completion-observations.json, original assertions, source transport, scope,
dependency and native inventories bind these claims. Raw seal SHA256
92f14072e00ac6e3092c82f801195784549a2c299140516f3741eac93eb550f4 covers 1616 files/1,672,620,793 bytes,
13 native build regular files and 6
stopped non-OOM network-isolated containers. RED is retained. Native/host evidence
walks do not follow links or reparse targets. Post-push delivery rehashes every
sealed file and verifies all 27 overlay blobs and preserved tips against the result.

The full original 12-case module is not rerun: its original flatfile recipe still
lacks seven established genuine-provider symbols before nine missing-record
controls. The last 11 PASS/1 link-error result retains its own scope. No stubs,
relaxed compiler flags or shared recipe changes. Original native keeper/producer/
room UID boundaries, actual flatfile service/capture journeys, complete opening/
treasury/player/routes, fault/load/ACK/recovery, private combined candidate,
activation, full Plan 5/R1–R8 and release remain unqualified. No new publication
blocker; no production write, activation, promotion, deploy, merge, audit auto-
correction, primary push, host timer installation or cross-chat message.

This report and additive remote follow-up are curator-ready notebook input. The
primary-local notebook remains nonblocking; curator application, primary import
and acknowledgment are unclaimed. This bounded slice does not close release.
