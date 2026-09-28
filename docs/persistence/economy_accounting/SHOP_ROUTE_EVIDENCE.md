# Shop route boundary evidence

The flatfile shop trade owner is still a schema 1 native route. Its successful
trade now stages each exact item UID and revision reference before the authority
journal commits the shop stock, item custody, player materialization, wallet,
keeper cash, and operation result. The shop record now captures and restores
keeper cash; a new trade fences the live cash snapshot against the retained
shop record, then writes its result in the same authority journal. Roaming
keepers refuse an unfunded sale. The VNUM 11005 exception and stationary
shop behavior retain the legacy rule that an unfunded sale leaves keeper cash
unchanged. The shop ID is the retained keeper identity; the VNUM selects the
exception and is never an account key. Version 1 shop catalogs remain readable
with unknown cash and must be saved with a captured live cash value before
admitting a version 5 trade. Version 1 shop operation catalogs replay their
original result lengths when converted to the current catalog format.

A produced item uses revision zero as the absent witness,
then its committed creation revision. A reference staging failure returns
before native commit. Recovery of an interrupted journal restores the
references alongside the trade; exact command replay returns the retained
result.

After exact replay lookup and before native preparation, the owner checks the
flatfile accounting control under the same authority lock. An active epoch
refuses a new legacy shop trade with `EAGAIN`; an absent accounting installation
continues to admit the legacy route. This is an unported-route refusal boundary,
not a typed shop accounting component.

The gameplay shop dispatcher now refuses buy, sell, peruse, repair, and smith
forge commands under an active economic epoch before any secondary shop
procedure runs. Direct shop actions and the shared payment helper repeat the
guard, so a caller that bypasses the dispatcher cannot mutate stock or cash.
The modern craft/forge command, smith forge, and refine command also refuse an
active epoch before recipe migration or input consumption. The focused
`test_compound_active_epoch_refusals.py` checks these mutation boundaries.

`python3 tests/async/test_flatfile_shop_trade_repository.py` verifies an
interrupted purchase, cash, and reference recovery; an unfunded roaming sale;
a forced pre-commit sale failure that leaves wallet, keeper cash, stock, and
references unchanged; both keeper cash exceptions; a produced-item creation
reference; and active-epoch refusal with exact replay still available. The shop,
auction, and collector accounting context tests and the broader flatfile
accounting gate test pass. Both server backends build with GCC 12 in the
configured WSL build environment.

The inactive `shop_trade_accounting` adapter now freezes the wallet, bank, and
keeper treasury account IDs and compiles the native buy, sell, and cleanup
results into a typed plan. A funded trade posts the exact wallet and keeper
cash changes. A stationary or VNUM 11005 sale that exceeds keeper cash posts
the seller's proceeds against issuance, matching the native unchanged keeper
cash result. It verifies selected item UIDs, revisions, owners, VNUMs, and
tree topology; produced items additionally require the stocked exemplar and
target ancestor witnesses. `python3 tests/async/test_shop_trade_typed_accounting.py`
exercises all five actions, nested production, cash exceptions, zero-price
denomination exchange, and corrupted evidence.

The flatfile mapping authority now recognizes keeper treasury locators by the
existing shopkeeper owner ID (`shop_id + 1`), including shop ID zero. It keeps
the allocated account ID independent of that locator and retains retired
lifetimes. The authority test checks lookup, identity refusal, and remapping.

Migration `0041_shopkeeper_cash_identity` adds a nullable SQL keeper cash field
and a revision. NULL distinguishes legacy rows whose cash was never captured.
The SQL saver now captures cash with a stable `shopkeepers.id`, increments the
revision, and replaces stock and affects inside the same transaction. Restore
applies captured cash before publishing the keeper and rejects invalid values.
`python3 tests/async/test_shopkeeper_save_runtime.py` and
`python3 tests/async/test_shopkeeper_population.py` cover these boundaries with
deterministic SQL doubles. The migration is required before deploying the SQL
server code; no historical cash value is backfilled.

This pure adapter is not yet invoked by either repository. Plan 4 still needs
keeper mapping creation and baseline coverage, followed by a typed shop
transaction that stages the plan with native state, receipt, and outbox. The SQL route needs a
composite instead of its separate legacy money and item calls. The shop fee is a
separate route and needs its own accounting treatment. Disposable MySQL and
MariaDB journeys must prove buy, sell, keeper cash, the VNUM 11005 exception,
replay, restart, and intermediate failure. No shop route is activated by this
boundary work.
