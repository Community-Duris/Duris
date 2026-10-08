# Plan 5: original item-plan starting positions - 2026-10-08

The independent read-only reconciler now compares every original EAP1 item's
before-position with all available valid captured opening and prior-history
positions at that UID/revision. A contradictory original capsule produces
`original_plan_preimage_mismatch` once per operation and prevents that operation
from being counted as a verified original plan. Full Plan 5/R1-R8 and release
remain unqualified.

## Delivery and ownership

- Local/remote branch: `codex/accounting-plan5`; no branch switch.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `9dd3a8da2286a14a8d17d007c4162d6864a0e7f4`.
- Result: this containing commit; exact SHA and remote equality are recorded
  after push in `D:/Dev/Tests/Duris/accounting-plan5/item-preimages-20261008/delivery/result.json`.
- Owned reader: `scripts/reconcile_economy_accounting.py`, existing `audit` call
  and `audit_original_plans`. Existing strict position grammar, typed projection
  comparison and exception/verified-plan accounting are reused.
- Owned tests: existing reconciliation module and existing
  `verify_collector_quarantine_views` in the whole-snapshot SQL runner.
- Owned docs: this report, `AUDIT_OPERATIONS.md` and additive remote follow-up.
  Scope binding proves every other AST and all 27 overlays/native/migrations exact.
- All seven earlier branch tips, prior tombstone c373f04fa, endpoint-state 99c4ce0dc
  and history-projection 9dd3a8da remain ancestors; post-push receipt checks this.

No shared interface/schema/native change is requested. The sole internal helper
call now receives the already captured `native.uid_history_events`; there are no
other callers. The existing opening/selected/lineage rows supply UID, revision,
root, parent, owner and state, plus equipment slot where recorded. Creation's
all-zero wire before-position maps to its UID-named logical creation opening.
Every valid candidate at a UID/revision is checked, including disagreeing copies;
input order does not choose one as authority. Historical omission remains unknown
under existing missing-origin/equipment/history findings. Invalid positions keep
their existing findings rather than becoming valid preimage evidence. A capsule
without a captured preimage does not gain opening or release qualification from
its existing capsule-verification counter.

The index is constructed once for referenced UIDs, with no per-root full-history
scan. No mutation implementation is imported or called; no evidence is changed.
Primary owns shared registration and candidate composition; retain these four
new existing-class methods:

- `ReconciliationTests.test_original_item_plan_preimage_binds_opening_and_retained_positions`
- `ReconciliationTests.test_original_item_plan_preimage_preserves_creation_and_unknown_history`
- `ReconciliationTests.test_original_item_plan_preimage_checks_every_captured_candidate`
- `ReconciliationTests.test_original_item_plan_preimage_cli_is_global_bounded_and_read_only`

Two existing `ItemOwnerHistoryTests` methods retain their owner-history finding
and now also assert the preimage mismatch/unverified count. Their earlier
expectations allowed a capsule to remain verified despite contradictory opening
ownership. No old finding or input guard is removed.

## Exact source and defect evidence

Primary was refreshed before selecting published base `5990082efc2e3c0e4deffac9791d3e8e982c9733`.
Each archive composes that tree with 27 owned overlays, including the previously
completed backup and tombstone fixes. Native tree `833d3085815b396861ad18a77635412212381e4b`;
migration tree `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; head `0064_auction_custody_history`.
The owned branch's historical native tree is not used for native qualification.
Required raw AGENTS/README/finish/requirements/Plan 5/latest checkpoint and private
source-integration note are retained. Published `AI_CONTEXT.md` is absent.
Qualification refresh primary `3ef54e021d22f931de1d5255dde02fffbaf7f310` changes
`['docs/persistence/economy_accounting/DAY1_ROOM_RESET_IMPLEMENTATION_HANDOFF_2026-10-07.md', 'docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md', 'docs/persistence/economy_accounting/FINISH_ACCOUNTING_PLAN.md', 'docs/persistence/economy_accounting/FULL_FLAT_COLD_DRIVER_SOURCE_INTEGRATION_2026-10-08.md', 'docs/persistence/economy_accounting/PERSISTED_PROVIDER_UNION_PRIMARY_HANDOFF_2026-10-07.md', 'docs/persistence/economy_accounting/REMAINING_REQUIREMENTS.md', 'docs/persistence/economy_accounting/domain-separation/CONTINUING_PROJECT_COORDINATION.md', 'docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md']`; later delivery refresh is receipt-authoritative.

| Freeze | Git tree | Archive SHA256 |
| --- | --- | --- |
| 00 | `2c351ca0a2783519e4e8f7948eaec134b5ddbc11` | `48ed0082f71e450c06c85243f080f4cf57f87868a480e66768ef1319225315da` |
| 01 | `155f58c4cc3cb8cae59e6bd55bda97c85aa3b6ae` | `0c24a3b10dd35776dd0d445e82a312da70d2e1c8afa9396d15397a71a4fae2bf` |
| 02 | `525f120a54d0f579e5607d918aba22ff445120ac` | `3128d4decd3979c11984604ae746e2a5ae34ac6ed3db6e26dd0d3dc96ee28388` |
| 03 | `cd14b6afa5646448745d54703f6785d86bc31e3c` | `f2e1e030ffbc74eefc14b8abf9b8a22d863bf3b43eb4ce444bcc77613a8faf1f` |

Each archive independently authenticates 6,513 Git blobs: 6,509 regular files/four
exact link targets, including source body/mode/link transport. Final reader blob
`b6d6d94581703dab8e813f8ce6a2ff3b301f0a85`, SHA256
`d615eabb225ccb24fce95c0ef38a79117b43f5783c502603971a896978c49e12`.

Freeze00 proves six healthy controls and six false-clean damaged capsules:
live/quarantined starting state in both directions at the opening and at retained
history, plus root/uncontained versus child/root82 starting topology in both
directions. Topology cases use complete two-UID native before/after forests.
Only `canonical_plan` and its matching `plan_digest` change; all captured opening,
selected ledger, lineage and native projections stay unchanged. All 12 capsules
are independently accepted and exactly round-trip SQL and flatfile native codecs
(24 executions). These are synthetic position models, not producer journeys.

Freeze01 runs the four new regressions against the old reader:44 failed subtests,
zero errors/skips, expected exit1. Freeze02 passes those same four methods. Its
broader run stops after the160-test reconciliation module with two failures in
the older owner-history expected results, zero errors/one existing opt-in skip.
The strengthened detector correctly adds the preimage finding and leaves the
plan unverified. Freeze03 changes only those two old methods' expected counts.
The failed checks02 source/log/terminal exit1 remain preserved; no full-suite or
SQL pass is claimed for that stage.

Final component/SQL/codec source is freeze03. Original canonical native source is
freeze02. Their only differing file is the reconciliation unit module, containing
those two expectation updates. Reader/native/schema/recipes/helpers/SQL runner
and all actual canonical inputs are byte-identical. `canonical-to-final.patch`
and qualification binding record this; canonical02 is not relabeled as03.

## Commands, backends and results

Tools image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3/Python3.12.3/nm2.42/mysql_config10.11.14; exact executable bodies/versions
are in sealed native build inventories. D evidence/build directories are directly
mounted. Each container has network none/no ports; database roots use private
tmpfs or the canonical runner's fresh layer on the existing D Docker disk.
Original recipes, C++20/ASan/UBSan flags and predeclared deadlines are preserved.
No existing database, service or volume is a test target.

Each stage entry is
`python3 -u -B /evidence/<stage>/observer.py <stage> <source-label>`.
Host Docker argv/mounts/image/limits are in its `docker-command.json`; exact
parent commands/deadlines/exit/timing and launches are in `commands.json` and
`launches.json`. Tests and nested SQL CLI argv are retained separately.

| Stage | Exact source tree | Exit | Seconds |
| --- | --- | ---: | ---: |
| canonical02 | `525f120a54d0f579e5607d918aba22ff445120ac` | 0 | 314.504809 |
| checks02 | `525f120a54d0f579e5607d918aba22ff445120ac` | 1 | 94.932240 |
| checks03 | `cd14b6afa5646448745d54703f6785d86bc31e3c` | 0 | 238.883911 |
| codec03 | `cd14b6afa5646448745d54703f6785d86bc31e3c` | 0 | 3.429800 |
| focused02 | `525f120a54d0f579e5607d918aba22ff445120ac` | 0 | 4.796410 |
| red01 | `155f58c4cc3cb8cae59e6bd55bda97c85aa3b6ae` | 1 | 4.936328 |
| reproduce00 | `2c351ca0a2783519e4e8f7948eaec134b5ddbc11` | 0 | 2.293836 |

Checks03 executes 16 component modules: 412 loaded / 394 PASS / 18 unchanged original
opt-in skips, zero failures/errors. The original bounded budget controls execute
with `DURIS_RUN_AUDIT_BUDGET=1`. Every method/skip is recorded. Four new regressions
cover healthy/damaged state/topology/history, creation, missing/unknown history,
contradictory copies in both orders and36 actual CLI calls across exceptions/
provenance at limits0/1/100. Inputs remain unchanged; private aliases stay excluded.
The existing seven-view owner-history CLI regression retains its old finding and
reports both global findings at every output limit.

The complete original SQL runner command is
`python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`, executed on
MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL8.0.46-0ubuntu0.22.04.4 via
isolated socket/schema environment settings and SELECT-only audit credentials.
Both PASS. Its existing manual 47-table partial fixture remains partial. Healthy
model capsules are temporarily stored in disposable metadata and recaptured by
the unchanged exporter as before. The new contradictory before-position capsule
is authored only into the saved capture; it is not claimed to have been stored
or recaptured through SQL. Captured opening/ledger/lineage/native positions and
all 47 authority inventories remain unchanged; fixture metadata is restored.

Per engine, two existing state directions now each include five saved-projection
damages: 10 probes / 60 CLI calls. The added two probes change only original capsule
before-state and matching digest, adding exactly `original_plan_preimage_mismatch`
while retaining the prior partial findings. All previous state/history probes,
47-table guards, input guards, limits and read-only capture checks still pass.
`sql-cli-command-records.json` retains 120 nested invocations recorded by the
executed SQL runner; these are distinguished from parent-observer raw calls.

The separately enabled original
`RestoreCoinEffectsTests.test_native_coin_effects_both_modes_and_canonical_engines`
runs with `DURIS_RUN_RESTORE_COIN_INTEGRATION=1` and
`DURIS_PLAN5_CANONICAL_EVIDENCE=1`:1 PASS/zero skips. Native SQL/flatfile decoders
agree on 3,026 cases/1,054 accepted. Each fresh 0064 engine completes 109 cuts/
90 expected refusals/58 full-entry cuts with unchanged authority, including the
original intent/history/pending-claim/boundary controls. Its parent observer's
18 successful g++ driver calls are 2 fixture compile/link calls plus 16 toolchain
queries, not 18 compiler jobs. Nested compiler totals are not inferred.

Final codec03 executes 40 exact native round trips: 12 original model capsules,
four healthy SQL-captured capsules and four saved wrong-before SQL capsules,
each in SQL/flatfile. Task-only native probes are authenticated reuse from the
zero-net slice: all source bodies/modes and binary hashes match 14 genuine
providers/original strict flags. No fresh probe or whole-server build/gameplay
boot is claimed. Native/shared files are unchanged. Windows and original full
backup modules are not repeated in this slice.

## Evidence and remaining gates

Evidence `D:/Dev/Tests/Duris/accounting-plan5/item-preimages-20261008`;
helpers `D:/Dev/Temp/accounting-plan5-item-preimages`;
builds `D:/Dev/Builds/Duris/accounting-plan5-item-preimages-20261008`.
Raw seal SHA256 `8b0eebd45e540dbd477037d79e9c755c2c6d8442f9e78023518ecc57602df616`: 1781 files/1328099087 bytes,
including 1317 regular build-root files, not compiler jobs.
All eight containers are stopped/no OOM/live recorded children. Expected old-reader
and old-expectation failures stay exit1. Native lstat/body inventory precedes
regular-only copying and extended-path Windows rehashing; no copied links or
reparse points. Reports, refreshed private-source notes and post-push receipts are post-seal artifacts, additional
to the raw seal's fixed inventory.

The earlier private source-integration checkpoint accepted packets through 50ac.
Latest primary 3ef54e0 documents private flat cold candidate a753f4b6, retaining
29 added Plan 5 methods and a smaller reader composition; all execution there
remains deferred. It explicitly rejects transfer of peer qualification to that
candidate. Import or combined qualification of 99c, 9dd and this slice is unclaimed. The prior tombstone c373f04fa/restore blob
23b17a3d158ac455105de1cd1303561dcc028ad8 remains included in 27 overlays and preserved
on the remote; primary published restore blob is still e78cef8d3dbd4e48c507d49e443b03fa5208f86f.
Retain/import the completed fix during backup integration. Older24-overlay reports
retain their originally tested scope.

Primary-owned original flatfile-authority fixture's seven genuine-provider link
symbols/nine missing-record controls/full12-method backup qualification, room-seed
UID owner link and original native SQL baseline recipe/head closure remain open.
They are not rerun here. Complete openings, real producer/cold-world/wallet
lifetimes, native collector/intent/command/publication journeys, admission/
recovery/ACK, registry/matrix, player/routes/fault/load, primary's published tested
combined candidate and activation-owner qualification remain open. Full Plan 5/
R1-R8/release remain unqualified. Inventory, models and isolated passes do not
establish those journeys.

This report/additive follow-up/receipts are curator-ready input for the nonblocking
primary-local notebook; curator application/ack and cross-chat messaging unclaimed.
Inactive behavior, wallet-root ITEM_MONEY exclusions and declined inactive spell
path stay. No activation, autocorrection, production access/write, primary push,
deployment or merge.
