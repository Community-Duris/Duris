# Implementation Summary

Session 08 cuts auction listing, bidding, buy-now settlement, expiration, removal, money
pickup, and item pickup onto the replay-safe critical command pipeline.

The new auction repository atomically composes revisioned wallet authority, stable item
ownership, auction lifecycle rows, one-time claims, ledgers, inbox results, and outbox
events. Live inventory and balances change only after committed ACK. External and offline
notifications are recovered from durable outbox rows and published on the game thread.

Additive schema introduces per-auction revisions, authoritative custody state,
operation-keyed listings, per-item stable custody, claim revisions, auction ledger, and
reconciliation quarantine. Legacy ambiguous rows remain untouched and excluded from the
new finalizer.

Focused source/codec tests and a real MySQL harness cover replay, stale rejection,
concurrent one-time claims, bid/refund/buy-now, early-finalize rejection, no-bid expiry,
item return, and cleanup isolation. Repository build, security, migration replay,
reconciliation, and the full regression suite pass.
