# Auction listing accounting route evidence

The schema 2 auction listing component has inactive SQL and flatfile owners.
The SQL owner uses the existing native listing action: one accepted operation creates the
auction row and custody slots, moves each original item UID from seller to
auction ownership, charges the listing fee, and writes the native auction,
currency, and item ledgers. The typed owner adds an EAP1 root, account effects,
balanced postings, and exact item references in that same transaction. The
caller writes the receipt and outbox before committing once.

`auction_listing_accounting_intent` freezes the seller's wallet and bank
lifetime mappings. The command binding fixes the item UID and revision sequence,
actual listing price, buy price, fee, and object payload. The new auction ID
cannot be frozen before admission because the native `auctions` table allocates
it. After the native listing insert, the SQL owner creates a durable auction
escrow mapping with `creating_operation_id` equal to the listing operation. It
verifies that mapping against the retained active epoch before writing the
plan. The zero-balance escrow effect opens revision 1, which is the revision
read by the later bid adapter. The listing operation is the source and
generation for subsequent auction operations; listing has no predecessor
operation.

Before native mutation, `economic_sql_auction_listing_lock` checks the active
epoch and wallet/bank mappings. The execute function renews those locks and
reads the seller's actual wallet, bank, owner revision, and each original item
row. A root with descendants is refused because the current native handoff
moves only roots. The plan compares native result balances, fee, auction
revision, owner revisions, and ordered item UIDs/revisions to that locked
authority. It posts the fee from wallet to listing-fee sink 26, with exact
denomination change. A zero-fee listing still records a zero-value wallet
posting if native canonicalization changes denominations. Every item event
references its matching `item_ownership_ledger` row by operation and event
index. Any failed native or accounting step requires the caller to roll back
the whole transaction.

The flatfile owner decodes the canonical frozen intent and renews the active
epoch and seller wallet/bank mappings under one authority lock. It snapshots
the native balances and item custody before preparing the listing. After the
native listing assigns its deterministic auction ID, it stages a new escrow
mapping whose creating operation is the listing, verifies the native result
with the same pure plan, and commits the auction catalog, wallet fee, item
custody, exact item reference, escrow mapping, and EAP1 record in one journal.
It refuses roots with descendants because the native auction handoff moves
only the selected roots. Native refusals retain an empty-plan accounting
record. The focused flatfile harness forces a journal interruption, recovers
the complete listing and mapping, checks balanced fee and custody evidence,
and verifies replay and the descendant refusal. This typed owner can be called
directly; common admission still refuses schema 2 auction listings.

The focused pure regression checks a paid listing, original UID transfer,
result tampering, and zero-fee denomination canonicalization. The disposable
SQL journey lists two original UIDs, forces failure while inserting a late item
reference, verifies rollback of auction, wallet, ownership, mapping, and all
ledgers, retries the same command, and checks both ordered item references,
the two balanced fee postings, escrow lifetime, native state, receipt, and
outbox. It rejects a retained lock after pausing the epoch. After reconnect it
checks the stored EAP1 plan and receipt and confirms the inbox key blocks a
duplicate insert. This journey passed on MariaDB 10.11 and MySQL 8.4. Run it
with `bash tests/async/run_auction_listing_sql_accounting_schema_mysql.sh`,
setting `AUCTION_ACCOUNTING_DB_IMAGE=mysql:8.4` for MySQL. The wrapper uses a
disposable database and removes its container. On a Windows host with Docker
CLI and Ubuntu WSL, run
`tests/async/run_auction_listing_sql_accounting_schema_windows.ps1 -Image mysql:8.4`.

The route remains inactive. Common admission and publication integration is
Plan 1 work. Inactive SQL and flatfile bid, settlement, item collection, and
money collection owners now follow this listing lifetime. Successful sale and
no-bid expiry retire the empty escrow mapping; trusted removal retains its
funded mapping. See the [bid and settlement evidence](AUCTION_BID_ROUTE_EVIDENCE.md)
and [pending claim source evidence](AUCTION_CLAIM_SOURCE_GAP.md). Claim-right
assignment still needs an exact EAP1 reference, and the inactive components
still need full live-route integration.
