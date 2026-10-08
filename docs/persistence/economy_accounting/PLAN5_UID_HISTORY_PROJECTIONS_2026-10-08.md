# Plan 5: conflicting selected/lineage UID projections - 2026-10-08

The read-only reconciler now compares overlapping selected-epoch and lineage
ownership projections at their original native `(operation_id,event_index)` key.
Previously lineage history could take precedence while disagreeing with the
selected event and its authenticated original EAP1 plan. Changing lineage and
current custody together concealed both live/quarantined substitutions, a
changed owner and a changed equipment slot: four false-clean cases. They now
produce `conflicting_uid_history_projection`, without modifying either copy.
Full Plan5/R1-R8 and release remain unqualified.

## Delivery and ownership

- Branch, local and remote: `codex/accounting-plan5`; no branch switch.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `d0fb98edb34f351442ea4ec397499461bf808eff`.
- Result: this containing commit, with its exact SHA and remote equality recorded
  after push in `D:/Dev/Tests/Duris/accounting-plan5/history-projections-20261008/delivery/result.json`.
- Owned reader change: `scripts/reconcile_economy_accounting.py`, only existing
  `Reconciler.audit`. Existing ownership index and strict `same_projection` reused.
- Owned tests: four new methods in the existing reconciliation class; the prior
  selected-state/history method retains its old capsule finding and adds the
  newly detected conflict. Existing `verify_collector_quarantine_views` is extended in the SQL runner;
  `verify_compound_item_actions` retains its invalid-supply finding and adds
  the newly required conflict count for its selected-only action damage.
- Owned docs: `AUDIT_OPERATIONS.md`, this report and additive remote follow-up.
  `scope-binding.json` proves all other ASTs/27 overlays/native/migrations exact.
- All seven earlier tips, prior endpoint-state99c4ce0dc and tombstonec373f04fa are
  preserved on this remote branch. The delivery receipt verifies ancestry.

No shared interface, schema or native change is requested. The already captured
common fields are UID, before/after revision, root, parent, owner, state and action.
Previous owner and from/to equipment slots are compared when recorded in both
copies; historical omission remains unknown under existing evidence-loss checks.
The matching key deliberately excludes UID, so a conflicting UID at the same
native ledger key cannot evade comparison. Events at distinct native keys are
not treated as overlapping. Invalid history containers keep their existing
`missing_lineage_uid_history` refusal, without dereferencing invalid rows.

This is a global consistency finding, separate from capsule authentication:
when the selected event still matches its EAP, its verified-plan count stays1;
the conflicting lineage copy makes the overall audit refuse. A selected-only
state substitution now retains the old `original_plan_custody_mismatch` AND the
new history conflict. The normal exception emitter exposes operation ID and UID;
the original event indices and conflicting values remain in bounded provenance.
No new exception schema/identifier policy is introduced. Primary owns shared
registration; retain these four existing-class methods:

- `ReconciliationTests.test_uid_history_projection_conflicts_with_selected_state_owner_and_slot`
- `ReconciliationTests.test_uid_history_projection_compares_full_record_at_native_event_key`
- `ReconciliationTests.test_uid_history_projection_keeps_historical_omission_unknown`
- `ReconciliationTests.test_uid_history_projection_cli_preserves_conflicts_and_limits`

## Exact source and established defect

Published primary refreshed before choosing the test base: `5a775b3eb6e5f859e8225bd5129216c97c8fcbcc`.
All freezes compose its exact tree with27 owned overlays, including prior backup
and tombstone fixes. Native tree `833d3085815b396861ad18a77635412212381e4b`; migrations
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; canonical head `0064_auction_custody_history`.
The owned branch's historical native tree is not used in place of primary native
source. Required raw AGENTS/README/finish/requirements/Plan5/latest checkpoint and
private source-integration note are retained; published `AI_CONTEXT.md` is absent.
Qualification refresh primary `5990082efc2e3c0e4deffac9791d3e8e982c9733` has documented changes
`['docs/persistence/economy_accounting/FINISH_ACCOUNTING_PLAN.md', 'docs/persistence/economy_accounting/domain-separation/CONTINUING_PROJECT_COORDINATION.md', 'docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md']`. Later post-push refresh is receipt-authoritative.

| Freeze | Git tree | Archive SHA256 |
| --- | --- | --- |
| 00 | `030a3f5db28a789dcec604109fe703935c80d6d2` | `05565f76d1bde8f2cc7403e9775cfe00185c2f5c33a46b3f99b4d85ac1a65aca` |
| 01 | `675fa1fc3e14c73537d0e0a463ab9c505346f7cf` | `11e4cce50aa35999520f8c418a071002f93e16a63184e5bbd014061203d1ca49` |
| 02 | `eaa85e5abaa425cb4052e252ea12649f068dbc1a` | `eae566b9b2efd2706a0c3aaf6c898e852da37ec80db22a9db617d567371a2af9` |
| 03 | `79d9f8c8bde6d2ca8e4e0e61fe05c808b49ee340` | `2e47a8a64bf357f63fe5ebd8afbefb723a59f47255ee491b45592722d45266db` |
| 04 | `6d45375ece36e404981f097413e01f73e17495f9` | `c4083b205d495a6a116a3d408e39a511ee7adb7f3ec565ea1fd792918944588b` |
| 05 | `2fa21977d57af9b5eb228f2ea420b822d1d8b788` | `c347275e60cbc0017c027d914302fde5f21aa3acda84e81ce1f92105d67c6fc9` |

Each archive independently authenticates6,513 Git blobs:6,509 regular files/four
exact link targets, including body and mode transport. Final reader blob
`7346bc57de775b9e203a9341d94b0c4e9c750bad`, SHA256
`c9540cb5e4b86e5892984972622a1070e1ee37612395121bd4bf323e3462b2b0`.

Freeze00 establishes two healthy controls, four false-clean lineage/current
substitutions and two history-only cases with an existing stale-native finding.
All eight original capsules independently decode and exactly round-trip both
native codecs:16 executions. Old-reader freeze01 produces36 failed subtests in
four methods, zero errors/skips, expected exit1. It remains preserved.

Freeze02's first fixed run detects all10 position conflicts but fails10 test
assertions because they incorrectly expected `event_index` in the existing
filtered exception row. It stops after the156-test reconciliation module;
no whole-suite/SQL pass is claimed for checks02. The malformed-container guard
is added before freeze03. Freeze04 corrects only that expected identifier row;
all intended findings, return codes, limits and input guards remain asserted.
The failed stage and both intermediate freezes are retained.

Checks04 passes all16 component modules but stops in the first SQL runner at the
older selected-only action damage, whose expected counts omit the new conflict.
Freeze05 changes only that expected SQL count. SQL05 executes both complete
original runners against that exact final runner. The failed checks04 SQL output
is retained; no complete SQL pass is claimed for checks04.

The native canonical run is exact freeze03, component run exact04, SQL and final
codec run exact05. Between03 and05 only two test files change: the new exception
identifier assertion and the old selected-only SQL expected count. All production
reader/native/schema/recipes/helpers/actual canonical inputs remain identical.
Between04 and05 only the SQL runner changes; all component inputs are identical.
`canonical-to-final.patch`/`component-to-final.patch` and qualification binding
record these distinctions. Earlier runs are not relabeled as runs on freeze05.

## Executed checks and backends

Tools image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3/Python3.12.3/nm2.42/mysql_config10.11.14; exact executable bodies/versions in
`seal/native-build-inventory.json`. D evidence/build directories mounted directly;
network none/no ports/private tmpfs databases. Canonical runner's required roots
use its fresh container layer on existing D Docker disk. Original strict C++20,
ASan/UBSan flags, recipes and predeclared deadlines remain. No production or
existing database/service/volume is a test target.

Every actual stage entry is
`python3 -u -B /evidence/<stage>/observer.py <stage> <source-label>`.
Full host argv/mounts/image/limits are in stage `docker-command.json`; parent
commands/deadlines/exit/timing and launches in `commands.json`/`launches.json`.

| Stage | Exact source tree | Exit | Seconds |
| --- | --- | ---: | ---: |
| canonical03 | 79d9f8c8bde6d2ca8e4e0e61fe05c808b49ee340 | 0 | 309.190460 |
| checks02 | eaa85e5abaa425cb4052e252ea12649f068dbc1a | 1 | 92.427022 |
| checks04 | 6d45375ece36e404981f097413e01f73e17495f9 | 1 | 140.603554 |
| codec05 | 2fa21977d57af9b5eb228f2ea420b822d1d8b788 | 0 | 2.023844 |
| red01 | 675fa1fc3e14c73537d0e0a463ab9c505346f7cf | 1 | 2.558771 |
| reproduce00 | 030a3f5db28a789dcec604109fe703935c80d6d2 | 0 | 1.603387 |
| sql05 | 2fa21977d57af9b5eb228f2ea420b822d1d8b788 | 0 | 102.315890 |

Component checks04 runs16 existing modules:408 loaded,390 PASS,18 original opt-in
skips, zero failures/errors. Original bounded budget controls run with
`DURIS_RUN_AUDIT_BUDGET=1`. Every method/skip is retained. The separately enabled
original `RestoreCoinEffectsTests.test_native_coin_effects_both_modes_and_canonical_engines`
runs with `DURIS_RUN_RESTORE_COIN_INTEGRATION=1` and
`DURIS_PLAN5_CANONICAL_EVIDENCE=1`:1 PASS,zero skips.

Final SQL05 command `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`
passes completely on MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and
MySQL8.0.46-0ubuntu0.22.04.4. Each uses the existing manual47-table partial fixture,
dedicated disposable socket/schema settings and SELECT-only audit credentials.
Existing collector quarantine/system custody controls stay; one explicit model
capsule is authored from each of their captured SQL cuts, temporarily stored in
its disposable root metadata and recaptured by the unchanged exporter. These
four captured capsules later pass both original native codecs. They are modeled
capsule/capture evidence, not genuine collector gameplay or native intent/command/
publication proof. Other original partial findings stay visible.

Per engine: two state directions x four damaged saved projections = eight cases,
48 actual CLI checks (exceptions/provenance x limits0/1/100). Selected-only state
adds both original-plan mismatch and history conflict; selected+history+native
state keeps the original-plan mismatch; lineage-only adds conflict and existing
stale-native; lineage+native adds conflict. All47 authority inventories remain
unchanged across capture/audit/CLI, all input objects/files remain unchanged.
Original SQL model metadata is restored after each phase, and the whole original
fixture is restored at end. Each recapture has one rollback/closed cursor.

`sql-cli-command-records.json` retains96 nested invocations recorded by the
executed SQL runner, distinguished from parent-observer calls. New pure CLI
regressions also preserve both contradictory provenance rows, total finding at
limit0 and private-alias exclusion. Full position cases cover every common field,
both native event identity components, unknown optional fields and malformed
containers; healthy/non-overlapping controls retain prior behavior.

Canonical suite independently agrees on3,026 cases/1,054 accepted in both native
modes. Each fresh0064 engine completes109 cuts/90 expected refusals/58 full-entry
cuts, preserving original pending-claim, intent/history/boundary controls and
unchanged authority. Its parent observer's18 successful g++ driver calls are
2 fixture compile/link calls plus16 toolchain queries, not18 compiler jobs.
Nested builds are not inferred from parent counts.

Final codec05 executes24 exact native round trips: eight original model capsules
and four captured SQL capsules x SQL/flatfile. Task-only probes are authenticated
reuse from the zero-net slice, with all native source bodies/modes and binary
hashes matching,14 genuine providers and original strict flags. No fresh probe
build or whole-server build/gameplay boot is claimed. Native/shared files are
unchanged; Windows and original full backup modules are not repeated here.

## Evidence and remaining gates

Evidence `D:/Dev/Tests/Duris/accounting-plan5/history-projections-20261008`;
helpers `D:/Dev/Temp/accounting-plan5-history-projections`;
builds `D:/Dev/Builds/Duris/accounting-plan5-history-projections-20261008`.
Raw seal SHA256 `1a2d4b31ba3ca99373dde0f130e5ab3bdd6584402c4e3b21ef2229d712e64860`: 1959 files/1858409855 bytes,
including1542 regular files under build roots (not compiler
jobs). All eight containers are stopped/no OOM/live recorded children; the
expected old-reader, test-assertion and old SQL-expectation failures remain exit1. Native lstat/body
inventory precedes regular-only copying and extended-path Windows rehashing;
no copied links or reparse points. Reports/post-push receipt are additional
post-seal artifacts, not retroactively counted in this raw seal.

Earlier tombstonec373f04fac0ef14f734bc086c1cef01246dc70fe, restore blob
23b17a3d158ac455105de1cd1303561dcc028ad8, stays included in27 overlays and preserved
remotely. Primary published restore blob remains e78cef8d3dbd4e48c507d49e443b03fa5208f86f;
retain/import the completed fix with backup integration. Prior24-overlay reports
keep their original scope. Primary's private source-integration note accepts
packets through50acfeeb9 only, with a narrower SQL diagnostic composition and no
compiler/native/SQL/gameplay/recovery qualification. It does not establish import
or combined qualification of99c4ce0dc or this later slice.

Primary-owned original flatfile-authority fixture's seven genuine-provider link
symbols/nine missing-record controls/full12-method backup qualification, room-seed
UID owner link and original native SQL baseline recipe/head closure remain open.
They are not rerun here. Independent original-plan preimage review, complete
openings, genuine producer/cold world/wallet lifetimes, admission/recovery/ACK,
registry/matrix, player/routes/fault/load, published combined candidate,
activation-owner qualification and fullPlan5/R1-R8/release remain open. Passing
components/manual fixtures do not establish those journeys or release completion.

This report/additive follow-up/receipts are curator-ready input for the nonblocking
primary-local notebook. Curator application/ack and cross-chat messaging unclaimed.
Inactive behavior, wallet-root ITEM_MONEY exclusions and declined inactive spell
path stay. No audit autocorrection, accounting activation, production access/write,
primary push, deployment or merge.
