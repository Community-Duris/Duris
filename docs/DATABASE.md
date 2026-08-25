# Database

DurisMUD stores all durable state in MySQL/MariaDB. This document covers how
the server talks to the database, what tables matter, and how schema changes
are managed. Setup steps (creating users/databases) are in
[README.md](../README.md#database-setup). An entity-relationship diagram of
the core tables is in
[diagrams/duris-database-model.html](diagrams/duris-database-model.html);
column details there are verified against
`migrations/bootstrap_multithread_safe.sql`.

## Connections and selection

Credentials are compile-time constants in `src/sql.h` (`DB_HOST`, `DB_USER`,
`DB_PASSWD`, `DB_NAME`) under `#ifdef TEST_MUD`. Changing them requires a
rebuild.

The listen port selects the database at runtime (`initialize_mysql()` in
`src/sql.c`):

| Port | Database |
|------|----------|
| 7777 (production default) | `duris` |
| any other port | `duris_dev` |

Never point a test run at production: use a non-7777 port for all development.

Connection architecture:

- **Main connection** — used synchronously by the game loop for gameplay
  queries (e.g. help lookups, property reads).
- **Connection pool** (`src/sql_pool.c`) — fixed-size pool of
  `CLIENT_MULTI_STATEMENTS`, `utf8mb4` connections shared by the three async
  persistence workers. Acquire/release is mutex + condition-variable based;
  shutdown waits for borrowers before closing.
- **Fallback** — if the pool fails to initialize, workers execute saves
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
  refuses to start if missing — a broken schema fails fast instead of losing
  saves silently.
- Retry/backoff exists for transient MySQL failures (see the dirty-flush and
  shopkeeper retry regression tests in `tests/async/`).

Redis complements MySQL: it buffers dirty-save state and holds periodic
world-state snapshots used for crash recovery after an unclean exit
(`src/redis.c`). Snapshots are cleared after successful recovery.

## Schema layout

- `migrations/bootstrap_legacy_baseline.sql` — legacy baseline schema
  (players, accounts, pages, mud_info, ...). Still the quickest way to see
  table definitions.
- `migrations/bootstrap_multithread_safe.sql` — the authoritative fresh-install
  baseline for this branch.
- `migrations/schema_migration_v*.sql` — incremental upgrades, versioned
  (accounts, hardcore, pets, obj UIDs, locker changes, ships/guilds retirements, ...).
- `migrations/run_migration.sh` — the single entrypoint that applies the
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

- Migrations live in `migrations/` — that directory is authoritative.
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

## Operational notes

- Connection problems at boot print `MySQL initialization failed! Dying!` —
  troubleshooting steps in [README.md](../README.md#troubleshooting).
- The cycle script records boot/shutdown timestamps and stop reasons into the
  database for reboot tracking ([RUNBOOK.md](RUNBOOK.md)).
