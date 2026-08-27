# Session Specification

**Session ID**: `phase02-session08-auction-settlement-and-claim-cutover`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `f49a1dea`

## Objectives

1. Represent auction listing, bid, close, and item/money claim as typed, operation-keyed
   commands whose complete economic and custody effects commit once.
2. Keep wallet and item current-state revisions in the same transaction as auction state,
   durable claims, inbox result, and notification outbox rows.
3. Publish live item removal/addition, wallet balances, messages, and web notifications
   only after an exact committed completion.
4. Reconcile existing open auctions and pickup rows without inventing missing ownership
   or deleting ambiguous custody/value.

## Design Boundary

Auction rows own lifecycle state; item authority remains in `item_current_owner`, and
wallet authority remains in revisioned player currency rows. Serialized item blobs are
compatibility payloads for reconstruction, never proof of current custody. A single
auction domain repository composes those authorities under canonical row locks and the
generic critical inbox/outbox transaction.

Accepted commands retain pointer-free scalar/blob snapshots. The game thread fences the
affected live participant/item until ACK and then applies the returned authoritative
result. Offline settlement writes durable claims and notification outbox entries only.

## Success Criteria

- [x] Listing fee, auction row, and auction item custody commit together once.
- [x] Bid holds, outbid refunds, close proceeds, and claims are revision-checked and idempotent.
- [x] Item and money claims can be consumed once under concurrent/replayed operations.
- [x] Live and web notification publication occurs only after committed ACK/outbox state.
- [x] Existing rows reconcile or quarantine without destructive repair.
- [x] Focused MySQL, source, format, build, security, and full-suite gates pass.
