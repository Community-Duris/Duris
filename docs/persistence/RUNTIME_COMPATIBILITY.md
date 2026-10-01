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

The migration manifest, compiled compatibility head, and runtime manifest now end
at `0051_quest_item_witness_lookup`, with 223 expected runtime tables.
Migration 0051 adds the nonunique `(reason_type, reason_id)` item-ledger index
used by bounded quest reward recovery. It preserves duplicate evidence and
refuses an existing index with a different shape. Both manifests append this
step without modifying older receipts or migration files.
Migration 0050 replaces the description-prefix unique indexes with indexes over
the complete description SHA-256. It preserves distinct long, case, and accent
variants and refuses duplicate complete values before permanent DDL; it never
deletes those conflicting rows. The metadata fingerprints were measured on
disposable MySQL 8.0.46 and MariaDB 10.11 schemas using the real immutable runner.
Completed canonical and staging histories reach identical schema metadata on
each engine. Migration 0042 records
a nullable keeper roaming policy; legacy rows remain unknown until a
shopkeeper checkpoint. Migration 0043 records nullable item condition, and
migration 0044 preserves dynamic properties while an item is held by a
shopkeeper. Migration 0029 adds
the replay-safe `critical_operation_inbox.failure_stage` receipt field as
`SMALLINT UNSIGNED NOT
NULL DEFAULT 0` immediately after `result_code`; it creates no table. A legacy clone
may already contain that column from the earlier compatibility DDL. The immutable
step is guarded and re-runnable: it verifies the existing shape, preserves all rows,
and records sequence 29 rather than trying to alter the old immutable history.
Migration 0030 adds the protected `telemetry_quarantine` table used to isolate and
replay record-specific telemetry storage failures without blocking the stream.
Migration 0041 adds nullable shopkeeper cash and a stable shop revision without
adding a table. Its verifier accepts the equivalent MySQL 8.0 and MariaDB 10.11
column metadata forms.
The verifier correction changes migration 0041's recorded checksum. A database
that already recorded the earlier checksum fails closed and needs an explicit
clone-based reconciliation before it can use this contract.
Future fingerprints must be measured on clean `mysql:8.0` and `mariadb:10.11`
schemas with the runtime verifier; they must not be copied from a
production-derived clone.

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

## Staging's immutable 0045 fork

Staging applied `0045_item_extra_description_fulltext_unique` before the canonical
quest migration. Its first 45 receipts and the original checksummed migration
files must stay intact. On a verified, isolated staging clone, select the explicit
manifest when running the existing migration runner:

```sh
python3 scripts/migration_runner.py \
  --manifest migrations/migration_manifest.staging_0045.json run
./migrations/verify_runtime_compatibility.sh --schema-only
```

| Staging sequence | Immutable migration ID |
| --- | --- |
| 45, already applied | `0045_item_extra_description_fulltext_unique` |
| 46 | `0045_quest_reward_obligation` |
| 47 | `0046_economic_realized_trade_price` |
| 48 | `0047_quest_xp_receipt` |
| 49 | `0048_quest_xp_entitlement` |
| 50 | `0049_player_spell_effect_receipt` |
| 51 | `0051_quest_item_witness_lookup` |

The canonical manifest continues to reject this fork before any migration runs.
The explicit manifest appends six steps and produces a different
history checksum from the canonical 51-step history. Both completed checksums
are compiled into the boot gate; every historical row is recomputed and matched
to its stored state. Partial histories and mixed head/state identities fail.

The maintained staging qualification supports Docker by default and an explicit
native mode for a caller-owned disposable MySQL 8.0 or MariaDB 10.11 server:

```sh
# Supply explicit DB_USER, DB_PASSWD, and DB_PORT for the isolated test server.
TEST_DB_DISPOSABLE=1 STAGING_FORK_DISPOSABLE_SERVER=1 DB_HOST=127.0.0.1 \
  python3 tests/async/test_staging_migration_fork_mysql.py \
  --disposable-loopback mysql8
# Repeat with --disposable-loopback mariadb10_11 on that engine's test server.
```

This mode creates uniquely named `duris_268_*test` databases and private
temporary verifier files. The caller owns stopping and discarding the server.
Socket connections and unguarded targets are refused. On 2026-10-01, both
native engines preserved all first 45 staging receipts, appended six receipts,
converged on the canonical schema, and passed shell/compiled boot checks plus
history, state, expression, advisory-lock, and connection-loss fault checks.

These commands are clone preparation instructions. A production-role staging
environment still requires the runner's target allow-list, fresh verified backup,
quiescence checks, and explicit operational authorization. This does not change
accounting activation or resolve ownership holds.

## Boot gate

`initialize_mysql()` opens the main connection through the shared trusted connection
constructor. Before any lookup write, item UID reservation, pool/worker startup,
recovery replay, listener acceptance, or gameplay publication, it verifies:

- the sealed baseline ID and table-name fingerprint;
- the exact completed canonical or staging-fork history, including all seven
  immutable receipt fields in sequence order, and its matching stored count and
  checksum; checking only the last row or the stored digest is insufficient;
- all 223 tables, InnoDB engine, and `utf8mb4_unicode_ci` collation;
- normalized table, column, default, index, and foreign-key metadata against the
  checked-in MySQL 8.0 or MariaDB 10.11 fingerprint;
- the exact normalized stored SHA-256 expressions on player and pet descriptions;
  the generic generated-column flag alone cannot prove those expressions;
- `utf8mb4`, UTC, READ COMMITTED, strict SQL modes, ten-second connection/read/write
  deadlines, exact target allow-listing, and verified TLS for remote hosts.

Failures abort boot with stable compatibility reason IDs. `COMPAT-E008` identifies
full-history/state disagreement; `COMPAT-E009` identifies description-expression
drift. Existing metadata failures retain `COMPAT-E001`, `COMPAT-E002`, and `COMPAT-E003`
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

The immutable migration runner retains one MySQL client connection for the
entire run. Its advisory lock, quiescence check, DDL, receipt insertion, and
history-state update use that connection. Automatic reconnect is disabled.
A receipt and its compare-and-swap state update commit together; a failed
comparison rolls back the receipt. Connection loss, SQL failure, timeout,
oversized output, allocation failure, or cancellation closes and fences the
session. A caller must start a new executor and revalidate history before
continuing; the failed executor cannot silently reacquire a connection.

The native migration-session fixture verifies lock exclusion, session identity,
receipt rollback, connection loss during an open transaction, and fencing after
SQL, output-bound, timeout, allocation, and cancellation failures on disposable
MySQL 8.0.46 and MariaDB 10.11.19 targets. Run just these fault cases with:

```sh
python3 tests/async/test_staging_migration_fork_mysql.py --lock-only
```

Omit `--lock-only` to also qualify both complete migration histories, immutable
prefix preservation, duplicate refusal, and shell/compiled boot drift checks.
The database restore qualifier also accepts exactly either completed history.
It refuses partial, mixed, edited, or extended histories before value-domain
qualification, and closes its client session on success or failure. The native
fixture checks this selector against the same actual rows as the compiled boot
predicate, including each history tamper and stored-state mismatch.

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
