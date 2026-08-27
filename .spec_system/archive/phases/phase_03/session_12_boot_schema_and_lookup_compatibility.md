# Session 12: Boot Schema and Lookup Compatibility

**Session ID**: `phase03-session12-boot-schema-and-lookup-compatibility`
**Status**: Complete
**Work Window**: One pre-service compatibility boundary from connection/session checks
through migration and schema manifest verification, versioned static lookup comparison,
transactional publication, worker startup, and fail-closed diagnostics.

---

## Objective

Prove the complete runtime database contract before boot mutates lookup rows, starts
workers, or accepts gameplay, then update race/class data atomically only when its
compiled version changes.

---

## Scope

### In Scope (MVP)

- Generate or maintain one authoritative runtime schema manifest covering required
  migration identity/checksums, tables, columns, types, nullability, defaults, primary
  and unique keys, critical indexes, foreign keys, engines, charset, and collation.
- Verify every main, read-worker, write-worker, maintenance, archive, and lifecycle
  connection contract for target role, `utf8mb4`, time zone, isolation level, SQL mode,
  TLS or protected local transport, and bounded connect/read/write deadlines.
- Move schema and connection preflight ahead of `sql_populate_lookup_tables()`, worker
  startup, recovery replay that writes SQL, and any listener/gameplay publication.
- Replace delete-then-row-by-row race/class population with a compiled lookup dataset
  version/checksum and one guarded InnoDB transaction that upserts changed rows and
  handles obsolete rows under verified reference rules.
- Compare the committed lookup version/checksum and avoid writes on unchanged boot;
  rollback fully on any validation or publication failure.
- Return stable redacted compatibility error IDs with expected/current migration and
  manifest identity but no credentials, SQL text, or bound data.
- Verify supported MySQL/MariaDB metadata differences and keep bootstrap, migration,
  runtime manifest, and documentation synchronized.

### Out of Scope

- Applying migrations automatically during normal server boot.
- Rebuilding unrelated static world tables or changing race/class gameplay definitions.
- Permitting a warning-only boot when a required contract is missing.

---

## Prerequisites

- [x] Session 11 immutable migration ledger and baseline identities are validated.
- [x] All Phase 03 schema/index/lifecycle changes are reflected in fresh bootstrap and
      migration verification.
- [x] Phase 00 connection trust-boundary controls remain authoritative.

---

## Deliverables

1. Authoritative required-schema and connection manifest plus read-only verifier shared
   by boot and isolated migration tests.
2. Reordered boot preflight that executes before database writes, worker startup, and
   gameplay availability.
3. Versioned/checksummed race/class dataset with transactional diff/upsert and unchanged-
   version no-op behavior.
4. Focused missing/drifted schema, checksum, engine, collation, SQL-mode, isolation,
   transport, timeout, unchanged lookup, failed lookup, rollback, and boot-order tests.

---

## Success Criteria

- [x] Boot validates the current migration identity and every required schema and
      connection invariant before its first database mutation.
- [x] A missing or incompatible table, column, type, key, engine, charset, collation, or
      connection setting aborts safely with a stable redacted reason.
- [x] Lookup rows are unchanged when the compiled dataset version/checksum matches.
- [x] A changed lookup dataset publishes atomically and readers can observe only the
      prior complete version or the new complete version, never an empty/partial table.
- [x] Failed lookup publication rolls back and cannot leave a falsely advanced dataset
      identity.
- [x] Fresh bootstrap, migrated clone, standalone verifier, and runtime boot agree on
      the same manifest and migration version on supported MySQL/MariaDB variants.
- [x] Focused regressions, isolated boot/schema tests, formatting checks, and
      `make -C src` pass.
