# Database

DurisMUD stores all durable state in MySQL/MariaDB. This document covers how
the server talks to the database, what tables matter, and how schema changes
are managed. Setup steps (creating users/databases) are in
[README.md](../../README.md#3-create-a-development-database). An entity-relationship
diagram of the core tables is in
[diagrams/duris-database-model.html](../diagrams/duris-database-model.html);
its authority groups are traced to the bootstrap, immutable migrations, and runtime
manifests rather than presented as a column-complete schema reference.

## Connections and selection

See [CONFIGURATION.md](../operations/CONFIGURATION.md) for the complete environment-variable
reference. In particular, `DB_NAME` is the requested name, while the runtime
port safety rule can redirect an implicit production name to `duris_dev` on a
non-`7777` port.

The server requires explicit `DB_HOST`, `DB_USER`, `DB_PASSWD`, and `DB_NAME`
values from the process environment after loading `.env`; `DB_PORT` is optional
and must be valid when present. It has no compiled credential or target
defaults. The resolved `host/database` pair must also appear in
`DB_ALLOWED_TARGETS`; remote TCP targets require verified TLS. See
[CONFIGURATION.md](../operations/CONFIGURATION.md) for parsing, trust-boundary, and precedence
details.

The listen port applies a production safety redirect in
`sql_persistence_db_name()` (`src/sql/sql.c`):

| Condition | Effective database |
|------|----------|
| Port `7777` | Requested `DB_NAME` (normally `duris`) |
| Any other port, requested name `duris` or `duris_prod` | `duris_dev` |
| Any other port, another requested name | Requested `DB_NAME` |

Never point a test run at production: use a non-7777 port, a development
credential, and a disposable database for all development.

Connection architecture:

- **Main connection** - owns boot verification and remaining synchronous legacy or
  administrative queries. It is not the normal player checkpoint, critical-command,
  maintenance, or existing-character login path.
- **Connection pool** (`src/sql/sql_pool.c`) - bounded, individually owned connections used
  by typed load, snapshot, critical-command, outbox, maintenance, locker, and retained
  compatibility workers. Acquire/release is mutex and condition-variable based.
- **Failure behavior** - typed routes return unavailable, retryable, or fenced outcomes
  when a connection cannot be acquired. Only explicitly retained legacy item/scalar/
  large compatibility producers have the historical synchronous fallback.

Every connection uses `utf8mb4`, UTC, READ-COMMITTED isolation, strict SQL modes, and
10-second connect/read/write deadlines. Remote targets require enforced TLS and CA
verification; a protected local loopback/socket path is the only plaintext exception.

## Persistence execution boundaries

| Boundary | Identity and ordering | Durable unit | Failure behavior |
|----------|-----------------------|--------------|------------------|
| Player load | Unique request ID, one PID | One consistent read transaction returning owned typed rows | Required-component, limit, timeout, cancellation, or stale result publishes no character |
| Player checkpoint | PID plus monotonic revision | Journaled immutable snapshot and revision-guarded component transaction | Retry/coalesce by PID; exact ACK only; terminal action retains live state on failure |
| Critical command | Stable 128-bit operation ID plus sorted entity keys | Inbox, typed domain rows/ledgers, result, and outbox in one transaction | Duplicate/ambiguity rereads result; affected gameplay stays fenced through retry |
| Item ownership | Operation ID plus item UID | Current owner, immutable ownership ledger, both revisions, and outbox | Guarded expected-owner mismatch fails without partial movement |
| Maintenance | Stable job/work ID plus continuation | Bounded row/time batch and success-last cursor | Retryable failure retains cursor; permanent failure is visible; lifecycle slot is disabled |
| World recovery | Sequence, checksum, and item UID graph | Immutable Redis generation plus current-pointer publication; SQL custody remains authoritative | Floor and generation trees are planned together; every UID/root/parent/VNUM/room/state is reconciled before rollback-capable materialization |
| Legacy event compatibility | Event key/generation where supported | Remaining item/scalar/large event row | Bounded queue/retry; never the player or critical-operation authority |

The typed player and critical journals contain schema versions, checksums, bounds, and
restrictive-permission checks. Unrestricted raw SQL is not accepted as a new durable
message contract. The older `src/persistence/persistence_queue.c` and
`src/sql/sql_persistence_raw.c` modules remain only for compatibility producers still named
by source and health output.

Redis is not an authority for player dirty state. It holds floor-delta recovery data
and optional sequence-numbered world generations used after graceful restart or an unclean exit
(`src/redis/redis.c`). Recovery reads every referenced item from SQL in batches and accepts only
an exact active room-owned graph before creating entities. A generation is cleared only
after successful validated recovery and atomic runtime-custody hydration.

Critical gameplay commands are distinct from coalesced checkpoints. Each accepted
command is independently journaled with one stable operation ID and remains fenced
through retry or ambiguous completion. The generic transaction stores canonical
identity/result metadata in `critical_operation_inbox`, applies typed state, and inserts
`critical_outbox` rows before one commit. Duplicate and ambiguous execution reread the
inbox. Delivery is at least once with `(consumer_id,outbox_id)` dedupe, bounded retry,
and retained dead letters. See
[CRITICAL_COMMAND_PIPELINE.md](../persistence/CRITICAL_COMMAND_PIPELINE.md).

## Persistence observability

All shared MySQL execution paths record bounded, metadata-only metrics. Wrapper
calls receive a compile-time `file:function:line` site; worker executors use an
explicit semantic site. Context distinguishes the main thread and the relevant
event, locker, or player-save worker. Statement classification records only a
kind such as `select`, `insert`, or `transaction`, never SQL bytes or values.

The fixed-capacity registry aggregates calls, failures, total and maximum
latency, and bounded latency buckets. When new sites exceed capacity, an
overflow counter increases instead of allocating memory. Snapshots are copied
under a short lock and sorted after unlock. Query execution never holds the
metrics lock and the record path performs no filesystem or network I/O.

Redis workers and the remaining shared boot/recovery/maintenance command adapter expose
separate bounded local health snapshots. Shared commands retain only a redacted subsystem
class, operation kind, outcome counters, latency aggregates, last-success age, and primary
connection/reconnect transitions. Presence, report-cache, floor, donation, and world
publication workers retain matching operation counters, latency aggregates, categorized
failures, failure streaks, and last-success age alongside existing bounded queue and
connection state. The typed snapshots feed both `redis detailed` and `world persistence`;
rendering either command performs no Redis query and stores no key, value, identity,
endpoint, or credential.

Failure events may contain a process-local operation ID, source site, context,
statement kind, duration, numeric MySQL error code, and SQLSTATE. They do not
contain SQL text, MySQL error prose, player/account/item values, or filesystem
paths. Operation IDs reset with the process and must not be used as durability,
transaction, replay, or idempotency identifiers.

## Schema layout

- `migrations/bootstrap_legacy_baseline.sql` - historical legacy input only; it is not
  the current install contract.
- `migrations/bootstrap_multithread_safe.sql` - the sealed 170-table fresh-install
  baseline for this branch.
- `migrations/schema_migration_v*.sql` -- incremental upgrades, versioned
  (accounts, hardcore, pets, obj UIDs, locker changes, ships/guilds retirements, ...).
- `migrations/run_migration.sh` -- the legacy additive upgrade/baseline-adoption path;
  re-runnable by design.
- `migrations/migration_manifest.json` and `scripts/migration_runner.py` -- the
  immutable manifest-driven path for every migration after the verified Session 11
  baseline. The current immutable head adds the `kingdom_realms` table, completing
  the 174-table boot contract. See
  [IMMUTABLE_MIGRATIONS.md](../persistence/IMMUTABLE_MIGRATIONS.md).
- `migrations/runtime_compatibility_manifest.json` and
  `migrations/verify_runtime_compatibility.sh` -- the read-only pre-boot contract for
  migration history, full metadata shape, storage engine, collation, and supported
  MySQL 8.0/MariaDB 10.11 variants. See
  [RUNTIME_COMPATIBILITY.md](../persistence/RUNTIME_COMPATIBILITY.md).

### Applying schema changes

The commands below mutate schema or migration history unless marked read-only. Run
them only against an empty disposable database or a backed-up development clone whose
resolved `host/database` is explicitly allow-listed. Stop the game and every other
writer first. Never use production for migration discovery, replay, or validation.

```bash
# Empty disposable database only. Load with the explicit .env target shown in README.
# This is a mutating operation.
MYSQL_PWD="$DB_PASSWD" mysql --host="$DB_HOST" --port="${DB_PORT:-3306}" \
  --user="$DB_USER" "$DB_NAME" < migrations/bootstrap_multithread_safe.sql
python3 scripts/migration_runner.py adopt --kind fresh_bootstrap
python3 scripts/migration_runner.py run

# Local development database only. --help is safe; there is no dry-run mode.
# A normal invocation mutates immediately and records verified legacy adoption as its
# final database gate. When REDIS=TRUE, it then deletes only Duris-owned key patterns from
# the explicit local REDIS_HOST:REDIS_PORT/REDIS_DB target in REDIS_ALLOWED_TARGETS.
# Stop the game and every other Redis writer first; Redis failure fails the migration.
# Keep this owner-readable clone configuration separate from the server's .env.
MIGRATION_ENV_FILE=/path/to/owner-readable-clone.env ./migrations/run_migration.sh

# After an adopted baseline, apply immutable post-baseline migrations:
python3 scripts/migration_runner.py run

# Read-only verification before starting the server or promoting a tested schema:
./migrations/verify_runtime_compatibility.sh
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
| `player_data`, player component tables, `accounts`, `account_characters` | Character/account state and identity |
| `pages`, `mud_info` | Help system content, MOTD/news/wizlist (see [HELP_SYSTEM.md](../content/HELP_SYSTEM.md)) |
| `critical_operation_inbox` result fields, `critical_outbox` | Idempotent critical operations and delivery state |
| `item_current_owner`, `item_ownership_ledger` | Authoritative item custody and immutable ownership history |
| player revision/domain tables | Current revisioned snapshot and transactional gameplay state |
| archive/export/erasure tables | Guarded lifecycle job, evidence, package, request, and tombstone state |
| `mud_schema_baselines`, `mud_schema_history`, `mud_schema_migration_state`, `lookup_dataset_state` | Migration and runtime compatibility identity |
| persistence event tables | Remaining bounded compatibility events; not the player/critical authority |
| frag leaderboard tables | Auto-populated as players log in and save |
| `corpses`, `corpse_items` | Player corpses across restarts (see below) |
| `kingdom_realms` | Guild kingdom realm territory (one claim integer per guild), harvested resource stores, and upkeep/arrears state; created by immutable migration 0006, read positionally by `src/kingdom/kingdom_db.c`, and part of the 174-table runtime boot contract, whose metadata fingerprints are sealed over it |

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

## Consistent player load

Existing-character login is asynchronous. `src/player/player_load_pipeline.c` owns one
bounded request queue and worker; `src/player/player_load_repository.c` borrows one validated
pool connection and opens a consistent read transaction. Required status, skills,
affects, current item ownership, item metadata, pet rows, and pet item metadata are
loaded in bounded set-based queries into owned DTOs. The worker never creates or
traverses live `P_char` or `P_obj` instances.

The game thread accepts a completion only when request identity and PID still match,
the durable revision is current, every required component succeeded, all configured
row/byte/depth limits hold, and the item/pet graphs validate. ID maps provide linear
assembly. Cancellation, timeout, missing component, malformed graph, overflow, and
stale completion all discard the DTO and fail login cleanly; no partial character is
published. The standalone database harness is
`tests/async/run_player_load_repository_mysql.sh`.

## Critical transactions and current item ownership

The critical-command repository uses prepared statements and a stable 128-bit
operation ID. In one InnoDB transaction it creates or rereads the inbox identity,
locks domain rows in deterministic key order, applies typed state and ledger changes,
stores the canonical result, and inserts any outbox record. Duplicate delivery returns
the stored result. If commit acknowledgement is ambiguous, the coordinator rereads by
operation ID instead of replaying an unidentifiable mutation.

`item_current_owner` is the authoritative custody row for each item UID.
`item_ownership_ledger` records immutable transfers, and ownership operations also
advance the affected inventory/domain revisions and outbox state in the same critical
transaction. Expected-owner mismatch, missing parent, invalid containment, duplicate
operation identity, or write failure rolls back without publishing in-memory movement.
Use the read-only reconciliation scripts in `migrations/reconcile_*.sh`; do not repair
ledgers by hand.

`root_item_uid` and `parent_item_uid` make containment part of that authority, not a
derived convenience. A transfer refuses any subtree whose recorded nesting disagrees
with the live object tree, and player load rebuilds nesting from these columns rather
than from the saved custody rows, so a command that moves an item into a container
without submitting a transfer strands the container: it can no longer be given or
dropped, and its contents un-nest on the next login. Every command that reparents a
generic-ownership item must therefore submit a transfer, including a move within a
single owner. `migrations/reconcile_item_ownership.sh` reports such drift as
`nesting_mismatch`, and `migrations/repair_item_nesting.sh` repairs it from the saved
container linkage (`--check` reports without writing). The repair rewrites only
`parent_item_uid` and `root_item_uid`, never ownership or `item_revision`, which stay
ledger-derived.

## Maintenance and data lifecycle

The maintenance scheduler gives each registered job a stable offset, row/time budget,
continuation cursor, retry classification, and game-thread completion. It persists
cursor/completion state under `MAINTENANCE_STATE_FILE`. The archive job is present but
disabled in the compiled registry until lifecycle policy is approved and the manifest
allows canonical mutation.

`migrations/data_lifecycle_manifest.json` inventories every database and non-database
store and classifies subject mapping, purpose, season behavior, retention, archive,
export, erasure, and protected exceptions. Validation fails closed on missing stores or
pending destructive rules. Archive, export, and erasure schemas and operator scripts
are implemented, but canonical mutation remains disabled where controller decisions
are pending. This is an engineering control record, not legal advice. See
[DATA_LIFECYCLE.md](../persistence/DATA_LIFECYCLE.md), [LIFECYCLE_ARCHIVE.md](../persistence/LIFECYCLE_ARCHIVE.md),
[PERSONAL_DATA_EXPORT.md](../persistence/PERSONAL_DATA_EXPORT.md), and
[ACCOUNT_ERASURE.md](../persistence/ACCOUNT_ERASURE.md).

## Active epic bonus read model

Active player epic bonuses are hydrated into fixed-capacity player-owned memory during
the database player load. The login query joins the selected `epic_bonus` row to the
union of historical positive, non-bottle `epic_gain` rows and committed positive,
non-bottle `epic_ledger` rows after both the selection time and configured rolling
cutoff, then groups them by calendar expiry boundary. The boundary calculation
preserves the strict cutoff for gains recorded exactly at midnight. It returns no more
than one row per supported expiry day rather than one row per historical gain.

The shipped rolling window is five days. The in-memory representation supports integer
windows from 1 through 31 days with at most 32 daily buckets. Invalid configuration,
malformed rows, query failure, or bucket overflow places that character's bonus state
in an explicit unavailable state and yields a zero modifier. It never triggers a lazy
query from regeneration, XP, shops, cargo, status, help, or award calculation.
Cap and maximum-modifier property changes take effect from the in-memory property table
on the next read. A rolling-window change marks existing player state unavailable until
the next login because already-expired history cannot be reconstructed without I/O.

Selection and qualifying award ACK paths update this state on the game thread. Daily
contributions expire locally at the same calendar boundary represented by the former
`CURDATE()` predicate. The state is an active-player read model, not a new durability
boundary. The materialized balance is `epic_balance_baseline.opening_balance` plus all
committed `epic_ledger.delta` values. `player_data.epics` and `epic_revision` are updated
atomically with each ledger row and are authoritative at login.

## Revisioned player checkpoints and terminal saves

Each player owns a monotonic revision plus per-component dirty, queued, inflight, and
acknowledged state. The game thread captures a bounded immutable snapshot without
unequipping objects or removing affects. A private append dispatcher writes typed,
checksummed journal records with restrictive permissions; only journaled snapshots are
submitted to the bounded 256-PID keyed worker queue. Same-PID work is ordered and
coalesced, while different PIDs may apply concurrently.

The repository locks the durable revision before replacing component rows. A stale
revision cannot replace a newer one. Ambiguous commits are reconciled by rereading the
durable revision, and exact completion alone clears the matching component state and
journal record. Replay suppresses duplicate PID/revision records, quarantines corrupt
frames, and stops fail-closed when durable application cannot proceed.

Camp, rent, death, idle/link-loss cleanup, ghost extraction, locker departure,
copyover, shutdown, and reboot use the same terminal fence. Live state may be released
only after the exact database ACK or an explicit durable journal handoff. Copyover and
shutdown quiesce and drain both player and world pipelines; failure cancels the
transition and resumes the live game loop.

## Player replacement components

`player_timers`, `player_undead_slots`, `player_forged_items`, and
`player_granted_cmds` are full replacement sets during a player status save. Each set
is deleted by PID inside the active player-save transaction before its current non-zero
entries are batch inserted. An empty in-memory set therefore removes every prior row
instead of allowing a cleared timer, slot, recipe-like forge entry, or revoked command
to return at the next login.

Every delete and insert is checked. A failure returns through the current transaction
owner: a direct status save rolls back its own transaction, while a full player save
rolls back the enclosing transaction. Languages and introductions use the same
replacement contract. These are save semantics only; no table or index shape changed.

## Operational notes

- Connection problems at boot print `MySQL initialization failed!` --
  troubleshooting steps are in [README.md](../../README.md#troubleshooting), and
  the effective database host, port, and selected database are logged before
  the connection is opened.
- The cycle script records boot/shutdown timestamps and stop reasons into the
  database for reboot tracking ([RUNBOOK.md](../operations/RUNBOOK.md)).
