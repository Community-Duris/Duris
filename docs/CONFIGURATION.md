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
```

Keep `.env` local and never commit passwords, HMAC secrets, or production
connection details.

## Database

| Variable | Default | Meaning |
| --- | --- | --- |
| `DB_HOST` | `localhost` | MySQL/MariaDB host. |
| `DB_PORT` | client default | Database TCP port; normally `3306`. |
| `DB_USER` | `duris` | Database account. |
| `DB_PASSWD` | build default | Database password. |
| `DB_NAME` | `duris_dev` in `TEST_MUD`, otherwise `duris` | Requested database name. |

The server selects the database through the listen port as a final safety
check. Port `7777` is the production default; on any other port an explicitly
production-like name (`duris` or `duris_prod`) is redirected to `duris_dev`.
Use a separate database account and a non-`7777` port for development. This
redirect does not make a production credential safe to reuse locally.

The `ENVIRONMENT` value in `.env.example` is informational; the server does
not use it to select behavior or protect a database. Treat the port, database
name, and credentials as the real safety controls.

## Redis

Redis is optional. It is disabled unless `REDIS=TRUE` (case-insensitive).
When enabled, it stores dirty-player queues, floor-drop recovery data, object
UID state, and optional world-state snapshots.

| Variable | Default | Accepted values / range | Meaning |
| --- | --- | --- | --- |
| `REDIS` | disabled | `TRUE` enables it | Enable Redis integration. |
| `REDIS_HOST` | `127.0.0.1` | hostname or IP | Redis host. |
| `REDIS_PORT` | `6379` | `1`-`65535` | Redis TCP port; invalid values fall back to `6379`. |
| `REDIS_WORLD_STATE` | disabled | `TRUE` enables it | Save a crash-recovery world snapshot in a child process. |
| `REDIS_WORLD_STATE_INTERVAL` | `10` seconds | `5`-`300` | Snapshot interval when world-state recovery is enabled. |
| `REDIS_WORLD_STATE_MAX_AGE` | `300` seconds | `60`-`3600` | Maximum snapshot age accepted during recovery. |

World-state recovery is intentionally separate from ordinary dirty saves. A
snapshot is marked valid only after it has been written; a valid, non-expired
snapshot is restored once at boot and then cleared. A failed or stale snapshot
is ignored and the server continues with a normal boot.

For a local development session, the following is a reasonable starting point:

```text
REDIS=TRUE
REDIS_HOST=127.0.0.1
REDIS_PORT=6379
REDIS_WORLD_STATE=TRUE
```

Stop the server before clearing Redis state. `scripts/clear-redis.sh` runs
`FLUSHDB` against the configured/default Redis database and is destructive to
other data in that Redis database; use a dedicated local database or inspect
its contents first.

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
| `DURISWEB_SECRET` | Shared secret for DurisWeb HMAC authentication. The client signature is a 64-character SHA-256 hex digest for the current Unix minute; the server accepts the adjacent minute on either side to tolerate clock skew. Keep this secret private and use the same value in the backend. |
| `DURIS_TRUSTED_PROXY_IP` | One immediate proxy IP address whose `X-Forwarded-For` header may be trusted for WebSocket and telnet connections. If unset, forwarded addresses are ignored. This is an address allow-list, not a CIDR range. |

WebSocket listens on `4050`. Plain telnet defaults to `7777` and TLS telnet to
`7778`; a custom plain-telnet port uses the following port for TLS. Configure a
real `duris.crt` and `duris.key` in the repository root for networked TLS; the
tracked self-signed certificate is suitable only for local testing.

## Diagnostics

Diagnostic switches are opt-in and are read once when the relevant subsystem
initializes. They can be noisy, so enable them only while investigating a
specific issue and restart the server after changing them.

| Variable | Value | Output / scope |
| --- | --- | --- |
| `SQL_TRACE` | any non-empty value except `0`, `false`, or `off` | SQL trace output for database execution. |
| `GET_TRACE` | any non-empty value except `0`, `false`, or `off` | Debug logging for object pickup paths. |
| `DURIS_ZONE_RESET_TRACE` | positive integer | Zone-reset tracing. |
| `DURIS_CORPSE_TRACE` | any non-empty value except `0` | Corpse decay tracing. |
| `DURIS_ACCEPT_DEBUG` | variable present, including an empty value | Connection-accept debug counters. |

Use the normal log files described in [RUNBOOK.md](RUNBOOK.md) and remove
these switches from `.env` when the investigation ends.

## Precedence and verification

1. The launching process environment has precedence over `.env`.
2. `.env` values are loaded from the server's data directory, normally the
   repository root or the directory supplied with `-d`.
3. Built-in defaults apply when neither source provides a value.
4. The selected database and host/port are recorded in the status log during
   boot. Confirm them before allowing clients to connect.

A configuration change generally requires a restart. Database credentials and
Redis settings are read before normal gameplay initialization; creation flags
are consulted when their menus or validation paths run, but restarting is the
simplest way to avoid stale process state.

See [README.md](../README.md) for initial database creation,
[DATABASE.md](DATABASE.md) for schema and migration procedures, and
[ARCHITECTURE.md](ARCHITECTURE.md) for command-line and listener details.
