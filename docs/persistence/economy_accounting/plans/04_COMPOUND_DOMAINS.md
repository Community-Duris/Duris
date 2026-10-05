# Plan 4: priced and compound gameplay domains

Start from add-double-entry HEAD 49af585c4. Build each domain as a standalone
inactive typed transaction and journey, using current money and item codecs
directly; do not wait for Plans 2 or 3 to activate. This plan owns shop,
collector, auction, crafting/refining, death/resurrection, and recovery
composites. See [R5](../REMAINING_REQUIREMENTS.md).

## Result

Whenever one existing critical mutation changes both value and custody, its
single commit contains the exact balanced money postings, ordered item events,
domain state, receipt, and outbox. Separate critical mutations remain separately
atomic and are linked by durable lineage.

## Starting files and first checks

Inspect src/economy/shop_trade_transaction.c,
src/economy/collector_service.c, src/economy/auction_repository.c,
src/persistence/corpse_lifecycle_transaction.c, src/economy/crafting.c,
and src/economy/tradeskill.c. Start with
python3 tests/async/test_shop_trade_accounting_context.py,
python3 tests/async/test_collector_accounting_context.py, and
python3 tests/async/test_auction_accounting_context.py. Use
tests/async/test_death_resurrection_mysql_journey.py only with its
disposable database and real-PC fixture.

## Work

1. For shop buy/sell and keeper cash, balance transactions against a shared system
   sink/issuance account rather than per-keeper bank accounts. Preserve keeper
   identity, shop ID, and dynamic item properties on metadata and ownership logs.
   Preserve SQL and flatfile semantics and test a failed cash/custody leg.
2. For collector purchases, record the money sink and item custody/destruction
   under the accepted root. Preserve disabled gem barter as refusal.
3. For auction listing, bid, outbid, settlement, claim collection, and trusted
   removal, give escrow and claims durable lifetimes and source links. Outbid funds
   credit automatically to the bidder's pending claim account (credit balance)
   instead of filling their physical wallet. New bids apply available claim credit
   first, debiting only the remaining delta from the wallet, with structured
   command feedback. Settlement spends escrow, never the buyer wallet a second time.
   Keep current trusted removal behavior, including the absence of an automatic reimbursement.
4. For crafting, forge, smith/refine, and spells that consume inputs, bind the
   selected outcome, input UIDs, costs, output source event, and item grant.
   Record intended gameplay failure as consumed inputs/cost with no output when
   that is the existing rule. A technical commit failure must roll back all
   effects and must not reroll a retained outcome.
5. For player death, corpse creation/loot, resurrection, and restitution,
   account for wallet-to-pile creation, pile custody/value consumption, original
   item handoffs, pre-claim drops, and restored wallet. Keep each death batch
   and later resurrection as its own operation. Death async custody drain must
   have a strict bounded timeout; if delayed, transition into sealed disputed
   custody and unconditionally release the descriptor to the account menu so
   characters are never stranded in limbo. Resurrection reverses unlooted corpse
   items back to the player; any temporary gear currently equipped by the player
   is dropped to the room floor before corpse items transfer.
6. Port supported composites to flatfile after SQL proof. Explicitly refuse
   any unported active-epoch domain route before the first native mutation.

## Independent acceptance

- Each domain has a disposable SQL journey on MySQL and MariaDB proving its
  native state, zero-sum postings, exact item references, result, replay,
  restart, and failure rollback. Repeat the same policy and essential faults
  on flatfile.
- The real-PC death/resurrection journey proves the entire wallet/pile/item
  sequence, including 12 original item UIDs in the existing fixture, without
  claiming one transaction across death and resurrection. The unassisted
  death-conflict probe reaches the account menu with durable recovery visible.
- Cases include auction outbid/removal/collection, keeper cash exception,
  collector sink, shop fee, smithing success and intended failure, replayed
  craft outcome, and rollback after an intermediate child.
- Focused domain tests and both server builds pass. Existing legacy domain
  ledgers alone do not satisfy these assertions.

## Boundary and handoff

This plan may add a narrow domain-specific typed adapter at the existing
repository owner but must keep common admission changes in Plan 1's interface.
It exports durable route evidence to Plan 5. It can qualify one domain at a
time; no domain success is a whole-game activation claim.

### October4 collector/shared owner source milestone; unqualified

The active SQL collector purchase now connects typed policy preparation, exact
original domain retention, save hold/coordinator seal, complete native payload
storage and guarded online/offline publication. Cold replay registers only the
original command before execution; successful actorless projection cannot mint
a UID, fabricate a receipt or invoke a physical effect with a null actor.
Rejected missing cache and incomplete materialization retain their obligations.
Schema1/inactive behavior is preserved by source review; flat collector stays
unsupported. Source-only integration is not collector/player/restart acceptance.
The original major-plan batch, other compound domains and full backend coverage
remain required; see the consolidated report for the31-file input scope.


### October 4 accounted shop source prerequisite

The [shop contract source milestone](../SHOP_ACCOUNTED_CONTRACTS_2026-10-04.md)
freezes v6 original status and exact item after-image facts and the agreed shared
sink/issuance capability. Source review and formatting pass; native/runtime
qualification is deferred to major-plan readiness. Complete native payload
loading/saving, migration, producer/admission/publication and cold recovery remain
open; active shop refusal remains. No complete writer or release proof is claimed.
Plan5 backup/cold-restart slice94e81480b is imported and pushed in4c2abb329, with
three exact peer blobs and Python AST checked; its native evidence remains tied
to the peer's frozen older source. Matching v3 discovery and combined qualification
remain pending.
