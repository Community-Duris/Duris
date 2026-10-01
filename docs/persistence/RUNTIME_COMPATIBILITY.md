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

The current head is `0030_telemetry_quarantine`, and the contract describes 203
current tables: the 170-table baseline plus the post-baseline runtime tables created
by immutable migrations. Migration 0029 adds the replay-safe
`critical_operation_inbox.failure_stage` receipt field as `SMALLINT UNSIGNED NOT
NULL DEFAULT 0` immediately after `result_code`; it creates no table. A legacy clone
may already contain that column from the earlier compatibility DDL. The immutable
step is guarded and re-runnable: it verifies the existing shape, preserves all rows,
and records sequence 29 rather than trying to alter the old immutable history.
Migration 0030 adds the protected `telemetry_quarantine` table used to isolate and
replay record-specific telemetry storage failures without blocking the stream.
Fingerprints are measured on clean `mysql:8.0` and `mariadb:10.11` schemas with
`tests/async/telemetry_rollup_schema_mysql.py --update-contract`; they must not be
copied from a production-derived clone.

An existing populated database must first be upgraded only on a disposable clone.
Run the legacy convergence, then the immutable runner against the same clone before
running the read-only runtime verifier:

```sh
MIGRATION_ENV_FILE=/path/to/clone.env ./migrations/run_migration.sh
# clone.env is owner-readable, mode 0600, and still targets only the clone.
set -a; . /path/to/clone.env; set +a
python3 scripts/migration_runner.py run
./migrations/verify_runtime_compatibility.sh
```

If the clone already has `failure_stage`, migration 0029 takes its no-op branch and
still records the checksummed migration. If it does not, the migration adds the
column with default zero; existing receipt rows remain present and at stage zero.
Rebuild the native server after the checked-in compatibility header changes (`make -C
src`, or the approved clean production build) and stage that rebuilt binary before
any boot attempt. Never treat a successful SQL migration as proof that an old
binary is compatible.

The legacy upgrade remains guarded and additive; a database left at head
`0029_critical_failure_stage` must not be booted with this contract. An existing database must
first complete the clone sequence above. Never run migration or destructive
verification commands against production.

## Boot gate

`initialize_mysql()` opens the main connection through the shared trusted connection
constructor. Before any lookup write, item UID reservation, pool/worker startup,
recovery replay, listener acceptance, or gameplay publication, it verifies:

- the sealed baseline ID and table-name fingerprint;
- immutable migration ID, sequence, apply/verifier hashes, applied count, and history
  checksum;
- all 203 tables, InnoDB engine, and `utf8mb4_unicode_ci` collation;
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
