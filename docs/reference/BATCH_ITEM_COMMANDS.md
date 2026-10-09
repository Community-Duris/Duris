# Batch item commands

Duris supports several commands that select or move more than one item at a
time. This reference records the accepted syntax and the persistence boundary
behind it. The handlers in `src/cmd/actobj.c` and `src/cmd/actoth.c` remain
authoritative.

## Core syntax

`take` is a complete alias for `get`; both names dispatch to `do_get()`.

The table describes ordinary item authority. When economic gameplay authority is
active, `wear all` and `remove all` are refused; single-item equipment changes use
the typed accounting route. This conditional behavior does not imply that the
[accounting release gate](../persistence/ECONOMY_ACCOUNTING.md) is qualified.

| Operation | Accepted batch syntax | Selection and destination |
| --- | --- | --- |
| Pick up from a room | `get all`, `take all` | Every eligible visible floor object. |
| Pick up matching floor objects | `get all.<keyword>`, `take all.<keyword>` | Visible floor objects with that complete name keyword. |
| Empty one source into inventory | `get all [from] <container>`, `take all [from] <container>` | Every eligible item in one container. |
| Take matching contents | `get all.<keyword> [from] <container>`, `take all.<keyword> [from] <container>` | Matching contents of one container. |
| Put inventory into a container | `put all [in] <container>` | Every eligible carried object except the destination. Wallet coins are excluded. |
| Put matching inventory | `put all.<keyword> [in] <container>` | Matching eligible carried objects. |
| Drop inventory | `drop all` | Every eligible carried object. Wallet coins are excluded. |
| Drop matching inventory | `drop all.<keyword>` | Matching eligible carried objects. |
| Equip inventory | `wear all` | Tries carried items against empty slots in equipment-table order. |
| Unequip equipment | `remove all` | Tries every occupied slot and stops if inventory capacity is exhausted. |

The general argument parser discards fill words including `in`, `from`,
`with`, `the`, `on`, `at`, and `to`. The bracketed words above are therefore
optional, although the displayed forms are clearest. Phrases such as `out of`
are not parsed as a unit.

### Containers and corpses

For `get` and `take`, a source may be carried, worn, or on the room floor.
Valid source types are ordinary containers, storage containers, quivers, and
corpses. Resolution prefers a valid carried source, then a valid worn source,
then a valid room source.

Player and mobile corpses use the ordinary container grammar:

```text
get all corpse
take all corpse
get all.<keyword> corpse
take all.<keyword> corpse
```

An ordinal such as `2.corpse` selects one corpse when several share the
keyword. Player corpses additionally enforce their loot-delay, combat,
front-line, logging, revision, and corpse-file rules.

The token after `all` is always a source. Consequently, `get all <object>`
does not mean every object with that name and fails unless `<object>` is a
valid source. Use `get all.<keyword>` for a name-filtered floor batch.

For `put`, destination lookup covers carried and room objects, not worn
equipment. Containers, storage containers, quivers, and corpses are accepted
subject to their closed state, capacity, and item-type rules.

## Coins

Coins have two representations:

- A floor or container pile is an `ITEM_MONEY` object whose keyword is
  `coins`.
- Carried currency is stored in the character's copper, silver, gold, and
  platinum wallet balances, not in the carried-object list.

Ordinary get batches include money objects and convert each complete pile into
wallet balances. Coin-only selection uses the normal keyword form:

```text
get all.coins
take all.coins
get all.coins corpse
take all.coins corpse
```

`isname()` requires a complete keyword, so singular `all.coin` does not match
the canonical `coins` object. A pile is indivisible; there is no
`get <amount> <coin-type>` form.

Wallet currency leaves through explicit forms:

```text
put all.coins [in] <container>
put <amount> <coin-type> [in] <container>
drop all.coins
drop <amount> <coin-type>
```

`<coin-type>` is `copper`, `silver`, `gold`, or `platinum`; accepted prefixes
are resolved by `coin_type()`.

The dot is significant. `drop all.coins` empties the wallet, while
`drop all coins` is parsed as `drop all` and ignores the trailing word.
Likewise, `put all.coins bag` targets the wallet, while `put all coins bag`
treats `coins` as the destination name. Only the exact `all.coins` token enters
the wallet special case; `all.coin` is an ordinary item-keyword filter.

Other wallet batch operations do not pass through item ownership:

| Command | Meaning |
| --- | --- |
| `deposit all` | Move every wallet denomination into the bank. Container coins are excluded. |
| `deposit <amount> <coin-type>` | Move one denomination from wallet to bank. |
| `withdraw <amount> <coin-type>` | Move one denomination from bank to wallet; there is no `withdraw all`. |
| `give <amount> <coin-type> <player>` | Transfer one wallet denomination; there is no `give all`. |
| `split <amount> <coin-type>` | Split one wallet denomination among present group members. |

## Unsupported and separate forms

- `get all all` and `get <item> all` are rejected; one command cannot search
  multiple containers.
- `wear all.<keyword>` and `remove all.<keyword>` are not batch forms.
- `wield all`, `hold all`, `grab all`, `give all`, `sell all`, and
  `hide all` are not supported item batches.
- `junk all`, `bury all.<keyword>`, `donate all[.<keyword>]`, and
  `empty <source> <destination>` are legacy direct loops with their own
  eligibility rules. `junk all` currently returns immediately for untrusted
  characters and does not accept `all.<keyword>`; `bury all` without a keyword
  is rejected.
- Producing shops mark unlimited stock with `[quantity 1-50]` in `list`.
  Use `buy <item> quantity <1-50>` for inventory delivery or
  `buy <item> quantity <1-50> into <container>` for a carried, open container.
  The historical `buy <item> <container> [quantity]` form remains supported.
  Examples: `buy ration quantity 10`, `buy #3 quantity 10 into backpack`,
  and `buy arrow quiver 50`. Quantity purchases are excluded from epic shops.
  Both persistence modes reject invalid quantities, destinations, and trailing
  arguments before charging or purchasing anything. A quantity is a complete
  unsigned decimal integer from 1 through 50.
  Accepted orders show quantity, destination, unit price, and total. Copies
  complete sequentially with the existing per-copy capacity checks and atomic
  payment/delivery rules; an entire order is not an all-or-nothing transaction.
  Full or stopped results give the delivered count and actual charge, with an
  uncharged remainder and stop reason. A rejected SQL delivery after payment
  identifies the charged copy and its pending refund. A durable purchase awaiting live delivery
  is charged and pending, not an uncharged failure: wait or reconnect and do not
  repeat the purchase. Batch output has one acceptance, one final summary, and
  at most one room event, with a separate delay notice if publication must wait.
- `auction offer` supports 1 through 9 adjacent same-VNUM items, and
  `auction pickup` can return a listing's item batch. These use the auction
  transaction domain rather than the general movement adapter.

## Atomicity and persistence

`get all`, `drop all`, and `put all`, including keyword, container, locker,
and corpse variants, capture every eligible durable root as one multi-root
forest and submit one ownership command. The command validates and moves the
whole selected forest under one owner-revision transition. Publication starts
only after every selected live root still matches its original source; a
failure does not publish a partial durable batch.

Selection performs cumulative capacity checks before submission. Binding,
trap, hitch, no-loot, cursed, soulbound, quiver, weight, count, and space rules
remain command-specific.

Objects with active generic ownership participate in the durable forest,
including a transient item whose UID has an active runtime ownership row.
Money objects, UID-less or unowned transient objects, player-corpse roots, and
get-side scrap remain on their lifecycle-specific synchronous paths, which run
only after the durable part commits. `wear all`, `remove all`, `junk`, `bury`,
`donate`, and `empty` remain synchronous loops rather than multi-root ownership
commands.

Item-transfer payload version 6 introduced the variable-length multi-root format.
Later versions extend it; current format constants are defined in
[`item_transfer_command.h`](../../src/item/item_transfer_command.h).
Versions 2 through 5 retain their fixed 12-entry compatibility layout. General
item transfers accept the runtime's 3,000-item staff inventory ceiling within
the existing 128 KiB snapshot limit; shop-trade payloads retain their 12-item
bound.

## Source and tests

- Registration, aliases, fill words: `src/cmd/interp.c`
- Get, put, drop, wear, remove, junk, empty: `src/cmd/actobj.c`
- Deposit, withdraw, split, bury, donate: `src/cmd/actoth.c`
- Money objects and keyword matching: `src/world/handler.c`
- Shops and auctions: `src/economy/shop.c`,
  `src/economy/auction_houses.c`
- Canonical money prototypes: `areas/obj/limbo.obj`, `areas_mini/mini.obj`
- Focused contracts: `tests/async/test_get_all_durable_chain.py`,
  `tests/async/test_bulk_drop_put_durable_chain.py`,
  `tests/async/test_actobj_get_limits.py`, and
  `tests/async/test_wear_all_regression.py`

## Asynchronous completion prompts

`src/net/comm.c::process_output()` leaves an ordinary command prompt pending
while item movement or currency publication reports the player busy, including
inbound transfers covered by those coordinators. Queued text and unrelated
asynchronous output still flush. Pager and string-editor prompts retain their
own behavior. The leading newline is chosen using the deferred-prompt state so
a delayed pickup preserves the synchronous byte ordering.

`test_item_movement_prompt_runtime.py` exercises real prompt generation and
Telnet/WebSocket text serialization with held coordinator completions, compact
and smart-prompt variants, combat, pager/editor and switched descriptors. It
uses fixture pickup text and transport endpoints; it is not a full browser
network or `do_get()` journey.
