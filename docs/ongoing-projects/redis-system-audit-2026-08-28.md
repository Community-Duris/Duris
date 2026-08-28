# Redis System Audit

Date: 2026-08-28
Branch: `redis-refactor`
Audit baseline commit: `68a916ec`
Status: Implementation in progress; RDS-006, RDS-012, RDS-014, and RDS-019 are remediated
and the remaining findings are open.

## Implementation progress

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

- All findings other than RDS-006, RDS-012, RDS-014, and RDS-019 remain open. The acceptance
  criteria are not yet met.

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
| World recovery | `mud:world_state:generation:<seq>`, `mud:world_state:current`, diagnostic metadata | Optional recent world reconstruction | Redis payload directly materializes mobs, objects, doors, zones, gold, affects, and equipment after structural validation. |
| Floor deltas | `mud:floor_drops`, `mud:floor_pickups` | Bridge changes around a world snapshot | Root SQL ownership is checked, but descendant identity/state is incomplete. |
| Ship cache | `ship:snapshot:<owner>` | Reconstructible SQL cache | Cache is returned before SQL without TTL, row revision, or schema/environment identity. |
| Content caches | `mud:cache:named`, `mud:cache:fraglist`, `mud:cache:epic_zones`, artifact variants | Reconstructible command output | Most are persistent rendered strings with scattered invalidation. |
| Presence | `mud:online`, `mud:player` | Web presence and login/logout events | Stores account, IP, client metadata, and invisible staff presence regardless of the documented privacy switch. |
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

The lifecycle manifest has one Redis entry, `redis:world_recovery`
([`migrations/data_lifecycle_manifest.json`](../../migrations/data_lifecycle_manifest.json#L4954)).
It omits presence/account/IP/client data, floor hashes, ship snapshots, content caches,
legacy UID state, and channels. The validator hardcodes that same single Redis entry as
the required inventory ([`scripts/validate_data_lifecycle.py`](../../scripts/validate_data_lifecycle.py#L57)),
so it cannot discover a new or forgotten Redis store. Tests then report full inventory
coverage because the hardcoded list matches itself.

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
