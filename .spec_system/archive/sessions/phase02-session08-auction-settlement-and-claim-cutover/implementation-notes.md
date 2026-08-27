# Implementation Notes

- 2026-08-27: Session opened from `f49a1dea` after Session 07 passed and published.
- Auction lifecycle effects now execute through one bounded typed command and the generic
  critical repository transaction. Listing composes fee debit, auction row, per-item
  custody, current owner, ownership ledger, currency ledger, inbox result, and outbox.
- Bid and buy-now commands lock the actor wallet and auction, stage prior-bid refunds,
  close sold auctions, and stage seller/item claims once. Closing fee and extension
  policy are captured in the command so replay does not depend on later configuration.
- Item blobs remain reconstruction payloads only. Stable item UID and revision rows in
  `auction_item_custody` prove custody and drive one-time player claims.
- Live wallet state uses the centralized currency ACK publisher. Item removal/addition,
  player messages, and WebSocket events publish only after committed completion or from
  the durable auction outbox queue on the game thread.
- The command decoder reconstructs and compares the exact player, account, auction, and
  item fence set. The completion envelope was widened to a bounded 512 bytes after the
  MySQL harness exposed truncation of multi-item auction results.
- Legacy open auctions without stable item identity are quarantined and excluded from
  automatic finalization. The additive migration is re-runnable and mirrored in the
  fresh bootstrap.
