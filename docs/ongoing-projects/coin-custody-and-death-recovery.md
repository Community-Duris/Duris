# Coin custody and death recovery

**In progress; not deployed.** See "Remaining fixes and verification" and
"Open issue found while verifying".

## Implemented so far

- Committed in `5ee2b380c`: atomic wallet/pile coin transactions for MariaDB and
  flatfile, command/completion handling, persisted coin payloads, loader updates,
  and migration `0010_coin_custody_payload`. Covers pile creation, merge, partial
  pickup, retirement, overflow rejection, and operation-ID replay.
- Committed in `f08101cbf`: full-wallet conversion through one coin transaction
  in `money_to_inventory()` (no 32,000-per-denomination clamp), corpse handoff
  waiting on currency completion, and legacy corpse money totals excluding
  tracked piles.

### This session

- `src/combat/fight.c` did not compile as committed: `currency_transaction_player_busy()`
  was used without including `economy/currency_transaction.h`. Fixed.
- **Rejected-corpse-transfer recovery.** `corpse_item_completion()` records the
  owner in a fixed, allocation-free dispute table instead of leaving the retry to
  resubmit the same refusal forever. `event_death_extract_retry()` finalizes a
  disputed death through `save_disputed_death_disposition()`, which captures the
  immutable death record and waits up to `DEATH_DISPOSITION_TIMEOUT_MSEC` (2000)
  for durability before releasing the character through
  `release_after_terminal_death()`. `die()` defers to that recovery event while a
  dispute is outstanding, clears any stale dispute at the start of a new death,
  and the retry clears one when a recovery is abandoned.
- **Wallet-conversion rejection is no longer silent.** `money_to_inventory()`
  returns whether the conversion was submitted and alerts on rejection;
  `make_corpse()` disputes the death when it was not, and the disposition
  re-creates the unattached pile from the live wallet so the coins are recorded.
- **A missing live corpse can no longer let an ordinary empty save complete a
  disputed death**: the retry keeps the live character and its refused assets and
  backs off instead.
- **New `player_save_pipeline_terminal_death()`** captures the death snapshot
  against a freshly marked terminal revision, enqueues it, and waits on the same
  fence the ordinary terminal save uses. The shared reserve/await steps were
  factored into `begin_terminal_fence()` and `await_terminal_fence()`.
- **Both save backends now apply death snapshots.**
  - MariaDB: `player_snapshot_repository_apply()` accepts the death schema version
    and writes `player_death_disposition` (operation id, corpse UID and room,
    wallet before conversion, wallet pile UID, encoded payload) plus one
    `player_death_custody` row per disputed custody observation, inside the same
    transaction that clears the player's items and bumps `save_revision`.
  - Flatfile: `flatfile_player_snapshot_apply()` publishes the record to
    `player-deaths/<pid>-<revision>.death` before writing the player file, and
    strips the death from the player file so a later ordinary save cannot
    overwrite the record.
- **Migration `0011_player_death_disposition`** (apply + verify + manifest
  entries + runtime compatibility head). Following the `kingdom_garrison`
  precedent, the two new tables are created only by their migration — not by
  `bootstrap_multithread_safe.sql` — and stay outside `RUNTIME_TABLE_SQL_LIST`
  and the data-lifecycle manifest, so no live MySQL 8 / MariaDB 10.11 fingerprint
  regeneration is required. Registering them there is the follow-up for whoever
  next has both servers to hand.
- `player-deaths` added to the provisioned flat-file directories.

### Idempotency

Retries and reconnects cannot repeat death consequences: the disposition commits
under the player's `save_revision` fence, so a replayed frame reports
`already_applied` and rewrites nothing; `REPLACE INTO` and a fixed flat-file name
make a re-applied record byte-identical rather than duplicated. The wallet, once
converted, is zero, and the death snapshot carries no inventory, so re-applying
clears an already-empty inventory.

## Verification performed

Build: `make -C src` clean (see "Build notes"). `./scripts/format.sh --check`
clean.

| What | How |
| --- | --- |
| MariaDB death apply, migration re-runnability, verify script, replay, ordinary save after death, malformed record rejection | `tests/async/run_player_death_disposition_mysql.sh` (new; disposable MariaDB 10.11 container) — passed |
| Flat-file death apply, empty-handed player file, disposition round-trip, replay `already_applied`, survives a later ordinary save | `tests/async/test_flatfile_player_repository.py` (extended harness) — passed |
| Terminal death fence, capture→enqueue→await ordering, invalid death without a corpse, timeout when neither backend can save | `tests/async/test_player_save_pipeline.py` (extended) — passed |
| Journal retains a death frame across restart and blocks replay until its own apply succeeds | `tests/async/test_player_save_journal.py` — passed (pre-existing coverage, still green) |
| Dispute, durable disposition, missing-corpse, wallet preservation, release ordering, stale-dispute clearing | `tests/async/test_death_item_custody_contract.py` (repaired and extended) — passed |
| Wallet-conversion dispute in `make_corpse()` | `tests/async/test_corpse_handoff_inflight_contract.py` (repaired and extended) — passed |
| Real in-game sequence on isolated flat-file storage: combat, NPC corpse loot, save, reconnect, player death, corpse recovery, reconnect | `tests/async/test_flatfile_combat_journey.py` — passed |
| Full-world flat-file boot, player and floor-item process-restart journey | `tests/async/test_flatfile_full_world_boot.py` — passed |
| Coin put / merge / partial pickup / retirement / replay identity | `tests/async/test_coin_custody_lifecycle.py`, `tests/async/test_currency_input_queue.py` — passed |

Both `test_death_item_custody_contract.py` and `test_corpse_handoff_inflight_contract.py`
were already failing against `f08101cbf` before any change here; they were
repaired as part of this work.

Tests updated because a contract they pin moved: `test_character_persistence_gap.py`,
`test_terminal_extract_item_retention.py` (extraction now happens inside
`release_after_terminal_death()`), `test_immutable_migration_runner.py`,
`test_runtime_boot_compatibility.py` (migration head 0011),
`test_flatfile_boot_preflight.py`, `test_flatfile_full_world_boot.py`,
`test_persistence_mode.py` (new `player-deaths` directory), and
`test_currency_input_queue.py` (`money_to_inventory()` now returns `bool` and
alerts, so its extracted-body harness needed the new signature and a
`persistence_alert()` stub).

New files: `migrations/immutable/0011_player_death_disposition.{sql,sh}`,
`tests/async/player_death_disposition_mysql_harness.cpp`,
`tests/async/run_player_death_disposition_mysql.sh`.

## Open issue found while verifying

Coins in an NPC corpse cannot be looted. `do_get()` reaches `submit_coin_get()`,
which requires an **active custody row** for the pile
(`item_ownership_runtime_lookup()` in `submit_coin_get()`, and again in
`prepare_coin_pile()`); a pile that reached a mob through `money_to_inventory()`'s
NPC branch or a zone reset has no such row, so the pickup is refused with
"The coin transfer could not start; nothing changed."

Reproduced by adding a money object to the flat-file journey fixture, giving it to
the fixture mob, and trying `get coins corpse` after the kill. This predates this
session's changes and is not on the remaining-work list, so it was left alone and
the fixture change was reverted rather than pinning the behaviour in a test. A fix
needs a deliberate design decision — most likely admitting an untracked pile as a
`creation`-reason transfer into the wallet, guarded so replay cannot mint coins.

## Remaining fixes and verification

- Verify the disputed path end to end in game, with an injected `EMSGSIZE` and a
  restart/replay across it. There is no in-server fault-injection hook for a
  refused transfer today, so this needs new test infrastructure and was not
  built here.
- Capacity limits (`PLAYER_SNAPSHOT_MAX_*`) against a death record large enough to
  exceed them; only the bounded-capture code path is covered today.
- No measured death-recovery wall time yet. Worst case by construction is one
  retry tick (`DEATH_EXTRACT_RETRY_INITIAL`, 4 pulses = 1s at `WAIT_SEC` 4) plus
  the 2s durability budget; the typical case is the retry tick plus a few
  milliseconds.
- Register `player_death_disposition` and `player_death_custody` in
  `RUNTIME_TABLE_SQL_LIST` and the data-lifecycle manifest once a MySQL 8 and a
  MariaDB 10.11 are available to regenerate the normalized metadata fingerprints.

## Build notes

`make -C src` needs cJSON, GnuTLS, hiredis, libbsd, libxml2 and ICU headers, none
of which were installed on this host and none of which can be installed without
root. They were staged into `~/.local` (headers under `~/.local/include`,
libraries under `~/.local/lib`, with `~/.local/include/libxml` symlinked to
`~/.local/include/libxml2/libxml`) and the build was run as:

```
make -C src -j8 EXTRA_CFLAGS="-isystem$HOME/.local/include"
LIBRARY_PATH=$HOME/.local/lib LD_LIBRARY_PATH=$HOME/.local/lib
```

`EXTRA_CFLAGS` is the Makefile's own wrapper hook; no repository file was changed
for this. Tests that invoke `g++` directly need `CPATH=$HOME/.local/include`
instead — without it `tests/async/test_world_recovery_pipeline.py` fails to
compile `hiredis/hiredis.h` on this host, unrelated to any change here.
