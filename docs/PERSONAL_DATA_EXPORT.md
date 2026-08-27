# Personal Data Export Boundary

The repository defines a bounded, authenticated personal-data export packaging and
delivery contract, but canonical export is **not enabled**. The lifecycle manifest's
shared-record disclosure decision is pending, and all 106 subject-bearing mappings
therefore remain `pending`. The inspection command reports `blocked_by_policy` and has
no database, account-file, or live collector surface.

This is an engineering control, not a claim of legal compliance. Activation requires
an externally reviewed disclosure decision, a durable approval reference, and a
store-by-store review of subject selectors and shared-field rules.

## Policy and schema

`migrations/data_lifecycle_manifest.json` assigns exactly one `export_rule` to every
one of the 180 lifecycle stores. Each rule states its disposition, subject route,
decision, excluded fields, and any explicitly shareable fields. Validation rejects
unknown or missing mappings, pending-to-active shortcuts, overlaps between excluded
and shared fields, and omission of known credential, delivery-token, journal, command,
or raw-security exclusions.

`migrations/personal_data_export.sql` adds three replay-safe metadata tables:

- `personal_data_export_requests` binds stable request identity to an HMAC account
  scope, policy version/checksum, snapshot, bounded counts, status, expiry, and only a
  hash of the delivery token.
- `personal_data_export_sections` records per-store disposition, status, snapshot,
  counts, and checksum without retaining exported values.
- `personal_data_export_audit` records only event/status codes and aggregate counts.

The migration is additive and re-runnable. Its verifier checks the exact InnoDB,
collation, column, index, and foreign-key shape. No migration should be run against
production as part of repository testing.

## Request and package contract

`scripts/personal_data_export.py` supplies the inactive runtime contract used by
synthetic tests. A future activated adapter must preserve all of these controls:

- password reauthentication with bounded failures, an account-scoped request
  cooldown, zeroing of the temporary password buffer, stable request identity, and a
  random owner token retained only as a hash;
- one policy checksum and consistent snapshot identity across every section, fixed
  per-batch and whole-bundle row/byte bounds, exact completion of all mapped stores,
  and no partial package publication;
- manifest-driven credential exclusion and allow-listed fields for shared records;
- canonical JSON ordering, per-section counts and SHA-256 checksums, a whole-package
  checksum, expiry metadata, and verification before publication and release;
- an absolute, non-symlink 0700 spool directory, atomic 0600 package files, owner-bound
  one-time retrieval, short TTL, and artifact deletion on release, cancellation, or
  expiry.

The ignored local spool class is `runtime/personal-data-exports/`. Remote or in-game
release is unavailable until an authenticated TLS connection can be bound to the exact
request owner and the disclosure policy is approved.

## Verification

Run the fail-closed source tests and disposable database test:

```sh
python3 scripts/personal_data_export.py inspect
python3 tests/async/test_personal_data_export.py
python3 tests/async/test_data_lifecycle_manifest.py
tests/async/run_personal_data_export_schema_mysql.sh
```

The MySQL script creates and destroys its own isolated container. Tests use synthetic
accounts and values only; they do not read `.env`, configured databases, real player
files, credentials, or production data.
