# Plan 5 — refuse incomplete database qualification reports

Status: owned publication-boundary fix and Python acceptance complete; primary integration, genuine native/database acceptance and release qualification pending. Accounting inactive/CLOSED; release BLOCKED.

## Exact source and ownership

Branch/local remote: codex/accounting-plan5. Worktree: C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max. Owned parent: f515a13514d6cbfba15560cd00641b111fe6dbae. Refreshed primary experimental-accounting: fdb06def22df4677ace25b91f878c319822daf44. Tested composition tree: 82247910c00ff6ee01243a0afc921148de4c5b4d; complete archive SHA256: b66106633b267dfe02896fd41ba3a4e9dca1f1f9f81ee0f76010500bca73509e. Native tree: f109e01ea9e714e0e39a82324e6c918b48a76094; migration tree: 9ebcee47a19a6a380372f13621224bdd9cecc635. Actual 31 maintained owned file bodies overlay this pinned primary through an alternate index. All frozen regular bytes/modes and four symlink targets were guarded before/after observations. No merge/rebase/checkout or native compilation constructed the source. The result/remote SHA is bound in delivery/result.json after push.

Owned files: scripts/persistence_restore.py, tests/async/test_persistence_backup.py, this report and additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. No shared coordinator, producer, native authority, accounting contract, migration, registry/matrix or original native test changed. All seven historical branch tips and previous curator-follow-up bytes remain preserved.

## Established defect and complete publication fix

qualify_database_restore.main preserves room diagnostic findings in its successful component JSON, including room_item_full_runtime_authority_unqualified and room_item_retained_root_authority_unqualified. Those are missing authority evidence, not proof of full room recovery. persistence_restore.database_qualify previously discarded stdout and accepted process exit zero. Both calls before/after service replay could therefore return successfully and allow restore.restore to publish QUALIFIED.json, or a drill qualified receipt, while that authority was explicitly unqualified.

The frozen old wrapper reproduced 42 failing subcases across the two added methods, with no test errors/skips. The end-to-end controls reached qualification despite incomplete reports in both ordinary restore and drill, before and after service replay. Direct controls also showed empty, malformed, duplicate-field and incomplete success reports accepted by exit status alone.

The wrapper now consumes the existing JSON result with backup.strict_json, requires history/reconciliation to be the exact existing 'ok' statuses, refuses any room_item_diagnostics field with restore_room_item_authority_unqualified, and permits only the existing two-field success shape. Malformed/duplicate/unknown-field reports refuse invalid_database_qualification_report. Subprocess failure keeps its existing process error; it is not converted into a parsing finding. Both original database_qualify calls use this same gate. Ordinary failures receive only the existing FAILED.json marker; drill failures clean their disposable candidate and preserve the previous drill receipt. No QUALIFIED.json or new qualified drill receipt is written after refusal.

This reuses the original strict parser and failure/cleanup paths. Signatures, persisted generations, database schema and emitted verifier JSON are unchanged. The standalone room checker continues to report structural diagnostics without inventing original-root/runtime authority. The native baseline-clone harness already demands exactly the same two-field success result at run_native_sql_baseline_audit.py; the wrapper now applies that existing invariant to publication. No audit finding is cleared or auto-corrected.

## Existing result contract and consumer handoff

No new shared interface/schema is requested. Producer: qualify_database_restore.main. Existing fields: history and reconciliation, both string 'ok' for successful component checks; optional room_item_diagnostics contains findings when room authority is incomplete. Consumer: persistence_restore.database_qualify before and after service replay; its original return/signature and frozen schema/profile dispatch remain. Publication invariant: only the exact two-field result permits advancement; diagnostics present, missing/unknown fields, invalid representation or process failure never permit a qualified candidate.

The unchanged room component tests intentionally preserve diagnostic findings, including direct successful component JSON with warnings. Their exit zero is not full restore/release qualification. Original genuine native/SQL tests must exercise the wrapper's refusal while those findings exist, preservation of the whole candidate/source evidence, and an accepting case only after the primary supplies full original root/runtime proof and current independent acceptance. Do not suppress warnings or relax original source/receipt/UID/equipment/expected-state checks to make that case pass.

The primary owns actual shared registry/source-pin/matrix integration. Independent source derivation in this composition changes exactly backup.capture, backup.retention and restore.qualification definition coordinates, with all other fields in those records unchanged. No writer/evidence/backend status was upgraded. The published integration-boundary handoff remains scoped to its earlier owned/primary pair; this report supplies the newer two owned source versions for review, without inferring primary local adoption. Shared original test corrections and native wallet acceptance remain separate pending gates.

## Exact tests and commands

Backend: Linux Python 3.12.3 in pinned image sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a. Executable SHA256 e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f. Containers use network none, read-only roots, CPU2/memory2g, directly bound D: evidence, /work1GiB and /tmp256MiB disposable tmpfs. All SQL/client/service/process reports in new regression cases are mocked; no compiler/native/database/game process ran.

Focused preimage: python3 -u -B /evidence/observer-focused.py before, standard unittest loader with the two new RestoreTests methods; two methods, 42 failing subcases, zero errors/skips. Before tree 75f7d7f87f3479584a3faf9071bd4689d9f6c059, archive SHA256 46512568196e6ef71a7071b8590df7ffe650eba01ef0fecf86648f3022143628. The intermediate same focused selection after the fix passed both methods with no failures/errors/skips.

Full run: python3 -u -B /evidence/observer.py complete invokes unittest.defaultTestLoader.loadTestsFromName and TextTestRunner(verbosity=2) for the existing modules. test_persistence_backup: 64 methods, 0 failures, 0 errors, 0 skips, 235.655864 seconds. The prior 62 methods plus two new methods pass. test_sql_room_item_restore: 6 collected, five pure methods passed, zero failures/errors, one explicit native SQL skip, 0.038360 seconds. Skip: RoomRestoreSQLTests.test_canonical_both_engines_cli_capture_and_restore_refusals, requires isolated canonical room restore SQL services; DURIS_RUN_ROOM_RESTORE_SQL was not enabled. This is an outstanding native gate, not zero-skip qualification.

The new direct method covers both profile selections 64/65, four valid reports, 30 refused reports and two unchanged process failures. Twelve end-to-end cases cover two room authority markers and a missing status before/after service for ordinary restore and drill. They verify service ordering, no qualified artifacts, failed ordinary markers/drill cleanup, preserved previous drill receipt, and unchanged generations, live files, journals and tombstone evidence. The existing profile-dispatch fixture now emits the verifier's actual two-field successful JSON instead of an unchecked empty object; all original profile/frozen-manifest/refusal assertions remain.

Source commands in /work:

```text
python3 -B scripts/validate_data_lifecycle.py --json
python3 -B scripts/validate_runtime_compatibility.py
python3 -B scripts/validate_runtime_compatibility.py --schema65
python3 -B scripts/validate_economy_accounting.py --root /work
python3 -B scripts/validate_economy_accounting.py --root /work --release
python3 -B scripts/generate_economy_writer_coverage.py --check
```

Exits 0,0,2,0,1,1. Lifecycle 231/51/42 with destructive rules disabled; original 64 profile valid and 65 registered/unmeasured; explicit 65 refuses pending actual two-engine fingerprints. Ordinary accounting contract validity passes 14 fixtures/931 routes/2911 candidate sites, release_ready=False. Release validator refuses missing writer executable evidence; shared matrix check refuses stale coordinates. Exact derivation retains all three changed records in matrix-differences.json. Six original/variant selectors reference 140 unique immutable inputs with 780 step/apply/verify checksum comparisons. git diff --check passes. Collection/container exit zero never changes the negative preimage's failing assertions into a pass.

## Evidence and remaining qualification

Evidence: D:/Dev/Tests/Duris/accounting-plan5/restore-report-20261010. complete-source.json/complete.tar bind the current composition. complete-result.json and complete-test_persistence_backup-unittest.log/complete-test_sql_room_item_restore-unittest.log bind the two module results and exact skip. before/after focused logs preserve defect and repair observations. contracts-result.json, matrix-differences.json and migration-inputs.json retain separate source/release gates. All helpers, exact Docker argv, image/tool identities, raw logs and terminal states are retained. Seal SHA256 a028f2411eacd5cd2c031129278ffaa3b584359b3b1e8766d822414a7e4a7c45. Post-push delivery/result.json rehashes sealed regular members, checks exact 31 tested owned blobs, parent/result/remote SHA, four committed files, clean worktree, historical ancestry and terminal network-none/non-OOM containers. No archive/log/private data is committed.

Current primary full native forest-codec checkpoint remains source-only and explicitly defers compiler/executable tests until major-plan readiness. Actual MySQL8/MariaDB10.11 full fresh/populated original/variant migration/refusal/receipt/tamper/shell/compiled-startup matrix with tests/async/test_staging_migration_fork_mysql.py --update-contract --report <owned-json>, current schema65 measurements, both-profile maintained builds, native/restore/recovery/gameplay, budgets, durable erasure/retention proof and R1–R8 remain required on the exact combined candidate. ROOM full authority remains an implementation/acceptance prerequisite; this fix correctly refuses that missing proof and does not establish accepting ROOM recovery. Native wallet correction remains SOURCE_CORRECTED/NATIVE_ACCEPTANCE_PENDING. Earlier 0055, later isolated historical scopes, inventory or synthetic passes do not qualify this candidate or release.

This report and additive follow-up are curator-ready for the primary-maintained notebook. Notebook maintenance and externally handled Docker recovery are nonblocking; no import/application/adoption/acknowledgment is claimed. Inactive behavior, wallet-root ITEM_MONEY exclusions and the declined inactive spell path are preserved. No activation, deployment, PR merge, production operation, primary-branch push, cross-chat message or audit autocorrection occurred. Full Plan5 and release remain incomplete; the goal stays active.
