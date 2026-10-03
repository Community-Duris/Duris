# Auction bid, settlement, and item claim accounting route evidence

The schema 2 auction bid, settlement, and item claim components are inactive
SQL transaction paths for an existing listing. The listing component is
documented in [auction listing route evidence](AUCTION_LISTING_ROUTE_EVIDENCE.md).
The bid component accepts an opening bid, a higher bid from the same player,
an outbid, and an immediate buy-now settlement.
The settlement component closes an existing auction after expiry or trusted
removal, and the item claim component transfers an item already staged for the
winning bidder or seller. The inactive typed money-collection component is
described in [pending claim source evidence](AUCTION_CLAIM_SOURCE_GAP.md).

`auction_bid_accounting_intent` freezes the listing operation, current auction
revision and price, prior winning bid operation, and the wallet, bank, escrow,
and applicable claim lifetime mapping IDs. Its source event names the prior bid
when there is one, while `original_operation_id` names the listing. The plan
debits the buyer wallet once. For an outbid it moves the previous escrow holding
to the former bidder's claim. A buy-now bid spends the resulting escrow into
seller proceeds and a closing-fee sink. The item remains in auction custody;
the native sale assigns its claim to the winner for a later item-claim
transaction.

The caller starts an inbox transaction, calls `economic_sql_auction_bid_lock`,
then `economic_sql_auction_bid_execute_and_record`. The component renews the
active epoch and lifetime mapping locks, compares frozen listing facts with
locked SQL rows before mutation, runs the native auction repository, and writes
the EAP1 root, account effects, and balanced postings in that transaction. The
caller writes the receipt and outbox, then commits once; any error requires
rollback. The legacy auction entry point remains schema 1 only.

`auction_item_claim_accounting_intent` freezes the staged claimant, auction
revision, listing and terminal staging operations, and exact item UID, slot,
revision, and VNUM sequence. The SQL component locks the active epoch and actor
wallet and bank lifetimes, then compares the frozen claim with native auction,
custody, owner revision, and item rows. It refuses a root that has descendants.
The native `claim_item` mutation transfers each original UID from auction to
player custody and writes `item_ownership_ledger` in the same transaction. The
component writes one EAP1 item event and an exact
`economic_accounting_item_reference` to each native ledger row, with no money
posting. Its source event names the sold, expired, or removed auction ledger
operation that staged the claim; its original operation names the listing.
The caller stores the result, receipt, and outbox before committing. A native
or accounting failure rolls back the entire transaction.

`auction_settlement_accounting_intent` freezes the listing, last winning bid,
escrow and seller-claim lifetimes, closing price, fee policy, and every staged
item UID. The SQL component locks the active epoch and mappings, verifies the
open native auction and singleton custody rows, runs the existing `finalize` or
`remove` repository action, then checks the changed auction, claim balance,
and item claim rights. A timed sale spends the bid's escrow into seller pending
claim and a fee sink with zero-sum postings; it does not debit the buyer again.
Expiry without a bidder has no money postings. Trusted removal leaves the
winning escrow amount held and stages the original item for its seller, with
no automatic bidder reimbursement. Closure changes claim rights but not item
ownership; its EAP1 plan retains unchanged UID custody witnesses. A later
`claim_item` operation records the exact ownership event.

The disposable SQL journey uses a native auction row and a custody item. It
forces a posting-write failure after the native bid, rolls back, retries the
same command, and then commits an outbid buy-now. It checks wallet and claim
rows, item claim assignment, the exact seven balanced posting legs, source IDs,
receipt/outbox, rejection of a retained context after an epoch change, and
retained intent, plan, and result bytes after reconnect. It then forces an item
reference failure after native claim transfer, verifies rollback of custody and
revisions, refuses a child-bearing root before transfer, retries the same claim,
and checks the original UID, item reference,
sale source, receipt/outbox, and retained plan and result after reconnect. The
combined journey passed against MySQL 8.4 and MariaDB 10.11. Run it with
`bash tests/async/run_auction_bid_sql_accounting_schema_mysql.sh` or set
`AUCTION_ACCOUNTING_DB_IMAGE=mysql:8.4` for MySQL. The runner creates a
disposable database and removes its container.

The combined journey now also collects the former bidder's refund and seller's
proceeds from the same buy-now bid. It verifies distinct source slots and source
claim events, zero native pending balances, both wallet credits and balanced
claim-to-wallet postings, then confirms the retained claim evidence after
reconnect. This extension passed against MariaDB 10.11 and MySQL 8.4.

A separate disposable settlement journey seeds a native open listing and
winning-bid ledger source. It forces late accounting failures after native
sale and removal mutations, verifies rollback, then commits timed sale,
trusted removal, seller item collection, and no-bid expiration. It checks
seller claim value and revision, the exact balanced sale legs, held removal
escrow, unchanged item ownership at closure, the original UID and exact
reference at seller collection, source links, receipt/outbox, epoch refusal,
and retained plans/results after reconnect. It passed on MariaDB 10.11 and
MySQL 8.4. Run it with
`bash tests/async/run_auction_settlement_sql_accounting_schema_mysql.sh`,
setting `AUCTION_SETTLEMENT_DB_IMAGE=mysql:8.4` for MySQL.

These components do not activate an auction route. Plan 1 still owns common
admission, receipt/outbox ownership, and replay verification. The bid and
settlement routes require preexisting mapped escrow and claim lifetimes. The
SQL source table is registered as immutable migration 0039. Buy-now assigns
`auction_item_custody.claim_pid` in the native row; the later item claim records
the actual ownership transfer. Claim-right assignment itself still lacks a
separate EAP1 exact reference. Non-auction money producers still need source
allocations before a mixed aggregate can be collected by the typed route.
Successful buy-now, timed sale, and no-bid expiry retire their zero-balance
escrow mapping in the same SQL transaction as the native closure, EAP1 plan,
source rows, receipt, and outbox. Trusted removal retains the funded escrow;
its eventual disposition remains a separate lifecycle decision. The SQL bid
journey forces the retirement update to fail and verifies full rollback; the
bid and settlement journeys passed against an isolated MySQL 8 server for this
change. MariaDB qualification of this retirement change remains pending.

Flatfile lifetime metadata now accepts native auction IDs for escrow mappings
and player IDs for pending-claim mappings, with retained identity and tombstone
checks. The authority regression resolves both through the active-epoch lock,
rejects a mismatched claim locator, and retains the retired escrow lifetime
after recreation. An inactive flatfile schema-2 item-claim owner now verifies
the frozen listing and terminal claim source against the native catalog, then
commits custody, the EAP1 root, and exact item references through one authority
journal. Its focused journey covers stale source refusal, interrupted commit
recovery, replay, and retained custody/source evidence.

An inactive flatfile schema-2 bid owner decodes the complete frozen listing
and mapped account identities, renews them under the auction authority lock,
and compares the live listing and custody rows before mutation. It records the
native bid and EAP1 plan in one authority journal. The focused flatfile journey
starts with the accounted listing's escrow lifetime, then covers an opening
bid, an incremental raise by the same bidder, stale prior-source refusal, an
outbid with a former-bidder pending claim, and buy-now with seller proceeds and
a closing fee. It verifies source IDs, postings, native pickup rights, exact
replay, and interrupted-journal recovery.
Buy-now also retires its now-empty escrow mapping in that journal.

Inactive flatfile schema-2 finalize and trusted-removal owners now decode the
entire frozen listing and item sequence, renew the escrow, optional seller
claim, and actor mappings, and compare the native listing and custody before
mutation. They retain the changed claim rights and EAP1 closure plan in the
same authority journal. The focused journey covers a timed sale with seller
proceeds and closing fee, interrupted sale recovery, removal with the winning
escrow still held and no bidder refund, and no-bid expiry with no postings. It
checks exact source links, unchanged item custody witnesses, native pickup
rights, stale revision refusal, and replay. These owners are not wired into
the live route. Timed sale and no-bid expiry retire their empty escrow mapping
in the closure journal; trusted removal keeps its funded mapping active. The
inactive typed flatfile money-claim owner journals the exact source set,
pickup clearance, wallet credit, and balanced plan. Its journey covers refund
and multi-source proceeds collection, stale sources, corrupted source files,
interrupted recovery, and replay.

The flatfile auction and collector repositories check the retained accounting
control under their authority lock after exact replay lookup. A new schema-1
command returns `EAGAIN` when an epoch is active, before preparing native state.
A missing accounting installation remains an inactive legacy route; a present
but corrupt control fails closed. This is a refusal boundary for unported
routes, not flatfile EAP1 domain parity.
