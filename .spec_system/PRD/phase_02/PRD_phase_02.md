# PRD Phase 02: Transactional Gameplay Domains

**Status**: In Progress
**Sessions**: 12 (initial estimate)
**Estimated Duration**: Adaptive; each session continues through its verification boundary

**Progress**: 0/12 sessions (0%)

---

## Overview

Phase 02 moves non-idempotent gameplay outcomes from relative timing across player
checkpoints, raw SQL queues, and ad hoc synchronous writes into explicit transactional
domains. Every accepted epic, currency, item-ownership, combat, artifact, guild, boon,
reward, and zone command receives one stable operation ID, survives retry through the
typed Phase 01 journal, and reaches one database transaction guarded by an inbox
dedupe record. Current state, immutable ledger entries, affected entity revisions, and
transactional outbox records commit together before the game publishes final success.

The phase first establishes a non-coalescing critical-command coordinator and generic
inbox/outbox contract. It then cuts over epic balance, account bank and player wallet,
and item ownership in independently reconcilable domains. Ownership work is divided
between the common transfer primitive, live player/floor/corpse movement, lockers, and
auctions because those paths have different custody and rollback boundaries. Later
sessions batch PvP, artifact, guild, boon, reward, and zone outcomes before removing
unrestricted raw SQL messages from durable queues.

Phase 00 and Phase 01 are complete and audited. Their correctness, trust-boundary,
immutable snapshot, revision, journal, exact-ACK, and bounded recovery contracts are
prerequisites for every Phase 02 session.

---

## Progress Tracker

| Session | Name | Status | Work Window | Validated |
|---------|------|--------|-------------|-----------|
| 01 | Critical Operation Identity and Coordinator | Not Started | Stable operation IDs, multi-key admission, journal handoff, fences, and exact completions | - |
| 02 | Transactional Inbox, Outbox, and Reconciliation | Not Started | Generic dedupe transaction, outbox delivery, ambiguous-commit lookup, and repair tooling | - |
| 03 | Epic Ledger and Balance Transactions | Not Started | Award and spend commands, opening baseline, balance publication, and bonus-state ACKs | - |
| 04 | Account Bank and Wallet Transactions | Not Started | Atomic denomination deltas, wallet revisions, online-alt publication, and reconciliation | - |
| 05 | Item Ownership Ledger and Transfer Primitive | Not Started | Durable item identity, current-owner row, subtree transfer, revisions, ledger, and outbox | - |
| 06 | Live Item Movement and Corpse Cutover | Not Started | Player, container, floor, trade, corpse creation, and corpse-loot ownership routes | - |
| 07 | Locker Ownership Cutover | Not Started | Public/private locker custody, immutable snapshots, exact transfer ACKs, and failure retention | - |
| 08 | Auction Settlement and Claim Cutover | Not Started | Listing custody, bid funds, settlement, refunds, claims, ownership, and publication | - |
| 09 | PvP and Combat Outcome Batching | Not Started | Immutable battle outcome capture and transactional group, frag, ledger, and outbox apply | - |
| 10 | Artifact and Guild Outcome Batching | Not Started | Set-based artifact deltas and ordered guild prestige/construction commands | - |
| 11 | Boon, Reward, and Zone Command Batching | Not Started | Typed progress, reward, and zone-touch commands with domain delegation and batch apply | - |
| 12 | Raw Event Queue Retirement and Domain Gate | Not Started | Producer inventory, unrestricted SQL removal, crash/replay reconciliation, and load gate | - |

---

## Completed Sessions

None yet.

---

## Upcoming Sessions

- Session 01: Critical Operation Identity and Coordinator

---

## Objectives

1. Give every non-idempotent gameplay command a stable operation ID that is reused
   across retries, replay, ambiguous commits, reconnect, copyover, and restart.
2. Extend the Phase 01 typed journal and bounded worker model with non-coalescing
   critical commands, deterministic multi-entity ordering, gameplay fences, and exact
   main-thread completions.
3. Commit operation dedupe, current state, immutable ledger or audit rows, affected
   revisions, and transactional outbox records in one InnoDB transaction.
4. Make epic awards and spends reconcile exactly between an opening balance, immutable
   deltas, and the materialized player balance.
5. Move account-bank and player-wallet changes into one checked multi-denomination
   transaction and publish committed bank balances to every online account character.
6. Maintain one authoritative current owner and immutable ownership ledger for every
   transferable durable item while advancing all affected inventory revisions.
7. Cut player, floor, trade, corpse, locker, and auction item routes over to atomic
   ownership commands without destroying or publishing state before durable ACK.
8. Persist PvP, artifact, guild, boon, reward, and zone outcomes as immutable typed
   commands and bounded set-based transactions instead of per-recipient SQL fan-out.
9. Remove unrestricted raw non-idempotent SQL from event queues, fallbacks, and replay
   while retaining required historical rows and compatibility evidence.
10. Prove duplicate replay, ambiguous commit, ordering, reconciliation, overload, and
    simulation-thread isolation under representative non-production workloads.

---

## Prerequisites

- All Phase 00 and Phase 01 sessions are completed, validated, and reconciled by their
  phase audits.
- Phase 01 provides monotonic player and inventory revisions, immutable capture,
  bounded keyed workers, exact main-thread ACKs, typed journal replay, and bounded
  shutdown spill.
- Phase 00 redacted observability, connection trust boundaries, account-bank delta
  containment, and terminal failure contracts remain enforced.
- Phase 01 carryforward and documentation evidence is reconciled before the first
  Phase 02 session is planned.
- All migrations, backfills, reconciliation writes, and fault/load tests use only an
  isolated development database or backed-up representative clone.
- C/C++ changes use the repository C++20 build, changed-line formatting checks, and
  focused regressions under `tests/async/`.

---

## Planning Assumptions And Resolutions

### Working Assumptions

- Phase 01 journal identities and completion routing are reusable for Phase 02 command
  envelopes. Critical commands differ from coalesced checkpoints: once accepted, each
  operation ID and its full effect set must remain independently replayable until the
  destination inbox proves it committed.
- The materialized epic balance is the operational authorization row after cutover,
  while a new immutable ledger beginning with an explicit opening-balance record is
  the audit and reconciliation history. Existing `epic_gain` rows lack mandatory
  operation IDs and do not record every spend, so treating that legacy history as a
  complete authority would invent missing facts.
- Existing `persistence_item_events` rows remain historical evidence but are not a
  safe current-owner authority. The latest-event query fails open, has no complete
  producer coverage, and is separate from inventory snapshots. Phase 02 therefore adds
  an authoritative current-owner row and a new operation-keyed immutable ledger while
  preserving legacy rows read-only.
- Transactional outbox delivery is at least once, with consumer-side dedupe by outbox
  identity. Exactly-once gameplay effects come from the operation inbox and one domain
  transaction, not from assuming a network delivery can occur exactly once.
- Item ownership is split across four sessions because identity and transaction
  invariants, live movement/corpse custody, locker custody, and auction settlement each
  have an independent executable result and fault boundary.
- When Phase 02 becomes active, `plansession` will re-run source and schema analysis
  against the implemented Phase 00 and Phase 01 system. It may refine file-level tasks
  without weakening the phase objectives, domain boundaries, or acceptance checks.

### Conflict Resolutions

- Normal `phasebuild` advances `current_phase`, but the user explicitly requested
  sequential advance planning before Phase 00 implementation. The analyzer reports
  Phase 00 as active, Session 00.01 planned, and no completed sessions. Phase 02 is
  therefore added as future tracked work while `current_phase` and `current_session`
  remain on Phase 00 so neither earlier phase is skipped.
- The master PRD leaves epic authority open. Current gameplay authorizes spends from
  `player_data.epics`, while `epic_gain` is incomplete and non-idempotent. The chosen
  transition keeps a locked materialized balance for authorization, seeds a documented
  opening ledger balance at cutover, records every later delta once, and requires exact
  reconciliation before and after enabling commands.
- Phase 01 revisioned snapshots include compatibility boundaries for economy and
  ownership, but snapshots cannot make non-idempotent actions exactly once. Phase 02
  removes transactional-domain fields from independent checkpoint authority and uses
  operation IDs, entity revisions, inbox dedupe, ledgers, and outboxes instead.
- Existing auction transactions and persistence-event dedupe are retained as useful
  foundations, but their local transaction IDs, relative player saves, and derived
  hashes do not satisfy stable command identity, atomic current ownership, or outbox
  publication. The relevant sessions adapt rather than merely relabel those paths.

---

## Technical Considerations

### Architecture

The game thread captures one immutable command, allocates its stable operation ID,
reserves the complete set of affected player, account, guild, item, locker, corpse, or
auction keys in deterministic order, and journals the command before it becomes the
only recoverable copy. Relevant gameplay actions remain fenced until an exact completion
is applied on the game thread; repeated client input attaches to or reports the same
accepted operation instead of allocating a second economic effect.

Workers receive typed values only. A domain transaction locks authoritative rows in a
canonical order, checks or creates the unique operation inbox row, validates expected
revisions and preconditions, applies current rows and immutable ledger records, advances
every affected revision, inserts redacted typed outbox messages, and commits. An
ambiguous response is resolved by querying the operation ID. A retry never regenerates
an ID or recalculates effects from changed live state.

Outbox workers deliver only after commit and mark delivery independently. Completions
return committed balances, owners, revisions, and classified outcomes to the game
thread, which then publishes messages, cache invalidations, and in-memory state. Player
snapshots must not later overwrite domain-owned epic, wallet, bank, or ownership rows.

### Technologies

- C++20 immutable command variants, bounded queues, and game-thread completion routing
- Phase 01 typed journal, replay identity, worker lifecycle, and shutdown spill
- MySQL or MariaDB InnoDB transactions, row locks, unique inbox keys, ledgers, and outbox
- Additive guarded migrations plus fresh-bootstrap and isolated schema verification
- Existing item UIDs, player revisions, locker snapshots, and auction transactions as
  migration inputs subject to reconciliation
- Python, shell, and isolated MySQL regressions under `tests/async/`
- Non-production crash-point, duplicate-replay, two-character, and 25-to-200-client tests

### Risks

- A retry can duplicate value if any producer regenerates an operation ID: create the
  ID once, journal it with the immutable payload, and assert inbox uniqueness at every
  destination.
- Multi-entity commands can deadlock or interleave: reserve game keys and lock database
  rows in one documented canonical order, classify deadlocks, and retry the same ID.
- Publishing memory before commit can expose false success: fence affected actions and
  apply only committed result values on the game thread.
- Generic checkpoints can overwrite transactional rows: remove domain-owned fields
  from snapshot apply or guard them by their independent domain revisions.
- Legacy ledger or owner history can be incomplete: seed explicit baselines, run
  read-only discrepancy reports first, quarantine ambiguous rows, and never fabricate
  historical operation identity.
- Outbox backlogs can hide integration failure: bound bytes and age, retain rows until
  delivery ACK, and expose destination-specific retry and dead-letter state.
- Broad producer coverage can miss a spend or transfer: maintain source inventories and
  fail the final gate while any direct critical mutation or raw durable SQL route remains.
- Logs and outbox payloads can disclose private state: use typed bounded fields and
  Phase 00 redacted site/operation metadata without raw SQL or private descriptions.

### Relevant Considerations

- [P00] **Critical domains need one durability boundary**: Every domain transaction
  includes current state, ledger or audit, revisions, inbox identity, and outbox rows.
- [P00] **Revision and acknowledgement identity are mandatory**: Operation identity and
  domain revisions survive retry, replay, reconnect, and stale completion.
- [P00] **Queues and recovery must be typed and bounded**: Critical commands extend the
  Phase 01 journal without unrestricted SQL payloads or unbounded memory growth.
- [P00] **Do not absolute-save shared balances**: Bank and wallet changes are checked
  deltas whose committed results are published to online characters.
- [P00] **Do not split balance, ownership, ledger, and audit writes**: Sessions 03
  through 08 make those effects one transaction for each accepted command.
- [P00] **Do not treat raw SQL as a queue or journal contract**: Session 12 inventories
  and removes every remaining non-idempotent raw durable message producer.
- [P00] **The game thread owns mutable objects**: Workers receive immutable commands and
  return values; no worker traverses live players, objects, guilds, rooms, or descriptors.
- [P00-S05] **Economy and ownership updates lack atomic integrity boundaries**: This
  phase is the planned remediation and closes the finding only after reconciliation and
  fault tests pass.

---

## Success Criteria

Phase complete when:
- [ ] All 12 sessions completed and validated
- [ ] Every accepted critical gameplay command has one stable operation ID reused by
      queueing, journal replay, database inbox, ledger, outbox, completion, and logs
- [ ] Duplicate submission or replay at every crash point produces one durable domain
      effect and one logical outbox event set
- [ ] Ambiguous commit responses are reconciled by operation ID without blind reapply
- [ ] Epic opening balance plus immutable deltas reconciles exactly to every current
      player epic balance, and every award or spend is recorded once
- [ ] Bank and wallet denomination deltas, both relevant revisions, ledger, and outbox
      commit atomically, and every online account character receives committed balances
- [ ] Every transferable durable item has one authoritative current owner, and each
      transfer commits its ownership ledger, all affected inventory revisions, and
      outbox rows atomically
- [ ] Player, floor, trade, corpse, locker, and auction failures retain the pre-command
      live or durable custody state until exact successful completion
- [ ] PvP, artifact, guild, boon, reward, and zone outcomes use bounded immutable typed
      commands and set-based apply without per-recipient raw-query fan-out
- [ ] No unrestricted raw SQL queue or fallback can carry a non-idempotent gameplay
      effect, and legacy event records remain preserved according to compatibility rules
- [ ] Normal critical mutation paths perform no database, Redis, or filesystem I/O on
      the simulation thread and publish no final success before durable ACK
- [ ] Queue, journal, inbox, outbox, retry, deadlock, operation age, reconciliation, and
      domain-fence health are bounded, redacted, and observable
- [ ] Epic, currency, and current-owner rows reconcile exactly with their ledgers after
      every duplicate, outage, worker-crash, game-crash, and 25-to-200-client workload
- [ ] Relevant focused tests, formatting checks, `make -C src`, and the full repository
      gate pass on non-production systems

---

## Dependencies

### Depends On

- Phase 00: Correctness and Immediate Lag Removal
- Phase 01: Replace Forked Full Saves
- Phase 01 carryforward, documentation, recovery-gate, and schema evidence before execution

### Enables

- Phase 03: Load Path, Schema, and Retention
- Authoritative current-item ownership for batched login validation
- Reconciled ledgers and outbox history for retention, archival, and data-rights policy
- Critical-domain latency and correctness evidence for the final 200-player gate
