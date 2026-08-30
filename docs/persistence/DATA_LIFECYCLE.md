# Data Lifecycle Contract

DurisMUD maintains one machine-readable technical inventory at
`migrations/data_lifecycle_manifest.json`. It currently covers 173 current database
tables and 21 declared Redis, journal, fallback, quarantine, runtime-file, log,
export-spool, and backup classes. Season reset, archive, export, erasure, restore, and
documentation work must consume this inventory instead of introducing independent
store lists.

Redis keys, patterns, prefixes, and channels have an additional runtime-authoritative
inventory in `src/redis_key_registry.def`. The C++ runtime consumes its constants, the
lifecycle validator derives Redis store IDs and locators from it, and destructive
maintenance is checked against its owned patterns. A new hardcoded `mud:` or
`ship:snapshot:` literal outside the registry fails focused validation.

This is an engineering control, not legal advice or a compliance conclusion. The
repository does not contain an approved lawful-basis or retention decision. Those
fields therefore remain `pending_controller_decision`, and the global policy keeps
destructive lifecycle rules disabled. A repository operator must not replace pending
values without an externally reviewed decision and traceable reference.

## Manifest contract

Each canonical store ID records its data category, subject key, technical purpose,
controller-decision reference, season behavior, active and archive retention state,
terminal action, protected-record exception, and dependencies. The manifest-level
technical owner applies to every entry unless a future schema version adds an explicit
per-entry override.

Protected financial, currency, ownership, outcome, inbox/outbox, audit, quarantine,
replay, and recovery records retain their reconciliation or replay-horizon exception.
They cannot receive a destructive terminal action. Foreign-key parents are recorded
as dependencies; self-referential container relationships are intentionally omitted
from the acyclic ordering graph.

The season actions have precise technical meanings:

- `retain`: the existing season reset does not mutate the store.
- `deactivate`: the reset keeps the row and changes its active/deleted state.
- `reset_update`: the reset restores selected fields without deleting the row.
- `reset_delete`: the reset deletes the table's season-scoped rows.
- `regenerate`: a future consumer may rebuild the store from authoritative input.

Season actions describe the already-established administrator pwipe contract. They are
not time-based retention approval and do not enable the lifecycle actions listed under
`destructive_actions`.

## Validation and change procedure

Run the validator and focused regressions after any schema, durable-store, or season
reset change:

```sh
python3 scripts/validate_data_lifecycle.py --json
python3 tests/async/test_data_lifecycle_manifest.py
python3 tests/async/test_redis_key_registry.py
python3 tests/async/test_season_reset_manifest.py
```

Validation fails closed on missing or duplicate stores, Redis registry/manifest drift,
unknown Redis store references, unknown fields or actions, stale policy versions,
schema/FK drift, missing dependencies, dependency cycles, unapproved destructive rules,
protected-record destruction, and symlink substitution.
The optional `--destructive-preflight STORE ACTION` additionally checks the exact
versioned rule, global approval, non-production environment, loopback database host,
and lifecycle administrator role. The checked-in policy cannot currently pass that
preflight because destructive execution is intentionally disabled.

When adding a store, update the manifest in the same change as its schema or durable
file contract. Do not weaken coverage to make validation pass. When an approved
decision eventually exists, record its durable external reference and review every
affected store, dependency, recovery copy, and protected exception before changing the
global gate. Execution mechanisms are separate, narrowly scoped sessions and must
perform their own preflight and postflight checks.

## Current boundary

The manifest provides inventory and fail-closed policy loading. The bounded archive
schema and state-machine boundary are documented in
[`LIFECYCLE_ARCHIVE.md`](LIFECYCLE_ARCHIVE.md), but canonical policy approval and
table-specific selectors are pending, so its scheduler slot remains disabled. No
archive, purge, export, erasure, backup rewrite, or production operation is enabled.
The authenticated export packaging boundary and its separate pending shared-record
decision are documented in
[`PERSONAL_DATA_EXPORT.md`](PERSONAL_DATA_EXPORT.md).
The separately disabled erasure coordinator and restore-time tombstone propagation
contract are documented in [`ACCOUNT_ERASURE.md`](ACCOUNT_ERASURE.md).
