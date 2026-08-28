# Redis System Audit

Date: 2026-08-28
Branch: `redis-refactor`
Audit baseline commit: `68a916ec`
Status: Implementation in progress; RDS-002, RDS-003, RDS-006, RDS-007, RDS-010, RDS-012,
RDS-013, RDS-014, RDS-019, and RDS-028 are remediated and the remaining findings are
open.

## Implementation progress

### 2026-08-28 - RDS-011/RDS-021 local report cache and asynchronous publication

Completed in this interval:

- Replaced synchronous Redis reads for named reports, fraglists, epic-zone reports, and
  artifact lists with a 32-entry in-process cache. Existing TTLs use a monotonic local
  deadline, and callers retain their SQL/generation fallback on a local miss.
- Moved cache `SET`, `SETEX`, and `DEL` operations to a dedicated worker. The queue is
  bounded to 64 jobs, 4 MiB total payload, 1 MiB per value, and 128 bytes per key.
- Coalesced queued mutations by key without replacing the in-flight front job, so an
  outage retains the newest cache state without accumulating redundant report payloads.
- Added bounded worker-only timeouts, exponential reconnect backoff, three-attempt
  command-error handling, one-second shutdown drain, and pwipe cancel/join before checked
  deletion.
- Preserved restart-time artifact cache hits with one boot-only Lua read of all six
  artifact variants and their remaining TTLs. Persistent or expired legacy values are not
  seeded, and local TTL is capped at the existing 900-second contract.
- Added local queue, byte, coalescing, drop, failure, reconnect, and entry-count health to
  administrator status without a Redis query.

Performance effect:

- Player-facing named-report, fraglist, epic-zone, and artifact cache hits perform no
  socket I/O. They take a short mutex and one bounded memory copy.
- Report publication and transaction-triggered cache invalidation no longer wait on Redis.
  Repeated mutations for one key collapse to the newest queued value.
- The only warm-cache read is one batched boot operation before the game loop. Named,
  fraglist, and epic-zone caches are generated during the existing boot sequence.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_cache_store_live.py`: passed under ASan/UBSan, covering
  local reads before Redis startup, outage healing, coalescing, TTL behavior, immediate
  local invalidation, job/byte/value bounds, and cancellation.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_artifact_cache_codec.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.

Remaining related work:

- RDS-011 still includes optional world/floor preflight I/O and deliberately invoked
  administrator queries on the shared context.
- RDS-021 still needs stable-data rendering for the fraglist countdown and explicit TTLs
  or revisions for named and fraglist data.

### 2026-08-28 - RDS-009/RDS-011 asynchronous presence publication

Completed in this interval:

- Moved Redis presence `HSET`, `HDEL`, and login/logout publication off login, quit, and
  disconnect paths onto a dedicated worker with an owned Redis connection.
- Added a fixed 1,024-job queue. Submission performs only a bounded payload copy under a
  short mutex; a full queue rejects new noncritical work without waiting for Redis.
- Added worker-only bounded connection and command timeouts plus exponential reconnect
  backoff from 100 ms to 60 seconds. Jobs queued during an outage remain ordered and are
  delivered after Redis heals.
- Combined each state mutation and optional event into one Lua operation. A one-hour
  operation token makes a timeout retry idempotent, avoiding duplicate pub/sub events when
  the first result was ambiguous.
- Retry markers are written only after the state mutation succeeds. Three consecutive
  command errors drop the noncritical job so a permanent wrong-type or ACL failure cannot
  block every later presence update.
- Pwipe cancels and joins the worker before checked presence deletion. Normal shutdown
  drains it for at most one second, then discards remaining noncritical work.
- Added local worker state, queue depth/high-water, completion, drop, failure, and
  reconnect counters to administrator status without adding a Redis query.
- Added the presence hash, channel, and one-hour retry-token keyspace to the lifecycle
  manifest and its fail-closed required-store validation.

Performance effect:

- Login, logout, link-loss, and invisibility handling perform no Redis connection or
  command wait. They now do bounded JSON encoding and queue submission only.
- A visible session transition previously made two sequential synchronous Redis calls;
  it now makes one Lua call on the background worker. Redis outage reconnects never run on
  the simulation thread.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_presence_worker_live.py`: passed under ASan/UBSan,
  including submission before Redis startup, ordered outage healing, final hash state,
  permanent Redis command errors, cancellation races, shutdown drain, and queue
  saturation.
- `python3 tests/async/test_redis_presence_privacy.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_data_lifecycle_manifest.py`: passed.
- `python3 tests/async/test_boot_log_hygiene.py`: passed.

Remaining related work:

- RDS-009 still needs a per-entry expiry model compatible with the external presence
  consumer.
- RDS-011 still includes synchronous cache reads/writes, administrative queries, and
  world/floor preflight work on the shared context.

### 2026-08-28 - RDS-011 pipelined floor-delta flush

Completed in this interval:

- Changed floor-delta flushes from one blocking request/reply cycle per `HDEL` or `HSET`
  to hiredis command buffering followed by one ordered reply collection.
- Preserved the existing fail-closed contract: queued deltas are released only after
  every reply is received and has the expected integer type. Partial append, timeout,
  Redis error, allocation failure, or unexpected reply retains the full retry buffer.
- Kept the existing 1,024-add and 1,024-remove memory bounds and the world-capture gate.

Performance effect:

- A full flush now uses one pipelined network exchange instead of as many as 2,048
  sequential round trips. It sends the same Redis mutations and does not add any command.
- No Redis work was added to object movement. This interval reduces the worst existing
  flush latency, but RDS-011 remains partial until noncritical gameplay writes and
  reconnects are fully isolated from the simulation thread.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-001/RDS-007 epoch-scoped world recovery

Completed in this interval:

- Changed world generations, pointer metadata, writer leases, and the floor-delta hash to
  the `mud:season:<epoch>:` namespace. Readers use the active SQL epoch; the background
  publisher captures its epoch once when it acquires the boot lease.
- Preserved `(epoch, sequence)` identity across generation publication, validation,
  restore, previous-generation cleanup, floor handoff, status, and administrator clear
  paths.
- Added cross-epoch live Redis coverage. The old and new writers can overlap, but an old
  writer advances only its old pointer while the new epoch remains unchanged.
- Replaced multi-round-trip watched publication with one Lua compare-and-set that checks
  the exact writer token and expected prior pointer before atomically writing the
  generation, pointer metadata, floor handoff, and lease renewal.
- Made pwipe invalidation check the current captured-epoch floor delete, legacy floor and
  recovery keys, presence, all cache scans, and ship snapshot scans. Removed redundant
  unchecked cache deletion calls from that path.

Performance effect:

- Added no SQL query or network round trip to snapshot capture, publication, restore, or
  gameplay. Epoch reads are process-local and key formatting occurs only where the same
  Redis operation already existed.
- Publication remains on the background worker. The atomic script reduces its Redis
  round trips; floor batching retains the same bounds and command count.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_world_store_live.py`: passed, including overlapping
  old/new epoch writers.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_season_reset_fence.py`: passed.

Remaining related work:

- RDS-001 remains partial until presence, content caches, and every other active Redis
  store are epoch-scoped or explicitly proven cross-season safe.
- RDS-007 is complete: publication is scoped by durable season epoch and requires both
  the exclusive writer token and expected prior pointer in one atomic operation.

### 2026-08-28 - RDS-001 durable season reset boundary

Completed in this interval:

- Added immutable migration `0003_season_reset_state` with a singleton monotonic season
  epoch and explicit `active`/`resetting` status. The migration is additive,
  re-runnable, verified, lifecycle-inventoried, and sealed into the MySQL 8 and MariaDB
  10.11 runtime compatibility fingerprints.
- Boot now reads the singleton exactly once and refuses to start unless it contains one
  valid positive epoch in the `active` state. An interrupted destructive reset therefore
  cannot silently reopen gameplay.
- Pwipe now verifies a fresh bounded Redis administrative connection before its SQL
  boundary, locks the singleton, advances the epoch, and commits `resetting` before the
  first destructive season mutation.
- The reset records its irreversible boundary conservatively before attempting the first
  state mutation. Any failed or ambiguous result after that point keeps `_pwipe` set and
  forces process shutdown instead of clearing the flags and telling players the old
  season resumed.
- A successful reset changes the exact new epoch back to `active` only after Redis
  invalidation, SQL postconditions, and account reward policy processing have finished.

Performance effect:

- Added one indexed singleton read during database boot and no recurring query, pulse,
  player-save, or gameplay work.
- The transaction, Redis preflight connection, and completion update run only during an
  explicitly requested pwipe while persistence workers are already quiesced.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_season_reset_fence.py`: passed.
- `python3 tests/async/test_pwipe_quiescence.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_data_lifecycle_manifest.py`: passed.
- `python3 tests/async/test_immutable_migration_runner.py`: passed.
- `python3 tests/async/test_runtime_boot_compatibility.py`: passed.
- Runtime full-schema, migration-history, engine, collation, index, and column drift tests
  passed against both `mysql:8.0` and `mariadb:10.11`.

Remaining related work:

- RDS-001 remains partial until the SQL epoch is present in every Redis namespace and
  every pwipe deletion/postcondition is checked. Explicitly disabled Redis must be safe
  against old keys when it is enabled again for a later season.
- RDS-007 now has durable monotonic season identity, but world generation keys and the
  pointer transaction are not yet scoped to that identity.

### 2026-08-28 - RDS-002/RDS-003 fenced atomic world publication

Completed:

- Added a random single-writer token with a renewable 10-minute Redis lease. Lease
  acquisition occurs at boot, and game-loop snapshot requests fail fast if it was not
  acquired.
- Moved publication into a focused store adapter. Its atomic compare-and-set verifies the
  fence token and expected prior pointer, writes the generation, advances all pointer
  metadata, consumes the stable pre-capture floor hash, and renews the lease.
- Removed the later game-thread floor-hash delete and its acknowledgment state. Floor
  changes made during capture remain in bounded memory and are flushed only after the
  completed transaction, so they are not consumed with the captured generation.
- Added explicit pipeline cancellation that discards active/queued captures and
  completions, joins an in-flight publisher, and prevents new capture requests.
- Administrator and pwipe world clears now acquire/retain the writer fence, cancel and
  join before deletion, remove all generation keys, check the clear result, and keep
  publication quiesced until process shutdown.
- Added a real isolated Redis test covering lease exclusion, stale-writer rejection,
  atomic pointer/floor behavior on both sides of publication, prior-generation cleanup,
  and compare-and-delete fence release.

Performance effect:

- Removed the synchronous post-publication floor `DEL` from the simulation thread and
  removed a redundant floor flush attempt from the periodic event wrapper.
- Fence acquisition is boot-only. Publication validation and lease renewal execute as
  one Redis script on the existing background publisher, not on a game pulse.
- Normal capture remains bounded to 64 records or 2 ms per pulse. Failed boot fencing
  disables publication for that process instead of retrying connections from gameplay.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_world_store_live.py`: passed against an isolated local
  Redis server.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed.
- `python3 tests/async/test_boot_log_hygiene.py`: passed.
- `./scripts/format.sh --check`: passed.

Remaining related work:

- RDS-001 still needs the SQL season epoch and irreversible reset state machine. Redis
  disabled-at-reset behavior and postconditions remain unsafe until that lands.
- RDS-007 is partially remediated by the exclusive renewable lease and watched pointer
  transaction, but sequence identity is not yet scoped to a durable SQL season epoch.
- Staged UID reconciliation and duplicate rejection remain part of RDS-004/RDS-005.

### 2026-08-28 - RDS-028 legacy Redis API retirement

Completed:

- Removed the obsolete `mud:next_obj_uid` read and write from server boot and shutdown. The
  SQL range allocator remains the sole UID authority.
- Removed the retired key from administrator status output so stale values cannot be
  mistaken for authoritative allocator state.
- Removed the unused generic ping, publish, and string-read exports and their
  implementations. The already-retired floor lookup exports remain absent.
- Added a source contract that prevents the legacy key and APIs from returning.

Performance effect:

- Redis-enabled boot and shutdown each perform one fewer synchronous network command.
- Administrator status performs one fewer sequential Redis read.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_legacy_api_retirement.py`: passed.
- `python3 tests/async/test_item_ownership_contract.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-010 donation subscriber hardening

Completed:

- Disabled the donation subscriber and its periodic polling job by default. Enabling it
  requires exact `REDIS_DONATION_SUBSCRIBER=TRUE` plus an independent secret of at least 32
  bytes.
- Added a versioned HMAC-SHA256 event envelope with constant-time signature comparison,
  stable event IDs, a five-minute timestamp window, and a bounded replay-ID window.
- Required integer cents, positive bounded amounts, uppercase currency, bounded text, and
  public-donor names. Rejects control bytes and the in-game color prefix before display or
  logging.
- Limited processing to eight messages per game pulse and added exponential reconnect
  backoff capped at 60 seconds.
- Documented the exact producer signature contract and the remaining at-most-once pub/sub
  delivery semantics.

Performance effect:

- The default server no longer opens a donation subscriber connection or runs its
  once-per-second polling event.
- An explicitly enabled subscriber still uses a zero-timeout socket poll, now has a hard
  per-pulse message budget, and replaces outage reconnect attempts every second with
  exponential backoff.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_donation_security.py`: passed.
- `python3 tests/async/test_boot_log_hygiene.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_nevent_periodic_rearm_runtime.py`: passed under ASan/UBSan.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-013 unsafe corpse cleanup retirement

Completed:

- Replaced the obsolete cleanup implementation with a fail-closed retirement stub.
- The stub exits nonzero without loading credentials, connecting to MySQL or Redis, or
  changing either persistent store.
- Updated the operations runbook so the command is no longer presented as a usable cleanup
  procedure.
- Added behavioral coverage with probe database and cache clients to prove invocation has
  no storage side effects.

Performance effect:

- None in the running server. This change affects only an explicitly invoked offline
  operations script.

Validation:

- `shellcheck scripts/delete_corpses.sh`: passed.
- `bash -n scripts/delete_corpses.sh`: passed.
- `python3 tests/async/test_delete_corpses_retired.py`: passed.

### 2026-08-28 - RDS-009 presence privacy and encoding (partial)

Completed:

- Centralized exact `DURISWEB_PRIVATE_PRESENCE=TRUE` policy and invisible-staff filtering
  for both WebSocket and Redis presence transports.
- Default Redis presence payloads now omit account name, IP address, client name, and client
  version. Private fields and invisible staff require the explicit opt-in.
- Replaced `snprintf` JSON assembly with cJSON encoding so quotes, backslashes, and control
  characters in client-provided metadata cannot corrupt the stored payload.
- Removes any prior hash entry when a now-invisible character logs in and suppresses its
  login/logout publication.
- Added a compiled payload harness for public/private field selection, exact opt-in parsing,
  and JSON escaping, plus updated the DurisWeb security contract and documentation.

Performance effect:

- Normal visible login/logout keeps the existing bounded Redis command count. Invisible
  staff skip publication, and JSON encoding performs only small in-memory work at session
  boundaries rather than on the game pulse hot path.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_presence_privacy.py`: passed.
- `python3 tests/async/test_durisweb_integration_security.py`: passed.
- `python3 tests/async/test_durisweb_auth_runtime.py`: passed.
- `python3 tests/async/test_durisweb_secret_config.py`: passed.
- `python3 tests/async/test_websocket_protocol_contract.py`: passed.
- `python3 tests/async/test_runtime_connection_trust.py`: passed.
- `./scripts/format.sh --check`: passed.

Remaining RDS-009 work:

- Replace the persistent `mud:online` hash entry model with per-session leases or another
  per-entry expiry contract so a failed logout cannot leave stale presence indefinitely.

### 2026-08-28 - RDS-014 artifact cache safety

Completed:

- Added a versioned artifact-cache schema and a typed codec that validates view identity,
  item count, every required field, integer ranges, booleans, and bounded strings before
  rendering.
- Invalidates one malformed cache variant and falls back to a freshly generated MySQL
  result instead of reporting false emptiness or dereferencing malformed JSON.
- Renders the owned MySQL result directly on a cache miss and makes cache publication best
  effort, removing the prior second Redis read.
- Distinguishes SQL/generation failure from a valid empty artifact list.
- Added allocation checks to artifact JSON generation and a 900-second cache TTL.
- Added a compiled cJSON harness covering valid mortal/staff payloads, mismatched schema
  identity, missing fields, invalid types, invalid ranges, and malformed JSON.

Performance effect:

- Cache hits still require one bounded Redis read. Cache misses no longer re-read Redis after
  publication; the SQL result already in memory is rendered directly.
- The 900-second TTL creates an occasional SQL refresh in exchange for bounded stale-data
  lifetime.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_artifact_cache_codec.py`: passed.
- `python3 tests/async/test_artifact_guild_transactional_cutover.py`: passed.
- `python3 tests/async/test_combat_artifact_persistence.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-019 floor-delta world-recovery gate

Completed:

- Gated floor-drop capture, removal, flush, and restore on `REDIS_WORLD_STATE` instead of
  general Redis configuration.
- Removed the unused floor-pickup writer and lookup APIs, eliminating a synchronous Redis
  `SADD` from every tracked floor pickup.
- Removed the unused floor-drop lookup API and stopped periodic player checkpoint work from
  attempting floor flushes when world recovery is disabled.
- Retained explicit cleanup of the legacy pickup set and floor hash for pwipe and operator
  cleanup.
- Added a focused contract that rejects floor-delta Redis work outside world recovery.

Performance effect:

- With the default `REDIS_WORLD_STATE=FALSE`, floor gameplay no longer performs floor-delta
  Redis commands, snapshot-string allocation/copying, or retry-buffer maintenance.
- World recovery enabled behavior retains its existing bounded batching and ACK rules.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-012 database backup integrity

Completed:

- Removed the Redis-dependent and legacy-flat-file mode selection. The backup script now
  always dumps the authoritative MySQL database.
- Required an explicit allow-listed database identity, safe local socket or TLS transport,
  owner-only environment file, and owner-only backup directory.
- Added `set -euo pipefail`, `MYSQL_PWD` credential handling, owner-only temporary output,
  gzip validation, required Duris schema markers, file and directory sync, and atomic rename.
- Made dump, compression, content validation, and publication failures exit nonzero without
  publishing a backup or retaining a temporary file.
- Made `cycle_mud.sh` stop before restart when its required database backup fails.
- Updated the runbook and added stubbed behavioral coverage for success, dump failure,
  malformed dump, compression failure, cleanup, and cycle failure propagation.

Validation:

- `shellcheck scripts/backup_pfiles.sh scripts/cycle_mud.sh`: passed.
- `bash -n scripts/backup_pfiles.sh scripts/cycle_mud.sh`: passed.
- `python3 tests/async/test_backup_pfiles.py`: passed.
- `python3 tests/async/test_cycle_pwipe_status.py`: passed.
- `python3 tests/async/test_start_mud_worktree_safety.py`: passed.

### 2026-08-28 - RDS-006 MySQL ship authority

Completed:

- Removed the Redis ship snapshot loader and publisher APIs and their JSON codec.
- Made `sql_load_ship()` always query MySQL and its dependent ship tables.
- Stopped publishing ship state from `sql_save_ship()`, including saves that participate in
  an outer transaction.
- Retained deletion of legacy `ship:snapshot:*` keys during ship deletion, owner rename,
  pwipe, and explicit namespace cleanup so stale snapshots cannot become active again.
- Updated focused source-contract tests to reject any Redis access in ship load/save paths.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_ship_snapshot_invalidation.py`: passed.
- `python3 tests/async/test_ship_save_guards.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_nevent_ship_volley_runtime.py`: passed.
- `python3 tests/async/test_ship_nested_transaction.py`: passed.
- `python3 tests/async/test_ship_shutdown_txn.py`: passed.

Remaining work:

- All findings other than RDS-002, RDS-003, RDS-006, RDS-010, RDS-012, RDS-013, RDS-014,
  RDS-019, and RDS-028 remain open. The acceptance criteria are not yet met.

## Executive summary

The Redis implementation has several good containment mechanisms: player saves do not
depend on Redis, network calls have bounded timeouts, world capture is sliced on the game
thread, the publisher receives owned bytes instead of live pointers, and recovery blobs
have framing, age, sequence, count, and CRC checks. Those controls are valuable and should
be preserved.

The system is not yet safe to treat as a reliable recovery layer or authoritative
read-through cache. This audit found 4 critical, 10 high, 12 medium, and 2 low severity
issues. The most important risks are:

1. A season reset can report Redis invalidation success without contacting Redis, or fail
   after SQL has already been destructively changed and then resume the old process.
2. An in-flight world publisher can republish pre-wipe state after the wipe or an operator
   clear.
3. A crash between world-generation publication and floor-delta deletion can restore the
   same floor object twice.
4. Container children and NPC-held items are restored without complete SQL custody
   validation. Floor-delta children are recreated with new UIDs, which can turn stale
   snapshots into newly authoritative items.
5. Ship snapshots are trusted before SQL and can be stale, uncommitted, malformed, or from
   another environment.

The safest short-term posture is to keep `REDIS_WORLD_STATE=FALSE`, avoid relying on the
current Redis-assisted pwipe path, disable the ship snapshot read path, and fix the backup
script before considering Redis recovery production-ready. The long-term fix is a small,
versioned Redis adapter with a season/environment namespace, authenticated connections,
writer fencing, typed codecs, and an explicit distinction between configured and healthy.

## Scope and method

The review traced runtime and shutdown call paths through:

- [`src/redis.c`](../../src/redis.c), [`src/redis.h`](../../src/redis.h), and
  [`src/wizredis.c`](../../src/wizredis.c)
- [`src/world_recovery_pipeline.c`](../../src/world_recovery_pipeline.c),
  [`src/copyover.c`](../../src/copyover.c), boot/event/shutdown code, and SQL item custody
- ship, artifact, fraglist, presence, and donation consumers
- `.env.example`, operational documentation, lifecycle policy, migrations, and scripts
- focused Redis, recovery, pwipe, ship, lifecycle, and documentation tests

Validation performed:

- `make -C src -j2`: passed with the repository warning-as-error profile.
- 11 focused Python test programs: all passed.
- `world_recovery_validate` runtime harness: passed its structural cases.
- `__NO_MYSQL__` syntax probe: failed, including unguarded MySQL and hiredis types.
- ShellCheck on Redis-facing operational scripts: failed and exposed the backup pipeline
  issue described below.

No game server was started. No configured MySQL or Redis service was contacted. No
migration, wipe, backup, clear, or corpse-cleanup script was executed.

## Severity rubric

| Severity | Meaning in this report |
| --- | --- |
| Critical | A reachable path can cause cross-season resurrection, durable duplication, destructive partial reset, or equivalent integrity failure. |
| High | A credible path can corrupt authoritative behavior, expose private data, crash the server, or cause sustained availability failure. |
| Medium | Material correctness, performance, operability, lifecycle, or maintainability weakness without the same immediate blast radius. |
| Low | Confirmed cleanup, build-option, documentation, or dead-code issue with limited direct runtime impact. |

## System map

| Area | Redis keys/channels | Intended role | Current authority behavior |
| --- | --- | --- | --- |
| World recovery | `mud:season:<epoch>:world_state:generation:<seq>`, current pointer, diagnostic metadata | Optional recent world reconstruction | Redis payload directly materializes mobs, objects, doors, zones, gold, affects, and equipment after structural validation. |
| Floor deltas | `mud:season:<epoch>:floor_drops`; retired `mud:floor_pickups` cleanup | Bridge changes around a world snapshot | Root SQL ownership is checked, but descendant identity/state is incomplete. |
| Ship cache | `ship:snapshot:<owner>` | Reconstructible SQL cache | Cache is returned before SQL without TTL, row revision, or schema/environment identity. |
| Content caches | `mud:cache:named`, `mud:cache:fraglist`, `mud:cache:epic_zones`, artifact variants | Reconstructible command output | Player reads use bounded local memory and Redis publication is asynchronous; named/fraglist freshness remains incomplete. |
| Presence | `mud:online`, `mud:player`, one-hour `mud:presence_op:*` retry tokens | Web presence and login/logout events | Privacy-safe payloads are published by a bounded worker; hash entries still lack per-entry expiry. |
| Donation integration | `mud:nchat` pub/sub | Broadcast external donation notices | Any publisher with channel access can generate an in-game and log message. |
| Legacy UID | `mud:next_obj_uid` | Retired counter | SQL allocator is authoritative, but Redis still reads, writes, and displays the legacy key. |

All runtime connections use only `REDIS_HOST` and `REDIS_PORT`; keys are fixed in Redis
database 0 and have no application, environment, deployment, or season prefix.

## Finding index

| ID | Severity | Finding |
| --- | --- | --- |
| RDS-001 | Critical | Season reset invalidation is non-atomic and has unsafe success and failure semantics. |
| RDS-002 | Critical | In-flight writers can republish state after a wipe or operator clear. |
| RDS-003 | Critical | World publication and floor-delta cleanup have a deterministic duplication window. |
| RDS-004 | Critical | Descendant item restoration violates complete SQL custody and identity rules. |
| RDS-005 | High | Recovery mutates the live world without staging, rollback, or exact success accounting. |
| RDS-006 | High | Ship snapshots can override MySQL with stale, uncommitted, or unsafe data. |
| RDS-007 | High | World sequence publication has no compare-and-swap, lease, or instance fencing. |
| RDS-008 | High | Redis lacks authentication, TLS, database selection, and namespace isolation. |
| RDS-009 | High | Redis presence bypasses the documented privacy policy and emits unsafe JSON. |
| RDS-010 | High | Donation pub/sub lacks authenticity, input limits, work budgets, and reconnect backoff. |
| RDS-011 | High | Synchronous calls run on the simulation thread and generic Redis outages are sticky. |
| RDS-012 | High | Automatic database backup selection and success detection are unreliable. |
| RDS-013 | High | The corpse cleanup script targets a retired world format and can over-delete floor data. |
| RDS-014 | High | Artifact cache failure can report false emptiness, while malformed cache data can crash. |
| RDS-015 | Medium | Existing tests are dominated by source-contract assertions rather than live Redis behavior. |
| RDS-016 | Medium | World recovery uses native C/C++ struct layout as its durable wire format. |
| RDS-017 | Medium | Read and publication memory bounds do not match the documented 64 MiB generation bound. |
| RDS-018 | Medium | Incremental world capture is a fuzzy, cross-time snapshot. |
| RDS-019 | Medium | Floor tracking remains active when world recovery is disabled and includes unused, unbounded data. |
| RDS-020 | Medium | Administrator status and clear commands can misreport success, omit stores, and stall the game. |
| RDS-021 | Medium | Content caches lack a coherent version, freshness, and invalidation contract. |
| RDS-022 | Medium | The lifecycle inventory omits most Redis stores and cannot discover that omission. |
| RDS-023 | Medium | Migration and clear scripts use broad `FLUSHDB` semantics without full connection safety. |
| RDS-024 | Medium | Fresh snapshots survive graceful shutdown and trigger a path documented as crash-only. |
| RDS-025 | Medium | Redis health and failure observability are too coarse for incident diagnosis. |
| RDS-026 | Medium | Redis responsibilities are concentrated in a monolith with unrelated persistence coupling. |
| RDS-027 | Low | The advertised no-MySQL build toggle is broken and hiredis remains a mandatory build dependency. |
| RDS-028 | Low | Retired keys and unused public APIs remain in the operational surface. |

## Detailed findings

### RDS-001 - Season reset invalidation is non-atomic and unsafe

Severity: Critical
Confidence: Confirmed from reachable code paths
Remediation status: Partially remediated; enabled-but-unavailable Redis now fails pwipe
invalidation closed, world writers are quiesced before checked deletion, and a durable
SQL epoch/reset fence prevents boot or live resumption after an interrupted destructive
boundary. World/floor keys and pwipe deletions are epoch-aware and checked; other active
Redis namespaces still need epoch isolation.

Evidence:

- [`redis_clear_pwipe_state()`](../../src/redis.c#L443) returns success immediately when
  Redis is configured off. Its direct world, floor, online, and cache deletes are `void`
  and their outcomes are ignored.
- [`redis_clear_scan_match()`](../../src/redis.c#L258) returns success when the main
  context is null. [`redis_clear_ship_snapshots()`](../../src/redis.c#L1734) does the
  same. An enabled Redis integration whose initial connection failed can therefore pass
  pwipe invalidation without deleting a key.
- Redis invalidation runs only after the destructive SQL sequence
  ([`src/sql.c`](../../src/sql.c#L4417)). If it returns false, `sql_pwipe()` returns false
  after many earlier mutations. The caller then clears `_pwipe`, cancels shutdown, and
  tells players that the wipe was aborted ([`src/actwiz.c`](../../src/actwiz.c#L4489)).

Impact:

- Old world generations, ship snapshots, presence, or caches can survive a successful
  season reset and reappear when Redis is later available or re-enabled.
- A real Redis failure late in reset can resume a live process against a partially or
  almost completely wiped SQL database. This is not a rollback.

Recommendation:

Implement pwipe as an irreversible, fenced state machine. Create a monotonic season epoch
in SQL, stop every Redis writer, verify a fresh administrative Redis connection before
the destructive boundary, and put the epoch in every key. Once SQL mutation begins, a
failure must leave the server fenced and stopped for recovery; it must never resume the
old live season. Make every invalidation return a checked result and verify postconditions
against the exact epoch.

### RDS-002 - In-flight writers can republish cleared state

Severity: Critical
Confidence: Confirmed
Remediation status: Completed on branch; destructive world clears cancel and join the
publisher before deletion and retain an exclusive fence until shutdown, while every
publication transaction rejects a stale writer token.

Evidence:

- Pwipe deliberately skips the normal 3-second world-recovery drain
  ([`src/comm.c`](../../src/comm.c#L1841)).
- After `sql_pwipe()` clears Redis, normal process cleanup still calls
  [`redis_cleanup()`](../../src/redis.c#L464), which drains an initialized world pipeline
  for up to 30 seconds and lets the publisher finish.
- The publisher opens its own connection and does not check `_pwipe`
  ([`src/redis.c`](../../src/redis.c#L108)). It can therefore publish a capture created
  before the wipe after Redis was cleared.
- `redis clear world` and `redis clear all` also delete keys without quiescing or fencing
  the active publisher ([`src/wizredis.c`](../../src/wizredis.c#L269)).

Impact:

Pre-wipe world state can be recreated after a successful clear. Operator clears are also
not durable: a queued generation can reappear seconds later.

Recommendation:

Add explicit `quiesce`, `cancel`, and `join` lifecycle operations. A destructive clear
must prevent new captures, discard queued pre-boundary generations, join the publisher,
and then clear. The Redis publication transaction must also compare a captured season
epoch and writer-fence token so that a stale worker cannot publish even if process-side
coordination fails.

### RDS-003 - Snapshot and floor-delta handoff can duplicate objects

Severity: Critical
Confidence: Confirmed failure window
Remediation status: Completed on branch for the deterministic handoff window; generation
publication and consumption of the stable pre-capture floor hash are now one watched
transaction, verified on both success and rejected-publication paths against live Redis.

Evidence:

- Floor deltas are flushed before capture. After the generation is published and its
  completion is consumed, the main thread separately deletes `mud:floor_drops`
  ([`src/redis.c`](../../src/redis.c#L1134)). The delete is not part of the pointer
  `MULTI`/`EXEC` transaction.
- A crash after pointer publication but before that delete leaves the same pre-boundary
  floor objects in both the current generation and the floor hash.
- Boot restores the floor hash before it fetches or validates the current generation
  ([`src/redis.c`](../../src/redis.c#L1284)), then restores the world generation.
- Both paths accept the same root UID when SQL still says it belongs to that room. There
  is no in-memory UID deduplication between the two restores.
- The comment at [`src/redis.c`](../../src/redis.c#L1012) says the world restore needs the
  floor hash, but the world restore no longer consults it. The current regression test
  explicitly asserts that the old Redis floor checks are absent.

Impact:

One narrow but ordinary crash timing can instantiate two roots with the same UID, plus
duplicated contents. Duplicate live identities undermine later ownership, movement, save,
and extraction logic.

Recommendation:

Use generation-scoped delta keys and atomically consume the pre-capture delta generation
in the same transaction that advances `world_state:current`. Independently, build an
in-memory set of planned UIDs during staged restore and reject duplicate roots or
descendants before materialization. Add kill-point integration tests immediately before
and after pointer `EXEC` and delta deletion.

### RDS-004 - Descendant restoration violates custody and identity

Severity: Critical
Confidence: Confirmed

Evidence:

- Floor-delta JSON stores at most 16 immediate child VNUMs, without child UIDs, state,
  affects, ownership, or nesting ([`src/redis.c`](../../src/redis.c#L612)). Restore calls
  `read_object()` for each VNUM, assigning a new SQL-reserved UID
  ([`src/redis.c`](../../src/redis.c#L983)).
- World records preserve immediate child UIDs, but
  [`world_recovery_restore()`](../../src/world_recovery_pipeline.c#L588) validates only
  the top-level root against SQL. `copyover_restore_obj_from_buffer()` then creates every
  child and applies its saved UID without checking owner, parent, state, or VNUM against
  SQL ([`src/copyover.c`](../../src/copyover.c#L1784)).
- The root SQL check hydrates only that one item. Normal movement requires every descendant
  to have matching runtime custody, root, parent, VNUM, and active state
  ([`src/item_movement_transaction.c`](../../src/item_movement_transaction.c#L62)).
- NPC equipment is serialized by VNUM only. Carried items include a UID, but restore
  ignores it and creates a fresh item ([`src/copyover.c`](../../src/copyover.c#L1735)).

Impact:

- A child moved away after capture can be recreated inside its old container.
- A floor-delta child receives a new UID and can be adopted as a new authoritative item,
  duplicating the value represented by the stale snapshot.
- A world-snapshot child with an old UID may collide with SQL or remain unusable because
  its runtime custody was never hydrated.
- NPC inventory and equipment can be duplicated or lose identity entirely.

Recommendation:

Serialize a complete, bounded item tree with fixed-width UID, VNUM, parent UID, root UID,
state, and required mutable fields. Batch-read every referenced UID from SQL and validate
the complete graph before creating any object. Hydrate the full verified graph atomically.
Define an explicit policy for non-authoritative NPC equipment; do not recreate valuable
items by VNUM alone.

### RDS-005 - Restore is not transactional or exactly accounted

Severity: High
Confidence: Confirmed

Evidence:

- Structural validation runs first, but restore then mutates the live world record by
  record. A later invalid door or zone returns false without removing mobs or objects
  already created ([`src/world_recovery_pipeline.c`](../../src/world_recovery_pipeline.c#L588)).
- The return values from mob and object restore helpers are ignored. Missing VNUMs or
  rooms can consume the full record, count as restored, and still produce overall success.
- Floor deltas are materialized before the generation pointer or blob is even fetched
  ([`src/redis.c`](../../src/redis.c#L1298)).
- Boot skips initial zone resets after the earlier `redis_has_world_state()` check. If
  restore later fails, it logs that normal boot state will be used, but the initial reset
  already did not run and partial recovery objects are not rolled back
  ([`src/comm.c`](../../src/comm.c#L951),
  [`src/new_events.c`](../../src/new_events.c#L1917)).

Impact:

A structurally valid but semantically unusable generation can yield a partial or mixed
world. The snapshot is then cleared, reducing the evidence available for diagnosis or a
controlled retry.

Recommendation:

Split recovery into `decode -> semantic plan -> authority reconciliation -> materialize`.
The planning phases must not mutate the world. Require exact planned/applied/skipped
counts, and treat an unexpected skip as failure. Materialize through a rollback-capable
arena or explicitly track and remove every created entity before falling back to a full
normal zone boot.

### RDS-006 - Ship snapshots override SQL with unsafe state

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; MySQL is now the only ship read authority.

Evidence:

- [`sql_load_ship()`](../../src/sql_player.c#L10429) returns a Redis snapshot before
  querying the SQL row. The key has no TTL, schema version, SQL revision, database
  identity, or season identity.
- Cache write and invalidation outcomes are ignored. A Redis outage during save leaves an
  older key that can become readable again later. SQL delete similarly returns success
  even if its cache invalidation did nothing ([`src/sql_player.c`](../../src/sql_player.c#L10551)).
- `sql_save_ship()` joins an outer transaction but always writes the cache before that
  outer owner commits ([`src/sql_player.c`](../../src/sql_player.c#L10196)).
  `shutdown_ships()` wraps all ships in such a transaction
  ([`src/ships/ship_base.c`](../../src/ships/ship_base.c#L210)). A later rollback leaves
  uncommitted Redis snapshots.
- Deserialization checks only that `m_class >= 0`. `new_ship(m_class)` indexes
  `ship_type_data[m_class]`, whose length is `MAXSHIPCLASS`, without an upper-bound check
  ([`src/redis.c`](../../src/redis.c#L1694),
  [`src/ships/ship_base.c`](../../src/ships/ship_base.c#L281)). Other numeric fields are
  also accepted without domain bounds.

Impact:

Redis can resurrect stale ship state, expose state that rolled back in SQL, or cause an
out-of-bounds read and server crash from malformed cache data.

Recommendation:

Disable the Redis ship read path until the cache is revisioned. At minimum, query SQL for
existence and row revision, require an exact revision match, validate every domain field,
and use a versioned codec. Publish only after the owning transaction commits, preferably
through an outbox. Add a short TTL and treat every cache error as a miss, never as
authority.

### RDS-007 - Publication has no fencing or compare-and-swap

Severity: High
Confidence: Confirmed; multi-instance impact is conditional on concurrent writers
Remediation status: Completed; a renewable exclusive writer lease and one atomic Lua
compare-and-set require the exact writer token and expected prior pointer. All world
recovery identities and keys use the durable SQL season epoch, so a stale process cannot
publish into the active season.

Evidence:

- Sequences are process-local. The pipeline reads `world_state:current` only when it is
  initialized and the main context already exists
  ([`src/redis.c`](../../src/redis.c#L202)).
- `redis_save_world_state()` initializes the pipeline before it reconnects the main
  context ([`src/redis.c`](../../src/redis.c#L1108)). If Redis was down at initialization
  and returns afterward, sequence numbering can restart at 1 despite durable generations.
- The publisher reads the prior pointer, writes its generation, and unconditionally sets
  the new pointer. It does not use `WATCH`, Lua compare-and-set, a lease, or a writer token
  ([`src/redis.c`](../../src/redis.c#L139)).

Impact:

A late recovery from startup outage can overwrite or rewind sequence state. Two server
instances, a stale process, or overlapping deployment can race, overwrite the same
generation key, move the pointer backward, and delete a generation another writer needs.

Recommendation:

Allocate sequences in Redis or SQL under the current season epoch. Advance the pointer
with one Lua script that requires the expected epoch, prior sequence, and writer-fence
token. Give each generation a globally unique writer/sequence identity. Explicitly
support one writer or enforce that invariant with a renewable lease.

### RDS-008 - Redis has no connection or namespace security

Severity: High
Confidence: Confirmed configuration gap; exploitability depends on deployment reachability

Evidence:

- Runtime connections support host and port only
  ([`src/redis.c`](../../src/redis.c#L91)). There is no ACL username, password, TLS,
  certificate verification, Redis database selection, URI, or Unix socket setting.
- Scripts use the same host/port-only model. Every key is fixed to database 0 with no
  environment or season namespace.
- Recovery integrity is CRC32, which detects accidental corruption but does not
  authenticate the writer ([`src/world_recovery_pipeline.c`](../../src/world_recovery_pipeline.c#L183)).
- A writer can influence mob stats, gold, affects, items, ships, artifact JSON, presence,
  and donation messages. Several consumers do not perform semantic validation.

Impact:

Any process or person with Redis write access can spoof donation notices, disclose online
PII, crash cache consumers, or inject a checksum-valid recovery payload. A shared Redis
can also cross-contaminate development, test, and production.

Recommendation:

Support ACL username/password, verified TLS, explicit database, Unix sockets, and an
application/environment/season prefix. Refuse a non-loopback production endpoint without
TLS. Apply least-privilege ACLs per subsystem and channel. Sign recovery generations and
external donation envelopes with independent keys so Redis access alone is insufficient.

### RDS-009 - Presence bypasses the documented privacy policy

Severity: High
Confidence: Confirmed
Remediation status: Privacy policy and JSON safety are completed on branch. Per-entry
presence expiry remains open.

Evidence:

- [`redis_player_online()`](../../src/redis.c#L2327) always writes character name,
  account, IP, client name, client version, and login time. It does not inspect
  `DURISWEB_PRIVATE_PRESENCE` and does not exclude invisible staff.
- The WebSocket implementation does enforce both controls
  ([`src/ws_handlers.c`](../../src/ws_handlers.c#L389)), and the configuration reference
  promises that the default feed omits these values
  ([`docs/operations/CONFIGURATION.md`](../operations/CONFIGURATION.md#L125)).
- Redis JSON is assembled with `snprintf` rather than cJSON. Client name and version come
  from client-supplied GMCP strings and may contain quotes, backslashes, or control
  characters ([`src/gmcp.c`](../../src/gmcp.c#L87)).
- `mud:online` has no per-entry TTL. Failed logout or boot cleanup leaves stale private
  data.

Impact:

The Redis feed discloses data the documented opt-in says is private and can reveal staff
who intentionally used invisibility. Client-controlled text can also make the stored JSON
invalid.

Recommendation:

Use the same central presence-policy helper for WebSocket and Redis. Build JSON with
cJSON, omit private fields unless exact opt-in is active, and skip invisible staff by
default. Use expiring per-session keys or a heartbeat/lease rather than a persistent hash.

### RDS-010 - Donation pub/sub is unauthenticated and unbounded

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; the subscriber is explicitly gated and requires
authenticated, bounded, fresh, replay-protected events, with reconnect backoff and a hard
per-pulse work budget.

Evidence:

- Donation polling is enabled whenever general Redis is enabled and runs every second
  ([`src/new_events.c`](../../src/new_events.c#L1994)). There is no separate feature flag.
- On disconnection, every poll immediately retries a bounded 250 ms connection with no
  exponential backoff or jitter ([`src/redis.c`](../../src/redis.c#L1959)).
- The payload needs only `type=donation`. Amount, currency, character, public flag, and
  message have no required-field, finite-number, sign, size, character-set, or replay
  validation ([`src/redis.c`](../../src/redis.c#L1890)).
- Text is placed directly into colorized game output and logs. Newlines and in-game color
  controls are not escaped. All replies already buffered by hiredis are drained in an
  unbounded loop.

Impact:

A Redis publisher can impersonate donors, inject misleading in-game/log text, and send a
burst that monopolizes a game pulse. An outage can impose a 250 ms hitch every second.

Recommendation:

Add an explicit donation feature flag, signed and replay-protected message envelope,
bounded strings and amount/currency validation, display escaping, per-pulse message/time
budgets, and exponential reconnect backoff. If these messages represent payment events,
use a durable stream with stable event IDs rather than at-most-once pub/sub.

### RDS-011 - Simulation-thread I/O and sticky failures

Severity: High
Confidence: Confirmed
Remediation status: Partially remediated; presence writes and reconnects run on a bounded
healing worker, and floor-delta batches use one pipelined exchange instead of up to 2,048
sequential round trips. Report cache reads, writes, invalidations, and reconnects are also
off the simulation thread. Administrator queries and optional world/floor preflight work
on the shared connection remain open.

Evidence:

- The main hiredis context is synchronous with a 100 ms command timeout. Floor pickups,
  logins, cache reads/writes/invalidations, presence, and administrator commands call it
  from game-thread paths.
- A player login performs a hash write and publish. Each floor pickup performs an
  immediate `SADD` even when world recovery is disabled
  ([`src/redis.c`](../../src/redis.c#L547)). Detailed status performs many sequential
  calls.
- Generic cache, presence, and helper methods return immediately when the context is null
  or errored and never reconnect. Only world-recovery paths invoke `redis_reconnect()`.
  With the default `REDIS_WORLD_STATE=FALSE`, an initial outage or first I/O error can
  disable the main integration until process restart.
- `redis_enabled` means configured, not available. Callers use it as both concepts.

Impact:

Redis latency can become game latency. Failure handling is inconsistent: donation retries
too aggressively, world recovery can heal, and ordinary caches/presence remain dead.

Recommendation:

Introduce one adapter state machine with `disabled`, `connecting`, `healthy`, `backoff`,
and `degraded` states. Queue noncritical writes to a bounded worker, pipeline batches, and
make reads fail fast through a circuit breaker. Expose configuration and live health as
separate APIs. Set and test a total per-pulse Redis budget.

### RDS-012 - Automatic backups are not reliably database backups

Severity: High
Confidence: Confirmed shell behavior
Remediation status: Completed on branch; backups are MySQL-only, validated, atomic, and
fail closed before restart.

Evidence:

- [`scripts/backup_pfiles.sh`](../../scripts/backup_pfiles.sh#L20) selects `mysqldump`
  only when `REDIS` is exactly lowercase `true` or `1`. The server accepts case-insensitive
  `TRUE`, and `.env.example` uses uppercase `TRUE`. The default documented configuration
  therefore selects the legacy flat-file branch even though MySQL is authoritative.
- Database backup mode is incorrectly coupled to Redis enablement.
- `mysqldump | gzip` runs without `set -o pipefail`, and `$?` checks `gzip`, not
  `mysqldump`. A failed dump can produce a valid empty gzip and print `Backup complete`.
- The script returns success after either the success or failure message. The cycle script
  invokes it automatically and does not check its result
  ([`scripts/cycle_mud.sh`](../../scripts/cycle_mud.sh#L182)).

Impact:

Operators can believe they have current database backups when only legacy files, or an
empty compressed dump, exist.

Recommendation:

Select backup mode from the actual database architecture, not Redis. Use `set -euo
pipefail`, write to an owner-only temporary file, verify `mysqldump` and gzip status,
validate the dump contains expected schema/data, fsync and atomically rename it, and exit
nonzero on any failure. Make the cycle stop or enter a clearly configured degraded mode
when its required backup fails.

### RDS-013 - Corpse cleanup operates on a retired Redis representation

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; the obsolete cleanup implementation is retired
and now exits nonzero without connecting to or changing either persistent store.

Evidence:

- [`scripts/delete_corpses.sh`](../../scripts/delete_corpses.sh#L67) reads and rewrites a
  legacy JSON key named `mud:world_state`. Current recovery uses binary
  `mud:world_state:generation:<sequence>` plus `mud:world_state:current`.
- Its `redis-cli` calls use the default endpoint and database, ignoring `REDIS_HOST` and
  `REDIS_PORT` loaded from `.env`.
- If a cleanup proceeds, it deletes the entire `mud:floor_drops` hash because it "may
  contain corpses" ([`scripts/delete_corpses.sh`](../../scripts/delete_corpses.sh#L168)).
  That hash can contain every kind of floor object.

Impact:

The script does not remove corpses from the active generation format and can remove
unrelated recovery data from the wrong Redis instance. The runbook nevertheless describes
it as purging Redis corpse state.

Recommendation:

Retire the script until it can decode the current versioned format offline. A replacement
should stop and fence the server, connect with the full safe Redis configuration, identify
exact authoritative corpse UIDs, preserve non-corpse deltas, create a verified backup, and
postflight both SQL and Redis.

### RDS-014 - Artifact cache errors are unsafe

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; cache payloads are versioned and validated, and
every miss/error/malformed payload falls back to the owned MySQL result.

Evidence:

- When Redis is configured but unavailable, `list_artifacts_sql()` generates fresh JSON
  from SQL, tries to cache it, then tries Redis again. If the cache write failed, it tells
  the player `No artifacts found` instead of rendering the generated result
  ([`src/artifact.c`](../../src/artifact.c#L365)).
- A parseable but malformed cached array is dereferenced without checking required field
  existence or type. Missing `vnum`, `locType`, `shortDesc`, or `racewar` can dereference
  null or a null `valuestring` ([`src/artifact.c`](../../src/artifact.c#L435)).
- Artifact keys have no TTL or schema version.

Impact:

A Redis outage produces false gameplay information. Corrupt, stale, cross-environment,
or malicious cache data can crash the game process.

Recommendation:

Generate once and return the owned result directly while asynchronously attempting the
cache write. Validate a versioned typed schema field by field; delete and ignore malformed
entries. Treat all cache failures as misses and preserve the SQL result.

### RDS-015 - Redis tests are mostly source contracts

Severity: Medium
Confidence: Confirmed by test inspection and execution

The focused tests all pass, but most Redis assertions search source text for helper names,
ordering, and forbidden tokens. The world harness exercises
`world_recovery_validate()` framing; it does not materialize a world or use a Redis
server. Current coverage does not exercise:

- initial outage and healing, command timeout, or uncertain `EXEC` outcomes
- pwipe with a null context, a real invalidation failure, or an in-flight publisher
- crash points between generation write, pointer `EXEC`, completion, and floor delete
- duplicate or moved descendants, NPC inventory, semantic restore failure, or rollback
- ship outer-transaction rollback, missed invalidation, malformed JSON, or stale revision
- presence privacy and escaping, donation authenticity/bursts/backoff, or oversized values
- script behavior against stubbed failing `mysqldump` and isolated Redis endpoints

Recommendation: add ephemeral Redis integration tests on a random Unix socket or isolated
port, plus small materialization harnesses with fake world/SQL custody adapters. Keep
source-contract tests only for invariants they can actually prove.

### RDS-016 - The recovery format is process-native

Severity: Medium
Confidence: Confirmed

Headers and records are copied with `memcpy` from native structs. They contain `time_t`,
`unsigned long`, compiler padding, and host-endian integers
([`src/world_recovery_pipeline.h`](../../src/world_recovery_pipeline.h#L17),
[`src/copyover.h`](../../src/copyover.h#L63)). Schema version 7 is a manual constant; it
does not describe field layout or feature compatibility.

Impact: recovery can fail across architecture, ABI, compiler, or struct changes, or can
misinterpret a same-sized layout change if the schema constant is not bumped.

Recommendation: use an explicit little-endian codec with fixed-width integers, bounded
lengths, and per-record versions. Add golden vectors and previous-version compatibility
tests. Keep the in-process structs separate from the durable format.

### RDS-017 - Memory and size bounds are incomplete

Severity: Medium
Confidence: Confirmed

- Capture is limited to 64 MiB, but validation does not reject an incoming value larger
  than that bound. Hiredis must receive and allocate the complete `GET` reply before
  validation runs ([`src/world_recovery_pipeline.c`](../../src/world_recovery_pipeline.c#L493)).
- The publisher holds the generation payload, allocates a second header-plus-payload
  vector, and then passes it to hiredis, which formats another command buffer. Peak memory
  is materially above the documented generation ceiling.
- The same 100 ms command timeout is used for a possible 64 MiB `SET`.
- Only the previously current generation is deleted. Crashes and races can leave orphan
  generation keys indefinitely; generations have no TTL or bounded garbage collector.

Recommendation: check `STRLEN` before `GET`, reject over-limit data, use a chunked or
streamed format, and document a true peak-memory budget. Use a publication-specific
deadline based on measured local throughput. Attach a conservative TTL and perform
bounded epoch-aware orphan collection.

### RDS-018 - Capture is not point-in-time

Severity: Medium
Confidence: Confirmed design property

Capture walks NPCs, room objects, doors, and zone ages over many pulses with a 64-record
and 2 ms per-pulse budget ([`src/world_recovery_pipeline.c`](../../src/world_recovery_pipeline.c#L395)).
Entities and doors can move or change after their record is captured but before the
generation is sealed. The timestamp records capture start, not a consistent boundary.

Impact: one generation can combine states that never coexisted. SQL root reconciliation
limits some stale item restores, but it does not make mob, door, zone, container, or floor
state point-in-time.

Recommendation: explicitly call this a fuzzy recovery snapshot and define acceptable
invariants. For invariants that must be exact, capture stable IDs and mutation revisions,
then retry or exclude a record when its revision changes before sealing. Consider a short
game-thread finalization pass that verifies captured revisions.

### RDS-019 - Floor tracking runs when recovery is off

Severity: Medium
Confidence: Confirmed
Remediation status: Completed on branch; disabled world recovery now performs no floor-delta
capture, write, removal, lookup, restore, or periodic flush work.

`.env.example` enables general Redis but disables world recovery. Floor drop buffering and
the immediate floor-pickup `SADD` check only general Redis, while floor restore occurs only
inside world recovery. `redis_check_floor_pickup()` and `redis_check_floor_drop()` have no
runtime callers. `mud:floor_pickups` has no TTL and is cleared only by successful recovery,
pwipe, or an administrator.

Impact: the default copied configuration performs synchronous writes and accumulates a
set that provides no recovery benefit. Floor hashes can also persist indefinitely when no
generation ACK clears them.

Recommendation: gate the entire floor subsystem on `redis_world_state_enabled`, or remove
it when recovery is off. Delete the unused pickup set or make deltas generation-scoped and
expiring. Add explicit caps and overflow counters to both addition and removal buffers.

### RDS-020 - Administrator Redis commands are unreliable

Severity: Medium
Confidence: Confirmed

- Status helpers map unavailable/error to `0`, `false`, or `-1`, which is also a valid
  empty, missing, or persistent-key result ([`src/redis.c`](../../src/redis.c#L2423)).
- `redis clear world`, `floor`, and individual caches always print `Cleared` even when no
  context exists or a command failed ([`src/wizredis.c`](../../src/wizredis.c#L209)).
- `clear all` omits ship snapshots and online presence, despite its name. It also does not
  clear in-memory floor batches.
- Detailed status performs many sequential synchronous commands on the game thread.
- As described in RDS-002, clear does not fence active writers.

Recommendation: return typed results (`ok`, `unavailable`, `partial`, `failed`), show live
connection health, and report exact postconditions. Drive status and clear from a complete
key registry. Run broad scans/clears outside the simulation thread, with confirmation,
writer fencing, and a bounded deadline.

### RDS-021 - Cache freshness is inconsistent

Severity: Medium
Confidence: Confirmed
Remediation status: Partially remediated; runtime report reads are bounded local-memory
lookups, Redis writes are asynchronous and coalesced, and existing artifact/epic TTLs are
enforced locally. Named/fraglist TTL or revision contracts and stable countdown rendering
remain open.

Named, fraglist, artifact, and ship caches are persistent and unversioned; only epic zones
has a 900-second TTL. Invalidation is distributed across unrelated gameplay files.
Fraglist caches an already-rendered countdown derived from `next_update`, so the displayed
timer freezes until another invalidation. The maintenance level-cap completion path does
not invalidate that cache ([`src/comm.c`](../../src/comm.c#L137),
[`src/redis.c`](../../src/redis.c#L2137)).

Recommendation: cache stable data, not time-relative presentation. Put schema version,
source revision, generated time, and bounded TTL in every cache contract. Centralize
invalidation after authoritative commit, or prefer revision-keyed reads so missed deletes
cannot make old data current.

### RDS-022 - Lifecycle inventory misses Redis stores

Severity: Medium
Confidence: Confirmed
Remediation status: Partially remediated; presence state, events, and retry tokens are now
required lifecycle entries alongside world recovery. Floor state, content caches, and the
remaining Redis surfaces still need registry-backed inventory.

The audit baseline lifecycle manifest had only `redis:world_recovery`. This branch now
also requires `redis:presence`, covering the online hash, player-event channel, and retry
tokens. It still omits floor hashes, content caches, and other Redis surfaces. The
validator's required list remains handwritten rather than generated from a runtime key
registry, so it cannot discover a new or forgotten Redis store automatically. Tests can
therefore still report full inventory coverage when both handwritten lists omit the same
surface.

Impact: privacy, export, erasure, retention, season reset, and documentation controls can
pass while known Redis data is outside the policy boundary.

Recommendation: create one declarative Redis key/channel registry used by runtime,
pwipe/admin tools, documentation, and lifecycle validation. The validator should compare
the manifest against keys/channels generated from that registry rather than a duplicate
handwritten list.

### RDS-023 - Broad FLUSHDB scripts lack full safety

Severity: Medium
Confidence: Confirmed

- [`scripts/clear-redis.sh`](../../scripts/clear-redis.sh#L31) performs `FLUSHDB` using
  host and port only. It has a good owner/mode/symlink check for `.env`, but no explicit
  Redis database, ACL, TLS, environment allow-list, or destructive confirmation.
- The legacy migration runner requires Redis host/port even if Redis is disabled and
  flushes the entire database. If `redis-cli` is absent it prints `skipped` without
  incrementing its failure count, then exits successfully
  ([`migrations/run_migration.sh`](../../migrations/run_migration.sh#L3033)).
- The runbook contradicts the implementation by saying `clear-redis.sh` does not read
  `.env` ([`docs/operations/RUNBOOK.md`](../operations/RUNBOOK.md#L238)).

Recommendation: replace `FLUSHDB` with deletion of an exact versioned application prefix.
Require explicit environment/instance identity, stopped-writer proof, full authenticated
connection settings, confirmation, and postflight verification. A missing client or
failed invalidation must fail a migration that depends on cache clearing.

### RDS-024 - Graceful shutdown is treated as crash recovery

Severity: Medium
Confidence: Confirmed behavior; product intent needs confirmation

Normal shutdown drains the recovery pipeline but does not clear the current generation
([`src/redis.c`](../../src/redis.c#L464)). A restart within the configured maximum age sees
that generation before `boot_db()` and takes the crash-recovery path. Documentation says
this path runs after an unclean exit
([`docs/operations/RUNBOOK.md`](../operations/RUNBOOK.md#L204)).

Recommendation: decide explicitly whether world preservation across graceful restart is
desired. If yes, rename and document it as restart recovery and record a clean-shutdown
marker. If no, clear the generation only after every required graceful shutdown save has
completed. Test manual shutdown, reboot, autoreboot, copyover, crash, and pwipe separately.

### RDS-025 - Observability is too coarse

Severity: Medium
Confidence: Confirmed
Remediation status: Partially remediated; presence and report-cache workers expose local
state, queue/byte high water, completions, drops, command failures, and reconnects without
network queries. The shared world/floor/admin context still lacks per-subsystem telemetry.

The world pipeline exposes useful counters and timing, but the shared command layer emits
only one global rate-limited line per second with a broad outcome. It omits command class,
subsystem, key class, retry state, consecutive failures, and recovery transition
([`src/redis.c`](../../src/redis.c#L81)). Status conflates empty data with unavailable
Redis, and there are no comparable health snapshots for donation, ship cache, presence,
or generic caches.

Recommendation: add redacted per-subsystem counters for calls, latency, timeout, error,
circuit state, reconnects, dropped work, last success age, and queue high water. Expose a
single typed health snapshot to both operator commands and health diagnostics.

### RDS-026 - Responsibilities are overly coupled

Severity: Medium
Confidence: Confirmed structural issue

`src/redis.c` is 2,525 lines and combines connection/configuration, world publication,
floor recovery, ship serialization, report caches, presence, pub/sub, administrator
helpers, and legacy UID handling. `redis.h` also owns revisioned player-save wrapper APIs
that do not use Redis. Twenty-nine C/C++ translation units include the header, many only
for dirty-player functions. Host/port parsing and connection construction are repeated.

Impact: changes have a broad compile/review surface, subsystem policy is inconsistent,
and tests tend to assert monolithic source layout rather than typed interfaces.

Recommendation: split into a small client/config adapter, key registry, world recovery,
floor deltas, ship cache, content cache, presence, donation, and admin modules. Move
player checkpoint wrappers to a persistence header. Keep each subsystem's availability,
codec, TTL, authority, and lifecycle policy beside its implementation.

### RDS-027 - The no-MySQL build option is broken

Severity: Low
Confidence: Confirmed by syntax probe

The Makefile advertises a commented `-D__NO_MYSQL__` toggle, but `redis.c` declares
`redisContext` pointers and a `redisReply` helper signature outside the hiredis guard.
Included SQL headers also expose unguarded `MYSQL` types. The syntax probe failed before
linking. Hiredis is linked unconditionally even though runtime Redis is optional
([`src/Makefile`](../../src/Makefile#L31)).

Recommendation: either remove the unsupported build promise or make the feature guards
complete and conditionalize libraries and objects. Add the supported variant to CI.

### RDS-028 - Legacy keys and unused APIs remain

Severity: Low
Confidence: Confirmed by call-site search
Remediation status: Completed on branch; the retired UID key is no longer read, written, or
displayed, and the unused generic Redis exports have been removed.

`mud:next_obj_uid` is read only to log that SQL remains authoritative, yet it is still
written at cleanup and displayed by the administrator command
([`src/redis.c`](../../src/redis.c#L512)). `redis_ping`, `redis_check_floor_pickup`,
`redis_check_floor_drop`, `redis_publish`, and `redis_get_string` have no production
callers.

Recommendation: remove retired key writes and dead exports after one explicit cleanup
release, or mark a bounded compatibility window. This reduces operator ambiguity and
keeps the key registry and lifecycle inventory accurate.

## Prioritized remediation plan

### P0 - Integrity blockers

1. Replace pwipe's delete-list protocol with a fenced season epoch and irreversible reset
   state machine. Stop old writers before any SQL mutation.
2. Make world publication and floor-delta consumption one epoch-aware atomic operation.
3. Stage recovery and validate every item UID, VNUM, parent, root, state, and owner before
   materialization. Add exact rollback or full normal-boot fallback.
4. Remove Redis as a ship authority. Re-enable only after SQL revision validation and
   post-commit publication exist.
5. Fix `backup_pfiles.sh` mode selection, `pipefail`, validation, atomic publication, and
   nonzero failure propagation.

### P1 - Trust and availability

1. Introduce the central Redis adapter, namespace/key registry, ACL/TLS/database settings,
   health state machine, circuit breaker, and reconnect backoff.
2. Replace native recovery structs with a fixed-width versioned codec and add a true
   inbound/peak-memory bound.
3. Enforce presence privacy centrally and harden donation envelopes, budgets, escaping,
   and feature gating.
4. Move noncritical Redis writes off the simulation thread and pipeline bounded batches.

### P2 - Operability and maintainability

1. Version and expire all reconstructible caches; centralize commit-aware invalidation.
2. Rewrite or retire destructive Redis scripts and make admin clear/status checked,
   complete, fenced, and asynchronous.
3. Expand lifecycle inventory to every key and channel from the same runtime registry.
4. Split the Redis monolith and remove legacy UID/dead APIs.
5. Reconcile clean-shutdown recovery semantics and operational documentation.

## Required regression matrix

The following tests should gate production recovery enablement:

| Test | Required assertion |
| --- | --- |
| Redis starts down, later heals | No game-thread stall loop; all intended subsystems recover through bounded backoff; sequences do not rewind. |
| Pwipe with null/error context | Reset cannot claim success; no destructive SQL begins without the Redis/epoch precondition. |
| Pwipe with capture queued/publishing | Old generation cannot publish after the reset fence. |
| Kill after generation `SET` | Prior pointer remains valid; orphan is later collected. |
| Kill after pointer `EXEC`, before floor cleanup | Each UID is materialized at most once. |
| Root child moved after capture | Stale child is rejected; no new UID is created for it. |
| Missing VNUM/room or invalid door/zone | Restore applies nothing and normal boot produces a complete world. |
| Ship save inside outer rollback | Redis never exposes the uncommitted ship; SQL remains authoritative. |
| Cache update/invalidation outage | A recovered Redis cannot make an older ship or deleted row current. |
| Malformed ship/artifact JSON | Typed validation rejects the key without a crash and falls back to SQL. |
| Presence with privacy off/invisible staff | No account, IP, client metadata, or invisible identity reaches Redis. |
| Donation flood/spoof/replay | Signature, limits, replay protection, work budget, and backoff hold. |
| Oversized generation | Rejected before a large allocation or materialization. |
| Concurrent/stale publishers | Epoch and fence CAS permit only the current writer to advance the pointer. |
| Backup dump failure | No backup is published, exit is nonzero, and cycle handling is explicit. |
| Clean shutdown vs crash vs copyover | Each transition follows its documented recovery policy exactly. |

## Acceptance criteria for closing this audit

- No Redis-derived entity is materialized before complete structural, semantic, and SQL
  authority validation.
- A stale process cannot write into the current season namespace.
- Every destructive reset or clear has a preflight, writer fence, exact checked mutation,
  and postflight; it never resumes a partially reset old process.
- MySQL remains authoritative for ships and all durable item custody under cache failure.
- Redis configuration supports least privilege and environment isolation.
- Private presence is omitted by default on every transport.
- No synchronous Redis failure can consume an unbounded or repeated game-thread budget.
- Every declared key/channel appears in one runtime key registry, lifecycle inventory,
  pwipe policy, admin tooling policy, and behavioral test matrix.
- Ephemeral live-Redis tests cover outage, retry, transaction ambiguity, crash boundaries,
  malformed data, and multi-writer fencing.
