# Plan 5: supplied UID history envelopes - 2026-10-08

The independent reader now examines supplied unattributed ownership rows even
when optional coverage is absent. Malformed supplied rows refuse reconciliation
instead of being silently accepted. Invalid lineage-history envelopes retain the
existing global `missing_lineage_uid_history` finding through item indexing and
all operator views. Full Plan 5/R1-R8/release remain unqualified.

## Delivery and ownership

- Local and remote branch: `codex/accounting-plan5`; no branch switch.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `a8f4a57da6d208d9faa7f31bab1ee940752ecad2`.
- Result is this containing commit. Exact result SHA, remote equality, clean
  worktree, preserved historical tips and post-push raw rehash are recorded in
  `D:/Dev/Tests/Duris/accounting-plan5/history-envelopes-20261008/delivery/result.json`.
- Production edits are confined to existing `Reconciler.audit`,
  `Reconciler.audit_unattributed_uid_history` and `view` in the owned reader.
- Four regression methods are added to the existing reader test class. The
  existing disposable-SQL runner gains `verify_uid_history_envelopes` and one
  call with the genuine captured prior-epoch creation/unlinked retirement.
- Docs: this report, `AUDIT_OPERATIONS.md`, additive `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.
- AST comparison excludes only those methods/functions and the exact new SQL
  call, and proves the rest unchanged. All 27 tested overlay blobs match.

No shared interface/schema/coordinator/native/producer/writer registry change is
requested. Existing history arrays and coverage fields suffice. No new finding
name or mutation dependency is introduced. The reader performs no correction.

## Established defect and behavior

Before this change, `audit_unattributed_uid_history` returned as soon as either
rows or coverage were absent. Supplying rows without coverage therefore bypassed
both their shape validation and `unattributed_ownership_event`. Seven reproduced
cases audited clean: six malformed collections and one valid unlinked retirement
of UID86 (revision1 to2, tombstone owner8). Adding coverage to the unchanged valid
row made the old reader report the event, confirming the coverage-dependent gap.

The lineage validator already emitted `missing_lineage_uid_history` for invalid
present envelopes. The later `audit_items` input set still iterated scalar1 or
True and raised TypeError, losing the structured result. The provenance view
also concatenated unchecked optional history values or called `row.get` on
invalid rows. Old CLI scalar/string errors were controlled exit2; list entries
None/string caused AttributeError tracebacks. They are recorded distinctly in
`reproduce00/models/model-results.json`; not every old failure was a traceback.

The fix validates any supplied unattributed collection before the coverage gate,
retains SQL-partial missing-coverage findings, and still checks every known row.
Outside SQL-partial capture, optional absent rows/coverage and empty rows remain
optional. Present coverage with absent rows reports missing history. Valid
supplied unlinked rows always report `unattributed_ownership_event`. Malformed
supplied unattributed history raises the existing SnapshotError, and CLI exits2
with empty stdout and the fixed diagnostic. Invalid lineage collections keep the
existing finding; item indexing and provenance only traverse list/dict entries.
They do not convert invalid capture evidence into valid history.

The same exact original EAP1 capsule appears in every model. The independent
Python decoder accepts it; authenticated reused native SQL/flatfile probes both
accept and reproduce its exact bytes before and after the reader fix (one unique
capsule, two executions per model stage). These checks qualify capsule structure
only. The observed false-clean behavior is in audit envelopes, not a native
producer or recovery journey. Objects and input files are unchanged.

## Exact tested composition

The branch's historical native tree is not the native test source. Tests use a
frozen refreshed published tree with the 27 explicit owned overlays, including
the prior tombstone restoration `c373f04fac0ef14f734bc086c1cef01246dc70fe` and
restore blob `23b17a3d158ac455105de1cd1303561dcc028ad8`.

- Refreshed primary before choosing the base: `960ddd80c1acba5611677270f0156675d6d020ff`.
- Final tested composed tree: `0b07878b22f2ab1410caa2837d5e7cc5ec25c5d2`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`.
- Migration tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; genuine original fixture migrates
  through `0064_auction_custody_history`, including canonical0056.
- Final archive SHA256: `399e3c7e24d62a7e0c7acf295a3e25c79488cffc1c19d486ec2c3c481dc7accc`.
- Delivery refresh: `95d6a8c686b34a5722bbc671df72dc9af87cadbb`. Its changed files are
  ["docs/persistence/economy_accounting/FINISH_ACCOUNTING_PLAN.md", "docs/persistence/economy_accounting/domain-separation/CONTINUING_PROJECT_COORDINATION.md", "docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md"]. Native/schema and actual executed
  test inputs remain unchanged. Private primary source integration remains
  separately unexecuted; this evidence does not qualify that candidate.

| Source | Composed Git tree | Archive SHA256 |
| --- | --- | --- |
| 00 | `3858c14eb52ac574024de150c9c93925e2a803ff` | `45547dc0fde3d38ff26e61a9dd7aef61102c3cdcf9f5d2a6a025794b5a65e29f` |
| 01 | `5a8f9d443c2de46457957a60ef57974df33b2b37` | `55ab7216c1fc52de8087a260db0a6f170116e37e242f412b51d081cb2e356947` |
| 02 | `0ec7a08dc3275abf3b396cfbcdaea7b92694fba1` | `d40c0eee5550d9549780d6c58787ee856a1420b860f52481719961d5d45ded94` |
| 03 | `a35cd863b9e07301010ff7676d226130dd3ec7e3` | `939a3925df36be5d273c382d181611bab1879c6a77572bbe99a591e2373a59ee` |
| 04 | `0b07878b22f2ab1410caa2837d5e7cc5ec25c5d2` | `399e3c7e24d62a7e0c7acf295a3e25c79488cffc1c19d486ec2c3c481dc7accc` |

Each of the five source archives authenticates all 6515 Git blobs (6511 regular
files and four link targets) against Git. Every executed stage guards source bodies, modes and link
targets before/after execution. There is no owner shared-recipe edit, shim,
replacement native test, stub, reduced fixture, or weakened original flags.

| Owned executed source | Git blob | SHA256 |
| --- | --- | --- |
| `scripts/reconcile_economy_accounting.py` | `da7eb5dfaa8a88bf126bf9675667ee8e10f81282` | `919f89151c5e85cc454c904037dd4211c874b2533ccf49be5b772bab1b1cb94a` |
| `tests/async/test_reconcile_economy_accounting.py` | `bb064cd79eb5bba7754ea9f0561125cc45a31897` | `35bcc57916879a2079aca20580da6a93463d0920d1682198a177efbb9fc5ba76` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `a9091d9419675272ac78530637644e6ba0216bf7` | `1674dd3354d194c1c320726c9644db52785f060ecf9cc56bc2c1fcbcdfc2d3ca` |

## Commands, backends and results

Each launch is recorded as full argv in `<stage>/docker-command.json`; every
parent-observed run is in `<stage>/commands.json` and launch in `launches.json`.
The copied observer/model/launch helpers define the exact bounded execution.
Windows launched `C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe`
with `D:/Dev/Temp/accounting-plan5-history-envelopes/launch.py <stage> <source>`.
The image is pinned to
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
All containers use network none and task-specific direct D: evidence/bin mounts;
build caches are off. Native tool paths, versions and binary hashes are in
`seal/native-build-inventory.json`; original recipes/flags and real provider
paths are retained in canonical command records. No full maintained server
rebuild is claimed for this Python-only slice.

| Stage | Source tree | Exit | Seconds |
| --- | --- | --- | --- |
| canonical02 | `0ec7a08dc3275abf3b396cfbcdaea7b92694fba1` | 0 | 312.042506 |
| checks02 | `0ec7a08dc3275abf3b396cfbcdaea7b92694fba1` | 1 | 196.515818 |
| checks03 | `a35cd863b9e07301010ff7676d226130dd3ec7e3` | 1 | 194.093256 |
| codec02 | `0ec7a08dc3275abf3b396cfbcdaea7b92694fba1` | 0 | 11.228989 |
| focused02 | `0ec7a08dc3275abf3b396cfbcdaea7b92694fba1` | 0 | 11.093537 |
| red01 | `5a8f9d443c2de46457957a60ef57974df33b2b37` | 1 | 12.216159 |
| reproduce00 | `3858c14eb52ac574024de150c9c93925e2a803ff` | 0 | 11.308533 |
| sql04 | `0b07878b22f2ab1410caa2837d5e7cc5ec25c5d2` | 0 | 120.373055 |

- `reproduce00`: 15 models, 90 actual reader CLI invocations; seven false-clean
  audits plus separately classified old lineage/API/provenance failures.
- `red01`: exact four new tests against old reader, four methods loaded,
  95 failed subtests and six errors, zero skips. Exit1 is expected and retained;
  errors reproduce old TypeError/AttributeError behavior. The later SQL
  expectation failure is retained separately below.
- `focused02`: the same four methods pass with zero failures/errors/skips.
- `checks02`: all 16 modules pass, but the new MariaDB test expectation failed
  because removing lineage also removes UID86's creation witness. This retained
  test-authoring failure was corrected to require missing creation and the
  corresponding anchored-owner count. The production reader is unchanged.
  Source03 adds five expected-count lines in the new SQL scenario.
- `checks03`: all 16 complete reader/exporter/custody modules again pass,
  419 loaded, 401 pass, 18 original opt-in skips, zero failures/errors. Existing
  enabled budgets also pass and remain synthetic component evidence. The new
  MariaDB scenario then failed its raw CLI/API equality assertion because view
  denomination tuples serialize as JSON arrays. The failure is retained.
- Final source04 only normalizes that new scenario's expected view through JSON.
  Read-only diagnosis confirms the representation difference in holdings and
  operation output; reader behavior and findings were already correct. The only
  source02-to04 changes are these two new SQL expectation fixes. All actual
  focused/unit/codec/canonical inputs are identical, explicitly compared.
- `sql04`: both complete disposable SQL audit snapshot runners pass from final04.
  Unchanged full module tests already pass in both checks02 and checks03; they
  were not repeated for the SQL-only comparison change.
- New regression CLI matrix: five saved models, seven existing views, limits
  0/1/100 =105 actual CLI calls per execution of the new method. All views retain
  global findings or strict refusal; details obey bounds, aliases do not leak,
  input objects/files remain unchanged. This runs in red, focused and checks.
- New SQL scenario: four damaged saved captures, seven views, limits 0/1/100 =84
  actual CLI calls per engine, 168 total. The genuine SELECT-only capture already
  includes prior-epoch UID86 creation and an unattributed retirement. Damage
  changes only saved JSON: scalar/list-invalid lineage, absent unattributed
  coverage, malformed unattributed rows. The missing-coverage case retains the
  known retirement and adds `missing_unattributed_uid_history`. All 47 application
  tables remain byte/logically inventoried identically; original capture remains
  unchanged. No new native producer/capture mutation is claimed.
- Original `test_restore_economic_coin_effects`: one full original method passes,
  both engines at current64; 3026 decoder decisions, 1054 accepted; each engine
  has 109 canonical restore cuts, 90 refusals, 58 full-entry cuts with authority
  unchanged. Native original canonical/coin/pending-claim markers are retained
  without claiming new production gameplay. This original fixture was freshly
  compiled from source02 native inputs with its existing flags. Source04 changes
  only the new SQL scenario expectations; actual canonical inputs are identical.
- Canonical parent observer sees two successful g++ fixture compile/link calls
  and 16 successful toolchain query calls (18 g++ driver calls, not 18 builds).
  Nested compiler totals are not inferred from parent observation.
- `codec02`: all 15 models/90 CLI calls have the expected fixed behavior; original
  capsule and native providers/binaries are authenticated against zero-net
  evidence, two exact codec executions, no fresh probe compile claimed.

Actual original fixture backend versions: 10.11.14-MariaDB-0ubuntu0.24.04.1 / 0064_auction_custody_history; 8.0.46-0ubuntu0.22.04.4 / 0064_auction_custody_history.
SQL runner stdout markers are in `sql04/whole-snapshot-{mariadb,mysql}.log`.
Each new SQL case retains its input, exact argv/exit/stdout/stderr in
`D:/Dev/Builds/Duris/accounting-plan5-history-envelopes-20261008/sql04/bin/tests/plan5-history-envelopes`;
aggregate records are in `sql-cli-command-records.json`. Assertions compare
full global counts and bounded CLI/API output while inventory guards SQL.

Original opt-in skips in the 16-module run (canonical fixture separately enabled):

- `test_native_stake_sql_both_engines (test_reconcile_economy_accounting.NativeStakeSQLTests.test_native_stake_sql_both_engines)`: requires explicit disposable Linux native/SQL stake invocation
- `test_native_captured_source_bindings_both_engines (test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_captured_source_bindings_both_engines)`: requires explicit disposable Linux native/SQL integration invocation
- `test_native_origins_mariadb (test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mariadb)`: requires explicit disposable Linux native/SQL integration invocation
- `test_native_origins_mysql_8 (test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mysql_8)`: requires explicit disposable Linux native/SQL integration invocation
- `test_fair_composite_pages_and_unattached_namespaces_both_engines (test_economic_sql_canonical_audit.NativeCanonicalAuditTests.test_fair_composite_pages_and_unattached_namespaces_both_engines)`: requires explicitly selected native and fresh private SQL checks
- `test_maximum_baseline_page_both_engines (test_economic_sql_canonical_audit.NativeCanonicalAuditTests.test_maximum_baseline_page_both_engines)`: requires explicitly selected native and fresh private SQL checks
- `test_original_plans_and_projection_faults_both_engines (test_economic_sql_canonical_audit.NativeCanonicalAuditTests.test_original_plans_and_projection_faults_both_engines)`: requires explicitly selected native and fresh private SQL checks
- `test_resumable_pages_delayed_commit_and_growing_tail_both_engines (test_economic_sql_canonical_audit.NativeCanonicalAuditTests.test_resumable_pages_delayed_commit_and_growing_tail_both_engines)`: requires explicitly selected native and fresh private SQL checks
- `test_near_limit_child_snapshot_is_bounded_and_limit_invariant (test_plan5_child_identity.ChildIdentityBudgetTests.test_near_limit_child_snapshot_is_bounded_and_limit_invariant)`: requires explicitly selected child workload qualification
- `test_native_child_plan_and_sql_cuts (test_plan5_child_identity.NativeChildIdentityTests.test_native_child_plan_and_sql_cuts)`: requires explicitly selected native/private SQL qualification
- `test_both_canonical_engines_native_capture_and_independent_read_only_cuts (test_sql_auction_custody_audit.NativeAuctionCustodyAuditTests.test_both_canonical_engines_native_capture_and_independent_read_only_cuts)`: requires explicit disposable Linux native SQL invocation
- `test_both_canonical_engines_native_sources_and_read_only_forests (test_sql_shop_custody_audit.NativeShopCustodyAuditTests.test_both_canonical_engines_native_sources_and_read_only_forests)`: requires explicit disposable Linux native SQL invocation
- `test_both_canonical_engines_native_cuts_and_coin_codec (test_sql_player_custody_audit.NativePlayerCustodyAuditTests.test_both_canonical_engines_native_cuts_and_coin_codec)`: requires explicit disposable Linux native SQL invocation
- `test_both_canonical_engines_unmodified_physical_parser (test_sql_corpse_custody_audit.NativeCorpseCustodyAuditTests.test_both_canonical_engines_unmodified_physical_parser)`: requires explicit disposable Linux native SQL invocation
- `test_both_canonical_engines_actual_current_capture_and_raw_locker_cuts (test_sql_locker_custody_audit.NativeLockerCustodyAuditTests.test_both_canonical_engines_actual_current_capture_and_raw_locker_cuts)`: requires explicit disposable Linux native SQL invocation
- `test_both_canonical_engines_actual_siege_capture_and_coin_codec (test_sql_siege_custody_audit.NativeSiegeCustodyAuditTests.test_both_canonical_engines_actual_siege_capture_and_coin_codec)`: requires explicit disposable Linux native SQL invocation
- `test_both_canonical_engines_current_capture_modern_presence_and_coin_codec (test_sql_saved_ground_custody_audit.NativeSavedGroundCustodyAuditTests.test_both_canonical_engines_current_capture_modern_presence_and_coin_codec)`: requires explicit disposable Linux native SQL invocation
- `test_both_canonical_engines_complete_raw_graph_and_refusals (test_sql_room_item_custody_audit.RoomItemSQLTests.test_both_canonical_engines_complete_raw_graph_and_refusals)`: requires isolated canonical SQL services

## Evidence, curator input and remaining gates

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/history-envelopes-20261008`. Build root:
`D:/Dev/Builds/Duris/accounting-plan5-history-envelopes-20261008`.
`qualification.json` binds stages/commands/results/versions to source. Raw seal
`seal/evidence.json` SHA256 is `b399daaef38b12bf52476ec432c866fcc703923fc23aa77f39729e7aecdbd428`; it covers
3281 regular files / 1612548819 bytes,
including 2710 regular build files. Build-file counts are
not compiler-job counts. Native lstat inventories precede regular-only hashes;
no symlink/reparse body is followed or copied. Nine containers are stopped,
network-isolated, not OOM-killed; the expected red exit is retained. Post-push
receipt rehashes the seal, verifies the exact remote result/owned blobs, and
preserves all seven historical tips plus subsequent delivered fixes.

This report and additive remote follow-up are curator-ready project notebook
input. The primary-local shared notebook is nonblocking under the user's
instruction; application/import/acknowledgement are not claimed. No cross-chat
message is sent. Primary integrates this slice from the sole expected remote
branch and publishes/tests the combined candidate.

Shared native fixture handoffs remain with the primary: genuine provider closure
for the original backup-record-loss and SQL-baseline recipes, and the original
room seed UID-owner boundary. Full original12-case backup qualification still
has its recorded unresolved link failure; this slice does not silently rerun or
substitute those shared tests. See
[the exact current64 full-backup handoff](PLAN5_CURRENT64_FULL_BACKUP_MODULE_HANDOFF_2026-10-08.md).

Other remaining gates: complete genuine producer/opening/wallet/treasury/player/
route coverage, original backup/restore/retention/fault/load acceptance, inactive
behavior and declined spell path, current private combined native/SQL/recovery
execution, ACK/notification provenance, activation-owner decision and full
R1-R8/release qualification. Synthetic models, inventory coverage and isolated
passes do not close these gates. No activation, production data modification,
audit autocorrection, deployment, PR merge or primary-branch push occurred.
