# Configuration

When started directly, Duris reads `.env` in the server's data directory during
startup. Its parser accepts one literal `KEY=VALUE` assignment per line; blank
lines and lines beginning with `#` are ignored. Values are not shell-expanded or
quote-aware, and quotation marks become part of the value. Existing process
environment variables are preserved because this loader only supplies variables
that are not already set.

`scripts/cycle_mud.sh` first changes to the repository root and sources its `.env`
as Bash with automatic export enabled. File assignments can replace inherited
environment values, and Bash interprets quoting and shell syntax. The server's
loader then preserves the resulting environment. Use unquoted assignments whose
values work with both parsers. For values that require quoting, set them in the
launching process environment and omit those keys from `.env`.

Start from [`.env.example`](../../.env.example):

```bash
cp .env.example .env
chmod 600 .env
```

Keep `.env` local and never commit passwords, HMAC secrets, or production
connection details. The server checks metadata before reading: `.env` must be
a regular file owned by the effective server user and must grant no permission
beyond owner read/write (`0600`).

## Output profiles

`DURIS_OUTPUT_PROFILES_FILE` optionally names a versioned JSON configuration of
channel profiles, dictionaries, and foreground recipes. An absolute path is
recommended; relative paths resolve from the server's data directory. Startup
loads the file once before gameplay; an absent setting uses Preserve, and an invalid
initial file logs a diagnostic and uses the same fallback. Explicit reload APIs
publish complete validated snapshots and retain the previous snapshot on failure.
Message rendering performs no configuration I/O. See the
[profile guide](../guides/OUTPUT_PROFILES.md) and
[versioned sample](../examples/output-profiles-v1.json) for the schema and bounds.

Loading a profile does not opt existing callers into styling. Callers still need
an explicit output context. Animation recipes and player preferences are
implemented for adopted contexts. `toggle color` provides channel choices,
previews, and resets; `toggle color motion off|on` controls decorative animation.
With motion off, animated output uses a stable frame.

## Telemetry runtime

Telemetry is opt-in at the server boundary. Startup calls
`telemetry_runtime_init(telemetry_runtime_options_from_environment())` before
gameplay. Environment parsing and reviewed property-catalog loading perform no
database access; the enabled transport worker owns its private SQL repository
connection. `TELEMETRY_ENABLED` must be an accepted true value (`TRUE`, `1`, `YES`,
or `ON`) before any telemetry worker starts. Unset, false, malformed, or
out-of-range values fail closed to the default-disabled snapshot.
A client-free (`__NO_MYSQL__`) build always returns `flatfile_disabled` and
remains disabled, even when the opt-in variable is true.

| Variable | Default when opted in | Accepted values / range | Meaning |
| --- | --- | --- | --- |
| `TELEMETRY_ENABLED` | disabled | `TRUE`/`1`/`YES`/`ON` enable; `FALSE`/`0`/`NO`/`OFF` disable | Explicitly opt into the SQL telemetry writer. |
| `TELEMETRY_BACKEND` | `sql` on SQL builds | `sql`, `flatfile_disabled`, `disabled`, `off` | Select SQL or the deliberate disabled backend; flat-file is not an observational sink. |
| `TELEMETRY_PROPERTY_CATALOG_FILE` | required for enabled SQL | owner-readable reviewed catalog path | Full effective-property digest to stable property-version mapping; missing or invalid input disables telemetry only. |
| `TELEMETRY_CONFIG_REVISION` | `1` | positive `uint64` | Reviewed effective configuration revision floor. |
| `TELEMETRY_BUILD_VERSION`, `TELEMETRY_CONTENT_VERSION` | `1` | positive `uint32` | Versioned inputs included in the config identity. |
| `TELEMETRY_CLASSIFIER_VERSION`, `TELEMETRY_POLICY_VERSION` | `1` | positive `uint32` | Classifier/policy identities attached to observations. |
| `TELEMETRY_SEASON_ID`, `TELEMETRY_ENVIRONMENT_ID` | `1` | positive `uint64` | Scope identity carried by session records. |
| `TELEMETRY_INTERVAL_USEC` | `60000000` (60 seconds) | `1`-`3600000000` | Activity interval cadence. |
| `TELEMETRY_CHECKPOINT_INTERVAL_USEC` | `300000000` (5 minutes) | `1`-`3600000000` | Cumulative session checkpoint cadence. |
| `TELEMETRY_ACTIVE_WINDOW_USEC` | `300000000` (5 minutes) | `1`-`3600000000` | Recent-evidence active window. |
| `TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE` | `8` | `1`-`64` | Context segment rate cap. |
| `TELEMETRY_PULSE_SLOT_COUNT` | `1` | `1`-`256` | Number of staggered pulse cohorts. |

The three `_USEC` ranges run from one microsecond through one hour. Boolean and
backend names are case-insensitive; an unset or empty `TELEMETRY_ENABLED` disables
telemetry.

The catalog is a reviewed, preloaded text file. Blank lines and lines beginning
with `#` are ignored; every other line must contain exactly four whitespace-
separated fields:

```text
<64 lowercase-or-uppercase hex digest> <property_version> <stable_namespace> <stable_catalog_version>
```

All three numeric fields are nonzero `uint32` values. Full digests must be
unique, and a `property_version` may not be reused for another full digest.
Unknown effective digests are refused; the runtime never derives a usable
property identity from a digest prefix, increments a process-local counter, or
uses `TELEMETRY_PROPERTY_VERSION`. The loader rejects malformed, duplicate, or
collision entries before the worker starts. After bootstrap, capture/reload,
pulse, and action paths perform no catalog file I/O or SQL query. The effective
reader calls the game's normal `get_property()` conversion with each registry
fallback, so a property reload is observed only after `apply_properties()` has
rebuilt its cached consumers.

These values are copied into one immutable snapshot and fingerprinted before
capture. Changing the environment requires the normal server restart/config
review path. Capture and pulse remain fixed-value, bounded operations: they do
not query SQL, append a spool, or call `fsync` on the game thread. The transport
worker owns its private repository connection; DB-down operation stays degraded
and does not borrow gameplay persistence or reject saves/copyover.

### Telemetry shutdown request and final reap

Shutdown has two lifecycle steps. `telemetry_runtime_shutdown()` closes
telemetry admission, marks the runtime `stopping`, asks the worker to stop, and
waits only until the supplied monotonic deadline for the worker to report done.
It does not detach the worker, reset runtime/config state, or call repository
teardown. `accepted` means the worker reported done within that request window;
the worker handle and borrowed callback binding remain owned until the reap.
If a repository callback is still running at the deadline, the request returns
`stopping` and retains all state so the callback can finish safely.

`telemetry_runtime_final_reap()` is mandatory for an enabled runtime after the
request, including when the request returned `stopping` or the runtime clock
could not provide a deadline. It joins the worker before invoking repository
and transport teardown, and may block on an in-flight repository callback.
Call it from the off-game-thread process-lifetime shutdown path, before
`shutdown_mysql()` or process return; it is not a hard bounded shutdown step.

Telemetry handoffs were introduced in native copyover version 15 and remain in
the current version 17. The writer emits version 17; the reader supports versions
12-17 and reads telemetry framing for version 15 and later. Each preserved telnet
session has one bounded handoff entry. Save accounts through the handoff cut and
writes only value data; recover allocates a new process-local connection and
resumes the logical session. Versions 12-14 have no telemetry trailer. Older
formats and missing or rejected individual handoffs use an explicit absent
handoff, so the next observation marks an unclosed tail instead of inventing
continuity. Telemetry resume failure is logged and never rejects the game-state
copyover. An invalid telemetry header or truncated telemetry section rejects
copyover recovery because the following world section cannot be read safely.
Before writing candidate handoffs, synchronous copyover requests a worker-owned
flush and waits at most 250ms. It writes absent handoffs unless all admitted
records are acknowledged without permanent rejection. This wait never issues
SQL or stops/joins the worker on the game thread; a failed copyover can continue
using the same runtime.

## Persistence

`PERSISTENCE_MODE` selects one whole-server authority. It defaults to
`mariadb-primary`. The supported choices are `mariadb-primary` for a SQL-client
build and `flatfile-primary` for a client-free flat build with a private
`FLATFILE_STATE_DIR`. Names are case-sensitive. The legacy
`mariadb-primary-flatfile-fallback` token is recognized for a clear diagnostic but
rejected by the server; mixed per-operation authority transfer is not supported.

| Variable | Requirement | Meaning |
| --- | --- | --- |
| `PERSISTENCE_MODE` | Optional; defaults to `mariadb-primary` | Select the complete persistence authority; mixed per-write failover is not supported. |
| `FLATFILE_STATE_DIR` | Required by `flatfile-primary` | Absolute server-user-owned directory with mode `0700` or stricter. |
| BACKUP_POLICY_FILE | Required for pre-cycle and scheduled backups | Absolute owner-only approved JSON policy; see [BACKUPS.md](BACKUPS.md). |
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
| `PLAYER_SAVE_JOURNAL_DIR` | Required for the player-save pipeline, including mini mode | Absolute server-user-owned `0700` directory for revisioned player snapshots. |
| `CRITICAL_COMMAND_JOURNAL_DIR` | Required for the critical-command pipeline, including mini mode | Server-user-owned `0700` directory for non-coalescing critical commands; use an absolute path. |
| `MAINTENANCE_STATE_FILE` | Optional; `bin/server/maintenance-scheduler.state` | Durable scheduler cursor/completion state; parent directory must be server-user controlled. |

`scripts/cycle_mud.sh --check-config` checks the launcher's environment requirements
without starting the server or invoking its persistence-mode selector. The launcher
currently recognizes the unsupported `mariadb-primary-flatfile-fallback` token and
can report success when both SQL and flat-file settings are supplied; that result
does not establish support in the binary. Select one of the two supported authorities
above. Add `--production` to require `ENVIRONMENT=production` and the production-port
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
Production role requires the production port, `7777` unless `DURIS_PRODUCTION_PORT`
selects another; on any other port an explicitly production-like name (`duris` or
`duris_prod`) is redirected to `duris_dev` before the allow-list check. Use a separate
database account, target, and non-production port for development. The
redirect does not make a production credential safe to reuse locally.

Every connection has 10-second connect/read/write deadlines and disables automatic
reconnect. MySQL's client default keeps reconnect off without invoking its deprecated
`MYSQL_OPT_RECONNECT` option; MariaDB builds set the still-supported option to false
explicitly. Every connection must establish the same verified session contract:
`utf8mb4`, time zone `+00:00`, `READ-COMMITTED`, and `STRICT_TRANS_TABLES`,
`ERROR_FOR_DIVISION_BY_ZERO`, and `NO_ENGINE_SUBSTITUTION`. Loopback TCP and an
explicit local-role socket are treated as protected local transport. Any other host
requires enforced TLS, CA verification, and a negotiated cipher. MariaDB boot also requires a
supported MySQL 8.0 or MariaDB 10.11 normalized metadata fingerprint before lookup
publication, SQL UID reservation, and pool startup. SQL lifecycle recovery and
connection-activity bookkeeping occur before this full schema/fingerprint check.

Both journal settings are required for their respective persistence pipelines,
which are initialized in mini mode too. Initialization failure is logged and
reported to staff, and startup can continue with the affected pipeline unavailable;
a running server does not establish healthy saving or critical-command persistence.
The affected pipeline's operations fail closed.

Use absolute paths for both journals. The player-save pipeline rejects relative
paths; the critical-command initializer currently permits them, resolving from
the server's data directory. Directories must be owned by the server user and
mode `0700` or stricter; their files are permission checked, checksummed, size
bounded, and fail closed on corruption or quota exhaustion. Do not place either
directory under a shared or automatically cleaned temporary path.

## Redis

Redis is optional. It is disabled unless `REDIS=TRUE` (case-insensitive).
When enabled, Redis holds floor-drop recovery records, caches, presence state,
and optional immutable world-recovery generations. It carries presence and
donation events. Item UID allocation belongs to SQL or the native flat-file
allocator; recovery records carry item identities. Player dirty state remains
local to the revisioned player-save pipeline and typed journal.

The current Redis runtime requires a nonzero SQL season epoch at startup. The
client-free build returns zero for that epoch, so setting `REDIS=TRUE` does not
activate the current Redis runtime in a client-free flat-file build.

| Variable | Default | Accepted values / range | Meaning |
| --- | --- | --- | --- |
| `REDIS` | disabled | `TRUE` (case-insensitive) enables it | Enable Redis integration. |
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
| `REDIS_TLS` | `FALSE` in the server | `TRUE` or `FALSE`, case-insensitive in the server; exact uppercase for shell cleanup | Enables verified TLS for every TCP runtime and maintenance connection. Non-loopback production runtime endpoints require `TRUE`; destructive maintenance requires it for every non-loopback TCP target. Unix sockets require `FALSE`. |
| `REDIS_CA_CERT` | empty | Readable CA bundle | Required when Redis TLS is enabled and used for peer verification. |
| `REDIS_TLS_SERVER_NAME` | `REDIS_HOST` | Certificate DNS name | Optional runtime SNI and certificate-name override, useful when connecting by IP to a certificate issued for a DNS name. |
| `REDIS_ALLOWED_TARGETS` | none | Comma-separated exact `host:port/database` or `unix:/absolute/socket/database` values | Required destructive-maintenance allow-list. |
| `REDIS_WORLD_STATE` | disabled | `TRUE` (case-insensitive) enables it | Enable bounded capture and background publication of crash-recovery world generations. |
| `REDIS_WORLD_STATE_INTERVAL` | `10` seconds | `5`-`300` | Snapshot interval when world-state recovery is enabled. |
| `REDIS_WORLD_STATE_MAX_AGE` | `300` seconds | `60`-`3600` | Maximum snapshot age accepted during recovery. |
| `REDIS_WORLD_STATE_SECRET` | none | `32`-`256` bytes | Independent HMAC key required when world recovery is enabled. It authenticates the manifest and complete generation payload; do not reuse Redis, database, donation, or DurisWeb credentials. |
| `REDIS_WORLD_STATE_SECRET_PREVIOUS` | empty | `32`-`256` bytes | Optional previous recovery HMAC key accepted only for reading and cleanup during a bounded rotation window. New generations are always signed by the current key. |
| `REDIS_DONATION_SUBSCRIBER` | disabled | `TRUE` (case-insensitive) enables it | Subscribe to authenticated external donation notices. No polling job or subscriber connection exists by default. |
| `REDIS_DONATION_SECRET` | none | At least 32 bytes | Independent HMAC key required when the donation subscriber is enabled. Do not reuse a Redis, database, or DurisWeb secret. |

The server reads `REDIS`, `REDIS_WORLD_STATE`, and `REDIS_DONATION_SUBSCRIBER`
case-insensitively; values other than `TRUE` leave those features disabled.
For `REDIS_TLS`, unset or empty selects `FALSE`; any other nonempty value must
be `TRUE` or `FALSE`, case-insensitively. Invalid TLS values disable Redis
during configuration. The shell cleanup helper requires an explicit uppercase
`REDIS_TLS=TRUE` or `REDIS_TLS=FALSE`. Use uppercase values in a `.env` shared
by the server and maintenance tooling.

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

World publication uses a renewable 10-minute writer lease. The recovery worker
first stages the immutable generation in chunks of at most 1 MiB, using keys
qualified by sequence and upload token, with an expiry on each chunk. It then
uses one Lua compare-and-set to verify the writer token and expected prior
pointer and atomically publish the authenticated 120-byte manifest, advance
the current pointer and diagnostic metadata, consume the pre-capture floor
hash and index, and renew the lease. The current pointer selects the staged
generation; payload uploads occur in separate Redis commands. A stale writer
or stale prior pointer is rejected before the publication swap.
All of those keys use `<REDIS_NAMESPACE>:season:<epoch>:` with the active SQL season epoch captured at
boot. An old process can therefore write only its abandoned epoch after a reset; it cannot
create a snapshot visible to the new season.
Boot authenticates the generation manifest and verifies the generation's SHA-256
digest before accepting its schema, exact sequence, age, size, record framing,
completeness, and CRC32. It combines the generation with separately loaded binary
floor records, validates the full semantic graph, and batch-reconciles every item
marked `authority_required` against SQL before creating any entity. Reconstructible
world-pop objects without that marker are restored without inventing SQL custody,
while SQL-restored player corpses are excluded from generation capture.

The separate `WRF5` floor-delta records are not covered by the generation's HMAC and
have no independent HMAC. They must pass format, root-UID, hierarchy, duplicate,
and applicable SQL custody checks. Rejected recovery falls back to a full normal
zone boot. The rejection path does not consume the generation, but any remaining
generation artifacts can expire or be replaced or cleaned up later; diagnostic
retention is not guaranteed.

World generations are capped at 128 MiB and stored in at most 128 chunks of 1 MiB
each. The accepted floor object payload is capped at 16 MiB, and generation plus
floor object payload is capped at 128 MiB; the floor total excludes each record's
five-byte `WRF5:` prefix. These are accepted encoded-payload ceilings, not a limit
on peak process memory. Redis replies, decoded records, and recovery-planning
allocations add memory beyond those totals.

Generation reads issue separate `STRLEN` and `GET` commands, then validate the
received length. Floor `HMGET` pages are validated after receipt. Changed or
oversized replies can therefore already have been transferred before rejection.
Every manifest and chunk receives a TTL of at least one hour or four times
`REDIS_WORLD_STATE_MAX_AGE`, whichever is greater.

The generation reader and background publisher request a minimum 500 ms command
timeout, which is 500 ms with the current shared runtime settings. This applies
to individual commands, not the complete multi-command upload, and does not
change the shared runtime connection deadlines.

Ordinary shutdown does not consume the current generation. Any retained
artifacts remain subject to their TTL. After eligible non-pwipe, non-copyover-quiesced
shutdowns drain world and floor work, the fenced writer attempts to record an
expiring clean-shutdown marker for the current sequence. A completed drain means
the work has finished; it does not prove that the last capture or publication
succeeded. Marker failure is logged without cancelling teardown.

The next boot attempts to read and consume that marker once. A matching valid
generation is labelled `clean restart`; recovery without a matching marker is
labelled `crash` recovery. After successful materialization, boot attempts fenced
consumption of the exact current generation. Consumption failure is logged while
boot continues. Boot attempts to re-enable periodic publication for the new process.

Floor deltas use a separate background worker with at most eight queued jobs and
16 MiB of accounted value bytes. Each mutation batch holds at most 2,048
mutations, each submitted value is capped at 2 MiB, and keys are capped at
128 bytes. Recovery object trees contain at most 512 identity-preserving items;
larger or otherwise invalid trees fail capture rather than being truncated.
These worker-queue limits do not bound all pending gameplay storage or process
memory.

Worker transactions group at most 64 mutations. The grouping code targets
1 MiB of value bytes, but that is not a hard ceiling in the current implementation:
a larger first value is allowed, and unsigned subtraction can then admit further
values into the same group. See [WORLD_RECOVERY_FORMAT.md](../persistence/WORLD_RECOVERY_FORMAT.md)
for the detailed limits.

Before world capture, a successful ordered barrier confirms earlier floor work
and pauses later worker publication. The Lua publication commit clears the stable
pre-capture hash and index; successful and failed completions both resume later
work. The server can commit publication even if the reply is lost.

Gameplay copies bounded native records and enqueues owned batches; portable
encoding and Redis commands run on the worker. Capture and enqueue paths use
dynamic allocations. Preflight, rejected trees, expiry, other capture failures,
and a full retry buffer can synchronously write log files through `logit`.

World capture is a fuzzy crash-recovery snapshot timestamped when capture starts.
The game thread attempts at most 1,024 capture steps per pulse and checks a
two-millisecond deadline between steps; a step already in progress is not
interrupted when that deadline passes. At the start of each capture pulse, an
active capture aged five minutes or more is discarded with a failure completion;
later periodic requests can retry. This expiry check does not run within a
capture step or again when the capture is queued for publication.

NPC inventory and equipment are absent from the snapshot payload, and NPC gold
is captured as zero. Restore separately reconstructs applicable zone-defined
NPC items after materialization. Recovery items marked `authority_required`
require SQL custody reconciliation before any recovery entities are created.
`REDIS_WORLD_STATE_MAX_AGE` independently limits the age of the capture-start
timestamp when boot attempts recovery.

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
REDIS_ALLOWED_TARGETS=127.0.0.1:6379/0
REDIS_WORLD_STATE=TRUE
REDIS_WORLD_STATE_SECRET=local-development-only-world-state-hmac-change-before-shared-use
REDIS_DONATION_SUBSCRIBER=TRUE
REDIS_DONATION_SECRET=local-development-only-donation-hmac-change-before-shared-use
```

Those fixed values are local-only placeholders so the example brings up every Redis
worker. Replace both with distinct random secrets before connecting any shared or
externally reachable service.

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

Shared runtime Redis connections use a 250 ms connect timeout and a 100 ms
command timeout; generation reads and publication use the minimum 500 ms
command timeout described above. A cache failure may degrade a report.
A rejected publication compare-and-set leaves the current pointer and floor
hash/index unchanged by that script, but a timeout or lost reply can be
reported after Redis has committed the pointer swap and floor clear.
A reported publication failure therefore does not prove that the previous
generation or floor deltas were preserved. Redis failures do not authorize
a synchronous player save or journal deletion.

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

The unrestricted creation switches are intended for local testing and should
remain disabled on a live server:

| Variable | Enabled when | Effect |
| --- | --- | --- |
| `CREATION_ALL_RACES` | value equals `TRUE` | Adds normally unavailable races to character creation. They are selected by typing the race name. |
| `CREATION_ALL_CLASSES` | value equals `TRUE` | Adds every defined class to character creation, including restricted classes. |

The character-creation settings affect the menus and validation paths; they do
not change the underlying race/class data or make restricted choices suitable
for production.

Above the account-name prompt, the login screen shows one blinking line naming
each enabled mode: `STAGING`, `CHAOS`, `ALL-RACES` and `ALL-CLASSES`. It shows
nothing when none is enabled. `DURIS_STAGING` exists only for this banner; set it
to `TRUE` (case-insensitive) on a public staging server.

Chaos is a separate, deliberately selected server-wide ruleset. Its values are
read at process start and are case-sensitive:

| Variable | Default | Accepted values / effect |
| --- | --- | --- |
| `CHAOS_MUD` | disabled | Exact `TRUE` enables Chaos rules, including rebuilding characters at mortal level 56. Unset or `FALSE` disables the mode; any other value warns and disables it. |
| `CHAOS_EQ_PROFILE` | `standard` | `standard` selects the high-end starter equipment profile; `enhanceable` selects only equipment and class fundamentals admitted by the enhancement index. An invalid value warns and uses `standard`. |
| `CHAOS_STARTER_BONUSES` | `TRUE` when Chaos is enabled | Master switch for all optional new-character starter bonuses. Exact `FALSE` disables them; an invalid value warns and fails closed. |
| `CHAOS_STARTER_FRIGATE` | `TRUE` | Grants the existing dock reward as a free-frigate claim. Requires Chaos and the master starter switch. |
| `CHAOS_STARTER_EPIC_SKILLS` | `TRUE` | Grants eligible no-specialization epic skills to a new Chaos character. Requires Chaos and the master starter switch. |
| `CHAOS_STARTER_EPIC_POINTS` | `TRUE` | Grants 20,000 epic points through the critical epic ledger. Requires Chaos and the master starter switch. |
| `CHAOS_STARTER_BANK_PLATINUM` | `TRUE` | Grants 1,000,000 bank platinum through the critical currency ledger. Requires Chaos and the master starter switch. |
| `CHAOS_STARTER_MATERIALS` | `TRUE` | Adds the persistent Chaos craft pouch to the new-character equipment bag and enables its material-source behavior. Requires Chaos and the master starter switch. |
| `CHAOS_TEST_COMMANDS` | disabled | Exact `TRUE` exposes bounded integration-test helpers, but only when `ENVIRONMENT=local`. It does not enable Chaos mode. |

Every starter feature switch accepts only exact `TRUE` or `FALSE`; an invalid
feature value disables that feature. The complete equipment, durability, and
craft-pouch contract is in [CHAOS_MODE.md](../reference/CHAOS_MODE.md).

## WebSocket and proxy settings

| Variable | Meaning |
| --- | --- |
| `LISTEN_ADDRESS` | Numeric IPv4 or IPv6 address applied to telnet, TLS telnet, and WebSocket listeners. Use `127.0.0.1` or `::1` for local development. |
| `DURIS_DEV_PORT` | Plain-telnet port selected by `--dev` and `--minimal`. It defaults to `4000`; values must be decimal ports from 1 through 65535 and must not be the production port. |
| `DURIS_PRODUCTION_PORT` | Optional plain-telnet port for the production role (`--production`). It defaults to `7777`; set it only when a second production-role install shares a host. Values must be decimal ports from 1 through 65535. |
| `DURIS_TLS_PORT` | Optional independent TLS telnet port. It defaults to `7778`, or to the plain-telnet port plus one when a custom plain port is supplied. Values must be decimal ports from 1 through 65535 and must differ from the plain port. |
| `DURIS_WEBSOCKET_PORT` | WebSocket and HTTP health-listener port. It defaults to `4050`; values must be decimal ports from 1 through 65535. |
| `DURIS_WEBSOCKET_LISTEN_ADDRESS` | WebSocket-only numeric listener address; defaults to `LISTEN_ADDRESS`, and to `127.0.0.1` when neither is set. Production requires exact loopback so a local TLS reverse proxy owns the public endpoint. |
| `DURIS_WEBSOCKET_ALLOWED_ORIGINS` | Exact comma-separated browser `Origin` allow-list. Required in production; non-browser service connections may omit `Origin`. |
| `DURISWEB_SECRET` | Current shared key for one-time DurisWeb challenge-response authentication. Production requires at least 32 characters and rejects the public example placeholder. See the DurisWeb API reference. |
| `DURISWEB_SECRET_PREVIOUS` | Optional previous service key accepted during a bounded zero-downtime rotation. In production it must be empty or at least 32 characters and non-placeholder; remove it after every backend has switched. |
| `DURISWEB_PRIVATE_PRESENCE` | Exact `TRUE` opts the authenticated backend into account names, IP addresses, client metadata, and invisible staff presence. The default WebSocket and Redis presence feeds omit them. |
| `DURIS_TRUSTED_PROXY_IP` | One numeric IPv4 or IPv6 address matched against the immediate socket peer for WebSocket forwarding. An unset or invalid value, or a nonmatching peer, leaves forwarded addresses untrusted. This accepts one address, not a list or CIDR range. |

For a trusted WebSocket peer, the HTTP handshake accepts the validated first
address in `X-Forwarded-For`. The WebSocket accept path also invokes a PROXY
protocol v1 parser for that trusted peer. Plain and TLS telnet do not invoke
that parser or read HTTP forwarding headers; they retain the immediate socket
peer address.

### DurisWeb hook toggles

`lib/duris.properties` carries one key per MUD-gated DurisWeb hook. Values are
floats; `>= 0.5` is enabled, and a missing key defaults to enabled. Change them
at runtime with `properties set <key> <value>`; no restart is required, and the
MUD pushes the new state to connected DurisWeb peers. That in-game command is
in-memory until `properties save`. The authenticated
`durisweb_hook_set` service command used by the website console persists its
exact-whitelisted property atomically before acknowledging it.

| Property | Gates |
|----------|-------|
| `durisweb.hook.auction_new` | New auction broadcasts |
| `durisweb.hook.auction_bid` | Auction bid broadcasts |
| `durisweb.hook.auction_close` | Auction close broadcasts |
| `durisweb.hook.player_presence` | Player login and logout broadcasts (both) |
| `durisweb.hook.mud_shutdown` | Shutdown and crash notifications |
| `durisweb.hook.wholist` | Wholist responses to `request_wholist` |
| `durisweb.hook.admin_delete_character` | Administrative character deletion |
| `durisweb.hook.donation_delivery` | Applying donation events from Redis |

Ids are shared with the DurisWeb repository and defined at
`backend/src/hooks/registry.ts`. `connection_log` has no MUD property: it gates
DurisWeb's ingestion, not the MUD's `LOG_COMM` operational logging. The other
website-only ids (`flag_parsing`, `guild_parsing`, `zone_builder_parsing`, and
`process_control`) likewise have no MUD property; `terminal` is always-on and
controlled only by its permission and live-session checks.

WebSocket and `GET /health` listen on `DURIS_WEBSOCKET_PORT` (default `4050`).
In production, the WebSocket listener must use loopback, the trusted proxy and
allowed origins must be configured, and the local reverse proxy must terminate
TLS before forwarding to this plaintext listener. The server refuses to create
the production listener when those controls are absent.
The health response reports only process and selected-persistence readiness.
MariaDB mode reads in-memory pool state, and flat-file mode reports ready only
after its private authority passes startup validation; neither path performs a
blocking backing-store query. Plain telnet defaults to `7777` and TLS telnet to
`7778`; a custom plain-telnet port uses the following port for TLS unless
`DURIS_TLS_PORT` provides an independent port. Configure a real `duris.crt` and
`duris.key` in the repository root for networked TLS. The operator key must be
owner-controlled and mode `0600` or stricter. The tracked self-signed key was removed;
run `./scripts/generate_localhost_cert.sh` to create an ignored machine-local fallback.
That fallback is accepted only with the explicit local role and an exact loopback
listener, and its key must also be owner-controlled and mode `0600` or stricter.

The WebSocket command table carries `request_reset` and `complete_reset` for
player connections. Both first reject DurisWeb service connections with an
`auth` failure. `request_reset` then checks the shared registration rate limits;
`complete_reset` checks the shared login rate limits. These checks cover the
descriptor and client address and run before request fields are validated.
A rate-limit rejection returns an account `error`: `Too many requests; try
again later` for requests, or `Too many attempts; try again later` for completion.

`request_reset` takes `{"account": "<name>"}`. After the checks above, disabled
mail returns an unavailable `error`. With mail enabled, the usual reply is
`{"type": "account", "action": "reset_requested"}`, whether or not a message
was queued. Missing or non-string account fields and names outside 3-20 bytes
also receive that reply. The separate mail-request address window applies
after account-name validation; valid names for unknown or unreadable accounts
consume it like accounts without email. Reaching that limit returns an `error`
asking the client to wait 10 minutes.

`complete_reset` takes `{"account": "<name>", "code": "<32 hex digits; dashes,
spaces, and letter case are ignored>", "newPassword": "<at least 6 characters>"}`.
After service and rate checks, missing or non-string fields return
`Missing reset fields`; a short password returns `Password must be at least
6 characters` before the code is checked. Code-related failures use the single
`Invalid or expired reset code` error. Completion does not check `MAIL_ENABLED`;
with mail disabled at boot, requests that reach code validation have no issued
code to accept and receive that uniform code error.

An accepted code starts asynchronous password work. Submission can return
`Password service is busy; try again later`; later failures can return
`Failed to hash password` or `Failed to save password change`. Success returns
`{"type": "account", "action": "reset_completed"}`; the client then issues an
ordinary `login`. Echo control has no meaning on this transport: the client
hides the password field and renders the uniform meaning that a code may have
been sent, with at most one code mailed per account every 10 minutes.

## Account recovery mail

A player who has an email address on file can reset a forgotten account password by
typing `?` at the telnet password prompt, or through the WebSocket `request_reset` and
`complete_reset` commands above. The server mails a one-time code through libcurl SMTP
from one bounded worker thread. The feature is off unless `MAIL_ENABLED=TRUE`; while it is
off the password prompt is unchanged and `?` answers one not-available line.

| Variable | Requirement | Meaning |
| --- | --- | --- |
| `MAIL_ENABLED` | Optional; `TRUE` or `FALSE` (case-insensitive), default `FALSE` | Enable switch. Unset, empty, or `FALSE` disables password reset by email; any other value is rejected. |
| `MAIL_HOST` | Required when `MAIL_ENABLED=TRUE` | SMTP relay host name or address, 1-253 bytes, accepted by libcurl's URL parser as a bare host (no `/`, `@`, `?`, `#`, or whitespace). Loopback means `localhost`, `127.0.0.1`, or `::1`. |
| `MAIL_PORT` | Optional; `1`-`65535`, default `587` | SMTP TCP port. `465` selects implicit TLS (`smtps://`); every other port uses `smtp://` with STARTTLS when `MAIL_TLS` is `TRUE`. |
| `MAIL_TLS` | Optional; `TRUE` or `FALSE` (case-insensitive), default `TRUE` | `TRUE` requires TLS for the whole session (a relay that refuses STARTTLS fails the send; there is no opportunistic mode). `FALSE` is accepted only for a loopback `MAIL_HOST` with no credentials. |
| `MAIL_USERNAME` | Optional; 1-255 bytes | SMTP AUTH user. Must be set together with `MAIL_PASSWORD`, and credentials require `MAIL_TLS=TRUE`. |
| `MAIL_PASSWORD` | Optional; 1-255 bytes | SMTP AUTH password. Read once at boot into the worker's private snapshot; never logged; no compiled default exists and the source contracts forbid one. |
| `MAIL_FROM` | Required when `MAIL_ENABLED=TRUE` | Envelope sender and `From:` header. A plain address that passes both the structural mail check and the account-layer email validation; the text after `@` forms the `Message-ID` domain. |

Validation is fail-closed by category: a rejected or incomplete setting disables the
feature for the whole run, the server boots normally, and `logs/log/status` names the
offending key, never its value. Certificate verification uses the system CA bundle and
cannot be disabled; there is no CA override and no verification switch.

Shell safety: the launcher and direct-server loading rules above also apply to
`MAIL_*` settings. For values supplied through `.env`, use only shell-safe
characters -- no spaces, quotes, `$`, backticks, `;`, `#`, or `!`. A relay application
password is the intended shape; the server cannot detect a value that Bash has
already reinterpreted. Supply values that require quoting through the launching
process environment and omit their assignments from `.env`.

The controls are compile-time constants in `src/account/account_recovery.h` and are
deliberately not environment-tunable: a code lives 15 minutes (`ACCOUNT_RECOVERY_TTL_SEC`),
at most one code is mailed per account every 10 minutes (`ACCOUNT_RECOVERY_COOLDOWN_SEC`),
a code dies after 5 wrong guesses (`ACCOUNT_RECOVERY_MAX_TOKEN_ATTEMPTS`), a telnet connection is dropped after 5 code attempts
(`ACCOUNT_RECOVERY_MAX_DESCRIPTOR_ATTEMPTS`; a WebSocket connection is instead refused with the
uniform code error until it reconnects), and one client address may
request 5 codes per 10 minutes (`ACCOUNT_RECOVERY_HOST_MAX_REQUESTS` over
`ACCOUNT_RECOVERY_HOST_WINDOW_SEC`; IPv6 clients are keyed by their /64 prefix). The mail
queue holds 256 messages and each send is bounded to 10 s connect / 20 s total with no
retry.

The per-address window uses the effective client address stored on the
descriptor. WebSocket can obtain that address through the trusted forwarding
paths above. Plain and TLS telnet retain the immediate socket peer address, so
players behind one telnet proxy share a single five-per-ten-minute reset-request
budget. Supplying PROXY protocol does not give those telnet listeners a separate
client address in the current implementation. Such a deployment needs a reviewed
change to `ACCOUNT_RECOVERY_HOST_MAX_REQUESTS` or acceptance of the shared limit.

Copyover and restart contract: codes, cooldowns, and queued mail live only in process
memory. A copyover or restart discards them, the player-facing text says so, and the
player simply requests a new code. Nothing is persisted, so there is no table, file, or
Redis key to migrate or clean. Operational log lines and the shutdown timing tail are
described in [RUNBOOK.md](RUNBOOK.md#logs) and
[RUNBOOK.md](RUNBOOK.md#restart-and-crash-recovery).

## Diagnostics

Diagnostic switches are opt-in and can be noisy. Their read points differ, and
several helpers cache the first value they observe. Set them before startup,
use them only while investigating a specific issue, and restart the server
after changing their environment values.

| Variable | Value | Output / scope | Read point |
| --- | --- | --- | --- |
| `SQL_TRACE` | any non-empty value except `0`, `false`, or `off` | Metadata-only SQL execution events in the normal logs. | First SQL trace check; cached. |
| `GET_TRACE` | any non-empty value except `0`, `false`, or `off` | Debug logging for object pickup paths. | First pickup trace check; cached. |
| `DURIS_ZONE_RESET_TRACE` | positive integer | Zone-reset tracing. | First zone-reset trace check; cached. |
| `DURIS_CORPSE_TRACE` | any non-empty value except `0` | Corpse decay and dracolich lifecycle tracing. | The necromancy helper caches its first check; each decay callback reads the environment again. |
| `DURIS_ACCEPT_DEBUG` | variable present, including an empty value | Connection-accept debug counters. | Game-loop entry. |

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

These settings are read by `nevent_config_limit()` through accessors that cache
each value on first use. Configure them before startup and restart the server
after changing them.

| Variable | Default | Effect |
| --- | --- | --- |
| `DURIS_NEVENT_BUDGET_USEC` | `25000` (25 ms) | Cooperative wall-clock budget for event processing per pulse. |
| `DURIS_NEVENT_MAX_CALLBACKS` | `4000` | Callback count cap per pulse. |
| `DURIS_NEVENT_CATCHUP_MAX_EXTENSION_USEC` | `5000` | Maximum time-budget extension while repaying deferred work. |
| `DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS` | `4000` | Maximum callback-cap extension while repaying deferred work. |
| `DURIS_NEVENT_PLAYER_PRIORITY` | `1` | Set to `0` to disable player-timed priority. |
| `DURIS_NEVENT_TRACE_PLAYER` | `0` | Set to `1` for per-player deadline timing logs. |
| `DURIS_NEVENT_ANALYTICS` | `0` | Set to `1` for 300-pulse scheduler and callback analytics. |

The time budget is cooperative. The scheduler checks elapsed time after each
completed callback and periodically while scanning future work. It does not
interrupt a callback that runs past the budget. After budget exhaustion,
remaining due callbacks can be deferred to a later pulse while retaining
their original deadlines.

A low callback cap can end pulses before the time budget is spent and build
a deferred backlog under load. A zero budget or callback cap disables that
one limit; zeroing both makes the scheduler intentionally unbounded and emits
a warning. Budget and callback values are limited to `0..1000000`, and boolean
switches to `0..1`; invalid values fall back to their defaults. See
[ARCHITECTURE.md](../reference/ARCHITECTURE.md#event-wheel).

## Kingdom harvest node populations

`lib/duris.properties` sets the target population for each harvest region:

| Property | Shipped default |
| --- | --- |
| `kingdom.nodes.map.total` | `80.000` |
| `kingdom.nodes.ud.total` | `60.000` |

Each target counts all resource kinds together. The server clamps each target
between 0 and 500 nodes per region. These properties override the matching
compiled defaults; the population figures in `help kingdoms` describe the
shipped configuration.

## Precedence and verification

1. A direct server start preserves the launching process environment. The
   `scripts/cycle_mud.sh` launcher first sources the repository-root `.env` as
   Bash, so its assignments can replace inherited values before the server starts.
2. The server loads literal `.env` assignments from its data directory, normally
   the repository root or the directory supplied with `-d`, and preserves values
   already in its environment, including those exported by the launcher.
3. The server validates the selected persistence mode against the build.
   `mariadb-primary` requires a MariaDB client build; `flatfile-primary`
   requires the client-free build. The mixed-fallback token is refused.
4. In `mariadb-primary`, required database values must be present, with no
   compiled credentials or database-name defaults. The resolved target must
   match `DB_ALLOWED_TARGETS` before connecting. Database validation logs
   report categories without printing credentials or target values.
5. MariaDB startup acquires runtime authority, performs lifecycle recovery
   and connection-activity bookkeeping, then verifies the complete schema and
   migration contract. That verification precedes active-season loading,
   lookup publication, SQL UID reservation, pool startup, later persistence
   workers, listeners, and gameplay. The persistence log worker starts before
   SQL initialization.
6. In `flatfile-primary`, startup validates or provisions the private absolute
   `FLATFILE_STATE_DIR` and its required directories, resets flat-file IP
   activity, reserves a flat-file UID range, and hydrates the system item-owner
   revision. These checks use native flat-file authority; startup skips the
   SQL connection, allow-list, and schema path.

A configuration change generally requires a restart. Database credentials and
Redis settings are read before normal gameplay initialization; creation flags
are consulted when their menus or validation paths run, but restarting is the
simplest way to avoid stale process state.

See [README.md](../../README.md) for initial database creation,
[DATABASE.md](../reference/DATABASE.md) for schema and migration procedures, and
[ARCHITECTURE.md](../reference/ARCHITECTURE.md) for command-line and listener details.
