# Plan 5 SQL audit output byte limit — 2026-10-11

Status: emitted-file byte-limit defect reproduced and fixed; focused component acceptance passes on the exact frozen source below. This does not qualify actual SQL/native authority, the combined candidate or release. Accounting remains inactive/CLOSED and release BLOCKED.

## Exact defect and behavior

The SQL exporter capture() measures its compact JSON body against the32MiB reconciler input limit. main() then appends a newline and writes the resulting file without checking its complete size. On the original owned source, an exact33,554,432-byte body returned exit0 and produced33,554,433 bytes; the real reconciler returned exit2 with snapshot or output limit exceeded. Thus a successful export could be unreadable by its intended consumer.

The complete fix adds a final encoded-byte check before os.open. The existing serialized JSON plus newline must fit MAX_INPUT_BYTES. An oversized result returns the existing refusal exit2 before creating an output file. A correctly bounded result retains the original exclusive-create behavior and owner-only mode request. Existing output is preserved, and the SQL connection is closed before the final check. SQL queries, transactions, findings, partial-evidence declarations, schemas and authority rules are unchanged.

Only main() changes in the production exporter; all42 other function AST bodies are identical. All16 original functions/methods in the UID-scope test module remain exact, including all13 prior test cases. One new method covers eight controls: body sizes limit-2,limit-1,limit,limit+1 crossed with absent/existing output. It checks actual CLI return, bytes, JSON, preserved output and no creation attempt on oversized data. The POSIX0600 assertion is conditional and was not executed on Windows; no POSIX permission acceptance is claimed.

## Branch, owned files and frozen source

Branch: local/remote codex/accounting-plan5. Worktree: C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max. Base: ed57174e251611fcc16065c828397c64260c00ee. Result commit is bound by Git and post-push delivery/result.json. No other branch is used or pushed.

Owned files:

- scripts/economic_sql_audit_snapshot.py — two-line final output-size guard.
- tests/async/test_economic_sql_uid_scope.py — existing module extended by SnapshotCliTests.test_output_byte_limit_includes_final_newline_before_file_creation.
- This report and an additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md entry.

Refreshed primary: 2f7bd2326213531f704d28d70f9bc7bb4d419680. Test source is primary plus31 actual maintained owned file overlays, frozen through a separate alternate Git index without checking out, merging or rebasing the primary. Before repair, with the new test: composition 285d122600cbdeb618f115d9e746d46a6add09e6, archive SHA256 83ecd11902e9961f2f91b27ce53948c22b989fba608178cea54c7e7c541f1770. After repair: composition 7e33436eb6dfe0fe1c6cf3e1cbc51a914121dbbd, archive SHA256 68cac7d55d576f289ed40eb1f21ec2b3e5c548238e06388a2e3b581c5f1b42e5. Primary native tree af5592499ce8297c723ee7f2635cc45a337fc885; migrations tree 95aa09d334004bc048dec65affa2cdd86a702cd6. These tree identities bind source only; no native code was executed.

The component archives contain scripts, the UID-scope test module and accounting documentation needed by the reader. They contain768 regular files each and deliberately omit unrelated game/native/world/build content. Whole byte inventories stayed exact before and after each invocation; all31 actual owned overlay identities also matched before publication. No compiled artifact, credential, player/account data or local environment file is included.

## Commands, backends and results

Backend: Windows Python3.12.10, executable SHA256 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a. The new cases mock SQL capture and the client connection; they execute the actual exporter CLI and file operations. The actual-size journey also executes the unmodified reconciler CLI and retains its findings. This is component output-protocol acceptance, not native SQL or player qualification.

Commands, from the corresponding frozen before/after source directory:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B tests/async/test_economic_sql_uid_scope.py SnapshotCliTests -v
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B tests/async/test_economic_sql_uid_scope.py -v
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B -X utf8 D:/Dev/Temp/accounting-plan5-export-byte-limit-20261011/actual-boundary.py
git -c gc.auto=0 diff --check
```

Exact collector commands are recorded in each result JSON. freeze.py before/after uses git -c gc.auto=0 read-tree, hash-object, update-index, write-tree and archive against the pinned primary/owned overlays. run.py records exact subprocess command, source, backend, result, elapsed time and full log. reproduce.py and reproduce-roundtrip.py preserve original actual32MiB observations; source-checks.py checks unchanged functions and current central registration; seal.py hashes the evidence before documentation/publication.

| Observation | Result |
| --- | --- |
| Original actual32MiB body | Export exit0; output33,554,433 bytes; real reconciler exit2 due byte limit |
| New focused test against original source | One collected method; four failing subcases;0errors/0skips; exit1 |
| New focused test after repair | One method/eight controls PASS;0failures/0errors/0skips; exit0 |
| Complete existing UID-scope module after repair | 14/14 PASS;0failures/0errors/0skips; exit0 |
| Actual32MiB output after repair | Export exit0; exactly33,554,432 bytes; JSON round-trip exact; real reconciler exit1 with evidence_loss and other partial-evidence findings, no byte refusal |
| Actual32MiB+1 output after repair | Exit2;0file creation attempts; absent output remains absent; existing output remains exact |
| Existing file at accepted size | Exit2 under original exclusive-create rule; prior bytes preserved |
| Production source comparison | Only main() changed;42 other exporter functions exact; original test functions preserved |

The actual-size journey exercises four controls: exact-limit/one-byte-over output crossed with absent/existing output. Its accepted fixture remains backend=sql_partial and complete=false; exit1 is the required retained diagnostic result, not an all-clear. There were no helper failures in this slice. The expected old-source four failures and old exporter/reconciler mismatch remain retained.

No C/C++ changed. Compiler/preprocessor/native builds, actual MySQL/MariaDB clients/services, migration/native SQL acceptance, gameplay/recovery and load/budget tests were NOT_RUN. SQL dispatch and query bodies are unchanged. Those broader combined gates remain scheduled under primary ownership at major-plan readiness. No test was silently skipped; the Windows-only run does not exercise the conditional POSIX mode check. No container or service was created, started, stopped or leased. Docker recovery is handled elsewhere and does not block this pure output fix.

## Narrow primary handoff

No new API, schema, wire fields or producer interface is requested. The invariant is that a successful emitted JSON file, including its final newline, fits the existing MAX_INPUT_BYTES consumer bound. Consumer: reconcile_economy_accounting.main and any existing operator reading that output. Existing incomplete/diagnostic findings remain unchanged.

Primary-owned tests/integration_manifest.json currently selects only UidScopeTests.test_legacy_previous_owner_nulls_stay_unknown_and_partial_values_refuse in row plan5_recent_uid_scope. Append SnapshotCliTests.test_output_byte_limit_includes_final_newline_before_file_creation to that row's required_cases and to arguments before -v; preserve the existing case/argument, engines, mode, environment, timeout and coverage. Run the maintained runner's list/registration validation and then both selected cases in the combined batch. Refresh existing source registry/matrix pins as required; do not independently edit shared registration or matrix here.

The proof needed after integration is the same focused/full component commands on the published frozen combined candidate, with the Linux conditional mode assertion and applicable original native SQL/restore/recovery gates in that candidate's qualification batch. No accounting activation or audit autocorrection is authorized.

## Evidence, curator and remaining gates

Evidence: D:/Dev/Tests/Duris/accounting-plan5/export-byte-limit-20261011. Logs: before-focused-unittest.log, after-focused-unittest.log and after-complete-unittest.log. Exact source archives/manifests, preimage.json, preimage-roundtrip.json, actual-boundary-result.json, source-checks.json, implementation.diff and publication-controls.json are retained. Seal SHA256 28eb5bddaffffb2915ab69e1aea61e2245a89cca9adb441c10007a25074ff712, 1564 regular files/72673178 bytes. delivery/result.json separately binds base/result/remote, four committed files, tested implementation blobs, other29 preserved owned blobs, all7 historical ancestors, exact prior follow-up prefix and rehashed sealed members.

This report and additive follow-up are curator-ready for the primary-local notebook, which is nonblocking. No notebook import/application/acknowledgment or primary adoption of this new fix is claimed. Existing sources, inactive behavior, wallet-root ITEM_MONEY exclusions and the declined inactive spell-path change remain preserved. No primary-branch push, production operation, deployment or PR merge occurs.

No blocker prevented this finite fix. Full Plan5 completion still requires primary integration of remaining independent room/custody/operator slices, native-wallet acceptance, actual schema65 measurements, original SQL/flat backup/restore/recovery/gameplay journeys, durable six-source erasure/retention evidence, predeclared fault/resource/load budgets, writer/activation owner evidence and R1-R8/release qualification on the same combined source. This passing component slice does not replace any of those requirements.
