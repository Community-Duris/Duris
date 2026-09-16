# Corpse bulk-loot journeys (#256, #378)

`get all corpse` now captures the corpse's display name for that operation. After
the first nonempty request is accepted, the looter and source-room observers see
the start. Equipment and coin acknowledgements accumulate only delivered items
and committed coin amounts. At terminal completion the player sees the sorting
line, the haul, and then partial/failure notices. Both NPC and player corpses use
this presentation; the player-corpse persistence flag keeps its existing meaning.

An accepted ownership transfer is the reservation boundary. After it commits,
the original container and selected roots must still match their captured source
topology, but the actor may have moved. The accepted items then enter the actor's
inventory. Stock adoption only establishes custody at the source. A selected
coin pile in a corpse may finish after flee while that exact corpse remains on
the original room floor; the request carries the originally selected
denominations, so coins added later cannot be swept into it. Ordinary
floor/container requests and any corpse that moved or disappeared remain
proximity/source-bound. Leaving during adoption or after a source change leaves
those contents at the source. No delayed physical loot narration is sent to
observers in the destination room.

The command's existing per-player busy gate and the persistence service's
operation identity serialize its callbacks. Disconnect completion clears
transient reporting state; persistence remains authoritative on reconnect.
Descriptions and coin formatter output are copied into owned strings. Missing
or moved source topology reports publication failure and alerts staff, without
claiming those items were acquired.

## Local validation

Both flatfile and MariaDB server builds passed with every runtime source changed
during the issue sweep restored from this branch's base and recompiled. The
following focused checks passed:

```sh
python3 tests/async/test_corpse_haul.py
python3 tests/async/test_bulk_get_publication.py
python3 tests/async/test_coin_get_completion.py
python3 tests/async/test_get_all_durable_chain.py
python3 tests/async/test_currency_input_queue.py
python3 tests/async/test_combat_movement_feedback.py
```

`test_corpse_haul.py` executes production selection, admission continuation,
container publication and completion functions under ASan/UBSan. Controlled
service callbacks cover movement during adoption and transfer, NPC/player
presentation, coin-only and mixed loot, multiple coin acknowledgements with a
later failure, partial capacity, immediately refused admission, moved/missing
source, failed live delivery, repeated commands, and detached actors. The older
coin test preserves ordinary floor/container and single-pickup formatting.
The currency queue test additionally verifies a stationary untracked corpse
admission whose pile grows before acknowledgement, then submits a tracked coin
pile from a stable corpse after the actor changes rooms. Both cases verify the
exact selected amount and remaining pile. The movement-feedback contract checks
that blocked ordinary directions point players to `flee` while the generic
combat refusal remains for other commands.

The actual three-player MariaDB journey also passed:

```sh
TEST_DB_HOST=127.0.0.1 TEST_DB_USER=... TEST_DB_PASSWORD=... \
  python3 tests/async/run_corpse_haul_journey.py /absolute/path/to/mariadb/server
```

Use a disposable loopback database. The runner creates and drops a unique
schema; it never reads checkout credentials. It creates three real accounts and
characters. The looter kills Raoul through actual combat; a second player stays
at the corpse and a third waits in the next room. A separate SQL connection
locks `item_current_owner`, holding real worker acknowledgements while the looter
walks north. It checks NPC stock adoption, then moves tracked equipment and an
initially untracked NPC coin pile together from the exact corpse after the
looter flees. After an actual player death, it repeats the mixed equipment-and-
coin movement against the player's corpse. Each stage asserts message order,
observer output, inventory/custody or wallet changes. A live reconnect retains
the acquired inventory without replaying a completed haul, and saves succeed.

Early integration attempts exposed fixture expectations rather than runtime
failures: `Raoul` is capitalized, a linkdead reconnect skips the ordinary
`Play as` confirmation, and a player's corpse has a character-name short
description even when its room description names the race. Those expectations
were corrected before the complete journey passed. This is local validation;
GitHub CI is not used as the acceptance gate.
