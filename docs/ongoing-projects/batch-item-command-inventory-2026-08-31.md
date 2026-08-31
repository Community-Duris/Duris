# Batch-item command inventory

Status: implementation complete; PR qualification in progress. Last updated:
2026-09-01. Branch:
`batch-item` (implementation started from `e71a54c7`).

## Scope

This note inventories the player commands that select or move more than one
item in a single command, with emphasis on the `get`/`take`, `put`, `drop`,
`wear`, and `remove` family. It distinguishes command syntax from special
sources such as player corpses and from wallet currency, which is not stored as
ordinary carried item objects.

The command handlers are authoritative. The old help text is useful context,
but at least one coin example in it is stale.

## Required behavior

Batch-item commands select the full eligible item set and commit it as one
atomic multi-item transaction, preserving the pre-refactor all-at-once
semantics. The prior one-item-at-a-time serialized transaction chains were
incorrect and have been replaced.

## Implementation progress

### Checkpoint 1: multi-root persistence foundation

Implemented on 2026-09-01:

- Added item-transfer payload version 6 with a variable-length item section and
  an explicit multi-root forest representation. Versions 2-5 retain their
  fixed 12-entry wire layout for compatibility.
- Raised the transfer and critical-command bounds to support the runtime's
  3,000-item staff inventory ceiling while retaining the existing 128 KiB item
  snapshot bound. Shop-trade payloads remain at their prior 12-item bound.
- Updated MariaDB, flat-file, and in-memory ownership application to validate
  and move every selected root under one owner-revision transition and one
  command result.
- Added compatibility, runtime, and flat-file regression coverage for a
  two-root atomic transfer. The clean server build succeeds with warnings
  treated as errors.

### Checkpoint 2: atomic `drop` and `put`

Implemented on 2026-09-01:

- Added a live movement adapter that captures multiple object trees, rebases
  snapshot parent indexes into one forest, and submits one version 6 command.
- Replaced the durable `drop all` / `drop all.<keyword>` chain with one batch
  transaction. Soulbound and cursed objects are excluded during selection;
  transient, money, and player-corpse roots run only after a successful durable
  commit.
- Replaced the durable `put all` / `put all.<keyword>` chain with one batch
  transaction. Selection now performs cumulative quiver-count, weight, and
  optional space preflight before submitting the full eligible set.
- Updated room-container flat-file materialization to attach every batch root
  and propagate their combined weight. Focused command, live-movement, and
  repository contracts cover the new boundary.

### Checkpoint 3: atomic `get` and multi-root extraction

Implemented on 2026-09-01:

- Replaced the durable room, container, locker, and player-corpse `get all` /
  `get all.<keyword>` chain with one batch transaction. Selection performs
  cumulative count and weight preflight and excludes binding, trap, hitch, and
  no-loot failures before ownership submission.
- Batch completion validates every selected root against the original live
  source before publishing any move. Player-corpse revision publication occurs
  once for the committed forest; money, transient roots, and scrap processing
  run only after that commit succeeds.
- Added normalized forest extraction for persisted item snapshots and applied
  it to flat-file room, locker, and corpse withdrawals. End-to-end repository
  coverage now exercises two independent roots at all three boundaries.
- Replaced the serialized-get source contract with atomic selection,
  submission, and publication assertions. The clean server build and the
  focused get, movement, snapshot, and flat-file suites pass.

### Checkpoint 4: final qualification

Completed on 2026-09-01:

- Rebuilt the server and ran the combined ownership, transfer compatibility,
  snapshot, flat-file repository, live movement, get-limit, and atomic
  get/drop/put regression suites. All checks pass, including changed-line
  formatting and whitespace validation.
- Booted the branch binary against the configured local development
  environment and exercised two durable objects through `drop all.paper` and
  `get all.paper`. Both commands moved the full two-item set, and the test
  objects were removed before the character was saved and the server shut down
  cleanly.
- Checked the resulting server log for batch-publication, stale-topology,
  ownership, persistence-alert, and critical-command failures; none were
  present.

### Checkpoint 5: PR gate integration repairs

Implemented on 2026-09-01:

- The first full pull-request gate exposed two integration gaps outside the
  focused matrix: the shared flat-file materialization journal still capped
  decoded events at the 12-item commerce limit, and two isolated player-load
  harnesses did not link the new item-transfer helpers.
- Raised only the internal materialization event reader to the item-transfer
  limit; shop command payloads retain their existing 12-item maximum. Added a
  13-root journal regression and linked the isolated hydration harness against
  the production transfer and critical-command helpers.
- The focused flat-file repository and hydration suites pass. Both formerly
  failing live flat-file journeys now pass, including combat, death, corpse
  recovery, save/reconnect, and full-world process restart.

Still to verify:

- Run the repository-wide `make test-all` gate, update this checkpoint with the
  result, and confirm the replacement pull-request checks are green.

## Core command matrix

`take` is a complete alias for `get`: both commands are registered to
`do_get()` in `src/cmd/interp.c`.

| Operation | Accepted batch syntax | Selection and destination |
| --- | --- | --- |
| Pick up from the room | `get all` / `take all` | All eligible visible floor objects. |
| Pick up matching floor objects | `get all.<keyword>` / `take all.<keyword>` | Floor objects whose name contains the exact keyword. |
| Empty a source into inventory | `get all [from] <container>` / `take all [from] <container>` | All eligible contents of one container. |
| Take matching contents | `get all.<keyword> [from] <container>` / `take all.<keyword> [from] <container>` | Matching contents of one container. |
| Put inventory into a container | `put all [in] <container>` | All eligible carried objects except the destination itself. Wallet coins are not included. |
| Put matching inventory | `put all.<keyword> [in] <container>` | Matching carried objects. |
| Drop inventory | `drop all` | All eligible carried objects. Wallet coins are not included. |
| Drop matching inventory | `drop all.<keyword>` | Matching carried objects. |
| Equip inventory | `wear all` | Tries to fill empty equipment slots from carried items in the order in `equipment_pos_table`. |
| Unequip equipment | `remove all` | Tries every equipment slot and stops early if inventory capacity is exhausted. |

`from` and `in` are optional because the general argument parser discards fill
words including `in`, `from`, `with`, `the`, `on`, `at`, and `to`. The canonical
forms above are preferable; phrases such as `out of` are not recognized as a
unit.

### Containers and corpses

For `get`/`take`, a source may be carried, worn, or on the room floor. Valid
source types are normal containers, storage containers, quivers, and corpses.
Resolution prefers a valid carried source, then a valid worn source, then a
valid room source.

A player corpse is a special case of the same container grammar, not another
command form:

```text
get all corpse
take all corpse
get all.<keyword> corpse
take all.<keyword> corpse
```

The same forms work for mob corpses. A player-corpse source additionally uses
corpse logging, revision publication, corpse-file updates, loot delay, and the
combat/front-line gates in `do_get_container_preflight()`. An ordinal such as
`2.corpse` can identify one corpse when several share the keyword.

The word after `all` is always interpreted as a source. Therefore `get all
<ordinary-object>` does **not** mean "get every object with this name"; it fails
unless that object is one of the valid source types. The matching form is
`get all.<keyword>`.

For `put`, the destination lookup covers carried and room objects, not worn
equipment. `put()` accepts containers, storage containers, quivers, and
corpses, subject to their capacity, closed-state, and item-type rules.

## Coins

There are two representations to keep separate:

- A coin pile on the floor or in a container is an `ITEM_MONEY` object with the
  keyword `coins`.
- Coins already held by a character live in the four wallet balances (copper,
  silver, gold, and platinum), not in `ch->carrying`.

### Coin piles entering the wallet

The ordinary get batches already include coin-pile objects:

```text
get all
take all
get all corpse
take all corpse
```

Coin-only selection uses the normal `all.<keyword>` filter:

```text
get all.coins
take all.coins
get all.coins corpse
take all.coins corpse
```

There is no hard-coded `get all.coins` branch; it works because the generated
money object is named `coins`. The active `isname()` implementation requires a
complete keyword. Consequently, the old help example `get all.coin corpse` is
not reliable against the canonical coin object, whose only name keyword is the
plural `coins`.

Coin piles are taken whole and converted into wallet balances. There is no
`get <amount> <coin-type>` form for taking only part of a pile.

### Wallet coins leaving the wallet

`put` and `drop` have explicit wallet-currency forms:

```text
put all.coins [in] <container>
put <amount> <coin-type> [in] <container>

drop all.coins
drop <amount> <coin-type>
```

`<coin-type>` is `copper`, `silver`, `gold`, or `platinum`; prefixes such as
`c`, `s`, `g`, and `p` are accepted by `coin_type()`.

The dot matters:

- `drop all.coins` empties the wallet. `drop all coins` is parsed as `drop all`
  and ignores the trailing word, so it drops carried objects instead.
- `put all.coins bag` empties the wallet into `bag`. `put all coins bag` treats
  `coins` as the destination name and does not mean wallet currency.
- `drop all.coin` and `put all.coin ...` are generic item-keyword batches, not
  wallet operations, because the currency special case is the exact string
  `all.coins`.

`drop all` and `put all` do not include wallet currency. They can only include
an actual carried money object, which normal coin pickup immediately converts
to wallet balances.

### Other multi-coin commands

These mutate wallet balances but do not move `ITEM_MONEY` objects through the
item-ownership path:

| Command | Meaning |
| --- | --- |
| `deposit all` | Move every wallet denomination to the bank; coins inside containers are excluded. |
| `deposit <amount> <coin-type>` | Move one denomination from wallet to bank. |
| `withdraw <amount> <coin-type>` | Move one denomination from bank to wallet; there is no `withdraw all`. |
| `give <amount> <coin-type> <player>` | Transfer wallet currency to one player; there is no `give all` or `give all.coins`. |
| `split <amount> <coin-type>` | Split one wallet denomination among present group members. |

## Parser exclusions and negative cases

- `get all all` is explicitly rejected with `You must be joking?!`.
- `get <item> all` is explicitly rejected; the command cannot search two or
  more containers at once.
- `wear all.<keyword>` is not supported. It is treated as the literal name of
  one carried object.
- `remove all.<keyword>` is not supported. Only exact `remove all` is a batch.
- `wield all`, `hold all`, and `grab all` are not batch forms.
- `give all` and `give all.<keyword>` are not supported item batches.
- `sell all` is not supported by the normal shop handler.
- `hide all` and `hide all.<keyword>` are explicitly rejected.

## Other player-visible bulk item commands

These are real multi-item commands, but they are separate legacy or commerce
paths rather than variants of the core get/put/drop equipment flow:

| Command | Behavior |
| --- | --- |
| `junk all` | Destroys eligible carried items. The current handler returns immediately for untrusted characters, so this is effectively staff-only. There is no `junk all.<keyword>`. |
| `bury all.<keyword>` | Marks all matching visible floor items buried. Exact `bury all` is rejected. |
| `donate all` | Donates eligible carried items while in the donation-well room. |
| `donate all.<keyword>` | Donates matching carried items in the donation-well room. |
| `empty <source-container> <destination-container>` | Moves all contents from one container to another until capacity fails. It does not use the word `all`. |
| `buy <item> <container> <quantity>` | At a producing shop, creates 1-50 copies and delivers them to the named carried container. |
| `auction offer <item> [start] [buy-now] [days] [quantity]` | Lists 1-9 adjacent same-vnum carried items in one auction payload. |
| `auction pickup` | Claims staged auction money or one pending listing's item batch. A quantity listing can return several items in that one claim. |

Admin-only mass operations, recipe ingredient consumption, and internal
multi-item persistence payloads are outside this player-command inventory.

## Persistence behavior by family

The commands do not share one generic batch implementation.

- Player `get all` / `take all`, including container and corpse variants,
  snapshot every eligible durable root and submit the full forest in one
  ownership transaction. Publication begins only after every selected live
  root has been validated against its original source.
- Player `drop all`, `drop all.<keyword>`, `put all`, and
  `put all.<keyword>` use the same multi-root movement adapter to submit one
  ownership transaction for the eligible durable set.
- Money objects, transient objects, and player-corpse roots are excluded from
  generic item ownership and run through their synchronous lifecycle-specific
  paths after the durable transaction commits successfully.
- `wear all` and `remove all` are direct synchronous loops over inventory or
  equipment. They do not use the durable get/drop/put batch adapter.
- `junk`, `bury`, `donate`, and `empty` are also direct legacy loops.
- Quantity auctions use one true multi-item auction payload. Producing-shop
  purchases sequence their individual shop transactions.

Focused source-contract and regression coverage protects atomic get, drop, and
put selection, submission, publication, and capacity behavior in:

- `tests/async/test_get_all_durable_chain.py`
- `tests/async/test_bulk_drop_put_durable_chain.py`
- `tests/async/test_actobj_get_limits.py`

`tests/async/test_wear_all_regression.py` protects several crash and slot
regressions reached by `wear all`, but it is not a complete behavioral test of
the batch command. No equivalent focused `remove all` command test was found.

## Primary source locations

- Command registration and aliases: `src/cmd/interp.c`
- Fill-word and argument parsing: `src/cmd/interp.c`
- `get`/`take`, `drop`, `put`, `wear`, `remove`, `junk`, and `empty`:
  `src/cmd/actobj.c`
- `deposit`, `withdraw`, `split`, `bury`, and `donate`: `src/cmd/actoth.c`
- Coin object creation and keyword matching: `src/world/handler.c`
- Producing-shop quantity purchase: `src/economy/shop.c`
- Auction quantity listing and pickup: `src/economy/auction_houses.c`
- Canonical coin object data: `areas/obj/limbo.obj` and
  `areas_mini/mini.obj`
- Legacy help examples: `help/duris_help.hlp`
