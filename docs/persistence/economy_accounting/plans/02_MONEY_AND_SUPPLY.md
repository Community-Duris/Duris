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
  remainder/morph, claims, and blackjack push/win/loss/interruption.
- Focused money regressions and both server builds pass. Report routes that
  remain intentionally unsupported with executable refusal tests.

## Boundary and handoff

The plan exports typed money effects and documented source/sink policy to Plan 4.
It does not change item UID ownership or invent a finite NPC/keeper treasury.
The independently testable deliverable is an inactive but fully evidenced set
of money routes; whole-game activation waits for the other plans.
