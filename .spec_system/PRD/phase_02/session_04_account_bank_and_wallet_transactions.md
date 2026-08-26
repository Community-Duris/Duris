# Session 04: Account Bank and Wallet Transactions

**Session ID**: `phase02-session04-account-bank-and-wallet-transactions`
**Status**: Not Started
**Work Window**: The complete shared-currency boundary across one account bank, one
player wallet, multi-denomination commands, online alternate characters, revisions,
ledger, outbox, and reconciliation.

---

## Objective

Commit each deposit, withdrawal, refund, pickup, or reward delta once across the
authoritative account bank and player wallet, then publish only the committed balances
to the initiating character and every online alternate on that account.

---

## Scope

### In Scope (MVP)

- Inventory account-bank and wallet reads, writes, absolute saves, delta helpers,
  deposit-all behavior, refunds, pickups, cash rewards, and direct coin mutations that
  cross a durability boundary after Phase 01.
- Add an immutable operation-keyed currency ledger recording denomination vectors,
  wallet and bank deltas, resulting balances, actor/source, and required revisions.
- Lock account-bank and player-wallet rows in canonical order; validate nonnegative
  balances and bounds; apply all denominations, both revisions, ledger, result, and
  outbox in one Session 02 transaction.
- Treat `deposit all` as one immutable vector command and return authoritative results
  from the transaction rather than a pre-update select or cached arithmetic.
- Remove absolute bank persistence and prevent generic player checkpoints from
  overwriting transactional wallet columns or shared account balances.
- Gate overlapping economy actions until completion and publish committed bank balances
  to every online character for the same account and racewar on the game thread.
- Add baseline/reconciliation tooling and two-character concurrency, crash, duplicate,
  insufficient-funds, overflow, stale-cache, and reconnect tests.

### Out of Scope

- Auction escrow and claim lifecycle, owned by Session 08, though it must call the
  currency transaction API rather than mutate wallets directly.
- Boon and reward producer batching, owned by Session 11.
- Guild treasury redesign or Phase 03 retention policy.

---

## Prerequisites

- [ ] Sessions 01 and 02 are validated.
- [ ] Phase 00 account-bank delta-only containment is complete and its call-site
      inventory is available.
- [ ] Currency schema and fault tests use isolated non-production databases.

---

## Deliverables

1. Currency ledger and revision schema, fresh-bootstrap synchronization, and verification
   under `migrations/`.
2. Typed multi-denomination bank/wallet command, repository, completion, and
   reconciliation modules under `src/`.
3. ATM, refund, pickup, reward-adapter, online-alt publication, and player-checkpoint
   integration across the audited call sites.
4. Focused two-character, deposit-all, withdrawal, stale-alt, duplicate, crash, bounds,
   and reconciliation regressions under `tests/async/`.

---

## Success Criteria

- [ ] Wallet delta plus bank delta nets to the intended command value and commits with
      one ledger row set, both revisions, inbox result, and outbox set.
- [ ] Duplicate or ambiguous replay cannot create or destroy currency.
- [ ] Insufficient funds, overflow, invalid denomination, or revision conflict changes
      no durable or published balance.
- [ ] `deposit all` is atomic across its complete denomination vector.
- [ ] Every online account character receives the committed shared balance, while stale
      cached values can never be absolute-saved back to SQL.
- [ ] Generic checkpoints cannot overwrite transaction-owned wallet or bank values.
- [ ] Current wallet and bank rows reconcile exactly with the operation ledger after
      concurrent, crash, retry, and restart tests.
- [ ] Focused regressions, isolated schema tests, formatting checks, and `make -C src`
      pass.
