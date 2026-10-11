# Plan 5 JSON nesting refusal and transition handoff — 2026-10-11

Status: the already-started malformed-input slice is fixed and focused acceptance passes. Publish its receipt, then pause the unchanged full Plan5 goal at the user-approved transition boundary. Do not select another adjacent slice until the replacement orchestrator/assignment is provided. Full Plan5/R1-R8/release completion remains unproven; accounting inactive/CLOSED.

## Defect, fix and exact owned source

An8,239-byte JSON input with excessive nesting passed the file-byte guard but json.loads raised RecursionError outside the CLI's controlled refusal path. The original reconciler produced a traceback and exit1, which otherwise denotes audit findings. The invalid input should refuse with exit2 before any audit or view. Actual original CLI/file/parser behavior and unchanged input hash are retained in preimage.json.

Only the JSON decoding call now catches RecursionError and raises SnapshotError with snapshot JSON nesting exceeds parser limit. The existing refusal handler returns exit2 without stdout or traceback. No audit/domain error is broadly caught, no recursion limit is raised and no new parser/scanner is introduced. All56 other reconciler function/method bodies and all202 existing test functions/methods are exact. Original finding exit1, valid exit0, duplicate fields, growing/oversized input and file preservation remain covered. The UID/origin output-bound fixes and their existing regression are byte-preserved.

New existing-module method: ReconciliationTests.test_cli_refuses_excessive_json_nesting_before_any_view. It exercises two real nested JSON shapes through all7 operator views and limits0/1/100:42 subprocess controls, exact refusal message, empty stdout and unchanged input bytes. These are actual operator CLI/file/parser executions, with no SQL or parser mock.

Branch: local/remote codex/accounting-plan5. Worktree: C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max. Base 0446c40d658b57f110e7bdc66b8d52aaba3bbf33; result is bound by Git and delivery/result.json. Owned files in this slice: scripts/reconcile_economy_accounting.py, tests/async/test_reconcile_economy_accounting.py, this report and an additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md entry. No shared maintained file, original native fixture, schema, registry/matrix, producer or activation owner is edited.

Refreshed/pinned primary 83e2e4392064667fc4f64780183d17b5acc431ef; primary native tree df568142d3c57d74683bed76da2a0eec67d4a326; migrations tree 95aa09d334004bc048dec65affa2cdd86a702cd6. Component source is that primary plus32 actual owned overlays through an alternate index, without checkout/merge/rebase. Before composition c49f60430220bd7353fb9c26fe1900b4b4b03029, archive SHA256 43c6483c308df44365a0986d355fde4bb88fdd9a14df74b1654fd94c12c49efe; after composition 3af5fac01ed3deb0030b19a4de9941c1202b295a, archive SHA256 c50952565cf23f0042cc85aa5d078791c31d8cd055e7cefc6d50778deb0ee3d2. Each component archive has771 files and contains scripts, three relevant test modules and accounting documentation; unrelated native/world/build content is omitted. Exact source-byte guards pass before and after every invocation. Tested32 blobs, other30 preserved owned files and all7 historical ancestors are verified at publication.

## Executed proof and limits

Backend: Windows Python3.12.10, executable SHA256 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a. Focused command against both frozen before/after directories:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B tests/async/test_reconcile_economy_accounting.py ReconciliationTests.test_cli_refuses_excessive_json_nesting_before_any_view -v
```

The applicable reader controls run together on after-source:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B tests/async/test_reconcile_economy_accounting.py ReconciliationTests.test_cli_refuses_excessive_json_nesting_before_any_view ReconciliationTests.test_cli_rejects_duplicate_snapshot_fields_before_any_view ReconciliationTests.test_snapshot_growth_after_size_check_cannot_bypass_byte_limit ReconciliationTests.test_oversized_snapshot_and_collection_refuse ReconciliationTests.test_cli_bounded_exception_result -v
git -c gc.auto=0 diff --check
```

| Proof | Result |
| --- | --- |
| Original actual CLI | Traceback/exit1 on8,239-byte nested JSON; input exact |
| New method on frozen old source | 42 failing subcases;0errors/0skips; one collected method |
| New method on repaired source | All42 controls PASS;0failures/0errors/0skips |
| Five relevant reader methods | 5/5 PASS;0failures/0errors/0skips; valid/findings/refusal behavior retained |
| Source fidelity | Only main changes;56 other functions and202 old test functions exact;32 live blobs guarded |

run.py/run-v2.py preserve exact commands, source, elapsed time and full logs. One copied collector backend label initially mentioned prior SQL mocks; its original JSON remains retained, and metadata-correction.json records the correction to actual CLI/file/parser. No test-result fields changed and no helper failed. Expected old-source42 failures remain retained.

Compiler/preprocessor/native builds, MySQL/MariaDB clients/services, migrations, original SQL/flat restore/recovery/gameplay and fault/load/budget suites were NOT_RUN. No C/C++ changed. No broader full-suite or POSIX acceptance is claimed; only the five named methods were selected and none skipped. Component parser refusal does not qualify native authority or release.

## Bounded transition inventory

The human approved the five-session plan in Assess accounting launch readiness and instructed the current Plan5 owner to finish the already-started malformed-input slice, publish it, then pause before selecting more work. The trusted read of the approval and preceding proposal is recorded in transition-authorization.json. This report is the completion/handoff for that named slice; it preserves the broad objective and all previous fixes/evidence.

Current dirty files after the verified commit/push: none. Current Python jobs9436 and67523 are terminal. No current native build/DB/game job or resource lease was started by this slice. Read-only Docker inspection against the exact five IDs/images in the prior restore-report receipt confirms all five containers exited/not running; they remain preserved. No container, service, volume or lease belonging to another owner was changed. No resource release is claimed where none was acquired. handoff-resources.json records the actual states and IDs.

Completed independent interfaces/source slices:

- Schema64/65 exact seven-field histories; frozen profile dispatch and legacy64 compatibility; six-selector archive capture/coherence; strict two-field qualification-report consumer before/after replay; schema65 lifecycle protection. Primary source adoption is documented in the earlier published adoption review; native combined acceptance is still pending.
- Independent read-only UID/account/source/custody components, including bounded room structural diagnostics. Room findings remain explicitly full-runtime/retained-root unqualified.
- Recent UID and origin output bounds: separately published5afb2af41337bc387b79b2d6956827e3eaa5c798 and0446c40d658b57f110e7bdc66b8d52aaba3bbf33. Their recent frozen module14/14,16 CLI controls, original captured-opening1/1 and actual32MiB observations remain at their recorded source/backend. This slice preserves them.
- This parser refusal fix and its42 controls/five reader methods.

Remaining interfaces/gates verified against pinned primary: economic_item_payload_audit.py and economic_room_restore_evidence.py remain absent; native-wallet locator exception remains absent/native-unqualified; schema65 is unmeasured, with both fingerprintsnull and231tables. Full retained-root/runtime/birth/publication authority, original native backup/restore/recovery/gameplay, durable six-source erasure/retention, predeclared resource/fault/load budgets, complete writer/activation ownership and R1-R8/release remain required. In-memory tombstone modeling or an empty fixture does not prove erasure propagation across database_backup, pfile_backup, conversion_backup, journal_replay, cache_rebuild and export_spool.

No new API/schema/interface is requested for this fix. Shared test-registration handoff: append ReconciliationTests.test_cli_refuses_excessive_json_nesting_before_any_view to required_cases and arguments before -v in the primary-owned offline plan5_recent_item_history row, preserving every existing case/control. Existing UID/origin named CLI registration handoff and source-pin/matrix refresh remain primary-owned.

## Recommended first native qualification step after transition

The most useful next step is actual two-engine schema65 measurement/startup on one frozen combined candidate. First obtain the orchestrator's exclusive resource grant/job lease and exact pinned image/toolchain identities, task-specific D: source/build/evidence mounts and cleanup receipt. Current native runner names mysql:8.0.46 and mariadb:10.11; resolve and record actual image identities/versions under the grant before starting. Use one heavy job at a time initially, consistent with the approved plan, and preserve other owners' services/volumes.

On an isolated candidate source, run the existing full command:

```text
python3 tests/async/test_staging_migration_fork_mysql.py --update-contract --report <owned-schema65-report.json>
```

It requires both engines in one qualification, not --lock-only or a single --disposable-loopback shortcut. Preserve fresh/populated/original/nullable-variant histories, refusal/tamper/receipt/session/shell and compiled startup evidence. Capture failures rather than bypassing them. Apply --update-contract only to the private frozen candidate; maintained shared contract changes return to primary as a narrow measured handoff. Exact existing fields are schema65.normalized_metadata_fingerprints.mysql8, schema65.normalized_metadata_fingerprints.mariadb10_11 and schema65.qualification in migrations/runtime_compatibility_manifest.json, plus RUNTIME_SCHEMA65_MYSQL8_METADATA_FINGERPRINT, RUNTIME_SCHEMA65_MARIADB10_11_METADATA_FINGERPRINT and RUNTIME_SCHEMA65_QUALIFIED in src/core/runtime_compatibility_contract.h. Preserve all schema64 fields, immutable payloads/receipts and table coverage. No measured value or qualified flag may be manufactured.

Then run default/explicit-schema65 compatibility validation and original SQL/flat restore qualification against that same measured source/binaries with genuine nonempty authority. Full room proof still needs the missing reader/producer integration and retained-root/runtime authority; an empty or structurally consistent fixture cannot close it. This recommendation is a handoff; no native command or shared contract update executes in the current transition slice.

## Evidence and disposition

Evidence D:/Dev/Tests/Duris/accounting-plan5/json-nesting-20261011; seal SHA256 0fec964e2dbfc1d49166a727aa5161a07c9f9705146f0dae2f754c04779e3fd0; 1574 files/74671758 bytes. Retained logs are before-focused-unittest.log, after-focused-unittest.log and reader-controls-unittest.log. Exact archives/manifests, preimage traceback/input hash, source checks, interface/resource/approval handoffs and helper versions are sealed. delivery/result.json binds base/result/remote, four committed files,32 tested blobs,30 preserved bodies, seven ancestors, exact prior follow-up prefix and rehashed members. Goal pause occurs after that receipt; its actual returned status is reported in the task.

Report/follow-up are curator-ready for the nonblocking primary-local notebook; import/application/acknowledgment and new-fix adoption are unclaimed. The Plan5 owner is retained and awaits the replacement orchestrator/transition prompt. No adjacent slice is started. Full objective remains unchanged and unfinished, not marked complete. Inactive behavior, wallet-root ITEM_MONEY exclusions and declined inactive spell change remain preserved. No audit autocorrection, accounting activation, production operation, deployment, PR merge or primary-branch push occurs.
