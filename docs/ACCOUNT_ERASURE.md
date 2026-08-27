# Account Erasure and Restore Tombstones

Canonical account erasure is **not enabled**. The lifecycle manifest has no approved
destructive actions, every store's controller decision remains pending, and inspection
reports all 188 stores as `retain` with `request_state=blocked_by_policy`. No live
database/file mutator or account-menu erasure hook exists.

Session 10 defines the safety boundary that any future approved adapter must satisfy.
It is an engineering control, not legal advice or approval to erase records.

## Request and ordering

`migrations/account_erasure.sql` adds request, per-store, redacted evidence, and
tombstone tables. Stable request keys bind the policy checksum, HMAC account scope,
and idempotency key without storing a raw account name. Store rows bind an exact
dependency-reversed sequence, action, affected count, remaining direct-identifier
count, and reconciliation checksum. Tombstones retain only a non-reversible subject
token, request/scope hashes, policy identity, completion time, and restore generation.

`scripts/account_erasure.py` is inspection-only for the canonical manifest. Synthetic
tests exercise this required transition order:

1. Password reauthentication and owner-token creation.
2. A separate exact request-bound confirmation that stores no confirmation phrase.
3. Fence account/character mutations and disconnect owned descriptors.
4. Drain Phase 01 snapshots and Phase 02 commands; reconcile all value domains.
5. Apply every store's approved dependency-ordered action through bounded domain
   adapters. Retained stores cannot be mutated; value stores require transactional
   disposition.
6. Verify zero direct identifiers where removal/pseudonymization was approved and
   exact reconciliation for every store.
7. Commit the durable tombstone, verify credentials absent and login impossible, then
   report completion.

Cancellation closes when the mutation fence begins. A repeated completed store action
is accepted only when its counts and evidence are identical; conflicting retry data
fails the request. Credentials can never be removed as an early success shortcut.

## Backup and replay propagation

Historical backups are not edited in place. A completed tombstone ledger must be
carried forward separately from the older backup and applied before restored service,
login, replay, import, cache publication, or export release. The preflight requires
every restored record to expose its stable account-scope hash and strips tombstoned
scopes from database backups, pfile backups, conversion backups, journals, cache
rebuilds, and export spools. Unscoped restored identities fail closed.

An operator must not reopen or publish a restored environment until the newer
tombstone set is present, its policy/generation identity verifies, every source class
has been scanned, and the erased account remains uncredentialed and unloadable.

## Verification

```sh
python3 scripts/account_erasure.py inspect
python3 tests/async/test_account_erasure.py
tests/async/run_account_erasure_schema_mysql.sh
```

The database test uses and destroys an isolated MySQL container. The coordinator tests
use synthetic accounts, records, backup generations, and policy approvals only. Never
run a migration or erasure operation against production during repository validation.
