# Plan 5 SQL player and pet custody qualification — 2026-10-07

Curator-ready reader evidence. This closes the independent SQL player/pet
physical metadata omission. Plan 5, full R1–R8 and release remain incomplete.
The primary-local notebook is nonblocking. No notebook application, curator
acknowledgement, primary adoption or tested combined release is claimed.

## Delivery and exact source

- Local/remote branch: `codex/accounting-plan5`; no branch switch in this slice.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Branch base: `bc4ba06496f80fd28a487d12aa4addda6924f159`.
- Code commit: `67bb326177608d14f23e91bc2076151e771aaf02`. The following documentation publication commit and
  exact remote tip are recorded in `delivery/result.json` under the evidence root.
- Refreshed published primary: `02df3a70f4300dc30feb3163f441e9541224cb01`.
- Tested composition tree: `4a35cfe112dd2d417ed373a3d1bf52419124251c`; source archive SHA-256
  `fa43e1105c57ccb480552c570974d20c409bbf2a843a51c0323e1e9aff3b43cc`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`; migrations: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
  Fresh canonical databases on both engines ended at
  `64 / 0064_auction_custody_history`. Populated upgrade is a separate open gate.

The frozen composition overlays these four owned blobs on the refreshed primary.
It also includes unchanged test dependencies: auction emitter blob
`56739f00864bb8058e9741871a1c6fa9ebbddedb` and shop test
blob `a6693c44cfa8cb673fd2e7f7d2c1d402d3236e79`.
Native bodies/headers, shared provider recipes and migration bytes are unchanged.

| Owned source | Tested Git blob |
|---|---|
| `scripts/economic_sql_audit_snapshot.py` | `2d39fd77da61e9308ed303f63a1460d177d9e29d` |
| `scripts/reconcile_economy_accounting.py` | `3f0c28542e433cbeab0821e31a1a7d5740e11060` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `587cc8fd13f09eb646a2d4a51e4fbecc7ea623f5` |
| `tests/async/test_sql_player_custody_audit.py` | `ceea680583725a36d550a3b273f96450720e89d4` |

Documentation ownership is this packet and an additive entry in
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. No coordinator, accounting contract,
producer, registry, matrix, activation or native recipe file changed. All seven
previously consolidated branch tips and their existing follow-ups stay on this
same branch; no earlier work was moved or discarded.

## Established defect and complete scoped fix

The predecessor could report no exceptions while physical player/pet rows
disagreed with current custody, or were missing altogether. Four authenticated
predecessor controls now produce findings: wrong player equipment, unadmitted
physical player UID, missing physical player UID and stale pet player context.
Inputs remain unchanged. See `green03/red-controls.json`; predecessor reconciler
SHA-256 `772a9337e45020a3d4c3ae6aa7d8487a07ad17cbd5c67b4b366a3f236f750ed0`.

Review also established a global false-clear in the first new implementation:
boolean owner and floating revision aliases compared equal to current integers
and excused an absent player row. `inline-alias-red.json` retains the input and
authenticated pre-fix source hash. The final observer replays that source from
the preserved green02 archive, proves its false-clear, and verifies refusal of
the unchanged input at limits 0/1/100. See `green03/inline-alias-controls.json`.
Inline ownership and revision now require exact native integer representations.

The exporter captures every player ID, pet mapping and physical player/pet row
inside its existing repeatable-read consistent-snapshot read-only transaction.
There are no ownership joins or filters that would hide orphans, stale projections
or legacy NULL identities. Aggregate preflight runs before detail reads and
limits these four projections together to 100,000 rows. The three added physical
tables must be present and InnoDB. Raw NULL fields are preserved; pet quantity
is native constant 1. Empty query results remain lists.

Additive diagnostic fields are `native.player_ids` (`pid`), `player_pets`
(`pet_id,pid,pet_uid`), `player_items` (`item_id,pid,parent_id,uid,vnum,
equipment_slot,quantity,item_type,value0..value3`), `pet_items` (same with
`pet_id` replacing `pid`) and exact `player_custody_coverage`
(`players,pets,items,pet_items`). Missing coverage in old SQL partial cuts is an
explicit finding. The existing 32 MiB encoded-input and output bounds remain.

The independent reconciler validates represented integers/NULLs before indexing.
Physical ancestry uses row IDs in separate player and pet table namespaces;
UID duplicate detection spans both. It checks missing/duplicate player/pet/row
identities, stable pet UID ambiguity, orphan/foreign/ambiguous parents, cycles,
32-row depth, 4096 aggregate player-plus-pet items, 64 pets, quantity exactly 1,
valid unique root equipment positions, positive physical vnums, legacy unknown
UIDs, current admission, owner/state/vnum/placement/equipment agreement and
reverse coverage of live player/pet UIDs.

Expected direct player ownership is `[1,pid,0]`. Pet rows accept the same
player's legacy owner even when a stable pet UID exists, or `[11,pet_uid,pid]`.
Inactive and foreign projections produce diagnostics and never grant current
custody. Tombstones are exempt from reverse live-row requirements. Root
ITEM_MONEY is excluded before UID/vnum/literal validation; excluded wallet roots
cannot supply ancestry for an item forest. Nested money retains NULL/negative/
payload-disagreement findings. No history or death payload becomes a current grant.

Only unique, decoded live inline coin evidence with exact matching owner/revision
and bounded amounts can excuse an absent player physical row, matching native
login reconstruction. Unknown, missing, duplicate or mismatched evidence cannot.
Pet inline coins still require physical rows. The shared pure row-ID resolver
has two actual consumers, shop and player/pet; the prior shop suite was rerun.
Findings expose numeric IDs; global counts are invariant at limits 0/1/100.
CLI input bytes and unrelated aliases are preserved. No mutation code is used.
The exporter remains `complete=false` with explicit broader coverage gaps.

## Exact commands and results

From the worktree:

```text
python -B D:/Dev/Temp/accounting-plan5-sql-player/freeze.py green03
python -B D:/Dev/Temp/accounting-plan5-sql-player/run.py green03
```

`green03/docker-command.json` records the full command and mounts. The container
uses network none, read-only root, 2 CPUs/4 GiB, RAM source/scratch/private DBs
and a direct D: bin mount. Pinned image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Entry point: `python3 -u -B /evidence/green03/observer.py green03`.

- `test_sql_player_custody_audit`: 8 methods passed, zero skips. The complete
  native method made 46 cuts, 23 per fresh canonical engine, including actual
  native coin encoding and the missing-player-physical reconstruction control.
- `test_sql_shop_custody_audit`: 7 methods passed, zero skips, including 38
  native cuts on both canonical engines after the ancestry resolver change.
- `test_reconcile_economy_accounting`: 134 loaded, 131 executed, 3 skips.
- `test_economic_sql_audit_origins`: 58 loaded, 55 executed, 3 skips.
- Full original `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`
  passed separately on private MariaDB 10.11.14 and MySQL 8.0.46. Its modeled
  schema subset is separate from the canonical native databases. Every prior
  assertion remains, with explicit missing-physical counts for deliberately
  incomplete fixtures. Six new missing/MyISAM table controls refuse capture
  per engine. Concurrent inserts into pet mappings and both physical tables
  are absent from the first RR cut and present in a later cut; cleanup restores
  the exact original snapshot.
- Four authenticated predecessor false-clear controls passed.
- Complete measured run: `186.311157286` seconds under the unchanged
  900-second whole-run budget; `1027` observed commands.
- AST and Git diff checks passed. No production C/C++ file changed, so this
  Python reader slice did not require a new whole-server Make build. Prior
  sealed native833 builds retain their original exact qualification scope.

The six existing skips, excluded from pass counts:

- `test_reconcile_economy_accounting.AuditBudgetTests.test_near_limit_mapping_snapshot_cli_budget_and_limit_invariance`: requires explicit Linux audit budget invocation.
- `test_reconcile_economy_accounting.AuditBudgetTests.test_near_limit_price_view_cli_budget_and_limit_invariance`: requires explicit Linux audit budget invocation.
- `test_reconcile_economy_accounting.NativeStakeSQLTests.test_native_stake_sql_both_engines`: requires explicit disposable Linux native/SQL stake invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_captured_source_bindings_both_engines`: requires explicit disposable Linux native/SQL integration invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mariadb`: requires explicit disposable Linux native/SQL integration invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mysql_8`: requires explicit disposable Linux native/SQL integration invocation.

Native test oracles compile actual `economic_sql_source_snapshot.c` and, for the
player test, actual `player_snapshot_codec.c`, with original headers. Commands
retain GCC13.3/C++20, `-Wall -Wextra -Wpedantic -Werror -O1 -g`, ASan/UBSan,
`-fno-omit-frame-pointer -fno-pie -no-pie`, function/data sections and gc, native
MySQL flags/libs and `-lcrypto`. Complete invocations are in `green03/commands.json`.
There are no fake providers or edited production bodies. GCC SHA-256
`1353e9bdd29a7295c7226bf6c63abccce056d8cac31f112e5cdbecc3f28c2769`;
Python3.12.3 SHA-256
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`.
Player oracle binary SHA-256 `2fca10598a1893a2d71ff48b94fcbc079eacb9be24a3bded9caacf1c3f1ec323`.

The public native capture corroborates player IDs; pet physical row ID/pet ID/
parent row/UID/vnum; and current UID root/parent/owner/revision/vnum/state/equipment
plus original coin payload bytes. Player physical rows, raw pet mappings and
physical equipment/quantity/type/value literals are absent from that public
capture. Those fields are checked by explicit canonical SQL fixtures, without
claiming native capture parity. Native semantics were inspected in
`player_load_repository.c`, `player_snapshot_repository.c` and the original
snapshot codec. This does not execute native player login, gameplay producers,
full runtime payload/hold/death verification or a native cold restoration.

SELECT readers reject UPDATE. Full application-table inventories match before/
after every native case. All six final private DB daemons stop normally; no game
server starts. Source bodies, modes and links are authenticated before/after.
POSIX attributes are captured before copying; Windows copy modes are not used
as native evidence. Only deliberate orphan fixtures temporarily disable FK
checks in the private writer session; original schema and reader stay unchanged.

| Whole runner log | Exit | SHA-256 |
|---|---:|---|
| `whole-snapshot-mariadb` | 0 | `76059a57460f949e1a041afb847a7f5a92795c5b07ec062c7bba25a254dbce5a` |
| `whole-snapshot-mysql` | 0 | `c5f624ab0412af13f248fa6a16e6db562b8908155028c0c86a94c85c7b219a5e` |

## Evidence, attempts and curator handoff

- Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/sql-player-20261007/`.
- Cut outputs: `D:/Dev/Builds/Duris/accounting-plan5-sql-player-20261007/green03/bin/tests/sql-player-custody/`.
- Helpers: `D:/Dev/Temp/accounting-plan5-sql-player/`; sealed copies in `helpers/`.
- Native/independent per-engine/per-case cuts and `evidence.json` remain in the
  build directory; observation SHA-256 `c368f08cab1278c1cf68a79c95955dbcf49fd28519d8468284795cec5d8241b2`.
- Source archives/manifests, commands, terminal states, controls, complete logs,
  private DB files and pre-copy native attribute inventories are retained.
  `seal/evidence.json` authenticates evidence/builds/publication;
  `delivery/result.json` records post-push remote equality and all seven ancestors.

Preserved prior attempts:

- `green`: exit 1, 152.657607666s. All four modules passed; the new predecessor control omitted current vnum and stopped before the full SQL runners. The observer fixture now supplies vnum=1. Audit code was not relaxed. Frozen source, logs, native cuts and normally stopped private DBs remain retained.
- `green02`: exit 0, 187.961247173s. The complete batch passed, including both full SQL runners. Subsequent review established a global false-clear from boolean owner/floating revision aliases in the new inline-coin exemption. This passing source was superseded by exact identity validation and added regression controls. Its authentic source is replayed in the final alias control; all evidence is retained.

The curator can apply this packet and the additive remote follow-up to the
primary-local notebook. Acknowledgement and application are not claimed or
required for continued independent work.

## Shared interface request and remaining gates

No shared interface change is needed to integrate these owned readers. For full
native capture parity, the narrow optional primary handoff is raw
`player_items.id,pid,container_id,obj_uid,vnum,equip_slot,quantity,item_type,
value0..value3`, raw `player_pets.id,owner_pid,pet_uid`, and the missing physical
pet `equip_slot,item_type,value0..value3` fields alongside its existing five.
Preserve exact NULLs, signed tinyint type/equipment, nullable uint16 quantity,
signed int32 vnum/values, uint32 row/owner IDs and nullable uint64 UIDs. No joins
may discard orphan/legacy rows. Consumers are this correspondence test and
independent reconciliation; shared source bounds, original RR/read-only session
and immutable source authentication must stay intact. Both-engine tests must
cover overlapping row IDs, changed mappings, NULL/legacy fields, wrong parents,
wallet exclusions, nested coins and late concurrent inserts. No shared schema
or implementation is edited here.

Full prototype/runtime payload/hold/retained-death authority and complete world/
value/UID/origin/writer census remain. Native EAB2 installation and recapture,
authenticated full retention/erasure/restore continuity, populated upgrades and
reruns, original native producers/player journeys/fault/restart/load batches,
the primary's combined builds and published combined qualification stay open.
The private RSC2/ZRO1/reset candidate is not a public native-qualified source.
Inventory, synthetic fixtures and component passes do not close release.
Accounting remains inactive; wallet-root exclusions and the declined inactive
spell-path change are preserved. No activation, merge, deployment, production
data change or auto-correction occurred. No independent-work impasse is claimed;
the goal stays active.
