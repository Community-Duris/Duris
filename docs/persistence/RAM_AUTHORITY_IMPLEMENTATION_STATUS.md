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
- Pure RAM preparation for basic currency, epic and item/coin commands.
- A coordinator path that prepares on the game thread and publishes only after
  durable admission, plus stronger currency and epic publication retention.
- Focused codec, journal fault, crash-replay, ordering and publication regressions.

These components have passed focused tests and a full development build, but
that is not evidence that the runtime migration below is complete.

## Required before activation

- Durable request identity and receipt lookup before preparing a new outcome,
  including after cache eviction, restart and migration from existing receipts.
- Startup replay before world/player hydration, with a defined legacy-journal
  transition and safe writer shutdown.
- Complete atomic trade and auction preparation/materialization, including
  offline wallets, item custody, escrow/refunds and deterministic deadlines.
- Completion of item sidecar handling for corpse, collector and crafting paths;
  the basic item reducer must not silently cover unsupported semantics.
- Read and login boundaries that cannot replace newer authoritative RAM with a
  lagging database projection.
- Exact frozen-result checks before either persistence backend commits.
- Crash-boundary and lifecycle tests across real runtime adapters, including the
  website contract's logged-out bidding and duplicate-request journeys.
- A documented rollout/recovery procedure: an older binary cannot simply ignore
  accepted records written by the new log.

The existing database service fallback is scaffolding for development. It must
not be described as a complete RAM-authority cutover.
