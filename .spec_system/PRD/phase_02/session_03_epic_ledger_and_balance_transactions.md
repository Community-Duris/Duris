# Session 03: Epic Ledger and Balance Transactions

**Session ID**: `phase02-session03-epic-ledger-and-balance-transactions`
**Status**: Complete
**Work Window**: The complete epic value boundary across legacy-baseline capture, every
award and spend producer, one balance/ledger transaction, post-ACK in-memory state, and
exact reconciliation.

---

## Objective

Make every epic award and spend idempotent and atomic so the materialized balance,
immutable ledger, player revision, required outbox, and in-memory bonus state agree
after normal execution, retry, and crash recovery.

---

## Scope

### In Scope (MVP)

- Inventory every direct epic increment, decrement, reset, refund, level cost, skill
  cost, ship cost, craft cost, bottle, administrator, and reward producer after Phase 01.
- Add an immutable operation-keyed epic ledger and explicit opening-balance baseline for
  each migrated player; preserve legacy `epic_gain` rows without inventing IDs for them.
- Apply awards and guarded spends by locking the operational balance, checking funds
  where required, inserting one delta and result, updating the balance and domain/player
  revision, and writing required outbox rows in the Session 02 transaction.
- Route all epic mutations through immutable commands and remove epic balance from any
  independent checkpoint apply that could overwrite the transactional result.
- Publish committed balance, rolling bonus contribution, task/level eligibility, and
  player-facing success only from the exact main-thread ACK; retain or restore staged
  state on rejection and retry.
- Provide pre-cutover discrepancy reporting, guarded baseline creation, post-cutover
  reconciliation, and duplicate/ambiguous replay tests for awards and spends.

### Out of Scope

- Full PvP battle-row batching, owned by Session 09.
- Set-based artifact and guild side-effect persistence, owned by Session 10.
- Phase 03 history retention or archive policy.

---

## Prerequisites

- [x] Sessions 01 and 02 critical command, inbox, outbox, and reconciliation contracts
      are validated.
- [x] Phase 00 in-memory epic-bonus behavior is understood and covered by regressions.
- [x] Baseline and migration tests use only guarded non-production databases.

---

## Deliverables

1. Epic ledger/balance migration, fresh-bootstrap update, indexes, and schema verification
   under `migrations/`.
2. Typed epic award/spend command, repository, result, reconciliation, and baseline
   modules under `src/`.
3. Cutover of every audited epic mutation call site and removal of independent balance
   writes from the player checkpoint path.
4. Focused award, spend, insufficient-funds, duplicate, crash, bonus-ACK, and baseline
   reconciliation regressions under `tests/async/`.

---

## Success Criteria

- [x] Opening balance plus every committed delta equals the materialized epic balance
      for each migrated player.
- [x] Every award and spend call site supplies one stable operation ID and reason/type
      metadata, including bottle and administrative paths.
- [x] Duplicate or ambiguous replay changes the balance and ledger exactly once.
- [x] A rejected or failed spend publishes no purchase, level, skill, item, or other
      success and leaves the prior balance authoritative.
- [x] Epic bonus state and user-visible balance update only from the committed ACK.
- [x] Generic player checkpoints cannot overwrite the transactional epic balance.
- [x] Reconciliation reports legacy discrepancies without fabricating historical
      operations or deleting `epic_gain` rows.
- [x] Focused regressions, isolated schema tests, formatting checks, and `make -C src`
      pass.
