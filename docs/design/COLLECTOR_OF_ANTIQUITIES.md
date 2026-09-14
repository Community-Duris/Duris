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
or expired, and resumption preserves their remaining holding time.

The indexed due queue returns at most the caller's requested batch size. Reading
due work leaves it scheduled for retry. Commit publication replaces its deadline
or removes a closed listing. The queue can be reconstructed from current records;
it is not itself a durable store.

These functions mutate a proposed record only. **A successful policy decision is
not proof of a committed item transfer or wallet debit.** Operation-ID replay,
atomicity, payload preservation, privacy at command dispatch, and real custody
must be implemented by the service and repositories below.

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
3. Define a collector namespace under system custody. Currently
   `item_owner_identity_valid()` permits only `{system, 0, 0}`, and the item
   transfer codec rejects transfers *to* system custody. The SQL repository,
   flatfile repository, and runtime cache also interpret transfers *from* system
   custody as creation. All must distinguish storage from creation together.
4. Add complete per-item payload storage, additive verified SQL migrations,
   versioned flatfile authority images, and equivalent operation-ID replay.
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

Run `python3 tests/async/test_collector_policy.py` for executable policy tests.
They cover boundary times, delayed activation, independent cancellation,
repeated-death records, stale revisions, beneficiary checks, funds/capacity
failures, terminal conflicts, arithmetic overflow, pause/resume, and bounded
scheduling of 100,000 records. They are not repository or in-game tests.

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
