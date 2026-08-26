# Database

DurisMUD stores all durable state in MySQL/MariaDB. This document covers how
the server talks to the database, what tables matter, and how schema changes
are managed. Setup steps (creating users/databases) are in
[README.md](../README.md#3-create-a-development-database). An entity-relationship
diagram of the core tables is in
[diagrams/duris-database-model.html](diagrams/duris-database-model.html);
column details there are verified against
`migrations/bootstrap_multithread_safe.sql`.

## Connections and selection

See [CONFIGURATION.md](CONFIGURATION.md) for the complete environment-variable
reference. In particular, `DB_NAME` is the requested name, while the runtime
port safety rule can redirect an implicit production name to `duris_dev` on a
non-`7777` port.

The server reads `DB_HOST`, `DB_PORT`, `DB_USER`, `DB_PASSWD`, and `DB_NAME`
from the process environment after loading `.env`; `src/sql.h` supplies
compiled fallback defaults. `TEST_MUD` changes the fallback database from
`duris` to `duris_dev`, but it does not override explicit environment values.
See [CONFIGURATION.md](CONFIGURATION.md) for parsing and precedence details.

The listen port applies a production safety redirect in
`sql_persistence_db_name()` (`src/sql.c`):

| Condition | Effective database |
|------|----------|
| Port `7777` | Requested `DB_NAME` (normally `duris`) |
| Any other port, requested name `duris` or `duris_prod` | `duris_dev` |
| Any other port, another requested name | Requested `DB_NAME` |

Never point a test run at production: use a non-7777 port, a development
credential, and a disposable database for all development.

Connection architecture:

- **Main connection** -- used synchronously by the game loop for gameplay
  queries (e.g. help lookups, property reads).
- **Connection pool** (`src/sql_pool.c`) -- fixed-size pool of
  `CLIENT_MULTI_STATEMENTS`, `utf8mb4` connections shared by the three async
  persistence workers. Acquire/release is mutex + condition-variable based;
  shutdown waits for borrowers before closing.
- **Fallback** -- if the pool fails to initialize, workers execute saves
  synchronously on the main path rather than dropping writes.

## Async persistence

Player/object/ship saves are not written inline by game code. Instead they are
enqueued (`src/persistence_queue.c`) and drained by three worker threads:

| Worker | Payload |
|--------|---------|
| item | Object/item rows |
| scalar | Small field updates (stats, flags, counters) |
| large-payload | Big blobs (descriptions, mail, lockers) via raw SQL in `sql_persistence_raw.c` |

Properties of the pipeline:

- Dirty-flag driven: entities mark themselves dirty; the queue deduplicates
  pending saves per entity.
- Boot verifies the required tables and their idempotency/index contract and
  refuses to start if missing -- a broken schema fails fast instead of losing
  saves silently.
- Retry/backoff exists for transient MySQL failures (see the dirty-flush and
  shopkeeper retry regression tests in `tests/async/`).

Redis complements MySQL: it buffers dirty-save state and holds periodic
world-state snapshots used for crash recovery after an unclean exit
(`src/redis.c`). Snapshots are cleared after successful recovery.

## Persistence observability

All shared MySQL execution paths record bounded, metadata-only metrics. Wrapper
calls receive a compile-time `file:function:line` site; worker executors use an
explicit semantic site. Context distinguishes the main process, a fork child,
an event worker, and the locker worker. Statement classification records only a
kind such as `select`, `insert`, or `transaction`, never SQL bytes or values.

The fixed-capacity registry aggregates calls, failures, total and maximum
latency, and bounded latency buckets. When new sites exceed capacity, an
overflow counter increases instead of allocating memory. Snapshots are copied
under a short lock and sorted after unlock. Query execution never holds the
metrics lock and the record path performs no filesystem or network I/O.

Failure events may contain a process-local operation ID, source site, context,
statement kind, duration, numeric MySQL error code, and SQLSTATE. They do not
contain SQL text, MySQL error prose, player/account/item values, or filesystem
paths. Operation IDs reset with the process and must not be used as durability,
transaction, replay, or idempotency identifiers.

## Schema layout

- `migrations/bootstrap_legacy_baseline.sql` -- legacy baseline schema
  (players, accounts, pages, mud_info, ...). Still the quickest way to see
  table definitions.
- `migrations/bootstrap_multithread_safe.sql` -- the authoritative fresh-install
  baseline for this branch.
- `migrations/schema_migration_v*.sql` -- incremental upgrades, versioned
  (accounts, hardcore, pets, obj UIDs, locker changes, ships/guilds retirements, ...).
- `migrations/run_migration.sh` -- the single entrypoint that applies the
  additive upgrade path; re-runnable by design.

### Applying schema changes

```bash
# Fresh database:
mysql -u duris -p duris_dev < migrations/bootstrap_multithread_safe.sql

# Existing populated database:
./migrations/run_migration.sh
```

Scoped persistence/auction repair tools exist for archive-restored clones:

```bash
./migrations/verify_persistence_contract.sh
./migrations/apply_persistence_contract.sh --confirm-db <clone_db_name>
```

Rules of thumb (enforced by repo conventions):

- Migrations live in `migrations/` -- that directory is authoritative.
- Keep them additive, guarded (`IF NOT EXISTS` / conditional columns), and
  re-runnable.
- Never run against a live database: back up, restore into a clone, validate
  replay against the clone first.
- Schema changes should come with a focused regression test where practical
  (several exist under `tests/async/run_*_schema_mysql.sh`; root-level
  `tests/test_migration_replay_safety.sh` checks replay safety).

## Tables worth knowing

| Table | Content |
|-------|---------|
| `players`, `accounts`, `account_characters` | Character/account state |
| `pages`, `mud_info` | Help system content, MOTD/news/wizlist (see [HELP_SYSTEM.md](HELP_SYSTEM.md)) |
| persistence/event tables | Async save queues consumed by the workers (boot validates their indexes) |
| frag leaderboard tables | Auto-populated as players log in and save |
| `corpses`, `corpse_items` | Player corpses across restarts (see below) |

### Player corpses

`corpses` holds the outer corpse object, `corpse_items` its normalized
contents. `sql_save_corpse()` deletes and reinserts the row on every save, so
`created_at` is the last save time, not the death time -- the stable
`save_id` (corpse value 6) is the incident identifier and decodes to the death
timestamp.

Beyond `player_name`, `save_id`, `room_vnum` and the display strings, the table
stores the corpse's own `name` (owner keywords), `weight`, and values 0-5 and 7:
death-time level, owner PID, recoverable death XP, race-war side, race, and the
flag set including the humanoid and carved-part bits. All of it matters --
`spell_resurrect()` reads value 4, necromancy gates on `CORPSE_LEVEL`,
`do_carve()` requires `HUMANOID_CORPSE`, and artifact looting checks the
race-war side before rebinding. Restoring a corpse from prototype `#2` alone
(zero values, generic keywords, weight 200) silently changes all of those after
a restart, which is what happened before `migrations/corpse_persistence_state.sql`
added the columns.

Those columns are nullable on purpose. The migration reconstructs only what the
table guarantees -- player-corpse classification and owner keywords -- and leaves
unknown legacy weight, level, PID, XP loss, race-war side, and race as `NULL`
rather than inventing values; the loader has runtime fallbacks for them. New
corpses store the complete state.

Two conventions in `sql_load_all_corpses()` are worth preserving. The loader
reads **named result columns, not numeric indexes**, and asserts the expected
field count so a query edit cannot silently shift the mapping: the display
fields were off by one from the day they were added (April 2026) and shifted
again when `ci.obj_uid`/`ci.item_condition` were inserted ahead of them, which
made every restored corpse display as the first contained item's condition
(`100`) and then persisted that back to SQL on the next save. And when an item
row fails to load, the loader records that `last_item_id` no longer names the
object at `obj_map[num_objs - 1]`, so a following affect row for that item is
not applied to a different object.

## Operational notes

- Connection problems at boot print `MySQL initialization failed!` --
  troubleshooting steps are in [README.md](../README.md#troubleshooting), and
  the effective database host, port, and selected database are logged before
  the connection is opened.
- The cycle script records boot/shutdown timestamps and stop reasons into the
  database for reboot tracking ([RUNBOOK.md](RUNBOOK.md)).
