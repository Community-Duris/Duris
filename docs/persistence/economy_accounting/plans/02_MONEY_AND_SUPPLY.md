# Plan 2: money holdings, transfers, issuance, and expenses

Start from add-double-entry HEAD 49af585c4. Develop and test in inactive mode
against direct typed owners and isolated backend fixtures; Plan 1's final
activation is not a prerequisite. This plan owns general money writer policy
and adapters. Shops, auctions, collector, death, and crafting composites belong
to [Plan 4](04_COMPOUND_DOMAINS.md). See [R2 and R3](../REMAINING_REQUIREMENTS.md).

## Result

Every ordinary money movement and supply change has durable counterparties,
actual denomination effects, a balanced root, and a source identity where
required. The source of new money and the destination of destroyed money are
queryable by reason and operation.

## Starting files and first checks

Inspect src/economy/currency_transaction.c,
src/economy/coin_transfer_accounting.c, src/cmd/actobj.c,
src/cmd/actoth.c, and src/economy/cardgames.c. Start with
python3 tests/async/test_coin_transfer_accounting.py,
python3 tests/async/test_coin_transfer_shared_bank_accounting.py, and
python3 tests/async/test_currency_completion_retention.py. Use
tests/async/run_currency_transaction_schema_mysql.sh with a disposable
database for native SQL acceptance.

## Work

1. Complete wallet/shared-bank/coin-pile account lifetime and native-revision
   adapters on SQL, then flatfile. Preserve denomination changes, including
   making change. Finish all supported pile create, merge, split, pickup, drop,
   and destruction paths; movement of a pile's custody is Plan 3 unless it
   changes value. Verify the current SQL coin component through the pooled
   path with Plan 1's fixture, or supply a failing integration test now.
2. Classify and bind peer transfers and group split recipients, including
   morphs and remainder. Preserve current per-child and command boundaries;
   do not silently promise a new global bulk atomicity rule. Resolve NPC money
   only after a durable holding lifetime/policy is specified, otherwise refuse
   active-epoch writes before native mutation.
3. Map rewards, loot (including mob death coin generation), quest/chaos/epic
   grants, bartender quest rewards, and admin grants to versioned issuance policy.
   Identify each logical source event before accepting a new operation ID. A failed
   reward that stages a claim must not mint again when collected.
4. Blackjack is permanently deprecated under active epochs. Retain the active-epoch
   refusal guard; do not implement table/round stake recovery. Schedule post-release
   removal of legacy card game sources and zone objects.
5. Make every unsupported cash writer fail at admission in an active epoch.
   Classify direct assignments and special procedures from the writer census,
   removing dead writers only with reachability evidence.

## Independent acceptance

- For each supported route, a disposable SQL test asserts native before/after
  vectors and revisions, exact account effects, at least two postings when
  value changes, zero copper sum, source claim where required, and one receipt.
  Repeat on MySQL and MariaDB; port the same behavior to flatfile.
- Fault tests cover denomination overflow, negative holdings, changed replay,
  source-event reuse with a new ID, endpoint failure, pile publication failure,
  lost reply, and restart. Player journeys cover ATM, change, drop/pickup, split
  remainder/morph and claims. Blackjack remains deprecated under active epochs:
  qualify refusal before wager, wallet or pending-payout mutation. Active blackjack
  push/win/loss and round-recovery journeys are outside the supported product;
  existing inactive legacy regression coverage remains separate.
- Focused money regressions and both server builds pass. Report routes that
  remain intentionally unsupported with executable refusal tests.

## Boundary and handoff

The plan exports typed money effects and documented source/sink policy to Plan 4.
It does not change item UID ownership or invent a finite NPC/keeper treasury.
The independently testable deliverable is an inactive but fully evidenced set
of money routes; whole-game activation waits for the other plans.


### Ordinary room coin producer slice integrated; unqualified

Local merge af900753 contains independent implementation6b6b7c10c and the
optional post-ACK staging release contract96e2af83a. Active ordinary-room single-root
coin drop and pickup now use a retained native publication adapter. Original UID,
canonical before/after literal bytes, denominations, custody and exact result
revisions must agree. Native materialization, amount updates and placement keep
explicit started/returned states; uncertain effects remain held. Every ACK retry
revalidates physical evidence and the current wallet body. Work is one attempt per
pulse; notifications and bulk continuation follow durable ACK and owner extraction.
Inactive/schema1 paths remain separate, unsupported active placements refuse and
callback-free cold replay stays held. Production Makefile registration is primary-owned.

Final BEFORE /opt/duris-accounting-coin-publication-before-final-18fbd004fc3e/source
manifest SHA-25648792b5f103e286305ec1c4282b0e864ef494cd8d7566347311b2a41aa2235c7
pins1238 files. Candidate /opt/duris-accounting-coin-publication-committed-3457636af08c/source
manifest45432924e1c0b872d1f9eb3b4916ac93fc033706dfd74089fc145af6a10860ba pins1240.
Prepared24physical+15owner cases per SQL-header/flat profile are unexecuted; BEFORE
uses --scope owner, AFTER --scope all. Their native capture/codec/runtime custody
plus controlled placement/render/materializer seams do not qualify actual actobj
handlers, either database or flatfile recovery. Original300-second compile and
30-second per-case bounds remain unmeasured. Four initial review findings were
corrected; primary final-pin source review remains separate from qualification.

Open: semantic writer/central owner registration, actual native producer and
backend journeys, cold routing, actorless hydration and explicit uncertain native
effect recovery. No source/fixture inventory is promoted to R1-R8 or route evidence.
