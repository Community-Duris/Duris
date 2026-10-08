# Plan 5: original-plan endpoint custody state - 2026-10-08

The independent original-plan check now binds the selected ownership event's
endpoint state to retained EAP1 bytes. Previously it compared owner, topology,
revision and equipment but omitted state. A live/quarantined substitution could
pass without findings if the current item projection was changed to agree, and
the original plan still counted as verified. Both state directions now produce
the existing `original_plan_custody_mismatch`, without rewriting the projection.
This closes the endpoint comparison defect; full Plan5/release remain incomplete.

## Delivery and ownership

- Local/remote branch: `codex/accounting-plan5`; no branch switch.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `50acfeeb9d67ea7284cdfd3053bb9d69cba7381c`.
- Result: this containing commit, with its exact SHA, remote equality and clean
  worktree verified after push in `D:/Dev/Tests/Duris/accounting-plan5/item-plan-state-20261008/delivery/result.json`.
- Owned code: `scripts/reconcile_economy_accounting.py`, only `audit_original_plans`;
  `tests/async/test_reconcile_economy_accounting.py`, three methods; and the existing
  `tests/async/run_economic_sql_audit_snapshot_mysql.py`, only
  `verify_collector_quarantine_views` extended.
- Owned docs: `AUDIT_OPERATIONS.md`, this report and additive remote follow-up.
  `scope-binding.json` authenticates untouched ASTs and all 27 tested overlays.
- All seven earlier tips and the prior tombstone fix remain ancestors on this
  same remote branch; the post-push receipt checks preservation and owned blobs.

No shared interface/schema change is requested. The reader consumes the already
captured `ownership_events.state` and independently decoded EAP1 after-position
state (`new[1]`: live1/tombstone2/quarantined3). It adds the state to the existing
strict custody comparison and reuses its finding/verification policy. It does
not import mutation logic, infer missing proof, change an action or correct data.
Shared native/contracts/producers/coordinator/migrations/registry/matrix and
activation-owner files remain untouched. Primary should retain these existing-
class methods when updating maintained registration:

- `ReconciliationTests.test_original_item_plan_binds_live_and_quarantined_state`
- `ReconciliationTests.test_original_item_plan_state_finding_precedes_history_scope_skip`
- `ReconciliationTests.test_original_item_plan_state_cli_keeps_projection_and_finding`

## Exact source and defect proof

All freezes use published primary `e6221a016ba8af26451831f1f4ec07ea264cf115` plus
27 owned overlays, including the prior backup/tombstone fixes. The delivery
refresh matches it. Native tree `833d3085815b396861ad18a77635412212381e4b`;
migrations `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; canonical head
`0064_auction_custody_history`. The branch's historical native source is not
substituted for the published native source in Linux qualification. Required
raw docs are retained; `AI_CONTEXT.md` is absent from the published tree.

| Source | Git tree | Archive SHA256 |
| --- | --- | --- |
| 00, unchanged reader | `64bbb9ec435b5777ccf110e21ad20830c98db9d9` | `4c82e22ebf78d9115d80beba8e8ad9d40415213f1cc48af640fa296a232086b5` |
| 01, three regressions/old reader | `ebd5025ffb6807c0b94db0074b544478d314bbab` | `df9b657f08fe311d5e3971a8bb6c406e36060aed26140ae51ae420782bf27b99` |
| 02, fixed reader/initial CLI assertion | `628ec3c419f85ba957d5b2190227d6fb2d7df6c8` | `6e9dd7560a026bfc120e18a75062509028a06ffa6666b101184b1635d25cf1b9` |
| 03, complete final slice | `325ae4bf625b0cb470bc64a642ed00189b9b1201` | `893dd5089c1a3e836122d11f43f6bc813d8d60ed884df5c5e88160ed49503d32` |

Every archive authenticates 6,512 Git blobs: 6,508 regular files/four link targets,
including exact modes and link bodies. Post-stage guards preserve them. Final
reader blob `df24019fef7139931b8381ed093183984fc714bd`, SHA256
`1c7133dcc9c57dfe0fd38e088780e479faf121012f15cacdda2d361bfce811e8`;
unit blob `cc09d083037dbef4536d14fc963ee09e47af95b8`;
SQL runner blob `14f5c152e7ab0dc42792ffc0d633575dba38d382`.

Two healthy modeled controls (live/quarantined) stay clean. The old reader accepts
both event/current-state substitutions with no findings and one verified original
plan. Event-only substitutions produce stale-native findings but still wrongly
verify the capsule. All six original EAP inputs pass independent decode and both
original native SQL/flatfile codecs, exactly re-encoded: 12 executions. The new
comparison adds exactly one custody mismatch for each damaged selected endpoint
and removes it from the verified-plan count. It also runs before the selected
history's lineage shortcut. Original bytes/objects remain unchanged; provenance
retains the captured state, including at limits0/1/100.

The old reader's three methods produce 18 failed subtests/zero errors/zero skips;
red01 retains exit1. The first fixed host run exposes six test-authoring errors:
the CLI exceptions view uses the report shape directly, rather than the separate
provenance view's coverage/rows shape. Host02 preserves those errors with exact
source02; only the assertion shape is corrected for source03. The intended
return code, exact finding and immutable-input checks remain. Failed runs are
not erased or relabeled as successes.

## Executed validation, backends and commands

Tools image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
GCC13.3/Python3.12.3/nm2.42/mysql_config10.11.14, with tool body/version hashes in
`seal/native-build-inventory.json`. New build/evidence roots are on D:, directly
mounted. Private partial-SQL databases use dedicated tmpfs; the original canonical
runner's required `/plan5-restore-coins-*` roots use a fresh isolated Docker layer
on the existing D: Docker disk. Network none/no ports, original strict flags,
ASan/UBSan, deadlines and recipes unchanged. No existing database/service/volume
is used as a test target. Docker recovery was handled elsewhere; Plan5 did not
repair/reset Docker storage. No test was restarted solely for an observation timeout.

For each container stage below, the exact entry is
`python3 -u -B /evidence/<stage>/observer.py <stage> <source>`. Full host argv,
mounts, limits and image are in each `docker-command.json`; observed parent argv,
exit statuses/deadlines/timings are in `commands.json`/`launches.json`.

| Stage | Source | Actual result | Seconds |
| --- | --- | --- | ---: |
| reproduce00 | 00 | Two false-clean substitutions, two incompletely diagnosed event-only controls, two healthy controls;12 native round trips | 1.838471 |
| red01 | 01 | Three methods/18 failed subtests, zero errors/skips; expected exit1 | 1.825720 |
| checks03 | 03 | 16 modules/404 loaded/386 PASS/18 original opt-in skips; both complete partial-SQL runners PASS | 240.531250 |
| canonical03 | 03 | Original complete native restore method PASS, zero skips, both fresh 0064 engines | 300.585445 |
| codec03 | 03 |20 native exact round trips: six modeled inputs/four captured SQL capsules x two modes; fixed reader findings exact | 2.583575 |

The Linux unit observer names all 16 existing modules and enables the original
`DURIS_RUN_AUDIT_BUDGET=1`; every skip and method result is retained. The 18 skips
are pre-existing separately opted-in native/SQL methods. The separately enabled
original `RestoreCoinEffectsTests.test_native_coin_effects_both_modes_and_canonical_engines`
runs with `DURIS_RUN_RESTORE_COIN_INTEGRATION=1` and
`DURIS_PLAN5_CANONICAL_EVIDENCE=1`, and has zero skips. The SQL command is
`python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`, with generated
private socket/disposable-schema settings and SELECT-only reader credentials.

MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1`; MySQL `8.0.46-0ubuntu0.22.04.4`.
The partial-SQL runner uses its existing manual 47-table fixture, not a complete
canonical economy. Each engine retains its two original collector/system-custody
controls, authors one explicit model capsule from each captured SQL cut, stores
it in that disposable root and recaptures it through the unchanged exporter.
Those four captured capsules subsequently pass both original native codecs.
This is modeled capsule/capture evidence, not genuine collector gameplay or
intent/command/publication authentication.

For each engine, four damaged saved projections (two state directions x selected-
only/history-and-native) yield exactly one added custody mismatch. Exceptions and
provenance CLI views run at limits0/1/100: 24 actual checks per engine. All 47 table
inventories remain unchanged across reads/audits/CLI; all objects/input files stay
unchanged. Each plan-bound capture has one rollback/closed cursor. The original
root metadata is restored after each model capsule, and the original source
fixture is restored at the end. Existing partial findings remain; the fixture is
not promoted to complete or clean. Exact nested CLI argv are recorded by the
executed runner in `sql-cli-command-records.json`; they are distinguished from
parent-observer records rather than described as parent raw observations.

The unchanged complete canonical native suite agrees with independent decode on
3,026 cases/1,054 accepted in both native modes. Each fresh 0064 engine completes
109 cuts/90 expected refusals, including 58 full-entry cuts, original pending-claim,
intent/history and boundary controls, with authority unchanged. Parent observation
records 58 run commands/60 launches. Its 18 successful g++ driver calls are two
fixture compile/link commands plus 16 toolchain queries; nested build totals are
not inferred. Separate task-only native probes are authenticated reuse from the
previous zero-net slice: binary hashes and all native bodies/modes match, with
14 genuine providers/original strict flags. No fresh probe build is claimed.

Windows host component validation also runs the same 16 unit modules against exact
owned blobs: 404 loaded/383 PASS/21 platform/opt-in skips, zero failures/errors,
171.328000s. Its actual command is Python3.12.10 `-B`
`D:/Dev/Temp/accounting-plan5-item-plan-state/host_checks.py`; logs/884 observed
parent calls and host interpreter SHA are in `hostchecks03`. It is supplementary
Python proof, not a native qualification of historical branch source. A read-only
WSL capability inventory was taken while Docker was unavailable; no WSL build or
database qualification is claimed.

## Evidence, integration and remaining gates

Evidence `D:/Dev/Tests/Duris/accounting-plan5/item-plan-state-20261008`;
helpers `D:/Dev/Temp/accounting-plan5-item-plan-state`;
builds `D:/Dev/Builds/Duris/accounting-plan5-item-plan-state-20261008`.
Raw seal SHA256 `c2624344ba1d609a07ab781eb0c0e0ff5a613887ceb0a020d9b9e68f36e9d6c4`: 1538 files/1225138937 bytes, including
1227 regular files under build roots. All six containers stopped/no OOM;
no live recorded children, copied links or reparse points. Native lstat/body
inventory precedes regular-only copying/Windows extended-path rehashing. Source
manifests, transport/AST binding, reproduction/red/failed-host/full unit/SQL/native
logs, retained EAPs, command records and original partial findings remain sealed.
Reports and post-push delivery helpers/receipt are additional post-seal artifacts,
not retroactively counted in the raw seal.

The earlier tombstone commit `c373f04fac0ef14f734bc086c1cef01246dc70fe`, restore blob
`23b17a3d158ac455105de1cd1303561dcc028ad8`, remains included in all 27 overlays and
preserved remotely. Published primary still has restore blob
`e78cef8d3dbd4e48c507d49e443b03fa5208f86f`; preserve/import the fix with backup
integration. Earlier 24-overlay evidence retains its original scope.

The original flatfile-authority fixture's seven genuine-provider link symbols,
nine missing-record controls/full 12-method backup qualification, room-seed UID-
owner link and original native SQL baseline recipe/head closure remain primary-
owned open gates. They are not rerun here. Further independent history/preimage
consistency review, complete openings, genuine producer/cold world/wallet lifetimes,
admission/recovery/ACK, registry/matrix, player/routes/fault/load journeys, the
published combined candidate, activation-owner qualification, Plan5/R1-R8 and
release completion remain open. Whole-server builds/gameplay boots are not rerun
for this Python-only endpoint-comparison change. Private primary preparation is
not substituted for published executable evidence.

This report and additive remote follow-up are curator-ready notebook input;
primary-local notebook nonblocking, curator application/ack unclaimed. Inactive
behavior, wallet-root ITEM_MONEY exclusions and declined inactive spell change
remain. No autocorrection, activation, production access/write, primary-branch
push, deployment or merge.
