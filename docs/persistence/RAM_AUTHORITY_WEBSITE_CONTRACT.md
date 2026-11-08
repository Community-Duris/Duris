# RAM authority and website auctions

## Agreed requirements

The MUD must be running and ready to accept bids. The bidding character may be
logged out. Both website and in-game bids must go through the MUD's sole economic
decision maker. Durable storage retains recovery evidence and read projections;
it must not independently decide or mutate live auction and wallet state.

This document specifies the integration contract. It does not mean that the RAM
cutover or the website bid command has been implemented or enabled.

## One operation, one acknowledgement

The authenticated website submits an account, a character belonging to that
account, an auction, a bid amount and a stable request ID. The MUD checks account
ownership, eligibility, the current auction and available funds on its game
thread, in the same order as in-game commands. The website's displayed price is
informational; it cannot authorize a debit.

An accepted operation records all coupled effects: the bidder's debit, the
auction's new winning bid and deadline, escrow and any previous bidder's refund.
A buyout or settlement includes the item claim and seller's proceeds. The log
must durably record the entire outcome before the MUD publishes success. Failure
before that boundary cannot debit the wallet. An uncertain append retains its
identity and blocks affected mutations until recovery determines the outcome.

Retries reuse the original request ID and intent. A lost response must return
the original committed receipt, including after a MUD restart or eviction of a
short-lived response cache. Reusing an ID for another bid must be rejected.
The website may display `pending` while waiting; queue admission alone is not
successful bidding. It must not fall back to database writes on timeout.

## Logged-out wallets and login

The authoritative wallet belongs to the MUD's economy state, not to a network
descriptor. A full character body does not need to remain loaded to keep its
wallet available for bidding. Offline economic state may be loaded from a
verified recovered base plus the operation-log tail and then retained by the
MUD.

Login and logout must transfer access to that same economic state at an ordered
boundary. Login may not install an old database balance over newer accepted
web bids. Logout may not create a second offline wallet while a live character
can still spend. Shared account-bank state needs the same single-owner rule.

Recovery must complete before either website or game commands can mutate these
domains. Auction deadlines use the MUD's authoritative clock and accepted
command order. Delayed database projection must not reinterpret a previously
accepted bid using a later wall-clock time.

## Website implementations inspected

The inspected revisions are
[duriswebapp c0982794](https://github.com/Community-Duris/duriswebapp/tree/c0982794e75d19b4cef303ec656650e3c585c3d8)
and
[DurisWebSTDB 987e76e8](https://github.com/Community-Duris/DurisWebSTDB/tree/987e76e8633fd89d5b60bd82bf3b98449e9d7535).

Both have auction bid and buy routes that call `deductCharacterMoney` separately
from `placeBid`. These routes are guarded by the `auctionWrites` mutation gate,
controlled by `ALLOW_UNSAFE_AUCTION_WRITES`, which is closed by default. The gate's
own explanation identifies the split transaction and bypass of MUD authority.
Keep it closed until these routes use the authenticated MUD command above.

Both already route administrative auction removal through the MUD bridge. That
provides an integration precedent, but its current acceptance response is not
proof of a committed bid or refund. A bidding endpoint needs a durable result or
an explicit pending receipt that can be queried and retried safely.

Switching to DurisWebSTDB does not by itself change this boundary. SQL or
SpacetimeDB may serve a read projection, but neither may become an independent
writer for the same wallet, escrow or auction. Read projections can lag; the
successful bid response should carry the new authoritative auction version so
the interface does not replace it with an older listing response.

## Required regression journeys

- An offline character bids on the website, then logs in with the resulting
  wallet and auction state, including while database projection is delayed.
- Website and in-game bids race for one auction and are resolved in one order;
  the losing bid cannot leave a debit behind.
- A response is lost after durable acceptance, and a retry before and after
  restart returns the same receipt without a second debit or refund.
- A crash between log append, RAM publication and database projection recovers
  the whole bid, refund, settlement or transfer.
- Logout during a pending bid cannot expose two independently spendable wallets.
- Removing an auction with a winning bid returns the item to its seller and
  refunds the bidder together. Refused refunds roll back the removal, and a
  repeated removal cannot refund twice.
- With the MUD stopped or recovering, both websites refuse new mutations while
  remaining free to display the last available read projection.
