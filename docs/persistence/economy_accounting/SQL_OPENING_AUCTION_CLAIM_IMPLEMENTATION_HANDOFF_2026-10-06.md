# SQL auction and claim opening implementation handoff

Source checkpoint: `6083deabbfe31fcad13699670fd5d5cdf09e245c`.
This is the remaining implementation boundary for existing R6 opening capture,
not a new acceptance gate or a qualification result. The independent persistence
review and primary source inspection agree on the gaps below. No production
authority or data was changed.

## Native opening selection

`src/persistence/economic_sql_accounting_lifecycle_transaction.c` currently
selects wallets, banks, shop treasuries and active coin piles in
`read_native_holdings`; `make_batch` supplies only these money holdings.
`economic_sql_source_normalize.c` already describes native auction escrow and
pending pickups. Complete selection must retain auction-escrow account kind4 /
locator4 and pending-claim kind5 / locator5 with their actual source rows.

An OPEN listing with a bidder contributes its current bid. OPEN without a bidder
contributes an explicit zero holding and mapping; its asking price is not money.
CLOSED listings remain history. A removed listing retaining an unresolved bidder
must refuse cutover. Preserve zero claim mappings, native PIDs and revisions,
nonnegative native amounts within the existing supported limits, and original
auction bid-event identity. Do not invent a bidder or source operation.

Extend `native_locator`, deterministic mapping allocation and coverage,
`native_digest`, and the baseline batch together. `fill_export` must explicitly
select wallets rather than treating every remaining mapped account as a wallet.
Historical ESM1/EIM1/ESN bytes remain compatible where the new domains are absent;
new contributions need an explicit versioned contract. Do not reseal a stored
request, witness or native hash under an original operation ID. A historical
partial capture cannot become complete through reinterpretation.

## Mapping recovery and spendable claim origin

`create_or_verify_mappings` currently requires every selected mapping to have
the opening preparation's creating operation and revision0. `recover_runtime`
calls that same verifier after gameplay. Separate exact opening replay from
current lifetime/cache verification: later listing/claim mappings and retired
listing mappings must survive cold restart, with original wallet/bank fences
retained. Recovery must not create missing mappings.

After baseline root and positive claim-account effects exist, stage claim origin
using `economic_sql_pending_claim_source_stage` inside the same transaction as
`economic_sql_baseline_transaction::evidence`. Use the actual baseline command's
operation ID, a deterministic nonzero uint16 source slot, actual mapping and PID,
and exact amount. Zero openings have no allocation. Existing migration0039 can
retain whole allocations. Replay must verify immutable origins while allowing
legitimate later consumption; never reinsert or reopen them. Historical EAB1/EAB2
claim witnesses need explicit compatibility treatment, not silent origin backfill.

The current `economic_sql_pending_claim_source_consume` returns ENOTSUP when a
debit splits an allocation. A single aggregate opening therefore permits whole
cashout but fails a smaller claim-funded bid. Complete support needs retained
partial consumption that preserves original source amounts/operations. Coordinate
any new allocation representation with Plan5's independent readers through this
narrow interface. Per-coin synthetic sources and bypassing the balance check are
not solutions.

## Existing qualification boundary

Use the original lifecycle/source-capture and native auction/claim suites on both
supported SQL engines when the coherent implementation is ready. Cover install,
exact replay and lost reply; atomic allocation rollback; whole cashout and smaller
claim-funded bids; zero-escrow first bid, outbid and settlement; restart after new
and retired mappings; and replay after legitimate allocation consumption. Preserve
the original flags, deadlines, assertions and actual providers.

The separate equipment v2 source repair is a prerequisite for item opening, not
completion of this money slice. Full stopped-world physical capture, complete item
forests, supported writer evidence and the authentic independent activation
verifier remain existing R6 work. No full Plan, R1-R8 or release pass is claimed.
