# Plan5 exact schema64/65 restore histories

Status: source/Python acceptance passed. Existing shared negative assertions need primary-owned updates. Native/database acceptance pending; accounting inactive, admission CLOSED, release BLOCKED and Plans1–5/R1–R8 incomplete.

## Exact delivery and source

- Branch codex/accounting-plan5; worktree C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max.
- Owned base ca24bb1b2c1b614b4ed103610bc80f8dcf67c3b5; result commit bound in the post-push receipt.
- Refreshed published primary 0d85b1a487a3920a0f41d309b977121eba78c28b; native tree 46ad5cfcc8ff4e30f399e7858e566ee429543f8f; migration tree 9ebcee47a19a6a380372f13621224bdd9cecc635.
- Tested composition tree 8996bf93171b51768cfb680c3335870a9f9cbb56; archive SHA256 b79b8398d42a73c524d64daec9b50c307529e34ca9b20cdfcc335c3a4c81c2fa;31 actual Plan5 overlays. Complete regular-file bytes/modes and link targets authenticate before and after each run. Published primary migrations/selectors remain unchanged.
- Owned implementation files: scripts/qualify_database_restore.py and tests/async/test_persistence_backup.py. The preserved room-item diagnostic path and every prior Plan5 reader remain. No shared native fixture/recipe, migration payload, accounting contract, registry/matrix, producer, activation or coordinator edit.

## Defect and resulting behavior

The previous restore selector accepted only the three current original manifests. With complete65 manifests published, it rejected authentic historical64 and all three nullable-default65 variants. Added regression controls reproduced nine rejection errors and one final nine-versus-three assertion failure; the other existing tests passed.

The existing function now bounds receipt count to64 or65 and chooses the three original selectors plus the three nullable-default selectors for65. Each selector loads through the existing strict immutable loader, and a historical prefix of the exact selected64/65 length passes through validate_applied_prefix. All seven original receipt fields are compared in order: migration_id,sequence,description,apply_checksum,verify_checksum,compatibility,runner_version. Partial or66-plus histories cannot become complete by truncating unknown rows. The original MysqlExecutor.applied() first verifies exact recorded applied_count and full framed history checksum; that state validation is unchanged. Schema64 uses only original selectors and does not require reading65 variants.

This accepts exactly nine unique authentic histories in the published candidate: three64 and six65. Existing lifetime/replay/economic/room integrity gates still execute separately. Passing immutable history is not sufficient economic restore qualification.

## Proof and limitations

Backend: Python3.12.3, pinned image sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a. Docker network none, root read-only, D: evidence directly mounted, source and scratch in tmpfs. SQL outputs are mocked; no DB/client connection, native, compiler/preprocessor, build, game, real restore/recovery or performance execution occurred.

Existing module invocation: unittest.defaultTestLoader.loadTestsFromName('test_persistence_backup'), unittest.TextTestRunner(verbosity=2), through the frozen source observer.54 methods passed,0 failures,0 errors,0 skips:51 existing methods plus three focused history acceptance/refusal methods.

The new positive method exercises12 selector/count combinations representing nine distinct histories and the real MysqlExecutor.applied state-joining code with mocked output. All six full65 checksums are pinned exactly to the primary handoff. Negative controls cover80 receipt histories: for each of six selectors, empty,63-prefix,66-overflow,missing-middle,reordering,seven individual receipt-field edits,terminal verify-checksum edit, plus mixed staging0045 and master0031 lineage. Corrupt histories are refused even when their synthetic recorded count/hash is recomputed consistently. Five recorded-state controls separately reject missing/short state,wrong count,zero digest and the64-prefix digest paired with65 receipts.

Source commands in /work:

```text
python3 -B scripts/validate_data_lifecycle.py --json
python3 -B scripts/validate_runtime_compatibility.py
python3 -B scripts/validate_runtime_compatibility.py --schema65
```

Exits0,0,2. Lifecycle231/51/42 with destructive rules disabled; default runtime preserves64 and reports65 registered/unmeasured; explicit65 correctly refuses pending actual two-engine measurement. git diff --check passes.

Two unchanged original shared controls were also executed with loadTestsFromNames: ImmutableMigrationRunnerTest.test_restore_accepts_all_complete_supported_histories and test_restore_rejects_partial_mixed_and_edited_histories in test_immutable_migration_runner. Two methods ran; the positive method passes, and the negative method has two failing subcases because authentic64 is now supported. No assertion was rewritten independently. Collection containers exit0 after preserving results; that does not mean the shared negative assertions passed.

Evidence: D:/Dev/Tests/Duris/accounting-plan5/schema65-history-20261010. after-final-backup-unittest.log and after-final-result.json bind final54-method acceptance. before-backup-unittest.log preserves the old selector defects. shared-controls-shared-unittest.log preserves original shared assertion failures. Source archives/manifests, observers, exact Docker/CLI argv, raw logs and terminal states are retained. Seal SHA256 e4b3f882489282a26a01ce4460ce9d688a2c918a6ec4dcc6683313cb10350766; post-push receipt rehashes every sealed member, binds result/remote commit and owned blobs, clean worktree and all seven preserved historical tips.

## Narrow shared handoff

The function signature and AppliedMigration/state schemas are unchanged. Consumers: qualify_database_restore.main(), the existing runtime_migration_history_fixture.py preceding native boot predicates, and original migration-runner restore tests. No runtime contract or metadata fingerprint change is requested.

Primary-owned test changes required in tests/async/test_immutable_migration_runner.py:

- test_restore_rejects_partial_mixed_and_edited_histories: original canonical missing_head rows=canonical[:-1] and master_missing_head rows=upgraded_master[:-1] now contain64 authentic receipts. Use a genuinely unsupported63 prefix for those refusal cases; separately retain positive64 cases for all three authentic lineages and add the three65 variants.
- test_restore_closes_session_on_success_and_history_refusal: its incomplete executor uses rows[:-1] (64); use63 for the intended refusal path and retain successful64/65 session-close/value-domain tests. This follow-on assertion mismatch is established by direct source review, not claimed executed by this slice.
- Preserve empty/common44/staging45/master31,mixed lineage,edited old receipt,overflow,recorded state and session-close checks. Do not relax full receipt comparison or state conservation.

These original tests are primary-owned and remain unedited. This is an integration/review gate, not evidence of a failed economic migration or fabricated DB qualification. The backup/restore runtime-profile dispatch handoff remains separate unfinished Plan5 work. The major-plan batch must run the actual six-lineage two-engine matrix, original native/DB restore/upgrade, recovery/gameplay/budget gates and all required combined candidate checks.

## Curator delivery

Add this report and the additive remote follow-up to the primary-maintained notebook using its curator workflow. Remote publication claims neither import/application/acknowledgment nor primary adoption. Notebook and external Docker recovery are nonblocking for this work; no cross-chat message, primary push, activation, deploy or production action occurred.
