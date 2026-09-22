# SQL accounting storage increment

Status: storage and identity-lock component on `codex/474-phase3-sql-storage`, based on guarded
intent PR #600. This is a partial delivery for #477, not an accounting activation
or completion claim. The source is the preserved `bb8c1e5c5` schema and
`0dc1079fd` identity-lock implementation, adapted to canonical master
`48c0aedd8e094eee37285111e46e735e4cf12320`.

## Storage and registration

The nine-table schema retains epochs, active lineage state, non-reused account
mappings, operation records, account effects, coin posting lines, child links,
item-ledger references, and source-event deduplication. The item reference uses
an additive unique index on the existing ledger so its composite foreign key
binds the exact legacy event, UID and revision. Existing balance/custody stores
remain authoritative; there is no mutable global mint/sink total.

`0030` belongs to telemetry quarantine on this baseline. The accounting migration
is provisionally `0031`; allocation must be checked again before merge. Bootstrap,
immutable verification, runtime metadata and lifecycle registration must agree
on both MySQL and MariaDB before this increment is review-ready. Historical
whole-schema fingerprints cannot be reused on the newer base. No baseline,
active epoch, mapping or gameplay value is seeded by this schema.

## Identity-lock helper

`economic_sql_lock_authority` borrows an already active transaction with automatic
reconnect disabled. It locks the lineage row shared, then ordinary account
mapping rows in ascending lifetime ID order. It checks lineage, epoch, account
kind/context, backend, native locator and active lifetime; duplicate mapping IDs
are rejected. Errors preserve the caller's output, but may leave locks held, so
the caller must roll back. It never starts/commits/retries transactions or writes
financial evidence. Client-free builds return `ENOTSUP` without changing output.

A successful snapshot proves identity under the caller's locks only. It does not
prove domain effects, inbox ownership or authorization to append accounting
records. There are no gameplay callsites in this increment. Existing schema-2
admission/replay guards remain in place.

## Verification scope

- Schema tests must exercise constraints, retained identities, child/item/source
  references and fresh bootstrap versus supported upgrade/replay on both engines.
- Native authority tests exercise transaction ownership, ordering, concurrent
  shared readers, exclusive-writer contention, inactive/stale epochs, retirement
  and native-ID reuse, output preservation, unsigned metadata, persisted foreign-lineage mismatch and
  connection loss after acquiring locks.
- The independently discovered client-free regression checks `ENOTSUP`, null
  output handling and preservation of an already populated output under ASan/UBSan.
- Both supported server builds, immutable migration validation and runtime/lifecycle
  consistency checks remain required. Results belong to the tested revision.

## Remaining #477 acceptance work

Typed transaction-local append/finalize adapters still must bind immutable intent
to actual locked domain effects and exact legacy rows, verify after-state and all
normalized entries, and finalize before every root commit. Existing compound
savepoint rejection paths must restore accumulated effects. Financial records
need append-only application-role protections. Fault tests must prove atomic
receipt/domain/evidence/outbox commits, original-ID replay after lost commit
acknowledgement, changed-payload rejection and specialized writer integration.
Neither a schema constraint suite nor the identity snapshot qualifies those gates.
## Local qualification results

MySQL 8.0.46 and MariaDB 10.11.14 passed fresh bootstrap, full migration run and
repeat execution. Separate upgrade databases started with predecessor
`511d04f16b613a000a857af4293fcd8b2d3fb48a` bootstrap and its 30-entry migration
manifest, then applied the target 0031 and replayed. All ten schema tests passed
on each fresh and upgraded database; final runtime compatibility verification
passed on all four. Fresh/upgrade metadata fingerprints matched per engine.

Both engines passed the final ASan/UBSan authority harness, including the added
foreign-lineage and lost-connection cases. The discovered client-free sanitizer
regression also passed. Static lifecycle/runtime validators and 37 existing
registration tests passed. MariaDB required a Linux temporary datadir after an
ALTER TABLE rename failed on the Windows-backed test filesystem; unchanged tests
passed on the fresh Linux directory. All owned database processes were stopped.

Run `tests/async/run_economic_accounting_schema_mysql.sh` with each supported
`ECONOMIC_ACCOUNTING_DB_IMAGE` for the disposable fresh-schema suite. The upgrade
journey must use a separate database adopted by the predecessor runner/manifest
before the target runner is used; replay on a target fresh bootstrap alone is
not upgrade evidence. Neither test flow may use an existing game database.

Full server build results are recorded with the PR revision; native SQL results
above qualify storage and identity locking only, not an accounting command commit.
