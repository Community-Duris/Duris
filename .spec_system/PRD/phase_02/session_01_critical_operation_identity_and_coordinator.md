# Session 01: Critical Operation Identity and Coordinator

**Session ID**: `phase02-session01-critical-operation-identity-and-coordinator`
**Status**: Not Started
**Work Window**: One accepted-command boundary from stable operation identity and
immutable capture through multi-key admission, typed journal handoff, gameplay fencing,
and exact main-thread completion, using a fake destination before database integration.

---

## Objective

Establish the non-coalescing critical-command contract that every Phase 02 domain can
reuse without allowing retries, reconnects, or replay to allocate a second gameplay
effect.

---

## Scope

### In Scope (MVP)

- Revalidate the implemented Phase 01 coordinator, journal, identity, replay, shutdown,
  and completion APIs and define which contracts can be reused safely.
- Define a bounded, schema-versioned immutable command envelope carrying one stable
  operation ID, command type, affected entity keys, expected revisions, payload version,
  source site, deadline class, and typed payload with no raw SQL or live pointers.
- Generate or adopt collision-resistant operation IDs once before acceptance and retain
  the same ID across queue retry, durable replay, ambiguous completion, reconnect,
  copyover, and process restart.
- Add deterministic multi-key admission and release for player, account, item, guild,
  locker, corpse, and auction keys so conflicting commands serialize without deadlock
  while unrelated commands can progress in parallel.
- Journal each accepted critical command before it can become the only recoverable copy;
  critical records remain independent and are never coalesced like player snapshots.
- Add game-thread fences, duplicate client-request attachment, cancellation rules, exact
  typed completions, and bounded operation/queue/journal/fence metrics using a fake
  destination adapter.

### Out of Scope

- Database inbox, outbox, ledger, or domain-table writes.
- Epic, bank, wallet, item, combat, artifact, guild, boon, reward, or zone cutover.
- Network notification delivery or legacy raw-queue removal.

---

## Prerequisites

- [ ] Phase 00 and Phase 01 are complete and their carryforward evidence is reconciled.
- [ ] Phase 01 journal replay and shutdown spill pass their crash-point gate.
- [ ] The implemented Phase 01 operation identity is inspected before extending it.

---

## Deliverables

1. Critical command envelope, operation identity, affected-key, result, and state
   contracts in focused `src/` modules.
2. Bounded non-coalescing multi-key coordinator integrated with Phase 01 journal,
   replay, completion, copyover, and shutdown lifecycle.
3. Game-thread fencing and duplicate-request APIs with redacted diagnostics.
4. Focused identity, ordering, replay, bounds, fence, reconnect, shutdown, and fake-
   destination regressions under `tests/async/`.

---

## Success Criteria

- [ ] One accepted gameplay intent has one operation ID across every retry and restart.
- [ ] Critical records are never coalesced, dropped, or replaced by a newer snapshot.
- [ ] Conflicting key sets execute in deterministic order without deadlock, while
      independent key sets can progress within configured bounds.
- [ ] A stale, duplicate, or mismatched completion cannot release a newer fence or
      publish success for another operation.
- [ ] The game thread performs no database, Redis, or filesystem work after the bounded
      journal acceptance contract defined by Phase 01.
- [ ] Payload, queue, journal, operation age, retry, and fence states are bounded and
      observable without private values.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
