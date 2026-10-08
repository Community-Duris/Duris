# Plan 5: rejected lineage history consumers - 2026-10-08

The independent reader now respects its existing lineage-envelope rejection at
every consumer. A mixed or oversized rejected history array cannot suppress
native UID checks, add original-plan preimages, index unvalidated keys or reach
provenance decoding. The original missing-history finding stays global. Full
Plan 5/R1-R8/release remain unqualified.

## Delivery and ownership

- Sole local/remote branch: `codex/accounting-plan5`; no branch switch.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `f4ee8a62d040a19baa887a8883394c872ef1da67`.
- Result is this containing commit; exact result SHA, matching remote, clean
  worktree, preserved ancestors and post-push rehash are verified in
  `D:/Dev/Tests/Duris/accounting-plan5/lineage-history-rejection-20261008/delivery/result.json`.
- Reader changes: one private shape predicate reused by existing `audit`,
  `audit_lineage_uid_history`, `audit_original_plans` and provenance `view`.
  Rejected optional history is excluded locally after the existing validator
  records its finding. The input object is not modified. Replaced per-entry
  bypasses are removed; the full existing row grammar remains in its validator.
- Three regression methods are added to the existing reader test class.
- The existing SQL envelope scenario gains five saved-capture cases; all four
  prior cases, authority inventories, CLI bounds and strict refusals remain.
- Owned docs: this report, operator note and additive remote notebook follow-up.
  AST comparison excludes only the listed functions/methods and proves the rest
  unchanged. All 27 tested owned overlay blobs match.

No shared schema/interface/contract/coordinator/producer/writer registry change
is requested. Existing field names, finding names and availability flags remain.
No new snapshot format or mutation dependency is introduced. This is read-only.

## Established defect and fixed behavior

The lineage validator already reports `missing_lineage_uid_history` and returns
for non-list, oversized or mixed non-dictionary history. Later consumers still
iterated dictionary entries from that rejected collection. UID list/dictionary
keys raised TypeError in the known-history UID set; operation/event-index
list/dictionary keys raised TypeError in selected-history lookup. An oversized
100001-entry array also reached indexing. Seven reproduced cases therefore lost
the structured audit result (API TypeError, controlled CLI exit2).

Two more rejected arrays had malformed previous-owner values. Their dictionary
entry contributed a spurious selected/history conflict, then provenance tried
to decode the malformed tuple and refused with exit2. The existing missing
finding did not prevent unvalidated data from reaching these later consumers.

The focused corruption proof also establishes a more consequential effect:
`[None,{"uid":81}]` made the old reader bypass native comparison for UID81,
losing `stale_native_item` when its actual captured owner was changed. A rejected
array with a contradictory unchanged container position also contributed an
original-plan preimage. The fix prevents both uses: native stale evidence is
reported, and rejected history cannot authenticate or contradict a plan witness.
Missing historical evidence stays missing; selected/opening evidence still runs.

The private predicate recognizes only a bounded list of dictionaries. It does
not validate row semantics or replace the existing strict row grammar. The same
shape rule gates validation, UID-set/overlap consumers, original-plan indexing
and provenance iteration. Valid history keeps its previous behavior. Optional
absent history outside SQL-partial capture remains optional. The missing finding
survives at every detail limit; exclusion does not create a healthy empty cut.

No authority correction occurs. Inputs, original capsules and SQL tables remain
unchanged. The two model capsules (ordinary clean control and valid retained
container control) are independently decoded and accepted with exact native
SQL/flatfile roundtrips before and after the fix: two unique capsules, four
executions per model stage. These prove capsule grammar and stable inputs,
not a new native producer, recovery path or combined release candidate.

## Exact tested source

The owned branch's historical native tree is not the native test source. Frozen
composition uses refreshed published primary plus the explicit 27 owned overlays,
including the prior tombstone restore and every earlier delivered Plan5 fix.

- Primary refreshed before selecting base: `4d1e5d938aecd884d1d7beade30ec99d2692205a`.
- Final composed tree: `2553beb0f7a5146ca48dab60155ffc5d89e3d7ab`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`.
- Migration tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
- Canonical fixture head: `0064_auction_custody_history`, including canonical0056.
- Final archive SHA256: `b65a251fe9771f2b50898f3bb545ec8ed39ad83285ef23f776d28f85fd3d7166`.
- Qualification refresh: `4d1e5d938aecd884d1d7beade30ec99d2692205a`; changes since pinned primary:
  []. Native/schema and actual test inputs
  remain unchanged. Post-push receipt records the independently observed ref.

| Source | Composed Git tree | Archive SHA256 |
| --- | --- | --- |
| 00 | `1b8c5420fac4cf6a276aabd459d80c232e2c3859` | `28e9da684c1641871cc7bfffc53d4ab4a1288ff58d54fef80a7e9638b0bca78b` |
| 01 | `da25a4e0e965e91b321c68af92a1d47f56937d56` | `a356cdea76f58bbcfd13a3a2fc688ce5b17a7bf200cb5fca0b75a987d01ede32` |
| 02 | `2553beb0f7a5146ca48dab60155ffc5d89e3d7ab` | `b65a251fe9771f2b50898f3bb545ec8ed39ad83285ef23f776d28f85fd3d7166` |

Each archive authenticates 6515 Git blobs: 6511 regular files and four link targets.
Every executed stage guards source bodies, modes and link targets before/after.
All final stages use source02 exactly. No shared native recipe or assertion is
changed, no genuine provider is replaced, and no stub or reduced fixture is used.

| Owned executed source | Git blob | SHA256 |
| --- | --- | --- |
| `scripts/reconcile_economy_accounting.py` | `0d1fc8185be7e8e9eb65975331c1e424fb0f6f3b` | `a3a6e8df2346735e784cf5b0f4a1050d3724899a246a098cf59d367929d1dbbf` |
| `tests/async/test_reconcile_economy_accounting.py` | `15b64928daa435de55115a3acb344e3dd3f21ed4` | `edffd3e999d07832b85648bba083f997c5ba2864d85d78d70fa4256ef56a4c46` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `322b30a68c36095a5494daba9feddd6b4dad2a6f` | `84e7ea63c7c41881be5f3bdfd38ec017965e51c1237a0f07b399ac9c75d9d88b` |

## Commands, backends and results

Windows launches the host Python at
`C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe` with
`D:/Dev/Temp/accounting-plan5-lineage-history-rejection/launch.py <stage> <source>`.
Each exact Docker argv is retained in `<stage>/docker-command.json`; every
parent-observed run/launch is retained in `commands.json` / `launches.json`.
Copied observer/model helpers define the actual suite selectors and 2400-second
stage guard. Disposable SQL runner invocation is
`python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py` under each
private database context. This test process receives only disposable credentials.

The image is pinned to
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
All containers use network none and direct task-specific D: evidence/bin mounts;
build caches are off. Native tool paths/versions/SHA256 are recorded in
`seal/native-build-inventory.json`. Original strict native flags/provider source
paths and full commands are retained. No maintained-server rebuild is claimed
for this Python-only slice; the original native canonical fixture is freshly built.

| Stage | Source tree | Exit | Seconds |
| --- | --- | --- | --- |
| canonical02 | `2553beb0f7a5146ca48dab60155ffc5d89e3d7ab` | 0 | 320.233324 |
| checks02 | `2553beb0f7a5146ca48dab60155ffc5d89e3d7ab` | 0 | 300.369716 |
| codec02 | `2553beb0f7a5146ca48dab60155ffc5d89e3d7ab` | 0 | 9.576647 |
| focused02 | `2553beb0f7a5146ca48dab60155ffc5d89e3d7ab` | 0 | 13.141095 |
| red01 | `da25a4e0e965e91b321c68af92a1d47f56937d56` | 1 | 11.498054 |
| reproduce00 | `1b8c5420fac4cf6a276aabd459d80c232e2c3859` | 0 | 8.434280 |

- `reproduce00`: eleven saved models /66 actual CLI calls. Two healthy controls
  audit clean. Seven invalid cases raise TypeError; two previous-owner cases
  lose provenance output and add a conflict from rejected data. Models/capsules
  and exact stdout/stderr/argv are retained without conflating refusal with clean.
- `red01`: three new methods, 113 failed subtests, 21 reproduced errors, zero skips;
  expected exit1 retained. Native-stale suppression and invalid preimage reuse
  are covered directly, together with the bounded CLI/refusal behavior.
- `focused02`: all three methods pass, zero failures/errors/skips. The five-case
  CLI matrix exercises every existing view at limits 0/1/100:105 calls.
- `checks02`: sixteen complete relevant modules, 422 loaded /404 pass /
  18 original opt-in skips,zero failures/errors; enabled existing audit budgets
  pass. It also executes both complete disposable SQL audit snapshot runners.
- Existing SQL envelope scenario: all nine cases pass on each engine,189 CLI
  calls per engine /378 total. Five added cases cover mixed UID/operation/index/
  previous-owner entries and actual 100001-row rejection. Four prior cases remain,
  including strict malformed-unattributed refusal. Damage is authored only in
  saved JSON from the actual SELECT-only fixture capture. All 47 application
  tables, original saved capture and source fixture remain unchanged. Losing
  valid lineage also retains missing-creation/owner-evidence findings as expected.
  These are disposable modeled SQL captures, not real gameplay or producer proof.
- Original `test_restore_economic_coin_effects`: one full original method passes,
  freshly compiled on final02; both current64 engines, 3026 decoder decisions,
  1054 accepted. Each engine passes 109 canonical restore cuts, 90 refusals,
  58 full-entry cuts with authority unchanged. Original coin/pending-claim markers
  remain retained. Parent observation records two fixture compile/link calls and
  sixteen g++ toolchain queries (18 driver calls, not 18 builds); nested compiler
  totals are not inferred.
- `codec02`: eleven models /66 CLI calls; every rejected case retains exactly the
  existing missing finding, both healthy controls remain clean; four exact native
  roundtrips. Native probe providers/modes/binaries are authenticated against
  zero-net evidence; this reuse does not claim a fresh probe compile.

Original canonical fixture engine versions: 10.11.14-MariaDB-0ubuntu0.24.04.1 / 0064_auction_custody_history; 8.0.46-0ubuntu0.22.04.4 / 0064_auction_custody_history.
New SQL inputs/results/full argv/stdout/stderr/inventories are under
`D:/Dev/Builds/Duris/accounting-plan5-lineage-history-rejection-20261008/checks02/bin/tests/plan5-history-envelopes`.
Markers are in `checks02/whole-snapshot-{mariadb,mysql}.log`; aggregate calls are
in `sql-cli-command-records.json`. All global totals and bounds are asserted,
including limits 0/1/100. A synthetic private marker is absent from output.

Original opt-in skips (canonical fixture separately enabled):

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

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/lineage-history-rejection-20261008`. Build root:
`D:/Dev/Builds/Duris/accounting-plan5-lineage-history-rejection-20261008`.
`qualification.json` binds commands/results/backends to source. Raw seal
`seal/evidence.json` SHA256 `ef18f70be50a11f9263246394723ec833c9e9034d0cc576db35bb676bf09db92` covers
1745 regular files /968398998 bytes, including
1380 regular build files. These are file counts, not
compiler jobs. Native lstat inventories precede regular-only hashing; no
untrusted link/reparse body is followed or copied. Seven containers are stopped,
network-isolated and not OOM-killed; the expected red failure is retained.
The post-push receipt rehashes the sealed evidence and verifies exact owned blobs,
clean worktree, matching remote result, seven historical tips and later ancestors.

This report and additive remote follow-up are curator-ready notebook input.
Primary-local shared notebook remains nonblocking under the user's instruction;
curator application, primary import and acknowledgement are not claimed. No
cross-chat message is sent. Primary integrates from `codex/accounting-plan5` and
publishes/tests its combined candidate separately.

Shared original native handoffs remain with primary: real provider closure for
backup-record-loss and SQL-baseline recipes, plus room seed UID-owner boundary.
The original 12-case backup module's recorded link failure remains unresolved;
this slice does not substitute or weaken it. See
[the exact current64 full-backup handoff](PLAN5_CURRENT64_FULL_BACKUP_MODULE_HANDOFF_2026-10-08.md).

Complete genuine producers/openings/wallet/treasury/player/routes, original
backup/restore/retention/fault/load and inactive behavior acceptance, private
combined native/SQL/recovery execution, keeper/notification/ACK provenance,
activation-owner decision and full Plan5/R1-R8/release remain open. Current public
source is unchanged; inaccessible private source/SHOP refusal fixture feasibility
reports do not provide combined executable qualification. Synthetic fixtures,
inventory coverage and component passes do not close these gates. No production
write, activation, audit autocorrection, deployment, PR merge or primary push
occurred. Wallet-root exclusions and declined inactive spell-path decision remain.
