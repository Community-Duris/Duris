# Siege and Kingdom Removal Research

Date: 2026-08-31
Branch: `master`
Research baseline: `63cb1ab9`
Status: Repository research complete; production and retained-backup preflight outstanding;
no implementation changes made

## Executive conclusion

The siege feature can be removed without removing any live kingdom system because no
separate kingdom implementation exists. The implemented town-defense behavior, siege
engines, town persistence, and the unfinished kingdom scaffolding are one dormant feature
cluster.

The feature is disabled in the supported server build, but it is not detached. The
flat-file siege regression explicitly compiles it with `-DSIEGE_ENABLED`, so it remains
an exercised optional test configuration even though normal builds do not enable it:

- `src/siege.c` is still compiled and linked.
- Ten quartermaster mobs still reset into hometowns, although their special procedure is
  not assigned in the supported default build.
- Dedicated siege object prototypes and an isolated siege zone still enter generated
  world data.
- Four of the object prototypes are also copied into the tracked `areas_mini` dataset.
- Five legacy database tables still exist.
- Current flat-file town and siege authority implementations, legacy import paths, a
  tracked town seed, and focused regressions still exist.
- The `add`, `deploy`, `toggle kingdom`, `help add`, and `help kingdoms` surfaces still
  leave user-visible or operator-visible residue.
- Object-destruction compatibility state is spread across 62 source files even though it
  cannot become active in the default build.

The recommended end state is:

1. Remove every runtime, command, UI, help, and world-data path belonging to siege and the
   unfinished kingdom design.
2. Remove the dead object-destruction state and its behavior-neutral guards.
3. Reserve persisted numeric identifiers instead of shifting or reusing them.
4. Keep the five legacy database tables as clearly classified compatibility tombstones
   until a future schema-baseline rollover. Do not rewrite historical migrations or add a
   destructive drop migration during this cleanup.
5. Preserve unrelated uses of ordinary words such as "siege", "kingdom", and "town" in
   zones, lore, historical news, and active generic systems.

This should be implemented as a focused project with two deployment gates if production
contains any of the dedicated object VNUMs. It should not be treated as deleting only
`siege.c` and `siege.h`.

## Scope and terminology

In this document:

- "Siege" means the code behind `SIEGE_ENABLED`, the ballista, battering ram, catapult,
  town gates, persistent siege objects, and object-destruction combat state.
- "Town defense" means the resource, guard, cavalry, portal, quartermaster, and hometown
  logic implemented inside `src/siege.c`.
- "Kingdom scaffolding" means the unused troop model, land-import helper, room fields,
  map toggle, and proposal help file. It does not include fantasy lore or zones whose
  proper name contains "Kingdom".
- "Historical schema" means the sealed legacy baseline and applied migration inputs.
  Those files are evidence, not active feature code, and must remain immutable.

## Evidence and current state

### Historical intent

Git history records the feature as unfinished and deliberately disabled:

- `8e4490f4` - `Added SIEGE_ENABLED flag to disable siege on master until finished`
- `73b25534` - `Changed how siege should be enabled`

No later commit re-enabled the macro in the supported build. Recent commits restored and
hardened dormant persistence without reconnecting the normal runtime feature:

- `72568298` - `Restore historical flat town persistence`
- `c511f4ac` - `Connect safe flat siege persistence`
- `ebcf5af8` - `Restore historical siege imports`

### Runtime state

[`src/siege.h`](../../../src/siege.h#L4) contains a commented-out
`SIEGE_ENABLED` definition. The normal build does not define it elsewhere.

The following external entry points are all guarded by `#ifdef SIEGE_ENABLED`:

| Runtime seam | Current location | Disabled behavior |
| --- | --- | --- |
| Town and siege boot | [`src/comm.c`](../../../src/comm.c#L762) | Does not load towns, siege objects, or special procedures |
| Town deployment on zone reset | [`src/db.c`](../../../src/db.c#L3933) | Does not deploy guards or cavalry |
| Gate movement checks | [`src/actmove.c`](../../../src/actmove.c#L726) | Does not block invaders with siege gates |
| Attacking siege objects | [`src/actoff.c`](../../../src/actoff.c#L767) and [`src/actoff.c`](../../../src/actoff.c#L1727) | `hit` and `kill` cannot enter object combat |
| Object-combat pulse | [`src/fight.c`](../../../src/fight.c#L9479) | No siege attacks execute |
| Immortal `add` command | [`src/interp.c`](../../../src/interp.c#L2608) | Handler is not registered |
| `IS_DESTROYING` | [`src/utils.h`](../../../src/utils.h#L738) | Expands to `false` |

Despite that, [`src/Makefile`](../../../src/Makefile#L366) still links `siege.o`. The current
default binary contains `init_towns`, `init_siege`, `warmaster`, `check_deploy`, `do_add`,
siege SQL functions, and `siege_objects`. Disassembly found no external calls into the
guarded feature entry points; calls among functions inside `siege.c`, including
`set_destroying`, remain.

The main source files contain 2,760 lines:

- [`src/siege.c`](../../../src/siege.c): 2,718 lines
- [`src/siege.h`](../../../src/siege.h): 42 lines

### What the implemented code actually does

The live-looking implementation is not the kingdom design described by the help file.
[`warmaster`](../../../src/siege.c#L1743) finds a `town` by current zone and uses player
surname rank to:

- donate item value into town resources;
- buy ballista, battering ram, catapult, or gates;
- toggle deployment of hometown guards, cavalry, or portals;
- persist town resource and deployment configuration.

It does not use guild membership, kingdom membership, kingdom land, virtual troop
movement, declarations of war, conquest, or the map ownership model.

[`lib/information/helpkingdoms`](../../../lib/information/helpkingdoms#L2) calls itself a
"Duris Kingdom Code Proposal" and describes systems which were never connected to the
runtime.

### Persistence modes

Both supported persistence modes still carry feature code:

- MariaDB town and siege persistence is split between `src/sql_player.c` and
  `src/siege.c`.
- Under `__NO_MYSQL__`, `src/sql_player.c` maintains a canonical `metadata/towns`
  authority, imports `Players/towns` when present, and otherwise seeds from the tracked
  [`defaults/towns`](../../../defaults/towns) file.
- Under `__NO_MYSQL__`, `src/siege.c` maintains a checksummed `metadata/siege` authority
  and imports the legacy `Players/siege` serialization.
- [`tests/async/test_flatfile_towns.py`](../../../tests/async/test_flatfile_towns.py) and
  [`tests/async/test_flatfile_siege.py`](../../../tests/async/test_flatfile_siege.py), with
  their C++ harnesses, exercise these paths and have dedicated CI steps.

The generic flat-file store, authority locking, backup mechanism, player/item custody
repository, and serializer are shared infrastructure and must remain.

### Local database evidence

Read-only inspection of the allow-listed local database on 2026-08-31 found:

| Store | Exact rows |
| --- | ---: |
| `towns` | 0 |
| `kingdom_land` | 0 |
| `siege_items` | 0 |
| `siege_item_affects` | 0 |
| `siege_item_extra_descr` | 0 |

No dedicated siege object VNUM was present in the direct item stores listed in the
preflight below, including active auction custody, open auctions, or the artifact
configuration/state tables. A guarded host-format inspection validated the version and
single-item header of the 38 unretrieved legacy `auction_item_pickups` blobs and found no
dedicated root VNUM. Production must use the bounded repository-format decoder specified
below rather than treating this local shortcut as reusable audit tooling.

The local `pages` table contains one `kingdoms` page and no `ADD` page. No local player
record has persisted `PLR2_KINGDOMVIEW` bit `BIT_4` set. These counts are local evidence
only; production must be audited independently through an approved read-only path or a
recent restored clone.

The only database foreign keys involving the feature are:

- `siege_item_affects.item_id -> siege_items.id`
- `siege_item_extra_descr.item_id -> siege_items.id`
- `siege_items.container_id -> siege_items.id`

There are no local views, routines, or triggers that reference the five legacy tables.

The local checkout uses `mariadb-primary`, has no configured `FLATFILE_STATE_DIR`, and has
no `Players/towns`, `Players/siege`, or `Players/Kingdoms` path. Consequently this audit
has no local flat-file production-state count; the tracked `defaults/towns` seed does
exist.

### Research coverage and limits

The repository audit covered tracked source, tests, CI, area inputs, the tracked minimal
world, help sources, migrations, manifests, operational scripts, `src-migrate`, Git
history, the built default binary, and a read-only local MariaDB instance. Generated and
ignored full-world outputs were treated as derivatives of tracked `areas/` inputs;
`areas_mini` was treated separately because it is tracked and is not regenerated by
`make world`.

No production database, production flat-file authority, external builder repository, or
retained backup generation was inspected. Those are explicit deployment gates below, not
evidence that production is empty.

## Dependency inventory

### 1. Core siege and town-defense implementation

Delete after callers and persistence dependencies are removed:

- `src/siege.c`
- `src/siege.h`
- `siege.o` from `src/Makefile`
- `event_move_engine` from the Siege Engines section of `src/prototypes.h`
- `ballista`, `battering_ram`, `catapult`, and `castlewall` declarations from
  `src/specs.prototypes.h`

Remove the feature includes and guarded hooks from:

- `src/comm.c`
- `src/db.c`
- `src/actmove.c`
- `src/actoff.c`
- `src/fight.c`
- `src/interp.c`
- `src/sql_player.c`

Do not retain the `SIEGE_ENABLED` macro as an optional build configuration. Once the
implementation is removed, keeping the flag would advertise a configuration that cannot
exist.

### 2. Object-destruction combat compatibility state

Siege object combat introduced a second combat target beside `GET_OPPONENT(ch)`:

- `char_special_data::destroying_obj`
- `char_special_data::next_destroying`
- global `destroying_list`
- `IS_DESTROYING(ch)`
- `set_destroying()`
- `stop_destroying()`
- object target rendering in both prompt implementations and character status output

Current baseline counts are:

| Surface | Count |
| --- | ---: |
| Source files containing destruction scaffolding | 62 |
| `stop_destroying(...)` occurrences | 125 |
| `IS_DESTROYING(...)` occurrences, including definitions | 287 on 285 source lines |

This is broad but behavior-neutral cleanup. In the default build, `IS_DESTROYING(ch)` is
always false, `set_destroying()` has no reachable caller outside `siege.c`, and the raw
object pointer remains null. Required transformations are therefore mechanical:

- `A || IS_DESTROYING(ch)` becomes `A`.
- `A && !IS_DESTROYING(ch)` becomes `A`.
- standalone `if (IS_DESTROYING(...))` branches are deleted.
- guarded `stop_destroying(...)` calls are deleted.
- raw prompt/status reads of `destroying_obj` are deleted.
- after all callers are gone, delete the fields, list, macro, prototypes, setter, stopper,
  and the destruction loop in `perform_violence()`.

Do not perform blind textual replacements. Parentheses, negation, early returns, and
`if`/`else` ownership must be reviewed file by file. The complete current file set is in
Appendix A.

### 3. Town and kingdom structs and constants

Remove these active-source remnants after the siege and SQL functions are gone:

- global `P_town towns` in `src/db.c`;
- the unused `extern P_town towns` in `src/actwiz.c`;
- `struct town` and `P_town` in `src/structs.h`;
- all `TROOP_*` and `NUM_TROOP_*` definitions in `src/structs.h`;
- `struct troop_info_rec` in `src/structs.h`;
- commented `room_data` kingdom number, kingdom type, and troop pointer fields;
- `troop_types`, `troop_levels`, `troop_offense`, `troop_defense`, and `troop_costs`
  in `src/constant.c`;
- unused `kingdom_type_list` in `src/constant.c` and its `src/actwiz.c` declaration;
- unused `Guild::is_kingdom()` declaration in `src/assocs.h` and its offline-tool stub in
  `src-migrate/migrate_stubs.c`;
- the commented kingdom room-stat output in `src/actwiz.c`;
- the commented troop display in `src/actinf.c`;
- stale commented kingdom calls in `src/db.c`, `src/handler.c`, and `src/weather.c`.

The troop definitions have no consumers outside their definitions. The room ownership and
troop fields are already commented out, so the kingdom map and land model cannot operate.

Do not remove these unrelated live concepts:

- surname ranks, including king and lord;
- `IS_INVADER` and hometown justice;
- generic zone mob scaling through `apply_zone_modifier()`;
- ordinary hometown logic;
- guilds and associations.

The active administrator text in [`src/assocs.c`](../../../src/assocs.c#L1784) should change
from "Standard Guilds and Kingdoms" to "Standard Guilds" because it currently advertises
a system that does not exist.

### 4. Command IDs and toggle compatibility

#### `add` and `deploy`

`CMD_ADD` is command number 827 and `CMD_DEPLOY` is command number 828. The command string
array and explicit numeric constants are positional interfaces used by special
procedures. Do not delete array slots or renumber later command IDs.

Required cleanup:

- remove the guarded `CMD_GRT(CMD_ADD, ..., do_add, ...)` line;
- remove `CMD_TRIG(CMD_DEPLOY, ...)`;
- replace command-array spellings 827 and 828 with unique internal tombstone names and
  rename their `src/interp.h` constants to explicit retired-slot names;
- remove their stale entries from
  `docs/lib/information/command_attributes.txt`;
- remove the `ADD (Immortal Command)` help entry.

Keep the two array elements in place. The command table and `CMD_*` constants are a
positional ABI: deleting the elements would shift every later command. Older removed
entries such as `speak`, `reloadhelp`, and `tether` retain their old user-facing spelling
and therefore still produce "that command has yet to be implemented". Do not copy that
visible residue for this cleanup; use unmistakable non-feature placeholders such as
`_retired_827` and `_retired_828`, with null command pointers. Never use `"\\n"`, which is
the command-array sentinel.

Keep shared commands used by other systems, including `CMD_RELOAD`, `CMD_FIRE`,
`CMD_PUSH`, `CMD_LIST`, `CMD_BUY`, and `CMD_DONATE`.

#### `toggle kingdom`

`toggle kingdom` is parallel state spread across:

- the status format and argument list in `src/actoth.c`;
- `toggle_names` index 32;
- `tog_messages` index 32;
- `do_toggle` case 32;
- `PLR2_KINGDOMVIEW` in `src/structs.h`;
- the static TOGGLE help example in `lib/information/help_index`.

Remove the visible option and carefully renumber the later toggle array/case positions.
Toggle option indexes are dispatch positions, not persisted values.

Do not shift the persisted `act2` bits. Rename `PLR2_KINGDOMVIEW` to a clearly reserved
value such as `PLR2_RETIRED_KINGDOMVIEW`, leave it assigned to `BIT_4`, and do not reuse
that bit. Existing player rows may retain it harmlessly. A separate approved data cleanup
may clear the bit with `act2 & ~BIT_4`, but clearing it is not required for runtime
correctness and must not be bundled as an assumed production mutation.

### 5. Runtime persistence code

Remove the MariaDB implementations for:

- `sql_save_towns()`
- `sql_load_towns()`
- `sql_save_kingdom_land()`
- `sql_save_siege_item()`
- `sql_save_siege_list()`
- `sql_delete_siege_items()`
- `sql_load_siege_list()`
- siege-only recursive save/load and affect helpers

Remove their declarations from `src/sql_player.h` and the `siege.h` dependency from
`src/sql_player.c`.

Remove the matching no-MySQL placeholder definitions for the SQL-only functions after
their callers and declarations are gone.

The no-MySQL feature path is not only stubs: town persistence and `siege.c` contain real
implementations. Remove the feature-only flat-file code at the same time:

- `flat_town_*` parsing, seed import, live-list replacement, save, and load code in
  `src/sql_player.c`;
- `flat_siege_*` legacy decoding, validation, materialization, save, and load code in
  `src/siege.c`;
- tracked `defaults/towns`;
- `tests/async/test_flatfile_towns.py` and
  `tests/async/flatfile_town_harness.cpp`;
- `tests/async/test_flatfile_siege.py` and
  `tests/async/flatfile_siege_harness.cpp`;
- the two corresponding steps in `.github/workflows/quality.yml`.

Preserve `src/flatfile_store.*`, the item-ownership authority, backup/restore tooling, and
all other shared flat-file infrastructure. Remove the direct `flatfile_store.h` include
from `src/sql_player.c` only if the town cleanup leaves it unused.

There is one unrelated private-chest log message in `src/sql_player.c` which incorrectly
says `sql_load_siege_item_contents`. Rename that label to its private-chest function while
removing the real siege loader; otherwise a misleading siege string survives in active
code.

### 6. Season reset and lifecycle inventory

`src/sql.c` currently treats the three siege item tables as season-reset state. When
runtime siege persistence is removed:

- remove the three tables from `sql_verify_pwipe_manifest()`;
- remove their `DELETE` statements from `sql_pwipe()`;
- rename the combined "siege and shopkeeper" log group to shopkeepers only;
- remove the explicit siege child/parent ordering assertion from
  `tests/async/test_season_reset_manifest.py`;
- change each `season_action` for the three siege table entries in
  `migrations/data_lifecycle_manifest.json` from `reset_delete` to `retain`, change
  `active_retention` from season-scoped to service-lifetime compatibility retention, and
  describe the `technical_purpose` as retired compatibility schema.

Keep the `towns` and `kingdom_land` lifecycle entries as `retain`; they already have that
classification. Keep all five entries while the tables exist because the lifecycle
validator requires exact schema coverage.

### 7. Historical schema boundary

Do not alter these historical and runtime-compatibility artifacts as part of the cleanup:

- `migrations/bootstrap_multithread_safe.sql`
- `migrations/bootstrap_legacy_baseline.sql`
- `migrations/run_migration.sh`
- `migrations/pfile_to_db_initial_schema.sql`
- `migrations/pfile_to_db_combined_migration.sql`
- `migrations/schema_migration_v2.sql`
- `migrations/item_ownership_ledger.sql`
- `migrations/migration_manifest.json`
- `migrations/runtime_compatibility_manifest.json`
- `src/runtime_compatibility_contract.h`
- `tests/test_run_migration_persistence_schema.sh`
- already checksummed migration files and verification inputs

Reasons:

1. The verified Session 11 baseline is an exact 170-table name fingerprint.
2. Fresh bootstrap and legacy adoption must reproduce that baseline before immutable
   migrations run.
3. Post-baseline migrations are required to be additive and re-runnable. Dropping the
   five tables is destructive and outside that contract.
4. Runtime compatibility includes an exact cross-engine metadata fingerprint.
5. The item ownership migration's reference to `siege_items` is a one-time allocator
   floor calculation, not a live view or routine. It remains valid while the compatibility
   table remains.

The five legacy tables should remain as schema tombstones during this cleanup regardless
of whether another environment contains rows. A future explicit baseline-rollover project
may remove them after retention approval, production archive verification, MySQL 8 and
MariaDB 10.11 fingerprint regeneration, bootstrap and legacy adoption redesign,
lifecycle inventory removal, and rollback planning.

### 8. World-data inputs

The tracked feature data consists of:

- `areas/mob/siege.mob` - ten quartermasters plus unused mob 401001;
- `areas/obj/siege.obj` - empty file;
- `areas/wld/siege.wld` - isolated room 401000 with no incoming exit;
- `areas/zon/siege.zon` - zone 4010 with no reset commands;
- `areas/AREA` entry `siege *4010`;
- ten quartermaster reset commands in hometown zone files;
- eight low-numbered object prototypes in `areas/obj/heavens.obj`;
- duplicate prototypes 160, 161, 178, and 179 in the tracked
  `areas_mini/mini.obj` runtime dataset;
- two siege-ammunition sale entries in `areas/shp/kzkrkeep.shp`.

Quartermaster reset references:

| Mob | Hometown zone source |
| ---: | --- |
| 401000 | `areas/zon/tharnadia.zon` |
| 401010 | `areas/zon/woodseer.zon` |
| 401020 | `areas/zon/ashrumite.zon` |
| 401030 | `areas/zon/charing.zon` |
| 401040 | `areas/zon/kimordril.zon` |
| 401050 | `areas/zon/faang.zon` |
| 401060 | `areas/zon/ghore.zon` |
| 401070 | `areas/zon/goblinht.zon` |
| 401080 | `areas/zon/khildarak.zon` |
| 401090 | `areas/zon/shady.zon` |

Dedicated object prototypes:

| VNUM | Object | Current acquisition/use |
| ---: | --- | --- |
| 160 | shattered ballista missile | No code reference found |
| 161 | shattered catapult missile | No code reference found |
| 178 | ballista missile | Siege engine ammo; still sold by `kzkrkeep` |
| 179 | catapult boulder | Siege engine ammo; still sold by `kzkrkeep` |
| 461 | ballista | Bought from a warmaster when enabled |
| 462 | battering ram | Bought from a warmaster when enabled |
| 463 | catapult | Bought from a warmaster when enabled |
| 464 | town gates | Bought from a warmaster when enabled |

Removal rules:

1. Remove the ten hometown reset commands before deleting `siege.mob`.
2. Replace the `areas/AREA` entry with a retired reservation comment for zone 4010 rather
   than advertising it as open space.
3. Delete the four dedicated siege area files.
4. Stop sale of VNUMs 178 and 179 before deleting any of the eight prototypes.
5. Delete the eight full-world prototypes and the four tracked minimal-world copies only
   after the production custody audit is zero or an explicit compensation/conversion
   policy has been completed.
6. Never renumber adjacent objects and do not reuse zone 4010, room/mob range 401000+, or
   the eight retired object VNUMs.
7. Run `make world` to regenerate ignored `areas/world.*` and `lib/misc/lookup*` outputs.
   Do not hand-edit or commit generated outputs. Edit `areas_mini/mini.obj` directly
   because it is a tracked source/runtime fixture and `make world` does not regenerate it.

VNUMs 178 and 179 are the only continuing acquisition risk because a live shop still
sells them. Any of the eight VNUMs can require a second release when custody is nonzero.
If the production audit is zero immediately before deployment, the acquisition freeze and
prototype deletion may be combined. Otherwise:

- Release A removes the shop entries and all acquisition/runtime paths but temporarily
  retains the prototypes so existing items can still load.
- Release B removes the prototypes after the custody count reaches zero or the approved
  player compensation/conversion is complete.

### 9. Help, documentation, and database pages

Remove or update these authoritative sources:

- delete `lib/information/helpkingdoms`;
- remove `helpkingdoms -> kingdoms` from `scripts/import_help_to_prod.sh`;
- remove the same source/title mapping from `src/flatfile_help_catalog.c`;
- delete the `ADD (Immortal Command)` entry from `lib/information/help_index`;
- remove `Kingdom View` from the TOGGLE help entry;
- remove `add` and `deploy` entries from
  `docs/lib/information/command_attributes.txt`;
- remove `SIEGE_ENABLED` from the active compile-sweep list in
  `docs/guides/BUILDING.md`;
- update `docs/content/HELP_SYSTEM.md` so it no longer lists kingdom help as an imported
  source.

Removing source files does not delete already imported pages. The help importer upserts
present sources and skips absent ones. After backup and target verification, deployment
must remove the exact `pages` rows whose case-insensitive titles are `ADD` and `kingdoms`,
or add an equivalent exact retired-title step to the importer. Do not use `--clean` merely
to remove these two pages; it deletes the entire help table before rebuilding it.

Preserve historical news such as the 2008 statement that guilds could become kingdoms.
It is dated release history, not current feature documentation.

### 10. Operational compatibility paths

`Players/wipers/wipe_it_all` still names a legacy `Players/Kingdoms` directory, and
`scripts/convert_all_pfiles.sh` excludes legacy `Kingdoms` directories from player-file
conversion.

The flat-file feature code also recognizes these live or import paths:

- `FLATFILE_STATE_DIR/metadata/towns` and `metadata/siege`;
- legacy `Players/towns` and `Players/siege`;
- tracked `defaults/towns` as the fallback seed.

Retain both as temporary compatibility safeguards:

- the wiper entry prevents abandoned legacy state from surviving an intentional full
  wipe;
- the converter exclusion prevents legacy non-player data from being misread as player
  files in restored archives.

They may be removed only after the backup retention horizon proves no supported archive
can contain the legacy directory.

Unlike the `Players/Kingdoms` safeguards, the town/siege authorities and seed are active
feature inputs and should be retired with the runtime. Before removal, inventory current
state and every still-supported flat-file backup generation. A backup containing these
authorities or dedicated object VNUMs must either be retired, converted during restore,
or documented as requiring a pre-removal binary; otherwise it can reintroduce state that
the cleaned runtime cannot load.

## Required preflight

Run preflight against an approved read-only production connection or, preferably, a
recent production restore. Never run cleanup mutations while researching.

### Database row and custody audit

```sql
SELECT 'towns' AS source, COUNT(*) AS rows_found FROM towns
UNION ALL SELECT 'kingdom_land', COUNT(*) FROM kingdom_land
UNION ALL SELECT 'siege_items', COUNT(*) FROM siege_items
UNION ALL SELECT 'siege_item_affects', COUNT(*) FROM siege_item_affects
UNION ALL SELECT 'siege_item_extra_descr', COUNT(*) FROM siege_item_extra_descr;

-- Discover every direct VNUM column on the actual target. Classify every result;
-- optional/operator tables may differ between environments.
SELECT table_name, column_name
FROM information_schema.columns
WHERE table_schema = DATABASE()
  AND column_name IN ('vnum', 'obj_vnum')
ORDER BY table_name, column_name;

SELECT source, vnum, COUNT(*) AS rows_found
FROM (
    SELECT 'player_items' AS source, vnum FROM player_items
    UNION ALL SELECT 'corpse_items', vnum FROM corpse_items
    UNION ALL SELECT 'locker_items', vnum FROM locker_items
    UNION ALL SELECT 'account_locker_items', vnum FROM account_locker_items
    UNION ALL SELECT 'saved_items', vnum FROM saved_items
    UNION ALL SELECT 'player_pet_items', vnum FROM player_pet_items
    UNION ALL SELECT 'shopkeeper_items', vnum FROM shopkeeper_items
    UNION ALL SELECT 'siege_items', vnum FROM siege_items
    UNION ALL SELECT 'item_current_owner', vnum FROM item_current_owner
    UNION ALL SELECT 'auction_item_custody', vnum FROM auction_item_custody
    UNION ALL SELECT 'auctions', obj_vnum FROM auctions
    UNION ALL SELECT 'artifact_bind', vnum FROM artifact_bind
    UNION ALL SELECT 'artifacts', vnum FROM artifacts
    UNION ALL SELECT 'artifacts_mortal', vnum FROM artifacts_mortal
    UNION ALL SELECT 'artifact_domain_state', vnum FROM artifact_domain_state
) AS custody
WHERE vnum IN (160, 161, 178, 179, 461, 462, 463, 464)
GROUP BY source, vnum
ORDER BY source, vnum;

SELECT COUNT(*) AS kingdom_view_flagged_players
FROM player_data
WHERE (act2 & 8) <> 0;

SELECT title, COUNT(*) AS rows_found
FROM pages
WHERE LOWER(title) IN ('add', 'kingdoms')
GROUP BY title;
```

The direct-column query is intentionally conservative. Classify matches using the
target's custody/status fields: for example, `item_current_owner.state`,
`auction_item_custody.claimed_at`, and `auctions.status`/`custody_state`. Historical or
derived tables such as `item_ownership_baseline`, `persistence_item_events`, `eq_drop`,
and item-sale/wiki caches are evidence, not current custody, but every discovered VNUM
column still needs an explicit classification.

`auction_item_pickups` has no VNUM column and its unretrieved rows are live custody. Run a
read-only repository-format decoder over every `retrieved = 0` `obj_blob_str`. Also verify
the blob/direct-VNUM agreement for active `auctions.obj_blob_str` and
`auction_item_custody.obj_blob`. Use the repository serializer version checks and bounded
decoder in a purpose-built audit harness; do not infer the VNUM with ad hoc SQL byte
offsets because the legacy serialization uses native-width/native-endian integer fields.
Record only aggregate VNUM counts and corrupt/unsupported-row counts.

### Filesystem and runtime audit

Without reading or publishing private data:

- check whether `Players/Kingdoms/kingdom.land` exists and record only its existence,
  ownership, mode, size, and backup status;
- inspect `Players/towns`, `Players/siege`, configured `FLATFILE_STATE_DIR/metadata/towns`,
  and `metadata/siege` with the repository decoders, recording aggregate counts and file
  metadata only;
- decode `FLATFILE_STATE_DIR/domains/item_ownership` and count all active owners/states for
  the eight VNUMs, not only player-owned items;
- take the final count at a quiesced/reconciled point and inspect or replay any pending
  `.critical-authority-transaction` after-image and critical-command journal frame before
  declaring the authority empty;
- run the same VNUM/state audit against every flat-file backup generation and other
  supported import/restore source inside the retention horizon, including legacy player,
  corpse, locker, auction, shopkeeper, saved-item, and siege serializations;
- confirm there are no operator-maintained town seed files outside the repository and
  record the custody/retirement policy for tracked `defaults/towns`;
- confirm current startup flags do not inject `-DSIEGE_ENABLED` through
  `EXTRA_CFLAGS`;
- confirm no external builder or deployment repository depends on the retired VNUMs;
- archive relevant row counts and checksums in the implementation notes, not player
  identities or item-owner details.

### Blocking conditions

Do not remove object prototypes from either `areas/obj/heavens.obj` or
`areas_mini/mini.obj` when any dedicated VNUM remains in authoritative custody unless an
explicit product decision defines conversion, compensation, or deletion. A corrupt or
unsupported live blob is a blocker until resolved; it is not evidence of zero custody.

Do not declare the gate clear from the live database alone when a supported flat-file or
database backup/import source can restore the dedicated VNUMs without a conversion rule.

Do not remove or repurpose `BIT_4`, command IDs 827/828, zone 4010, room/mob VNUMs
401000+, or object VNUMs 160/161/178/179/461-464 based only on the local zero-row result.

## Recommended implementation sequence

The following sequence keeps commits reviewable and preserves buildability. It may be one
pull request, but the item compatibility gate may require two deployments.

### Step 1 - Add a removal contract test

Add a focused source-contract test before deleting code. It should fail while the feature
is present and pass only when:

- `src/siege.c`, `src/siege.h`, and `siege.o` are absent;
- no production source contains `SIEGE_ENABLED`, `P_siege`, `P_town`, `troop_info_rec`,
  `kingdom_type_list`, `Guild::is_kingdom()`, `flat_siege_*`, `flat_town_*`,
  `IS_DESTROYING`, `set_destroying`, `stop_destroying`, or the destruction fields;
- no dedicated siege area files, hometown quartermaster resets, full-world prototypes,
  or tracked minimal-world prototype copies remain after the custody gate;
- no feature-only flat-file town/siege authority, seed, regression harness, or CI step
  remains;
- the active help/import sources no longer advertise siege or kingdom mechanics;
- no runtime SQL statement reads or writes the retired tables;
- historical migrations, lifecycle tombstones, lore, and explicitly allow-listed zone
  names do not cause false failures.

Use semantic allow-lists rather than a repository-wide ban on English words.

### Step 2 - Freeze acquisition and remove dedicated world wiring

1. Remove VNUMs 178 and 179 from `areas/shp/kzkrkeep.shp`.
2. Remove all ten quartermaster reset commands.
3. Reserve zone 4010 as retired in `areas/AREA`.
4. Delete the four dedicated siege area files.
5. Regenerate world and lookup outputs locally.
6. If custody preflight is zero, delete the eight dedicated full-world object prototypes
   and the four corresponding `areas_mini/mini.obj` copies. Otherwise, leave those
   prototypes until Release B.

This step prevents new data while preserving prototype-based load compatibility if
production contains any legacy dedicated item.

### Step 3 - Remove runtime persistence and town types

1. Delete MariaDB and flat-file siege/town/kingdom-land functions and helpers from
   `src/sql_player.c`, `src/sql_player.h`, and `src/siege.c`.
2. Correct the unrelated private-chest log label.
3. Delete `defaults/towns`, the two flat-file feature regressions and harnesses, and their
   CI steps.
4. Delete `P_town`, `struct town`, troop constants/arrays/structs, and town globals after
   their SQL and siege consumers are gone.
5. Remove `kingdom_type_list`, `Guild::is_kingdom()`, its migration-tool stub, and stale
   commented kingdom room/stat/call code.

Compile here before deleting the main siege translation unit. This isolates persistence
and type-order mistakes.

### Step 4 - Delete the siege translation unit and runtime hooks

1. Remove guarded boot, reset, movement, attack, combat-pulse, and command hooks.
2. Remove siege includes.
3. Remove `event_move_engine` from `src/prototypes.h` and the four siege object-special
   declarations from `src/specs.prototypes.h`.
4. Remove `siege.o` from the Makefile.
5. Delete `src/siege.c` and `src/siege.h`.
6. Replace command spellings and constants 827/828 with retired tombstones without
   shifting later IDs.

Compile again. At this boundary, no siege code remains, but the dead destruction
compatibility state may still compile as a no-op.

### Step 5 - Remove object-destruction compatibility state

1. Apply the behavior-neutral condition simplifications across Appendix A.
2. Remove prompt and status object-target branches.
3. Remove the destruction list and per-pulse loop.
4. Remove `set_destroying`, `stop_destroying`, their prototypes, the two character fields,
   and `IS_DESTROYING`.
5. Format touched C/C++ lines with `./scripts/format.sh`.

This should be a separate reviewable commit because it touches many files despite making
no intended gameplay change.

### Step 6 - Remove kingdom UI and stale association wording

1. Remove `toggle kingdom` from all parallel arrays, status rendering, messages, and
   switch cases.
2. Reserve persisted `act2` `BIT_4` under a retired name.
3. Change the administrator association heading to guilds only.
4. Update TOGGLE help and command attributes.

Exercise every toggle after the removed index; an off-by-one error would toggle the wrong
persisted flag.

### Step 7 - Retire runtime lifecycle handling, not historical schema

1. Remove siege tables from the runtime pwipe preflight and delete sequence.
2. Reclassify their lifecycle actions to `retain` as compatibility tombstones.
3. Remove the obsolete siege ordering assertion.
4. Leave baseline, bootstrap, legacy migration, immutable migration, runtime fingerprint,
   and persistence-schema tests unchanged.

Run lifecycle and season-reset validation immediately after this step.

### Step 8 - Remove current help and documentation surfaces

1. Delete the kingdom proposal help file and both its script and flat-file catalog import
   mappings.
2. Delete ADD help, remove Kingdom View from TOGGLE help, and remove command attribute
   entries.
3. Update build and help-system documentation.
4. On an approved target, remove only the stale `ADD` and `kingdoms` page rows after
   backup and verify their absence.
5. Preserve historical news and unrelated lore.

### Step 9 - Full validation and deployment

Complete all checks below. Use both the tracked minimal dataset and a full cold development
boot: minimal mode catches stale copies in `areas_mini/mini.obj`, while only the full world
contains the dedicated zone and hometown resets.

## Validation matrix

### Static checks

```bash
git diff --check
./scripts/format.sh --check

test ! -e src/siege.c
test ! -e src/siege.h
! rg -n 'SIEGE_ENABLED|#include "siege\.h"|\bsiege\.o\b' src
! rg -n '\b(P_siege|P_town|troop_info_rec|IS_DESTROYING|set_destroying|stop_destroying|destroying_obj|next_destroying|destroying_list)\b' src
! rg -n '\b(flat_siege_|flat_town_)' src
! rg -n '^int (ballista|battering_ram|catapult|castlewall)\(' src/specs.prototypes.h
! rg -n '\b(kingdom_type_list|is_kingdom)\b' src src-migrate
! rg -n '^M [0-9]+ 4010(00|10|20|30|40|50|60|70|80|90)\b' areas/zon
! rg -n '^siege[[:space:]]+\*4010\b' areas/AREA
! rg -n '^#(160|161|178|179|461|462|463|464)\b' areas/obj/heavens.obj
! rg -n '^#(160|161|178|179)\b' areas_mini/mini.obj
test ! -e defaults/towns
test ! -e tests/async/test_flatfile_towns.py
test ! -e tests/async/flatfile_town_harness.cpp
test ! -e tests/async/test_flatfile_siege.py
test ! -e tests/async/flatfile_siege_harness.cpp
! rg -n 'test_flatfile_(towns|siege)\.py' .github/workflows/quality.yml
! rg -n 'helpkingdoms' scripts/import_help_to_prod.sh src/flatfile_help_catalog.c
```

The final repository will still contain allowed siege/kingdom strings in historical
migrations, lifecycle tombstones, legacy documentation, dated news, ordinary prose, Sea
Kingdom, and Kingdom of Torg. Review remaining matches rather than requiring global zero.

### Build and focused regressions

```bash
make -C src
make -C src-migrate
make world
python3 tests/async/test_season_reset_manifest.py
python3 tests/async/test_data_lifecycle_manifest.py
python3 tests/async/test_hometown_racewar_safety.py
python3 tests/async/test_minimal_boot.py
python3 tests/async/test_flatfile_full_world_boot.py
python3 tests/async/test_documentation_contract.py
python3 tests/async/test_immutable_migration_runner.py
```

Run the new siege-removal contract test directly, then run the complete regression suite:

```bash
make test
```

The removed `test_flatfile_towns.py` and `test_flatfile_siege.py` feature regressions must
not remain in the final matrix or CI. The generic flat-file and full-world boot tests stay.

The persistence-schema regression must continue to pass because the five compatibility
tables remain in the sealed baseline.

### Full-world runtime smoke test

After `make world`, perform a cold full-world development boot and verify:

- boot has no town/siege initialization line;
- zone 4010 and room 401000 are absent from generated runtime data;
- mob VNUMs 401000, 401001, and 401010 through 401090 are absent;
- the ten former hometown rooms do not contain quartermasters after reset;
- object VNUMs 160, 161, 178, 179, and 461 through 464 are absent only after their data
  compatibility gate is satisfied;
- `add` and `deploy` are not registered or listed;
- the command-array tombstones still occupy slots 827/828, while the ordinary `add` and
  `deploy` spellings resolve as unknown commands;
- `toggle kingdom` is not accepted and later toggle options still change the correct
  flags;
- `help add` and `help kingdoms` do not return the retired pages after help cleanup;
- `help toggle` does not show Kingdom View;
- an existing player with retired `act2 BIT_4` still loads, saves, and reconnects safely;
- ordinary ranged weapons, `reload`, `fire`, `push`, ships, surnames, guilds, hometown
  justice, Sea Kingdom, and Kingdom of Torg still work.

Also perform a cold `--minimal` development boot and verify VNUMs 160, 161, 178, and 179
are absent. Minimal success does not replace the full-world checks above.

### Persistence postflight

Verify on the deployment target:

- the five compatibility tables still exist;
- runtime logs show no query to those tables;
- the siege tables are classified `retain`, not reset state;
- stale `ADD` and `kingdoms` pages are absent;
- no new dedicated item VNUM appears after acquisition is frozen;
- no identity, migration-history, or runtime-compatibility fingerprint changed.

For a flat-file target or retained flat-file restore generation, also verify that
`metadata/towns` and `metadata/siege` are retired according to the approved policy, the
item-ownership authority has no active dedicated VNUM, and a cold boot does not recreate
either feature authority from a legacy path or seed.

## Risk register

| Risk | Consequence | Control |
| --- | --- | --- |
| Deleting only `siege.c/h` | Link failures and large dead combat state remain | Follow dependency order and contract test |
| Blind removal of `IS_DESTROYING` | Changed conditions or broken `else` ownership | Separate mechanical commit, format, compile, full tests |
| Renumbering command IDs | Special procedures receive wrong command numbers | Retire slots 827/828 in place |
| Shifting `act2` bits | Existing player flags change meaning | Reserve `BIT_4`; never reuse it |
| Deleting object prototypes with persisted rows | Silent item loss or load failures | Production custody gate and optional two-release removal |
| Auditing only direct VNUM columns | Legacy auction blobs retain loadable items | Decode unretrieved blobs with repository format and fail closed on corruption |
| Auditing only live MariaDB | Flat-file authorities or retained backups reintroduce removed state | Audit both persistence modes and all supported restore generations |
| Removing area definitions but not resets | World generation errors or missing-mob resets | Remove ten reset lines first and run `make world` |
| Updating only full-world area inputs | Tracked minimal mode still loads retired ammo prototypes | Remove gated copies from `areas_mini/mini.obj` and cold-boot both datasets |
| Editing generated `world.*` | Source/output drift | Change tracked area inputs only; regenerate ignored outputs |
| Removing source help only | Stale database pages remain live | Exact page postflight and targeted deletion |
| Editing historical migrations | Baseline adoption and checksums fail | Keep sealed schema history immutable |
| Dropping legacy tables now | Violates additive migration contract and fingerprints | Retain classified schema tombstones |
| Broad keyword cleanup | Breaks lore and unrelated live zones/systems | Use semantic inventory and explicit preserve list |
| Validating only one world dataset | Misses either minimal copies or full-world area/reset faults | Require minimal and full-world cold boots |

## Rollback strategy

Keeping the legacy tables, flag bit, command slots, and VNUM reservations makes source
rollback straightforward:

1. Revert the removal commits.
2. Restore tracked area inputs and regenerate `world.*`.
3. Restore the feature flat-file seed/authorities only with the matching pre-removal
   runtime and approved state-generation policy.
4. Restore help sources and re-import exact pages.
5. Restart from cold minimal and full-world boots.

No schema rollback is required because this project does not drop or rewrite the five
tables. If Release A retains object prototypes, it also preserves legacy item loadability
during the observation window.

## Acceptance criteria

The removal is complete only when all of the following are true:

- No siege or town-defense runtime entry point, translation unit, special procedure, or
  persistence function remains.
- No character object-destruction state or behavior-neutral destruction guard remains.
- No unfinished virtual troop, kingdom room, kingdom land-import, or map-display runtime
  scaffolding remains.
- Dedicated quartermasters, siege zone/room, shop sales, and gated object prototypes are
  absent from tracked full-world inputs, tracked minimal-world data, and regenerated
  full-world data.
- Feature-only flat-file town/siege code, authorities, tracked seed, regressions, and CI
  steps are removed without deleting shared flat-file infrastructure.
- `add`, `deploy`, `toggle kingdom`, `help add`, and `help kingdoms` no longer advertise
  or expose the feature.
- Persisted `act2 BIT_4`, command IDs 827/828, and retired VNUMs are not reused.
- Runtime pwipe and SQL code no longer access the feature tables.
- The five historical tables remain inventoried as retained compatibility schema; sealed
  migrations and schema fingerprints remain unchanged.
- Full build, area generation, focused persistence/lifecycle tests, complete regressions,
  and cold minimal/full-world smoke tests pass.
- Remaining siege/kingdom text has been reviewed and is explicitly unrelated lore,
  historical evidence, or compatibility metadata.

## Appendix A - Object-destruction cleanup file set

The current 62-file inventory is:

```text
src/actinf.c
src/actmove.c
src/actnew.c
src/actobj.c
src/actoff.c
src/actoth.c
src/actwiz.c
src/affects.c
src/auction_houses.c
src/bard.c
src/beh_magic.c
src/buildings.c
src/comm.c
src/db.c
src/disguise.c
src/drannak.c
src/ethermancer.c
src/ferryact.c
src/fight.c
src/grapple.c
src/group.c
src/handler.c
src/innates.c
src/interp.c
src/limits.c
src/magic.c
src/mining.c
src/mobact.c
src/mobcombat.c
src/new_skills.c
src/nexus_stones.c
src/prompt.c
src/prototypes.h
src/psionics.c
src/quest.c
src/range.c
src/siege.c
src/sillusionist.c
src/smagic.c
src/sparser.c
src/specials.c
src/specs.ailvio.c
src/specs.hoa.c
src/specs.lohrr.c
src/specs.mobile.c
src/specs.object.c
src/specs.ravenloft.c
src/specs.snogres.c
src/specs.undermountain.c
src/specs.underworld.c
src/specs.vecna.c
src/specs.venthix.c
src/specs.winterhaven.c
src/specs.zalrix.c
src/spells.c
src/structs.h
src/track.c
src/tradeskill.c
src/transport.c
src/trap.c
src/utility.c
src/utils.h
```

After deleting `src/siege.c`, 61 files remain in the mechanical compatibility sweep.

## Appendix B - Explicit preserve list

Do not remove the following merely because a name matches:

- Sea Kingdom area procedures and world content;
- Kingdom of Torg content and epic payout comments;
- ordinary uses of siege/kingdom in area descriptions, quests, credits, and dated news;
- `seige` misspellings in unrelated authored area prose unless handled by a separate
  content-editing task;
- generic missile, ranged weapon, `reload`, and `fire` systems;
- generic `push`, `buy`, `list`, and `donate` commands;
- active missile types 5 and 6, which are also used by non-siege sling, stone, wrist-bow,
  dart, and other ranged assets;
- scenic ballista/catapult objects at unrelated VNUMs;
- guild, association, surname, hometown, justice, and invader systems;
- sealed schema history and temporary legacy backup/wiper safeguards;
- generic flat-file persistence, item custody, authority transactions, and backup/restore
  infrastructure.
