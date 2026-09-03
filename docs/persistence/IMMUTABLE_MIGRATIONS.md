# Immutable Migration Ledger

Duris has two deliberately separate histories:

- `migrations/run_migration.sh` is the legacy additive upgrade path. Its 143 progress
  steps and `mud_schema_migrations` data-copy markers are not complete historical
  execution evidence and are never backfilled as if they were.
- `migrations/migration_manifest.json` is the authoritative immutable history after
  the Session 11 baseline. Every post-baseline step receives one ordered ID, apply file,
  verifier, compatibility label, and exact SHA-256 hashes.

## Honest baseline adoption

The checked-in baseline identifies the exact 170-table authoritative contract by a
sealed positive table inventory and sorted-name fingerprint. A fresh bootstrap and a
fully completed legacy upgrade must contain that complete inventory. Unrelated tables
from a combined game/website dump may coexist without weakening the canonical check.
Only then may an operator record either
`fresh_bootstrap` or `verified_legacy_adoption` in `mud_schema_baselines`.

The legacy runner creates the ledger and attempts verified adoption only after its last
schema operation. Extra, missing, or renamed tables refuse adoption. This creates one
honest observation at the current boundary; it does not invent timestamps or checksums
for historical steps.

For a fresh isolated database:

```sh
mysql "$DB_NAME" < migrations/bootstrap_multithread_safe.sql
python3 scripts/migration_runner.py inspect
python3 scripts/migration_runner.py adopt --kind fresh_bootstrap
python3 scripts/migration_runner.py run
```

The runner requires `ENVIRONMENT` to be local/development/test, a loopback `DB_HOST`,
a non-production database name, and explicit credentials. Never point it at production.

The current immutable head is `0006_kingdom_realms`. After it is applied, the
database contains the 174-table runtime boot contract, and the history singleton
records applied count 6 plus the exact history
checksum. If a pre-b029 launcher already created the legacy `server_reboots`
shape, 0004 copies every lifecycle row into the canonical table and atomically
swaps it into place; an interrupted conversion can be retried without making the
legacy table unavailable or duplicating rows. 0006 creates the guild kingdom
realm table with a guarded `CREATE TABLE IF NOT EXISTS` and then converges a
database that already ran the deleted pre-registration
`migrations/kingdom_realms.sql`: that file carried no `COLLATE` clause, so its
table holds the character set's default collation instead of
`utf8mb4_unicode_ci`, and a guarded `CONVERT TO CHARACTER SET utf8mb4 COLLATE
utf8mb4_unicode_ci` brings it to the verified shape. Every column is an integer,
so the conversion changes no stored value, and on an already-correct table the
guard issues no `ALTER` at all, which is what keeps 0006 exactly re-runnable.

`kingdom_realms` is part of the boot contract's *table list*:
`runtime_compatibility_manifest.json` counts 174 runtime tables and both
normalized metadata fingerprints are sealed over an inventory that includes it,
so on the database backend the gate proves the table's engine, collation, columns
and indexes before gameplay publishes. `kingdom_initialize()` still disables
kingdoms for the boot when it cannot read the table, which remains reachable on
the flat-file build, where no boot gate stands in front of it. The *ledger* is
fail-closed too, exactly as it is for every other immutable
migration: `src/core/runtime_compatibility_contract.h` compiles
`RUNTIME_MIGRATION_HEAD_ID = "0006_kingdom_realms"` with sequence 6, and
`sql_verify_boot_database()` in `src/sql/sql.c` requires the matching
`mud_schema_history` row, its two checksums, and `applied_count=6` in
`mud_schema_migration_state`. On the MariaDB/MySQL backend a database left at
head `0005_level_cap_singleton` therefore refuses to boot, aborting with
`COMPAT-E002`. An operator upgrading an existing database must apply 0006 with
`python3 scripts/migration_runner.py run`, which applies the SQL, runs the
verifier, and only then writes the history row and advances the head, before
starting the server.

## Post-baseline migration contract

Files live under `migrations/immutable/` and are listed explicitly in the manifest.
Before opening a database, the runner reads every file with no-follow and fixed-size
limits and rejects duplicate JSON keys, invalid paths, missing/duplicate/reordered IDs,
unknown fields, and checksum changes.

At runtime it acquires one database lock, verifies the adopted baseline, validates the
entire applied prefix, and checks the singleton history count/head checksum. For each
pending migration it runs apply, then verifier, and only then atomically inserts the
history row and advances the history head. A verifier failure leaves the step
unrecorded. The migration itself must therefore be additive and re-runnable so an exact
retry can finish partial MySQL DDL safely.

Editing or reordering an applied row fails against the manifest. Deleting even the
trailing row fails against the retained history count/head rather than silently
reclassifying it as pending. Exact replay with a complete prefix performs no work.

## Verification

```sh
python3 tests/async/test_immutable_migration_runner.py
tests/async/run_legacy_migration_mysql.sh
tests/async/run_immutable_migration_ledger_mysql.sh
tests/async/run_runtime_compatibility_mysql.sh
RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh
```

The isolated MySQL tests verify the full legacy upgrade, exact fresh-bootstrap
equivalence, replay, schema compatibility, ordered uniqueness, honest baseline kind
uniqueness, preservation of legacy data-copy markers, and record-preserving
convergence of the pre-b029 `server_reboots` table on both supported engines.
