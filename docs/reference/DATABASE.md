# Database

DurisMUD selects one persistence backend for the whole server. The default
`mariadb-primary` mode stores durable player and domain state in MySQL/MariaDB.
A client-free build can instead select `flatfile-primary` and store that state
under `FLATFILE_STATE_DIR`. Mixed per-write failover is rejected. Typed player
and critical-command journals also retain durable local recovery records;
Redis supplies optional caches and world recovery. See
[ARCHITECTURE.md](ARCHITECTURE.md#persistence) for the backend boundaries.

This reference describes implementation base
`7f3da9c3a1b2b423da85a24abab603d8cdbee149` on `codex/docs-cleanup`, reviewed on
October 7, 2026. The architecture guide's
[newer branch implementations](ARCHITECTURE.md#newer-branch-implementations)
section identifies separately reviewed accounting, telemetry, and quest work;
documentation updates do not integrate those implementations.

The SQL connections, tables, and schema procedures below apply to
`mariadb-primary`. Setup steps (creating users/databases) are in
[README.md](../../README.md#3-create-a-development-database). An entity-relationship
diagram of the core tables is in
[diagrams/duris-database-model.html](../diagrams/duris-database-model.html);
its authority groups are traced to the bootstrap, immutable migrations, and runtime
manifests rather than presented as a column-complete schema reference.

## Connections and selection

See [CONFIGURATION.md](../operations/CONFIGURATION.md#persistence) for runtime
environment and target-validation details. `DB_NAME` is the requested database;
the resolved target is subject to both the port safety redirect and the allowlist.

In `mariadb-primary`, the server requires explicit `DB_HOST`, `DB_USER`,
`DB_PASSWD`, and `DB_NAME` values from the process environment after loading
`.env`; `DB_PORT` is optional
and must be valid when present. It has no compiled credential or target
defaults. The resolved `host/database` pair must also appear in
`DB_ALLOWED_TARGETS`; remote TCP targets require verified TLS. See
[CONFIGURATION.md](../operations/CONFIGURATION.md) for parsing, trust-boundary, and precedence
details.

The production-role plain-telnet port is configured by `DURIS_PRODUCTION_PORT`,
defaulting to `7777`. The listen port applies an additional safety redirect in
`sql_persistence_db_name()` (`src/sql/sql.c`):

| Condition | Effective database |
|------|----------|
| Configured production-role port | Requested `DB_NAME` |
| Any other port, requested name `duris` or `duris_prod` | `duris_dev` |
| Any other port, another requested name | Requested `DB_NAME` |

With `ENVIRONMENT=production`, SQL configuration also refuses a listen port that
differs from the configured production-role port. For development, use
`ENVIRONMENT=local`, a different port, development credentials, and an explicitly
allowlisted disposable database. The port redirect does not make production
credentials safe to reuse locally.

Connection architecture:

- **Main connection** - owns boot verification and remaining synchronous legacy or
  administrative queries. It is not the normal player checkpoint, critical-command,
  maintenance, or existing-character login path.
- **Connection pool** (`src/sql/sql_pool.c`) - bounded, individually owned connections used
  by typed load, snapshot, critical-command, outbox, maintenance, locker, and retained
  compatibility workers. Acquire/release is mutex and condition-variable based.
- **Optional telemetry writer** - owns a private connection opened by
  `sql_open_telemetry_connection()` with `TELEMETRY_DB_USER` and
  `TELEMETRY_DB_PASSWD`. It uses the same resolved target, allowlist, and verified
  connection factory, without borrowing the main connection or pool or falling
  back to the gameplay credentials.
- **Failure behavior** - typed routes return unavailable, retryable, or fenced outcomes
  when a connection cannot be acquired. Player login has explicit synchronous load
  fallback paths; retained legacy item/scalar/large compatibility producers also
  retain synchronous fallback paths. A pooled connection does not by itself imply
  execution on a worker thread.

The verified connection factory establishes `utf8mb4`, UTC, strict SQL modes, and
READ-COMMITTED as the session isolation default. When the player-load repository
opens its own consistent read transaction, it requests `REPEATABLE READ` for that
transaction. Main and pooled connections have 10-second connect/read/write
timeouts; telemetry's private connection has 2-second connect/read/write timeouts
and a 2-second InnoDB lock-wait timeout. Remote targets require enforced TLS and CA
verification; a protected local loopback/socket path is the only plaintext exception.

## Persistence execution boundaries

| Boundary | Identity and ordering | Durable unit | Failure behavior |
|----------|-----------------------|--------------|------------------|
| Player load | Unique request ID, one PID | Consistent read transaction returning owned typed rows and explicit load outcome | Core identity/status and the death-recovery gate remain mandatory; secondary failures can admit a degraded character with ordinary saves fenced |
| Player checkpoint | PID plus monotonic revision | Journaled immutable snapshot and revision-guarded component transaction | Coalesce by PID; matching revision/component ACKs; attached operations require exact success; terminal release follows caller policy |
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

## External consumers and writers

The server owns gameplay state even when another service presents or
administers it. A website may read documented projections and maintain its own
application tables, but it must not issue direct SQL mutations against
MUD-owned player, account, item-custody, balance, auction, or lifecycle state.
Those mutations must enter through an authenticated, authorized, typed server
command so the game thread, revisions, ledgers, fences, and outbox remain one
coherent authority.

The DurisWeb bridge follows this boundary. Its authenticated administrative
character-deletion route is implemented by the MUD; auction bid and buy
mutations are intentionally not exposed because they require the live player's
expected wallet revision. Adding a web action means adding a typed server-side
contract and authorization policy, not granting a web database account broader
write access. See [api/durisweb.md](api/durisweb.md).

## Persistence observability

The shared SQL wrappers and instrumented worker executors record bounded,
metadata-only metrics. Wrapper calls receive a compile-time `file:function:line`
site; worker executors supply a source or semantic site. Context distinguishes
the main process, forked child, and event, locker, player-save, or player-load
worker. Statement classification records only a kind such as `select`, `insert`,
or `transaction`, never SQL bytes or values.

The fixed-capacity registry aggregates calls, failures, total and maximum
latency, and bounded latency buckets. When new sites exceed capacity, an
overflow counter increases instead of allocating memory. Snapshots are copied
under a short lock and sorted after unlock. Query execution never holds the
metrics lock and the record path performs no filesystem or network I/O.

This registry covers instrumented calls. Raw client calls, including some schema
preflight queries, bypass it. The telemetry repository executes SQL directly on
its private connection and exposes separate bounded transport/health counters;
its queries do not populate this per-site registry.

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
- `migrations/bootstrap_multithread_safe.sql` - fresh-install input containing the
  sealed 170-table baseline inventory. This is a required-table set, not an exact
  count of every table created by the bootstrap: it also precreates some later
  migration tables, and baseline adoption permits additional tables. The current
  runtime inventory is checked separately below.
- `migrations/schema_migration_v*.sql` -- incremental upgrades, versioned
  (accounts, hardcore, pets, obj UIDs, locker changes, ships/guilds retirements, ...).
- `migrations/run_migration.sh` -- the legacy additive upgrade/baseline-adoption path;
  re-runnable by design.
- `migrations/migration_manifest.json` and `scripts/migration_runner.py` -- the
  immutable manifest-driven path for every migration after the verified Session 11
  baseline. The current manifest head is `0065_zone_reset_item_birth_origin` at
  sequence 65. The compiled/runtime head remains `0064_auction_custody_history`
  at sequence 64 with the 230-table runtime compatibility contract.
  Manifest/runtime alignment remains pending; a full manifest application is
  not proof of current SQL boot compatibility. See
  [IMMUTABLE_MIGRATIONS.md](../persistence/IMMUTABLE_MIGRATIONS.md) and
  [RUNTIME_COMPATIBILITY.md](../persistence/RUNTIME_COMPATIBILITY.md).
- `migrations/runtime_compatibility_manifest.json` and
  `migrations/verify_runtime_compatibility.sh` -- the read-only pre-boot contract for
  migration history, full metadata shape, storage engine, collation, and supported
  MySQL 8.0/MariaDB 10.11 variants. See
  [RUNTIME_COMPATIBILITY.md](../persistence/RUNTIME_COMPATIBILITY.md).

### Applying schema changes

The commands below mutate schema or migration history unless marked read-only. Qualify them only
against an empty disposable database or a backed-up development clone. Confirm the
resolved host, port, and database against the approved target before connecting, and
set the tool's allow-list where supported. Stop the game and every other writer first.
Never use production for migration discovery, replay, or validation; after clone qualification,
the runbook defines the separately authorized, backup-bound immutable production application.

These are operator requirements; the tools do not all enforce them. For the local
examples, use an isolated loopback target. The raw `mysql` command and Python runner
read exported process settings; the Python runner does not load `.env` itself.
`MIGRATION_ENV_FILE` selects configuration only for the legacy shell runner, not the
other tools listed here.

```bash
# Empty disposable loopback database only. Export the selected clone settings as in README.
# Mutates schema; raw mysql does not enforce Duris target or backup checks.
MYSQL_PWD="$DB_PASSWD" mysql --host="$DB_HOST" --port="${DB_PORT:-3306}" \
  --user="$DB_USER" "$DB_NAME" < migrations/bootstrap_multithread_safe.sql
# Mutates baseline history, then applies pending immutable schema/history changes.
python3 scripts/migration_runner.py adopt --kind fresh_bootstrap
python3 scripts/migration_runner.py run

# Local development database only. --help is safe; there is no dry-run mode.
# A normal invocation mutates immediately and records verified legacy adoption as its
# final database gate. When REDIS=TRUE, it then deletes only Duris-owned key patterns from
# the explicit local REDIS_HOST:REDIS_PORT/REDIS_DB target in REDIS_ALLOWED_TARGETS.
# Stop the game and every other Redis writer first; Redis failure fails the migration.
# Keep this owner-readable clone configuration separate from the server's .env.
MIGRATION_ENV_FILE=/path/to/owner-readable-clone.env ./migrations/run_migration.sh

# With the clone settings exported again after legacy adoption, apply immutable migrations:
python3 scripts/migration_runner.py run

# After exact clone qualification, owner authorization, writer shutdown, and a fresh backup only:
# Export the separately approved production settings before this mutating command.
python3 scripts/migration_runner.py run \
  --confirm-production-target "$DB_HOST/$DB_NAME" \
  --production-backup /absolute/path/to/fresh-production.sql.gz

# Read-only verification before starting the server or promoting a tested schema:
./migrations/verify_runtime_compatibility.sh
```

Scoped persistence/auction repair tools exist for archive-restored clones. Select
the isolated loopback clone using their configuration rules below:

```bash
# Read-only database verification.
./migrations/verify_persistence_contract.sh
# Mutates schema, then verifies it; replace the placeholder with the exact clone name.
./migrations/apply_persistence_contract.sh --confirm-db <clone_db_name>
```

Configuration and enforced safeguards differ by entry point:

| Tool | Configuration source | Enforced checks and limits |
|------|----------------------|----------------------------|
| Raw bootstrap `mysql` command | Exported `DB_*` values in the command | No Duris role, allow-list, backup, or writer checks; the operator must select the empty isolated target. |
| `scripts/migration_runner.py` | Process environment only | Non-production SQL commands require a local/development/dev/test role and loopback host and reject production-named databases. Production `run` requires exact `host/database` confirmation and `DB_ALLOWED_TARGETS` membership, a validated backup, and no other target-database connections at its preflight. Remote production connections require verified TLS with a CA file. See the immutable migration runbook for the complete procedure. |
| `migrations/run_migration.sh` | `MIGRATION_ENV_FILE`, else `migrations/.env`, else repository `.env` | Archive-column preflight and fail-closed steps; no early role/loopback/SQL allow-list or verified-TLS gate before DDL. Its verified baseline-adoption gate comes after the schema steps. It can leave partially committed DDL on failure. |
| `migrations/verify_persistence_contract.sh`, `migrations/apply_persistence_contract.sh` | Process environment if `DB_HOST`, `DB_USER`, `DB_PASSWD`, and `DB_NAME` are all present; otherwise `migrations/.env`, then repository `.env` | Apply checks `--confirm-db` against both `DB_NAME` and the selected SQL database. Neither tool enforces clone isolation, a role/allow-list, backup validation, or writer shutdown. |
| `migrations/verify_runtime_compatibility.sh` | Process environment when `DB_HOST` is present; otherwise repository `.env` | Read-only schema/history checks; does not enforce a role/target allow-list or backup/writer preflight. |

The scoped persistence tools and runtime compatibility verifier use
`--ssl-mode=PREFERRED`, or `--skip-ssl` on clients without that option, for TCP
connections. They do not inherit the server's verified-TLS policy. The legacy runner
also does not set verified-TLS options. Keep these examples on the isolated loopback
clone; a remote operation needs a separately reviewed connection procedure.

Migration rules:

- Migrations live in `migrations/` -- that directory is authoritative.
- Never edit an applied immutable migration's SQL/verifier body, manifest identity,
  or checksums. Append a new manifest step for a new change and preserve the sealed
  history.
- Keep new schema changes additive and guarded where practical. The legacy upgrade
  path is re-runnable by design; that does not permit rewriting applied immutable
  steps or bypassing their history checks.
- Never run against a live database: back up, restore into a clone, validate
  replay against the clone first.
- Schema changes should come with a focused regression test where practical
  (several exist under `tests/async/run_*_schema_mysql.sh`; root-level
  `tests/test_migration_replay_safety.sh` checks replay safety).

## Tables worth knowing

These rows describe SQL storage roles in this checkout. The runtime compatibility
manifest provides the exact inventory; accounting coverage, activation, and release
qualification requirements remain in
[ECONOMY_ACCOUNTING.md](../persistence/ECONOMY_ACCOUNTING.md) and its delivery plan.

| Table | Content |
|-------|---------|
| `player_data`, player component tables, `accounts`, `account_characters` | Character/account state and identity |
| `pages`, `mud_info` | Help system content, MOTD/news/wizlist (see [HELP_SYSTEM.md](../content/HELP_SYSTEM.md)) |
| `critical_operation_inbox` result fields, `critical_outbox` | Idempotent critical operations and delivery state |
| `item_current_owner`, `item_ownership_ledger` | Authoritative item custody and immutable ownership history |
| `currency_wallet_baseline`, `currency_bank_baseline`, `currency_ledger`, `epic_balance_baseline`, `epic_ledger` | Opening wallet/bank/epic balances and committed deltas used to reconcile the materialized gameplay balances |
| `economic_epoch`, `economic_lineage_state`, `economic_account_mapping`, `economic_baseline_*`, `economic_sql_*` | Accounting lineage/epoch and account identities, baseline reservations/witnesses, and guarded SQL installation/activation evidence |
| `economic_accounting_*`, `economic_pending_claim_source` | Typed operation receipts, account effects, coin postings, child-operation and item references, and source-claim/allocation evidence for implemented accounting routes |
| player revision/domain tables | Current revisioned snapshot and transactional gameplay state |
| `player_death_disposition`, `player_death_custody`, `player_death_conflict_evidence`, `player_death_restitution_*` | Durable death disposition and custody, conflict evidence, and reviewed restitution receipts, staged items, delivery, and resume state |
| `quest_reward_obligation`, `quest_reward_xp_entitlement` | Frozen reward continuation attached to the committed offering, acknowledgement state, and per-recipient XP entitlements recoverable at login |
| `player_spell_effect_receipt`, `player_craft_progression` | Spell-effect receipts committed with player state; craft terms frozen in the item root and marked applied with the owning player snapshot |
| `saved_item_recovery_handoff` | Durable replacement-graph acknowledgement required before retiring the named saved-item source root |
| `collector_catalog_state`, `collector_deaths`, `collector_listings`, `collector_ledger`, `collector_reconciliation_quarantine` | Collector catalog revision, death/listing authority records and held payloads, transaction history, and unresolved reconciliation evidence |
| `telemetry_*` | Observation configuration, sessions/intervals, encounter/combat summaries, derived rollups/reward projections, and quarantine; these records do not authorize gameplay balance or custody changes |
| archive/export/erasure tables | Guarded lifecycle job, evidence, package, request, and tombstone state |
| `mud_schema_baselines`, `mud_schema_history`, `mud_schema_migration_state`, `lookup_dataset_state` | Migration and runtime compatibility identity |
| persistence event tables | Remaining bounded compatibility events; not the player/critical authority |
| frag leaderboard tables | Auto-populated as players log in and save |
| `corpses`, `corpse_items`, `corpse_item_affects`, `corpse_item_extra_descr`, `corpse_catalog_state` | Revisioned player corpses and contained payloads across restarts, plus the catalog revision used to serialize corpse changes (see below) |
| `kingdom_realms` | Guild kingdom realm territory (one claim integer per guild), harvested resource stores, and upkeep/arrears state; created by immutable migration 0006, read positionally by `src/kingdom/kingdom_db.c`, and included in the current runtime compatibility inventory and sealed metadata fingerprints |
| `towns`, `kingdom_land`, `siege_items`, `siege_item_affects`, `siege_item_extra_descr` | Retired siege-era schema tombstones retained in the lifecycle and compatibility manifests. Runtime SQL must not revive them; the current kingdom system owns `kingdom_realms` instead. |

### Player corpses

`corpses` holds the outer corpse object, `corpse_items` its normalized
contents. `sql_save_corpse()` deletes and reinserts the row on every save, so
`created_at` is the last save time, not the death time -- the stable
`save_id` (corpse value 6) is the incident identifier and decodes to the death
timestamp.

Current SQL corpse persistence also carries `corpses.corpse_revision` and the
singleton `corpse_catalog_state.catalog_revision`. The save path locks the catalog
and commits its revision advance with the corpse/item graph. Typed lifecycle
commands in `src/persistence/corpse_lifecycle_repository.c` handle release,
destruction, resurrection, and follower creation through the critical-command
transaction, coordinating corpse/catalog revisions with item-custody transfers
and other required domain effects before live publication.

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
uses **named enum constants for positional numeric indexes** (`CORPSE_COL_*`) and
checks the result field count against `CORPSE_COL_COUNT`. The SELECT order and enum
order must stay aligned: the count check rejects a different column count but
cannot detect a reorder with the same count. The display fields were off by one
from the day they were added (April 2026) and shifted
again when `ci.obj_uid`/`ci.item_condition` were inserted ahead of them, which
made every restored corpse display as the first contained item's condition
(`100`) and then persisted that back to SQL on the next save. And when an item
row fails to load, the loader records that `last_item_id` no longer names the
object at `obj_map[num_objs - 1]`, so a following affect row for that item is
not applied to a different object.

## Consistent player load

Existing-character login normally submits to the bounded queue and worker in
`src/player/player_load_pipeline.c`. Account character selection and legacy login
also have explicit `player_load_pipeline_execute_sync()` paths when submission is
refused or selected outcomes require a retry. The SQL synchronous path borrows a
pool connection and runs repository reads on the calling thread.

`src/player/player_load_repository.c` opens a consistent read transaction and
loads status, skills, affects, current item ownership, item metadata, pet rows,
and pet item metadata through set-based queries into owned DTOs. The worker never
creates or traverses live `P_char` or `P_obj` instances. Publication and any
degraded-state decisions during materialization belong to the game thread.

Admission distinguishes three outcomes:

- **Normal load** - request identity and PID match, core status is valid, requested
  components and configured limits pass, and the item/pet graphs validate. ID maps
  provide linear assembly.
- **Degraded admission** - valid core identity/status can survive selected failures
  in secondary components, items, pets, gameplay reads, bank state, or recovery
  projections. A deadline or budget failure after core loading can also produce an
  explicitly degraded result. The affected state is omitted or quarantined, and
  `CHAR_RFLAG_LOAD_DEGRADED` fences ordinary checkpoints and terminal saves so an
  incomplete runtime projection cannot overwrite durable state.
- **Refused load** - invalid identity, schema, or core status cannot be admitted.
  Unresolved death-recovery conflicts or an unavailable death-recovery gate also
  refuse normal loading. Completions that no longer match the pending request are
  discarded; selected failure outcomes may be retried synchronously before the
  caller decides whether login can proceed.

An otherwise valid load with missing item payloads retains its valid remaining
graph and sets `CHAR_RFLAG_LOAD_ITEM_PAYLOAD_GAP`. Ordinary saves remain fenced.
The narrow terminal-save exception is an immutable typed death disposition that
records the captured graph and quarantines matching durable custody; it still
requires an applied-state acknowledgement. Other degraded loads cannot use that
exception. The standalone database harness is
`tests/async/run_player_load_repository_mysql.sh`.

## Critical transactions and current item ownership

The critical-command repository uses prepared statements and a stable 128-bit
operation ID. In one InnoDB transaction it creates or rereads the inbox identity,
locks domain rows in deterministic key order, applies typed state and ledger changes,
stores the canonical result, and inserts any outbox record. Exact duplicate delivery
with matching command identity returns the stored result after any required checks of
retained evidence; conflicting reuse of the operation ID fails. If commit acknowledgement
is ambiguous, the coordinator rereads by operation ID instead of replaying an
unidentifiable mutation.

`item_current_owner` is the authoritative custody row for each item UID.
`item_ownership_ledger` records immutable transfers, and ownership operations also
advance the affected inventory/domain revisions and outbox state in the same critical
transaction. Expected-owner mismatch, missing parent, invalid containment, conflicting
operation identity, or write failure rolls back without publishing in-memory movement.
Use a named reconciliation tool and mode; the `reconcile_` prefix does not imply
read-only behavior. Do not repair ledgers by hand.

| Tool/mode under `migrations/` | Effect | Configuration source |
|-------------------------------|--------|----------------------|
| `reconcile_epic_balances.sh`, `reconcile_currency_balances.sh` | Reports balance/baseline/ledger mismatches without database writes. | Sources repository `.env` when present, overriding matching process values; otherwise uses process settings. |
| `reconcile_item_ownership.sh`, `reconcile_auction_transactions.sh` | Reports custody/revision/quarantine drift without durable table changes. Item reconciliation also invokes nesting `--check`, which creates and populates connection-local temporary tables. | Always sources repository `.env`. |
| `reconcile_coin_custody_pair.sh --classify /absolute/private/pair.tsv` | Reads candidate evidence without durable database changes and writes an owner-only local artifact; refuses to overwrite an existing artifact. | Process environment when `DB_HOST` is present; otherwise repository `.env`. |
| `reconcile_coin_custody_pair.sh --apply /absolute/private/pair.tsv SHA256` | Mutates a reviewed `player_items.obj_uid` projection in a guarded transaction and writes a local receipt. | Same as classify; additional clone-only gates apply below. |

The epic/currency reports have no role or SQL target allow-list gate. Item/auction
reports require environment and database names containing `dev`, `local`, or `test`;
that name check does not prove clone isolation. All four reports use preferred TLS
or disable TLS according to client support, rather than enforcing server identity
verification for their own queries. Use them on an isolated loopback clone with the
checkout's configuration selected deliberately; `MIGRATION_ENV_FILE` does not select
their target.

Coin-custody apply requires a development/local/test role, one of `duris_dev`,
`duris_local`, or `duris_test`, an exact `DB_ALLOWED_TARGETS` entry in
`host:port/database` form, an owner-only reviewed artifact with the supplied SHA-256,
`WRITERS_QUIESCED=TRUE`, and a nonempty `COIN_CUSTODY_BACKUP_ID`. These last two values
are operator declarations, not proof that writers stopped or that a backup is valid.
The script checks live row evidence and CHECK-constraint enforcement before its
guarded correction. Both modes require verified TLS with a CA file for a remote host.

`root_item_uid` and `parent_item_uid` make containment part of that authority, not a
derived convenience. A transfer refuses any subtree whose recorded nesting disagrees
with the live object tree, and player load rebuilds nesting from these columns rather
than from the saved custody rows, so a command that moves an item into a container
without submitting a transfer strands the container: it can no longer be given or
dropped, and its contents un-nest on the next login. Every command that reparents a
generic-ownership item must therefore submit a transfer, including a move within a
single owner. `migrations/reconcile_item_ownership.sh` reports such drift as
`nesting_mismatch`, and `migrations/repair_item_nesting.sh` repairs it from the saved
container linkage. `--check` reports without changing durable tables, but creates and
populates connection-local temporary tables; it needs permission to use them. The
default invocation mutates nesting. The repair rewrites only
`parent_item_uid` and `root_item_uid`, never ownership or `item_revision`, which stay
ledger-derived.

Nesting repair always sources repository `.env`; it does not honor
`MIGRATION_ENV_FILE` or provide an `--env-file` option. Rehearse from an isolated clone
checkout whose own `.env` selects the development clone. Its environment/database
name guards check for `dev`, `local`, or `test`, and remote hosts require verified TLS
with a CA file. It does not enforce the server's `DB_ALLOWED_TARGETS` gate or validate
backup and writer shutdown. Follow the clone and production-correction requirements
below even when the name checks pass.

For production-safe read-only classification, use:

```bash
python3 scripts/classify_item_topology.py --env-file /absolute/private/production.env
```

This runs one aggregate-safe snapshot query and reports separate counts for expected
quarantined/inactive lifecycle rows, acyclic projection drift repairable by the existing
tool, and corruption. Categories cover duplicate or ambiguous payloads, missing
payload owners, missing payload/current parents, cycles, excessive depth,
owner/context disagreement, vnum/state differences, and item-revision evidence.
It never enables the development-only mutation script.

At an approved quiesced save boundary, stop the MUD, web process, workers, and every SQL
writer, then write exact identifiers only to an existing owner-only directory:

```bash
install -d -m 0700 /private/item-topology
python3 scripts/classify_item_topology.py \
  --env-file /absolute/private/production.env \
  --artifact /private/item-topology/classification.tsv
```

Artifact mode refuses while another target-database connection exists. The file is mode
`0600` and contains payload parent/root, current parent/root, owner/context, vnum, state,
and revision evidence; stdout remains aggregate-only. Record its SHA-256 without copying
rows into tickets or logs.

Rehearse `migrations/repair_item_nesting.sh` only against a fresh, isolated production
clone configured as development. Its existing guards repair only
`repairable_projection_lag`; owner disagreement, missing/foreign ancestors, cycles,
depth failures, revision mismatches, and ambiguous evidence require a separately reviewed
narrow correction. Compare byte-level fingerprints for every unaffected payload,
`item_current_owner`, baseline, quarantine, and ledger row before and after rehearsal.

Any production correction requires explicit owner authorization, a fresh validated
backup, the same protected artifact digest, writer quiescence, exact transactional row
guards, and captured rollback evidence. Never relax the mutation script's production
refusal. Before restart, require zero foreign-key violations and run the UID allocator,
item ownership, nesting, runtime compatibility, health, and log checks. Restore the exact
backup on any discrepancy; do not regenerate UIDs, delete rows, reassign owners, or
disable foreign keys to force a clean report.

## Maintenance and data lifecycle

The maintenance scheduler gives each registered job a stable offset, row/time budget,
continuation cursor, retry classification, and game-thread completion. It persists
cursor/completion state under `MAINTENANCE_STATE_FILE`. The archive job is present but
disabled in the compiled registry until lifecycle policy is approved and the manifest
allows canonical mutation.

`migrations/data_lifecycle_manifest.json` inventories every database and non-database
store and classifies subject mapping, purpose, season behavior, retention, archive,
export, erasure, and protected exceptions. Inventory validation rejects missing stores
or inconsistent policy metadata, but accepts the intentionally pending decisions and
requires destructive rules to remain disabled. Passing that check establishes a valid
inventory, not permission to perform destructive work. The separate destructive
preflight refuses execution while approval is pending. Archive, export, and erasure
schemas and operator scripts are implemented, but canonical mutation remains disabled
where controller decisions are pending. This is an engineering control record, not
legal advice. See
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
revision cannot replace a newer one. Ambiguous commits are reconciled against the
durable revision and any attached operation receipts. Worker completions acknowledge
only the matching inflight request revision and component mask, preserving newer
dirty state. A save carrying a death disposition, quest XP, spell-effect, or craft
receipt requires success at that exact requested revision before acknowledging the
attached operation to live state.

Journal retirement has a separate proof policy. Ordinary snapshots superseded by a
verified durable revision may be retired. An obsolete non-death frame carrying
operation receipts may be retired only after every attached operation is explicitly
verified; a newer revision alone is insufficient. Successful retirement of a death
disposition requires exact request proof. Replay orders records by PID and revision,
suppresses exact duplicate payloads, and preserves corruption evidence under the
journal's quarantine policy.
Retryable or ambiguous application results stop the current replay pass. Other
application failures or missing operation proof can retain the evidence in protected
PID quarantine and continue with unrelated players. Unresolved retained quarantine,
scan failures, or failure to retain the required evidence can still block replay. See
[PLAYER_SAVE_JOURNAL.md](../persistence/PLAYER_SAVE_JOURNAL.md).

Terminal callers choose the durability policy; the save-intent name alone does not
define it. An applied-state acknowledgement is named `database_acknowledged` in the
API; in a client-free flatfile build it confirms the selected native backend's commit.
A `journal_durable` result confirms a synced recovery record, without proving that
the backend has applied it.

| Terminal route | Required durability |
|----------------|---------------------|
| Camp or inn rent through `persistence_save_character_terminal()` | Applied-state acknowledgement within the 5-second wait; journal-only handoff is disabled. |
| Typed death disposition through `player_save_pipeline_terminal_death()` or its resume path | Exact pinned request, revision, and component acknowledgement; journal-only handoff is disabled and retries preserve the immutable request. |
| Copyover through `persistence_save_character_terminal_database_acknowledged()` | Applied-state acknowledgement within the 5-second wait, followed by the required lifecycle drains. |
| Other calls to the generic terminal helper, including `RENT_LINKDEAD`, `RENT_CRASH`, and the ordinary `RENT_DEATH` checkpoint path | Applied-state acknowledgement or explicitly permitted journal handoff within the 2-second wait. The client-free flatfile build disables journal-only handoff. |
| Client-free flatfile terminal writes through `writeCharacter()` | Applied-state acknowledgement within the 5-second wait; journal-only handoff is disabled. |
| Ordinary shutdown or reboot | Per-player `RENT_CRASH` terminal policy, followed by the required lifecycle drains. |

Copyover and ordinary shutdown quiesce and drain the relevant critical-command,
outbox, player, and world-recovery pipelines. The player pipeline drain waits for
accepted append work, including the journal dispatcher's inflight record, to become
journal-durable; it does not establish that every queued worker save is applied.
Failure of a required terminal save or drain cancels the transition and resumes the
live game loop. A player transition that lacks its required durability retains live
state for retry. See [PLAYER_SAVE_PIPELINE.md](../persistence/PLAYER_SAVE_PIPELINE.md).

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
- `scripts/cycle_mud.sh` attempts to insert boot/shutdown timestamps and stop reasons
  into `server_reboots` after exit in the SQL-required modes (`mariadb-primary` and
  `mariadb-primary-flatfile-fallback`). `flatfile-primary` skips this SQL bookkeeping.
  The insert is best-effort: the launcher prints its reboot-log message without
  checking whether the insert succeeded ([RUNBOOK.md](../operations/RUNBOOK.md)).
