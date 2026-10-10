# Plan 5 — coherent migration archives and read-only verification

Status: owned source fix complete, independent Python acceptance passed; primary integration, native/database acceptance and release qualification pending. Accounting remains inactive/CLOSED; release BLOCKED.

## Branch, exact source and ownership

Branch/local remote: codex/accounting-plan5. Worktree: C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max. Owned parent: b68c452774cb6ee3cc0e7948893c6b38fe03a01d. Refreshed primary experimental-accounting tested: 8490c935139a2c34e93d6e346195b887d075dc26. Composition tree: 8892587fe6fc4061a026ec23fd70396c1f7e8ec1. Complete archive SHA256: 3058adf55e2b546e9098c23d58a122395d7035011d8dfb50ef45ed07ef382921. Native src tree: 72635d0a2434389bdeb5c85afeae17e69a7b8332; migration tree: 9ebcee47a19a6a380372f13621224bdd9cecc635. An alternate index overlays the actual31 maintained Plan5 bodies on this primary. Authenticated full regular-file bytes/modes and four symlink targets were checked before and after observations. No checkout, merge, migration or native compilation constructed this source.

Owned files in this slice: scripts/persistence_backup.py, tests/async/test_persistence_backup.py, this report and additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. No shared coordinator, accounting contract, producer, authority, migration, registry, matrix, activation owner or original shared test changed. The result/remote commit SHA is recorded in the post-push delivery receipt because this report is part of that commit. All seven preserved historical branch tips remain ancestors; follow-up preserves its previous bytes as a prefix.

The primary's newer pure currency/restitution CURRENT source observer companions are present in the frozen source, with their existing methods preserved. This slice runs no compiler/preprocessor/native test and does not qualify those companions or establish financial proof. Their primary review remains source-only with the major-plan native batch deferred. AI_CONTEXT.md is absent from current source inventory; no replacement or invented content was introduced.

## Established defect and complete owned correction

After migration capture, before manifest inventory creation, checkpoint after_capture could change a staged immutable payload, remove a selector, add an unexpected migration input or change runtime-schema.json after schema verification. The old code inventoried those changed bytes and published a complete generation. A checksummed generation inventory alone also accepted an internally inconsistent immutable migration archive when its inventory was recomputed: it never validated archived selectors and their referenced checksums.

migration_capture now returns its expected path/SHA256/byte-count mapping from validated source bytes actually written. backup compares staged runtime-schema.json with its saved digest and staged migrations with that saved mapping after capture and before manifest publication. Source changes refuse with schema_changed_during_capture or migration_archive_changed, preserve prior generations and skip retention pruning. Existing cleanup removes the unpublished stage through the existing failure path.

The private read-only validate_migration_archive uses the existing migration_runner.load_manifest on each of the six archived original/variant selectors. It validates the existing strict manifest fields, baseline table fingerprint, ordered immutable references and payload checksums; requires the complete65-step selectors for these new snapshots; and requires exact referenced file membership. Missing/corrupt/truncated selectors or payloads and unexpected files refuse invalid_migration_archive. No apply/verify SQL or shell script is executed. This reuses the existing loader and introduces no new parser or dependency.

Generation verify invokes that semantic check whenever an archive is present or a SQL runtime_schema_profile is recorded. Legacy generations containing neither field nor archive retain their prior supported behavior. All existing outer inventory/runtime-schema/dump/profile checks continue. This proves archive self-consistency and capture coherence within the existing owner-only generation contract; it is not external financial authentication, an actual metadata measurement or protection against a fully authorized coherent rewrite of every contract and inventory. Audit verification never auto-corrects findings.

There is no persisted field, wire/schema version, public signature or shared interface change in this slice. The earlier runtime_schema_profile handoff remains as published in PLAN5_SCHEMA64_65_OPERATOR_PROFILE_2026-10-10.md.

## Exact execution and results

Backend: Linux Python3.12.3 in pinned image sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a. Python executable SHA256 e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f. Docker network none, root read-only, CPUs2/memory2g, evidence bound directly from D:, /work1GiB and /tmp256MiB disposable tmpfs. Exact argv, image identity, observer body, frozen source manifests and terminal states are retained. Operator SQL/client/service behavior in these unit tests is mocked; no database or native backend was connected or executed.

Full existing module used unittest.defaultTestLoader.loadTestsFromName('test_persistence_backup') and unittest.TextTestRunner(verbosity=2) through python3 -u -B /evidence/observer.py complete, with all source guards exact. Result:62 methods passed,0 failures,0 errors,0 skips in232.907067 seconds, including the prior59 and three new methods. This observed duration is not performance-budget qualification.

The three added methods in the existing GenerationTests exercise19 controls: eight staged-source faults across flatfile-primary and mariadb-primary, nine semantically invalid archives with recomputed consistent outer inventories (eight payload/missing-selector/unexpected-input/truncated-selector cases plus a whole missing SQL archive), and two valid legacy generations without archives. Each staged fault checks no new published generation, no pruning and preservation of prior generation bytes; each archive refusal checks reader nonmutation.

The frozen preimage bf665e07c174692f5a495cbaecac5db027e6b29f (archive SHA256051e829c90e67e9301292049ea30c02a695b3d45faa7ade1700aa6e7f33ff463) ran the three focused methods and recorded13 failing subcases,0 errors/0 skips. That preimage contained eight stage faults, four payload/missing-selector archive cases and the whole missing archive case, plus the two passing legacy positives. The unexpected/truncated reader archive cases were subsequently added for completeness and are in the final62-method pass; they were not part of the old13 failures. An intermediate focused repair ran three methods with0 failures/errors/skips on tree c7f2acc71ce130ebd0ee6d49bac5f47021b9f4c0. Collection/container exit0 never converts failed preimage assertions into passing tests.

Additional source commands in /work:

```text
python3 -B scripts/validate_data_lifecycle.py --json
python3 -B scripts/validate_runtime_compatibility.py
python3 -B scripts/validate_runtime_compatibility.py --schema65
python3 -B scripts/validate_economy_accounting.py --root /work
python3 -B scripts/validate_economy_accounting.py --root /work --release
python3 -B scripts/generate_economy_writer_coverage.py --check
```

Exits0,0,2,0,1,1 respectively. Lifecycle validates231 tables/51 non-database stores/42 Redis surfaces with destructive rules disabled. Runtime64 remains valid and65 registered/unmeasured; explicit65 refuses because actual two-engine metadata fingerprints are pending. Ordinary accounting contract validity passes14 fixtures/931 routes/2911 candidate sites, release_ready=False. Release check refuses writer has no executable evidence; matrix check refuses stale generated coordinates. Six archived source selectors reference140 unique files (six selectors plus134 distinct payloads), with780 step/apply/verify checksum comparisons. git diff --check passes.

Independent matrix derivation changes exactly backup.capture, backup.retention and restore.qualification definition coordinates; all other fields in those three records are identical. Primary must refresh its owned registry/source pins and generated matrix after integrating the actual owned slices. No route, backend, fixture, acceptance or coverage status was upgraded here. The shared immutable-runner authentic64-negative/session-close63 input corrections and older function-owned source-test requests remain outstanding primary work.

## Evidence seal, curator handoff and remaining gates

Evidence directory: D:/Dev/Tests/Duris/accounting-plan5/migration-archive-coherence-20261010. complete-source.json/complete.tar bind the final source; complete-result.json/complete-backup-unittest.log bind62/62 acceptance. before-result.json and before-backup-unittest.log retain the initial negative evidence. contracts-result.json/contracts-command-*.json, matrix-differences.json and migration-inputs.json bind separate contract and input derivations. All helpers, observations, raw logs, exact Docker commands and terminal states are retained. Seal SHA256 c4ded87695fac393403cda4207950828326d79c21fc431222954aed933e516b4. The post-push delivery/result.json rehashes all sealed regular members and binds base/result/remote commits, exact31 tested owned overlays, four committed files, clean worktree, terminal network-none/non-OOM containers and seven historical ancestor tips. No log/archive/private data is committed to Git.

This report and additive remote follow-up are curator-ready for the primary-maintained notebook. They claim no notebook import/application/acknowledgment, primary adoption or tested combined-candidate publication. The shared notebook and externally handled Docker recovery are nonblocking.

Native/compiler/preprocessor/build tests, real disposable MySQL8/MariaDB10.11 migration/restore/recovery, native audits/custody/gameplay, performance budgets, retention/erasure evidence and R1–R8 remain deferred/open. At major-plan readiness the primary must run the existing two-engine tests/async/test_staging_migration_fork_mysql.py --update-contract --report <owned-json>, all fresh/populated original/variant/refusal/receipt/tamper/shell/compiled-startup branches, then ordinary/explicit65 validators and original upgrade/restore/recovery/gameplay on the exact combined candidate. Current65 metadata is unmeasured; constants/synthetic fixtures/inventory/source tests/isolated old passes cannot qualify this candidate or release.

Native wallet locator correction remains SOURCE_CORRECTED/NATIVE_ACCEPTANCE_PENDING. Durable erasure proof and true current native financial/history/birth-proof acceptance remain open. Accounting inactive behavior, wallet-root ITEM_MONEY exclusions and the declined inactive spell change are preserved. No activation, live migration, production data operation, deployment, PR merge, primary-branch push, cross-chat message or audit autocorrection occurred.
