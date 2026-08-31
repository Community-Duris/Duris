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

The current immutable head is `0005_level_cap_singleton`. After it is applied, the
runtime schema contains 173 tables and the history singleton records applied
count 5 plus the exact history checksum. If a pre-b029 launcher already created
the legacy `server_reboots` shape, 0004 copies every lifecycle row into the
canonical table and atomically swaps it into place; an interrupted conversion
can be retried without making the legacy table unavailable or duplicating rows.

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
