# Legacy Dump Import Guide

This is the handoff document for importing an old Duris MySQL dump into the current
development schema. It explains what is replaced, what is preserved verbatim, what is
normalized for the current server, where displaced source rows live, and which files own
each contract.

The short version is:

- the importer replaces one explicitly allow-listed local/development database; it is not
  a row-by-row merge tool;
- every source table must still exist after migration;
- unrelated website and administration tables may coexist with the game schema and keep
  their data;
- a source row removed from a canonical game table by required uniqueness cleanup must
  first be retained in a `legacy_import_*` archive;
- source values that the current runtime schema cannot represent are retained in a raw
  archive before a compatible runtime projection is produced; and
- the game still requires an exact 173-table positive runtime contract. Extra imported
  tables do not weaken that check.

Never point this process at production. The importer deliberately accepts only a loopback,
non-production target whose exact `host/database` pair appears in `DB_ALLOWED_TARGETS`.

## Vocabulary

| Term | Meaning |
| --- | --- |
| Source table | A base table restored directly from the supplied dump. |
| Runtime table | One of the 173 canonical game tables required by the current server. |
| Extension table | A source table used by the website, administration tools, or an older subsystem, but not owned by the game runtime contract. |
| Preservation archive | A `legacy_import_*` table containing source rows that cannot remain verbatim in a canonical runtime table. |
| Legacy migration | The additive 143-step upgrade in `migrations/run_migration.sh`. |
| Immutable migration | A checksummed post-baseline migration in `migrations/immutable/`. |

## End-to-end flow

| Stage | What happens | Failure behavior |
| --- | --- | --- |
| 1. Validate | Check the environment file, dump, target, permissions, and explicit `--replace` acknowledgment. | Stop before touching the database. |
| 2. Quiesce check | Refuse the import if another connection is using the target database. | Stop before backup or replacement. |
| 3. Backup | Write an owner-only `mysqldump` of the current target, including routines, events, and triggers. | Stop before replacement. |
| 4. Replace | Drop target views/tables and stream the source dump into the same database. | Restore the backup when the failure is caught by the importer. |
| 5. Converge | Run the legacy migration, adopt/advance the immutable ledger, and reach migration head `0006_kingdom_realms`. | Restore the backup when the failure is caught by the importer. |
| 6. Verify runtime | Check the migration ledger and the exact metadata of all 173 runtime tables for MySQL 8 or MariaDB 10.11. | Restore the backup. |
| 7. Verify preservation | Require all source tables, reject unexplained row loss, validate known archives, and require extension-table row counts to remain equal. | Restore the backup. |

The implementation entrypoint is
[`scripts/import_legacy_dump.py`](../../scripts/import_legacy_dump.py). The commands it runs,
in order, are:

```text
migrations/run_migration.sh
scripts/migration_runner.py run
migrations/verify_runtime_compatibility.sh
```

## Safety gates

The importer fails closed unless all of these are true:

- `--replace` was supplied;
- the environment file and dump are regular, non-symlink files with mode `0600` or
  stricter;
- `ENVIRONMENT` is `local`, `development`, `dev`, or `test`;
- `DB_HOST` is `127.0.0.1`, `localhost`, or `::1`;
- `DB_NAME` is a safe identifier and does not look like a production name;
- `DB_ALLOWED_TARGETS` contains the exact `DB_HOST/DB_NAME` pair;
- the database port is valid and any configured socket path is absolute;
- the dump begins with a recognized `mysqldump` header;
- the dump does not contain a leading `CREATE DATABASE`, `DROP DATABASE`, or `USE`
  directive that could redirect the import; and
- no other connection is using the target database.

The default backup location is:

```text
tmp/legacy-import-backups/<database>-<UTC timestamp>.sql
```

Both the backup directory and backup file are owner-only. The importer prints the exact
backup path and source SHA-256 on success.

## Dump portability changes

The source dump is streamed; it is not rewritten on disk. On non-`INSERT` lines only,
the importer makes two compatibility substitutions:

| Source metadata | Imported metadata | Reason |
| --- | --- | --- |
| `utf8mb4_0900_ai_ci` | `utf8mb4_unicode_ci` | MariaDB 10.11 does not provide MySQL 8's `0900` collation. |
| A dump-owned ``DEFINER=`user`@`host` `` | `DEFINER=CURRENT_USER` | A historical view definer may not exist on the target. |

`INSERT INTO` lines are passed through byte-for-byte, so a string value that happens to
contain either token is not changed. The legacy migration later converts existing base
tables to the configured database character set/collation and normalizes canonical table
metadata. Extension-table data is preserved, but its storage engine or collation may be
modernized by that legacy migration.

## Source-to-destination rules

### Default rule

Every source base table remains under its source name. The importer rejects a missing
source table after migration. A source row-count reduction is rejected unless that table
has an explicit archive rule below.

For extension tables, the final row count must exactly equal the restored source row
count. Extension tables are deliberately excluded from runtime metadata fingerprints;
they are preserved data, not newly adopted game-server dependencies.

The source `players_view` view is restored/recreated separately and is not counted as a
base table.

### Explicit preservation and projection rules

| Source | Trigger | Canonical result | Preservation destination |
| --- | --- | --- | --- |
| `account_characters` | More than one row has the same `char_name`. | Keep the lowest-`pid` row required by the unique character-name contract. | Only the discarded higher-`pid` rows are copied to `legacy_import_account_characters`. |
| `player_item_extra_descr` | Duplicate `(item_id, keyword, LEFT(description, 255))`. | Keep the lowest-`id` row and add `uk_item_descr`. | Copy the complete source table to `legacy_import_player_item_extra_descr` before cleanup. |
| `player_pet_item_extra_descr` | Duplicate `(item_id, keyword, LEFT(description, 255))`. | Keep the lowest-`id` row and add `uk_pet_item_descr`. | Copy the complete source table to `legacy_import_player_pet_item_extra_descr` before cleanup. |
| `player_item_affects` | Duplicate `(item_id, location, modifier)`. | Keep the lowest-`id` row and add `uk_item_affect`. | Copy the complete source table to `legacy_import_player_item_affects` before cleanup. |
| `player_pet_item_affects` | Duplicate `(item_id, location, modifier)`. | Keep the lowest-`id` row and add `uk_pet_item_affect`. | Copy the complete source table to `legacy_import_player_pet_item_affects` before cleanup. |
| Legacy `server_reboots` with an `id` column | The launcher-era table predates immutable migration 0004. | Convert to the canonical `record_id` table and supported lifecycle values. | Copy every raw source row to `legacy_import_server_reboots`, using `source_id` for the old `id`. |
| `account_banks` plus bank values derivable from player rows | A bank row already exists for `(account_name, racewar)`. | The imported `account_banks` row remains authoritative; derived rows fill only missing keys via `INSERT IGNORE`. | No separate archive is needed because the imported row is not overwritten. |

The four item-metadata archives are created only when duplicates exist. They contain the
complete source table, not merely the discarded rows, so a developer can reconstruct the
exact pre-deduplication state. The account-character archive is different: it contains
only rows that the uniqueness cleanup would discard.

### `server_reboots` projection

The raw archive retains `source_id`, `boot_time`, nullable `shutdown_time`, nullable
`uptime_seconds`, the original `shutdown_type`, `initiated_by`, `reason`, and `created_at`.
The runtime projection then applies these rules:

- old `id` becomes canonical `record_id`;
- negative epoch/duration values are clamped to zero;
- a null `shutdown_time` becomes the non-negative `boot_time`;
- a null `uptime_seconds` is derived from shutdown minus boot time, or becomes zero;
- supported types remain unchanged: `shutdown`, `reboot`, `copyover`, `autoreboot`,
  `pwipe`, `hung`, `autoreboot_copyover`, `crash`, and `unknown`;
- any other type becomes `unknown` in the runtime table; and
- legacy `created_at` has no canonical runtime column and therefore remains in the raw
  archive only.

The archive is the source of truth when investigating a pre-import lifecycle value. The
canonical table is the source of truth for current server operation.

## Runtime versus extension ownership

The current schema contract has two layers:

1. The Session 11 baseline requires a positive inventory of 170 canonical tables.
2. Immutable migrations add `lookup_dataset_state`, `season_reset_state`, and
   `server_reboots`, yielding 173 runtime tables. The head is
   `0006_kingdom_realms`; the count stays at 173 because the `kingdom_realms`
   table that 0006 creates is deliberately outside the runtime table inventory
   until the maintainers reseal the normalized metadata fingerprints on live
   MySQL 8 and MariaDB 10.11. The head's ledger identity is enforced at boot
   even so, so convergence must still reach 0006.

The baseline and runtime checks ask whether every required table and its expected metadata
is present. They do not require unrelated tables to be absent. This distinction lets a
combined game/website dump survive while missing, renamed, or modified game tables still
prevent migration adoption and server boot.

The authoritative inventories are:

- baseline tables: [`migrations/migration_manifest.json`](../../migrations/migration_manifest.json);
- all runtime tables: [`migrations/data_lifecycle_manifest.json`](../../migrations/data_lifecycle_manifest.json);
- normalized runtime metadata and migration head:
  [`migrations/runtime_compatibility_manifest.json`](../../migrations/runtime_compatibility_manifest.json); and
- the compiled boot contract:
  [`src/core/runtime_compatibility_contract.h`](../../src/core/runtime_compatibility_contract.h).

[`scripts/validate_runtime_compatibility.py`](../../scripts/validate_runtime_compatibility.py)
cross-checks those inventories so they cannot drift independently.

## Reference import: `prod.sql`, 2026-08-31

This section records the completed import that established this process. Counts describe
the database immediately after migration and before normal server activity added new
lifecycle rows.

| Audit item | Result |
| --- | --- |
| Source file | `tmp/session14-gate/prod.sql` |
| Source/target engines | MySQL 8 dump imported into local MariaDB 10.11 `duris_dev` |
| Source size | 345,366,975 bytes |
| Source SHA-256 | `8f882d16be743b42467890f7714052d64b1cf8eca75be89c77efaeb3d7e5cc44` |
| Source inventory | 190 base tables, one view, 1,757,000 rows |
| Source/runtime overlap | 112 tables |
| Source extension inventory | 78 tables |
| Immediate post-migration inventory | 254 base tables, 1,773,767 rows |
| Runtime contract | 173 tables; migration count 5 at `0005_level_cap_singleton`, the head on the run date. The current head is `0006_kingdom_realms` at count 6, still 173 runtime tables; a repeat of this import today must converge to it. |
| Added preservation archives | Three: two item-description archives and the reboot archive |
| Missing source tables | None |
| Unexpected source row reductions | None |
| Extension preservation | All 78 tables had equal row counts and equal database `CHECKSUM TABLE` results against an exact normalized raw restore |
| Account-bank comparison | All 120 rows matched, including timestamps compared as Unix epochs |

The run retained owner-only pre-import backups named
`duris_dev-20260831T044313Z.sql` and `duris_dev-20260831T044859Z.sql` under the
default backup directory. These are ignored local recovery artifacts, not repository
assets; a future operator must use the backup path printed by their own run.

Only two canonical source tables lost duplicate rows, and their complete source contents
remain archived:

| Table | Source rows | Canonical rows | Archived source rows |
| --- | ---: | ---: | ---: |
| `player_item_extra_descr` | 289,015 | 16,072 | 289,015 |
| `player_pet_item_extra_descr` | 441 | 36 | 441 |

The source had 648 `server_reboots` rows. All 648 were present in both the initial
canonical projection and `legacy_import_server_reboots`. The archive retained nine rows
with null shutdown fields and 14 rows using the legacy `web_stop`/`web_restart` types.
Those values were normalized only in the canonical projection. After the service was
restarted during verification, the canonical table contained 652 rows because four new
runtime lifecycle records had been written; the archive correctly remained at 648.

No duplicate item-affect or account-character cleanup reduced rows in this reference
import, so their conditional archive tables were not needed.

The final code and imported database passed the focused Python tests, full legacy
migration/replay/bootstrap-equivalence tests on MySQL 8 and MariaDB 10.11, runtime
compatibility tests on both engines, the C++20 server build, the standalone verifier,
and a live service health check reporting persistence `ready`.

### Preserved extension-table inventory

These 78 source tables were retained outside the game runtime contract. Names are listed
exactly as imported; in particular, legacy `prepstatment_duris_sql` is distinct from the
canonical `prepstatement_duris_sql` table.

```text
account_login_history
admin_account_permissions
admin_account_roles
admin_action_log
admin_permission_audit
admin_permissions
admin_role_permissions
admin_roles
builder_activity_log
builder_flags
builder_mentions
builder_notifications
builder_proc_requests
builder_zone_comments
builder_zone_info
builder_zone_info_history
builder_zone_permissions
deployment_log
donations
forum_categories
forum_category_permissions
forum_mentions
forum_moderation_log
forum_notifications
forum_permission_audit
forum_poll_options
forum_poll_vote_history
forum_poll_votes
forum_polls
forum_post_images
forum_posts
forum_reactions
forum_settings
forum_subscriptions
forum_threads
gemini_analysis_log
help_file_suggestions
knex_migrations
knex_migrations_lock
mud_backups
mud_control_log
mud_process_state
mud_restores
notifications
page_views
players_core
prepstatment_duris_sql
push_subscriptions
pvp_battle_comments
pvp_battle_favorites
pvp_battle_likes
server_health_metrics
server_incidents
siege_objects
suspicious_accounts
terminal_logs
terminal_sessions
user_bans
user_profile_stats
user_profiles
visitor_sessions
web_sessions
web_settings
website_changelog
website_changelog_reads
wiki_continents
wiki_map_positions
wiki_mob_flags
wiki_mobs
wiki_object_affects
wiki_object_classes
wiki_object_races
wiki_object_slots
wiki_object_spell_effects
wiki_objects
wiki_settings
wiki_zone_entrances
wipe_history
```

## Operator runbook

### 1. Confirm the target

Review `.env` without copying credentials into logs or documentation. At minimum, the
configuration must describe a local target and explicitly allow it:

```dotenv
ENVIRONMENT=local
DB_HOST=127.0.0.1
DB_NAME=duris_dev
DB_ALLOWED_TARGETS=127.0.0.1/duris_dev
```

The file must also contain `DB_USER` and `DB_PASSWD`. Keep `.env` at mode `0600` and do
not commit it.

### 2. Quiesce the database

Stop the game and any website/job process connected to the target. For the local user
service used by this repository:

```bash
systemctl --user stop duris-mud.service
systemctl --user is-active duris-mud.service
```

The second command should report `inactive`. The importer independently checks live
database connections and refuses to proceed if another one remains.

### 3. Protect and identify the dump

```bash
chmod 600 /path/to/legacy.sql
sha256sum /path/to/legacy.sql
```

Record the checksum in the migration ticket or handoff notes. Do not commit the dump.

### 4. Run the guarded replacement

```bash
python3 scripts/import_legacy_dump.py \
  --env-file .env \
  --replace \
  /path/to/legacy.sql
```

Use `--backup-dir /private/path` when the default ignored repository directory is not an
appropriate backup destination. A successful final line reports source table/row counts,
the dump checksum, and backup path.

### 5. Verify before exposing gameplay

```bash
python3 scripts/validate_runtime_compatibility.py
./migrations/verify_runtime_compatibility.sh
make -C src
systemctl --user start duris-mud.service
./scripts/healthcheck.sh
```

The health endpoint is acceptable only when its payload is:

```json
{"status":"healthy","persistence":"ready"}
```

Keep the pre-import backup until the migrated system has passed the required gameplay and
website acceptance checks.

## What the automatic proof does and does not establish

The importer automatically proves:

- every source base table remains present;
- no source table has fewer rows unless it uses a known archive rule;
- required deduplication archives contain enough rows to reconstruct the source;
- each source extension table has the same row count after migration;
- the immutable ledger is complete and internally checksummed; and
- every runtime table matches the expected engine, collation, columns, defaults, indexes,
  and foreign keys for the detected database engine.

The importer does not perform a cryptographic value-level comparison of every canonical
row because canonical migrations intentionally transform some schemas and values. For a
high-risk import, repeat the stronger reference audit in a disposable database: restore
the dump without running migrations, compare every extension table with `CHECKSUM TABLE`,
and perform domain-specific canonical comparisons such as account-bank rows and archive
counts. Never weaken the canonical verifier merely to make an old dump pass.

Normal runtime activity changes counts after startup. In particular, `server_reboots`,
logs, audit tables, and lookup publication state may no longer match the immediate
post-import snapshot.

## Failure and recovery

If a normal import, migration, or verification command fails after replacement begins,
the importer drops the partial target objects and restores the pre-import backup. It
reports the original failure after recovery.

If automatic restore also fails, the error prints the exact backup path. Keep the game
stopped, preserve both the source dump and backup, and restore only after reconfirming the
target database. Do not improvise a merge into the partial database.

An external interruption such as process termination or host failure may bypass the
normal caught-error recovery path. Treat the target as partial until the pre-import backup
has been restored and the runtime verifier passes. The backup is the old target state; it
is not a second copy of the incoming legacy dump.

## Troubleshooting decisions

| Symptom | Correct response |
| --- | --- |
| Target is rejected | Fix `ENVIRONMENT`, loopback host, database name, or the exact `DB_ALLOWED_TARGETS` entry. Do not relax the check. |
| Active connections are reported | Stop the game, website, workers, and SQL sessions using that database, then retry from the beginning. |
| A source table is missing | Treat it as import/migration failure and restore. Do not add it to an ignore list. |
| An extension row count changed | Identify the migration touching that table. Preserve the source data or narrow the migration; do not classify it as runtime merely to bypass the check. |
| A new deduplication is required | Add a deliberate `legacy_import_*` archive before cleanup and teach the importer how to verify it. |
| A legacy value cannot fit the runtime schema | Preserve the raw row first, then define and test an explicit canonical projection. |
| Runtime metadata verification fails | Compare the live runtime table against the appropriate MySQL 8/MariaDB 10.11 contract. Extra extension tables are not the cause. |
| Import was interrupted | Assume partial state and recover from the named backup before restarting services. |

## Contract ownership and change checklist

| Concern | Authoritative file |
| --- | --- |
| Import validation, backup, replacement, rollback, and preservation checks | [`scripts/import_legacy_dump.py`](../../scripts/import_legacy_dump.py) |
| Additive old-schema upgrade and data derivation | [`migrations/run_migration.sh`](../../migrations/run_migration.sh) |
| Legacy metadata normalization and raw archives | [`migrations/legacy_schema_convergence.sql`](../../migrations/legacy_schema_convergence.sql) |
| Baseline inventory and immutable history | [`migrations/migration_manifest.json`](../../migrations/migration_manifest.json) |
| Immutable application and adoption logic | [`scripts/migration_runner.py`](../../scripts/migration_runner.py) |
| Immutable SQL/verifiers | [`migrations/immutable/`](../../migrations/immutable/) |
| Runtime table inventory | [`migrations/data_lifecycle_manifest.json`](../../migrations/data_lifecycle_manifest.json) |
| Runtime metadata/head contract | [`migrations/runtime_compatibility_manifest.json`](../../migrations/runtime_compatibility_manifest.json) |
| Standalone runtime verifier | [`migrations/verify_runtime_compatibility.sh`](../../migrations/verify_runtime_compatibility.sh) |
| Compiled server boot gate | [`src/core/runtime_compatibility_contract.h`](../../src/core/runtime_compatibility_contract.h) and [`src/sql/sql.c`](../../src/sql/sql.c) |
| Import regressions | [`tests/async/test_legacy_dump_import.py`](../../tests/async/test_legacy_dump_import.py) |
| Cross-engine legacy convergence | [`tests/async/run_legacy_migration_mysql.sh`](../../tests/async/run_legacy_migration_mysql.sh) |
| Cross-engine runtime contract | [`tests/async/run_runtime_compatibility_mysql.sh`](../../tests/async/run_runtime_compatibility_mysql.sh) |

When behavior changes, update this guide only after the owning code and focused regression
test agree. Then run:

```bash
python3 tests/async/test_legacy_dump_import.py
python3 tests/async/test_immutable_migration_runner.py
python3 tests/async/test_runtime_boot_compatibility.py
python3 scripts/validate_runtime_compatibility.py
tests/async/run_legacy_migration_mysql.sh
LEGACY_DB_IMAGE=mariadb:10.11 tests/async/run_legacy_migration_mysql.sh
tests/async/run_runtime_compatibility_mysql.sh
RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh
```

For the migration ledger and server boot-gate rationale, continue with
[`IMMUTABLE_MIGRATIONS.md`](IMMUTABLE_MIGRATIONS.md) and
[`RUNTIME_COMPATIBILITY.md`](RUNTIME_COMPATIBILITY.md).
