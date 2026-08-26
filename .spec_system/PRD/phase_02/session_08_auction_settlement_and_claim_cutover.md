# Session 08: Auction Settlement and Claim Cutover

**Session ID**: `phase02-session08-auction-settlement-and-claim-cutover`
**Status**: Not Started
**Work Window**: The full auction economic and custody lifecycle from listing and bid
escrow through close, refund, item/money claim, committed publication, and replay-safe
offline settlement.

---

## Objective

Make auction listing, bidding, settlement, refunds, and claims idempotent transactions
over wallet value and item custody, including offline participants and external
notifications.

---

## Scope

### In Scope (MVP)

- Inventory listing, removal, bid, buy-now, expiration, finalization, refund, item
  pickup, money pickup, web broadcast, and offline message paths after earlier cutovers.
- Move listed items from player to auction custody through Session 05 and charge the
  listing fee through Session 04 in one stable multi-domain command or compensatable
  transaction contract that never publishes a half-result.
- Represent bid holds, increases, outbid refunds, settlement proceeds, fees, and money
  claims as operation-keyed currency effects rather than in-memory subtraction plus
  later player saves.
- Commit auction status, item owner, bidder/seller wallet effects or durable claims,
  ownership/currency ledgers, revisions, inbox result, and outbox rows atomically.
- Preserve stable item identity through auction serialization and quantity handling;
  make item and money claims conditional, idempotent, and safe under concurrent pickup.
- Publish WebSocket updates, player messages, and offline notices only from committed
  ACK/outbox results and dedupe them by outbox identity.
- Reconcile existing open auctions and staged pickups before enabling the new contract;
  quarantine conflicting owner or claim state without destructive repair.

### Out of Scope

- Auction user-interface redesign, pricing changes, or anti-shill policy changes.
- General WebSocket architecture outside transactional auction notifications.
- Phase 03 retention or query-index tuning.

---

## Prerequisites

- [ ] Sessions 04 and 05 currency and ownership transactions are validated.
- [ ] Session 02 outbox delivery and consumer dedupe are available.
- [ ] Existing auction InnoDB and row-lock regressions remain passing.

---

## Deliverables

1. Typed listing, bid, settlement, refund, and claim command adapters in
   `src/auction_houses.c` and focused domain modules.
2. Additive auction operation/claim schema updates and verification under `migrations/`
   where existing tables cannot carry stable identities and revisions safely.
3. Ownership/currency transaction composition plus committed WebSocket/offline outbox
   publication.
4. Focused concurrent bid, buy-now, expiration, no-bid return, refund, offline seller,
   item/money claim, crash, replay, and reconciliation tests under `tests/async/`.

---

## Success Criteria

- [ ] Listing can neither charge a fee without durable auction custody nor remove an
      item without the corresponding committed auction row.
- [ ] Bid, outbid, buy-now, expiration, settlement, and claim retries change wallet,
      claim, item owner, and auction status at most once per operation ID.
- [ ] One item or money claim can be consumed successfully once under concurrent calls.
- [ ] Offline seller/buyer results survive restart and publish from durable outbox state.
- [ ] WebSocket and player notifications occur only after commit and dedupe on retry.
- [ ] Existing open auctions reconcile to one owner/claim state or remain safely
      quarantined without item or currency deletion.
- [ ] Generic player checkpoints cannot overwrite auction-related wallet or ownership
      results.
- [ ] Focused regressions, isolated schema tests, formatting checks, and `make -C src`
      pass.
