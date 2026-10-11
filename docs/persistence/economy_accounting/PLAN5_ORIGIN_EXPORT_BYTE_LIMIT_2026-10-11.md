# Plan 5 origin export byte limit — 2026-10-11

Status: origin-CLI emitted-file byte-limit defect reproduced and fixed. Exact frozen component acceptance passes; native/combined/release qualification remains open. This companion issue is committed separately from the already-published snapshot-export fix5afb2af41337bc387b79b2d6956827e3eaa5c798.

## Established defect and complete fix

read_origins_in_transaction() checks its compact JSON body against MAX_INPUT_BYTES. The origin CLI adds a newline before writing without a final bound. The original source returns exit0 for a33,554,432-byte body and emits33,554,433 bytes, exceeding its existing audit-input bound. preimage.json records that actual32MiB observation, output hash and closed connection. This is an output-protocol defect, not a claim about native origin correctness.

main() now checks the complete encoded JSON-plus-newline size before os.open and raises the existing OriginError/refusal exit2 for oversized output. A32MiB output remains accepted. Oversized output is refused before any creation attempt; absent output remains absent and existing output remains exact. The existing exclusive-create behavior, owner-only mode request and connection cleanup remain intact.

The production change is two lines in the Plan5 independent origin reader. Only main() changes; all21 other origin function/method AST bodies are exact. Original SQL capture, transaction/rollback, opening grammar, account/UID fields, protected captured input, EAB1/EAB2/source interpretation and native authority remain unchanged. The prior snapshot-exporter fix is byte-preserved. No schema, producer/accounting contract, API signature or mutation implementation is changed.

The existing SnapshotCliTests.test_output_byte_limit_includes_final_newline_before_file_creation is parameterized for two actual CLI callers, avoiding another duplicate fixture. It now covers16 controls: both exporters, four body sizes around the bound, absent/existing output. Correct capture arguments, connection closure, final bytes/JSON, exclusive creation, refused creation and existing output preservation are verified. The other16 original UID-scope functions/methods are exact, including13 existing tests. The original test_economic_sql_audit_origins.py native/component fixture is byte-preserved; an existing captured-opening CLI method executes unchanged.

## Branch, owned files and exact source

Worktree: C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max. Branch: local/remote codex/accounting-plan5. Base: 5afb2af41337bc387b79b2d6956827e3eaa5c798. Result commit is bound by Git and delivery/result.json; no self-referential commit hash is embedded here.

Owned files are scripts/economic_sql_audit_origins.py, tests/async/test_economic_sql_uid_scope.py, this report and an additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md entry. The origin reader is within Plan5's independent reader scope. Original native fixtures and all shared coordinators, contracts, producers, migrations, registry/matrix and activation remain primary-owned and unedited.

Refreshed/pinned primary: 83e2e4392064667fc4f64780183d17b5acc431ef. Tests use that source plus32 actual owned file overlays through an alternate Git index, without checkout/merge/rebase. Before repair, with extended regression: composition e906782042bf381662cdc6eb051537899a847975, archive SHA256 cf77f1567f0b67e3ed1d1bb8f12ba5045ac818d9a64b0c7a84467f7826757d88. After repair: composition 242002a191c195fe148fd79174ba750be63570f7, archive SHA256 7a696e526e74520cf11f340c278a480d8cf79a361aa657a9d3c4ef3c501f86aa. Primary native tree df568142d3c57d74683bed76da2a0eec67d4a326; migrations tree 95aa09d334004bc048dec65affa2cdd86a702cd6. Those tree identities are source bindings, not executable acceptance.

Component archives contain scripts, both test modules and accounting documentation; each has770 regular files. Whole-byte source guards pass before and after every invocation and the actual-size journey. No native/game/world/build artifact is executed from these component archives. All32 actual owned overlay blobs match tested source before publication. Other30 owned implementation/test blobs and all7 historical tips are preserved. No credentials, local environment files or player/account data are included.

## Exact commands, backend and results

Backend: Windows Python3.12.10, executable SHA256 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a. SQL connection/capture are mocked in the new controls; real origin/snapshot CLIs and file operations execute. The original captured-opening method retains its existing synthetic client/input checks. No actual database is connected.

Commands from the frozen after-source directory:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B tests/async/test_economic_sql_uid_scope.py SnapshotCliTests -v
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B tests/async/test_economic_sql_uid_scope.py -v
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B tests/async/test_economic_sql_audit_origins.py CapturedItemBindingTests.test_captured_opening_operator_read_boundary_and_protected_input -v
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B -X utf8 D:/Dev/Temp/accounting-plan5-origin-output-limit-20261011/actual-boundary.py
git -c gc.auto=0 diff --check
```

The first focused command also runs against the frozen before-source. freeze.py records exact primary/overlay tree and archives via git -c gc.auto=0 read-tree/hash-object/update-index/write-tree/archive. run.py records each command, working directory, source, exit, elapsed time and full log. reproduce.py records original actual32MiB behavior. seal.py authenticates source/function preservation, all results and the retained evidence.

| Check | Result |
| --- | --- |
| Original origin CLI at32MiB body | Exit0;33,554,433-byte output; defect reproduced |
| Extended regression before origin repair | One method; four origin subcases fail; snapshot controls pass;0errors/0skips |
| Extended regression after repair | One method/16 controls PASS;0failures/0errors/0skips |
| Complete UID-scope module | 14/14 PASS;0failures/0errors/0skips |
| Original captured-opening CLI method | 1/1 PASS;0failures/0errors/0skips; source unchanged |
| Actual origin output at32MiB | Exit0;33,554,432 bytes; exact JSON round-trip |
| Actual origin output at32MiB+1 | Exit2; no creation attempt; prior output preserved or absent |
| Existing output at accepted size | Exit2 under original exclusive-create rule; prior bytes preserved |
| Source guards | Both770-file inventories exact;32 live overlay blobs exact |

The actual-size journey has four controls crossing exact-limit/one-byte-over with absent/existing output. All pass. Expected old-source failures are retained; no helper failed. These fixtures prove complete output byte bounds, not actual native opening/birth/lifecycle acceptance or an all-clear. No C/C++ changed. Compiler/preprocessor/native builds, actual MySQL/MariaDB services/clients, migrations, gameplay/recovery and performance/budget tests are NOT_RUN. The conditional POSIX0600 assertion is not executed on Windows. No test is silently skipped; no Linux permission proof is claimed. No container/service is created, started, stopped or leased; Docker recovery remains handled elsewhere.

## Narrow integration and curator handoff

No new interface or schema is requested. Invariant: complete emitted output, including its delimiter, fits the existing MAX_INPUT_BYTES bound. Consumers retain the existing origin-export format and diagnostics; no original evidence, unknown state or audit finding is corrected or promoted.

The primary-owned integration row plan5_recent_uid_scope still needs the prior handoff: append SnapshotCliTests.test_output_byte_limit_includes_final_newline_before_file_creation to required_cases and arguments before -v, preserving the original selected UID case and all execution controls. The same named method now covers both CLIs; no second registration row is required. Refresh applicable existing source pins/matrix under primary ownership. Run the named component commands on the final frozen combined candidate, with Linux mode verification and original native database/restore/recovery gates in the major qualification batch.

Evidence: D:/Dev/Tests/Duris/accounting-plan5/origin-output-limit-20261011. Retained logs: before-focused-unittest.log, after-focused-unittest.log, after-complete-unittest.log and original-captured-opening-unittest.log. Exact archives/manifests, preimage.json, actual-boundary-result.json, source-checks.json, implementation.diff and helper versions are sealed. Seal SHA256 a55e1482dfab9b96d19608e93b531db4ba4bd89f95533d6edd36f8f47c3465d1; 1565 files/73139867 bytes. Post-push delivery/result.json binds base/result/remote, four committed files, tested32 blobs, other30 preserved bodies, seven ancestors, exact prior follow-up prefix and rehashed members.

Report and additive follow-up are curator-ready for the nonblocking primary-local notebook. Notebook import/application/acknowledgment and primary adoption of this new slice are unclaimed. Accounting remains inactive/CLOSED; wallet-root ITEM_MONEY exclusions and declined inactive spell change are preserved. No primary-branch push, activation, production operation, deployment, PR merge or audit autocorrection occurs.

No blocker prevented this finite fix. Full Plan5/R1-R8/release still requires the maintained combined candidate, remaining reader/custody/operator integration, native-wallet acceptance, actual schema65 measurements, original SQL/flat backup/restore/recovery/gameplay evidence, durable six-source erasure/retention, predeclared fault/resource/load budgets and writer/activation owner evidence. Component output acceptance does not close those gates.
