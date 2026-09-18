# Artifact control implementation work packages and acceptance matrix

This is the executable backlog for [the specification](ARTIFACT_CONTROL_SPEC.md) and [operator flows](ARTIFACT_CONTROL_OPERATIONS.md). Test/tool names marked NEW below are deliverables, not commands already available. Implement all packages before claiming the implementation goal complete. Early milestones do not reduce the final scope.

## 1. Execution rules

1. Read `AGENTS.md`, the specification, operator contract, baseline research, and current implementation. Create a `codex/artifact-control` implementation branch/worktree from current origin/master while preserving these documents. Do not overwrite unrelated work or assume this detached research worktree has a commit containing the plan.
2. Compare current code with researched revision `440248b17`. Record changed seams, schema head, active backend contracts, and migration number allocation in `docs/implementation/ARTIFACT_CONTROL_PROGRESS.md` (NEW). Preserve contract intent where file locations changed.
3. For each package: implement production code and corresponding meaningful tests, format changed C++, build, run focused tests, then record evidence. Move on after passing; do not repeatedly run an unchanged broad suite.
4. Keep a requirements table R01–R10, journeys U01–U10, tests T01–T16, package status, files, exact commands, source revision, result, and known gaps. A compile/source-string test does not substitute for behavior. Do not label a fixture-only test as live-server evidence.
5. Use disposable local SQL/flat-file state. No production migration, live tuning, publish, or deployment is authorized by this plan. Preserve all existing live defaults; exercise modern/pinned/holder-split modes in fixtures. Do not require private editor access to deliver the public system.
6. Reuse the existing scheduler/mana/custody/transaction machinery. Do not resolve a hard case by adding another mutable authority or disabling checks. If a test is broken, establish whether baseline or new behavior is responsible and keep evidence.
7. At completion, run final requirement audit from implementation evidence. List any unsupported artifacts honestly; their declared legacy capabilities must still be controllable/inspectable as specified. Do not claim every legacy power was redesigned.

## 2. Packages

### W01 — Reproducible catalog and capability inventory

Dependencies: none. Requirements: R01, R02.

Implement:

- Extend `scripts/artifact_source_inventory.py` (or a companion sharing its parser) to produce deterministic source/capability/placement evidence from active area indexes and compiler configuration. Preserve existing `--check` contract or update its fixture deliberately.
- Resolve direct reset commands with stable IDs and conditional mob/container provenance; enumerate indirect object-table and script/native creation references. Mark uncertain dynamic resolution and imported legacy ownership explicitly, never invent certainty.
- Record all power paths per template: base affects, packed/device values, native callback/helper, `.trg`, proc library, periodic registration, reactive timing, NPC exclusions, and resource/timer use.
- Add compiled registry descriptors for delivered pilot powers and legacy adapter identities; annotate editable parameters versus source-only behavior.
- Add a read-only aggregate inspection model that does not instantiate live objects. Persist no state during inventory/inspection.

Touch likely seams: `src/specs/specs.assign.c`, `src/world/db.c`, `src/mob/studioproc.c`, `src/item/*actions*`, object parsers and existing inventory tests.

Acceptance: 169 baseline templates accounted for at baseline revision; counts may legitimately change on new master but every delta has evidence. Placeholder tokens retain no invented power. Tsunami's unique category and its `E` placement are correct; inactive `.zon` matches excluded. Same input produces same hash. Preview adds no live UID/mana/custody record.

Tests: T01, existing `test_artifact_source_inventory.py`. Output: generated compatible catalog baseline and a capability report documenting hard-coded fields.

### W02 — Pure schema, typed profiles, and deterministic resolution

Dependencies: W01. Requirements: R01, R02, R06, R07.

Implement the model/registry/policy modules in the specification, JSON Schema documentation, canonical serializer, strict errors, exact units, profile-reference validation, holder precedence, and side-effect-free preview. Add full schema examples for legacy Tsunami, player/NPC split Tsunami, pinned Avernus, synchronous mirrored ioun, stateful swords/necroplasm, and a typed Studio power. Samples are nonproduction fixtures and must parse through production code.

Define semantic IDs for every owned power and shared cooldown; descriptors must identify registration sites and event dispatch. Reject two owners for one selected power, unsupported holder/variant, illegal form/resource association, or attempted reactive windup. Implement prototype override descriptors and `futureInstances/restart` application classification.

Acceptance: changing a cosmetic field cannot change classification; equivalent JSON key order yields same canonical content/hash; duplicate/unknown keys and oversized/escaped paths reject. Preview produces selected policy and reasons for player/wild/controlled NPC, placement and UID cases. Roundtrip preserves exact integer quantities/revisions.

Tests: T02, T03. Output: `artifact_catalog_model`, `artifact_adapter_registry`, `artifact_policy` and NEW `docs/reference/artifact-control.schema.json` plus tested examples.

### W03 — Durable control repository, schema, and restricted SQL surface

Dependencies: W02. Requirements: R04, R05, R08, R09.

Implement SQL/flat-file repository interfaces, all proposed tables or documented reuse of exactly equivalent facilities, draft generations, immutable submission snapshotting, request deduplication, audit, result state, head CAS, lease/fencing, and crash-replay file journal. Add migrations with current sequence, verifiers, manifest checksums, runtime compatibility inventory, backup/lifecycle/restore coverage.

Implement SQL roles/procedure grants, authenticated principal mapping, draft UPDATE trigger/generation semantics, strict size/valid-JSON enforcement, read-only status views, and no writes by operator accounts to runtime state. Procedures copy frozen candidate bytes inside a transaction; later edits cannot alter requests. Service schemas distinguish unknown UID/row from backend failure.

Acceptance: same operation+payload retries idempotently; different payload with same ID conflicts. Two publishers cannot both win one base. Both MySQL 8.0 and MariaDB 10.11 enforce expected constraints; direct draft UPDATE works and forbidden active/state UPDATE fails. Flat-file crash at every multi-file boundary replays consistently. No fallback to another backend on failure.

Tests: T04, T05, T06. Output: repository, NEW `run_artifact_control_mysql.sh`, migration and restricted operator setup instructions. Production credentials/state must not be used.

### W04 — Validation worker, publication, boot recovery, and diagnostics

Dependencies: W03. Requirements: R07–R09.

Implement queue limits/polling, immutable worker messages, world snapshot epochs, prepare/commit/apply state machine, admission barriers, bounded cleanup, atomic prepared registry swaps, applied receipts, boot scheduled activation and last-head restore, exclusive offline publication, and complete health diagnostics. Typed profile dry-validation must precede mutation of any live mana registry.

Integrate startup after actual prototypes/spell pointers/native adapter capabilities are available, before accepting game input; document relation to `studioproc_boot`. Runtime properties reload must not overwrite catalog-owned state. Validate actual world references and native power capabilities, not merely JSON structure.

Acceptance: running old valid revision survives a rejected/offline-storage candidate. Crash before commit uses old head; crash after commit applies new head at boot once, with no restored pending actions. Invalid initialized head blocks listener rather than free fallback. Lost result acknowledgements can be queried/retried without duplicate application. No SQL/fsync in command/combat callbacks. Successful `APPLIED` corresponds to actual running revision.

Tests: T06, T07, T15. Output: runtime/game bridge, health API and boot/offline validation path.

### W05 — Holder variants, durable UID policy, cooldowns, native tuning

Dependencies: W04. Requirements: R02, R06, R07.

Replace vnum-only mode decisions at actual selected-power boundaries with a source/actor/event-aware resolver. Keep compatibility wrappers only for callers that cannot yet own migrated behavior; do not accidentally allow their result to override catalog policy. Cover `native_artifact_owns`, weapon/device/Studio bridges, and native callbacks.

Implement UID pin/follow/provenance state and existing custody integration; resolve charm/pet/possession changes. Persist shared semantic cooldown obligations for catalog-managed powers, including Studio, without altering unrelated ability IDs. Complete source-linked cleanup for necroplasm and class/form contributions. Preserve legacy sword energy independently from paid reserve.

Parameterize at minimum:

| Adapter | Required controls |
| --- | --- |
| Avernus | Drain chance/cap and retained healing constraints; speech skin level/cooldown; offensive windup and optional cost. |
| Tsunami | Tap/wave cooldowns, captured level rule, windup, per-power cost, bounded knockdown selection parameters that do not bypass terrain/target legality. |
| Mirrored ioun | Intercept chance, cost/floor, legal-target cap; synchronous timing locked. |
| Necroplasm | Transformation level/timing/cost and supported proc selection; curse/equipment lifecycle remains explicit and tested. |
| Swords | Named combat/defense powers with levels/cost multipliers, drain cap, skin/challenge cooldowns, flurry min/max, nova minimum, class-stat profiles. Preserve native race/permission gates. |
| Packed/random weapons | Per-template mode and original slot power/chance semantics, offensive windup/cost, timer where applicable. No double random selection. |
| Devices/wonder | Per-template mode/timing, typed effects/outcomes with existing charge/ink rules; invalid arbitrary spell remapping is rejected. |
| Studio | Existing typed definition editing through shared validation; durable managed cooldown IDs and single ownership with native bridges. |

NPC automatic active ability policy is opt-in and calls the same admission with legal target/interval/cost; no proc while only in inventory when equipment is required. Required variants exist only where corresponding compiled adapters and holder eligibility are supported. Do not manufacture NPC necroplasm behavior that the existing design forbids; expose exact limitation and test rejection.

Acceptance: player/new + NPC/legacy and reverse on supported pilots; pinned/follow-policy transfer and restart; controlled NPC inheritance; no duplicate proc or refill; old cooldown debt survives mode change; base curse penalties not removed by mana disable.

Tests: T03, T08, T09, T10 plus existing native/weapon/sword/Studio tests. Output: all delivered pilots wired through one resolver and typed descriptors.

### W06 — Unified acquisition, placement/lifecycle control, and compatibility commands

Dependencies: W05. Requirements: R01, R02, R07, R08.

Audit and classify all instantiation callers as preview/fresh/restore. Integrate family scarcity reservations with existing custody establishment, including failure/retry. Import active resets with stable IDs and hand off only migrated artifact loads to catalog policy. Implement zone/mob/container destination and dependency checks, effective chance exactly once, placement override/post-loot semantics, recovery without reroll and uniqueness across aliases.

Extract lifetime/feed/binding/war calculations into typed policy; use the same operation paths for SQL transactional domain and flat-file equivalents. Make scheduler intervals configurable within spec bounds. Classify legacy dead properties and reject catalog-owned property mutation with explanatory help. Bridge existing artifact timer/poof/reset/swap/load commands into typed service where they affect managed artifacts, preserving permissions/semantics and preventing bypass.

Implement base prototype overlays for future instances and restart-required staging; show native derived stats separately. Add state-operation preview/request/results with expected revisions and exact target; config placement edits never silently relocate existing items. Resolve temporary inspection calls without rarity or ledger effects.

Acceptance: reset/shop/reward/script/staff/recovery each obey the chosen policy; no double chance halving or duplicate artifact; backend error denies fresh creation; boot-only remains boot-only; expiry/feed/war formulas match imported baseline; runtime clock/jobs bounded.

Tests: T10, T11, T12; existing artifact/guild, flat-file and zone reset tests. Output: spawn/lifecycle services, safe legacy command compatibility, full source-load-route coverage report.

### W07 — Full offline CLI and database operator journeys

Dependencies: W04, W06. Requirements: R04, R05, R09, R10.

Build `bin/tools/artifactctl` using production libraries. Implement validate-files/export/import/draft/show/diff/rebase/validate/publish/schedule-boot/activate-offline/status/history/health/instance-preview/submit with documented flags, JSON output, exact exit codes, reasons/operation IDs and no password logging. Provide shell completion and field-help from the registry.

Implement production schema examples and SQL procedures exactly as documented or update documents atomically to matching contracts. Demonstrate ordinary SQL draft JSON editing, procedure submission, rejection, conflict, accepted/applied distinction, and prevented runtime-table updates with the actual restricted user. Stopped-server state requests stay queued; offline configuration activation acquires exclusive lease and actual world validator.

Acceptance: U06/U07 end-to-end against a running disposable server; stopped-server boot schedule and exclusive offline activation; config export/import roundtrip without policy drift. Files, SQL, and UI drafts converge to the same canonical candidate hash and effect.

Tests: T05, T13, T14. Output: working CLI, SQL cookbook, files workflow and runtime result integration.

### W08 — Immortal workbench and command equivalents

Dependencies: W07 (backend/service complete), though pure renderer work may be developed after W02. Requirements: R03, R06, R09, R10.

Implement all screens and flows in operator contract: browser/search/filter, overview, mode/holder picker, ability/balance editor, placement wizard, lifecycle/base-equipment editor, holder preview, live-instance state, script capability map, drafts, review, publication status, conflict resolution, history, rollback, health.

Integrate descriptor session route carefully: preserve pager/string editor and ordinary game command access. Durable draft generation, bounded undo/redo (last 50 edits; keep newest full draft), async save indicator, disconnect/resume, idle close, permission revocation, output escaping, terminal width and color-off support. Preview and field help come from production model/registry, not duplicated magic constants.

Every workflow has a tested typed command equivalent. Lifecycle destructive confirmation ties to preview hash/revisions; one explicit publish action for config; no fake Save-as-publish. UI renders worker completion via descriptor/session identity, never a stale character pointer.

Acceptance: U01–U05 and U08–U10 over actual Telnet. A player cannot enter editor paths; lower-rank reader cannot publish; authority revoked mid-session prevents commit. Pager/chat/string-editor behavior and queued commands remain correct. All screens readable at 80/60 columns and no ANSI mode.

Tests: T14, existing command-latency/session behavior tests as applicable. Output: usable workbench, transcript evidence and help content installed through existing help workflow.

### W09 — Whole-system hardening, restore, and measured rollout

Dependencies: W01–W08. Requirements: all.

Execute fault/role/transfer matrix and SQL/flat-file journeys. Include concurrent drafts, changing world digest, lost acknowledgements, lease expiry with stale writer, malformed SQL/file documents, crash during registry commit/cleanup, partial backup and restored head mismatch. Prove coherent backup restores control + existing item/mana/lifecycle state.

Measure config apply work per pulse and normal combat overhead in an idle comparable baseline with representative full catalog and realistically populated UID state. Track pending actions/callback latency and full-loop p99, workload and host. Initial canary max pending 128; never use 4096 as an unmeasured live-capacity claim. Profile critical violations before raising caps. No SQL/pointer safety waiver to achieve speed.

Document existing mana crash-refund bound honestly. Require no new unbounded refill/duplicate-effect window from this control feature. Include immutable migration fresh/replay/restore tests on both SQL engines and flat-file protected state.

Acceptance: all T cases and U journeys pass with evidence; tracked config defaults retain current gameplay/availability; invalid changes demonstrably leave game running safely; no production action needed to prove implementation.

### W10 — Completion handoff and requirement audit

Dependencies: W09. Requirements: R10 and verification of all others.

Deliver `docs/operations/ARTIFACT_CONTROL.md`, `ARTIFACT_CONTROL_SQL.md`, `ARTIFACT_CONTROL_CLI.md`, schema/field reference, compatibility/migration/rollback runbook, source-capability map, working examples, and implementation progress/evidence report. Replace proposed syntax with exact tested syntax and link every named command.

Audit each R/U/T/W item against actual source/test/transcript/manifest evidence. Run `make -C src`, `./scripts/format.sh --check`, final applicable focused/regression suites and `make test-all` after integration; explain any baseline/environment failure without claiming its coverage passed. Obtain representative real server behavior evidence on both persistence families. The goal is not complete with missing required journeys, placeholders, TODO control handlers, or parser-only SQL/UI validation.

Do not wait for CI to substitute for local evidence. Do not merge/deploy solely because this document exists. Return changed behavior, test results, remaining operational rollout steps, and the exact build revision.

## 3. Test matrix with explicit assertions

All T-named files below are NEW recommended stable names. Test production seams, not copies of implementation logic. Use disposable fixtures and fault injection confined to harnesses; no production cheat endpoints.

| ID | Test file / harness | Required assertions |
| --- | --- | --- |
| T01 | `test_artifact_control_inventory.py` | Active index, direct/indirect references, placeholders, dynamic uncertainty, category discrepancy, deterministic source maps, preview has no state effect. |
| T02 | `test_artifact_control_model.py` | Strict parser, duplicate/unknown keys, bounds, path escapes, exact units, schema roundtrip, profiles, immutable IDs, revision overflow, invalid capabilities and synchronous constraints. |
| T03 | `test_artifact_control_policy.py` | Every precedence combination; player/wild/controlled NPC, pin and post-loot expiry of override, absent/moved source, suppress versus legacy, explain provenance. |
| T04 | `test_artifact_control_repository.py` | SQL-independent contract and real flat-file store: generation/CAS, immutable snapshot, operation retry, conflicting payload, journal crash replay, lease fence. |
| T05 | `run_artifact_control_mysql.sh` | Same contract on MySQL8/MariaDB10.11; actual restricted SQL account UPDATE/submit, forbidden active/state writes, authenticated audit, atomic draft snapshot under race. |
| T06 | `test_artifact_control_publication.py` | Every state-machine crash boundary, simultaneous writers, world epoch change, lease loss, no partial runtime publication, resource registry unchanged on rejection, commit-before-apply recovery. |
| T07 | `test_artifact_control_boot.py` | Uninitialized legacy mode, initialized invalid head refuses listener, scheduled boot success/conflict, offline lock rejection, startup committed replay, no stale export fallback. |
| T08 | `test_artifact_control_variants.py` | Both mode directions on supported pilots, all delivered adapters, native/Studio single ownership, held/equipped constraints, NPC command AI, unsupported variant rejection. |
| T09 | `test_artifact_control_cooldowns.py` | Native-to-new and new-to-native transfer, Studio persistence, restart, backward time, profile rollback monotonicity, no shorten-on-switch or capacity refill. |
| T10 | `run_artifact_control_custody_journey.py` | Real give/drop/get, nested container, death/corpse/loot, pet/charm/possession, relog/copyover, form cleanup, UID pin/expiry/mana continuity, stale preview conflict. |
| T11 | `test_artifact_control_spawn.py` | Zone E/G/O/P/table, script/native/wizard fresh creation, preview/restore distinction, shared family aliases, population/chance once, storage fault denies spawn, failed custody reservation cleanup. |
| T12 | `test_artifact_control_lifecycle.py` | Ten-day imported behavior, feed integer rounding/difficulty, binding/wars, dead key diagnostics, timed job uniqueness, typed timer operation and domain projection consistency. |
| T13 | `run_artifact_control_cli_journey.py` | Actual CLI with server and stopped server; exit codes/JSON/status, files roundtrip, SQL same-hash parity, retries/timeout, no credential output. |
| T14 | `run_artifact_control_imm_journey.py` | Real Telnet U01–U10 including reader/editor/publisher ranks, invalid input, stale conflict, disconnect, pager/string editor/chat routing, no accidental command execution, 60/80 columns, color-off. |
| T15 | `test_artifact_control_limits.py` | Full-size malformed inputs, queue/backpressure, bounded pulse cleanup, profile registry allocation before commit, no per-hit blocking I/O; measured runtime/callback/loop overhead with workload recorded. |
| T16 | `run_artifact_control_restore.sh` | Coherent SQL and flat-file backup/restore, instance/cooldown/catalog/resource retention, corrupt hash/partial generation detection, additive migration/replay and old-binary incompatibility reported honestly. |

Existing tests to keep passing when relevant:

```sh
python3 scripts/artifact_source_inventory.py --check
python3 tests/async/test_item_actions_runtime.py
python3 tests/async/test_weapon_actions_runtime.py
python3 tests/async/test_native_artifact_runtime.py
python3 tests/async/test_sword_actions_runtime.py
python3 tests/async/test_studio_ability_model.py
python3 tests/async/test_studio_abilities_runtime.py
python3 tests/async/test_studio_editor_stub.py
python3 tests/async/test_artifact_mana_runtime.py
python3 tests/async/test_artifact_mana_game.py
python3 tests/async/test_artifact_mana_restore.py
python3 tests/async/test_artifact_guild_transactional_cutover.py
python3 tests/async/test_flatfile_artifact_repository.py
python3 tests/async/test_zone_reset_timing_contract.py
python3 tests/async/test_command_latency_runtime.py
```

Use existing `run_artifact_mana_mysql.sh`, `run_artifact_guild_schema_mysql.sh`, and `run_weapon_actions_journey.py` per their documented flags when their contracts change. Do not copy production `.env` into fixtures. Test harnesses must explicitly target disposable roots/databases and terminate their own server processes.

## 4. Requirement-to-evidence checklist

| Requirement | Packages | Minimum completion evidence |
| --- | --- | --- |
| R01 inventory and addition | W01/W02/W06 | Reproducible source map; exact delta report; actual new-definition fixture added through all required validation. |
| R02 full declared control scope | W02/W05/W06 | Capability coverage per artifact; all delivered adapters; legacy-only cases controlled honestly; every editable parameter shown to affect real behavior. |
| R03 interactive immortal workbench | W08 | Production handlers plus U01–U05/U08–U10 real Telnet transcripts and tested menu interactions. |
| R04 no-login config and CLI | W03/W04/W07 | File import/export and CLI publication offline/online; status/exit codes; stopped-server activation. |
| R05 safe database authoring | W03/W07 | Restricted SQL editor accepted/rejected/conflict journeys and forbidden active-table writes on both engines. |
| R06 individual/holder variants | W05/W08 | Player/NPC mode matrix, pinned UID across loot/restart, controlled-NPC inheritance, unsupported capability errors. |
| R07 continuity/no exploit | W04–W06/W09 | T06/T08–T12/T16 with real UID/resource/cooldown/custody assertions. |
| R08 persistence/deployment contract | W03/W09 | Current-head additive migration, both SQL engines, flat-file recovery, manifests and coherent restore. |
| R09 validation/concurrency/observability | W03/W04/W07–W09 | Reject/conflict/crash/lease/backpressure tests; applied receipts; measured bounded game-thread work. |
| R10 operability/handoff | W07–W10 | Exact working commands/help, all U journeys, documentation consistent with implementation, complete evidence ledger. |

No implementation evidence exists merely because this plan names a test. The implementation agent must populate commands/results/artifacts and inspect their coverage before setting the implementation goal complete.
