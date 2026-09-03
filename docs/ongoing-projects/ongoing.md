# DurisWeb Database Improvement Plan

> Reviewed on 2026-09-03. This is an analysis of a historical database snapshot,
> not an audit of the live production database.

## 1. Scope, evidence, and source of truth

The primary artifact is `/home/aiwithapex/projects/durisweb/tmp/prod.sql`, a
MySQL dump completed on 2026-06-23 at 23:24:20. It is 345,366,975 bytes
(329.37 MiB) and has SHA-256
`8f882d16be743b42467890f7714052d64b1cf8eca75be89c77efaeb3d7e5cc44`.

The word "production" below refers only to the database name and metadata in
that dump. No live production database was queried. The deployed MUD and
DurisWeb revisions may also differ from the current checkouts, so live state
must be verified before any operational change.

### Evidence labels

| Label | Meaning |
| --- | --- |
| `SNAPSHOT` | Directly observed in the 2026-06-23 dump. Historical evidence only. |
| `WEB CODE` | Directly observed in the current DurisWeb checkout on 2026-09-03. |
| `LOCAL DB` | Read-only observation of the running `duris_dev` MySQL 8.0.46 container on 2026-09-03. |
| `MUD CODE` | Directly observed in the current sibling MUD checkout at `../duris`. This is the schema authority for game-owned tables, but is not proof of deployment. |
| `LIVE` | Unknown until an operator runs the read-only verification gate in Section 8. |

### Authority boundary

DurisWeb and the MUD share a database, but they do not share schema ownership:

- The MUD migration manifest and persistence contract own game state, including
  accounts, players, items, auctions, PvP data, and statistics.
- DurisWeb TypeScript migrations own web extension tables and must not alter a
  sealed MUD runtime shape without a coordinated contract change.
- The dump is migration input and historical evidence. It is not a canonical
  schema baseline.
- The current local database is a useful diagnostic target, but its mixed
  migration state is not a clean baseline either.

Relevant authority references are `docs/onboarding.md`, `docs/deployment.md`,
`backend/src/scripts/productionPreflight.ts`, and, in `../duris`,
`migrations/migration_manifest.json`, `migrations/persistence_contract.sql`,
and `migrations/runtime_compatibility_manifest.json`.

### Review method and limits

This review used:

- quote-aware parsing of extended `INSERT` statements, so delimiters embedded
  in terminal output and other text were not counted as rows;
- DDL, engine, collation, index, and migration-ledger inventory from the dump;
- searches of current non-test DurisWeb code and migrations;
- read-only `information_schema`, `SELECT`, and `EXPLAIN` queries against the
  local database; and
- the current MUD persistence migrations and auction implementation.

The dump was not restored, no database was mutated, no row-level production
values were copied into this document, and no production-scale query was
benchmarked. Dump payload sizes below are serialized `INSERT` payload, not
physical table or index sizes.

## 2. Executive decisions

1. **Stop treating direct web auction writes as safe.** `WEB CODE` commits the
   character debit before it starts a separate auction transaction, contains an
   integer timestamp mismatch, and bypasses the current MUD auction ledger,
   custody, and revision contract. Web auction browsing can remain read-only;
   bid, buy-now, and administrative removal must use an authenticated,
   idempotent MUD-owned command path.
2. **Disable direct duplicate-item deletion until it is MUD-authoritative.** The
   current bulk action can delete every locker copy while claiming to keep one,
   misses cross-`vnum` UID conflicts, and bypasses the MUD ownership ledger and
   quarantine system.
3. **Retire the web-owned player-wipe implementation.** It hard-codes a legacy
   table list while the current MUD owns the season-reset manifest, durable
   epoch, Redis invalidation, reward rules, and new persistence ledgers.
4. **Stop clearing SQL mode on pooled connections.** PvP interactions set the
   session SQL mode to an empty string and release the connection without
   restoring it, silently weakening later unrelated queries.
5. **Secure the raw dump.** It is gitignored, but its mode is `0644` inside a
   `0755` directory. It contains credentials, network identifiers, user content,
   and raw administrative terminal I/O. Gitignore is not an access control.
6. **Gate the current restore feature until recovery is proven.** Its "full"
   mode merges rows from only 51 hard-coded tables, three selective-restore
   mappings name columns that do not exist, and its UTF-8 conversion changes
   bytes in an auction BLOB from this dump. A stopped MUD is necessary but does
   not make this a coherent point-in-time restore.
7. **Build an explicit shared-schema baseline; do not add an automatic
   `migrate:latest` to startup.** Current Knex history is not replayable from an
   empty database and 14 SQL artifacts are ignored by the configured migrator.
8. **Fix schema consumers before recreating legacy tables.** `players_core` is
   legacy compatibility data outside the current MUD runtime contract. Refactor
   user management to canonical player and lookup tables instead of creating a
   new web-owned copy.
9. **Prioritize measured work.** `statistics(date)` is the strongest index
   candidate in this snapshot. Other proposed indexes belong behind query-plan
   and workload evidence rather than being described as proven production
   bottlenecks.

## 3. Evidence snapshot

### 3.1 Historical dump inventory

| Property | `SNAPSHOT` result |
| --- | --- |
| Dump client/server | `mysqldump` 8.0.46 / MySQL 8.0.46 Ubuntu |
| Database in header | `duris_prod` |
| Completion time | 2026-06-23 23:24:20 |
| Base tables | 190 |
| Engines | 155 InnoDB, 35 MyISAM |
| Table defaults | 174 `utf8mb4_0900_ai_ci`, 8 `utf8mb4_unicode_ci`, 8 `latin1` |
| Applied Knex rows | 74 TypeScript migrations, ending with `20260415120000_widen_forum_category_guild_name.ts` |
| Restore SQL mode | Temporarily replaced by `NO_AUTO_VALUE_ON_ZERO`, so strict zero-date behavior is not tested by restoring this file |
| Byte encoding | Not valid UTF-8: decoding first fails at byte 291,799 inside `auction_item_pickups.obj_blob_str` |
| Stored view security | Contains a `DEFINER=newduris@127.0.0.1` clause, which is not portable to arbitrary environments |

The 35 MyISAM tables are listed in Appendix A. This proves historical engine
drift, not current production engine state.

### 3.2 Quote-aware row and payload inventory

| Table | Rows in dump | `INSERT` payload | Interpretation |
| --- | ---: | ---: | --- |
| `player_item_extra_descr` | 289,015 | 73.73 MiB | Large game-owned state; do not purge from a web retention job |
| `page_views` | 195,092 | 67.00 MiB | High-volume web analytics with identifiers and URLs |
| `terminal_logs` | 21,742 | 37.30 MiB | Raw admin terminal input/output; unusually large rows |
| `wiki_map_positions` | 206,232 | 32.55 MiB | Large web projection used by viewport queries |
| `pkill_info` | 802 | 28.16 MiB | Few but very large PvP payloads |
| `log_entries` | 158,139 | 17.63 MiB | Game-owned logs; lifecycle must be coordinated with the MUD |
| `statistics` | 256,292 | 11.16 MiB | Time-series data with no snapshot index on `date` |
| `wiki_mobs` | 19,462 | 9.35 MiB | Generated/reference projection |
| `locker_items` | 56,264 | 8.29 MiB | Authoritative item state |
| `player_items` | 50,239 | 8.09 MiB | Authoritative item state |
| `server_health_metrics` | 54,037 | 5.79 MiB | Operational time series |
| `deployment_log` | 31 | 5.74 MiB | Very large captured output; review for secrets and truncation |
| `frag_leaderboard` | 1,060 | 0.09 MiB | Small materialized leaderboard |
| `account_characters` | 875 | 0.08 MiB | Small in this snapshot, but its next auto-increment was already 19,112,780 |
| `accounts` | 329 | 0.06 MiB | Small but sensitive |
| `auctions` | 249 | 0.20 MiB | Small historical workload |
| `pkill_event` | 187 | 0.01 MiB | Small historical workload |

The earlier count of 144,091 `terminal_logs` rows was caused by counting the
text `),(` inside stored terminal output. The quote-aware count is 21,742 and is
consistent with the table's next auto-increment value of 21,743.

### 3.3 Current repository and local database

| Property | Result |
| --- | --- |
| DurisWeb migration artifacts | 82 `.ts` files and 14 `.sql` files |
| Knex configuration | `backend/knexfile.ts` sets `extension: 'ts'`; the 14 SQL files are ignored |
| `LOCAL DB` migration status | 32 complete, 50 pending |
| `LOCAL DB` base tables | 221 |
| `LOCAL DB` engine/collation | All 221 InnoDB and `utf8mb4_unicode_ci` |
| Historical tables absent locally | 30 |
| Local tables absent from the old dump | 61, mostly evidence that the MUD schema advanced after the snapshot |
| `players_core` / `players_view` locally | Neither exists |

The dump itself has 74 applied migration rows, not 32. The 32/50 split belongs
only to today's local `duris_dev` database. Eight current TypeScript files are
absent from the dump ledger: the backfilled
`016a_bootstrap_admin_permission_tables.ts` and seven migrations dated after the
snapshot: `20260828172744_expand_web_session_refresh_token.ts`,
`20260828235000_add_manage_server_incidents_permission.ts`,
`20260831120000_secure_donation_delivery.ts`,
`20260901000000_hook_toggles.ts`,
`20260902143000_preserve_mud_runtime_contract.ts`,
`20260902144000_add_mud_ws_host.ts`, and
`20260902190000_backfill_required_web_settings.ts`.

Thirty snapshot tables are absent locally:

```text
account_login_history
admin_permission_audit
builder_flags
deployment_log
donations
gemini_analysis_log
help_file_suggestions
mud_restores
notifications
players_core
prepstatment_duris_sql
server_health_metrics
server_incidents
siege_objects
suspicious_accounts
user_bans
website_changelog
website_changelog_reads
wiki_continents
wiki_map_positions
wiki_mob_flags
wiki_mobs
wiki_object_affects
wiki_object_classes
wiki_object_races
wiki_object_slots
wiki_object_spell_effects
wiki_objects
wiki_settings
wiki_zone_entrances
```

Current non-test backend code references 28 of these names. The two without a
current reference are legacy `prepstatment_duris_sql` and `siege_objects`.
Absence from this one local database does not establish that all 28 tables are
mandatory in every deployment; it establishes that feature readiness is not
currently represented by one reliable baseline or preflight.

Observed local behavior is more specific than "all status and wiki routes
500":

- wiki settings and endpoints that query the missing wiki tables fail;
- `/api/status/history` and `/api/status/incidents` fail when their tables are
  absent;
- `/api/status` can still return degraded data because some failures are caught;
- `/api/status/uptime` silently returns `100` when its query fails, which can
  report false health; and
- periodic health recording logs and suppresses insert failures.

There is no current database named `duris_migtest`, so the earlier claim that a
clone accepted all migrations and that 72 suites / 593 tests passed is not
reproducible from retained evidence. It is intentionally not used as a finding.

## 4. Priority findings and required changes

### P0-A: Route auction mutations through the MUD transaction boundary

#### Proven defects

- `SNAPSHOT` and `LOCAL DB`: `auction_bid_history.date` is a signed `INT` Unix
  epoch field.
- `WEB CODE`: `placeBid()` inserts `NOW()` into that field. In numeric context
  MySQL 8 produces a value like `20260903114410`, which exceeds signed `INT`.
- `WEB CODE`: `getAuctionBidHistory()` calls `UNIX_TIMESTAMP(date)` even though
  `date` is already epoch seconds. The local read-only check
  `UNIX_TIMESTAMP(1767225600)` returned `0` for a representative epoch value;
  selecting `date` directly is the correct contract.
- `WEB CODE`: the bid and buy routes call `deductCharacterMoney()`, which opens
  and commits its own transaction, and only then call `placeBid()` using another
  connection and transaction. The failure branch says "Refund" but performs no
  refund. A validation failure, race loss, timestamp error, or database failure
  after the first commit can therefore remove money without recording the bid.
- `SNAPSHOT`: the four auction tables were MyISAM, so their transaction and
  `FOR UPDATE` semantics were ineffective in that historical database.
- `LOCAL DB`: all four auction tables are now InnoDB.
- `MUD CODE`: `migrations/persistence_contract.sql` explicitly converts all
  four tables to InnoDB, while `src/economy/auction_repository.c` locks the
  wallet and auction and updates wallet value, previous-bidder refund, auction
  revision, pickup/custody state, and auction ledger within the authoritative
  critical command. The current schema includes state absent from the old dump.

The first-pass proposal to change only `NOW()` and add a DurisWeb engine
migration is insufficient. It would leave the independently committed wallet
debit and would let the web path bypass MUD revisions and ledgers. A duplicate
web-owned engine migration would also violate the ownership boundary.

#### Required change

1. If web bid, buy-now, or admin removal endpoints are enabled, gate their
   mutation path until an authoritative command is available. Keep auction
   browsing and history read-only.
2. Add authenticated, authorized, idempotent auction commands to the MUD bridge
   and call them from DurisWeb. Include a command/dedupe ID and return the
   committed auction and wallet revisions.
3. Remove direct DurisWeb writes to `player_data`, `auctions`, auction pickup
   tables, and auction ledgers from these flows.
4. For the read-only history query, select integer `date` directly. Any temporary
   direct-write compatibility code must use `UNIX_TIMESTAMP()`, but that is not
   a substitute for the command cutover.
5. Extend release preflight to verify the MUD auction contract and all four
   InnoDB engines before exposing mutations.

#### Acceptance evidence

- Concurrent bids cannot both debit and win.
- Fault injection after every write leaves wallet, auction, custody, pickups,
  and ledgers reconciled.
- Retrying the same command is idempotent.
- Outbid refunds, same-bidder increases, buy-now, and administrative removal
  pass integration tests against the canonical MUD schema.
- Bid history preserves the epoch value and renders the expected time.

### P0-B: Replace destructive web duplicate cleanup with MUD reconciliation

`WEB CODE` exposes the duplicate tools only to `requireOverlord`, but privilege
does not make the data operation safe:

- detection groups by `(obj_uid, vnum)`, so the same global UID attached to two
  different `vnum` values is not reported if each pair occurs once;
- `deleteAllDupesForUid()` keeps the lowest `player_items.id` and then deletes
  every matching `locker_items` row. If a duplicate exists only in lockers, it
  deletes every copy while its route says it keeps an original;
- individual and bulk delete operations directly mutate authoritative item
  tables without ownership revisions, audit ledger events, or quarantine;
- the MUD schema now has `item_current_owner`, `item_ownership_ledger`, and
  `item_ownership_quarantine`, which the old dump and web delete path do not
  represent; and
- `obj_uid` is `BIGINT UNSIGNED`, but the service, request validator, and mysql2
  configuration use JavaScript `number`. Values above `Number.MAX_SAFE_INTEGER`
  cannot be represented safely.

#### Required change

1. Disable the four direct delete operations; retain a read-only diagnostic UI.
2. Detect conflicts by global `obj_uid` first, then separately flag inconsistent
   `vnum` metadata. Join player, locker, and wiki metadata only for candidate
   UIDs, which also fixes the expensive join-before-aggregation pattern.
3. Return and accept UIDs as canonical decimal strings, and enable mysql2 big
   number string handling at the relevant boundary.
4. Send reviewed repair requests through a MUD-owned reconciliation/quarantine
   operation that records before/after ownership evidence and is safe while the
   game is running. Do not decide which copy is authoritative from the smallest
   surrogate row ID.

#### Acceptance evidence

- Tests cover locker-only duplicates, inventory-only duplicates, mixed
  inventory/locker duplicates, same UID with different `vnum`, nesting, values
  above `2^53 - 1`, retries, and a concurrent MUD save.
- The MUD ownership verifier passes and every repair has an auditable operation
  or quarantine record.
- Query plans show metadata joins scale with duplicate candidates rather than
  all 106,503 snapshot item rows.

### P0-C: Route player/season reset through the MUD owner

`WEB CODE`: `/api/admin/mud/wipe/execute` requires Overlord confirmation and
checks that the MUD appears stopped, but then implements its own reset by
deleting from a hard-coded set of game tables. It deletes lockers, player data,
auction pickups, guild/progression/PvP data, and other state in a web transaction
and treats legacy `players_core` as a best-effort exception.

That implementation no longer describes the current persistence model:

- `MUD CODE`: `sql_pwipe()` has a schema manifest and preflights persistence
  event columns/indexes and auction engines before its first destructive step.
- It establishes a durable `season_reset_state` epoch/boundary, validates and
  clears Redis administrative state, and applies account-bound reward policy.
- The current runtime manifest contains ownership, ledger, baseline, outcome,
  custody, and reset-state tables absent from the old dump and the web route's
  list. The local database has 61 tables that the snapshot did not have.
- A process-state observation is not a durable fence; the MUD can be started or
  another maintenance operation can begin after the web check.
- `SNAPSHOT`: many tables in the web route's deletion set were MyISAM, so the
  route's transaction would not have been atomic against that historical
  schema. `LOCAL DB` is all InnoDB, but `LIVE` remains unknown.

#### Required change

1. Disable the direct SQL reset endpoint. Let the web UI request and monitor a
   MUD-owned season-reset operation or an equivalent offline MUD maintenance
   tool; it must not carry a second table manifest.
2. Use an authenticated operation ID, durable maintenance/season fencing, an
   explicit approval record, and idempotent resume/status behavior.
3. Preserve excluded-player behavior only if the MUD owner defines and verifies
   it across every dependent ledger and global season rule. Otherwise remove
   that option rather than implementing a partial reset.
4. Keep web audit/history as a projection of the authoritative operation and
   its result, including the deployed revision and manifest checksum.

#### Acceptance evidence

- A dry run records bounded counts and validates the complete MUD reset manifest
  before authorization to execute.
- Missing tables, wrong engines, concurrent maintenance, or unavailable Redis
  fail before the destructive boundary.
- Retry, crash/restart, and duplicate-request tests converge on one season epoch.
- Item, auction, persistence, account-reward, and Redis reconciliation gates
  pass before the season returns to active.
- A backup/restore rehearsal proves the operator recovery procedure.

### P0-D: Stop pooled-session SQL mode poisoning in PvP interactions

`WEB CODE`: `addBattleLike()`, `removeBattleLike()`, and
`createBattleComment()` each execute `SET sql_mode = ''` to work around the
legacy `pkill_event.stamp` default. SQL mode is session-scoped. None of the
functions captures or restores the prior value before releasing the connection
to the mysql2 pool, and the pool configuration does not reset session state on
checkout. A later unrelated request can therefore inherit a connection with
strict, zero-date, division, and grouping safeguards disabled.

There is a second historical boundary problem: `SNAPSHOT` has the web-owned
`pvp_battle_likes`, `pvp_battle_comments`, and `pvp_battle_favorites` tables on
InnoDB but `pkill_event` on MyISAM. The like/comment row and its denormalized
counter were not one atomic transaction in that schema. `LOCAL DB` has all of
them on InnoDB, but `LIVE` is unknown.

#### Required change

1. Do not change session SQL mode inside request handlers. Gate the affected
   interaction writes if they cannot succeed under the configured strict mode.
2. Normalize the MUD-owned `pkill_event.stamp` contract as described in P1-G,
   then remove all three workarounds. If a short-lived compatibility measure is
   unavoidable, capture and restore the exact session value in `finally` before
   release and discard the connection if restoration fails.
3. Verify the target event exists and that social-row/counter changes are one
   InnoDB transaction. Prefer deriving counts from indexed web-owned rows or a
   reconciled web projection instead of requiring web counters on a MUD-owned
   table.
4. Add pool checkout assertions/telemetry for required session invariants:
   SQL mode, time zone, isolation level, and foreign-key checks.

#### Acceptance evidence

- A freshly checked-out connection has the configured strict SQL mode after
  successful, duplicate, validation-failure, and injected database-error paths.
- Like/comment operations cannot create an orphan interaction or a counter
  mismatch for a missing/concurrently changed PvP event.
- A reconciliation query returns zero counter drift, and feature preflight
  rejects a mixed-engine boundary before writes are enabled.

### P0-E: Put the dump under real access and retention controls

`SNAPSHOT`: `tmp/prod.sql` is mode `0644` and `tmp/` is mode `0755`. The dump
contains password hashes, email and network identifiers, account and forum
content, analytics identifiers, and raw administrative terminal input/output.
It also contains destructive `DROP TABLE` statements, binary blobs, a fixed
view definer, and session-setting changes.

The enclosing home directory currently prevents traversal by unrelated users,
but the artifact itself still grants group/other read bits and those bits follow
ordinary copies unless explicitly changed. Its leaf permissions are broader
than this data requires.

Required operator action:

1. Restrict the artifact to the minimum OS identity (`0600` or a more tightly
   controlled encrypted store), record its approved owner and expiration, and
   ensure backups and copies follow the same controls.
2. Do not attach it to tickets, paste values into logs/docs, commit it, or load
   it into a shared developer database.
3. Securely remove it and unmanaged copies after the approved analysis/retention
   window. This document does not delete it because destruction requires an
   explicit operator decision.
4. If a restore is required, use an isolated disposable database with no
   outbound application connections, neutralize definers before export, and
   follow the recovery redesign in P0-F.

### P0-F: Gate restore until it is coherent, byte-safe, and rehearsed

The admin UI says that **Full Restore** restores all game data and overwrites
existing data. The current implementation does neither. It is a row merge with
an incomplete, stale domain manifest, and the snapshot exposes failures that
the synthetic unit fixtures conceal.

#### Proven defects

- `WEB CODE`: `filterDumpForFullRestore()` extracts rows from only 51 hard-coded
  `ALL_RESTORE_TABLES`, and `buildRestoreSql()` emits columnless `REPLACE INTO`
  statements. It does not clear rows created after the backup and ignores every
  table outside that list. Even the primary `auctions` table is excluded while
  three auction pickup/history tables are included.
- `LOCAL DB`: current ownership and transaction state such as
  `item_current_owner`, `item_owner_revision`, `item_ownership_ledger`,
  `auction_item_custody`, `auction_ledger`, currency/epic/combat ledgers, and
  `season_reset_state` is outside the restore list. Restoring older player,
  item, pickup, or balance rows while retaining newer authoritative ledger and
  revision rows cannot represent one valid point in time.
- `WEB CODE`, `SNAPSHOT`, and `LOCAL DB`: three selective-restore mappings name
  nonexistent columns: `accounts.name` must be `account_name`,
  `player_data.id` must be `pid`, and `player_pets.pid` must be `owner_pid`.
  `buildFilterColumnIndex()` silently omits a table when its configured column
  is absent. Account restore therefore omits the account and player core rows;
  character restore omits `player_data`; and pet restore omits the pet parent
  and its dependent item chain.
- The unit tests encode the same invented `accounts.name`, `player_data.id`,
  and `player_pets.pid` shapes, so they validate the implementation against
  fixtures that disagree with both inspected schemas.
- Eight restore-listed tables already have different column counts between the
  snapshot and local schemas: `account_banks`, `auction_money_pickups`,
  `corpse_items`, `corpses`, `locker_items`, `player_data`, `player_items`, and
  `player_pet_items`. For example, `player_data` grew from 105 to 109 columns.
  Because restore SQL has no explicit column list or compatibility manifest,
  this old dump is rejected by the current target rather than mapped through a
  reviewed upgrade. On any MyISAM target, earlier `REPLACE` statements would
  also sit outside the advertised rollback transaction.
- `SNAPSHOT`: the file is not valid UTF-8; the first invalid sequence occurs at
  byte 291,799 inside the restore-listed `auction_item_pickups.obj_blob_str`
  BLOB. `WEB CODE` buffers and calls `.toString('utf-8')` in listing,
  validation, and execution. A read-only 128-byte round-trip around that offset
  grew to 132 bytes and introduced two replacement characters. Re-emitting the
  parsed row therefore changes authoritative serialized item bytes. The backup
  command also omits `--hex-blob`.
- Both `mysqldump` and `mysql` run through `child_process.exec()` with database
  values interpolated into a shell command. The password is placed on the
  client command line; quotes or shell metacharacters in configuration can
  break or alter the command.
- A 500 MiB compressed upload limit is not an uncompressed-size limit. The SQL
  entry is fully decompressed into memory, converted to a string, and scanned
  repeatedly. There is no archive expansion-ratio/entry-size budget, streaming
  bound, or production-scale test for a 329.37 MiB dump.
- Upload validation checks only for two `CREATE TABLE` strings. Execution
  checks that the MUD appears stopped and that no recent restore row exists,
  but not archive checksum, source/target identity, schema and manifest
  versions, table coverage, column compatibility, engine atomicity, free space,
  or post-restore reconciliation.
- `--single-transaction` gives a consistent nonblocking snapshot only for
  transactional tables. It would not make a coherent backup of the 35 MyISAM
  tables seen historically. `LOCAL DB` has converged to InnoDB, but `LIVE` must
  be checked before relying on that option.

Current `master` has one relevant improvement: backup and restore now both use
the validated `environment.mudDatabase.connection` configuration. This closes
the earlier environment-prefix/port mismatch, but an archive still has no
manifest binding its captured source identity to the approved restore target.

#### Required change

1. Gate both stored-backup and uploaded-backup restore endpoints and clearly
   mark existing archives as **unverified for recovery**. Continue creating and
   protecting backups if needed; do not advertise recoverability until a drill
   passes.
2. Define two different products. Disaster recovery must restore the declared
   complete database scope into an empty, isolated target at one coherent point
   in time. Selective account/character recovery must be a versioned,
   MUD-owned import or compensation operation that updates ownership, custody,
   revisions, ledgers, caches, and audit evidence together. Do not implement
   either as an unqualified `REPLACE` merge.
3. Publish a signed or access-controlled manifest with the archive checksum,
   source identity, capture time, engine/version, schema/baseline versions,
   migration checksums, included tables, byte and row counts, consistency
   method, and required restore tooling. If web and MUD data are separated,
   explicitly capture both or narrow the recovery claim.
4. Preserve bytes end to end. Stream dumps and restores without UTF-8 decoding;
   use a logical format with `--hex-blob` when SQL text must contain BLOBs.
   Bound compressed and uncompressed sizes, entry count, expansion ratio,
   memory, disk, and execution time before accepting an upload.
5. Use `spawn`/`execFile`-style argument arrays and streams rather than a shell.
   Supply client credentials through a protected temporary defaults file or an
   equivalent secrets mechanism, remove it in `finally`, and fail closed unless
   the resolved target exactly matches the approved manifest and application
   configuration.
6. Restore first into a disposable database, run the MUD migration and runtime
   compatibility verifiers, compare manifest checksums/counts, boot both
   applications in an isolated mode, and reconcile every ownership and balance
   ledger. Promote or perform the approved offline restore only after those
   gates pass.
7. Record and rehearse RPO, RTO, restore ownership, maintenance fencing,
   rollback/fail-forward procedure, credential access, archive retention, and
   evidence of the most recent successful drill.

#### Acceptance evidence

- This historical dump is rejected before any target write because its schema
  does not match the current manifest, or it is byte-preservingly restored to
  staging and upgraded through a reviewed path. It is never partially merged
  into a current database.
- A newly generated backup restores into an empty disposable target with the
  declared complete table set. Row/count checks, sampled or full checksums,
  application smoke tests, and all MUD ownership/currency/auction verifiers
  pass; data created after the backup is absent as point-in-time semantics
  require.
- Selective-recovery tests use canonical account, player, and pet schemas and
  prove recovery of core rows and every dependent domain without ledger,
  revision, cache, or ownership drift.
- Invalid UTF-8 BLOB fixtures round-trip byte-for-byte, malicious/oversized
  archives fail within bounded resources, client secrets do not appear in
  process arguments, and injected failures cannot leave a partial restore.
- Operations retain a dated drill record with measured RPO/RTO and the exact
  archive, source, target, code, schema, and manifest revisions.

### P1-A: Establish a reproducible shared-schema baseline and migration policy

The migration problem is architectural, not just "49 migrations are pending":

- the MUD creates and migrates core tables outside Knex;
- historical numeric and timestamp names sort differently from their real
  deployment order;
- some TypeScript migrations defensively skip work when a prerequisite table is
  absent, which can record success without producing the intended schema;
- 14 SQL files are invisible because Knex is configured for `.ts` only; and
- current local, historical dump, and current MUD schemas each represent a
  different point in time.

There are concrete ownership violations in released history as well.
`039_standardize_all_collations.ts` labels its targets as web tables but includes
MUD runtime tables such as `server_reboots` and `frag_leaderboard`.
`20260123034134_fix_mud_table_collations.ts` disables foreign-key checks and
alters `accounts`, account/guild tables, and `player_data`; its down path targets
MySQL-only `utf8mb4_0900_ai_ci`. Do not rewrite applied history, but classify
these as legacy exceptions, preserve the current forward repair, and prevent
new cross-boundary migrations in review/CI. Likewise, ignored
`005_create_accounts_table.sql` attempts to create a MUD-owned table, so the SQL
artifacts cannot safely be treated as a set of merely pending migrations.

Required change:

1. Publish a versioned, schema-only baseline built from the canonical MUD
   bootstrap plus a clearly owned DurisWeb extension baseline. Do not derive it
   from player rows.
2. Reconcile `knex_migrations` as an explicit, reviewed baseline operation.
   Never mark a migration applied unless its postcondition has been checked.
3. Classify each ignored SQL artifact as superseded, converted to an idempotent
   TypeScript migration, or deliberately retained as documentation. Do not
   simply enable all extensions and replay both implementations.
4. Keep timestamp prefixes for new migrations. Do not rename already released
   files or collapse history in place; introduce a new baseline/version boundary
   and preserve the tested upgrade path.
5. Add two disposable-database CI paths: clean canonical baseline to head, and
   previous supported baseline to head. Run the MUD runtime compatibility
   verifier and DurisWeb feature-contract checks on both MySQL and the deployed
   MariaDB family where supported.
6. Rebuild local development from that approved baseline. Only after this work
   may startup automation safely apply pending DurisWeb migrations.

This agrees with the warning already present in `docs/onboarding.md`; adding an
unconditional `pnpm migrate:latest` to `scripts/dev.sh` now would be unsafe.

### P1-B: Remove the `players_core` dependency and fix the alignment query

`SNAPSHOT` includes `players_core`; `LOCAL DB` includes neither that table nor
`players_view`. `MUD CODE` describes `players_core` as a legacy table to remove
and provides an optional `players_view` derived from `player_data`, `races`, and
`classes`. The legacy import documentation retains `players_core` only as an
extension table outside the game runtime contract.

`WEB CODE` still joins and queries `players_core` in
`userManagementService.ts`, and the admin wipe performs a best-effort delete
from it. In addition, the alignment filter adds `pd.racewar = ?` without ever
joining `player_data pd`, so that filtered query fails even where
`players_core` exists.

Required change:

- Read player IDs, level, race/class IDs, and alignment from canonical
  `player_data`; join MUD-populated `races` and `classes` for labels.
- If a compatibility view is preferred, make it an explicit MUD-owned contract
  and preflight it; do not recreate a writable `players_core` table in Knex.
- Remove the case-folded name join and the legacy wipe delete after cutover.
- Cover unfiltered listing, race, class, alignment, search, sort, pagination,
  and wipe behavior with integration tests against the canonical baseline.

### P1-C: Make preflight describe feature readiness, not just migration names

`WEB CODE`: `productionPreflight.ts` checks 12 tables, the names of all 82
TypeScript migration files, one detailed `server_reboots` shape, one foreign-key
boundary, and refresh-token width. It does not check auction column types and
engines, the current MUD schema version, most feature tables, or ignored SQL
artifact postconditions. `verifyDatabaseSchema()` only proves that two PvP
tables exist; its `DESCRIBE` results are unused.

Required change:

- Define a machine-readable ownership and feature contract with required table,
  column, engine, collation, index, and schema-version postconditions.
- Add an operation-level allowlist for every DurisWeb mutation of a MUD-owned
  table. Inventory existing direct writes (including account metadata/password,
  PvP counters, statistics, MUD content/zones, reboot observations, auctions,
  item cleanup, and resets), document the authoritative writer and concurrency
  contract, and gate any unclassified write in review/CI.
- Validate only enabled features, but fail startup or keep a feature gated when
  its contract is absent. Do not silently return fabricated success values such
  as 100 percent uptime on query failure.
- Resolve and compare the configured backup source, restore target, schema
  manifest, engine guarantees, and latest successful recovery drill before any
  restore control is exposed.
- Invoke the MUD runtime compatibility verifier for MUD-owned structures and a
  separate DurisWeb verifier for extension structures.
- Treat migration names as provenance, not proof that schema postconditions are
  present.

The snapshot's 35 MyISAM tables and mixed collations remain conditional live
verification items. If either drift still exists, the owning repository must
plan the conversion on a representative clone, including rebuild time, locks,
free space, replication impact, and rollback. Today's local database already
has every base table on InnoDB and `utf8mb4_unicode_ci`, so the snapshot alone
does not justify duplicate DurisWeb conversion migrations.

### P1-D: Add a privacy and lifecycle program for web-owned telemetry

The snapshot proves accumulation but not the correct legal or operational
retention period. Periods must be approved from actual product, security, and
regulatory needs rather than copied from this report.

| Data | Objective improvement |
| --- | --- |
| `terminal_logs` | Stop storing raw keystrokes and raw terminal output by default. Prefer structured command audit, redact secrets, cap row size, tightly restrict access, and use a short documented retention period. |
| `deployment_log` | Redact environment and credential material before persistence, cap/truncate output, and separate outcome metadata from bulky logs. |
| `page_views` / `visitor_sessions` | Minimize or pseudonymize IP/account identifiers, strip URL query strings and unnecessary referrers, aggregate old events, and delete granular rows on an approved schedule. |
| `account_login_history` / security findings | Define a security-specific retention and access policy; include these records in account export/erasure rules where applicable without destroying active abuse evidence prematurely. |
| `server_health_metrics` | Keep raw samples only for the diagnostic window, retain downsampled aggregates for longer trend windows, and align status APIs with the policy. |
| `statistics` | Downsample before pruning if long-term population trends are needed. |
| Game-owned items, PvP payloads, and logs | Use the MUD data-lifecycle manifest and archive/reconciliation process; do not attach an independent web purge job. |

Every job needs bounded batches, an index supporting its cutoff, dry-run counts,
audit output, retry/idempotency behavior, replica/backup consideration, and a
test proving that active product queries still have the required window.

### P1-E: Add the `statistics(date)` index through the MUD owner

`SNAPSHOT`: `statistics` has 256,292 rows and only its primary key. `WEB CODE`
performs repeated epoch range filters and chronological sorts on `date`.
`LOCAL DB` `EXPLAIN` shows `ALL` plus `Using filesort`, although the empty local
table cannot quantify real latency.

Candidate: `INDEX idx_statistics_date (date)`, added through the MUD schema
owner, not an uncoordinated web migration. Validate it on a protected,
representative clone with the exact analytics/public-statistics queries.

The daily query using `DATE(FROM_UNIXTIME(date))` is non-sargable. If it remains
material, use a maintained daily rollup or a tested generated/functional key
appropriate to both supported database engines rather than assuming the plain
index solves that expression.

### P1-F: Publish generated reference data atomically

The dump shows that wiki data is large enough for a failed rebuild to be
operationally visible. The current publishers do not preserve the previous
generation on failure:

- `backend/scripts/import-wiki-data.ts` starts a transaction and then truncates
  eight live wiki tables. MySQL `TRUNCATE TABLE` implicitly commits, so the
  advertised rollback cannot restore those rows. The script then performs many
  single-row inserts.
- `backend/src/scripts/extractMapData.ts` deletes live map and entrance rows on
  pooled autocommit connections before repopulating them over many statements.
  A parse, packet, connection, or insert failure leaves an empty or partial map.
- `backend/src/scripts/syncBuilderFlags.ts` deletes every live flag and then
  inserts categories in separate statements, with the same partial-publication
  outcome on failure.

Required change:

1. Parse and validate source files before touching the published generation.
2. Load bounded batches into staging/versioned tables using one explicit
   connection. Keep foreign-key checks active and validate keys, expected count
   ranges, uniqueness, and a source revision/checksum.
3. Promote the complete generation atomically, using a tested shadow-table swap
   or a version pointer that readers resolve consistently. Retain the previous
   generation until post-promotion checks pass.
4. Serialize publishers with an advisory/job lock, expose generation status,
   and make retry cleanup idempotent.

Acceptance requires injected failures during every load phase to leave the old
generation fully queryable, plus a successful run whose counts and source
revision are visible to preflight. Benchmark load time, packet size, lock time,
and reader latency on both supported database families.

### P1-G: Normalize the `pkill_event.stamp` default through the MUD owner

`SNAPSHOT` and `LOCAL DB`: `stamp` is `DATETIME NOT NULL DEFAULT
'0000-00-00 00:00:00'`. The local server enables `NO_ZERO_DATE` and
`NO_ZERO_IN_DATE`, but the dump disables strict SQL mode while loading and
current MUD writes provide `NOW()` explicitly. No current DurisWeb insert that
omits `stamp` was found.

This is portability/schema debt, not a proven active production outage. A MUD
migration should first normalize any zero values and then choose an explicit
portable contract (`CURRENT_TIMESTAMP` or no default with mandatory writers).
Test it on both supported engine families.

### P2-A: Benchmark a layer-leading wiki map index

`SNAPSHOT`: `wiki_map_positions` has 206,232 rows and index `(x_coord,
y_coord)`. `WEB CODE` filters `z_coord = ?`, applies ranges to both X and Y, and
orders by Y then X.

Benchmark at least `(z_coord, y_coord, x_coord)` and `(z_coord, x_coord,
y_coord)` against real viewport shapes and `getMapBounds`. A B-tree can use the
leading equality and usually only one following range efficiently, so neither
candidate should be called a spatial solution without plan and latency data.
Consider map tiling/materialization if two-dimensional range scans remain the
dominant cost. This table is a web projection, so the winning index belongs in
the DurisWeb extension schema.

### P2-B: Create synthetic development fixtures, not a scrubbed SQL text file

The prior proposed sanitizer is unsafe and internally invalid:

- `accounts` has no numeric `id` for `CONCAT('user', id, ...)`;
- `spells` and `continent_data` are not tables in the snapshot;
- one shared password hash creates predictably accessible accounts;
- text replacement cannot safely transform SQL strings, binary blobs,
  encodings, or definers; and
- retaining real game/user rows creates continuing re-identification and access
  risk even after obvious fields are changed.

Use the canonical schema-only baseline, generate deterministic synthetic
accounts through normal application hashing, and create small auction, PvP,
analytics, and performance fixtures with no production lineage. Generate wiki
and map projections from checked-in MUD world/area sources using the existing
parsers, but do not run the current destructive publishers against a shared
database until P1-F makes promotion atomic.

If a sanitized production-derived clone is ever essential, restore it in an
isolated disposable database, transform it table-by-table with reviewed SQL,
exclude credentials/content/audit tables by default, run automated scans and
referential checks, and export only the approved result. Never commit that
result as the normal developer seed.

### P2-C: Stop `account_characters` surrogate-ID churn

`SNAPSHOT`: only 875 rows survived in `account_characters`, but its signed `INT`
auto-increment had reached 19,112,780. `MUD CODE` updates the projection with
`INSERT ... ON DUPLICATE KEY UPDATE` without supplying `id`; MySQL consumes an
auto-increment value on duplicate-key attempts as well as successful inserts.
This explains a plausible source of churn, although only live write-rate
evidence can estimate exhaustion time.

The MUD owner should measure `AUTO_INCREMENT`, attempted writes, and growth per
day, then remove allocation from the steady-state update path. Options include
an update-first/insert-if-absent operation with concurrency tests or a reviewed
key redesign around the stable player identity. Independently decide whether
the surrogate must be widened to `BIGINT UNSIGNED`; widening alone does not fix
the write amplification. Add a capacity alert before the signed `INT` ceiling.

Acceptance must cover concurrent character creation, rename/account movement,
soft-delete/tombstone restoration, duplicate PID/name repair, and both supported
database families. A repeated update of an existing mapping must not advance
the identity counter.

### P3: Re-evaluate small-table index candidates after measurement

The following are plausible growth safeguards, not snapshot-proven urgent
bottlenecks:

| Candidate | Snapshot evidence | Required validation |
| --- | --- | --- |
| `account_characters(pid)` or a workload-specific composite | 875 rows; several PID lookups/joins | Coordinate with the MUD contract, inspect current cardinality and plans, and include account/soft-delete predicates where useful. |
| `pkill_event(stamp)` | 187 rows; chronological and recent-window queries | Measure current live/clone cardinality and query frequency. Add through MUD ownership if justified. |
| `auctions(status, end_time)` | 249 rows; history and summary can filter/sort by these fields | First rewrite `UNIX_TIMESTAMP(end_time) > UNIX_TIMESTAMP()` as sargable `end_time > NOW()`. The default open-list sort is `id ASC`, so one composite does not cover every path. |
| `accounts(email)` | 329 rows; donation lookup uses email | Confirm lookup frequency, uniqueness semantics, and MUD ownership; current size does not establish urgency. |

The explicit `COLLATE utf8mb4_unicode_ci` in the frag join is also not proven to
force a scan. `LOCAL DB` `EXPLAIN` used `account_characters.idx_char_name_unique`
as an `eq_ref` lookup with index condition. Standardize the shared contract to
the cross-engine `utf8mb4_unicode_ci`, then remove workarounds only after plans
and semantics are verified. Do not standardize on MySQL-only
`utf8mb4_0900_ai_ci` when the deployment contract includes MariaDB.

## 5. Corrections to the first-pass report

| First-pass statement or action | Corrected conclusion |
| --- | --- |
| The dump proves current production failures. | It proves the 2026-06-23 schema/data snapshot. Live deployment is unknown. |
| Web auction rollback restores `player_data` while MyISAM writes remain. | The route commits `player_data` in `deductCharacterMoney()` before the separate `placeBid()` transaction. There is no implemented refund on failure. Historical MyISAM adds another problem, but is not the transaction boundary described. |
| Add a web migration to convert auction tables. | Current local tables are InnoDB and the MUD persistence contract owns and verifies that conversion. Route writes through the MUD transaction and verify deployment instead. |
| `pkill_event` zero default is itself a P0 insert outage. | The dump restores with strict mode disabled and current MUD inserts explicit timestamps. It is still P1 schema debt because the web currently clears pooled SQL mode to work around it; remove that runtime behavior immediately. |
| Local has 30 missing tables that all prove immediate backend failure. | Thirty snapshot tables are absent; 28 are named in current non-test code. Feature effects differ, and local is not proof of live production. |
| `/api/status` always returns 500. | Root status can degrade; history/incidents fail; uptime can incorrectly report 100; health writes are suppressed after logging. |
| A temporary clone proved 49 migrations and 593 tests pass. | No retained database or report currently supports that assertion. Re-run only after a canonical disposable baseline exists. |
| Add `migrate:latest` to `scripts/dev.sh`. | Unsafe until the shared baseline, ordering, ignored SQL artifacts, and postconditions are resolved. Existing onboarding warning is correct. |
| Create `players_core` in Knex. | Refactor to canonical `player_data` and MUD lookup tables or an explicit MUD-owned compatibility view. Also fix the missing `pd` join. |
| Standardize on `utf8mb4_0900_ai_ci`. | Use the cross-engine `utf8mb4_unicode_ci` contract already used locally and by current migrations. |
| Explicit `COLLATE` necessarily forces a full scan. | The local plan retained the indexed `eq_ref` lookup. Measure before changing it. |
| `terminal_logs` has 144,091 rows. | It has 21,742 rows; embedded SQL-like delimiters in terminal text broke the naive count. |
| High-volume tables include `account_characters`, `pkill_event`, and `auctions`. | Snapshot counts are 875, 187, and 249 respectively. Their indexes are lower priority without newer workload evidence. |
| Create a text-processing `sanitize-prod-dump.sh`. | Use schema-only canonical bootstrap plus synthetic fixtures. Raw-dump text rewriting is unsafe and the proposed table/column assumptions were wrong. |

## 6. Ordered implementation backlog

| ID | Priority | Deliverable | Owning area | Proof required before closure |
| --- | --- | --- | --- | --- |
| DB-01 | P0 | Gate direct web bid, buy-now, and auction removal; implement authenticated idempotent MUD auction commands | DurisWeb + MUD | Concurrency, retry, and fault-injection tests; ledger/custody/wallet reconciliation |
| DB-02 | P0 | Disable direct web item deletes; implement MUD reconciliation/quarantine workflow and BigInt-safe UIDs | DurisWeb + MUD | Edge-case matrix, audit records, current-owner verifier |
| DB-03 | P0 | Disable direct web player wipe and route reset through the MUD-owned manifest/epoch workflow | DurisWeb + MUD + operations | Dry-run, fence, retry/recovery, full reconciliation, and restore drill |
| DB-04 | P0 | Remove PvP session SQL-mode overrides and make interaction/counter updates contract-safe | DurisWeb + MUD | Pool-state failure matrix, counter reconciliation, mixed-engine feature gate |
| DB-05 | P0 | Restrict, inventory, expire, and securely dispose of raw dump copies | Operations/security | Owner, permissions, copy inventory, expiry, and deletion record |
| DB-06 | P0 | Gate and redesign full/selective restore with byte-safe archives, exact target/manifest checks, and a recovery drill | DurisWeb + MUD + operations/security | Empty-target point-in-time restore, canonical selective recovery, reconciliation, failure injection, and measured RPO/RTO |
| DB-07 | P1 | Publish versioned shared schema-only baseline and migration ownership manifest | DurisWeb + MUD + operations | Clean and upgrade CI on supported engines; exact postcondition checks |
| DB-08 | P1 | Classify all 14 ignored SQL artifacts and audit legacy cross-boundary TypeScript migrations | DurisWeb | Each SQL artifact is converted, superseded with evidence, or archived; every ownership exception has a forward-safe disposition |
| DB-09 | P1 | Refactor user management off `players_core` and fix alignment alias | DurisWeb | Canonical-schema integration tests for all filters/sorts and wipe |
| DB-10 | P1 | Add the MUD-write allowlist and expand deployment/startup feature-contract verification | DurisWeb + MUD | Classified direct writes plus negative tests for missing tables, wrong engine/type/version, and disabled features |
| DB-11 | P1 | Add and validate `statistics(date)` through MUD schema ownership | MUD + DurisWeb consumers | Representative plan/latency comparison and no write regression |
| DB-12 | P1 | Approve and implement telemetry/privacy lifecycle policy | Product + security + operations | Data inventory, approved windows, redaction tests, bounded purge/rollup runbook |
| DB-13 | P1 | Replace destructive wiki/map/flag refreshes with atomic generation publishing | DurisWeb | Failure injection preserves old data; validated source revision/counts; cross-engine load metrics |
| DB-14 | P1 | Normalize `pkill_event.stamp` and remove request-time SQL-mode overrides | MUD + DurisWeb | Zero-value audit, cross-engine migration tests, strict-mode interaction tests |
| DB-15 | P2 | Benchmark wiki layer index or map materialization | DurisWeb | Representative viewport plans, p95 latency, write/build cost |
| DB-16 | P2 | Add deterministic synthetic development fixtures | DurisWeb + MUD content tooling | No production lineage/PII; representative feature and performance tests |
| DB-17 | P2 | Remove `account_characters` auto-increment churn and set a capacity guard | MUD | Existing-row updates do not allocate IDs; concurrency/tombstone tests; live growth alert |
| DB-18 | P3 | Reassess smaller-table indexes | Correct table owner | Current cardinality, query frequency, plan, and measured benefit |

## 6.1 Implementation status

Recorded 2026-09-03 on branch `webdb001`. `Done` means the DurisWeb-side
deliverable is merged and verified by the checks named in the commit; it does
not close a ticket whose owner is the MUD repository or operations.

| ID | DurisWeb-side status | Remaining owner and work |
| --- | --- | --- |
| DB-01 | Removal cut over. `adminRemoveAuction()` now calls the MUD command `durisweb_auction_remove` and writes no auction table; that route is no longer gated. Bid and buy-now still return 503 unless `ALLOW_UNSAFE_AUCTION_WRITES` is set. Bid history reads and writes the integer epoch correctly, and preflight verifies the four InnoDB engines and the `date` column type. | MUD: the removal command ships (`ws_cmd_durisweb_auction_remove`, actor-less critical command, idempotent on a non-open auction). Bid and buy-now remain out of scope: they lock the bidder's live wallet through `expected_wallet_revision`, which only an online character carries, so a web bid needs an offline wallet custody contract that does not exist. Those two stay gated by decision, not by oversight. |
| DB-02 | Gated deletes. Detection now groups by global `obj_uid`, reports `vnum_count`, aggregates before the metadata join, and carries UIDs as decimal strings end to end. | Settled: the MUD's reconciliation path is the operator-run `migrations/reconcile_item_ownership.sh` over `item_current_owner`, `item_ownership_ledger`, and `item_ownership_quarantine`. The intended end state is web diagnoses, operator reconciles, so the delete gate stays closed permanently rather than waiting for a runtime command that should not exist. |
| DB-03 | Gated. `/api/admin/mud/wipe/execute` returns 503 unless `ALLOW_UNSAFE_PLAYER_WIPE` is set. The legacy `players_core` delete is removed. | Settled: the MUD already owns season reset through `sql_pwipe()` and the durable `season_reset_state` epoch and fence. Exposing a full player wipe over the web bridge is deliberately not built; the offline MUD maintenance tool is the authoritative path this document already allows. The web endpoint stays disabled. Remaining work is operational only: approval record and drill runbook. |
| DB-04 | Done. Request handlers no longer leak a relaxed session SQL mode to the pool; the value is captured, restored, and the connection discarded if restoration fails. Startup now checks out a connection from every configured pool and refuses to boot when its `sql_mode`, transaction isolation, time zone, or `foreign_key_checks` have already drifted from the server defaults. | The invariant probe runs once at startup, not as per-request checkout sampling. |
| DB-05 | Local artifact restricted to `0600` in a `0700` directory. | Operations: owner, copy inventory, expiry, and deletion record. |
| DB-06 | Gated. Both restore endpoints return 503 unless `ALLOW_UNSAFE_DATABASE_RESTORE` is set. The three filter columns that named nonexistent columns are corrected, and an archive missing its filter column is now a hard error rather than a silent omission. | Full redesign: archive manifest, byte-safe streaming, `spawn` with a defaults file, disposable-target drill, measured RPO/RTO. |
| DB-07 | Not started. Investigation confirmed the forum, session, and profile tables have no TypeScript creator at all, so Knex history is not replayable from empty. | DurisWeb + MUD + operations: versioned baseline and disposable-database CI. |
| DB-08 | Done. All 14 ignored SQL artifacts are classified in `backend/migrations/sql-artifacts.json`, and preflight fails on an unclassified or stale entry. | Conversion of the `baseline-pending-conversion` artifacts is DB-07 work. |
| DB-09 | Done. User management reads canonical `player_data` joined to `races` and `classes`; the alignment filter now has the `pd` alias it referenced. | — |
| DB-10 | Partial. Preflight gained the SQL-artifact gate and the auction engine/column contract. All 52 DurisWeb statements that write a MUD-owned table are classified in `backend/mud-write-allowlist.json` with their authoritative writer, concurrency contract, gate, and ticket; `pnpm verify:mud-writes` runs in CI and fails on an unclassified write or a stale entry. | The broader feature-contract verification, covering per-feature table, column, engine, index, and MUD schema-version postconditions, remains open. |
| DB-11 | Not applicable here. | MUD: done. Immutable migration `0008_statistics_date_index` adds `idx_statistics_date`, guarded and re-runnable, verified on MySQL 8 and MariaDB 10.11. Latency comparison on a representative clone is still open. |
| DB-12 | Not started. | Product + security + operations: approved retention and redaction policy. |
| DB-13 | Done. The wiki, map, and builder-flag publishers parse and stage a complete generation, refuse an empty one, and swap it in a single transaction with foreign-key checks enabled. `TRUNCATE`, which commits implicitly, is gone. | Cross-engine load benchmarking remains open. |
| DB-14 | Web half done (see DB-04). | MUD: done. Immutable migration `0007_pkill_event_stamp_contract` normalizes surviving zero stamps to the epoch and replaces the default with `CURRENT_TIMESTAMP`, verified on MySQL 8 and MariaDB 10.11. |
| DB-15 | Open-auction predicate rewritten as sargable `end_time > NOW()`. | Benchmarking the wiki layer index needs a representative clone. |
| DB-16 | Not started. | DurisWeb + MUD content tooling. |
| DB-17 | Not applicable here. | MUD: identity churn removed. `sql_update_account_character()` and `sql_save_account_characters()` resolve the mapping id and `UPDATE` in place, so only a genuinely new mapping allocates; `tests/async/run_account_character_identity_mysql.sh` proves five repeated projections consume no identity value. Widening the surrogate to `BIGINT UNSIGNED` and the capacity alert remain open. |
| DB-18 | Not started; requires measurement. | Correct table owner per candidate. |

Section 8.4's definition of done is not met. The unmet conditions are the
recovery drill, the versioned baseline and deterministic fixtures, the live
feature-contract preflight, the telemetry policy, the performance
measurements, and the dump ownership and expiry record.

## 7. Safe sequencing

1. Restrict the dump and gate unsafe restore, auction, item, reset, and PvP
   mutations.
2. Capture read-only live schema evidence and deployed revisions.
3. Agree on schema ownership, complete recovery scope, and the auction, item,
   season-reset, and selective-recovery command contracts.
4. Build and verify the schema-only baseline and archive manifest in disposable
   databases.
5. Implement the three MUD-authoritative mutation cutovers and the byte-safe,
   manifest-checked recovery paths.
6. Refactor legacy consumers, make generated-data promotion atomic, and expand
   preflight.
7. Benchmark indexes and retention jobs on a protected representative clone.
8. Rehearse full recovery, forward-only deployment, and
   rollback/feature-disable procedures.
9. Apply only the rehearsed changes during approved operational windows.

Never experiment on the shared/live database, rewrite a production migration
ledger ad hoc, or run a bulk purge/index/engine conversion without a tested
backup and recovery path.

## 8. Verification gates

### 8.1 Read-only live-state capture

An operator should record the deployed DurisWeb and MUD revisions and run
read-only queries for:

- database engine and version, SQL mode, time zone, and database name;
- `knex_migrations` names and MUD schema migration/baseline version;
- required feature tables and exact column/index definitions;
- table engine and collation for every shared mutation boundary;
- current row counts and `information_schema` size estimates;
- current `account_characters.AUTO_INCREMENT` and its measured growth rate;
- zero-date counts, open auction/custody reconciliation, and item ownership
  verifier results; and
- the exact backup source and restore target identities, archive manifest and
  consistency method, included table scope, and most recent successful drill.

Do not select or export sensitive row values for this gate. Mark each finding
`confirmed`, `not present`, or `changed since snapshot`; do not silently carry
snapshot conclusions forward.

### 8.2 Disposable database gates

- Canonical schema-only baseline boots from zero.
- Previous supported baseline upgrades to head in repository order.
- All migration postconditions pass even when optional features are gated.
- MUD runtime compatibility and persistence verifiers pass.
- DurisWeb backend integration tests run against the resulting schema.
- MySQL and deployed MariaDB-family behavior is covered where the contract says
  both are supported.
- A current archive restores into an empty target with point-in-time semantics;
  byte checks, table coverage, schema versions, row/checksum comparisons, and
  application smoke tests match its manifest.
- The old archive is rejected before writes on incompatibility, while selective
  recovery uses canonical schemas and leaves ownership, custody, balance,
  revision, and cache reconciliation clean.
- Invalid UTF-8 BLOB and oversized/archive-bomb fixtures prove byte preservation
  and bounded resource failure behavior.

### 8.3 Performance gates

For every index or query rewrite, retain the query, parameters/selectivity,
engine/version, row cardinality, plan before/after, warm and cold latency where
practical, write/storage cost, and rollback trigger. Local empty-table
`EXPLAIN` is structural evidence only, not a performance benchmark.

### 8.4 Definition of done

This initiative is complete only when:

- no web path directly mutates authoritative auction/item ownership state or
  executes its own MUD season reset;
- restore controls remain gated unless full recovery has point-in-time
  semantics and selective recovery is MUD-authoritative, with a current
  successful drill and measured RPO/RTO;
- pooled connections retain the required strict session invariants after every
  success and failure path;
- live preflight verifies the schema version and contracts for every enabled
  feature;
- a new developer and CI can create the same non-sensitive database from a
  versioned baseline and deterministic fixtures;
- no migration artifact is silently ignored;
- generated wiki/map refresh failures leave the prior generation available;
- telemetry has an approved classification, retention, redaction, and deletion
  path;
- performance changes are backed by representative measurements; and
- the historical dump and all known copies have an owner and enforced expiry.

## Appendix A: Historical MyISAM inventory

```text
artifact_bind
artifacts
artifacts_mortal
auction_bid_history
auction_item_pickups
auction_money_pickups
auctions
boons
boons_progress
boons_shop
ctf_data
epic_bonus
epic_gain
guild_transactions
guildhall_rooms
guildhalls
ip_info
locker_access
mud_info
multiplay_whitelist
nexus_stones
offline_messages
outposts
ping
pkill_event
pkill_info
players_core
poll_options
poll_votes
polls
progress
racewar_stat_mods
ship_cargo_market_mods
ship_cargo_prices
shop_trophy
```

The eight `latin1` table defaults in the snapshot are `locker_access`,
`mud_info`, `offline_messages`, `ping`, `pkill_event`, `progress`,
`ship_cargo_market_mods`, and `ship_cargo_prices`.

## Appendix B: SQL migration artifacts ignored by current Knex configuration

```text
005_create_accounts_table.sql
006_create_forum_tables.sql
007_add_dynamic_permissions.sql
008_create_moderation_log.sql
009_create_subscriptions.sql
010_update_notifications_schema.sql
011_create_user_profiles.sql
012_create_mentions.sql
013_create_forum_polls.sql
014_category_management.sql
015_add_ip_tracking.sql
017_fix_emoji_icons.sql
018_add_admin_action_log.sql
019_create_wipe_history.sql
```
