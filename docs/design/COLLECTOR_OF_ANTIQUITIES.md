# Collector of Antiquities — implementation status

Specification: [discussion #336](https://github.com/Community-Duris/Duris/discussions/336).

**This branch is an incomplete implementation and must remain a draft.** Actual
deaths are not enrolled yet, and no live command, due worker, or collector NPC
currently collects objects or offers purchases. There is no operational
enablement switch in this revision. The SQL transaction, catalog projection,
and asynchronous read foundations described below are implemented but remain
fail-closed without those service entry points.

## Implemented policy

`src/economy/collector_policy.{h,c}` supplies deterministic, side-effect-free
transition decisions for candidate, collected, available, purchased, cancelled,
and expired records. Rules are copied at enrollment. Defaults are disabled,
twelve hours to collection, twenty-four hours to availability, seven days of
holding, 200 percent of stored base value, and a one-gold minimum. Money uses
the existing economy's copper units; 100 copper equals one gold. Fractional
copper is rounded upward. Bags and contents must be priced individually by
the transaction layer, using each item's own `cost`.

Collection checks both listing and observed item revisions. It allows a newer
item revision after environmental movement only when the caller has verified
active, unclaimed current custody. Activation starts the holding period at
actual activation, including late processing. Purchase checks beneficiary,
capacity, funds, and expiry. Terminal records cannot reactivate; a later death
must use a new listing and death operation. Paused listings cannot be bought
or expired, overdue listings cannot enter pause, and resumption preserves their
remaining holding time. Cancelling an item already held by the collector advances
its item revision so the eventual transaction cannot reuse stale custody state.

The indexed due queue leases at most the caller's requested batch size until an
explicit retry time. This keeps a repeatedly failing deadline from occupying the
head of every batch while preserving automatic retry after the lease. Commit
publication replaces its deadline or removes a closed listing. The queue and its
ephemeral leases can be reconstructed from current records; neither is a durable
store.

`src/economy/collector_codec.{h,c}` now defines the canonical version-one
collector catalog metadata image: a revisioned catalog, monotonic next-listing
cursor, strictly ordered records, fixed-width little-endian fields, and packed
death operation IDs. Encoding and decoding validate the defined
state/timing/pause/reason invariants, reject unknown versions and noncanonical
input without partially publishing output, and cap catalogs at 262,144 records.
This establishes the shared persistence boundary used by the implemented SQL
repository. The equivalent flat-file authority remains to be implemented.

These functions mutate a proposed record only. **A successful policy decision is
not proof of a committed item transfer or wallet debit.** Operation-ID replay,
atomicity, payload preservation, privacy at command dispatch, and real custody
must be implemented by the service and repositories below.

`src/economy/collector_command.{h,c}` defines the version-one critical-command
boundary for collection, activation, purchase, expiry, cancellation, pause, and
resume. Collection commands fence the complete current corpse or room root even
though only the selected item enters collector custody. This gives repositories
enough evidence to reject an incomplete root, detach a bag without taking its
excluded or later-added contents, and deterministically repair the topology that
remains. Purchase separately carries the listing revision, wallet and bank
revisions, collector and player owner revisions, and exact item revision; a
collision between the player wallet key and player-owner key never discards the
second revision because it remains in the versioned payload for repository
validation. Successful fixed-size results contain the complete canonical record
for due-queue publication and remain well below the coordinator's result limit.
`collector_transaction.{h,c}` submits this command through the critical-command
coordinator, retains interactive completions across disconnect/reconnect, and
publishes committed custody, currency, and catalog results idempotently. No live
collector service currently builds these payloads, and the selected flat-file
backend still fails closed.

The ownership contract now reserves append-only owner type 10 for collector
listings and maps it to a dedicated critical-command fence key. SQL checks,
fresh bootstrap, player recovery validation, and the ownership audit lookup all
recognize that namespace. The legacy shopkeeper widening step remains monotonic
through type 10, so a later full migration rerun cannot narrow live collector
rows. The generic item-transfer command deliberately rejects collector owners
and collector-only ledger reasons: admitting them there would update ownership
metadata without atomically updating the source object store and catalog. Only
the dedicated collector transaction described below may cross that boundary.

## Implemented SQL authority and runtime publication

`0017_collector_catalog` adds a revisioned singleton catalog cursor, canonical
per-listing metadata and exact item payload storage, death hint state, an
immutable collector ledger, and reconciliation quarantine evidence. The SQL
collector repository executes collect, activate, purchase, expire, cancel,
pause, and resume inside the caller's critical-operation transaction. Collection
rechecks the complete source root, detaches only the selected shell, reparents
its children, preserves the exact singleton item snapshot, and transfers its UID
to collector custody. Purchase atomically verifies capacity admission and all
revision fences, debits carried currency, restores the exact player-item rows,
and transfers custody to the permanent beneficiary. Expiry and held-item cancel
move the UID to terminal custody. The repository emits the canonical result to
outbox destination 11 before the encompassing transaction commits.

`collector_runtime.{h,c}` maintains the game-thread catalog map, private
beneficiary listing projection, available count, and leased due index. Startup
and five-minute reconciliation load one bounded, streaming SQL statement off the
game loop. That statement validates every denormalized listing field against its
fixed-width record and cross-checks every collected/available listing against
the exact collector owner row and owner revision. Game-thread publication first
stages the complete catalog, then atomically replaces only the collector-owned
portion of the ownership runtime; unrelated player, corpse, room, and locker
custody is preserved. A snapshot older than an outbox publication is discarded
and retried rather than rolling runtime backward.

`collector_listing_pipeline.{h,c}` supplies the bounded asynchronous read path
needed by inspect and buy preparation. It admits at most 64 active requests and
64 completions, rejects duplicate IDs, supports cancellation, fences malformed
or mismatched replies, and never carries live game pointers. SQL detail reads
are non-locking and cap the exact item-blob projection; commands will revalidate
the returned record and payload under the durable transaction locks before any
mutation.

## Remaining integration

1. Extend the committed death transfer with versioned eligibility records and
   exact applicable rules. Record exclusions by UID and permanent beneficiary ID.
   Persist enrollment atomically with corpse custody on both backends. Deaths
   before activation must never be enrolled retrospectively.
2. Add successful claimant and destruction updates at every authoritative
   boundary, including NPC/pet acquisition, entire acquired container subtrees,
   resurrection, character deletion, quarantine, and season reset. Preserve
   candidates through corpse decay and environmental movement. Returning an item
   must not remove its earlier cancellation.
3. Implement the equivalent flat-file catalog, item-payload, currency, custody,
   ledger, restart, and operation-ID replay authority. SQL is not permission to
   enable a feature that becomes unavailable or semantically different after a
   backend switch.
4. Build the live collection/purchase/expiry callbacks around the existing
   coordinator and bounded detail pipeline. Collection must move only the
   selected live shell after its commit while preserving remaining live
   topology. Purchase must materialize the exact committed singleton without
   duplicating a UID. A disconnected player or failed live publication must
   reconcile committed state before another action is admitted.
5. Add the once-per-minute bounded due service and coordinate each operation
   with movement/corpse fences. Retry uncertainty with the same operation ID;
   never perform database or filesystem I/O on the game pulse.
6. Implement the dedicated collector command, private stable listings,
   identification-preserving inspection, and service access checks. Share an
   explicit auction-room registration source across both backends. Spawn one
   stationary protected collector per registered room only while global saleable
   stock exists; reconcile after reset, restart, and copyover.
7. Implement one availability hint per death, offline delivery, staff UID/death
   inspection, counters/age metrics, and durable administrative pause/resume.
   The per-record pause policy still needs bounded service orchestration; calling
   it synchronously for an entire catalog would not meet the pulse requirement.

The current movement API explicitly rejects NPC actors in
`item_movement_transaction_submit()` and its batch counterpart. NPC acquisition
must receive a durable claimant path before intake is enabled. An in-memory
callback from `obj_to_char()` alone would not survive a crash or resolve a
collection race.

## Validation and promotion gate

Run `python3 tests/async/test_collector_policy.py` for executable policy tests and
`python3 tests/async/test_collector_codec.py` for canonical codec, corruption,
round-trip, and 100,000-record rebuild coverage. Together they cover boundary
times, delayed activation, independent cancellation, repeated-death records,
stale revisions, beneficiary checks, funds/capacity failures, terminal conflicts,
arithmetic overflow, pause/resume, strict catalog validation, and bounded indexed
leased scheduling without head-of-line starvation. They are not repository or
in-game tests.

Run `python3 tests/async/test_collector_command.py` for command/result round trips,
canonical fence reconstruction, malformed topology rejection, distinct wallet
and owner revision evidence, metadata-only transitions, held-item cancellation,
and the maximum 3,000-row source-root boundary. It proves the transaction input
contract, not a durable collector mutation.

Run the collector-focused regression set for catalog/cache publication,
transaction completion behavior, and the bounded listing pipeline. Run
`tests/async/run_collector_repository_schema_mysql.sh` only against a disposable
database: its real MariaDB journey covers empty and populated restart bootstrap,
held-custody validation, corruption rejection with strong output guarantees,
exact blob reads, source-tree shell detachment, wallet debit, player restoration,
terminal custody, operation replay, and all seven lifecycle actions. Complete
MariaDB and flat-file builds remain required after every integration change;
the latter currently proves compilation and fail-closed behavior, not backend
parity.

Run `python3 tests/async/test_item_transfer_version_compatibility.py` for the
collector ownership/fence codec boundary. Run
`tests/async/run_collector_item_owner_schema_mysql.sh` against its default
MariaDB image and again with `COLLECTOR_OWNER_DB_IMAGE=mysql:8.0`; the isolated
upgrade test proves type-9 preservation, type-10 admission across all three
ownership authorities, type-11 rejection, exact rerun behavior, and protection
against a later shopkeeper-migration narrowing pass.

Before this PR can leave draft, implement and execute the full #336 journey on
both backends with shortened timers: actual player death, partial loot, forced
corpse decay into room custody, collection, sale activation, inspection, purchase,
save/reconnect, and durable expiry. Assert exact UID, payload, wallet, and source
custody after every stage. Extend to NPC/pet claims, nested mixed containers,
repeated deaths, copyover/restart, offline hints, all auction rooms, faults before
and after commit/publication, and pickup/purchase/expiry races. Verify migrations
against an isolated database and responsive pulses with a blocked worker.

Existing corpse/combat journeys are useful regressions but do not substitute for
the collector-specific journey. GitHub CI status is not promotion evidence for
this feature.
