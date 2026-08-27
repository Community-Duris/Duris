# Configuration

Duris reads environment settings from `.env` in the data directory during
startup. The parser accepts one `KEY=VALUE` assignment per line; blank lines and
lines beginning with `#` are ignored. Values are not shell-expanded or
quote-aware, so do not wrap values in quotes. Existing process environment
variables are preserved because `.env` only supplies variables that are not
already set.

Start from [`.env.example`](../.env.example):

```bash
cp .env.example .env
chmod 600 .env
```

Keep `.env` local and never commit passwords, HMAC secrets, or production
connection details. The server checks metadata before reading: `.env` must be
a regular file owned by the effective server user and must grant no permission
beyond owner read/write (`0600`).

## Database

| Variable | Requirement | Meaning |
| --- | --- | --- |
| `ENVIRONMENT` | Required: `local` or `production` | Runtime trust role. |
| `DB_HOST` | Required | MySQL/MariaDB host. |
| `DB_PORT` | Optional; `1`-`65535` | Database TCP port; the client default applies when omitted. |
| `DB_USER` | Required | Database account. |
| `DB_PASSWD` | Required | Database password. |
| `DB_NAME` | Required | Requested database name. |
| `DB_ALLOWED_TARGETS` | Required | Comma-separated exact `host/database` pairs; the resolved pair must match. |
| `DB_SOCKET` | Optional, local role only | Protected local Unix socket used instead of remote transport. |
| `DB_TLS` | Required as `TRUE` for non-loopback hosts | Enforce encrypted database transport. |
| `DB_SSL_CA` | Required for non-loopback hosts | Regular CA file used to verify the database server certificate. |

The server selects the database through the listen port as a final safety
check. Port `7777` is the production default; on any other port an explicitly
production-like name (`duris` or `duris_prod`) is redirected to `duris_dev`.
Use a separate database account and a non-`7777` port for development. This
redirect does not make a production credential safe to reuse locally.

Every connection has bounded connect/read/write deadlines, disables automatic
reconnect, and must establish the same verified session contract: `utf8mb4`,
UTC, READ COMMITTED, and strict transactional SQL mode. Loopback TCP and an
explicit local socket are treated as protected local transport. Any other host
requires enforced TLS, CA verification, and a negotiated cipher.

## Redis

Redis is optional. It is disabled unless `REDIS=TRUE` (case-insensitive).
When enabled, it stores floor-drop recovery data, object UID state, caches, and
optional immutable world-recovery generations. Player dirty state remains local to the
revisioned player-save pipeline and typed journal.

| Variable | Default | Accepted values / range | Meaning |
| --- | --- | --- | --- |
| `REDIS` | disabled | `TRUE` enables it | Enable Redis integration. |
| `REDIS_HOST` | `127.0.0.1` | hostname or IP | Redis host. |
| `REDIS_PORT` | `6379` | `1`-`65535` | Redis TCP port; invalid values fall back to `6379`. |
| `REDIS_WORLD_STATE` | disabled | `TRUE` enables it | Enable bounded capture and background publication of crash-recovery world generations. |
| `REDIS_WORLD_STATE_INTERVAL` | `10` seconds | `5`-`300` | Snapshot interval when world-state recovery is enabled. |
| `REDIS_WORLD_STATE_MAX_AGE` | `300` seconds | `60`-`3600` | Maximum snapshot age accepted during recovery. |

World recovery is intentionally separate from player saves and reconstructible caches.
The publisher writes an immutable sequence-keyed payload, then atomically advances the
current pointer and diagnostic metadata. Boot accepts only a complete, non-expired
generation whose schema, sequence, size, and checksum validate. A failed or stale
generation is ignored and the server continues with a normal boot.

For a local development session, the following is a reasonable starting point:

```text
REDIS=TRUE
REDIS_HOST=127.0.0.1
REDIS_PORT=6379
REDIS_WORLD_STATE=TRUE
```

Stop the server before clearing Redis state. `scripts/clear-redis.sh` runs a
bare `redis-cli FLUSHDB`, so it uses the CLI's default endpoint and database
(normally `127.0.0.1:6379`, database `0`) rather than reading `.env`. Use the
script only for that exact dedicated local instance; otherwise connect and
inspect the intended Redis database explicitly.

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
| `DURISWEB_SECRET` | Shared secret for DurisWeb HMAC authentication. The client signature is a 64-character SHA-256 hex digest for the current Unix minute; the server accepts the adjacent minute on either side to tolerate clock skew. Keep this secret private and use the same value in the backend. |
| `DURIS_TRUSTED_PROXY_IP` | One immediate proxy IP address whose `X-Forwarded-For` header may be trusted for WebSocket and telnet connections. If unset, forwarded addresses are ignored. This is an address allow-list, not a CIDR range. |

WebSocket listens on `4050`. Plain telnet defaults to `7777` and TLS telnet to
`7778`; a custom plain-telnet port uses the following port for TLS. Configure a
real `duris.crt` and `duris.key` in the repository root for networked TLS. The
operator key must be owner-controlled and mode `0600` or stricter. The tracked
self-signed certificate is accepted only with the explicit local role and an
exact loopback listener.

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

The wall-clock budget is intended to be the binding limit. Setting the callback
cap low enough that pulses end well inside the time budget starves the wheel and
builds a deferred backlog; see [ARCHITECTURE.md](ARCHITECTURE.md#event-wheel).

## Precedence and verification

1. The launching process environment has precedence over `.env`.
2. `.env` values are loaded from the server's data directory, normally the
   repository root or the directory supplied with `-d`.
3. Missing required values fail closed; no database credentials or names have
   compiled defaults.
4. The resolved database target must be present in `DB_ALLOWED_TARGETS` before
   a connection is attempted. Logs report validation categories without
   printing credentials or target values.

A configuration change generally requires a restart. Database credentials and
Redis settings are read before normal gameplay initialization; creation flags
are consulted when their menus or validation paths run, but restarting is the
simplest way to avoid stale process state.

See [README.md](../README.md) for initial database creation,
[DATABASE.md](DATABASE.md) for schema and migration procedures, and
[ARCHITECTURE.md](ARCHITECTURE.md) for command-line and listener details.
