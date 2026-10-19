# RAM authority runtime

PR #718 enables the RAM authority runtime. The game thread decides economic
outcomes, records their complete accepted results in the durable atomic operation
log, and then publishes them to live state. A single ordered writer materializes
those decisions and player checkpoints. No accounting features are enabled.

## Runtime and recovery

Startup replays the legacy player journal and outstanding critical operations
before loading wallets, shared account banks, custody, auctions, corpses, collector
state, rewards, and immutable death evidence. Admission stays closed if recovery
or hydration fails. World-boot artifact registration is reconciled before opening
admission. Shutdown drains the coordinator and ordered writer before releasing
the operation-log lease.

A sealed operation includes its exact result bytes and revision. SQL adapters
check them before committing; flat-file adapters check them before publishing
staged after-images. Projection failure or divergence retains the record and
prevents later writes from overtaking it. Ambiguous append does not publish
success and requires recovery. Live publication failures retain their fences.

Player checkpoints share the ordered log with economic operations. Their capture
revision and death evidence are staged before append; accepted death checkpoints
quarantine the captured items in RAM. Recovery applies the complete recorded
outcome, rather than recalculating prices, deadlines, or refunds at replay time.

The coordinator processes up to 64 completions within a 10 ms scheduling budget,
with each live publication observed before preparing the next decision. A single
durable filesystem sync can exceed that budget; storage latency still matters.

## Integrated domains

- Wallets, shared account banks, coin transfers, epics, combat rewards, and zone
  claims use RAM preparation and sealed outcomes.
- Shops log payment, stock/custody changes, complete item snapshots, and collector
  boundaries as one operation. Existing stock, produced stock, stored sales,
  destroyed sales, and invalid-stock removal use the same path.
- Auctions log bidder debits, escrow, outbid refunds, buyouts, seller proceeds,
  item claims, and removal/expiry outcomes together. Expiry and pickup selection
  use the RAM catalog. Distinct items in a stacked listing retain their own UIDs,
  condition, affects, and runtime data through listing and pickup recovery.
- Corpse lifecycle and item transfers preserve physical item data and custody.
  Collector decisions use the recovered RAM catalog. Death restitution checks
  immutable evidence, quarantine state, exact artifacts, and competing custody
  before admission.
- Boon rewards and permanent-stat changes retain their revisioned outcomes and
  override older login projections.
- Session audit events use the ordered operation path on SQL backends.

Administrative artifact maintenance and older artifact lifecycle registration
remain synchronous compatibility boundaries. They drain the ordered projector
before reading legacy rows and refresh the artifact RAM state and revision before
returning to command admission. Failed drainage or refresh fences admission. This
prevents maintenance from overwriting a logged feed while it waits for SQL. It is
not a claim that every administrative or world-data write in the server has been
converted into a new economic command.

The flat-file backend retains its existing unsupported domains: artifact/guild
outcomes, session audit, zone claims, and death restitution are refused before
append. World-corpse follower raises containing artifacts retain the prior
unsupported transition and are also refused before durable acceptance. Unsupported
operations never silently fall back to database decision-making.

## Offline characters and website bids

The economy owns one wallet per PID and one bank per account/racewar, independent
of character descriptors. Recovered offline wallets remain available while the
MUD runs. Login takes current balances and revisions from that owner, even if its
SQL load began before an accepted website bid. Logout does not create another
spendable wallet. Offline completions refresh shared-bank revisions for online
characters on the same account.

The authenticated MUD bridge implements `durisweb_auction_bid`, including stable
request identity and explicit pending/committed/rejected/unavailable replies.
See [the bridge contract](RAM_AUTHORITY_WEBSITE_CONTRACT.md). The separate website
repositories still need their bid/buy routes changed to use this command; their
direct database mutation gates must remain closed.

## Receipts and historical operations

`CRITICAL_COMMAND_JOURNAL_DIR/receipts/<32-lowercase-hex-operation-id>` retains
checksummed receipts after redo records are checkpointed. DOR1 success receipts
remain readable; DOR2 also records durable rejections. Receipt persistence precedes
WAL checkpointing. Requests retain the same identity across retries, reconnects,
cache eviction, and restart. Acceptance time is excluded from ordinary request
identity; different intent under an existing ID is rejected.

Historical SQL inbox IDs are loaded at startup. An old checkpointed operation
whose original request cannot be reconstructed is refused as an identity conflict,
never executed again. Outstanding legacy records replay under their original
schema before their receipts are adopted. Flat-file repositories retain their
historical operation catalogs; the new website request namespace starts with this
runtime.

Receipts, journals, and projections must be backed up and restored together.
Receipts cannot be pruned because a request has aged out of the process cache.
Corruption stops admission, and divergent replay retains evidence for repair.

## Rollout and rollback

1. Stop admission and shut down the existing MUD using its normal shutdown path.
   Preserve its pending journals; do not clear them to make startup succeed.
2. Take a consistent backup of the database/flat-file authority, both configured
   journal directories, operation receipts, and other required world state.
3. Apply the guarded immutable migrations through `0032_ram_authority` with the
   repository migration runner against the intended environment. This adds shop
   clocks and runtime item extensions for shops, corpses, saved items, and pets,
   plus the permanent-stat revision. Do not manually edit migration history.
4. Start the new binary with the same durable journal locations and exclusive
   ownership. Recovery and full economic hydration must succeed before admission.
5. Connect the website bridge only after the server is ready. Retry pending bids
   using their original request IDs; never replace a timeout with direct SQL.

After the new binary has accepted operations, do not start an older binary against
its journals. Prefer repairing/rolling forward with a reader for these records.
A rollback needs a consistent pre-cutover backup of all authorities and an explicit
plan for every subsequently accepted operation; deleting the log loses outcomes.

## Validation

Focused native tests cover pure RAM decisions, abandoned preparation, immutable
checkpoint/restitution evidence, exact artifact preconditions, custody, publication
retention, replay, receipt backup/restore, and source routing. The assembled-runtime
SQL harness links the actual server objects and uses an isolated database. It
exercises offline bids with a paused projector, stale login, abrupt exit after
acknowledgement, cold recovery, duplicate/conflicting request IDs, outbid refunds,
buyouts, stacked item claims, artifact maintenance ordering, and each shop action.

Use `python3 tests/async/test_ram_authority_runtime_mysql.py` only with explicit
`TEST_DB_HOST=127.0.0.1`, `TEST_DB_PORT`, `TEST_DB_USER`, and `TEST_DB_PASSWORD` for an
isolated test service. It creates and drops only its own random test schema and
never reads `.env`. `RAM_RUNTIME_BIN_ROOT` may point to an already-built server;
otherwise it builds the default tree. Run the focused `test_*_ram.py` regressions
and the existing transaction, journal, load/save, and flat-file suites alongside
`make -C src`.
