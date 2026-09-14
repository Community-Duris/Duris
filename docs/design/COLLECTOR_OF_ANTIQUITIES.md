# Collector of Antiquities — implementation status

Specification: [discussion #336](https://github.com/Community-Duris/Duris/discussions/336).

**This branch is an incomplete implementation and must remain a draft.** The
policy module does not enroll actual deaths, collect objects, spawn NPCs, or
offer purchases. There is no operational enablement switch in this revision.

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
This establishes a shared persistence boundary; object payloads and both backend
repositories remain unwired.

These functions mutate a proposed record only. **A successful policy decision is
not proof of a committed item transfer or wallet debit.** Operation-ID replay,
atomicity, payload preservation, privacy at command dispatch, and real custody
must be implemented by the service and repositories below.

The ownership contract now reserves append-only owner type 10 for collector
listings and maps it to a dedicated critical-command fence key. SQL checks,
fresh bootstrap, player recovery validation, and the ownership audit lookup all
recognize that namespace. The legacy shopkeeper widening step remains monotonic
through type 10, so a later full migration rerun cannot narrow live collector
rows. The generic item-transfer command deliberately rejects collector owners
and collector-only ledger reasons: admitting them there would update ownership
metadata without atomically updating the source object store and catalog. Only
the dedicated collector transaction described below may cross that boundary.

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
3. Implement a versioned collector critical command and specialized SQL,
   flatfile, and runtime transactions for the reserved collector namespace. The
   generic item-transfer path must remain closed to collector custody because it
   cannot commit the source object store and collector catalog together.
4. Wire the versioned catalog records into complete per-item payload storage,
   additive verified SQL migrations, flatfile authority images, and equivalent
   operation-ID replay.
   Collection must atomically recheck containment, detach eligible entries, leave
   excluded/added contents at source, update source payloads, and retain exactly
   one authoritative item location. Purchase must atomically debit the carried
   wallet and transfer the same UID; expiry must use durable destruction.
5. Add bounded worker requests and completion publication under movement/corpse
   fences. A failed or uncertain commit must reuse the same operation ID. A
   disconnected player or failed live publication must reconcile committed state
   before another action is admitted. No new filesystem or database I/O belongs
   on the game pulse.
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
