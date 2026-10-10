# Plan5 schema64/65 operator profile and migration-input retention

Status: Python source/operator acceptance passed; native/database release acceptance pending. Accounting remains inactive, admission CLOSED, coverage incomplete and release BLOCKED. Plans1–5/R1–R8 remain unfinished.

## Exact source and delivery

- Branch codex/accounting-plan5; worktree C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max. Owned base 80db4cec8da297483799307092f96469b762b733; result commit is bound in the post-push receipt.
- Refreshed published primary f3846d344b2678f94e8e1bb28ab2a56d099b47fb; native tree 80e43e17e47239ced3d99b6a0bc4f0869eee3eac; migration tree 9ebcee47a19a6a380372f13621224bdd9cecc635.
- Tested composition tree e9a7ed603af3cb85d6783b08454d39b64fe4ac64; archive SHA256 e5d5b4707e94f24bec7bfe76ed1269ad5dd7d1403667ef5631db8f5273544ed5;31 actual Plan5 overlays. Full regular-file bytes/modes and link targets authenticate before/after the tests and CLI commands. No private primary inputs were imported.
- Owned bodies: scripts/persistence_backup.py, scripts/persistence_restore.py, tests/async/test_persistence_backup.py, this report and additive follow-up. Shared runtime contracts, migrations/selectors, original native fixtures/recipes, coordinators, writer registry/matrix and activation owner were not edited.

## Established defects and complete owned fix

Backup/restore invoked only the schema64 shell profile, so a valid schema65 database could never dispatch to its separate contract. The generation did not record that selection, dump coverage used only the230-table top-level list, and original/variant migration inputs were absent from backups. Source review and the added preimage tests establish those gaps. A separate focused legacy64 test reproduced restore_requires_matching_runtime because the additive nested65 object changed the whole runtime-manifest hash while all64 fields remained identical.

Use the recorded database state's applied_count only to select64 or65; malformed,unsupported or multiple-row outputs refuse. The maintained shell verifier then authenticates the selected complete history, recorded state, baseline, schema predicates and engine fingerprints. Count alone grants no restore authority. Schema65 requires qualification=measured and both actual fingerprint slots present in the frozen contract; the published candidate remains unmeasured and refuses. No measured constants were generated or changed.

Backup verifies the frozen schema before and after capture. Capture independently selects and records runtime_schema_profile; all three selections must match, or no generation publishes/prunes. Dump verification requires the selected230/231 table list. Restore binds both pre/post-service database qualifications to the generation's exact frozen schema and recorded profile. Direct existing database_qualify(env) callers remain supported and select from that isolated database's recorded state.

For SQL64 only, an older frozen runtime file may differ from the current file solely by the additional nested schema65 object: every remaining64 field must compare exactly. The full file remains hashed in its generation inventory, all64 fields and subsequent real schema/history checks remain required, and a changed64 fingerprint refuses before a candidate database starts. Schema65 and flatfile runtime-file matching retain the existing whole-file hash requirement.

Every new generation also retains the six exact original/variant selector files and all referenced apply/verify inputs under migrations/. Existing strict immutable loaders validate source references/checksums, copied bytes must match those checksums, and capacity/free-space limits apply before each copy. These are archival owner-only600 files and700 directories; no archived script executes or migration applies. Existing retention/replication covers them in the generation's checksummed inventory. Original and variant inputs remain separate, with no rewrite/backfill. Current source:140 unique inputs=6 selectors+134 distinct payloads. Tests compare all780 selected step/apply/verify references and exact selector bytes, and corrupted archival input is refused.

## Narrow generation/interface handoff

The additive owned generation field is runtime_schema_profile, JSON integer64 or65, emitted for SQL generations only. Invariants: reject bool/string/null/unsupported values; missing field means legacy64; frozen runtime_schema_sha256 remains exact; selected dump table list matches the field; before/capture/after database profiles match; restored full recorded state/history must authenticate that same field before and after service replay. It is not an activation flag, epoch, baseline, native financial proof or measurement receipt. Generation version remains1 and older valid64 generations are preserved.

Consumers: persistence_backup.mariadb_capture produces it, backup.verify/retention/replication/checksummed-generation consumers carry it, and persistence_restore.restore passes it with the frozen runtime-schema.json into database_qualify. The latter keeps its original positional env call and adds optional schema/profile arguments. Existing native integration callers using only env automatically select from their isolated database. Review this additive operator-generation interface in the primary's combined candidate; no shared schema or production migration request is introduced.

Primary must refresh its owned registry pins and generated matrix after integrating actual Plan5 blobs. Independent source derivation finds exactly three changed route records: backup.capture,backup.retention,restore.qualification; matrix-differences.json preserves both published/computed records. No writer coverage/evidence/backend status was upgraded by Plan5. The shared immutable-runner negative64 assertions and session-close63 input request from PLAN5_SCHEMA64_65_RESTORE_HISTORY_2026-10-10.md remain outstanding. Earlier original source-contract test corrections and native wallet locator acceptance remain separate primary/native gates.

## Exact commands and observations

Backend: Python3.12.3 in pinned image sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a, network none, root read-only, task D: evidence mounted directly, /work and /tmp tmpfs. All SQL/client/service results in operator tests are mocked synthetic data. No native, compiler, preprocessor, maintained build, DB/client connection, game, actual restore/recovery or performance-budget execution occurred.

Existing module: unittest.defaultTestLoader.loadTestsFromName('test_persistence_backup'), unittest.TextTestRunner(verbosity=2) through the authenticated source observer.59 methods passed,0 failures,0 errors,0 skips:the prior54 plus five focused acceptance methods. It covers64/65 dispatch,frozen manifest propagation,unmeasured/unsupported refusal,profile mismatch and capture drift,no publication/pruning on failure,full231-table coverage,legacy64 metadata/contract compatibility,changed64-fingerprint refusal,and original/variant archival preservation/tamper rejection. Synthetic measured flags exist only in disposable unit JSON to exercise dispatch/coverage; they do not establish actual measurements.

CLI commands in /work:

```text
python3 -B scripts/validate_data_lifecycle.py --json
python3 -B scripts/validate_runtime_compatibility.py
python3 -B scripts/validate_runtime_compatibility.py --schema65
python3 -B scripts/validate_economy_accounting.py --root /work
python3 -B scripts/validate_economy_accounting.py --root /work --release
python3 -B scripts/generate_economy_writer_coverage.py --check
```

Exits0,0,2,0,1,1. Lifecycle231/51/42 with destructive rules disabled; runtime64 retained and65 registered/unmeasured; explicit65 required refusal pending actual two-engine measurement. Ordinary accounting contract validity passes14 fixtures/931 writer routes/2911 candidate sites,release_ready=False. Release validator refuses writer has no executable evidence. Matrix check refuses stale generated coordinates; the three exact changed records are retained for the primary owner. git diff --check passes. No isolated/source pass is treated as release completion.

Evidence: D:/Dev/Tests/Duris/accounting-plan5/schema65-profile-20261010. complete-result.json and complete-backup-unittest.log bind the59-method pass. contracts-result.json and matrix-differences.json bind separate source/release gates. before results preserve11 failures/7 errors from the old code plus initial fixture mocking; two intermediate58-method runs preserve four fixture-mocking failures. The dispatch test now stops its existing verifier stub to exercise the real owned function. compat-before preserves the concrete whole-file hash refusal on a legacy64 generation. Capture accounting was refined to accumulate staged bytes once rather than rehash the growing stage for every input; all source checksum/capacity guards remain. Recorded unit timings are not native performance qualification.

Seal SHA256 3dd0bae8b1e43ce2cf3a701d8bd709b2694f9f8b153af7509c831d4debf01434. All frozen source archives/manifests, helper versions, raw logs, exact Docker argv and terminal states are retained. A post-push receipt rehashes sealed members and binds result/remote SHA,exact31 tested owned blobs,clean worktree,all seven historical ancestor tips and terminal owned containers. Collection exit0 does not convert any failing preimage or intermediate test into a pass.

## Remaining release gates and curator workflow

The primary must integrate/review these actual owned slices,update shared source/test bookkeeping,and publish the combined candidate. The human-directed major-plan batch must run the existing complete two-engine matrix with tests/async/test_staging_migration_fork_mysql.py --update-contract --report <owned-json>, then ordinary/explicit65 validators and original upgrade/restore/recovery/gameplay checks on that exact candidate. Fresh/populated original and variant lineage,receipt conservation,refusal/tamper controls,compiled startup,actual native audits/custody,journeys,memory budgets,retention/erasure evidence and R1–R8 remain required. Fingerprint constants alone or these synthetic operator tests cannot qualify release.

This report and additive remote follow-up are curator-ready for the primary-maintained notebook. Publication claims no import/application/acknowledgment,primary adoption or combined qualification. The notebook and externally handled Docker recovery are nonblocking. No cross-chat message,primary push,activation,deploy,merge,production data operation or audit autocorrection occurred. Wallet-root money exclusions and the declined inactive spell path retain their exact owned source bodies.
