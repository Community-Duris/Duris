# Session 10: Artifact and Guild Outcome Batching

**Session ID**: `phase02-session10-artifact-and-guild-outcome-batching`
**Status**: Not Started
**Work Window**: The epic/combat downstream boundary for immutable artifact feed/bind
deltas and ordered guild prestige/construction changes through set-based transaction,
ownership delegation, cache outbox, and exact completion.

---

## Objective

Replace per-artifact queries and intermediate guild saves with typed batched outcomes
whose complete deltas are calculated once, committed in order, and published only after
durable acknowledgement.

---

## Scope

### In Scope (MVP)

- Inventory award-linked artifact feed, timer, bind, active-owner, cache invalidation,
  guild prestige, construction-point, ledger, and save call sites after Session 09.
- Maintain sufficient active artifact and guild state in game-thread memory to calculate
  immutable set-based deltas without database reads in epic or combat callbacks.
- Persist one bounded artifact delta set per parent outcome instead of repeated bind,
  artifact, existence, and update queries for each equipped item.
- Apply guild prestige and threshold-derived construction points as one ordered delta
  without saving between the two mutations; record stable operation identity and audit.
- Delegate actual artifact item owner changes to Session 05 while composing the required
  owner revisions and ledger in the same parent transaction where atomicity is required.
- Publish cache invalidation, operator audit, and player/guild notification records from
  typed outbox rows after commit.
- Add hydration, stale-memory, duplicate, parent-operation, large-group, and restart
  reconciliation tests for artifact and guild state.

### Out of Scope

- Auction or ordinary item ownership routes.
- Guild gameplay, permission, hall, treasury, or membership redesign.
- Phase 03 history retention and query-index tuning.

---

## Prerequisites

- [ ] Sessions 03, 05, and 09 epic, ownership, and parent combat outcomes are validated.
- [ ] Phase 00 deterministic artifact-bind results remain covered.
- [ ] Active artifact and guild hydration can fail closed without partial gameplay state.

---

## Deliverables

1. In-memory artifact/guild persistence state and immutable delta capture contracts in
   focused `src/` modules.
2. Set-based artifact and guild transaction repositories composed with parent operation
   identity, ownership, inbox, and outbox.
3. Cutover of audited per-item artifact queries and intermediate guild save routes.
4. Focused feed, bind, owner, prestige threshold, construction, duplicate, stale-cache,
   group fan-out, crash, and reconciliation regressions under `tests/async/`.

---

## Success Criteria

- [ ] Epic and combat callbacks calculate artifact/guild effects from in-memory state
      and enqueue no database or Redis work directly.
- [ ] One parent outcome applies each artifact, prestige, and construction delta at most
      once with deterministic derived identity.
- [ ] Guild prestige and threshold-earned construction commit together and never expose
      the current save-before-construction ordering gap.
- [ ] Artifact owner changes use the authoritative ownership transaction and cannot
      diverge from artifact bind/current state.
- [ ] Cache invalidation and notifications publish only after commit and survive outbox
      retry without duplicate logical delivery.
- [ ] Hydration or reconciliation failure retains prior safe state and blocks only the
      affected domain with a redacted operator signal.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
