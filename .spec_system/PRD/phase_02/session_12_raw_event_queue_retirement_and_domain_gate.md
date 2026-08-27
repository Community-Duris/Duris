# Session 12: Raw Event Queue Retirement and Domain Gate

**Session ID**: `phase02-session12-raw-event-queue-retirement-and-domain-gate`
**Status**: In Progress
**Work Window**: One final transactional-domain cutover boundary inventorying every
durable event producer, replacing unrestricted SQL records, reconciling ledgers/current
rows, and proving all domains under crash, replay, outage, and bounded load.

---

## Objective

Remove every remaining non-idempotent raw SQL queue or fallback route and prove Phase 02
domains converge exactly under duplicate, ambiguous, failure, restart, and 200-player
workloads before Phase 03 depends on their current-state tables.

---

## Scope

### In Scope (MVP)

- Inventory all producers and consumers of item, scalar, large-payload, fallback-file,
  and Phase 01 journal records plus direct critical SQL mutations after Sessions 01-11.
- Convert remaining required audit/event producers to bounded schema-versioned typed
  records with mandatory stable IDs and destination dedupe, or remove obsolete producers.
- Disable and delete unrestricted durable raw SQL execution, raw-query fallback append,
  and replay paths once compatibility import/quarantine tests prove no active producer
  depends on them; preserve historical database rows and source evidence.
- Provide safe one-time inspection/import or quarantine for legacy fallback records
  without executing unvalidated SQL or inventing operation IDs for ambiguous effects.
- Run full read-only and isolated-write reconciliation across epic balances, account
  banks, wallets, current item owners, ledgers, inbox results, outbox delivery, and
  affected revisions; repair only through guarded idempotent commands.
- Execute the Phase 02 crash matrix at enqueue, journal, DB begin/apply/commit, ACK,
  outbox, and checkpoint boundaries, including duplicate replay, deadlock, DB outage,
  worker crash, game crash, disk failure, reconnect, copyover, and shutdown.
- Run representative non-production 25, 50, 100, and 200-client epic, banking, trade,
  corpse, locker, auction, PvP, artifact, boon, reward, and zone workloads with redacted
  operation-age, queue, pulse, and reconciliation evidence.
- Repair gate failures without weakening operation identity, atomicity, ordering,
  bounds, privacy, or fail-closed contracts and update operator/developer documentation.

### Out of Scope

- Phase 03 login N+1, query/index, migration-ledger, retention, archival, and data-rights
  implementation.
- Production migrations, destructive production tests, or deletion of historical
  financial, ownership, audit, player, account, or fallback data.

---

## Prerequisites

- [ ] Sessions 01 through 11 are completed and validated.
- [ ] A backed-up representative development clone and non-production game ports are
      available for schema, reconciliation, load, and fault work.
- [ ] Legacy fallback locations are isolated, ignored, permission-restricted, and
      copied before compatibility testing.

---

## Deliverables

1. Complete raw/direct producer inventory and removal or typed-conversion changes across
   `src/`, `migrations/`, and persistence lifecycle code.
2. Legacy fallback inspection/import/quarantine tooling that never executes unrestricted
   SQL records as trusted commands.
3. Phase 02 reconciliation, duplicate/crash, concurrency, and bounded 25-to-200-client
   harnesses under `tests/async/` and ignored output locations.
4. Repaired gate findings plus updated architecture, database, configuration, testing,
   recovery, and runbook documentation.

---

## Success Criteria

- [ ] No unrestricted raw SQL queue, journal, or fallback can carry or replay a
      non-idempotent gameplay effect.
- [ ] Every remaining durable message is typed, bounded, schema-versioned, stably
      identified, redacted, and deduplicated at its destination.
- [ ] Epic, bank, wallet, and current-owner rows reconcile exactly with ledgers and
      revisions after every defined crash and duplicate-replay boundary.
- [ ] No final gameplay success, cache publication, WebSocket message, or destructive
      custody transition occurs before its exact durable ACK/outbox contract.
- [ ] Queue, journal, inbox, outbox, retry, fence, and worker resources remain within
      configured bounds through the defined outage and 200-client workload.
- [ ] Normal Phase 02 mutation paths perform no database, Redis, or filesystem I/O on
      the simulation thread.
- [ ] Historical event and fallback data is preserved or explicitly quarantined; no
      financial, ownership, audit, player, or account record is deleted by the gate.
- [ ] Operator documentation and diagnostics describe the implemented commands,
      reconciliation, recovery, overload, and outbox behavior accurately.
- [ ] Focused tests, isolated schema tests, formatting checks, `make -C src`, and the
      full repository gate pass.
