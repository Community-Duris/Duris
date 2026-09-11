# Area-authored coin pickup regression

Issue #213 affects `ITEM_MONEY` prototypes other than the generic `VOBJ_COINS`
prototype. The reported example is `areas/obj/library.obj` #402013: ten platinum
hidden inside statue #402001 by a `P` zone reset.

The pickup command accepted money by type, but its transaction payload substituted
vnum 3 while capturing the object's actual vnum in the snapshot. The coin command
then required both records to use vnum 3. Repository validation and player reload
also assumed that all physical money used this one prototype.

On the current base, a second problem prevents the first pickup after admission:
the admission callback runs while its completed movement remains in the pending
map. The callback's pickup continuation consequently sees its own actor as busy.
Ordinary admission now releases that completed movement before calling the
continuation, following the existing movement-completion convention. Creation
grants still retain pending work until publication succeeds.

## Behavior

- Coin transactions accept any `ITEM_MONEY` snapshot whose UID and vnum match
  the item-transfer identity. Non-money snapshots and identity mismatches remain
  invalid.
- Pickup retains the original vnum in the request, durable payload, and runtime
  custody record. Wallet credit and pile consumption/remainder still commit in
  one transaction, with the existing revision and amount checks.
- Both MariaDB and flat-file repositories handle new area piles and legacy
  custody rows. Existing rows without a canonical coin payload read their
  baseline from the recorded owner's saved item payload. No ledger reset or
  migration is required.
- Reload uses authoritative remainders and consumption tombstones for area
  money. The legacy vnum-3 tombstone rule remains compatible with old SQL item
  rows that did not populate `item_type`.
- A rejected coin-command build reports its validation reason and player ID in
  the debug log.

The vnum-3 requirement for the pile created from a player's death wallet is
unchanged: that check validates a specific generated object, not ordinary loot.
The separate inventory-count restriction on taking money from containers is
outside this fix.

## Regression checks

`python3 tests/async/test_area_coin_pickup.py` boots isolated flat-file servers
and copies the actual statue and hidden-money prototypes into the minimal world,
changing only their vnums. An `O` reset places the statue and a `P` reset puts the
coins inside it. Each fresh character searches the statue and picks up its coins
using one of `get coins statue`, `get all.coins statue`, and `take all statue`.
The test checks a durable ten-platinum wallet credit, an empty statue, and no
additional credit on a repeated pickup. `--server /absolute/path/to/dms_new` can
reuse an already built flat-file binary. All account, world, and authority data
are synthetic and temporary; the checkout's `.env` is not read.

Additional focused checks:

```sh
python3 tests/async/test_coin_custody_lifecycle.py
python3 tests/async/test_currency_input_queue.py
python3 tests/async/test_flatfile_item_repository.py
bash tests/async/run_currency_transaction_schema_mysql.sh
python3 tests/async/test_creation_grant_reconciliation.py
python3 tests/async/test_creation_grant_batch_submission.py
python3 tests/async/test_live_item_movement_contract.py
```

The flat-file repository runs its full coin matrix with both vnum 3 and #402013.
The SQL coin matrix uses #402013, including legacy player, room, corpse, and
locker custody, partial pickup, replay, rollback, and reload. The command codec
tests also reject non-money and mismatched-vnum snapshots and verify diagnostics.
The currency input-queue harness backs `OBJ_VNUM` with real fixture index entries
and preserves the prototype in captured and materialized snapshots. Its area-coin
case holds a second pickup behind a pending transaction while allowing `score`,
then checks rejection/retry, detached restoration of a partial remainder, and
final consumption under ASan/UBSan in both backend configurations.
