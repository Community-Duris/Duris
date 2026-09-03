# Runtime Database Compatibility

The server refuses to mutate database state or publish gameplay until the configured
database proves the exact migration, schema, and connection contract. This is a
read-only compatibility gate, not an automatic migration mechanism.

## Required installation sequence

For a fresh development database, load the sealed 170-table Session 11 baseline,
adopt that exact fingerprint, and run the immutable migration head:

```sh
mysql "$DB_NAME" < migrations/bootstrap_multithread_safe.sql
python3 scripts/migration_runner.py adopt --kind fresh_bootstrap
python3 scripts/migration_runner.py run
./migrations/verify_runtime_compatibility.sh
```

The current head is `0008_statistics_date_index`, and the contract describes 174
current tables: the 170-table baseline plus `lookup_dataset_state`,
`season_reset_state`, `server_reboots` and `kingdom_realms`, each created by an
immutable migration. Migrations 0007 and 0008 change existing inventory tables,
so their fingerprints were resealed against `mysql:8.0` and `mariadb:10.11` with
`tests/async/run_runtime_compatibility_mysql.sh`. A database left at head
`0007_pkill_event_stamp_contract` therefore fails this gate. An existing database
must first
complete the guarded legacy upgrade and verified baseline adoption described in
[IMMUTABLE_MIGRATIONS.md](IMMUTABLE_MIGRATIONS.md). Never run migration or
destructive verification commands against production.

Compatibility fingerprints and table counts use the positive 174-table runtime
inventory. Additional tables restored from a combined game/website dump are ignored
by the game contract, while every runtime table still has to match exactly.

## Boot gate

`initialize_mysql()` opens the main connection through the shared trusted connection
constructor. Before any lookup write, item UID reservation, pool/worker startup,
recovery replay, listener acceptance, or gameplay publication, it verifies:

- the sealed baseline ID and table-name fingerprint;
- immutable migration ID, sequence, apply/verifier hashes, applied count, and history
  checksum;
- all 174 tables, InnoDB engine, and `utf8mb4_unicode_ci` collation;
- normalized table, column, default, index, and foreign-key metadata against the
  checked-in MySQL 8.0 or MariaDB 10.11 fingerprint;
- `utf8mb4`, UTC, READ COMMITTED, strict SQL modes, ten-second connection/read/write
  deadlines, exact target allow-listing, and verified TLS for remote hosts.

Failures abort boot with stable `COMPAT-E001`, `COMPAT-E002`, or `COMPAT-E003`
reason IDs. Messages identify only expected contract identities and never include
credentials, SQL text, or bound values.

## Race/class publication

The compiled race/class dataset is length-framed and SHA-256 checksummed. If both the
committed dataset state and a fresh checksum of live lookup rows match, boot performs
no lookup writes. Otherwise one InnoDB transaction upserts compiled rows, removes
obsolete IDs, recomputes the live checksum/counts, advances `lookup_dataset_state`
last, and commits. A statement or validation failure rolls back. A failed commit is
treated as ambiguous and also aborts boot with `COMPAT-E007`; the next boot revalidates
both state and live rows, so state cannot claim a version whose rows were not committed.

## Verification

```sh
python3 scripts/validate_runtime_compatibility.py
python3 tests/async/test_runtime_boot_compatibility.py
tests/async/run_lookup_dataset_mysql.sh
tests/async/run_runtime_compatibility_mysql.sh
RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh
```

The disposable full-schema tests prove a valid fresh schema and reject migration
history, missing-table, engine, collation, index, and column drift on both supported
variants. The standalone verifier is read-only and may be used against an explicitly
configured development clone before starting the server.
