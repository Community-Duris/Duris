# Plan5 supply view refuses noncommitted and ambiguous roots — 2026-10-06

The independent supply view summed captured system postings without checking
the owning root's outcome. A clean synthetic shop expense reported a3-copper
sink. Changing that root to rejected retained the same supply total even though
the independent audit reported rejected effects. Unknown outcomes and duplicate
root identities likewise produced supply totals. The original view fails the
new regression cases; the fixed view requires exactly one root with a committed
outcome before a system posting contributes to supply.

The fix covers issuance, sink, opening and restitution account kinds7–10. It
does not select a root outcome or reason by export order. It preserves global
audit findings, the CLI's whole-snapshot exit status, bounded row/count behavior,
personal-alias restrictions and immutable inputs. It does not certify damaged
committed postings: their existing audit exceptions still require investigation.
Existing output fields and selected-epoch scope remain unchanged. There is no
new producer, native, schema or shared accounting contract.

## Branch, source and owned files

- Branch/worktree: `codex/accounting-plan5`,
  `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Base: `7c0d6cf6cf475e717aa1e68d207bcb376e939980`; refreshed primary ancestor
  `c00f1868174ea3481ec39b4f533f0063a5b2a658`.
- Result: the separate solved-issue commit containing this report. The delivery
  receipt identifies its SHA, any subsequent normal primary merge and exact
  remote tip, and rechecks preservation of all seven earlier branch heads.
- Native tree `bf7a92a728ad9b5b813626462e56533f8ba39c97`, migrations
  `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`: canonical61. Earlier0055 evidence
  does not qualify this source.
- Owned implementation: `scripts/reconcile_economy_accounting.py`.
- Owned tests: `tests/async/test_reconcile_economy_accounting.py` and
  `tests/async/run_economic_sql_audit_snapshot_mysql.py`.
- Owned documentation: this report and `AUDIT_OPERATIONS.md`.
  Shared coordinator, producers, contracts, registry/matrix, migrations,
  activation and central manifests are not independently edited.

The existing reconciler test owner collects both new methods. The modeled SQL
probe runs inside the existing original native baseline recipe, before its
required qualification marker. All105 integration rows and918 regression entries
remain unchanged. No new shared registration or interface request is required.

Frozen archive SHA256
`7624531199d08cdc961b80404783422307e3d10a52802e77d18f4bf9e74ed3d0`
contains3544 regular source/helper files, no links, credentials, player data or
live source mounts. Every byte is verified before and after execution. This is
component source capture rather than complete world/runtime authority.

| Qualified changed input | SHA256 |
| --- | --- |
| Independent reconciler | `d49ecc0a185e300dab85da9ce956fe9486959977879a145b64d8c0a9de833e4d` |
| Reconciler fixture/tests | `17450c6ff5e3431c3acb63f2b6f5cf6ea252d949e631b68eba3888829bc41c30` |
| Modeled SQL fixture with supply probes | `bf2619a2c937760306e95236f87400807f35e08c10578b8c9a4ba1b526ec1fdb` |

The original reconciler source is frozen separately from this same base. The
negative control runs the two new methods against its original in-memory audit
and view functions, retaining the failing output. It is an original-view unit
control, not a claim that changed-outcome CLI cases ran against the old CLI.

## Commands, runtime and evidence scope

Preparation and frozen Linux execution:

```text
python -B tmp/plan5/prepare-supply-outcome.py
python3 -u -B tmp/plan5/qualify-supply-outcome.py
python3 -u -B tmp/plan5/supply-outcome-original-reader.py
python3 -u -B -m unittest -v test_item_equipment_reconciliation test_economic_sql_uid_scope test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_reconcile_economy_accounting.ReconciliationTests test_reconcile_economy_accounting.AuditBudgetTests test_plan5_child_identity.ChildIdentityTests test_plan5_child_identity.ChildIdentityBudgetTests
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/supply-outcome-current61 DURIS_REGRESSION_BUILD_CACHE=off python3 -u -B tests/async/test_native_sql_baseline_audit.py
```

The image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
Python3.12.3, GCC13.3, MariaDB10.11.14 and MySQL8.0.46. Container
`plan5-supply-outcome-view-01-20261006` has network disabled, no published ports,
2 CPUs and4GiB memory. Workspace, tmp and original parent database use private
RAM mounts. Original child clone paths require ephemeral writable container
storage; neither a read-only root nor RAM-only clones is claimed. No live
database, environment file or world data is consumed.

The negative control exits1 as expected. The fixed Linux suite passes199 methods
in46.919s, zero skips. It retains all original audit-budget and child-budget
assertions; synthetic budget results do not qualify release-host workloads.
The two new methods cover clean shop expense, rejected and unknown outcomes,
identical/conflicting root duplicates in both export orders, all four system
account kinds, limits0/1/100, global findings, aliases and unchanged data. The
clean sink policy fixture is valid before damage. Other system-kind variants
exercise the view rule while retaining any policy findings; they are not valid
native issuance/opening/restitution journeys.

Windows command
`python -B tests/async/test_reconcile_economy_accounting.py ReconciliationTests -v`
passes116 methods in46.394s, zero skips. Normal accounting validation, generated
writer-matrix check and current61 runtime metadata pass. The release command
`python -B scripts/validate_economy_accounting.py --release` exits1 for missing
executable writer evidence; this remains a failing release gate.

The whole original native baseline recipe retains fresh SQL and client-free
C++20 builds, strict warnings as errors, ASan/UBSan, non-PIE/frame-pointer flags,
all15 original fixture/production units and all original guards/cases/deadlines.
SQL/client-free compiles report zero reused objects, respectively41.211s and
37.133s. Their original binary hashes remain
`72d6d5027b5ed550921efd6b11d14b120dacc1b42d53b6ba32406f96b3f54f2c` and
`dd503bd0c1ba66557ff96a9a2004d5007d60525c5c7259bdb5224ced1072ce0d`.
Every literal compiler/tool command and timeout is retained in the native process
receipt. No C/C++ source change or new full-server build is claimed.

The additional SQL probe changes only the existing explicitly modeled disposable
fixture: one captured counterparty becomes a system sink, and its existing root
is observed with committed, rejected and unknown flags. It also tests conflicting
root copies in both export orders without changing database constraints.
These modeled SQL probes do not establish native system-root admission or a
valid gameplay counterparty. Partial-capture and policy findings remain visible.
The complete maintained exporter runs under its real SELECT-only role and always
rolls back/closes the cursor. All application-table data hashes remain unchanged
around each audit and CLI run. Only the private fixture owner installs/restores
damage, returning the complete fixture data to its original hash. No audit
finding is autocorrected.

The complete original native unittest method passes both engines in333.495s,
zero skips. Its execution wrapper takes336.5695610609837s and the outer command
including artifact collection336.66304503311403s, within the unchanged900-second
central limit. Each engine retains161 original corruption/restore refusal cuts,
seven constraint refusals, ten key-hash cuts, both command-binding cuts, two
original native books/epochs and the original cold clone. The prior equipment
coverage remains eight captures, four additional dump/imports, four exact native
replays and24 bounded CLI checks, with all228 application tables unchanged and
the expected slot-drift finding retained through restore.

New supply coverage is six actual SELECT-only export transactions, ten snapshot
probes including four damaged copied exports, and30 SQL CLI invocations. All24
application-table data inventories remain unchanged around each probe; each
capture rolls back and closes its cursor once. The deliberately modeled committed
SQL control retains broken account history, missing original-plan/equipment,
counterparty/mapping and partial-capture findings. Both engines agree on all
phase exception counts. No SQL all-clear, native system admission or gameplay
qualification is claimed. Duplicate-root order can change the existing global
audit findings, but neither order contributes supply, and each input's totals
and findings stay invariant under the detail limit.

The final seal authenticates182 artifacts: exact completed recipe timing,
both-engine native/refusal results, all added SQL captures/probes/CLI results,
preserved equipment cold restores, source/binary/command hashes, raw reports and
terminal container state. It was generated after the whole recipe succeeded;
there is no failed whole-run candidate in this slice. Protected evidence is
`D:\CodexEvidence\accounting-plan5\bin\supply-outcome-view-01-20261006`;
the seal and post-publication receipt are respectively
`tmp/plan5/supply-outcome-evidence.json` and
`tmp/plan5/supply-outcome-delivery.json`. Protected logs, dumps, binaries and
snapshots are not committed. No passing subset replaces the whole recipe.
The evidence seal SHA256 is
`da4a23c4573350c0f8c15cb5a56bf4de067d232e1e661a10b0ba0dec9719d5db`.
The refreshed primary remainsc00f18681 and is already an ancestor; no new primary
merge is necessary. All3081 code/test input hashes match the successful frozen
source. The publication receipt is generated only after the separate commit,
remote push and complete artifact/source/ancestry revalidation.

## Remaining gates and curator handoff

Supply remains selected-epoch captured evidence; this change does not add complete
lineage-wide supply export or certify damaged committed postings. Complete native
holdings/UID/world authority, real producer/player journeys, final combined
backup/restore/retention/corruption/lifecycle and erasure evidence, flatfile
equipment parity, reviewed resumable audit commit-watermark semantics and original
release-host mixed-root measurements remain required. Inventory, modeled fixtures,
metadata and component passing results do not establish full Plan5/R7–R8 or release
completion. No evidence transfers to the primary's private birth candidates.

Accounting stays inactive. Wallet-root exclusions and the declined inactive
spell-path change remain. Only remote `codex/accounting-plan5` is published;
previous branch work and follow-ups remain ancestors. No activation, deployment,
production mutation, PR merge or experimental-accounting push occurs. The primary
maintains the authoritative shared notebook locally and it is nonblocking, per
the user. This owned report and exact seals/delivery receipt supply its curator
handoff. Shared checkpoint/notebook edits remain primary-owned. Independent work
is not blocked; the full qualification goal remains active.
