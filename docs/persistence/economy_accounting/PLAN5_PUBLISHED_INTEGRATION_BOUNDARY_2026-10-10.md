# Plan 5 — published integration boundary

Status: source comparison complete; combined candidate and release qualification unproven. This is a read-only integration handoff, not a new implementation fix or executable qualification. Accounting remains inactive/CLOSED and release BLOCKED.

## Exact source and scope

Owned branch/local remote: codex/accounting-plan5. Worktree: C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max. Owned implementation head and parent for this report: a3b04e5db4d58442d11a71599282ce6dcf2654eb. Verified remote implementation head: the same SHA. Refreshed primary experimental-accounting: fdb06def22df4677ace25b91f878c319822daf44. Primary native tree: f109e01ea9e714e0e39a82324e6c918b48a76094; primary migration tree: 9ebcee47a19a6a380372f13621224bdd9cecc635. The result/remote documentation commit is bound in the post-push receipt. This slice owns only this report and additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md.

The current Git objects show 11 of 31 checked Plan5 paths absent from the primary commit and 17 with different file bodies. Three paths match mode/type/blob exactly: tests/async/test_economic_sql_uid_scope.py, tests/async/test_item_equipment_reconciliation.py, tests/async/test_plan5_child_identity.py. These 31 paths are the actual maintained owned overlays used by the last frozen source/test packet, not a whole-repository ownership claim. A difference establishes an exact-source mismatch; it does not prove every existing different implementation is defective, nor reveal the primary's unpublished local candidate or curator adoption. No primary file was edited, no alternate branch was used and no merge/cherry-pick occurred.

## Exact owned versions for primary review

These are existing published source versions for integration/review. Preserve newer primary work and apply only reviewed owned deltas; this table does not authorize replacing primary/shared files wholesale. Blob IDs are complete Git object identities. comparison-modes.json also records modes/types for all 31 paths, including the three equal files.

| Owned path | Published Plan5 blob | Published primary blob |
|---|---|---|
| migrations/data_lifecycle_manifest.json | 45a6e5ca1d79b693fbc66e8cc2f9b7512f57a588 | f90f85447d9040653ab2091ca5e09031ae7600ab |
| scripts/economic_item_payload_audit.py | 0aef0a342626a77a86e31c523ede5e95c9e67ee0 | absent |
| scripts/economic_room_restore_evidence.py | c6803020249dae18f353a65c7abe8a0a315002e8 | absent |
| scripts/economic_sql_audit_snapshot.py | b090121997ec8acf1fbc6eb47ccff30dca309449 | ba7e446cbaf59f3d4e3ef7ae80d420383dd79629 |
| scripts/persistence_backup.py | 542ceb7845d06b994565a86c3a8725b02d8c4817 | a60e9c64f2687f5503f3bea6c0f293602e15b7f8 |
| scripts/persistence_restore.py | ab54e1282c5ae36892e862e023577ae2e3e74cf5 | 23b17a3d158ac455105de1cd1303561dcc028ad8 |
| scripts/qualify_database_restore.py | e59b5c96b45df333c1decb971102ad481427570b | 6b8f0c2057b93ca6426c1ecddf16c64a647b222d |
| scripts/qualify_flatfile_economic_authority.h | 8b0c51679771b4108a635762efa4ef13aac05871 | 593272f3ab668e970c0f561deed60cb3ba11a232 |
| scripts/reconcile_economy_accounting.py | 4e004b5042cd06fe757f45a7a3df4a277214e72f | 436d892980819c0a55692ae01a980c0a88a996ec |
| scripts/validate_data_lifecycle.py | d0c330ecad32a80c3dda39d3c0bccf380eba7c35 | a93edf784d4a3dace2813494f8cac110c2b1b2b3 |
| tests/async/run_economic_sql_audit_snapshot_mysql.py | 322b30a68c36095a5494daba9feddd6b4dad2a6f | 3df0a81c96cb380677f4ed2b63bf2048b96b5085 |
| tests/async/run_plan5_retention_journeys.py | 722404e4377a70a0e666328dd45902236399c49e | 915826708723c8d0bc11d5c64bf6228ea114527a |
| tests/async/run_restore_accounting_evidence_mysql.py | ff6293257cc7c7f27b9d732be8f6a88fed0473ab | 6fa61150ae763e4f112473d353b22ef9a4e3d281 |
| tests/async/run_saved_item_recovery_journey.py | 01c73781bc5ed4bdb86a3a35d901a031fcf759b3 | b490a839c61ae2fb2b9af670a30ebfa9538675c3 |
| tests/async/test_data_lifecycle_manifest.py | 77de10bab6ea3fd58df6f1d23fa8fe24e470d5e1 | 2288ec4a6f31d4487398bd756815cd32ca660e92 |
| tests/async/test_persistence_backup.py | 6212534d2da1764b71200462c1ffa3e1e0bb517f | 8a60947a0fb801ff5782b0c9b17c5e9781e94955 |
| tests/async/test_persistence_backup_integration.py | d1ef31c19e79228e994e5c764a8553875db0f86e | a1243f146d4023e7ba4a73d073f61683d2441436 |
| tests/async/test_reconcile_economy_accounting.py | 97ac730d9830fd15fb010ccdd2717ea667497741 | 9724db788daf5bb42eda3308687cceb738d76e3b |
| tests/async/test_restore_economic_coin_effects.py | f46125cca129f6e94e9f8dbf876d9e75f353bd57 | 3659eecf7126342d04f21cbc9bd3e1a71ef2a81e |
| tests/async/test_sql_auction_custody_audit.py | 56739f00864bb8058e9741871a1c6fa9ebbddedb | absent |
| tests/async/test_sql_corpse_custody_audit.py | 88281147ccea9d41212be815ef479ae7d625212d | absent |
| tests/async/test_sql_locker_custody_audit.py | ada7769056c0e071dfe28b1c89f317899d97ee9d | absent |
| tests/async/test_sql_player_custody_audit.py | ceea680583725a36d550a3b273f96450720e89d4 | absent |
| tests/async/test_sql_room_item_custody_audit.py | b5271dce048496452a610a32ab61061d9cb7635c | absent |
| tests/async/test_sql_room_item_restore.py | 42796d6984002a15f49ae631e8aef2dbb3145818 | absent |
| tests/async/test_sql_saved_ground_custody_audit.py | 39331565e527278792d5042245b2790565523bb4 | absent |
| tests/async/test_sql_shop_custody_audit.py | a6693c44cfa8cb673fd2e7f7d2c1d402d3236e79 | absent |
| tests/async/test_sql_siege_custody_audit.py | d7d7f08094f7d9932fce8961c5fefbbf0ea2388b | absent |

Two independent reader modules and nine focused custody/restore test files are absent from this published primary. This prevents treating the primary commit as the exact source qualified by the Plan5 frozen composition. Existing body differences likewise require integration review and current acceptance; neither inventory matching nor source-count equality substitutes for executable proof.

## Narrow interfaces, invariants, consumers and tests

No new schema, field, API or wire change is requested by this report. The following previously published owned changes need review on the eventual combined candidate:

- Lifecycle commit ca24bb1b2c1b614b4ed103610bc80f8dcf67c3b5 registers the table introduced by migration 0065 and the 231-table lifecycle inventory, retaining reset birth-origin evidence and excluding canonical_origin/terminal_publication_context from personal export. Original quest-origin and core economic evidence protection remain exact. Consumers: lifecycle validation, backup/restore policy and enabled export/erasure inspection. Source acceptance: existing test_data_lifecycle_manifest module: 23/23 on its recorded primary 7262 scope. See PLAN5_SCHEMA65_LIFECYCLE_COVERAGE_2026-10-10.md.
- Restore-history commit 80db4cec8da297483799307092f96469b762b733 accepts three authentic 64 histories and six authentic 65 histories through exact ordered seven-field receipts plus the existing recorded-state checks. No partial/mixed/edited receipt relaxation. Consumer: qualify_database_restore.require_completed_history and its existing MysqlExecutor.applied caller. Source acceptance: test_persistence_backup: 54/54 on its recorded primary 0d85 scope, including 80 receipt/five state controls. Primary-owned test_immutable_migration_runner negative 64 cases must use truly partial 63 rows, preserve authentic 64 positives, and retain session-close/refusal checks. See PLAN5_SCHEMA64_65_RESTORE_HISTORY_2026-10-10.md.
- Operator-profile commit b68c452774cb6ee3cc0e7948893c6b38fe03a01d uses additive SQL generation runtime_schema_profile, exact JSON integer64/65; missing legacy field means 64. Frozen schema hash, selected 230/231-table dump list and before/capture/after profiles must agree. Restored full state/history must authenticate that profile before and after replay; unmeasured 65 refuses. Consumers: backup capture/verify/retention and persistence_restore.database_qualify; original env-only calls remain supported. New generations retain all six original/variant migration selectors and 134 distinct payloads. Source acceptance: test_persistence_backup: 59/59 on recorded primary f384 scope. See PLAN5_SCHEMA64_65_OPERATOR_PROFILE_2026-10-10.md for exact signatures and invariants.
- Archive-coherence commit a3b04e5db4d58442d11a71599282ce6dcf2654eb binds staged migration/schema bytes to their captured preimage, refuses drift before publication/pruning and validates archived complete selectors/checksums/exact membership read-only. Legacy generations without archives remain supported. Consumers: backup generation creation and independent verify/retention/restore readers. Source acceptance: test_persistence_backup: 62/62 on frozen primary 8490 composition 8892587fe6fc4061a026ec23fd70396c1f7e8ec1, 0 failures/errors/skips. It is not acceptance of the newer primary fdb source. See PLAN5_MIGRATION_ARCHIVE_COHERENCE_2026-10-10.md.

The already-published native wallet locator correction 2b7c1fe85df5f260b7788304cec7cd0d31edfd16 remains SOURCE_CORRECTED/NATIVE_ACCEPTANCE_PENDING. It admits only kind 1/context 12/locator 7/nonzero ID below MAX/empty name for that genuine native-wallet location. Primary-owned original native fixtures must exercise genuine acceptance and all refusals. Other PC/bank/escrow/claim/treasury grammar and wallet-root ITEM_MONEY exclusions remain exact.

After integrating actual file versions, the primary owns registry/source-pin/matrix refresh. The last 8490 composition independently identified backup.capture, backup.retention and restore.qualification definition-coordinate changes without upgrading any other record field; this report did not regenerate a matrix for fdb or claim those are its only possible changes. Original function-owned shared source-test requests remain open. Shared producer/authority/schema repairs and activation stay with the primary.

## Read-only commands and authoritative evidence

Backend: host Python3.12.10 and Git; comparison uses immutable Git objects and a live remote ref read. Exact operations: git -c gc.auto=0 fetch --no-tags origin experimental-accounting; git -c gc.auto=0 rev-parse HEAD; git -c gc.auto=0 status --porcelain; git -c gc.auto=0 ls-remote origin refs/heads/codex/accounting-plan5; git -c gc.auto=0 ls-tree -r <owned-and-primary-SHAs>; git -c gc.auto=0 rev-parse <owned-SHA>:<each31-path>; git -c gc.auto=0 show <primary-SHA>:<current-review-documents>. Result: clean owned branch/remote exact, 31 checked paths, 11 missing/17 different/three exact. The complete actual tree inventory distinguishes absent paths from different bodies. An initial path-only rev-parse stopped at the first absent file; the complete inventory comparison succeeded and makes no executable-test claim.

Current FINISH_ACCOUNTING_PLAN.md, REMAINING_REQUIREMENTS.md and EXPERIMENTAL_REVIEW_CHECKPOINT.md remain authoritative incomplete requirements. The new COMPLETE_NATIVE_TRANSFORM_FOREST_RECOVERY_CODECS_2026-10-10.md explicitly says compiler/executable testing remains deferred to major-plan readiness and full selecting-host/foreign ownership/producer/publication/running-drain/native32MiB/gameplay/persistence/recovery/R1–R8/release remain OPEN. Its raw source bytes are retained; this existing primary document decodes as CP1252, and no encoding repair was made to shared source. Review-source SHA256 5286f2f779a9ad18e6a42aff96a320e60c05647c4feafa91a6edf8d531d5bb24.

Evidence: D:/Dev/Tests/Duris/accounting-plan5/current-integration-audit-20261010-a3b04e5-fdb06de. comparison.json binds full body identities; comparison-modes.json binds mode/type/blob and all 31 classifications; summary.json binds counts and current native/migration trees. The three current coordinator/review documents and original codec checkpoint bytes are retained with task helper versions and host-tool identities. Evidence seal SHA256 2ae1f7fc874606aea03390b57d0549a830b03dddb7e30be4fff73d97cbb05b05; post-push delivery/result.json binds the two committed documentation files, parent/result/remote SHA, rehashed sealed members, preserved seven historical tips and clean branch. git diff --check is required before commit.

No executable suite, native/compiler/preprocessor/build, SQL client/database, gameplay/recovery or budget test was run for this documentation-only audit. Previous executable/source packets remain limited to their recorded source and backend. No current schema 65 metadata measurement, original0056/0065 combined qualification or release completion is implied by this comparison. Actual MySQL8/MariaDB10.11 full migration matrix with tests/async/test_staging_migration_fork_mysql.py --update-contract --report <owned-json>, original native/flat/SQL restore/recovery/gameplay acceptance, durable erasure/retention proof, budgets and R1–R8 remain required on the same combined candidate. The explicitly diagnostic room reader continues to report absent full-runtime and retained-root authority instead of granting unproven birth-origin acceptance.

## Curator and completion boundary

This report and additive remote follow-up are curator-ready for the primary-maintained notebook; notebook maintenance and externally handled Docker recovery are nonblocking. No notebook import/application/acknowledgment, primary adoption or local-candidate state is claimed. No primary branch push, deployment, production operation, PR merge, accounting activation or audit autocorrection occurred. Inactive behavior, declined inactive spell path and existing source remain preserved. The goal remains active and full Plan5/release completion unproven.
