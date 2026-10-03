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
Migration `0042_shopkeeper_roaming_witness` adds nullable roaming configuration;
NULL distinguishes legacy rows whose policy was never captured. A successful
shopkeeper checkpoint stores the current configured roaming value with cash and
stock. The inactive SQL lock accepts only a known 0/1 value that agrees with
the frozen command. Source capture includes this field; normalization reports
unknown configuration and the lifecycle owner refuses to open a treasury
baseline from it.
The SQL saver now captures cash with a stable `shopkeepers.id`, increments the
revision, and replaces stock and affects inside the same transaction. Restore
applies captured cash before publishing the keeper and rejects invalid values.
The stock saver also persists each original `shopkeeper_items.obj_uid`; restore
keeps it, advances the UID allocator, and refuses zero, overflowed, or duplicate
saved UIDs before publishing stock. Legacy rows with NULL UID still load with a
new UID and remain dirty for a replacement snapshot. The population regression
exercises the production save formatter and restore path for these cases.
Migration 0043 adds nullable `shopkeeper_items.item_condition`. Existing stock
rows remain unknown rather than receiving a fabricated condition. A successful
keeper checkpoint writes each root and contained item's live condition; restore
uses a known saved value and retains the prototype condition for legacy NULL.
The raw SQL item-source snapshot now binds that value, and lifecycle installation
refuses any remaining NULL keeper item condition before opening a baseline.
`python3 tests/async/test_shopkeeper_save_runtime.py` and
`python3 tests/async/test_shopkeeper_population.py` cover these boundaries with
deterministic SQL doubles. Migration 0044 adds nullable
`shopkeeper_items.item_properties`. A successful checkpoint encodes dynamic
affects and extra flags with the player item codec; restore decodes them before
publishing stock and refuses a malformed payload. Legacy NULL rows retain their
prototype state. Migrations 0041 through 0044 are required before deploying the
SQL server code; historical cash, roaming policy, item condition, and dynamic
properties are not backfilled.

The SQL source capture now includes each stable shopkeeper row, including
nullable cash and the shop revision. Normalization reports unknown cash as a
defect, and the lifecycle owner refuses a baseline until every keeper has a
known, nonnegative cash value. It assigns a treasury mapping to the stable
`shopkeepers.id`, preserves `shop_id` in the readback receipt, and includes the
exact keeper cash and revision in the opening witness. A changed keeper row
changes the native boundary digest. The disposable MySQL source and lifecycle
fixtures cover a known keeper, a null-cash refusal, and treasury readback.

An inactive SQL shop authority lock now validates the frozen three-account
intent under the caller's transaction. It locks the active epoch and mappings,
then the native player wallet, account bank, and keeper cash rows. It locks the
owner revisions, selected item ownership tree, and matching physical
`shopkeeper_items` or `player_items` rows. It verifies the physical parent links,
child counts, VNUMs, and unequipped state; a produced item also locks its
stocked exemplar and target ancestors while proving the new UIDs absent from
those inventories. Migration 0038 supplies the custody equipment slot witness.
A disposable MySQL fixture passes all five shop actions and a nested stock tree
through this lock and the typed plan. It checks the VNUM 11005 issuance
exception and an unfunded roaming refusal. It rejects null or stale cash, a
null or mismatched roaming policy,
hidden custody or native child, missing stock, an equipped item, a mismatched
physical parent, and an inactive epoch. The client-free build returns
`ENOTSUP`. The disposable runner can use MariaDB
10.11 with `bash tests/async/run_shop_trade_sql_lock_schema_mysql.sh`
or MySQL 8.4 with `SHOP_TRADE_LOCK_DB_IMAGE=mysql:8.4` before that command.
This fixture has no native shop mutation, receipt, or outbox. The SQL composite
still needs restart reconciliation of shop custody into the runtime cache
before activation.

The pure accounting adapter is not yet invoked by either repository. Plan 4
still needs a typed shop transaction that stages the plan with native state,
receipt, and outbox. The SQL route needs a composite instead of its separate
legacy money and item calls. The shop fee is a separate route and needs its own
accounting treatment. Disposable MySQL and MariaDB journeys must prove buy,
sell, keeper cash, the VNUM 11005 exception, replay, restart, and intermediate
failure. No shop route is activated by this boundary work.
