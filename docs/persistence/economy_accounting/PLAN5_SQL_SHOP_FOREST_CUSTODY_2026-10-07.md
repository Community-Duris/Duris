# Plan 5 SQL shop forest custody qualification — 2026-10-07

Curator-ready evidence packet. This closes the independent SQL persisted shop
forest metadata omission. Plan 5, complete R1–R8 and release remain incomplete.
The primary-local notebook and acknowledgement are nonblocking. No notebook
application, primary adoption or combined release qualification is claimed.

## Delivery and exact source

- Local/remote branch: `codex/accounting-plan5`; unchanged throughout this slice.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Branch base: `fbecf592e4c3b424c8e7f4d83538db0bf01143ef`.
- Code commit: `a27e5fe58ed054bd1ea8e976b3e1a22ff6d3e9aa`. The following documentation-only publication commit
  is recorded in `D:/Dev/Tests/Duris/accounting-plan5/sql-shop-20261007/delivery/result.json`.
- Refreshed primary: `6d2bd242df08fbd74c97f9ae5a2dd1617ad8c1c9`.
- Tested composition tree: `71c4a0ccf8de091c14ef3d1815423877741896fb`; source archive SHA-256
  `c0778293b5a646a3cb5b84f029128810e219c1a54da68ad46cd2a4a2d18388eb`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`; migrations tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
  Both canonical engines ended at `64 / 0064_auction_custody_history`.
  These are fresh-schema reader checks; populated upgrade remains open.

The tested composition is the refreshed primary plus the four owned blobs below
and the unchanged prior auction test dependency, blob
`56739f00864bb8058e9741871a1c6fa9ebbddedb`. The latter
provides only the test JSON emitter. Native production source/header bytes,
provider recipes, sanitizer flags, source modes and links are unchanged.

| Owned source | Tested Git blob |
|---|---|
| `scripts/economic_sql_audit_snapshot.py` | `308b778499e2ca5f6b3e9e22d163f7373df5ef7d` |
| `scripts/reconcile_economy_accounting.py` | `e7b7d30063ceb09cdaffcd006b9f7a79f9e91038` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `c201149b213cd32112a7967112c69d4ff0670659` |
| `tests/async/test_sql_shop_custody_audit.py` | `a6693c44cfa8cb673fd2e7f7d2c1d402d3236e79` |

Owned documentation is this packet plus an additive
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md` entry. No shared coordinator, contract,
producer, registry, matrix, migration, native recipe or activation file changed.
All seven previously consolidated branch tips remain ancestors and have their
existing follow-ups on this same remote branch. No work was moved or discarded.

## Established defect and scoped fix

The predecessor exported `item_current_owner` without reading physical
`shopkeeper_items` or mapping logical shop IDs. Authenticated predecessor controls
accepted wrong logical ownership, an unadmitted physical UID, wrong physical
equipment and a missing physical root. All four produce findings now, with
unchanged input. See `green03/red-controls.json`; predecessor reconciler SHA-256
`b6e02a4e90280efb35447a3891be377df6d3b5b330a54c87a1f521bbeb0235c1`.

The exporter reads every keeper and physical row in the existing repeatable-read,
consistent-snapshot, read-only cut. It does not join against current custody or
filter out legacy, orphan or foreign-parent rows. It checks the combined source
count before bounded detail reads, requires the physical table to be InnoDB,
and keeps empty collections as lists. MariaDB's SQL expression promotion is
avoided: raw integer quantity is read and NULL receives native default 1.

The additive diagnostic fields are `native.shop_keepers` (`keeper_id,shop_id`),
`native.shop_items` (`item_id,keeper_id,parent_id,uid,vnum,equipment_slot,quantity`),
and `native.shop_custody_coverage` (`keepers,items`). Coverage must exactly match
collection counts, with aggregate at most 100,000 rows. The existing global
32 MiB encoded-input bound and output maximum 100 remain. Missing shop coverage
in an old SQL partial cut yields a finding instead of implicit completeness.

The reconciler validates exact representations before indexing. Native ownership
comes from logical `shop_id + 1`, owner type 9 and context 0, independently of
current metadata and the keeper's database key. Parent links traverse physical
row IDs, then compare UID root/parent positions. It reports missing/ambiguous
keepers, duplicate IDs/UIDs, missing or foreign parents, cycles, unresolved legacy
UID ancestors, inactive/unadmitted UIDs, owner/vnum/equipment disagreement,
quantity violations, 32-row native depth and 4096-item keeper limits, the forbidden
UINT64_MAX physical UID, and reverse live-custody coverage. Tombstones receive
no physical-row requirement or historical ownership grant. NULL/zero UIDs remain
unknown; no enrollment or source defaults are invented.

Findings expose bounded numeric IDs only. Global counts remain identical at
limits 0/1/100. The CLI preserves input bytes and suppresses unrelated aliases.
The export remains `complete=false` with explicit shop payload/prototype/coin/
enrollment-history and broader native coverage gaps.

## Exact commands, backends and results

From the worktree, the successful frozen run was:

```text
python -B D:/Dev/Temp/accounting-plan5-sql-shop/freeze.py green03
python -B D:/Dev/Temp/accounting-plan5-sql-shop/run.py green03
```

`green03/docker-command.json` records the entire Docker command: network none,
read-only root, 2 CPUs/4 GiB, source/strict scratch/database on RAM filesystems,
and a direct D: bin bind mount. Image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
The entry point is `python3 -u -B /evidence/green03/observer.py green03`.

- `test_sql_shop_custody_audit`: 7 methods passed, zero skips, including the
  original complete opt-in native method on both canonical engines.
- `test_reconcile_economy_accounting`: 134 loaded, 131 executed, 3 skips.
- `test_economic_sql_audit_origins`: 58 loaded, 55 executed, 3 skips.
- Full original `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`
  passed separately on private MariaDB 10.11.14 and MySQL 8.0.46. Its modeled
  schema subset is separate from the fresh canonical native capture databases.
  All prior assertions are retained. The repeatable-read control additionally
  proves that a concurrent shop ID change/physical insert is absent from the
  first cut and present only in a later cut.
- Four authenticated predecessor false-clear controls passed.
- Complete measured run: `133.460695803` seconds under the unchanged
  900-second whole-run budget; `829` observed commands.
- Python AST and Git diff checks passed. No production C/C++ file changed, so
  no redundant full Make build was run for this Python reader slice. The prior
  sealed native833 fresh 754-unit flat build remains recorded at its exact source.

The six existing skips, not counted as passes, are:

- `test_reconcile_economy_accounting.AuditBudgetTests.test_near_limit_mapping_snapshot_cli_budget_and_limit_invariance`: requires explicit Linux audit budget invocation.
- `test_reconcile_economy_accounting.AuditBudgetTests.test_near_limit_price_view_cli_budget_and_limit_invariance`: requires explicit Linux audit budget invocation.
- `test_reconcile_economy_accounting.NativeStakeSQLTests.test_native_stake_sql_both_engines`: requires explicit disposable Linux native/SQL stake invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_captured_source_bindings_both_engines`: requires explicit disposable Linux native/SQL integration invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mariadb`: requires explicit disposable Linux native/SQL integration invocation.
- `test_economic_sql_audit_origins.NativeSQLOriginTests.test_native_origins_mysql_8`: requires explicit disposable Linux native/SQL integration invocation.

The genuine oracle compiles unmodified
`src/persistence/economic_sql_source_snapshot.c` and original headers with GCC
13.3, C++20, `-Wall -Wextra -Wpedantic -Werror -O1 -g`, ASan/UBSan,
`-fno-omit-frame-pointer -fno-pie -no-pie`, native MySQL flags/libs and `-lcrypto`.
The complete command is in `green03/commands.json`; there is no fake provider
or edited production body. Compiler SHA-256
`1353e9bdd29a7295c7226bf6c63abccce056d8cac31f112e5cdbecc3f28c2769`;
Python 3.12.3 SHA-256
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`.
Oracle binary SHA-256: `b4866271aca41268030e341ab71c3dc5adfc769694c7db76284f54f691abb965`.

There are 38 genuine source captures (19 per engine): healthy nested row-ID
forest/logical shop zero, wrong database-key owner, wrong root, missing physical
child, unadmitted child, foreign keeper parent, cycle, duplicate physical UID,
legacy NULL root, tombstone with physical root, vnum/equipment/quantity damage,
dangling keeper/parent, UID sentinel, NULL equipment and legacy NULL quantity.
Only the two deliberate orphan mutations temporarily disable the private writer
connection's FK checks; production schema and reader sessions stay unchanged.

The oracle corroborates keeper ID/logical shop ID; physical ID/keeper/parent/UID/
vnum; and every current UID owner/root/parent/revision/vnum/state/equipment field.
The public native physical capture does not include physical equipment/quantity;
those independent SELECT fields are checked by explicit canonical SQL fixtures,
not claimed as native capture parity. Native semantics are traced to
`shop_item_runtime_payload.c` keeper forest verification and
`item_transfer_command.c` shop owner encoding. This does not execute shop trade,
checkpoint payload verification, gameplay or a native cold restoration.

SELECT-only readers reject UPDATE. Every complete application-table inventory
is identical before/after each cut. All four final private database daemons stop
normally; no game server is started. Source bodies, native modes and symlink
targets are authenticated before/after. Copied native filesystem attributes are
captured before copying; Windows copies are not treated as POSIX mode evidence.

| Whole runner log | Exit | SHA-256 |
|---|---:|---|
| `whole-snapshot-mariadb` | 0 | `844cf3551fad17fb88b41596158195f6a9c2e5bcae6bb0f2be062d2cec3dda47` |
| `whole-snapshot-mysql` | 0 | `da6bd95cae80ace5db2c09b60e2ab188b1b6537f6498b1d701485f3bb26c1c0c` |

## Evidence, failed attempts and curator handoff

- Raw evidence: `D:/Dev/Tests/Duris/accounting-plan5/sql-shop-20261007/`.
- Build/cut outputs: `D:/Dev/Builds/Duris/accounting-plan5-sql-shop-20261007/green03/bin/tests/sql-shop-custody/`.
- Helpers: `D:/Dev/Temp/accounting-plan5-sql-shop/`; copies are sealed in `helpers/`.
- Per-engine/per-case raw native and independent JSON cuts and `evidence.json`
  are in the build directory; observation SHA-256 `54462494c769903ca738cde43fece99c085362ae354a0a6dc180a00b981d61d3`.
- `green03/source.json`, `source.tar`, `commands.json`, `terminal.json`,
  `red-controls.json`, full container/SQL logs, preserved private DBs and native
  attribute inventories are retained. `seal/evidence.json` authenticates raw
  evidence/builds/publication; `delivery/result.json` records the post-push result,
  remote equality and preservation of seven prior tips.
- `green` failed after 20.914732827 seconds because MariaDB promoted the SQL
  COALESCE quantity to Decimal. The exporter now reads raw integer quantity.
- `green02` failed after 19.364093085 seconds because the new fixture reset tried
  to delete a parent before its child. The reset now deletes children first;
  the original restrictive FK is preserved. Both failed captures, logs, source
  archives and normally stopped private databases remain sealed.

The notebook curator can apply this packet and the additive remote follow-up.
No curator acknowledgement or notebook application is asserted or required to
continue independent work.

## Shared interface request and remaining gates

No shared interface change is required to integrate this owned reader fix.
For primary-owned native physical capture/union completeness, the narrow optional
request is to retain raw `shopkeeper_items.equip_slot` (nullable signed tinyint)
and `quantity` (nullable unsigned smallint) alongside existing ID/keeper/parent/
UID/vnum, and preserve raw `shopkeepers.id,shop_id` as independent mapping facts.
NULL quantity must remain raw in native capture, with native semantic default 1
applied only by the consumer; NULL equipment must not silently become zero.
Consumers are this correspondence test and the independent audit input. Tests
must compare healthy/nested/NULL/changed equipment and quantity on both engines,
retain dangling rows without joins, preserve bounds/read-only behavior and use
the genuine source reader. This request edits no shared implementation here.

Complete literal/prototype/runtime-payload/coin and enrolled-history authority
for shops remains open, as do other physical SQL forests and complete world/
UID/value/origin census. Native EAB2 installation/recapture, authenticated full
retention/erasure/restore continuity, populated upgrades/reruns, original native
producer/gameplay/fault/restart/load journeys, both maintained builds for the
eventual combined candidate, and primary publication/qualification remain gates.
The private reset interface is still not a public native-qualified source.

This is a completed reader slice, not release completion. Inventory, synthetic
fixtures and isolated passes do not close Plan 5. Accounting stays inactive;
wallet-root money exclusions and the declined inactive spell path are preserved.
No deployment, merge, production data change, auto-correction or activation
occurred. There is no current independent-work impasse; the goal remains active.
