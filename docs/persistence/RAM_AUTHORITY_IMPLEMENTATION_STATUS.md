# RAM authority implementation status

This branch is a draft continuation of PR #717. The runtime cutover is disabled.
Do not deploy it as a completed RAM-authoritative persistence implementation.

## Contract

The game thread decides economic outcomes from authoritative live state. A
durable atomic operation log records the complete accepted outcome before live
publication and success acknowledgement. A single ordered writer materializes
those outcomes and player checkpoints. Recovery completes before hydration or
new economic admission. No accounting features are part of this migration.

Website bidding requires a running MUD but supports logged-out characters. See
[the website contract](RAM_AUTHORITY_WEBSITE_CONTRACT.md) for ownership, retry,
login/logout and read-projection requirements.

## Foundation implemented

- Versioned sealed command outcomes and bounded player-checkpoint records.
- Exclusive journal ownership and ordered writer admission before durable append.
- Retention after uncertain append or divergent projection, without publishing a
  successful gameplay result or allowing following writes to overtake it.
- Exact result-byte and revision checks before discarding a projected log record;
  revision divergence retains recovery evidence both live and during cold replay.
- Durable bounded receipts are consulted before preparation, including after
  completed-cache eviction and restart. Conflicting ID reuse is rejected. An
  arrival timestamp is not part of a RAM request's identity.
- Receipt persistence precedes WAL checkpointing. Faults at either boundary retain
  the redo record. Successful legacy journal requests are reconciled using their
  original schema before their resulting receipts are adopted.
- Allocation failures during fence checks or after projection retain the writer
  job for retry, including receipt construction after a backend commit.
- Operation-log shutdown waits for active projector callbacks; callbacks retained
  from a previous log lifetime cannot touch a newly opened journal.
- Pure RAM preparation for basic currency, epic and item/coin commands.
- A coordinator path that prepares on the game thread and publishes only after
  durable admission, plus stronger currency and epic publication retention.
- The coordinator's native regression uses the actual ordered persistence writer
  and covers publication fences, ambiguous retry ordering, capacity and restart
  of the worker/coordinator. Conflicting active publication modes are rejected.
- SQL finalization checks the sealed revision and bytes before commit. Flat-file
  epic, wallet/bank, item, coin, auction and shop transactions do the same before
  publishing any candidate state. Coin receipts now include their actual materialized revision.
- Backup capture retains the journal lease file and operation receipts. Native
  restore qualification verifies receipt checksums, IDs, sizes and private files.
- Focused codec, journal fault, crash-replay, ordering and publication regressions.

These components have passed focused tests and a full development build, but
that is not evidence that the runtime migration below is complete.

## Required before activation

- Migration of historical receipts already checkpointed out of the legacy
  journals. SQL stores the old request hash, not the complete request or its
  acceptance timestamp; it cannot simply be rehashed into the new identity
  contract. Historical IDs must not be treated as newly admitted RAM requests.
- Wire startup replay before world/player hydration, integrate the separate
  player journal transition, and qualify the runtime shutdown ordering. The
  operation-log APIs are tested but are not invoked by normal server startup.
- Complete atomic trade and auction preparation/materialization, including
  offline wallets, item custody, escrow/refunds and deterministic deadlines.
- Completion of item sidecar handling for corpse, collector and crafting paths.
  The basic item reducer now explicitly refuses these unsupported semantics.
- Read and login boundaries that cannot replace newer authoritative RAM with a
  lagging database projection.
- Extend sealed preparation/materialization to the remaining domain adapters;
  the projection guards do not supply the missing RAM auction/shop/sidecar reducers.
- Crash-boundary and lifecycle tests across real runtime adapters, including the
  website contract's logged-out bidding and duplicate-request journeys.
- Qualify admission throughput and world-pulse latency: preparation currently
  permits one decision per pulse and its durable append is synchronous.
- A documented rollout/recovery procedure: an older binary cannot simply ignore
  accepted records written by the new log.

The RAM coordinator no longer has a database-decision fallback. Unsupported
operations fail explicitly. This is a guard against an accidental mixed-authority
cutover, not a replacement for implementing the remaining domains.

## Receipt storage and recovery

`CRITICAL_COMMAND_JOURNAL_DIR/receipts/<32-lowercase-hex-operation-id>` holds each
success receipt after its redo record is checkpointed. The versioned `DOR1` codec
contains the operation ID, SHA-256 request digest, exact outcome revision, bounded
result bytes, and a SHA-256 checksum over those fields. It retains no second copy
of a player's full snapshot or an item's full tree. The complete request digest
includes its operation ID, keys, expected revisions and payload, with acceptance
time normalized. For ordinary requests, sealed result fields are excluded from
the request digest; checkpoint identity includes the complete captured checkpoint.

Receipts must be backed up and restored with the journal and projections. They
must not be pruned merely because the in-process cache evicted an operation or its
projection completed. Corrupt receipts stop admission; failed or divergent replay
retains evidence. A receipt format or retention change requires a migration that
preserves deduplication. An older deployment must not ignore these records.
