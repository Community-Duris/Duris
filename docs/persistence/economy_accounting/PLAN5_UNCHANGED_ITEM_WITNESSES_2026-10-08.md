# Plan 5: unchanged original item witnesses - 2026-10-08

The independent reader now compares every initial EAP1 item witness with known
captured opening and prior-history positions at its UID/revision. Previously a
plan could carry a contradictory unchanged container witness without an item
event and still audit clean. The existing `original_plan_preimage_mismatch`
finding now covers that witness and leaves the operation unverified. All prior
event before-position checks remain. Full Plan 5/R1-R8/release remain unqualified.

## Delivery and ownership

- Local/remote branch `codex/accounting-plan5`; no branch switch.
- Worktree `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base `9d64fd0942f98a792f08ba88c0bd9139410ed8e1`.
- Result is this containing commit; exact SHA/remote equality/clean worktree and
  ancestor preservation are verified in `D:/Dev/Tests/Duris/accounting-plan5/unchanged-witnesses-20261008/delivery/result.json`.
- Production change is six changed lines in existing
  `Reconciler.audit_original_plans`, in `scripts/reconcile_economy_accounting.py`.
  The original capture index now retains valid positions beyond event-reference
  UIDs. Initial witness positions join all existing event before-positions.
  Strict position grammar/typed comparisons/exceptions/verified counters remain.
- Owned regression module adds three methods and two explicit model helpers.
  The unchanged-witness encoder is reused by the existing SQL runner; it authors
  test capsules only and is never called by the reader or exporter.
- Existing `verify_collector_quarantine_views` in the SQL runner gains a sixth
  damage and a matching saved-capture control. All old probes/guards remain.
- Owned docs are this report, `AUDIT_OPERATIONS.md` and additive remote follow-up.
  AST binding proves all other methods/functions unchanged and all 27 overlay
  blobs equal to the tested composition.

No shared interface/schema/native change is requested. Existing before forests,
opening/selected/lineage positions, UID/revision and equipment fields suffice.
No mutation implementation is imported. Missing historical positions remain
unknown under existing findings; a verified capsule counter is not complete
opening or release qualification. All candidates at a known UID/revision are
compared, preserving unknown optional equipment and creation's logical opening.
Indexing is linear in captured rows, then in witnesses and event preimages; no
per-plan full-history scan is added. Native codec rules require unchanged witness
positions to agree between its initial/final forest, so the initial comparison
also constrains its final unchanged position.

Primary owns shared registration and candidate integration. Retain these three
new methods in the existing reconciliation class:

- `ReconciliationTests.test_original_item_plan_unchanged_witness_binds_opening_and_history`
- `ReconciliationTests.test_original_item_plan_unchanged_witness_preserves_unknown_history`
- `ReconciliationTests.test_original_item_plan_unchanged_witness_cli_is_global_and_read_only`

## Exact source and established defect

Primary refreshed before choosing base `3ef54e021d22f931de1d5255dde02fffbaf7f310`. Each freeze composes
that exact published tree with 27 owned overlays, retaining previous backup,
tombstone, endpoint/history and event-preimage fixes. Native tree
`833d3085815b396861ad18a77635412212381e4b`; migration tree `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`;
canonical head `0064_auction_custody_history`. Historical native files on the
owned branch are not substituted for primary native source. Required raw
AGENTS/README/finish/requirements/Plan 5/latest checkpoint/private cold-source
note are retained; published `AI_CONTEXT.md` is absent. Qualification refresh
primary `59d610f16bd4d19c24fda313d9e86ac17b689844` has only documentation changes, if any; exact
paths/raw documents/patch are in qualification evidence. Delivery refresh is
receipt-authoritative.

| Freeze | Git tree | Archive SHA256 |
| --- | --- | --- |
| 00 | `1fcd0b320547818f317f84bd57de88c4587a644d` | `4beea6441f4cd7f5593f20c5e7ddb5968ab291e2e61222eea617035007c721d4` |
| 01 | `7fa60aa23de07e21d929b4012ec2b837dd2990e8` | `e31b48562d7102a81906bf76a822519e447a7e013012b6528dc4b487e5d4620c` |
| 02 | `66daf1c32d457df9758d0a4e04a1f2eabdc7b492` | `1ed9d0c970706d62f3997f4e1a37cc2842716bc22d9e1f4b965dce96fa0777a7` |

All three archives authenticate 6,514 Git blobs: 6,510 regular files/four exact
link targets, including body/mode/link transport. Final reader blob
`8e92bce83ea976755e1bc13648faaa202508a6b3`, SHA256
`39cf9fdbdf3826c7eada39ad63332d0bdc1e781543bb6c343517dcc316aa9e06`.

Freeze00 establishes four healthy and four false-clean damaged capsules: live
versus quarantined state in both directions, using either a matching opening
or a preceding retained history position. UID81 moves out of container82; both
UIDs occur in complete native before/after forests, but only81 has an event or
accounting reference. Only unchanged witness82's state in both capsule forests
and its matching digest differ; captured opening/ledger/lineage/native rows stay
exact. The old reader reports no exception and verified1 for all eight cases.
All eight independently decode and exactly round-trip SQL/flatfile native codecs:
16 executions. This establishes valid but contradictory model evidence, not a
real producer journey.

Freeze01 executes the three new methods on the old reader: 28 failed subtests,
zero errors/skips, expected exit1. Freeze02 passes the same three methods, then
runs all component/SQL/native checks below on the exact same source. No earlier
run is relabeled. Matching controls stay clean; four damaged cases now produce
exactly one existing preimage finding and verified0. Unknown container origin
retains its `unknown_legacy_origin` finding without inventing a historical match.
The 24 new CLI calls cover exceptions/provenance, both history sources and state
directions at limits0/1/100, with immutable input and excluded personal aliases.

## Executed checks and backends

Tools image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
GCC13.3/Python3.12.3/nm2.42/mysql_config10.11.14. Exact executable bodies/versions
are retained in native inventory. D evidence/build roots are directly mounted;
network none/no ports, fresh private database directories and no existing service
or volume target. Canonical runner uses its required fresh container layer on
existing D Docker storage. Original recipes/C++20/ASan/UBSan flags and declared
deadlines remain unchanged.

Each stage entry is
`python3 -u -B /evidence/<stage>/observer.py <stage> <source-label>`.
Full host Docker argv/mounts/image/limits are in its `docker-command.json`;
commands/deadlines/exit/timing and launches in `commands.json`/`launches.json`.
Every unit result/skip and nested SQL CLI invocation remains recorded.

| Stage | Exact source tree | Exit | Seconds |
| --- | --- | ---: | ---: |
| canonical02 | `66daf1c32d457df9758d0a4e04a1f2eabdc7b492` | 0 | 292.914475 |
| checks02 | `66daf1c32d457df9758d0a4e04a1f2eabdc7b492` | 0 | 239.011413 |
| codec02 | `66daf1c32d457df9758d0a4e04a1f2eabdc7b492` | 0 | 11.063264 |
| focused02 | `66daf1c32d457df9758d0a4e04a1f2eabdc7b492` | 0 | 3.336919 |
| red01 | `7fa60aa23de07e21d929b4012ec2b837dd2990e8` | 1 | 3.218384 |
| reproduce00 | `1fcd0b320547818f317f84bd57de88c4587a644d` | 0 | 1.584565 |

Checks02 executes 16 original component modules: 415 loaded / 397 PASS / 18 existing
opt-in skips, zero errors/failures. Original near-limit budget methods execute
with `DURIS_RUN_AUDIT_BUDGET=1`. The complete original SQL snapshot runner,
`python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`, passes on
MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL8.0.46-0ubuntu0.22.04.4 using
isolated disposable socket/schema settings and SELECT-only audit credentials.

Its manual 47-table fixture remains partial. Existing healthy model capsules
are temporarily stored/recaptured as before; the new unchanged-witness pair is
authored only into saved captures. A real captured opening for UID82 supplies the
matching control; its alternate quarantined state supplies the contradiction.
Both new roots preserve the original UID84 event and all captured authority;
root witness counts/digest reflect the explicit added unchanged witness. The
matching control retains existing partial findings; damage adds only the existing
preimage mismatch. These are not claims of a capsule produced or recaptured by
native gameplay. Per engine: two phases, six damages each, 12 probes/72 actual
CLI calls. All 47 application tables/input objects/files stay unchanged during
reads; original fixture metadata is restored. `sql-cli-command-records.json`
retains 144 nested argv/results recorded by the executed SQL runner, distinguished
from parent-observer raw commands.

The original native canonical method
`RestoreCoinEffectsTests.test_native_coin_effects_both_modes_and_canonical_engines`
executes with `DURIS_RUN_RESTORE_COIN_INTEGRATION=1` and
`DURIS_PLAN5_CANONICAL_EVIDENCE=1`: 1 PASS/zero skips. Native decoder modes agree
on 3,026 cases/1,054 accepted. Each fresh 0064 engine completes 109 restore cuts,
90 expected refusals and 58 full-entry cuts with unchanged authority, preserving
original pending-claim/intent/history/boundary controls. Parent observer records
18 successful g++ driver calls: two fixture compile/link calls and 16 toolchain
queries, not 18 compiler jobs. Nested compile totals are not inferred.

Codec02 executes 48 exact native round trips: eight original model capsules and
16 saved SQL capsules, each in SQL/flatfile. The SQL set includes four original
healthy, four old wrong-before, four new unchanged-witness healthy and four new
unchanged-witness damaged capsules. Native probes are authenticated reuse from
the zero-net slice: source bodies/modes, 14 genuine providers, original strict
flags and binary hashes match. No fresh probe or whole-server build/gameplay
boot is claimed. Native/shared files are unchanged; Windows and the original
full backup module are not repeated here.

The same final source additionally executes six CLI measurements using the
existing `measure_audit_cli` helper and original 30s/256MiB limits: 100,000 captured
opening positions, 33,552,384-byte input including permitted trailing whitespace,
matching/damaged witness and limits0/1/100. All pass with input unchanged and
exact global counts: 99,998 missing native items stay visible, plus the damaged
witness finding when applicable. This deliberately incomplete synthetic capture
exercises the enlarged index; it is not a full capture or release-host workload.
Measured seconds/peak bytes and input hashes are in `codec02/budget-results.json`.

## Evidence and remaining gates

Evidence `D:/Dev/Tests/Duris/accounting-plan5/unchanged-witnesses-20261008`;
helpers `D:/Dev/Temp/accounting-plan5-unchanged-witnesses`;
builds `D:/Dev/Builds/Duris/accounting-plan5-unchanged-witnesses-20261008`.
Raw seal SHA256 `6b8d77048899f240d3d0cfcfba73f8f305d6cbae31373b60513a810b13a91f7a`: 1721 files/1030153869 bytes,
including 1356 regular build-root files, not compiler jobs.
All seven task containers are stopped/no OOM/live recorded children. The expected
old-reader failure remains exit1. Native lstat/body inventory precedes regular-only
copying and extended-path Windows rehashing; no copied links or reparse points.
Reports/post-push receipts are additional post-seal artifacts.

All seven earlier tips and subsequent Plan 5 fixes remain on this remote branch.
Prior tombstone c373f04fa/restore blob 23b17a3d158ac455105de1cd1303561dcc028ad8 stays
included in 27 overlays; published primary still has restore blob e78cef8d3dbd4e48c507d49e443b03fa5208f86f.
Primary should retain/import that solved fix during backup integration. Older
24-overlay reports keep their original scopes. Primary's latest private cold
candidate a753f4b6 retains a smaller reader composition and execution remains
deferred; this peer qualification does not transfer to it. Later 99c/9dd/9d64/
this-slice import, combined qualification and curator acknowledgement are unclaimed.

Primary-owned original flatfile-authority fixture's seven genuine-provider link
symbols/nine missing-record controls/full12-method backup qualification, room-seed
UID owner link and original baseline recipe/head closure remain open and are not
rerun here. Complete openings, real producer/cold-world/wallet lifetimes, native
collector/intent/command/publication, admission/recovery/ACK, registry/matrix,
player/routes/fault/load, published tested combined candidate and activation-owner
qualification remain open. Full Plan 5/R1-R8/release remain unqualified; inventory,
models and isolated passing suites do not prove those journeys.

Report/operator note/additive follow-up/receipts are curator-ready input for the
primary-local nonblocking notebook; application/ack and cross-chat messaging are
unclaimed. Inactive behavior, wallet-root ITEM_MONEY exclusions and declined
inactive spell path remain. No activation/autocorrection/production access or
write/primary push/deployment/merge occurs.
