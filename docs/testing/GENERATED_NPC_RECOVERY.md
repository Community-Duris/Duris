# Generated NPC identity recovery (#245)

Vnums 1255 and 1256 are shared templates. Their generated strings, race, class,
level, size, base statistics, combat bases, damage dice, spell slots, affects,
aggression, act flags and all four coin balances belong to each instance. Reloading only the template
loses that state. The new bounded GNP1 extension carries those fields using the
existing portable state codec, without applying pet ownership or summon timers.

File copyover version 14 appends an extension after each NPC's inventory. The
fixed record layout stays unchanged, and readers still accept versions 12/13.
Redis's existing record codec accepts an optional validated extension after its
affects/transport data; ordinary wire records remain byte-compatible. Recovery
applies owned strings and base attributes before affects/equipment, then updates
derived values and reinstates the saved resource values. Existing instance IDs,
rooms and combat references retain their normal recovery path.

File copyover retains all four NPC coin balances. Redis world capture deliberately
excludes currency from its recoverable view; its generated extension also zeros
all four balances, avoiding both stale replay and random template-wallet values.
The Redis journey checks this policy separately from retained identity/stats.

## Local validation

Both flatfile and MariaDB builds pass. Changed lines were formatted through
`scripts/format.sh`; `git-clang-format --diff` reports no changes.

- `test_generated_npc_state.py`: production capture/apply, file extension helpers
  and Redis wire codec under ASan/UBSan, for both vnums over five cycles. Checks
  exact encoded state, string ownership, ordinary control, legacy handling,
  truncations, oversized data and vnum mismatch.
- `test_world_recovery_codec.py`, `test_world_recovery_pipeline.py`,
  `test_world_singletons.py`, and `test_copyover_custody.py`: existing production
  recovery, singleton, transport and custody contracts, including sanitizer
  runs and actual custody save/exec/recovery.
- `test_redis_fault_recovery_live.py` and `test_redis_floor_store_live.py`: real
  isolated Redis, timeout/uncertain publication and asynchronous floor barriers.
- `run_generated_npc_journey.py <server> file`: real account creation, offline
  promotion of the disposable staff character, string/setattr/stat/look/scan,
  two live file copyovers, stable IDs/counts/base stats and same-socket saves.
- `run_generated_npc_journey.py <mariadb-server> redis`: a uniquely named local
  MariaDB schema and private Redis process, acknowledged snapshot, forced
  process crash, clean reboot, reconnect and retained NPC identity/state.
  Requires `TEST_DB_HOST`, `TEST_DB_USER`, `TEST_DB_PASSWORD`; the host must be
  loopback. Redis needs a SQL season epoch, so the flatfile-only server cannot
  run this mode. The test does not read a checkout `.env`.

The player journeys use controlled instances of 1255/1256 whose strings are set
through real staff commands. They do not run the full random-world generator.
That distinction does not change the capture/recovery path under test. The unit
test supplies independent generated attributes beyond those edited by commands.
The patch does not extend the older file-copyover NPC equipment representation,
which still uses vnums; full generated-equipment preservation is a separate
existing limitation and is not claimed by these tests.

## Older state and population

An old snapshot has no information from which the original generated identity
can be recovered. It remains readable, retains its existing instance, and emits
an explicit recovery-review log with vnum, instance and room. An already
degraded, unowned template is represented by an empty extension on subsequent
captures so it cannot block saving the entire world. No name is invented, no
new random individual is substituted, and no shared prototype string is freed.
New generated instances with owned strings retain their state thereafter.

The earlier issue audit attributed possible boot growth to `create_randoms()`'s
15-map-NPC loop. That body is under `#ifndef RANDOM_ZONES`, while the shipped
configuration defines `RANDOM_ZONES`; the loop is excluded. The issue was
corrected rather than changing inactive boot-generation behavior. Repeated live
recovery asserts the two controlled counts independently of their names.
