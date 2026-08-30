# Configuration

Duris reads environment settings from `.env` in the data directory during
startup. The parser accepts one `KEY=VALUE` assignment per line; blank lines and
lines beginning with `#` are ignored. Values are not shell-expanded or
quote-aware, so do not wrap values in quotes. Existing process environment
variables are preserved because `.env` only supplies variables that are not
already set.

Start from [`.env.example`](../../.env.example):

```bash
cp .env.example .env
chmod 600 .env
```

Keep `.env` local and never commit passwords, HMAC secrets, or production
connection details. The server checks metadata before reading: `.env` must be
a regular file owned by the effective server user and must grant no permission
beyond owner read/write (`0600`).

## Persistence

`PERSISTENCE_MODE` selects one whole-server authority. It defaults to
`mariadb-primary`. The accepted values are `mariadb-primary`,
`mariadb-primary-flatfile-fallback`, and `flatfile-primary`. A MariaDB client build accepts
`mariadb-primary`; a client-free flat build accepts `flatfile-primary`. The legacy mixed
fallback token is recognized for a clear diagnostic but fails closed because mixed
per-operation authority transfer is not supported.

| Variable | Requirement | Meaning |
| --- | --- | --- |
| `PERSISTENCE_MODE` | Optional; defaults to `mariadb-primary` | Select the complete persistence authority; mixed per-write failover is not supported. |
| `FLATFILE_STATE_DIR` | Required by `flatfile-primary` | Absolute server-user-owned directory with mode `0700` or stricter. |
| `FLATFILE_BACKUP_DIR` | Optional in `flatfile-primary`; defaults to `backups/flatfile` | Absolute backup root outside `FLATFILE_STATE_DIR`; each pre-boot snapshot is owner-only. |
| `ENVIRONMENT` | Required: `local` or `production` | Runtime trust role. |
| `DB_HOST` | Required by `mariadb-primary` | MySQL/MariaDB host. |
| `DB_PORT` | Optional; `1`-`65535` | Database TCP port; the client default applies when omitted. |
| `DB_USER` | Required by `mariadb-primary` | Database account. |
| `DB_PASSWD` | Required by `mariadb-primary` | Database password. |
| `DB_NAME` | Required by `mariadb-primary` | Requested database name. |
| `DB_ALLOWED_TARGETS` | Required by `mariadb-primary` | Comma-separated exact `host/database` pairs; the resolved pair must match. |
| `DB_SOCKET` | Optional, local role only | Protected local Unix socket used instead of remote transport. |
| `DB_TLS` | Required as `TRUE` for non-loopback hosts | Enforce encrypted database transport. |
| `DB_SSL_CA` | Required for non-loopback hosts | Regular CA file used to verify the database server certificate. |
| `PLAYER_SAVE_JOURNAL_DIR` | Required outside mini mode | Absolute server-user-owned `0700` directory for revisioned player snapshots. |
| `CRITICAL_COMMAND_JOURNAL_DIR` | Required outside mini mode | Absolute server-user-owned `0700` directory for non-coalescing critical commands. |
| `MAINTENANCE_STATE_FILE` | Optional; `bin/server/maintenance-scheduler.state` | Durable scheduler cursor/completion state; parent directory must be server-user controlled. |

`scripts/cycle_mud.sh --check-config` validates the selected mode without starting the
server. Add `--production` to require `ENVIRONMENT=production` and the port-7777
runtime role; the production systemd unit always supplies that flag. `--production`
cannot be combined with `--dev` or `--minimal`. In `flatfile-primary`, the launcher
does not require database settings, run migrations or schema checks, invoke MySQL
shutdown logging, or select a database backup because Redis happens to be enabled. It
snapshots the selected `FLATFILE_STATE_DIR` before boot and refuses to start if that
snapshot fails. Build the client-free binary with
`make -C src PERSISTENCE_BACKEND=flatfile` for this mode.

Flat-file IP connection history is stored in the owner-only metadata authority so the
existing one-hour racewar-side rule and staff/player information paths remain functional
without a database. Treat the state root and its backups as private player data. Boot
closes sessions left active by an interrupted prior run and refuses corrupt IP history.

`DB_NAME` selects the requested database and `DB_ALLOWED_TARGETS` authorizes the
resolved target. The listen port is an additional guard, not the primary selector.
Production role requires port `7777`; on any other port an explicitly production-like
name (`duris` or `duris_prod`) is redirected to `duris_dev` before the allow-list check.
Use a separate database account, target, and non-`7777` port for development. The
redirect does not make a production credential safe to reuse locally.

Every connection has 10-second connect/read/write deadlines, disables automatic
reconnect, and must establish the same verified session contract: `utf8mb4`, time zone
`+00:00`, `READ-COMMITTED`, and `STRICT_TRANS_TABLES`,
`ERROR_FOR_DIVISION_BY_ZERO`, and `NO_ENGINE_SUBSTITUTION`. Loopback TCP and an
explicit local-role socket are treated as protected local transport. Any other host
requires enforced TLS, CA verification, and a negotiated cipher. Boot also requires a
supported MySQL 8.0 or MariaDB 10.11 normalized metadata fingerprint before mutation.

Both journal directories are mandatory for normal operation. They must be absolute,
owned by the server user, and mode `0700` or stricter; their files are permission
checked, checksummed, size bounded, and fail closed on corruption or quota exhaustion.
Do not place either directory under a shared or automatically cleaned temporary path.

## Redis

Redis is optional. It is disabled unless `REDIS=TRUE` (case-insensitive).
When enabled, it stores floor-drop recovery data, object UID state, caches, and
optional immutable world-recovery generations. Player dirty state remains local to the
revisioned player-save pipeline and typed journal.

| Variable | Default | Accepted values / range | Meaning |
| --- | --- | --- | --- |
| `REDIS` | disabled | `TRUE` enables it | Enable Redis integration. |
| `REDIS_HOST` | `127.0.0.1` | hostname or IP | Redis TCP host. Must be empty when `REDIS_SOCKET` is set. |
| `REDIS_PORT` | `6379` | `1`-`65535` | Redis TCP port. Must be empty when `REDIS_SOCKET` is set; an explicitly invalid TCP value disables Redis at boot. |
| `REDIS_SOCKET` | empty | Absolute path, at most 107 bytes | Optional local Unix socket used instead of TCP. It is mutually exclusive with `REDIS_HOST`/`REDIS_PORT` and with TLS. Every runtime worker uses the same socket through the shared adapter. |
| `REDIS_DB` | `0` | `0`-`255` | Database explicitly selected by every runtime connection and destructive maintenance command. |
| `REDIS_NAMESPACE` | none | `duris:<ENVIRONMENT>:<deployment>` | Required isolation prefix for every active key and channel. Deployment is 1-32 lowercase letters, digits, hyphens, or underscores and must not begin or end with punctuation. |
| `REDIS_USERNAME` | empty | Redis ACL username | Shared local-development fallback. Production does not accept it in place of scoped identities. |
| `REDIS_PASSWORD` | empty | Redis ACL password | Password paired with the local-development fallback identity. |
| `REDIS_WORLD_USERNAME`, `REDIS_WORLD_PASSWORD` | local fallback | Complete ACL pair | World/floor recovery identity. Required in production. |
| `REDIS_PRESENCE_USERNAME`, `REDIS_PRESENCE_PASSWORD` | local fallback | Complete ACL pair | Presence key and event-channel identity. Required in production. |
| `REDIS_CACHE_USERNAME`, `REDIS_CACHE_PASSWORD` | local fallback | Complete ACL pair | Reconstructible content-cache identity. Required in production. |
| `REDIS_DONATION_USERNAME`, `REDIS_DONATION_PASSWORD` | local fallback | Complete ACL pair | Donation subscription identity. Required in production only when the subscriber is enabled. |
| `REDIS_MAINTENANCE_USERNAME`, `REDIS_MAINTENANCE_PASSWORD` | local fallback | Complete ACL pair | Retired ship cleanup, pwipe, and stopped-server destructive-maintenance identity. Required in production. The shell helper passes its password through `REDISCLI_AUTH`, not a command argument. |
| `REDIS_TLS` | `FALSE` | Exact `TRUE` or `FALSE` | Enables verified TLS for every TCP runtime and maintenance connection. Non-loopback production runtime endpoints require `TRUE`; destructive maintenance requires it for every non-loopback TCP target. Unix sockets require `FALSE`. |
| `REDIS_CA_CERT` | empty | Readable CA bundle | Required when Redis TLS is enabled and used for peer verification. |
| `REDIS_TLS_SERVER_NAME` | `REDIS_HOST` | Certificate DNS name | Optional runtime SNI and certificate-name override, useful when connecting by IP to a certificate issued for a DNS name. |
| `REDIS_ALLOWED_TARGETS` | none | Comma-separated exact `host:port/database` or `unix:/absolute/socket/database` values | Required destructive-maintenance allow-list. |
| `REDIS_WORLD_STATE` | disabled | `TRUE` enables it | Enable bounded capture and background publication of crash-recovery world generations. |
| `REDIS_WORLD_STATE_INTERVAL` | `10` seconds | `5`-`300` | Snapshot interval when world-state recovery is enabled. |
| `REDIS_WORLD_STATE_MAX_AGE` | `300` seconds | `60`-`3600` | Maximum snapshot age accepted during recovery. |
| `REDIS_WORLD_STATE_SECRET` | none | `32`-`256` bytes | Independent HMAC key required when world recovery is enabled. It authenticates the manifest and complete generation payload; do not reuse Redis, database, donation, or DurisWeb credentials. |
| `REDIS_WORLD_STATE_SECRET_PREVIOUS` | empty | `32`-`256` bytes | Optional previous recovery HMAC key accepted only for reading and cleanup during a bounded rotation window. New generations are always signed by the current key. |
| `REDIS_DONATION_SUBSCRIBER` | disabled | Exact `TRUE` enables it | Subscribe to authenticated external donation notices. No polling job or subscriber connection exists by default. |
| `REDIS_DONATION_SECRET` | none | At least 32 bytes | Independent HMAC key required when the donation subscriber is enabled. Do not reuse a Redis, database, or DurisWeb secret. |

World recovery is intentionally separate from player saves and reconstructible caches.
At boot the server constructs immutable connection settings for each subsystem. In
production, every required scoped username must be nonempty and distinct; an incomplete
pair or reused username disables Redis before gameplay starts. Authentication occurs only
when a worker, boot/recovery path, or maintenance path opens or reconnects a connection.
Gameplay enqueue and cache-read paths do not perform authentication or connection work.

Provision ACL users with unique passwords and the narrowest command set supported by the
deployed Redis version. Key/channel boundaries should be:

| Identity | Allowed keys/channels |
| --- | --- |
| world | `<REDIS_NAMESPACE>:season:*:world_state:*` and `<REDIS_NAMESPACE>:season:*:floor_*` |
| presence | `<REDIS_NAMESPACE>:season:*:presence:*`, `<REDIS_NAMESPACE>:season:*:presence_op:*`, and publish only to `<REDIS_NAMESPACE>:season:*:player` |
| cache | `<REDIS_NAMESPACE>:season:*:cache:*` |
| donation | subscribe only to `<REDIS_NAMESPACE>:season:*:nchat`; no key access |
| maintenance | `<REDIS_NAMESPACE>:*`, `mud:*`, and `ship:snapshot:*`; allow only connection, scan, delete, and required Lua execution commands |

The world, presence, and cache workers need their respective read/write/Lua commands plus
`PING` and `SELECT`; they do not need administrative, server-management, or cross-prefix
access. The donation identity needs only `PING`, `SELECT`, and `SUBSCRIBE` with the channel
pattern above. Maintenance is deliberately broader in key scope because it removes active
and retired Duris surfaces, but it must not have access to other applications' prefixes.
Test the exact ACL rules on a disposable Redis instance before deployment; Redis command
categories and Lua ACL behavior can differ across supported server versions.

At boot, one publisher claims a renewable 10-minute writer lease. Each background
publication verifies that lease and expected prior pointer, writes the immutable
sequence-keyed payload, advances the current pointer and diagnostic metadata, consumes
the pre-capture floor hash, and renews the lease in one atomic Lua compare-and-set. A
stale or second writer cannot publish. The single script also reduces background Redis
round trips compared with a watched transaction.
All of those keys use `<REDIS_NAMESPACE>:season:<epoch>:` with the active SQL season epoch captured at
boot. An old process can therefore write only its abandoned epoch after a reset; it cannot
create a snapshot visible to the new season.
Boot accepts only a complete, non-expired generation whose schema, sequence, size, and
checksum validate. It combines the generation with versioned binary floor records,
validates the full semantic graph, and batch-reconciles every item UID against SQL before
creating any entity. A failed or stale generation is retained for diagnosis and the
server performs a full normal zone boot.

World generations are capped at 64 MiB. Restore checks the value length inside Redis
before transfer. Each published generation receives a TTL of at least one hour or four
times `REDIS_WORLD_STATE_MAX_AGE`, whichever is greater, so abandoned generations expire.
The background publisher scales its write timeout for the blob size, up to five seconds;
this does not extend the game-loop Redis command deadline.

Graceful shutdown preserves the latest valid world generation for restart recovery. After
all world and floor work drains, the fenced writer records a one-use clean-shutdown marker
for that exact sequence. The next boot consumes the marker and reports `clean restart`
only when the validated current generation matches; otherwise it reports `crash`
recovery. Successful restore consumes that generation without disabling future snapshots.

Floor deltas use a separate background worker bounded to eight batches and 16 MiB. Each
batch holds at most 2,048 mutations, each value is capped at 256 KiB, and keys are capped
at 128 bytes. Each value is a binary tree of at most 12 identity-preserving items; larger
trees fail capture closed rather than being truncated. Before world capture, an ordered
worker barrier confirms all earlier deltas and pauses later publication; the generation
handoff deletes the acknowledged hash atomically, then post-barrier deltas resume.
Gameplay performs bounded fixed-memory serialization but no Redis socket, SQL, disk,
process, or logging I/O for floor drops, pickups, or snapshot preflight.

World capture is an explicitly fuzzy crash-recovery snapshot with a hard five-minute
capture deadline. It keeps the existing 64-step/2-ms per-pulse gameplay budget; an expired
capture is discarded and retried later rather than published. NPC inventory/equipment and
carried gold are excluded from recovery, while all floor-item UIDs must pass complete SQL
custody reconciliation before any recovery entity is created. `REDIS_WORLD_STATE_MAX_AGE`
still controls how old a completed durable generation may be when boot attempts restore.

The in-game `redis` and `redis detailed` commands read bounded local worker/pipeline
telemetry only; they never query Redis. Shared boot, recovery, and stopped-server
maintenance commands are grouped by redacted subsystem and operation kind, with local
call, failure, latency, last-success-age, and reconnect counters. Presence, report-cache,
floor, donation, and world-publication workers expose the same operation health dimensions
alongside their bounded queue state. No key, value, account, player, item, endpoint, or
credential is retained. Online artifact, fraglist, epic-zone, and named cache clears remove
the local entry and submit a background invalidation, reporting whether that submission
was accepted. The in-game `redis clear world`, `redis
clear floor`, and `redis clear all` commands are refused while the server is running
because their scans, writer fencing, and exact postflight cannot safely run on the
simulation thread. Stop the server and use the maintenance clear workflow for broad state
removal.

For a local development session, the following is a reasonable starting point:

```text
REDIS=TRUE
REDIS_HOST=127.0.0.1
REDIS_PORT=6379
REDIS_SOCKET=
REDIS_DB=0
REDIS_NAMESPACE=duris:local:default
REDIS_TLS=FALSE
REDIS_WORLD_STATE=TRUE
REDIS_WORLD_STATE_SECRET=replace-with-an-independent-random-secret
```

Stop the server before clearing Redis state. `scripts/clear-redis.sh --confirm
<host:port/database|unix:/absolute/socket/database>` loads the owner-only `.env`, requires
`ENVIRONMENT=local`, checks the
exact target against `REDIS_ALLOWED_TARGETS`, applies the configured database, ACL, and TLS
settings, uses the maintenance identity when configured, and deletes only
`<REDIS_NAMESPACE>:*`, legacy `mud:*`, and retired
`ship:snapshot:*` keys. It uses cursor scans and at most 128 keys per `DEL`, verifies that
all three Duris patterns are empty afterward, and leaves
unrelated application keys intact. Missing `redis-cli`, connection failure, unexpected replies, wrong
confirmation, or a failed postflight returns nonzero.

Redis uses a 250 ms connect timeout and 100 ms command timeout. A cache failure may
degrade a report, while a world-generation failure preserves the prior generation and
floor deltas. Neither case authorizes a synchronous player save or journal deletion.

Presence login/logout updates use a dedicated worker with a fixed 1,024-job queue, bounded
timeouts, and exponential reconnect backoff. Gameplay paths only encode the bounded JSON
payload and enqueue it; they never wait for a presence connection or Redis command. Each
state change and optional `<REDIS_NAMESPACE>:season:<epoch>:player` event is one idempotent Lua operation. Pwipe joins
and cancels this worker before checked deletion, and shutdown gives it a one-second drain
deadline. Connection outages retain ordered jobs until Redis returns; a job is dropped
after three command-level failures so a permanent schema or ACL error cannot block the
queue indefinitely. Online state uses `<REDIS_NAMESPACE>:season:<epoch>:presence:current` plus
`<REDIS_NAMESPACE>:season:<epoch>:presence:session:<instance>:<pid>` keys with a 180-second TTL. The worker refreshes
active leases every 60 seconds in batches of at most 64; a crashed server, failed logout,
or superseded worker therefore cannot leave persistent presence data. A due heartbeat is
processed ahead of queued login/logout work so a sustained backlog cannot starve active
leases past their TTL.

Named, fraglist, epic-zone, and artifact report reads use a bounded 32-entry in-process
cache and never wait for Redis during gameplay. Redis publication and invalidation use a
separate worker bounded to 64 jobs and 4 MiB of queued values; repeated mutations for one
key are coalesced. Values are limited to 1 MiB and keys to 128 bytes. Existing artifact
cache values under `<REDIS_NAMESPACE>:season:<epoch>:cache:*` are seeded with their remaining TTL in one boot-only Redis operation, while
expired or persistent legacy artifact values are ignored. Pwipe cancels the worker before
checked deletion and shutdown gives it a one-second drain deadline.

Retired `ship:snapshot:*` invalidations use a separate bounded asynchronous worker with
the maintenance identity. Ship deletion and owner-rename paths only enqueue the legacy
key; connection, authentication, and deletion stay off the gameplay thread. Pwipe cancels
the worker before its checked maintenance sweep, and normal shutdown gives it a one-second
drain deadline.

Every report cache has a bounded freshness contract: named-set output expires after 24
hours, while fraglist, epic-zone, and artifact output expire after 15 minutes. Successful
combat-outcome and level-cap commits also invalidate the fraglist asynchronously. The
fraglist stores only stable leaderboard/cap content plus an absolute cap deadline in the
versioned `FRC1` frame. Each local hit renders the countdown from that deadline, so the
timer advances without a Redis or SQL query. Frame schema, generated time, content
revision, component lengths, maximum age, and clock skew are validated before display.

Donation notices use a separately gated, authenticated subscriber worker. Connect,
subscribe, socket reads, validation, replay filtering, and reconnect backoff all run off
the simulation thread. It subscribes only to `<REDIS_NAMESPACE>:season:<epoch>:nchat`, where the epoch is
captured from SQL once at boot. Its delivery queue holds at most 64 fixed-size validated events;
each game pulse dequeues at most eight and performs no Redis work. Invalid, stale,
oversized, replayed, or excess envelopes are counted and ignored. The publisher contract
and signature format are in [the donation event reference](../reference/api/donation-events.md).

## Character creation and gameplay modes

These switches are intended for local testing and should remain disabled on a
live server:

| Variable | Enabled when | Effect |
| --- | --- | --- |
| `CREATION_ALL_RACES` | value equals `TRUE` | Adds normally unavailable races to character creation. They are selected by typing the race name. |
| `CREATION_ALL_CLASSES` | value equals `TRUE` | Adds every defined class to character creation, including restricted classes. |
| `CHAOS_MUD` | value equals exactly `TRUE` | Enables chaos rules, including automatic level-56 characters. Any other value disables the mode and reports a warning for invalid input. |

The character-creation settings affect the menus and validation paths; they do
not change the underlying race/class data or make restricted choices suitable
for production.

## WebSocket and proxy settings

| Variable | Meaning |
| --- | --- |
| `LISTEN_ADDRESS` | Numeric IPv4 or IPv6 address applied to telnet, TLS telnet, and WebSocket listeners. Use `127.0.0.1` or `::1` for local development. |
| `DURIS_TLS_PORT` | Optional independent TLS telnet port. It defaults to `7778`, or to the plain-telnet port plus one when a custom plain port is supplied. Values must be decimal ports from 1 through 65535 and must differ from the plain port. |
| `DURIS_WEBSOCKET_PORT` | WebSocket and HTTP health-listener port. It defaults to `4050`; values must be decimal ports from 1 through 65535. |
| `DURIS_WEBSOCKET_LISTEN_ADDRESS` | WebSocket-only numeric listener address; defaults to `LISTEN_ADDRESS`, and to `127.0.0.1` when neither is set. Production requires exact loopback so a local TLS reverse proxy owns the public endpoint. |
| `DURIS_WEBSOCKET_ALLOWED_ORIGINS` | Exact comma-separated browser `Origin` allow-list. Required in production; non-browser service connections may omit `Origin`. |
| `DURISWEB_SECRET` | Current shared key for one-time DurisWeb challenge-response authentication. See the DurisWeb API reference. |
| `DURISWEB_SECRET_PREVIOUS` | Optional previous service key accepted during a bounded zero-downtime rotation. Remove it after every backend has switched. |
| `DURISWEB_PRIVATE_PRESENCE` | Exact `TRUE` opts the authenticated backend into account names, IP addresses, client metadata, and invisible staff presence. The default WebSocket and Redis presence feeds omit them. |
| `DURIS_TRUSTED_PROXY_IP` | One immediate proxy IP address whose `X-Forwarded-For` header may be trusted for WebSocket and telnet connections. If unset, forwarded addresses are ignored. This is an address allow-list, not a CIDR range. |

WebSocket and `GET /health` listen on `DURIS_WEBSOCKET_PORT` (default `4050`).
In production, the WebSocket listener must use loopback, the trusted proxy and
allowed origins must be configured, and the local reverse proxy must terminate
TLS before forwarding to this plaintext listener. The server refuses to create
the production listener when those controls are absent.
The health response reports only process and in-memory database-pool readiness and
performs no blocking database query. Plain telnet defaults to `7777` and TLS telnet to
`7778`; a custom plain-telnet port uses the following port for TLS unless
`DURIS_TLS_PORT` provides an independent port. Configure a real `duris.crt` and
`duris.key` in the repository root for networked TLS. The operator key must be
owner-controlled and mode `0600` or stricter. The tracked self-signed key was removed;
run `./scripts/generate_localhost_cert.sh` to create an ignored machine-local fallback.
That fallback is accepted only with the explicit local role and an exact loopback
listener, and its key must also be owner-controlled and mode `0600` or stricter.

## Diagnostics

Diagnostic switches are opt-in and are read once when the relevant subsystem
initializes. They can be noisy, so enable them only while investigating a
specific issue and restart the server after changing them.

| Variable | Value | Output / scope |
| --- | --- | --- |
| `SQL_TRACE` | any non-empty value except `0`, `false`, or `off` | Metadata-only SQL execution events in the normal logs. |
| `GET_TRACE` | any non-empty value except `0`, `false`, or `off` | Debug logging for object pickup paths. |
| `DURIS_ZONE_RESET_TRACE` | positive integer | Zone-reset tracing. |
| `DURIS_CORPSE_TRACE` | any non-empty value except `0` | Corpse decay tracing. |
| `DURIS_ACCEPT_DEBUG` | variable present, including an empty value | Connection-accept debug counters. |

`SQL_TRACE` never writes query text, bound values, MySQL error prose, account or
player values, or per-query files. Each event contains only a process-local
operation ID, compile-time source site, execution context, statement kind,
duration, numeric error code, and SQLSTATE. The operation ID is useful for log
correlation within one server process; it is not a durable transaction or
idempotency ID. Trace events still add log volume, so leave the switch disabled
outside a focused investigation.

Use the normal log files described in [RUNBOOK.md](RUNBOOK.md) and remove
these switches from `.env` when the investigation ends.

### Event-wheel limits

These are tuning knobs rather than traces, read by `nevent_config_limit()` when
the event system initializes:

| Variable | Default | Effect |
| --- | --- | --- |
| `DURIS_NEVENT_BUDGET_USEC` | `25000` (25 ms) | Wall-clock budget for event callbacks per pulse. |
| `DURIS_NEVENT_MAX_CALLBACKS` | `4000` | Callback count cap per pulse. |
| `DURIS_NEVENT_CATCHUP_MAX_EXTENSION_USEC` | `5000` | Maximum time-budget extension while repaying deferred work. |
| `DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS` | `4000` | Maximum callback-cap extension while repaying deferred work. |
| `DURIS_NEVENT_PLAYER_PRIORITY` | `1` | Set to `0` to disable player-timed priority. |
| `DURIS_NEVENT_TRACE_PLAYER` | `0` | Set to `1` for per-player deadline timing logs. |
| `DURIS_NEVENT_ANALYTICS` | `0` | Set to `1` for 300-pulse scheduler and callback analytics. |

The wall-clock budget is intended to be the binding limit. Setting the callback
cap low enough that pulses end well inside the time budget starves the wheel and
builds a deferred backlog. A zero budget or callback cap disables that one
limit; zeroing both makes the scheduler intentionally unbounded and emits a
warning. Budget and callback values are limited to `0..1000000`, and boolean
switches to `0..1`; invalid values fall back to their defaults. See
[ARCHITECTURE.md](../reference/ARCHITECTURE.md#event-wheel).

## Precedence and verification

1. The launching process environment has precedence over `.env`.
2. `.env` values are loaded from the server's data directory, normally the
   repository root or the directory supplied with `-d`.
3. Missing values required by the selected persistence mode fail closed; no database
   credentials or names have compiled defaults.
4. The resolved database target must be present in `DB_ALLOWED_TARGETS` before
   a connection is attempted. Logs report validation categories without
   printing credentials or target values.
5. Boot verifies the connection and complete schema/migration contract before lookup
   publication, UID reservation, workers, listeners, or gameplay.

A configuration change generally requires a restart. Database credentials and
Redis settings are read before normal gameplay initialization; creation flags
are consulted when their menus or validation paths run, but restarting is the
simplest way to avoid stale process state.

See [README.md](../../README.md) for initial database creation,
[DATABASE.md](../reference/DATABASE.md) for schema and migration procedures, and
[ARCHITECTURE.md](../reference/ARCHITECTURE.md) for command-line and listener details.
