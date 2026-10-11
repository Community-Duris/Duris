# Plan 5 published adoption review — 2026-10-11

Status: independent source adoption comparison complete. This is a documentation-only handoff; combined executable and release qualification remain open. It updates the October 10 published integration boundary with demonstrated primary adoption, while retaining its historical source and evidence.

## Branch, source and owned files

Worktree: C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max. Delivery branch: local/remote codex/accounting-plan5. Base: 73e11477f0ccf2784f0c4439abaf57ab8a231061. Refreshed and pinned primary: 58880a1ef453db2a94c989f06b9ef0c4984ff363. The resulting documentation commit is bound by Git and the post-push delivery/result.json receipt; no self-referential result hash is embedded here.

Owned changes in this slice are this report and an additive entry in PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. All 31 maintained owned implementation/test blobs and all seven historical branch tips are preserved. No shared coordinator, producer, migration, registry, matrix, activation or original native fixture is edited. AI_CONTEXT.md is absent from the complete pinned primary tree inventory; existing review and requirement documents are retained without inventing it.

The pinned primary native tree is fd48c7a6baecb9185d8958af88a18e69416cb64c; migrations tree is 95aa09d334004bc048dec65affa2cdd86a702cd6; immutable migration subtree is 304ca67f080c727b4ef3f1e17c3a872c20b790f5. The current runtime manifest and complete immutable subtree exactly match prior tested primary fdb06def22df4677ace25b91f878c319822daf44. This dependency match is source evidence; it does not carry prior executable results forward to the changed combined candidate.

## Adoption actually established

The current primary reports explicitly adopt the lifecycle, exact schema64/65 history, frozen runtime profile, migration archive coherence and strict qualification report slices. Independent Git/AST comparisons establish:

- Lifecycle manifest, lifecycle validator and lifecycle test module are exact mode/type/blob matches to the owned branch. Their previous 23/23 pure result retains its original source/backend scope.
- Seven selected production helpers have identical parsed function bodies and identical function source hashes: backup.runtime_schema_profile, backup.database_runtime_profile, backup.migration_capture, backup.validate_migration_archive, backup.verify_database_schema, restore.database_qualify and qualifier.require_completed_history. AST equality excludes source locations and does not prove dependency behavior or native execution.
- All 13 new operator test methods from the history/profile/archive/report slices match the final owned test AST: 3 history, 5 profile, 3 archive and 2 report cases. The profile dispatch method was subsequently corrected on the owned branch to supply the real success report; comparison against its original earlier slice correctly differs. The final owned and current primary versions match.
- Primary-owned original migration-runner test repairs are documented and the actual current file is retained. The earlier request to stop missing-head negatives at63, preserve authentic64/65 positives and repair stale terminal expectations is resolved at source level. Those suites have not been executed on this candidate; the source repair is not a passing-test claim.

Across the 31 actual owned paths, six are exact full mode/type/blob matches, 14 existing paths differ and 11 are absent. The absent paths remain the two independent room modules and nine custody/component test modules. Differences are an integration inventory, not 14 established defects. Full per-path Git blob IDs, raw SHA256s and per-function comparisons are in comparison.json and methods.json.

| Owned path | Current primary source state |
| --- | --- |
| migrations/data_lifecycle_manifest.json | exact |
| scripts/economic_item_payload_audit.py | absent |
| scripts/economic_room_restore_evidence.py | absent |
| scripts/economic_sql_audit_snapshot.py | different |
| scripts/persistence_backup.py | different |
| scripts/persistence_restore.py | different |
| scripts/qualify_database_restore.py | different |
| scripts/qualify_flatfile_economic_authority.h | different |
| scripts/reconcile_economy_accounting.py | different |
| scripts/validate_data_lifecycle.py | exact |
| tests/async/run_economic_sql_audit_snapshot_mysql.py | different |
| tests/async/run_plan5_retention_journeys.py | different |
| tests/async/run_restore_accounting_evidence_mysql.py | different |
| tests/async/run_saved_item_recovery_journey.py | different |
| tests/async/test_data_lifecycle_manifest.py | exact |
| tests/async/test_economic_sql_uid_scope.py | exact |
| tests/async/test_item_equipment_reconciliation.py | exact |
| tests/async/test_persistence_backup.py | different |
| tests/async/test_persistence_backup_integration.py | different |
| tests/async/test_plan5_child_identity.py | exact |
| tests/async/test_reconcile_economy_accounting.py | different |
| tests/async/test_restore_economic_coin_effects.py | different |
| tests/async/test_sql_auction_custody_audit.py | absent |
| tests/async/test_sql_corpse_custody_audit.py | absent |
| tests/async/test_sql_locker_custody_audit.py | absent |
| tests/async/test_sql_player_custody_audit.py | absent |
| tests/async/test_sql_room_item_custody_audit.py | absent |
| tests/async/test_sql_room_item_restore.py | absent |
| tests/async/test_sql_saved_ground_custody_audit.py | absent |
| tests/async/test_sql_shop_custody_audit.py | absent |
| tests/async/test_sql_siege_custody_audit.py | absent |

The operator suite still lacks these eight existing owned test methods. Their associated completed fixes retain their earlier handoffs and tested scope; this review does not import broader scheduling/metadata/drill code or infer unpublished primary-local state.

- `GenerationTests.test_status_capture_age_is_nonnegative_and_within_rpo`
- `GenerationTests.test_required_drill_receipt_rejects_malformed_completion`
- `GenerationTests.test_current_completion_receipt_is_valid_before_health_or_capture`
- `GenerationTests.test_schedule_requires_nonfuture_integer_completion`
- `GenerationTests.test_schedule_preserves_due_boundary_and_failure_receipt`
- `CapacityAndInputTests.test_metadata_growth_after_size_observation_is_bounded`
- `CapacityAndInputTests.test_metadata_open_rechecks_guards_after_path_validation`
- `RestoreTests.test_drill_only_defers_for_current_qualified_integer_receipt`

## Existing narrow integration requests and authority limits

No new API, schema, wire format or interface change is requested by this slice. The remaining original requests retain their existing consumer and test boundaries:

1. Integrate the independent room reader/restore producer as a reviewed owned slice. read_room_item_custody emits the five bounded raw collections room_item_payloads, room_item_proofs, room_item_roots, room_item_members and room_item_seasons plus exact room_item_custody_coverage. capture_room_items adapts the existing SQL executor; require_room_item_integrity preserves structural findings and returns the explicit full-runtime/retained-root diagnostics. Consumers are the independent SQL exporter/reconciler and qualify_database_restore.main. Nonempty room authority remains unqualified; the now-adopted database_qualify consumer must refuse diagnostic reports before and after replay, before QUALIFIED.json or a qualified drill receipt. Existing test_sql_room_item_custody_audit.py and test_sql_room_item_restore.py supply the corresponding component controls and two-engine native cases. No mutation implementation should be imported into the reader, and no absent birth/publication authority should be inferred from payload consistency.
2. Native wallet locator acceptance remains SOURCE_CORRECTED/NATIVE_ACCEPTANCE_PENDING on the owned branch and absent from the pinned primary header. Exact condition: kind1/context12 requires locator type7, native ID greater than0 and less than UINT64_MAX, and an empty name. Consumer: qualify_flatfile_economic_authority.h locator and original native restore lifecycle checker. Primary must add the genuine original native accepted case and refusals for zero/MAX ID, wrong type/context and nonempty name before native acceptance. The ordinary PC/bank/escrow/claim/treasury grammar and wallet-root ITEM_MONEY exclusions remain unchanged.
3. Publish one frozen combined candidate for actual schema65 and original restore/recovery/gameplay acceptance. schema65 remains qualification=unmeasured, normalized_metadata_fingerprints.mysql8=null and mariadb10_11=null, current_table_count=231. Actual fresh/populated/original/nullable-variant migration, refusal, receipt, shell and compiled-startup matrix is still required on MySQL8 and MariaDB10.11 via tests/async/test_staging_migration_fork_mysql.py --update-contract --report <owned-json>. Table inventories and literal fingerprints do not qualify it.

The primary's recorded production SQL/flat builds and inactive flat boot passed only frozen3049fb7712de7ccd957c61d5d345d2c8a4cf838d. That checkpoint predates newer providers and the pinned58880a1ef candidate; SQL service smoke was NOT_RUN. Preserve that successful scoped evidence without presenting it as current Plan5 or release acceptance.

The new architecture requirement preserves independent Plan5 ownership and narrow shared handoffs. Its required focused builds/executable checks apply to maintained extractions under primary ownership. No architecture extraction is performed here. Current primary Plan5 integration reports continue to schedule affected test suites at major-plan readiness.

## Commands, results and evidence

Backend: host Windows Python3.12.10, Git2.40.1.windows.1, immutable Git objects, stdlib ast/hashlib/json only. Python executable SHA256: 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a. Evidence directory: D:/Dev/Tests/Duris/accounting-plan5/adoption-20261011-73e1147-58880a1-v2.

Commands executed from the owned worktree:

```text
git -c gc.auto=0 fetch --no-tags origin experimental-accounting
git -c gc.auto=0 status --short
git -c gc.auto=0 rev-parse HEAD
git -c gc.auto=0 ls-remote origin refs/heads/codex/accounting-plan5
git -c gc.auto=0 ls-tree -rz <owned-or-pinned-primary-SHA>
git -c gc.auto=0 show <pinned-SHA>:<each-source-or-review-path>
git -c gc.auto=0 hash-object <each31-owned-path>
git -c gc.auto=0 merge-base --is-ancestor <each7-historical-tip> <owned-SHA>
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-adoption-20261011/audit-v2.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-adoption-20261011/checks-v3.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-adoption-20261011/seal.py
```

Result: 31 complete path comparisons, 13 adopted operator test AST matches, seven selected helper source/AST matches, 31 live owned blob guards, seven preserved historical ancestors, unchanged prior-test runtime/immutable inputs and exact clean owned remote. Final helpers exit0. Three earlier helper attempts exit1: incorrect AST Module iteration, comparison to an earlier historical test body rather than the final owned body, and an incorrect table-count key. Originals and the initial partial capture are retained; helper-attempts.json records corrections. These are evidence-collector failures, not executable regression results.

Seal SHA256: ead2731b69c32caec68ed3d10894a5ce9cdee2ae6ddf49fc88fd59c15f2771fb; 83 regular files,7,523,666 bytes. The seal includes the actual source/review snapshots, full comparisons, controls, helper versions and failed first capture. delivery/result.json separately binds final parent/result/remote SHAs, two documentation blobs, preserved implementation blobs/ancestry, exact prior follow-up prefix and rehashed sealed members. git diff --check must pass before commit.

No executable regression suite, compiler/preprocessor, native build, SQL client/database, gameplay, recovery, load or budget test ran for this documentation slice. There are no newly skipped tests because no suites were launched. Earlier 64/64 operator and five pure room passes/one explicit native room SQL skip remain at composition82247910c00ff6ee01243a0afc921148de4c5b4d over primaryfdb06def, documented in PLAN5_RESTORE_REPORT_QUALIFICATION_2026-10-10.md; they do not qualify58880a1ef. Zero containers were created, started, stopped or leased; no resource release is claimed. User-confirmed Docker recovery is handled elsewhere and is not treated as a blocker to this source review.

## Remaining completion and curator boundary

The unresolved gates are full primary integration of independent room/custody and omitted operator slices; actual65 measurements and native locator acceptance; original SQL/flat migration/backup/restore/recovery/gameplay journeys on the frozen combined source; durable erasure propagation across all six source classes and retained-evidence continuity; predeclared resource/fault/load budgets; complete writer/activation owner evidence; and R1-R8/release qualification. Synthetic or isolated source/component results do not remove them.

This report and additive follow-up are curator-ready input for the primary-local notebook, which the user confirms is nonblocking. Primary repository adoption is established only for the exact source described above; notebook import/application and human acknowledgment are unclaimed. Accounting stays inactive, admission CLOSED and release BLOCKED. No audit autocorrection, production data change, activation, deployment, primary-branch push or PR merge occurs. The declined inactive spell-path change remains declined. The full Plan5 goal remains unfinished.
