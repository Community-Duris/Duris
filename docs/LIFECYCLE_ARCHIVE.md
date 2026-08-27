# Lifecycle Archive Execution

Phase 03 provides a bounded archive execution contract, but the checked-in lifecycle
policy has no approved destructive rule. The live scheduler therefore exposes a
`lifecycle_archive` slot with `enabled=0`, and the operator command reports
`scheduler_state=blocked_by_policy`. No archive, purge, pseudonymization, or active-row
deletion can run from the canonical repository state.

This is an engineering boundary, not a retention or legal decision. Enabling a rule
requires a new reviewed policy version with an external controller-decision reference,
store-specific due-row/source-key selection, reconciliation ownership, and an isolated
non-production qualification run.

## Durable schema

`migrations/lifecycle_archive_execution.sql` adds four guarded InnoDB tables:

- `lifecycle_archive_jobs` holds stable policy/job identity, target, approval, cursor,
  budgets, aggregate verification, reconciliation, retry, and status metadata.
- `lifecycle_archive_batches` records exact cursor windows and copy/verify/finalize
  progress with unique stable batch identity.
- `lifecycle_archive_rows` stores a same-database payload envelope keyed by batch and
  stable source identity. A retry cannot create a duplicate source row.
- `lifecycle_archive_evidence` stores redacted counts, status, event type, and error code;
  it has no source key, payload, account, player, or network identifier.

The migration is additive and re-runnable. Fresh bootstrap contains the same definitions,
and `migrations/verify_lifecycle_archive_schema.sh` verifies engines, collations,
required columns, indexes, and foreign keys. Do not run the migration or verifier
against production as part of development or validation.

## Batch contract

`scripts/lifecycle_archive.py` defines the typed planner and batch state machine. Stable
job and batch IDs bind the policy ID/version/checksum, store, action, cutoff, starting
cursor, upper bound, and sequence. Every copy is constrained to at most 256 rows,
1 MiB, and 500 ms; checked-in defaults are tighter.

The transition is:

```text
planned -> copying -> copied -> verified -> finalizing -> completed
             ^          |           |            |
             +-- retry -+     exact checks    exact acknowledgment
```

Copy retries must contain the identical stable source set and payload checksums.
Verification compares exact source/archive counts and deterministic checksums and
requires the pre-finalization domain reconciliation. Finalization returns a stable
authorization bound to the manifest and approval identities; completion is separate
and requires exact affected/remaining counts plus post-finalization reconciliation.
Restore rechecks the archive checksum before returning rows.

No table-specific mutation adapter is active while policy decisions and selectors are
pending. This prevents a generic cleanup default from being mistaken for an approved
execution path.

## Dry-run and operator controls

Inspect the current policy without a database connection:

```sh
python3 scripts/lifecycle_archive.py inspect
python3 scripts/lifecycle_archive.py plan \
  --store database:accounts --action archive \
  --cutoff 2025-01-01T00:00:00Z --upper-bound 999
```

The second command returns `blocked` with machine-readable reason codes under the
canonical pending policy. `--state-file` writes only redacted dry-run metadata using
mode `0600` and atomic replacement. `pause`, `resume`, and `report` operate on that
metadata and reject symlinks or a stale manifest checksum. They do not connect to
MySQL or mutate active/archive rows.

## Review checklist before any future enablement

1. Record a new policy version and externally reviewed controller/approval references.
2. Define the exact sargable due predicate, stable source key, upper-bound capture, and
   source serialization for every approved store; do not infer them from a generic date.
3. Map domain reconciliation before and after finalization and preserve protected
   replay, ownership, moderation, financial, and audit horizons.
4. Implement the table-specific transaction adapter so it revalidates the authorization,
   source keys, counts, checksum, policy, and approval inside the same transaction.
5. Qualify duplicate retry, crash after copy/verify/finalize, corruption, policy change,
   late rows, restore, locks, and working-set impact on an isolated disposable database.
6. Only then enable the scheduler definition in a separately reviewed change.
