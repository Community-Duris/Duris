# Plan 5 SQL corpse custody qualification — 2026-10-07

Curator-ready independent reader evidence. This closes the scoped SQL corpse
physical metadata omission. Full Plan 5, R1–R8 and release remain incomplete.
The primary-local notebook is nonblocking; its application, curator acknowledgement,
primary adoption and tested combined release are not claimed.

## Delivery and exact source

- Local/remote branch: `codex/accounting-plan5`; no branch switch in this slice.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Branch base: `0cf5a60840c9d65c98f5ce87dda9132de9767632`.
- Code commit: `6c87078c9dee655149e4d8823534b90486f7ff4f`. The following documentation publication commit and
  exact remote tip are recorded in evidence `delivery/result.json`.
- Refreshed published primary: `02df3a70f4300dc30feb3163f441e9541224cb01`.
- Tested composition tree: `1e31292851410ad1aa616961bc63530bcf2e2e9f`; source archive SHA-256
  `e561bf4e7a640b9e66d83c1b0933c0cc6903d18c2c8774b2020a9c6e8b4986f8`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`; migrations: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
  Both fresh canonical databases ended at `64 / 0064_auction_custody_history`.
  Populated upgrade, rerun and the private reset/0065 candidate remain separate.

During the run, primary advanced to `996ce9ebbb7863eb6b149b254ba111a6c5544e9a` with six
documentation files only. `primary-publication-refresh.json` and its authenticated
patch preserve that comparison. Native/migration trees are identical. The newest
checkpoint still reports private reset84 publication/recovery context with no
compiler/native/SQL/gameplay/recovery execution, and a pending original terminal
service retention contract. It does not alter this exact tested composition or
establish a release gate.

These four owned blobs were overlaid on the refreshed primary. Native bodies,
headers, shared recipes and migration bytes were unchanged.

| Owned source | Tested Git blob |
|---|---|
| `scripts/economic_sql_audit_snapshot.py` | `a0cb4e439fbd690c23473b66e0b629ae9cb357a4` |
| `scripts/reconcile_economy_accounting.py` | `d57bdc49987f70bd51f9e9631f16e1ad60ca6645` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `d90e22f185741d1416a5dfa29436aec830ac319f` |
| `tests/async/test_sql_corpse_custody_audit.py` | `88281147ccea9d41212be815ef479ae7d625212d` |

Unchanged test dependencies in that exact composition:

- `tests/async/test_sql_auction_custody_audit.py`: `56739f00864bb8058e9741871a1c6fa9ebbddedb`.
- `tests/async/test_sql_player_custody_audit.py`: `ceea680583725a36d550a3b273f96450720e89d4`.
- `tests/async/test_sql_shop_custody_audit.py`: `a6693c44cfa8cb673fd2e7f7d2c1d402d3236e79`.

Documentation ownership is this packet and an additive entry in
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. No shared coordinator, accounting contract,
producer, registry, matrix, activation, schema or maintained native recipe changed.
All seven previously consolidated tips and all earlier follow-up entries remain
ancestors on this same branch. No earlier branch work was discarded.

## Established defect and scoped fix

The predecessor could clear current corpse custody while its physical rows were
absent or disagreed. Five authenticated predecessor controls now diagnose wrong
row-ID ownership, unadmitted physical UID, missing physical UID, missing physical
parent and unknown physical literal. Inputs remain unchanged. See
`green03/red-controls.json`; predecessor reconciler SHA-256
`211f1b1caaa0ed0b40a6676afbf80d45fa0f2d8589b31ea33df630b04a0b878d`.

Review also reproduced a global false-clear in the first implementation where
current `vnum=True` matched physical `vnum=1`. `current-vnum-red.json` authenticates
that input and source. The final observer replays the preserved first archive,
proves the false-clear and verifies exact-type refusal at limits 0/1/100 without
changing input. See `green03/current-vnum-controls.json`.

The exporter adds raw unjoined `corpses` and `corpse_items` projections to the
existing RR consistent-snapshot read-only transaction. Aggregate preflight bounds
both together to 100,000 rows before buffered detail queries. Both tables must
exist and be InnoDB; the fence now covers 27 tables. Raw NULLs, orphans, inactive
projections and legacy UIDs stay visible. No player names are exported. Its
`complete=false` and explicit broader coverage gaps remain intact.

The independent reconciler uses native numeric owner identity
`[4,(pid << 32) | save_id,0]`, distinct from `corpses.id`. It verifies signed-width
PID/save/revision/room facts; duplicate numeric identities; physical row-ID
ancestry, UID duplicates, missing/foreign/cyclic parents, unknown identities,
owner/root/parent/vnum/equipment correspondence and reverse live presence.
Tombstones do not acquire current grants. Raw NULL/invalid literals and unsupported
quantity are findings. Ordinary negative weight remains valid; the first four
values must be known and nonnegative, matching the actual physical parser.

The corpse limit is 3,000 physical rows per corpse, with 33-deep forests accepted.
Shop/player limits retain their original defaults. The shared pure resolver has
actual shop, player/pet and corpse consumers; prior suites were rerun. Cached
topology and ancestor-kind scans keep the maximum projection linear. A declared
30-second control with 99,000 rows across 33 corpses at depth 3,000 took
`1.392346888` seconds in the final pinned container, including unchanged
input comparison. This is a reader complexity control, not a release workload.

Money vnum 3 requires unique decoded live literal evidence matching exact current
owner/revision/state and bounded amounts; SQL values must agree. Duplicate,
unknown or mismatched payloads cannot grant authority. Coin sums above int32,
money ancestors and invalid transient ancestry diagnose independently. The
native transient mask is 524288. UID-less coin/transient legacy rows may be
accepted by native loading but remain explicitly unknown to the audit.
Counts are invariant at limits 0/1/100, private aliases do not appear in CLI
output and input bytes are unchanged. The auditor imports no mutation logic.

## Exact commands and results

From the owned worktree:

```text
python -B D:/Dev/Temp/accounting-plan5-sql-corpse/freeze.py green03
python -B D:/Dev/Temp/accounting-plan5-sql-corpse/run.py green03
```

`green03/docker-command.json` records the exact command and mounts. Network none,
read-only root, 2 CPUs/4 GiB, RAM source/private scratch/databases, direct D: bin
bind. Pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
entry `python3 -u -B /evidence/green03/observer.py green03`.

- `test_sql_corpse_custody_audit`: 8 methods passed, zero skips; 68 native cuts,
  34 per fresh canonical MariaDB 10.11.14 and MySQL 8.0.46. The complete native
  method includes actual numeric owner helper, actual coin encoding, accepted
  33-deep and exact-3,000-row wide forests, rejected 3,001 rows and coin-sum overflow,
  malformed literals/topology/identity and unsupported quantity controls.
- `test_sql_player_custody_audit`: 8 methods passed, zero skips, 46 native cuts.
- `test_sql_shop_custody_audit`: 7 methods passed, zero skips, 38 native cuts.
- `test_reconcile_economy_accounting`: 134 loaded, 131 executed, 3 skips.
- `test_economic_sql_audit_origins`: 58 loaded, 55 executed, 3 skips.
- Complete original `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`
  passed separately on both private engines. Its modeled schema subset remains
  separate from canonical native databases. All previous assertions remain.
  Each engine checks missing/MyISAM refusal for five physical-custody tables
  (ten controls, including four for the two new tables). Concurrent corpse and
  physical-row inserts stay absent from the first RR cut and appear with exact
  raw fields/counts in a later cut; cleanup restores the original snapshot.
- Five authenticated predecessor false-clears and the authenticated current-vnum
  alias replay passed.
- Whole final run: `276.569801899` seconds under the unchanged
  900-second budget; `1319` observed commands. Each original
  SQL runner retains its 600-second limit.
- AST and Git diff checks passed. No production C/C++ changed; no additional
  whole-server Make was required for this Python reader slice. Existing sealed
  native833 builds keep their original scope.

Six existing skips, excluded from executed/pass counts:

- `test_reconcile_economy_accounting.AuditBudgetTests.test_near_limit_mapping_snapshot_cli_budget_and_limit_invariance`: requires explicit Linux audit budget invocation.
- `test_reconcile_economy_accounting.AuditBudgetTests.test_near_limit_price_view_cli_budget_and_limit_invariance`: requires explicit Linux audit budget invocation.
- `test_reconcile_economy_accounting.NativeStakeSQLTests.test_native_stake_sql_both_engines`: requires explicit disposable Linux native/SQL stake invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_captured_source_bindings_both_engines`: requires explicit disposable Linux native/SQL integration invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mariadb`: requires explicit disposable Linux native/SQL integration invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mysql_8`: requires explicit disposable Linux native/SQL integration invocation.

The new test oracle includes the actual unchanged
`persistence/corpse_lifecycle_repository.c` body in its test translation unit to
call its private physical parser; GC discards unrelated mutation functions.
It links actual `economic_sql_source_snapshot.c`, `item_transfer_command.c` and
`player_snapshot_codec.c` with original headers. Strict GCC13.3/C++20 flags remain
`-Wall -Wextra -Wpedantic -Werror -O1 -g`, ASan/UBSan, frame pointers, no PIE,
function/data sections, GC, native MySQL flags/libs and `-lcrypto`. No production
body is rewritten and no provider is faked. Full commands: `green03/commands.json`.
GCC SHA-256 `1353e9bdd29a7295c7226bf6c63abccce056d8cac31f112e5cdbecc3f28c2769`;
Python3.12.3 SHA-256
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`.
Corpse oracle binary SHA-256 `1cccae17b4dc81894385721f2616f04d3a8482834cacdbfc1b90fdcdfaec8d0b`.

Actual public native capture corroborates current UID metadata/equipment and
original coin bytes. The private native parser independently corroborates
physical id/parent/UID/vnum/weight/flags/values and admitted root/parent, or its
native error code. It executes SELECT FOR UPDATE only on private synthetic
databases inside a transaction rolled back after inspection; the operational
exporter uses its SELECT-only account and READ ONLY transaction. Full application
table inventories match before/after each cut. Reader UPDATE is denied. All
eight final private DB daemons stop normally, and no game server starts.
Source bodies/modes/link targets authenticate before/after. Native POSIX
attributes are captured before copies, never inferred from Windows copy modes.
Only orphan controls temporarily disable FK checks in the private fixture writer.
Fixture reset detaches parent links before deletion to respect deep FK limits;
canonical schema/FKs and audit sessions remain intact.

The native physical parser does not select `quantity`: NULL and 2 both return
success in these cuts. Its current writer emits 1. The audit labels other values
unsupported and does not claim native rejection. Numeric corpse identity is
source-inspected and its actual packing helper executed, but the complete native
corpse frontend/restore and revision/catalog/name checks are not executed. Full
runtime payload, prototype/template, artifacts and lifecycle history remain open.

| Whole runner log | Exit | SHA-256 |
|---|---:|---|
| `whole-snapshot-mariadb` | 0 | `2dec09b99519af703b044de609d7812db1d7427b26e6699416d0e3604bada498` |
| `whole-snapshot-mysql` | 0 | `53a50281760e6b4b444106510e2de37f389dcdd76b27725067cdfdeb50c60ee1` |

## Evidence, attempts and curator handoff

- Evidence: `D:/Dev/Tests/Duris/accounting-plan5/sql-corpse-20261007/`.
- Cut outputs: `D:/Dev/Builds/Duris/accounting-plan5-sql-corpse-20261007/green03/bin/tests/sql-corpse-custody/`.
- Helpers: `D:/Dev/Temp/accounting-plan5-sql-corpse/`; sealed copies in `helpers/`.
- Native/independent per-engine/case cuts and `evidence.json` remain in builds;
  observations SHA-256 `e04bf6afd4da24313ab774909d296e0cd4452e82d0edc41ff759bcb363f5f1e7`.
- Source archives/manifests, complete commands/logs, terminal states, controls,
  private DB evidence and pre-copy POSIX inventories are retained.
  `seal/evidence.json` authenticates evidence/builds/publication;
  `delivery/result.json` records remote equality and all seven preserved tips.

Preserved previous attempts:

- `green`: exit 1, 57.592581440s. The genuine native oracle compiled and progressed through the MariaDB controls, including the 33-deep forest. Fixture cleanup then hit InnoDB's cascading-delete depth limit. Reset now detaches only private fixture parent links before deleting; the canonical schema/FKs remain unchanged. Review also reproduced a global false-clear from current vnum=True versus physical vnum=1. Exact current-vnum validation and archived pre-fix replay were added. All first-attempt source, individual cuts, logs and normally stopped private DB evidence are preserved.
- `green02`: exit 1, 57.099562871s. All seven pure tests passed and the genuine native oracle reached the 33-deep MariaDB cut again. Cleanup then hit the current-owner parent FK: deleting all child rows at once still includes parents of deeper rows. Reset now detaches private item_current_owner.parent_item_uid links before deletion, alongside corpse row detachment. No production body or canonical schema change; failed source, logs and normally stopped DB remain preserved.

The curator can apply this packet and additive remote follow-up to the primary-local
notebook. Application and acknowledgement are not claimed or required for
continued independent work.

## Narrow shared handoff and remaining gates

No shared change is required to integrate these owned readers. For full public
native capture parity, request raw `corpses.id,value3,save_id,corpse_revision,room_vnum`
and `corpse_items.id,corpse_id,container_id,obj_uid,vnum,quantity,weight,extra_flags,
value0..value3` with exact NULLs and original signed/unsigned widths. No joins may
discard orphan or legacy rows. Consumers are this correspondence test and
independent reconciliation; original RR/read-only fences, aggregate bounds and
immutable source authentication must remain. Both engines must cover duplicate
numeric PID/save identities, row-ID versus owner-ID overlap, missing/foreign/
cyclic parents, NULL/UID-less literals, 3,000/3,001 bounds, nested coins/transients
and late concurrent inserts. This optional shared capture request is not applied.

Quantity needs a separate primary decision if one UID per physical item is a
mandatory native admission invariant: read and validate `corpse_items.quantity`
in `load_physical_items` before granting its UID, with an explicit legacy NULL
policy. Consumers are corpse loading/disposal/retained-plan validation. Native
tests on both engines must distinguish 1 from NULL/0/2 and retain UID, money,
transient, topology and aggregate bounds. No schema or mutation edit is made here.

Full physical/value/UID/origin/writer census, remaining SQL locker/world/siege/
modern-room/collector correspondence, complete prototype/runtime/artifact/history
authority, native EAB2 install/recapture, authenticated retention/erasure/restore
continuity, populated upgrades/reruns, native gameplay/producers/fault/restart/load
batches and the primary's published tested combined candidate remain gates.
The private RSC2/ZRO1/0065 candidate is not a public native-qualified source.
Inventories, synthetic fixtures and component passes do not close release.
Accounting stays inactive, wallet-root exclusions and the declined inactive
spell-path change remain. No activation, merge, deployment, production mutation
or auto-correction occurred. No independent-work impasse is claimed; goal active.
