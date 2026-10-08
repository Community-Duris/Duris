# Plan 5: item supply action/state reconciliation - 2026-10-08

The independent reader now refuses missing or malformed selected item actions
and reports a tombstone projected as a move. It validates action/state consistency
before opening and lineage-history skips, and counts an inconsistent event once
across overlapping projections. Valid labels and original evidence remain intact.
This fixes false-clean audit results and silent provenance omissions. It does
not qualify genuine compound producers, the unpublished combined candidate or
release completion.

## Delivery and ownership

- Local/remote branch: `codex/accounting-plan5`; this slice stays on that branch.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `0a962f23f7f1664cc2b80c912dd4aa6153492c6b`.
- Result: this containing commit; its exact SHA and verified remote equality are
  recorded after push in `D:/Dev/Tests/Duris/accounting-plan5/item-actions-20261008/delivery/result.json`.
- Owned code: `scripts/reconcile_economy_accounting.py` (one internal helper, one
  event-identity set and two call sites), `tests/async/test_reconcile_economy_accounting.py`
  (five methods), and the existing `tests/async/run_economic_sql_audit_snapshot_mysql.py`
  (only `verify_compound_item_actions` extended).
- Owned documentation: `AUDIT_OPERATIONS.md`, this report and the additive remote
  follow-up. `scope-binding.json` verifies the unchanged AST outside the named
  edits and all 27 tested overlay blobs.
- All seven earlier branch tips and the earlier tombstone fix remain ancestors.
  The post-push receipt checks them and the clean owned worktree. Earlier branch
  work remains available through this same remote history.

No shared interface/schema change is requested. The existing exporter already
projects actions as `create`, `move` or `destroy`. Creation requires live state;
destruction and tombstone state require one another. Collector quarantine remains
`move` with `quarantined` state. The native plan bytes encode custody state rather
than this separate action label. Existing lineage/unattributed action envelope
checks remain unchanged. The new selected-action refusal is `SnapshotError` with
`invalid item history action`; the existing `invalid_item_supply_state` finding
covers valid labels inconsistent with their endpoint state. Its identity is
operation ID/event index/UID. Original labels are retained, never corrected.

Primary should retain these methods when updating its maintained registration;
no shared registry/matrix is edited here:

- `ReconciliationTests.test_selected_item_history_refuses_invalid_action_before_origin_lookup`
- `ReconciliationTests.test_tombstone_requires_destroy_action_in_both_history_scopes`
- `ReconciliationTests.test_invalid_selected_item_action_refuses_every_cli_view_and_limit`
- `ReconciliationTests.test_tombstone_action_cli_preserves_finding_and_original_projection`
- `ReconciliationTests.test_supply_state_checked_before_scope_skips_and_counted_once`

## Exact tested source and established defect

Every source freeze composes published primary
`5f5a8bdfd0306a6c65857cf936fd6b332832c90b` with 27 owned overlays. They include the
prior backup/tombstone fixes. The latest refresh is
`e6221a016ba8af26451831f1f4ec07ea264cf115`, whose eight changed paths since the
pinned primary are documentation only. Native and migration trees remain equal.
Required raw documents and the refresh patch are retained; `AI_CONTEXT.md` is
absent from the published tree. Primary's private preparation is not treated as
published or executed evidence.

| Source | Composed Git tree | Archive SHA256 |
| --- | --- | --- |
| 00, old reader | `b34fbb29dfc91c9e8a241044ed4649b634ff5aa7` | `1259dec15c170a6ac0b5a7aa2135a9a2aee0e26f38635672f408c10f5ab7d054` |
| 01, initial four regressions | `ce0677a745b4fca9b6b1b92d23394a1f955b66f0` | `fc32c240799ffb5a39573c89820681592cb1d779dd2a8debc8989ebf9cc7c1f9` |
| 02, corrected four regressions | `6fdc64b5587d1396c480e771312f73b6ed914e69` | `9cbb2a10e1cb26f7683e3a3ca4f5a32976c13d240b2f15b067a695e77438dc08` |
| 03, final five regressions/old reader | `1d6e750acdc10a980a5ef3658f6601ad26f32a4f` | `6d979c98ba6b5b1196beb85f1507eb670f288c5c5c3b1cc7a3306ef34f9ea64b` |
| 04, fixed reader/unit tests | `78d25904cec046a70c1606f28526cb184d07177c` | `2d315e9b5ee8d8ac359a253414c9f51becce6e9f2bef1524177ac1ea10d0a5f3` |
| 05, complete fix with SQL extension | `114ad7d7e81d453311c8f78ed5c6ce8d34f0638a` | `620040ab3bab87ce41ad72fc8f36d76051585a1f4139da2029a86276115d4b3c` |

Each authenticates 6,512 Git blobs: 6,508 regular files and four exact link targets.
Native tree: `833d3085815b396861ad18a77635412212381e4b`; migrations:
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; canonical head:
`0064_auction_custody_history`. The owned branch's historical native tree is not
used in place of this published native tree. Source guards verify bodies,
modes and link targets after every stage.

Final reader blob: `96b744b27cfff03c13358396991ecfc38d8e8a49`, SHA256
`0c6d56acc234f2b4450751e7055e87ed56648aa52b160a77ae1df8cac0cc1226`.
Final unit blob: `9724db788daf5bb42eda3308687cceb738d76e3b`; final SQL runner blob:
`2fbef9dd5746a674ab66f177571e87d5fa050703`. Sources04 and05 differ only in that
SQL runner, so the source04 unit run proves the exact final reader/unit blobs.

The old reader gives zero findings for nine bad projections: one correctly
encoded retirement relabeled `move`, seven malformed present actions (`null`,
boolean, number, object, array, unknown string and `quarantine`) and one missing
action. Seven malformed/missing projections silently disappear from provenance.
The `quarantine` label remains visible but falsely passes full reconciliation.
The move-labeled retirement also remains visible and falsely passes. Direct
provenance drops unknown strings, so the suspected private-value disclosure was
disproved; no disclosure fix is claimed.

Two healthy controls remain clean. All 11 original EAP inputs decode and exactly
re-encode in the native SQL and flatfile builds: 22 checks before and after the
reader fix. Labels are changed after binding the original plan, leaving its
bytes/digest intact. These task-authored inputs establish the independent-reader
projection defect; they are not admitted producer journeys. The native probes
are authenticated reuse from the previous zero-net slice, linked to 14 genuine
published providers with the original strict fixture flags. Their binary hashes
and every native source body/mode match; no fresh probe build is claimed here.

The final five methods on the old reader produce 46 failed subtests, zero errors
and zero skips. Earlier red01 (54 failures) and red02 (42 failures) remain sealed.
Test authoring corrected the expected CLI prefix and removed redundant missing-
action iterations before adding the scope/dedup test. A SQL insertion-anchor
assertion stopped after the reader edit; PowerShell continued to source04/checks.
That partial stage is preserved and scoped honestly. The SQL edit was then
completed as source05 and its complete runner executed separately. No failed or
terminal run was replaced, and no timeout triggered a restart.

## Executed validation and commands

Tools image: `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
GCC13.3, Python3.12.3, nm2.42 and mysql_config10.11.14 have version/body hashes in
`seal/native-build-inventory.json`. D: evidence/build roots are mounted directly.
Checks and SQL stages use private tmpfs databases. The canonical restore runner's
required `/plan5-restore-coins-*` roots use its fresh isolated Docker layer on the
existing D: Docker disk. No ports, network, existing volumes or production access.
Sanitizers, original strict flags, deadlines and native recipes remain unchanged.

For each stage below, the exact tools-image entry is
`python3 -u -B /evidence/<stage>/observer.py <stage> <source>`; the table supplies
both arguments. Each `docker-command.json` records the full actual host argv,
mounts/resources/image, and `commands.json`/`launches.json` retain observed parent
argv, deadlines, exits and timings, including expected refusal commands.

| Stage | Source | Result | Seconds |
| --- | --- | --- | ---: |
| reproduce00 | 00 | Nine bad projections falsely clean, two healthy clean, 22 original native codec checks | 1.861922 |
| red01 | 01 | Four methods, 54 failures, zero errors/skips; expected exit1 | 3.441478 |
| red02 | 02 | Four methods, 42 failures, zero errors/skips; expected exit1 | 3.400749 |
| red03 | 03 | Five methods, 46 failures, zero errors/skips; expected exit1 | 3.340074 |
| checks04 | 04 | 16 modules, 401 loaded/383 PASS/18 original opt-in skips; existing full partial-SQL runners PASS both engines | 201.143071 |
| sql05 | 05 | Final complete partial-SQL runners PASS both engines, including all new projection cases | 106.534019 |
| canonical05 | 05 | Original complete native restore method PASS, zero skips, both fresh canonical0064 engines | 344.568825 |
| codec05 | 05 | Two healthy clean, eight invalid actions refuse, retirement mismatch found once; original 22 native round trips PASS | 2.686700 |
| budget05 | 05 | Six 100,000-event component cuts, all input bytes unchanged, within original budget | 8.942700 |

The 16 actual module names, every original skip reason and all per-module counts
are in `qualification.json` and the checks observer. The existing audit-budget
opt-in is enabled. The 18 pure-suite skips are its pre-existing separate native/
SQL opt-in methods, not the separately enabled canonical restore test.
SQL invocation is `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`
with generated private disposable-schema/socket settings. Original native method:
`RestoreCoinEffectsTests.test_native_coin_effects_both_modes_and_canonical_engines`,
with `DURIS_RUN_RESTORE_COIN_INTEGRATION=1` and `DURIS_PLAN5_CANONICAL_EVIDENCE=1`.

MariaDB: `10.11.14-MariaDB-0ubuntu0.24.04.1`; MySQL:
`8.0.46-0ubuntu0.22.04.4`. For each engine the existing compound helper captures
four partial SQL cuts (craft creation and craft/quest/collector retirement), then
damages only saved JSON projections. Four invalid-action cases exercise all seven
CLI views at limits0/1/100 (84 CLI refusals, exit2). Three selected-only and three
overlapping retirement relabels exercise exceptions/provenance at those limits
(36 CLI findings, exit1). Total: ten added projection cases/120 actual CLI checks
per engine, all 47 application tables unchanged, source projections unchanged,
original fixture restored. Existing partial-fixture findings, including missing
original plans, remain; these are not complete clean snapshots or genuine
compound gameplay. Input bytes/objects remain unchanged after audit and CLI.

The 240 added CLI vectors in `sql-cli-command-reconstruction.json` are explicitly
reconstructed from the authenticated executed runner source, result records and
retained output paths. They are not raw parent observations of nested subprocess
calls. The original runner checks actual CLI return codes/stdout/stderr and
retains each output; outer command records remain raw observations.

The original native decoder corpus and independent decoder agree on 3,026 cases,
1,054 accepted, in SQL and flatfile modes. Each engine completes 109 canonical
cuts/90 expected refusals, including 58 full-entry restore cuts. Original pending
claims, intent/history controls and bounds remain included; authority unchanged.
Parent observation records 58 run commands/60 launches in canonical05. Its 18
successful g++ driver calls are two fixture compile/link commands and 16 toolchain
queries. Nested runner compilation totals are not inferred from this count.

The new identity set is measured on 100,000 selected events/33,553,408 input bytes
at limits0/1/100. Valid move/live controls retain 100,000 missing-reference and
100,000 unknown-origin findings. Destroy/live corruptions add exactly 100,000
supply-state findings before missing-origin skips. Maximum elapsed time is
1.053800s; peak RSS 198,094,848 bytes, within 30 seconds/256MiB. These synthetic
component cuts establish bounded overhead on this tools image, not release-host
or real producer workload qualification.

## Evidence, integration and remaining gates

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/item-actions-20261008`.
Helpers: `D:/Dev/Temp/accounting-plan5-item-actions`.
Builds: `D:/Dev/Builds/Duris/accounting-plan5-item-actions-20261008`.
`qualification.json`, six source manifests, transport authentication,
`scope-binding.json`, model EAPs, reproduction/red logs, complete unit/SQL/native
logs, nested CLI outputs and native inventories remain local protected evidence.
They are not committed as runtime/test data.

Raw seal SHA256: `21364c4b44791ca460d423a8e83e4a872d83a3462869056675ce9aef6aa76c96`; 2433 files/1834515270 bytes, including
2051 regular files under build roots. All ten containers are stopped,
with no OOM or live recorded children; all three red stages retain exit1.
Native lstat/body inventory and Windows extended-path regular-only hashing find
no copied links or reparse points. Post-seal report/delivery/helper files are
additional artifacts, separately bound by the post-push receipt.

The prior tombstone commit `c373f04fac0ef14f734bc086c1cef01246dc70fe` and restore blob
`23b17a3d158ac455105de1cd1303561dcc028ad8` are included/preserved in these 27 overlays.
Primary still must import/preserve that fix with backup integration. Earlier
24-overlay reports retain their original scope. No shared change is requested
by this action/state fix.

The unchanged original flatfile-authority fixture's seven genuine-provider link
symbols, nine missing-record controls and full 12-method backup qualification
remain open from the prior slice. The original room-seed UID-owner link and SQL
baseline source-recipe/head closure remain primary-owned; no rerun is claimed
here. Maintained whole-server builds and gameplay boots are not rerun for this
Python-only change. Complete openings, genuine producers and cold native/wallet
lifetimes, admission/recovery/ACK, registry/matrix, integrated player/routes/fault/
load workloads, the published combined candidate, activation-owner qualification,
Plan5, R1-R8 and release completion remain open. Latest primary preparation of
ordinary world readers/cold flat census is documentation/private preparation,
not new published executable qualification.

This report and the remote follow-up are additive curator-ready notebook inputs.
The primary-local notebook is nonblocking; curator application/ack is unclaimed.
Inactive behavior, wallet-root ITEM_MONEY exclusions and the declined inactive
spell-path change are preserved. No autocorrection, activation, production
access/write, primary-branch push, deployment or merge.
