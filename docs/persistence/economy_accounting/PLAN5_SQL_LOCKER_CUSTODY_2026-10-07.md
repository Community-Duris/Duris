# Plan 5 SQL locker custody qualification — 2026-10-07

Curator-ready independent reader evidence closes the scoped SQL locker physical
metadata omission. Full Plan 5, R1–R8 and release remain incomplete. The primary-local
notebook is nonblocking. Curator acknowledgement, notebook application, primary
adoption and a tested combined release are not claimed.

## Delivery and exact tested source

- Same local/remote branch `codex/accounting-plan5`; no branch switch.
- Worktree `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Branch base `f00c1adba9515a8c2234dac405c1e806f209af5f`; code commit `27a3873f4729dc71f1d3c4dc5962c41de4b3741a`.
  The following documentation publication commit and exact remote tip are in
  evidence `delivery/result.json`.
- Refreshed primary base `996ce9ebbb7863eb6b149b254ba111a6c5544e9a`.
- Tested composition tree `127200f47edd27ac6c26d749e043963610321cdf`; archive SHA-256
  `fae72284d364f5b5c01ce2276b2f81eaa1831c9bae5c1d408119cc0e42a65dc9`.
- Native tree `833d3085815b396861ad18a77635412212381e4b`; migrations `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
  All fresh canonical native databases finish at `64 / 0064_auction_custody_history`.
  Populated upgrade, rerun and private reset/0065 storage remain separate gates.
- Publication refresh `996ce9ebbb7863eb6b149b254ba111a6c5544e9a` preserves those same native
  and migration trees. Its exact documentation-only comparison is retained in
  `primary-publication-refresh.json` and the authenticated patch. The latest read
  checkpoint still treats reset84 publication/recovery and terminal-service
  retention as private implementation, without native/SQL/gameplay qualification.

Four owned blobs overlay that primary. Native bodies, headers, shared maintained
recipes and canonical migration bytes stay unchanged.

| Owned source | Tested Git blob |
|---|---|
| `scripts/economic_sql_audit_snapshot.py` | `bf1cd0ca9f49c761ff6b13f3114a3b90dcd75cca` |
| `scripts/reconcile_economy_accounting.py` | `ffe492e3f9bb110a1e7ce16826292975ff84acfb` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `bbd5a82d781794f1fc7d31eda45d689599f8db71` |
| `tests/async/test_sql_locker_custody_audit.py` | `ada7769056c0e071dfe28b1c89f317899d97ee9d` |

Unchanged test dependencies in the frozen composition:

- `tests/async/test_sql_auction_custody_audit.py`: `56739f00864bb8058e9741871a1c6fa9ebbddedb`.
- `tests/async/test_sql_corpse_custody_audit.py`: `88281147ccea9d41212be815ef479ae7d625212d`.
- `tests/async/test_sql_player_custody_audit.py`: `ceea680583725a36d550a3b273f96450720e89d4`.
- `tests/async/test_sql_shop_custody_audit.py`: `a6693c44cfa8cb673fd2e7f7d2c1d402d3236e79`.

Documentation ownership is this packet and an additive entry in
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. No shared coordinator, accounting contract,
producer, registry, matrix, activation, schema or maintained native recipe changed.
All seven earlier consolidated tips and all prior follow-up entries remain on this
same branch. Earlier branch work was preserved.

## Established defect and complete reader fix

The predecessor can clear current locker custody when physical rows are absent,
unadmitted or disagree. Six authenticated before/after controls establish wrong
locker/chest identity, missing physical row, unadmitted physical UID, missing
physical parent, unknown physical literal and NULL legacy chest. See
`green02/red-controls.json`; predecessor reconciler SHA-256
`180b19f7ea6d52808a7d964b69f4e1199fa28d26a013eb92b694eab11038fcd2`. Counts stay invariant at limits 0/1/100; inputs
remain unchanged.

Review also reproduced a global false-clear in the first frozen candidate where
`lockers.racewar=NULL` was silently accepted. `racewar-red.json` preserves the
input and authenticated source. The final observer replays the unchanged source
from the green archive, proves its false-clear and verifies the exact unknown-side
diagnostic at limits 0/1/100. See `green02/racewar-controls.json`. Both locker
namespaces now diagnose NULL side without inventing a replacement.

The exporter reads six raw projections in the original RR consistent-snapshot
READ ONLY transaction: `lockers`, `private_chests`, `locker_items`,
`account_lockers`, `locker_chests`, `account_locker_items`. Aggregate preflight
bounds their combined rows to 100,000 before buffered detail queries. All six
must exist and be InnoDB, bringing the engine fence to 33 tables. No join or filter
discards orphans, NULLs or retained rows. Names, account aliases, chest keywords,
password hashes, sort settings and item text are not exported. `complete=false`
and broader explicit gaps remain.

Active locker identity is `[5,locker_id,chest_id]`, as used by native transfer and
hydration. The retained account baseline uses `[5,chest_id,0]` in
`baseline_item_ownership.sh` and `classify_item_topology.py`. Equal numeric IDs in
these namespaces are never merged. The audit compares historical account metadata
under that historical identity and always reports
`locker_account_runtime_authority_unqualified` when account physical rows exist;
those rows cannot establish qualified runtime authority.

Raw NULL/zero legacy chest IDs remain unknown and cannot substitute for a durable
positive public-chest context. Locker/chest existence and duplicates, exactly one
public chest per catalog locker, public-policy unknowns, owner PID/association
ambiguity and side unknowns are diagnosed. This checks numeric metadata, not access
authorization or alias-derived ownership. Access/history/erasure completeness stays open.

Physical row-ID ancestry is scoped by both locker and chest, independently for each
namespace. The audit detects duplicates within/across physical namespaces,
missing/foreign/cyclic parents, unknown UIDs, unadmitted UID, inactive projection,
owner/root/parent/vnum/equipment disagreement and missing live physical rows in
reverse. Historical/inactive rows never become current grants. Quantity NULL or
other than 1 is unsupported; NULL weight/flags/type remains unresolved prototype
data. Ordinary nullable values remain prototype fallback, not guessed coin authority.

Money vnum 3 or raw ITEM_MONEY 20 validates known nonnegative first-four values,
even for UID-less or unadmitted physical rows. Unique decoded live coin payloads
must match exact current owner/revision/state and physical amounts. Unknown,
duplicate, aliased and mismatched payloads do not grant evidence. Counts and CLI
input bytes stay unchanged across limits; private aliases never appear in output.
No operational audit imports mutation or correction code.

The loader-source recursion bound admits row depths 0 through 64: the independent
reader permits 65 path rows and diagnoses 66. This is separate from a selected
active transfer's 3,000-item/codec bounds; no invented whole-chest count cap is
applied. A declared 30-second complexity control with 99,900 rows in reversed
65-deep forests took `1.507212639` seconds, including unchanged input
comparison. This is a reader complexity control, not a release workload.

## Exact commands and results

From the owned worktree:

```text
python -B D:/Dev/Temp/accounting-plan5-sql-locker/freeze.py green02
python -B D:/Dev/Temp/accounting-plan5-sql-locker/run.py green02
```

Full container/mount command is `green02/docker-command.json`: network none,
read-only root, 2 CPUs/4 GiB, RAM source/private scratch/databases, direct D: bin.
Pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
entry `python3 -u -B /evidence/green02/observer.py green02`.

- `test_sql_locker_custody_audit`: 8 methods passed, zero skips, with 66 raw
  SQL/current-native-capture cuts, 33 each on MariaDB 10.11.14 and MySQL 8.0.46.
  Both locker schemas use canonical fresh databases, overlapping IDs and distinct
  owner tuples. Cases include orphan/duplicate/legacy/stale rows, money, exact
  source-derived depth bounds and unknown metadata.
- `test_sql_corpse_custody_audit`: 8 passed, zero skips, 68 native cuts.
- `test_sql_player_custody_audit`: 8 passed, zero skips, 46 native cuts.
- `test_sql_shop_custody_audit`: 7 passed, zero skips, 38 native cuts.
- `test_reconcile_economy_accounting`: 134 loaded, 131 executed, 3 existing skips.
- `test_economic_sql_audit_origins`: 58 loaded, 55 executed, 3 existing skips.
- Complete original `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`
  passed separately on both private engines. Its modeled subset remains separate
  from canonical native databases; all previous assertions remain. Each engine
  refuses missing/MyISAM sources for eleven physical-custody tables (22 controls,
  including twelve new controls for these six tables). Concurrent inserts in all
  six new projections are absent from the first cut and present with exact raw
  fields and counts in the later cut; cleanup restores the original snapshot.
- Six authenticated predecessor false-clears and the archived NULL-side
  false-clear/diagnosis replay passed.
- Final complete batch `339.160112484` seconds under unchanged
  900-second whole-run budget, `1536` observed commands;
  original SQL runners retain their individual 600-second limits.
- AST and Git diff checks passed. No production C/C++ changed; a new whole-server
  Make is not required for this Python reader slice. Prior native833 builds retain
  their recorded qualification scope.

Six existing skips, excluded from executed/pass counts:

- `test_reconcile_economy_accounting.AuditBudgetTests.test_near_limit_mapping_snapshot_cli_budget_and_limit_invariance`: requires explicit Linux audit budget invocation.
- `test_reconcile_economy_accounting.AuditBudgetTests.test_near_limit_price_view_cli_budget_and_limit_invariance`: requires explicit Linux audit budget invocation.
- `test_reconcile_economy_accounting.NativeStakeSQLTests.test_native_stake_sql_both_engines`: requires explicit disposable Linux native/SQL stake invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_captured_source_bindings_both_engines`: requires explicit disposable Linux native/SQL integration invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mariadb`: requires explicit disposable Linux native/SQL integration invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mysql_8`: requires explicit disposable Linux native/SQL integration invocation.

The new oracle compiles actual unchanged `economic_sql_source_snapshot.c` and
`player_snapshot_codec.c` with original headers, GCC13.3/C++20,
`-Wall -Wextra -Wpedantic -Werror -O1 -g`, ASan/UBSan, frame pointers, no PIE,
function/data sections, GC, native MySQL flags/libs and `-lcrypto`. There are no
fake providers or rewritten production bodies. Commands: `green02/commands.json`.
GCC SHA-256 `1353e9bdd29a7295c7226bf6c63abccce056d8cac31f112e5cdbecc3f28c2769`;
Python3.12.3 SHA-256
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`.
Locker oracle binary SHA-256 `916debf2ffe283258d95f62a63ff2408ee66767f88c5799bc2ea382e6f36a550`.

The public native capture corroborates current UID root/parent/owner/revision/
vnum/state/equipment and retains actual encoded coin bytes. It has no raw locker
catalog/chest/physical projections: those fields, including quantity/type/values
and both identity mappings, have explicit canonical SQL fixture and source-inspection
evidence, not full native physical capture parity. The complete native locker
hydrator, access frontend, gameplay transfer and runtime forest restoration are
not executed by this new test. Source-derived 65/66 controls do not prove actual
native gameplay hydration. Full prototype/runtime payload, artifacts and historical
source admission remain gates. Native hydration ignores quantity while its writer
emits 1; the audit's unsupported-quantity diagnosis does not claim native rejection.

Reader UPDATE is denied. Complete application-table inventories match before/after
every cut. All ten final actual private DB servers stop normally, excluding
version/init processes from that count; no game server starts. Source bytes,
modes and link targets authenticate before/after. POSIX inventories are captured
before copying, never inferred from Windows copy modes. Only deliberate orphan
controls temporarily disable FK checks in the private writer. Fixture reset
detaches only private parent links before deletion; canonical FKs/schema stay intact.

| Whole runner log | Exit | SHA-256 |
|---|---:|---|
| `whole-snapshot-mariadb` | 0 | `b29425f1445b93e48e41a07a3f1cb3d43dcb99c1e0b72d4886d46f26d4491494` |
| `whole-snapshot-mysql` | 0 | `e5905d17b3e7711c2777229c7f6bbb4c302d62c1ae6474f5bbfb0b3e4e8352de` |

## Evidence and curator handoff

- Evidence `D:/Dev/Tests/Duris/accounting-plan5/sql-locker-20261007/`.
- Cut outputs `D:/Dev/Builds/Duris/accounting-plan5-sql-locker-20261007/green02/bin/tests/sql-locker-custody/`.
- Helpers `D:/Dev/Temp/accounting-plan5-sql-locker/`, sealed copies in `helpers/`.
- Observation `evidence.json` SHA-256 `613a1269f4a66691cc024819618a5b212f2617ebdd6e0b89ec10c5fb2c014876`.
- All source archives/manifests, commands, complete logs, terminal/control records,
  private DB evidence and original POSIX inventories are preserved.
  `seal/evidence.json` authenticates evidence/builds/publication;
  `delivery/result.json` verifies remote equality and the seven earlier tips.

Preserved preceding attempts:

- `green`: exit 0, 333.790614871s. Complete original first-candidate batch passed all six modules, 64 locker cuts, prior native suites, six authenticated predecessor controls and both original SQL runners. It is superseded: review independently established a global false-clear when lockers.racewar was NULL. The original source/input are authenticated in racewar-red.json. Final code adds explicit unknown-side diagnostics in both namespaces, pure/global controls and a new canonical both-engine case; the final observer replays the first archived body and proves the repair. No budget or compiler relaxation; all first-run source, outputs, commands, normally stopped private databases and native attribute inventories are preserved.

The curator can apply this packet and the additive follow-up to the primary-local
notebook. Application/acknowledgement/adoption is not claimed or needed to keep working.

## Narrow shared requests and remaining gates

Central qualification registration remains primary-owned. The new owner is
`tests/async/test_sql_locker_custody_audit.py`: seven pure methods in
`LockerCustodyAuditTests` and one opt-in method
`NativeLockerCustodyAuditTests.test_both_canonical_engines_actual_current_capture_and_raw_locker_cuts`.
Pure command: `python3 -B tests/async/test_sql_locker_custody_audit.py LockerCustodyAuditTests`.
Complete native command: `DURIS_RUN_SQL_LOCKER_AUDIT=1 python3 -B tests/async/test_sql_locker_custody_audit.py`.
The central inventory/runner must preserve all existing owners and policies,
original compiler/providers, both canonical engines and the declared budgets;
the owned observer already executes the complete new module with zero skips.
No shared registry, matrix or maintained runner is edited here.

No shared contract change is needed to integrate these owned readers. For public native
capture parity, request raw numeric `lockers.id,racewar,owner_pid,owner_assoc_id`,
`private_chests.id,locker_id,is_public`, `locker_items.id,locker_id,chest_id,
container_id,obj_uid,vnum,quantity,weight,extra_flags,item_type,value0..value3`,
`account_lockers.id,racewar`, `locker_chests.id,locker_id,is_public` and
`account_locker_items.id,chest_id,container_id,obj_uid,vnum,quantity,weight,
extra_flags,value0..value3`. Preserve exact NULLs/widths and namespace/table identity;
no joins or alias fields. Consumers are native correspondence, independent audit,
backup/restore source authentication. Preserve original RR/read-only fences and
cumulative bounds. Both-engine tests must include overlapping IDs, NULL public
chests, missing/foreign/cyclic parents, UID duplicates across schemas, erased
aliases, source-derived bounds, coins and late inserts.

Historical account runtime authority needs an explicit primary contract before
qualification. Exact fields are current `owner_type,owner_id,owner_context_id`,
account physical `chest_id`, `locker_chests.locker_id` and `account_lockers.id`.
Invariant: the old `[5,chest_id,0]` baseline cannot alias active
`[5,locker_id,chest_id]` or silently become current replay authority. Consumers
are native hydration/transfers, baseline/restore authentication and independent
reconciliation. Required tests bind overlapping IDs and original legacy evidence,
refuse ambiguity/orphans, preserve erasure-safe numeric identity and authenticate
any explicit cutover. This reader keeps it unqualified and changes no shared schema.

If one UID per physical item is a mandatory native admission rule, the primary
must separately decide exact `locker_items.quantity` handling in both loader paths,
including the legacy NULL policy. Tests distinguish 1 from NULL/0/2 with original
UID/owner/ancestry/money behavior. No mutation or native policy change is applied here.

Remaining gates include complete physical/value/UID/origin/writer census; SQL
world/siege/modern-room/collector correspondence; complete payload/prototype/
artifact/history authority; native EAB2 install/recapture; authenticated full
retention/erasure/restore continuity; populated upgrades/reruns; original gameplay,
producer, fault/restart/load batches; and primary's published tested combined
candidate. Inventory, synthetic fixtures and component passes do not close release.
Accounting remains inactive; wallet-root exclusions and declined inactive spell
behavior remain. No activation, merge, deployment, production mutation or
auto-correction occurred. No independent-work impasse; goal remains active.
