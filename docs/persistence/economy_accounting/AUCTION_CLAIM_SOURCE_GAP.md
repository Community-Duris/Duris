# Pending auction-money source and collection evidence

`auction_money_pickups` is one aggregate balance and revision per player. It is
shared by auction refunds and proceeds and by epic, ship, boon, and utility
writers. The aggregate and `auction_ledger` alone do not identify every source
operation, especially the former bidder receiving an outbid refund.

The inactive typed SQL bid and settlement adapters now write
`economic_pending_claim_source` rows in their native transaction. Each positive
credit has a source accounting operation, a distinct source slot, the exact
beneficiary, claim lifetime mapping, and copper amount. The helper checks that
the source root's account effect credits the same mapped claim by that amount.
Buy-now outbid uses slots 1 and 2 for the former bidder and seller; timed
settlement uses slot 2 for the seller. Zero proceeds have no source row.

The inactive typed money-claim adapter locks the active epoch, mapped wallet,
bank, and pending claim, native aggregate, and all open source rows. Admission
freezes the ordered source-set digest, source count, aggregate value and revision,
and mapping IDs. Before the native pickup it requires every source row to belong
to that claim lifetime and requires the exact source sum to equal the aggregate.
The native wallet credit, balanced claim-to-wallet postings, source consumption
links, root, receipt, and outbox share one caller-owned transaction. A source
change, legacy/unattributed credit, or source-link failure refuses or rolls back
the entire pickup. The first source operation is the claim's original operation;
all contributors remain individually linked through the allocation rows.
The claim also writes `economic_accounting_source_claim` using the first
allocation's source slot. This keeps a former bidder's refund and seller's
proceeds from the same buy-now operation distinct in the logical source-event
index.

The disposable `run_auction_claim_source_schema_windows.ps1` runner passed the
bid/outbid, timed settlement, and collection journeys against MariaDB 10.11 and
MySQL 8.4. It forces late source insertion and consumption failures, verifies
native rollback and exact source rows, tests mixed unattributed money and a
changed source after admission, and checks receipt, plan, source links, and
result after reconnect. The integrated bid journey also collects both buy-now
credits and verifies their distinct source claims, native pickup balances,
postings, and outbox entries. A pure claim regression checks balancing and rejects
an unaccounted aggregate. The new SQL table has a re-runnable schema file and
is included in fresh bootstrap and lifecycle inventory.

The inactive flatfile bid and settlement owners now journal the same positive
credit source rows with their native pickup and EAP1 records. The inactive
flatfile money-claim owner requires the complete unconsumed source set to match
the native aggregate and frozen digest. It journals wallet credit, pickup
clearance, individual source consumption, the balanced EAP1 plan, and the first
source's claim marker together. The focused journey checks stale sources,
source-file corruption, interrupted recovery, replay, and collection of both
a one-source refund and a two-source seller payout.

These components are inactive. Existing non-auction writers still stage money
without source rows and therefore prevent typed collection of that aggregate
until they are ported or separately corrected. The SQL source table is
registered as immutable migration 0039; this work did not apply migrations to
any database. A claim is bounded to 4096 source rows; a larger aggregate is
safely refused until a batch policy exists.
