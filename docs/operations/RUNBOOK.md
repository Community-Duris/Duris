# Operations Runbook

Day-to-day operation of a DurisMUD instance. First-time setup is in
[README.md](../../README.md).

## Starting and stopping

```bash
./scripts/start_mud.sh     # preferred: systemd user service if installed,
                           # otherwise nohup cycle_mud.sh -> logs/duris-console.log
./scripts/cycle_mud.sh     # foreground supervised run (what start_mud wraps)
./scripts/cycle_mud.sh --dev   # development listener/build role on port 4000
./scripts/cycle_mud.sh --production  # require production role; configured port defaults to 7777
./scripts/cycle_mud.sh --check-config  # check launcher environment requirements without booting
```

Set `DURIS_DEV_PORT` to use a different development plain-telnet port; its
default is `4000`. `DURIS_PRODUCTION_PORT` selects the production-role port,
defaulting to `7777`, and the development port must differ from that configured
production port. Both launcher range checks allow values from `1` through
`65535`, while the server rejects positional plain-telnet ports at or below
`1024`. Use `1025..65535` for server startup.

`--check-config` validates the launcher's environment, role, port, and
mode-specific fields without connecting to a database or starting the server.
It creates `bin/server/history/` if needed and exits before the build-stamp,
schema, backup-policy, world-data, and listener checks. A successful check
therefore does not qualify a boot. The launcher can currently accept a fully
configured `mariadb-primary-flatfile-fallback` token even though the server
rejects it; use one of the two supported authorities described in
[CONFIGURATION.md](CONFIGURATION.md#persistence).

`cycle_mud.sh`:

- Anchors itself to the repository root and sources `.env` as Bash if present;
  matching file assignments can replace inherited environment values.
- Requires `ENVIRONMENT=local` or `production`. MariaDB launches also require
  `DB_HOST`, `DB_USER`, `DB_PASSWD`, `DB_NAME`, and `DB_ALLOWED_TARGETS`;
  `flatfile-primary` requires an absolute `FLATFILE_STATE_DIR`.
- Applies pending immutable migrations for a local MariaDB launch and verifies
  schema compatibility for both local and production MariaDB launches. Production
  migrations follow the separately approved procedure below. Flat-file launches
  skip these database tools and MySQL shutdown logging.
- Raises core dump limits (`ulimit -c unlimited`).
- In full-world mode, builds missing area helpers and regenerates `areas/world.*`
  on every cycle iteration. `--minimal` uses the tracked `areas_mini` files instead.
- Regenerates `lib/misc/event_names` from the active executable.
- Promotes `bin/server/dms_new` to `bin/server/dms` on initial startup and on later
  cycle iterations following exit code `53` or `57`. An ordinary exit-code `52`
  reboot does not promote a staged binary. The default history limit is five prior
  executables; set `DMS_BINARY_HISTORY_LIMIT` to change it. A `--production` launch
  requires a stamped `PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production` build.
- Before each server start attempt, rotates files directly under `logs/log/` into
  `logs/old-logs/<timestamp>/` and runs `scripts/backup_pfiles.sh`. Backup failure
  prevents that launch. `SKIP_PREBOOT_BACKUP=1` bypasses the pre-cycle backup step
  and supplies no backup evidence; an initial unprovisioned flat-file authority
  also has no generation to capture. Verify policy, scheduling, retention and
  isolated drills using [BACKUPS.md](BACKUPS.md).
- After the process exits, records the stop reason in console output and attempts
  a MySQL reboot record for MariaDB mode. Configured legacy email helpers may
  attempt notifications; successful process launch does not establish delivery.

### Exit codes interpreted by the cycle loop

| Code | Meaning | Restarted? |
|------|---------|------------|
| 0 | clean shutdown | no |
| 52 | reboot | yes |
| 53 | copyover reboot | yes |
| 54 | auto reboot | yes |
| 55 | pwipe shutdown | no |
| 56 | mud hung reboot | yes |
| 57 | auto reboot with copyover | yes |
| 139 | crash (SIGSEGV) | yes |
| other | unknown | yes |

Graceful shutdown from inside the game: immortal `shutdown` command
(`src/cmd/actwiz.c`). It writes `logs/shutdown_info.txt`
(`initiated_by|reason`), which `cycle_mud.sh` consumes for its reboot record
and then removes. Copyover (`copyover` command) execs a fresh binary while
keeping player connections alive via `copyover.dat`.

With the opt-in `--persistent-transport` launcher option, a parent keeps client
sockets, TLS and compression alive while only the world child is replaced.
Signal the parent or its watchdog once for lifecycle operations; do not signal
both executable PIDs. Non-playing or otherwise ineligible sessions veto planned
copyover. The watchdog still measures completed world loops. See the
[persistent transport guide](../network/PERSISTENT_TRANSPORT.md) for enabling the
mode, authenticated restoration, bounded queues and recovery after either process
fails. Default single-process copyover retains its all-or-nothing plain-Telnet
eligibility guard.

### Executable rollback and callback labels

Before an authorized clean build or rollout, identify the actual supervisor,
its system/user scope, and the executable behind the listener. A service name
or port alone does not establish environment role. Preserve the executable
bytes and matching backend/profile stamp outside `bin/`, together with any
runtime maintenance-scheduler state under that deletion boundary.
`make clean-all` removes `bin/`.

When copying `/proc/<pid>/exe`, dereference it into a regular file (for example,
`cp -L`), verify that the copy is not a symlink, and compare SHA-256 digests.
An archived proc symlink can hash correctly while the process lives and still
be useless after exit. Verify the running executable after promotion as well;
an ordinary in-game reboot is not proof that staged code was loaded.

Callback labels in `lib/misc/event_names` must match the loaded executable.
The launcher regenerates them with `nm --demangle`; an in-process code copyover
must also use a matching map. Stale addresses can invalidate callback-family
profiles even when the executable is correct. Follow the existing deployment
path and verify both identities before comparing profiles.

### Stopping a local instance

Use the same mode that started the instance:

```bash
# systemd user service, when installed
systemctl --user stop duris-mud.service

# foreground cycle_mud.sh session
Ctrl-C
```

For the fallback background mode started by `start_mud.sh`, use the in-game
immortal `shutdown` command when possible. The fallback does not create a PID
file; do not guess with a broad `kill` or `pkill` command. Check
`logs/duris-console.log`, the listener port, and the process command line before
stopping a specific local process. A normal shutdown lets the server drain its
persistence work and the supervisor attempt its reboot record. Log rotation
occurs before the next cycle's launch, not when the process stops.

### Production systemd service

The checked-in production unit is rendered from
`deploy/systemd/duris-mud-production.service.in`. It is a system service, so it starts
from `multi-user.target` without a login session or user lingering. It runs under the
checkout owner, sets `Restart=always`, disables systemd's restart-rate limit, waits ten
seconds between attempts, and invokes `cycle_mud.sh --production`. The launch flag
refuses `ENVIRONMENT=local`; the unit cannot silently publish a development role as
production. A deliberate `systemctl stop` remains stopped because systemd suppresses
restart jobs requested by the service manager.

`cycle_mud.sh` runs each game child under `scripts/game_loop_watchdog.py`. The
observer uses its own monotonic clock and a private Unix datagram channel. The
world thread increments a 64-bit counter only after the connection, session input,
output, event, recurring persistence, activity, combat, and pulse-reset phases all
finish. Worker threads and forked helpers cannot publish through the heartbeat
API. Fresh timestamps with an unchanged counter, a listening socket, a live PID,
and successful HTTP health probes do not renew the progress deadline.

This detects a callback spinning forever, an interruptible blocking I/O wait, a
deadlock, or a broken heartbeat channel that prevents another complete world loop.
It also bounds game startup, copyover preparation/recovery, and shutdown drains.
The observer starts **after** the launcher's schema verification, backup, and world
generation steps; those operational steps are outside the game-loop watchdog.
Launching `bin/server/dms` directly does not provide this external protection.

Deadlines can be configured in the existing protected `.env`. Values are finite,
positive seconds (fractional seconds are accepted), up to 86400, and are validated
by `cycle_mud.sh --check-config` before any game starts:

| Variable | Default | Deadline |
| --- | ---: | --- |
| `DURIS_WATCHDOG_STARTUP_SECONDS` | 300 | Spawn to first complete loop; also copyover exec boot to its first complete loop. |
| `DURIS_WATCHDOG_STALL_SECONDS` | 30 | Time since the most recent increasing completed-loop heartbeat. |
| `DURIS_WATCHDOG_COPYOVER_SECONDS` | 120 | Entire pre-exec save/drain/serialization operation. |
| `DURIS_WATCHDOG_SHUTDOWN_SECONDS` | 60 | Entire terminal save/drain/worker-join operation, including a forwarded stop signal. |
| `DURIS_WATCHDOG_ABORT_SECONDS` | 10 | Maximum wait after requesting a native core dump, before forced termination. |

Only an explicit lifecycle transition grants its bounded grace. Repeated boot or
drain records cannot renew it. A refused shutdown/copyover returns to the normal
running deadline; another drain allowance requires another completed loop.
Copyover keeps the same game PID, channel, and increasing counter across `exec`.
Initialization also disarms a virtual checkpoint timer inherited from an older
binary, since interval timers survive `exec`.
A cold restart creates a new child and private channel, so old records cannot
make the replacement healthy. If the observer dies, Linux's parent-death signal
kills its game child, including across copyover, rather than leaving a world
running without supervision.

On expiry, the observer writes a reason, phase, last counter, and completed-loop
age to the console/journal. A separate diagnostic process has a two-second budget
to write an owner-only `logs/watchdog/<timestamp>-<game-pid>.txt` snapshot of the
executable and process/thread `stat`, `status`, `wchan`, `syscall`, and kernel stack
where permitted. Missing permissions are recorded. Reports stay outside rotated
`logs/log/`; include them when investigating an incident and manage retention with
the deployment's existing log policy. The observer then sends `SIGABRT` to the game
for a native core dump, waits the configured abort budget, and sends `SIGKILL` if
needed. Core availability/location depend on the host's core-dump policy; neither
diagnostics nor termination calls the hung game thread. The game process group is
cleaned up before its replacement starts.

After a timeout on a game that previously completed a loop, the observer returns
`56` (`mud hung reboot`). The launcher records that result, waits its existing ten
seconds to prevent core flooding, and repeats its normal guarded boot, including
persistence/Redis recovery. Recovery can lose state that never reached existing
durable journals/checkpoints. A stalled intentional shutdown returns `0`, and a
service-manager stop remains stopped. Watchdog failure never synthesizes the
`55` exit code that triggers a player wipe.

A timeout before any completed loop, invalid watchdog configuration, or a game
that cannot exit after `SIGKILL` returns `78`. The launcher stops and the unit's
`RestartPreventExitStatus=78` prevents an automatic boot loop. Inspect the watchdog
report and boot logs, correct the cause, then explicitly restart the service.
Ordinary exited-process recovery keeps the existing restart policy. Linux cannot
force an uninterruptible kernel wait to finish; in that case detection and the
kill request are possible, but replacement is refused while the old game remains.

The unit stays `Type=simple` with the launcher as systemd's main process. It uses
the external observer rather than `WatchdogSec`: systemd's documented watchdog
starts after startup and requires periodic `WATCHDOG=1` notifications; notification
access defaults to the main process. Configuring a watchdog for the launcher
without forwarding completed-game-loop progress would measure the shell instead
of the world. See [systemd.service(5), WatchdogSec and NotifyAccess](https://man7.org/linux/man-pages/man5/systemd.service.5.html).
Keep `TimeoutStopSec` above the shutdown allowance plus two seconds of diagnostics,
the abort/kill budgets, and the launcher's ten-second tail. The default 90 seconds
covers the shipped values; raise it with an inspected systemd override if you
increase those deadlines.

Prepare the production `.env` and protected runtime directories before installation.
The service account must own `.env`, which must remain mode `0600`. Complete the
production checklist below, qualify migrations on a restored non-production clone,
apply approved migrations through the migration runbook, and run this offline preflight:

```bash
sudo -u DURIS_USER /absolute/path/to/duris/scripts/cycle_mud.sh \
  --production --check-config
```

Install a disabled copy for inspection without disturbing the current listener:

```bash
sudo /absolute/path/to/duris/scripts/install-production-service.sh \
  --user DURIS_USER --no-enable
sudo systemctl cat duris-mud-production.service
```

The installer renders the absolute checkout path and account into
`/etc/systemd/system/duris-mud-production.service`, validates the unit with
`systemd-analyze verify` and reloads systemd. Enabling repeats the production
launcher configuration check. When the production unit is inactive, the installer
also checks whether port `7777` is occupied. That installer check is hard-coded:
it does not follow `DURIS_PRODUCTION_PORT`, and it is skipped when the unit is
already active. Verify the owner of the configured production listener before
cutover. The `--no-enable` staging path does not require production credentials;
the service enforces them when it is started.

For the cutover, stop and disable any prior service that owns the configured
production port (default `7777`), then start the production unit. Do not run the
local and production units concurrently:

```bash
# If this checkout currently uses the local user service:
systemctl --user disable --now duris-mud.service

sudo /absolute/path/to/duris/scripts/install-production-service.sh \
  --user DURIS_USER --start
sudo systemctl status duris-mud-production.service
sudo journalctl -u duris-mud-production.service -f
```

Alternatively, `install-production-service.sh --start` performs enablement and startup
in one explicitly requested step. Installation does not create credentials, change
`.env`, run legacy migrations, or promote a database. A failed production preflight is
a deployment blocker, not a reason to weaken the unit or reuse development secrets.

Do not use the `pwipe` shutdown path for ordinary restarts: exit code `55`
causes `cycle_mud.sh` to run the filesystem player wipe artifact after the
server exits.

Pwipe advances `season_reset_state.season_epoch` and records `resetting` before the first
destructive SQL statement. If any later step fails, the server exits and subsequent boots
refuse to start while that state remains. Treat this as an incomplete destructive reset:
preserve the database and logs, determine which reset postcondition failed, and recover
or complete the reset under operator control. Do not change the row back to `active`
merely to bypass the boot fence.

Every active Redis key and channel includes the boot-captured SQL season epoch. The old
process continues to target only the old epoch while pwipe is in progress; after restart,
the new process targets only the completed new epoch. Redis invalidation deletes and then
verifies the old epoch when Redis is enabled. If Redis is explicitly disabled, old keys
cannot become visible when it is enabled in a later season because no active unscoped
surface remains.

## Pre-service safety gate

Before a development start, confirm the intended role and target without printing
credentials:

```bash
# Validate mode-specific requirements without contacting a database.
./scripts/cycle_mud.sh --check-config

# Inspect names only. Do not print or copy secret values.
sed -n 's/^\(ENVIRONMENT\|PERSISTENCE_MODE\|FLATFILE_STATE_DIR\|DB_HOST\|DB_PORT\|DB_NAME\|DB_ALLOWED_TARGETS\)=.*/\1=<set>/p' .env

# Source-only contract checks; these do not connect to a configured database.
python3 tests/async/test_runtime_connection_trust.py
python3 tests/async/test_runtime_boot_compatibility.py
```

In MariaDB mode, `DB_NAME` selects the requested database and `DB_ALLOWED_TARGETS`
authorizes the exact resolved `host/database` pair. Away from the configured
production port, the names `duris` and `duris_prod` resolve to `duris_dev` before
allow-list validation. The runtime accepts only `ENVIRONMENT=local` or `production`;
production must use the configured production port. Either role can use loopback
TCP, while `DB_SOCKET` is restricted to local loopback mode. Remote TCP requires
verified TLS and a CA file in either role. Use an isolated loopback target for
development and migration qualification.

SQL connections have ten-second connect/read/write deadlines. Full schema and
migration verification precedes lookup publication, SQL UID reservation and pool
startup; persistence logging, SQL lifecycle recovery and connection-activity
bookkeeping start earlier. Native flat-file startup uses its private state root
and does not run this SQL path. See [CONFIGURATION.md](CONFIGURATION.md#persistence)
and [RUNTIME_COMPATIBILITY.md](../persistence/RUNTIME_COMPATIBILITY.md#boot-gate).

Do not start the game if the target name, role, host, allow-list, TLS posture, or
backup status is uncertain. Qualify the exact target first; never probe a migration
script against a configured database to discover its command-line behavior.

### HTTP health probe

After startup, verify process and selected-persistence readiness without logging in:

```bash
scripts/healthcheck.sh
```

The probe targets `http://127.0.0.1:4050/health` by default. For an isolated local
instance, set `DURIS_WEBSOCKET_PORT` on the server and the matching
`DURIS_HEALTH_URL` for the probe. A healthy response is HTTP 200 with only
`status=healthy` and `persistence=ready`; the handler performs no blocking
database round trip.

### Authenticated post-deployment smoke

When a deployment has been explicitly authorized for live validation, record
the current service PID/restart count, listener ownership, health result, and a
timestamp or cursor for each active log before connecting. Load the configured
`GAME_ACCOUNT_NAME`, `GAME_ACCOUNT_PASSWORD`, and
`GAME_ACCOUNT_CHARACTER_NAME` without putting their values in command
arguments, transcripts, or evidence files.

The client must accept both account-selection paths: a normal selection can ask
`Play as <character>?`, while reclaiming a link-dead character can enter the
game immediately. TLS can also reach the account prompt without the same
terminal preamble as plain telnet, and an SSL client must consume data already
buffered by the handshake. Treat prompts as states instead of sending the next
command after a fixed delay.

Use read-only gameplay commands such as `look`, `time`, `weather`, `score`,
`inventory`, `equipment`, `exits`, `who`, and permission-appropriate `users`.
Confirm the HTTP health probe still passes during the session, leave gameplay
with `quit`, select `0` at the account menu, and verify that no test session is
left attached or link-dead.

After logout, monitor the authoritative service journal and every current game
log through a quiet interval. Recheck the exact service PID/restart count,
listeners, health response, and absence of a new core file. Correlate expected
EOF, refused-connection, and orderly TLS-close messages with the smoke probes;
do not dismiss an uncorrelated persistence, crash, or integrity diagnostic as
test noise.

## Logs

File logs use paths under `logs/`. Each cycle moves the direct contents of
`logs/log/` (except `.gitignore`) into `logs/old-logs/<timestamp>/`; that rotation
does not include `logs/player-log/` or the console file.

| File | Content |
|------|---------|
| `logs/log/status` | Boot progress, MySQL connection status, system messages |
| `logs/log/sys` | System diagnostics |
| `logs/log/events` | Event diagnostics and legacy persistence fallback records |
| `logs/log/file` | File and persistence diagnostics |
| `logs/log/cmd.debug` | Command trace while `debug_mode` is enabled; opened afresh at boot and rewound every 500 recorded commands |
| `logs/player-log/wizcmds` | Immortal commands and staff audit events |
| `logs/duris-console.log` | stdout/stderr from the `start_mud.sh` nohup fallback |

The checked-in production systemd unit sends stdout/stderr to the service journal;
inspect it with `journalctl -u duris-mud-production.service`.

In `flatfile-primary`, events sent through the database-backed audit logger remain
available in the ordinary files above: staff events use `logs/player-log/wizcmds`,
experience events use `logs/log/exp`, and player, quest, connection, and session events
use `logs/player-log/player`. Each entry retains its kind, player ID and name, IP, zone,
room, and message. Control characters are flattened so one event cannot forge another
log line.

Account password recovery by email writes to two of those files. At boot `logs/log/status`
carries the disposition: `Account recovery enabled (smtp port=<port> tls=<0|1>).` when
`MAIL_ENABLED=TRUE` and every `MAIL_*` setting validated, otherwise lines ending in
`password reset by email disabled.` that name the reason (`MAIL_ENABLED is not TRUE`,
`configuration rejected: <category>` where the category names the offending key, never its
value, or `mail sender failed to start`), followed by the boot sequence's own `Account
recovery unavailable; password reset by email disabled.` While the feature runs,
`logs/player-log/player` records one line per request, per completion, per mail result, and
when wrong guesses exhaust a code, carrying only the request id, the outcome category, and
the integer libcurl and SMTP codes (for example
`account recovery mail request=<id> outcome=<sent|retryable|terminal> curl=<n> smtp=<n>`);
`(account=redacted)` is literal. Recovery subsystem log lines contain no reset code,
email address, account name, client address, or libcurl error prose. Completions,
exhausted codes, save failures, and (rate-limited to one line per 60 s) terminal mail
failures or live-token evictions and host-window slot recycling also raise a `*** STATUS:`
notice to immortals watching status, which is mirrored into `logs/log/status`. A run of
terminal mail failures requires investigation of the numeric libcurl/SMTP result.
Relay configuration, credentials, certificates, recipient rejection, and local send
setup failures can all produce terminal outcomes. Correct the identified cause and
restart when changing `MAIL_*` settings; never work around it by weakening TLS
verification, which the source contracts forbid.

Useful checks:

```bash
tail -f logs/log/status                 # boot + DB issues
rg 'NEVENT BUDGET' logs/log/status # event-callback latency telemetry
rg 'telemetry_health' logs/log/status  # telemetry writer state and alerts
```

### Telemetry writer health

The game loop observes only the writer's cached atomic health; it never performs
SQL or waits for the telemetry worker. Each `telemetry_health` status line is
metadata-only and includes the backend/schema, producer and record sequence,
record kind, numeric error and failure class, queue/in-flight/retry state,
advisory-lock state, counters, and coverage gaps. It never includes a player
name, credentials, SQL text, or a record payload.

A retained record with no commit progress raises a warning after two configured
telemetry intervals and becomes critical after five minutes. An open circuit,
permanent repository failure, queue use at or above 80%, or a dropped control
record is critical immediately. Identical active failures produce at most one
reminder every five minutes, while state, severity, reason, and failure-signature
changes are logged immediately. An idle writer with no outstanding accepted
records does not become stale merely because it has no recent commit.

A trusted operator can run `world telemetry` to inspect the same live metadata
without a debugger. Start with `state`, `reason_flags`, `failure class`, numeric
`error`, `record_kinds`, `queue`, and `retry`; compare `last_admitted_seq` with
`last_committed_seq` to locate the unresolved range. Fix the named schema,
permission, connection, or storage fault rather than weakening SQL/TLS policy.
A permanent open circuit retains its in-flight batch and requires the normal
reviewed telemetry lifecycle restart after the dependency is repaired. Recovery
is logged once, only after fresh commit progress, with the alert duration and
affected producer/sequence range.

### Command and event latency

Automatic 300-pulse windows in `logs/latency_trace.log` and stderr use the same
immutable snapshot. Join command, slow-tick, and scheduler records by boot ID,
absolute tick, and monotonic pulse-start time; the trace file spans boots.
Worker samples render unavailable ticks as `-`. Window counts/min/max/means and
the exact bounded top ten describe that window, not process lifetime. Check
`dropped_section_samples`, `dropped_contended_samples`, and
`invalid_clock_samples` before interpreting an incomplete capture.

`COMMAND OP SLOW` and command-sweep reports start at 50,000 microseconds even
with debug profiling off. They distinguish playing, nanny, pager, editor, SSL,
and descriptor maintenance. `unattributed_sweep_us` covers work outside the
explicit scopes, including gates and dequeue. Playing labels use canonical
command names or `unknown`; arguments and nanny/editor input are excluded.
Reports retain at most eight slow operations and throttle output to one report
per four pulses, carrying suppression counts and the worst suppressed operation
into the next due report. A small event bucket does not explain a large command
bucket; a budget-exhausted event pass is not necessarily a 250 ms loop overrun.

For an authorized, bounded profiling capture, use `debug profile off`,
`debug profile reset`, `debug profile on`, then after the observation interval
`debug profile off` and `debug profile save`. These timers measure monotonic
elapsed work, including blocking time, not total process CPU. Compare callback
counts and per-call cost as well as totals. Nested scopes overlap and must not
be summed. The `nevent_defer_collect`, `nevent_defer_unlink`,
`nevent_defer_sort`, and `nevent_defer_merge` scopes isolate deferral phases;
`short_affect_liveness` measures the conditional owner-membership guard.
Verify profiling is off after capture.

`DURIS_NEVENT_TRACE_PLAYER=1` adds per-callback due/actual ticks, lateness,
sequence, and elapsed time to `logs/log/status`; `DURIS_NEVENT_ANALYTICS=1`
adds callback-family windows. These settings are cached after first use and
must be supplied to the process before use. Player traces contain player IDs:
keep raw captures private, bound their duration, and separate startup from
steady-state observations. The maximally late callback in a budget report is
not a complete count of late callbacks and may belong to an NPC.

For casting complaints, correlate a controlled mortal cast's intended duration,
callback timing, completion/abort, and queued input. `event_spellcast()` schedules
continuations relative to actual execution, so segment lateness can accumulate.
The casting/wait mismatch is tracked in
[#186](https://github.com/Community-Duris/Duris/issues/186). Aggregate scheduler
samples alone do not establish a particular player's delay or justify changing
NPC cadence, priority ordering, or budgets.

### Persistence health

A trusted character can run `world persistence` for a fresh, read-only view of
database and save health. Repeating the command takes new snapshots; it does not
cache output or mutate queue, Redis, deferred-save, or query state.

The report includes up to eight deterministically ranked query source sites,
total calls and failures, registry overflow, item/scalar/large queue counters,
player capture/journal/worker depths and ages, exact revision progress, world capture
and publication health, redacted shared Redis boot/recovery/maintenance calls, failures,
timeouts, maximum latency and reconnect transitions, per-worker Redis operation latency,
failure streak, and last-success age, critical-command queue/journal/fence health, flat
shop materialization event/byte capacity and reclaimable counts, and the oldest aggregate
save age. Output is metadata-only and
must not be copied into a workflow that expects SQL, player, account, item, IP, or path
values.

Interpret explicit states as follows:

- `state=empty` means the observed subsystem currently has no pending work or
  has recorded no query calls.
- `state=disabled` means the reported optional backend or integration is configured off.
- `state=unavailable` means the subsystem is enabled but its local health state
  cannot currently confirm availability. `heartbeat=unavailable` means that
  queue worker has never published a heartbeat.
- Failed deferred work normally remains in `scheduled` while its bounded retry is
  pending. `failed_unscheduled` should remain zero; a non-zero value means scheduling
  invariants were violated and requires investigation.
- `registry_overflow` greater than zero means additional source sites were not
  retained; recorded totals remain bounded and should not be treated as a full
  site inventory.

The displayed query operation IDs and any `SQL_TRACE` operation IDs are scoped
to the current process. They are correlation aids, not durable transaction or
idempotency identifiers.

For `critical_commands`, `blocked>0`, a growing oldest age, journal corruption or I/O
failure, or journal quota exhaustion must stop the affected gameplay and any process
transition. Restore the storage or destination and preserve the journal for replay.
Never delete or edit the journal to clear a fence. See
[CRITICAL_COMMAND_PIPELINE.md](../persistence/CRITICAL_COMMAND_PIPELINE.md).

For `critical_outbox`, pending age may briefly rise during destination recovery.
`dead_letter>0`, `incomplete_inbox>0`, or `committed_without_outbox>0` is an integrity
incident. Preserve the journal and database rows, stop affected domain cutovers, and run
the typed reconciliation report. After correcting the destination, retry only the
specific numeric dead-letter ID through the guarded repair API; never edit payloads or
execute SQL copied from a command.

For `shop_materialization`, `state=degraded` means the checksummed catalog has reached
80% of its event or byte limit. `state=unavailable` means its lock, read, checksum, or
bounded decode failed. Preserve the authority files and transaction journal, stop new
flat-primary shop trades, and investigate the storage or catalog before attempting any
repair. The health read is lock-scoped and on demand; it never prints player, item, or
path data.

### Retained terminal-save failures

`deferred_save_retry_scheduled` means the live character remains the recovery source;
the alert includes only delay and aggregate counters. Let the bounded retry run and
watch `world persistence` for pending age and failure growth.

`terminal_save_failed` or `terminal_not_durable` with `extract_refused=1` means camp,
rent, death cleanup, ghost extraction, an offline artifact transition, or a locker
transition deliberately kept its live object graph. Do not manually extract that
character or locker. Restore database availability, retry the originating action or a
trusted save, and verify the pending count clears.

`terminal_not_durable` with `leave_vetoed=1` means locker snapshot preparation did
not complete. The occupant and dynamic locker room remain live; do not purge either.
Restore database availability and have the occupant retry departure.

A copyover or shutdown alert with `shutdown_cancelled=1` means the process deliberately
returned to the live game loop. No fallback restart should be forced. Correct the
database failure, confirm every pending age is falling or stable, then request the
copyover/shutdown again. A `fallback_saved` player-pfile alert is recovery evidence
only; it does not mean MySQL committed and is not automatically replayed.

## Restart and crash recovery

The automatic recovery paths are:

1. **Redis world-state recovery** -- after either a graceful restart or an unclean exit,
   the current immutable generation is accepted only after manifest authentication,
   complete-payload digest verification, and validation of schema, completeness,
   sequence, checksum, size, and age. Floor deltas are decoded with the matching
   generation into one semantic plan. Every authority-marked item must exactly match
   SQL UID, root, parent, room owner, VNUM, and active state before rollback-capable
   materialization. Reconstructible world-pop objects have no SQL custody, and player
   corpses use their separate authoritative restore path. NPC equipment and inventory
   are omitted from the snapshot; restore reconstructs applicable zone-defined NPC
   items under normal population, artifact, and duplicate checks. After a successful
   restore, boot attempts to consume the exact generation under the writer fence;
   consumption failure is logged while boot continues. An expiring one-use marker
   labels a matching valid generation as clean-restart recovery. Recording that marker
   is attempted during eligible drained shutdowns; failure does not cancel shutdown.
2. **Copyover recovery** -- only with `-C` boot flag / copyover flow.

If Redis recovery fails, the server runs a full normal reset for every zone. Check
`logs/log/status` for `Performing redis clean restart recovery...` or
`Performing redis crash recovery...` followed by `applying full normal zone boot`, and
verify player integrity before reopening. The rejected generation and floor data are not
cleared by the failed restore.

For queue or dependency incidents, use `world persistence` and the detailed `redis`
status command. Do not clear a player save queue: player state is owned by the local
revision coordinator and journal, not a Redis dirty set. A rejected publication
compare-and-set leaves the prior current generation selected. A lost Redis reply can
instead hide a committed pointer swap and floor clear, so a reported publication
failure does not prove that the prior generation and floor deltas remain selected.
Preserve the authority state and inspect publication health before attempting repair.
See [WORLD_RECOVERY_PIPELINE.md](../persistence/WORLD_RECOVERY_PIPELINE.md).

Account password recovery keeps no durable state. Reset codes, their per-account cooldown
records, and any queued or in-flight recovery mail live only in process memory, so a
copyover or restart discards them; the player-facing text already says to request a new
code after a restart, and no operator action or cleanup is needed. Mail delivery is
outside the durable shutdown drain chain and does not veto a copyover or shutdown.
Normal process teardown drops queued mail and joins the worker; an in-flight send can
therefore delay process exit by up to 20 s (libcurl is bounded to 10 s connect / 20 s
total per send, with no retry). Successful copyover exec discards the old process's
mail state. The production unit's
`TimeoutStopSec` must stay above that tail; the checked-in
`deploy/systemd/duris-mud-production.service.in` uses 90 s. `scripts/change_password.sh`
remains the operator fallback for an account with no usable email address on file.

### Interpreting boot diagnostics

These messages describe specific boot/reset paths. They do not by themselves
establish that the entire boot or world data is healthy.

| Line | Meaning |
|---|---|
| `Heaven has invalid number: 1 (should be 0)` | `recalc_zone_numbers()` adjusts the in-memory zone number to match the lowest room vnum. It does not rewrite the source area file; inspect the zone before deciding whether a persistent data change is needed. |
| Mob log `M cmd not executed` | The zone reset's random roll did not satisfy that mobile command's configured load percentage. This branch clears the current mobile context; inspect the reset command and surrounding diagnostics if the resulting population is unexpected. |

`PERSISTENCE: worker_unavailable_flat_fallback` requires investigation rather than
being treated as a successful boot replay. Preserve the fallback records and
inspect the selected persistence pipeline. Raw SQL fallback execution is retired:
the compatibility replay entry point quarantines records without executing them,
and normal startup does not call it. These records are not a substitute for the
typed player or critical-command journals. See
[PLAYER_SAVE_PIPELINE.md](../persistence/PLAYER_SAVE_PIPELINE.md).

## Backups and maintenance scripts

| Script | Purpose |
|--------|---------|
| `scripts/backup_pfiles.sh` | Publish verified full generations under the approved policy; also gates each cycle iteration. |
| `scripts/restore_flatfile_backup.sh` | Qualify a flat-file generation in a fresh isolated candidate with current erasure evidence. |
| `scripts/delete_corpses.sh` | Retired safety stub; exits nonzero without reading or changing MySQL or Redis. |
| `scripts/clear-redis.sh` | With the game stopped, use the scoped maintenance ACL identity to delete only the configured `REDIS_NAMESPACE`, legacy `mud:*`, and retired `ship:snapshot:*` keys from an explicitly confirmed, local, allow-listed Redis target; unrelated keys are preserved. |
| `scripts/import_help_to_prod.sh` | Import help sources to MySQL; use `--dry-run` first and treat `--clean` as destructive. |
| `scripts/migrate_players_to_accounts.sh`, `scripts/convert_all_pfiles.sh` | One-shot legacy data conversions; back up and review their assumptions before use. |
| `bin/migrations/*` | Offline pfile/schema conversion binaries built from `migrations/tools/`. |

Schema operations follow the safety rules in [DATABASE.md](../reference/DATABASE.md):
back up, clone, validate replay on the clone -- never against live data.

Full backup policy, generation format, journal consistency, retention,
separate/off-host custody, erasure preflight, isolated restore qualification and
operator cutover/rollback are maintained in [BACKUPS.md](BACKUPS.md).
BACKUP_POLICY_FILE is mandatory; legacy per-mode backup-root variables and the
old raw-copy restore interface no longer select or bypass the policy.

### Migration procedure

All migration qualification is offline work on a disposable database or a restored,
backed-up development clone. Stop every writer before cloning. Record the source
backup identity, restore it under an explicitly non-production name on a loopback
host, and set `ENVIRONMENT`, `DB_HOST`, `DB_NAME`, and `DB_ALLOWED_TARGETS` so the
clone is the only permitted target. Keep the original backup untouched.

Before converting locker authority to flat files, run the read-only, aggregate-only
`migrations/check_flatfile_account_locker_conversion.sh --expect-count <known-count>`
against that clone. It proves each `account.<account>.<side>.locker` row maps to an
authoritative account and valid racewar side, and checks owner fields, chests, item
custody identifiers, duplicate active UIDs, and access references. Quarantined or
otherwise inactive legacy payload rows are outside this authority conversion and remain
subject to their separate recovery workflow. Any nonzero mismatch or unexpected total
blocks the cutover; the checker never emits account or locker names.

The legacy runner is mutation-capable and has no dry-run mode. `--help` is safe, and an
unknown argument is rejected before configuration is loaded. A normal no-argument run
begins work immediately. When `REDIS=TRUE`, its final step requires `ENVIRONMENT=local`,
an exact `REDIS_HOST:REDIS_PORT/REDIS_DB` or `unix:REDIS_SOCKET/REDIS_DB` entry in
`REDIS_ALLOWED_TARGETS`, and the configured ACL/TLS settings. It deletes only
`<REDIS_NAMESPACE>:*`, legacy `mud:*`, and
retired `ship:snapshot:*` keys, verifies
the postcondition, and fails the migration if `redis-cli`, the connection, deletion, or
postflight fails. When Redis is disabled, the step reports `not enabled` without requiring
Redis connection fields. The game and every other Redis writer must remain stopped.

Production runtime startup requires distinct `REDIS_WORLD_*`, `REDIS_PRESENCE_*`,
`REDIS_CACHE_*`, and `REDIS_MAINTENANCE_*` credential pairs, plus a distinct
`REDIS_DONATION_*` pair when donation subscription is enabled. Do not temporarily reuse a
runtime identity for maintenance: correct the ACL configuration and repeat preflight.
Local maintenance may fall back to `REDIS_USERNAME`/`REDIS_PASSWORD`, but an explicitly
configured maintenance pair must be complete. Rotate one subsystem at a time by updating
that Redis ACL user and its matching environment pair, then restart; connection settings
are boot-captured and credentials are never reread on a gameplay path.

World recovery requires an independent `REDIS_WORLD_STATE_SECRET`. To rotate it without
accepting unsigned data, move the old value to `REDIS_WORLD_STATE_SECRET_PREVIOUS`, install
the new current value, restart, and wait for a new acknowledged generation. Then remove the
previous value and restart again. A manifest signed by neither key, or a generation whose
SHA-256 digest differs from its authenticated manifest, is rejected before materialization.

On the qualified clone only, use this order:

```bash
# 1. Legacy additive upgrade and exact verified adoption. No arguments; mutates immediately.
MIGRATION_ENV_FILE=/path/to/owner-readable-clone.env ./migrations/run_migration.sh

# 2. Inspect the checked-in manifest identity without opening the database.
python3 scripts/migration_runner.py inspect

# 3. Apply the immutable post-baseline prefix and verify boot compatibility.
python3 scripts/migration_runner.py run
./migrations/verify_runtime_compatibility.sh
```

After that exact backup has passed on the clone, an explicitly authorized production rollout may
apply only the immutable pending prefix. Keep every production writer stopped, create a fresh
backup with `scripts/backup_pfiles.sh`, and pass both the resolved target and backup explicitly:

```bash
set -a
source .env
set +a
python3 scripts/migration_runner.py run \
  --confirm-production-target "$DB_HOST/$DB_NAME" \
  --production-backup /absolute/path/to/fresh-production.sql.gz
./migrations/verify_runtime_compatibility.sh
```

The production path refuses baseline adoption and legacy migration. It requires
`ENVIRONMENT=production`, an exact `DB_ALLOWED_TARGETS` match, a backup no more than two hours old
that is an owner-only regular gzip containing the configured Duris schema, CA-verified TLS for a
remote database, and zero other connections to the target before it applies each step. Never stop
or bypass one of these checks. Preserve the backup through deployment and the final soak.

Keep the clone configuration separate from the server's `.env`; the legacy runner
loads the file named by `MIGRATION_ENV_FILE` and rejects symlinks, non-regular files,
and files readable by group or others. That file must describe the allow-listed
loopback clone and must not contain production credentials.

For a fresh disposable bootstrap, import `migrations/bootstrap_multithread_safe.sql`
and use `adopt --kind fresh_bootstrap` instead. Without the two explicit production arguments, the
immutable runner rejects production roles, non-loopback hosts, production-like names, manifest
drift, incomplete baselines, or broken history. The compatibility verifier is read-only but
database-connected and must receive the intended target. Never use production for migration
discovery, qualification, or exploratory replay. The guarded immutable application and its final
read-only compatibility verification are the only production steps in this procedure.

If any step fails, keep writers stopped and preserve the database, command output,
manifest, and backup. Do not edit ledger rows, skip a verifier, rerun a partial legacy
bundle blindly, or guess a reverse DDL. MySQL DDL may already have committed. Recovery
is to investigate on another clone or discard the failed clone and restore the known
backup, then repeat the entire qualification. See
[IMMUTABLE_MIGRATIONS.md](../persistence/IMMUTABLE_MIGRATIONS.md) and
[RUNTIME_COMPATIBILITY.md](../persistence/RUNTIME_COMPATIBILITY.md).

### Domain reconciliation

The reconciliation scripts below are read-only reports, but they connect to the
selected database. Run them only after repeating the exact clone target qualification
above; never use production as a development or validation target.

```bash
./migrations/reconcile_epic_balances.sh
./migrations/reconcile_currency_balances.sh
./migrations/reconcile_item_ownership.sh
./migrations/reconcile_auction_transactions.sh
./migrations/reconcile_combat_frags.sh
./migrations/reconcile_artifact_guild_outcomes.sh
./migrations/reconcile_boon_reward_zone.sh
./migrations/reconcile_phase02_domains.sh
```

A nonzero mismatch is an integrity incident, not permission to edit current rows.
Stop the affected domain, preserve its journal, inbox, outbox, ledger, and report,
then trace the stable operation identity. Use only the domain's guarded retry or
repair interface after the cause is known.

For character-baseline readiness, `migrations/check_character_baseline_readiness.sh`
is production-safe and aggregate-only. It requires every active, unblocked,
account-mapped character to have wallet, epic, and combat-frag opening rows. The same
gate runs during MariaDB boot, runtime compatibility verification, and guarded legacy
dump import; a missing row makes the character generation unready.

Classify missing combat baselines only at an approved quiesced save boundary. Create a
private operator directory, then write the row-level result there; routine output stays
aggregate-only:

```bash
install -d -m 700 /absolute/private/combat-baseline-review
./migrations/repair_missing_combat_baselines.sh \
  --classify /absolute/private/combat-baseline-review/classification.tsv
```

`safe_no_history` has revision zero and no ledger row, so its opening value is the
locked current value at revision zero. `ledger_history_requires_review` includes only a
proposed arithmetic candidate and is never applied by the tool; prove the complete,
contiguous history and review separate targeted DML. `revision_without_ledger` has no
defensible automated opening value. Keep all PIDs and row details in the owner-only
artifact. Classification is not a resolution: every non-safe row requires a separate,
owner-only per-PID decision record containing the frozen source evidence, authoritative
opening value and revision, reviewed DML checksum, reviewer identity, and either an
explicit approval or `blocked_no_authoritative_opening`. The repair tool deliberately
does not consume those records. Do not mark the production incident resolved until every
affected row has an approved disposition and the aggregate readiness gate is zero.

Rehearse safe rows only on a fresh production clone after a verified backup and with
all writers stopped. Supply the reviewed artifact digest and backup identity:

```bash
WRITERS_QUIESCED=TRUE COMBAT_BASELINE_BACKUP_ID='<backup-generation>' \
  COMBAT_BASELINE_ROLLBACK_EVIDENCE=/absolute/private/combat-baseline-review/rollback.sql \
  COMBAT_BASELINE_ROLLBACK_SHA256='<reviewed-sha256>' \
  ./migrations/repair_missing_combat_baselines.sh --apply \
  /absolute/private/combat-baseline-review/classification.tsv '<sha256>'
```

The insert-only transaction preserves existing baselines, verifies locked player and
ledger state, fails on a conflicting baseline, and rolls back unless character readiness
plus combat, currency, epic, item-ownership, and required-FK reconciliation all pass.
It writes those aggregate results to an owner-only receipt. The required rollback file
must contain reviewed inverse DML limited to the artifact's exact PIDs, values, and
revisions, with guards rejecting any subsequent combat revision or ledger activity. A
repeat is idempotent only when the existing row exactly matches the reviewed opening.
This command refuses production targets; it is rehearsal evidence, not permission to
repair production. Before any separately authorized production repair, retain the
backup, protected per-PID decisions, reviewed forward and rollback DML, all digests, and
the rehearsal receipt. Afterwards rerun the same full reconciliation set. Never delete
or rewrite ledger history to make readiness pass.

Loopback combat-baseline clone targets are permitted directly. A remote clone additionally
requires verified TLS and an exact port-aware `DB_ALLOWED_TARGETS` entry in the form
`host:port/database`; a non-default port cannot reuse authority granted to another endpoint.

For the physical-coin replacement signature, first stop all writers at a clean save
boundary and create a private operator directory. Classification writes the exact
row-level old-custody/new-payload pair only to that directory; routine output contains
only a count and digest:

```bash
install -d -m 700 /absolute/private/coin-custody-review
./migrations/reconcile_coin_custody_pair.sh --classify \
  /absolute/private/coin-custody-review/pair.tsv
```

Review the protected artifact against the backup and frozen reference. It is eligible
only when there is exactly one canonical money payload without custody and one matching
active, baselined player-custody row without any payload, with the same player, vnum, and
container parent. Never infer a second pile from currency totals and never delete the old
custody history.

Rehearse only on a fresh non-production clone. The guarded transaction restores the
payload's UID to the still-authoritative custody identity; it does not alter denominations,
wallets, baselines, owner revisions, or ledger rows. Apply first proves that the database
rejects an invalid temporary-table `CHECK`; it aborts without DML or a receipt when that
guard is disabled or cannot be verified. Name the clone exactly `duris_dev`, `duris_local`,
or `duris_test`. Every coin-custody apply, including loopback, also requires an exact
port-aware `DB_ALLOWED_TARGETS` entry in the form `host:port/database`:

```bash
WRITERS_QUIESCED=TRUE COIN_CUSTODY_BACKUP_ID='<backup-generation>' \
  ./migrations/reconcile_coin_custody_pair.sh --apply \
  /absolute/private/coin-custody-review/pair.tsv '<sha256>'
```

Preserve the owner-only receipt and rollback evidence. Before any separately authorized
production repair, prove the same preconditions under row locks, retain reviewed DML and
the exact backup, and run the player materializer plus item-ownership, currency, schema,
and FK reconciliation on the clone. Rollback is the inverse UID update to the exact payload
row and is safe only before the repaired player is loaded or saved again.

### Player death restitution: offline SQL and native live modes

The audited restitution tool is evidence recovery, not an automatic reimbursement
command. It reads the immutable `0020_player_death_restitution` contract and
accepts supported normalized death schemas after the native bridge has validated
the raw wire version. Death requests use wires 2, 4, 6, 8, 13, 15, and 18, or
ward-bearing equivalents 21, 26, 28, and 31. Evidence envelopes are excluded;
an unknown or corrupt wire is refused.

The production target probe is read-only. Native preparation uses a target-info
artifact containing the exact database-name confirmation and actual server
fingerprint; it does not require a stopped service or a maintenance boundary:

```bash
python3 scripts/player_death_restitution.py --env-file /secure/duris.env \
  target-info --confirm-production-target <exact-db-name> \
  --artifact /secure/restitution-target-info.json
```

Pass that artifact to `inspect --target-info` with the exact confirmation and
fingerprint, and to `plan --target-info --preparation-mode native`. The native
`inspect -> plan -> export` preparation is read-only and does not run backup,
quiescence, service-stop, or SQL mutation code. It still carries the target
fingerprint, actor/plan identity, explicit approval, expiration/deadline metadata,
recipient-only native fence, and revision/custody evidence into the exported command.

The offline SQL path has the stronger stopped-boundary probe. Use the installed
system manager when the production unit is root-managed:

```bash
python3 scripts/player_death_restitution.py --env-file /secure/duris.env \
  target-info --confirm-production-target <exact-db-name> \
  --maintenance-kind systemd --maintenance-id duris-mud-production.service \
  --artifact /secure/restitution-target-info.json
```

For the `.sbs` deployment, which is user-manager managed, run the probe as the
service owner and bind the exact manager explicitly. The tool checks the owner
UID, `user@UID.service` manager, manager/unit state and PIDs, the owner runtime
socket, the real cgroup hierarchy, and visible cgroup processes. It does not
stop or mask the unit; prepare that state through the approved service operation
before probing, and treat any mismatch as a refusal. A masked inactive/dead unit
may report `LoadState=masked` with `ControlGroup=` empty after systemd removes
its dead cgroup; the probe records that empty value and accepts it only with
zero service PIDs and complete owner-manager process visibility. It never
invents a cgroup path for an absent group:

```bash
python3 scripts/player_death_restitution.py --env-file /secure/duris.env \
  target-info --confirm-production-target <exact-db-name> \
  --maintenance-kind systemd-user \
  --maintenance-id duris-mud-production.service \
  --maintenance-owner "$(id -u)" \
  --artifact /secure/restitution-target-info.json
```

Do not substitute `--user` for `--system`, omit the owner, accept an active
unit, or create an offline-proof file as a lifecycle attestation. The target
fingerprint and maintenance record are carried into the backup, plan, apply,
and verification artifacts; a changed manager, owner, unit, PID, cgroup, or
SQL target makes the artifact stale.

There are two intentionally separate execution modes:

- The native live-runtime handoff uses `inspect`, `plan --preparation-mode native`,
  and `export`. Production `inspect`/`plan` must carry the read-only target-info
  artifact with exact target confirmation and server fingerprint; native `plan`
  must not carry an offline backup or maintenance boundary. `export` requires the
  exact SQL-derived evidence lineage, explicit staff approval, actor/reason, and
  native revision fences, but does **not** require `--offline-proof` or a
  server-wide stop. Submit the protected native payload through the existing
  authorized staff command and wait for its durable native receipt/readback;
  admission or queued output is not delivery proof.
- Direct SQL `apply` is offline-only. It additionally requires the pinned target,
  backup receipt, explicit approval, owner-only v3 offline context, process and
  SQL-writer quiescence checks, and the existing advisory-lock/custody,
  authorization, ownership, artifact, revision, and idempotency fences. The tool
  never stops or masks a service automatically.

Production `verify --plan` has two read-only policy branches and neither is a
live-native verification mode. For an explicit native plan, the original plan and
its digests are retained: supply a **fresh protected** `target-info` artifact bound
to the same production target and server fingerprint, captured with the explicit
stopped/masked maintenance boundary, plus the matching maintenance arguments.
The verifier validates that boundary before any receipt/item read, performs the
existing exact readback, does not require or inspect a plan-bound backup, does not
replan or convert the native artifact, and never promotes receipt status. A native
plan still refuses SQL `apply` and `--mark-verified`. For an `offline-sql` plan,
the existing stopped-boundary, plan-bound backup, offline-proof, and explicit
approval gates remain unchanged; `--mark-verified` remains its separate write.

Use the dedicated restitution runbook for the complete inspect/plan/export or
backup/apply/verify command sequence:
[`PLAYER_DEATH_RESTITUTION.md`](../persistence/PLAYER_DEATH_RESTITUTION.md).

### Maintenance, lifecycle, export, and erasure

The maintenance scheduler is bounded and persistent. Use `world persistence` to
inspect slot state, lag, errors, and deferred work. A disabled lifecycle slot is the
expected checked-in state, not a fault. Do not enable it by editing state files.

These commands are local inspection or source-contract checks and do not connect to
the configured database:

```bash
python3 scripts/lifecycle_archive.py inspect
python3 scripts/lifecycle_archive.py plan \
  --store database:accounts --action archive \
  --cutoff 2025-01-01T00:00:00Z --upper-bound 999
python3 scripts/personal_data_export.py inspect
python3 scripts/account_erasure.py inspect
python3 scripts/validate_data_lifecycle.py --json
python3 tests/async/test_data_lifecycle_manifest.py
```

Under the checked-in policy, archive planning reports blocked, export and erasure
inspection report `blocked_by_policy`, and canonical mutation remains disabled. These
are engineering controls, not controller approval. They are not a claim of legal
compliance. Do not invent policy references, selectors, retention periods,
shared-record decisions, or destructive adapters. Follow
[DATA_LIFECYCLE.md](../persistence/DATA_LIFECYCLE.md),
[LIFECYCLE_ARCHIVE.md](../persistence/LIFECYCLE_ARCHIVE.md),
[PERSONAL_DATA_EXPORT.md](../persistence/PERSONAL_DATA_EXPORT.md), and
[ACCOUNT_ERASURE.md](../persistence/ACCOUNT_ERASURE.md).

### Restore and tombstone preflight

Never reopen a restored environment immediately. Keep listeners, login, replay,
imports, cache publication, and export release disabled while qualifying the restore.
Restore the backup into an isolated non-production target, load the newer erasure
tombstone ledger separately, verify its policy and generation identity, then scan
database rows, pfiles, conversion backups, journals, cache rebuild inputs, and export
spools by stable account-scope hash. Unscoped identities fail closed.

Only a future approved adapter may strip a tombstoned scope, and it must do so before
any restored service is published. Verify that every completed tombstone remains
uncredentialed and unloadable and that all domain reconciliation reports pass. If a
tombstone set is missing, stale, unverifiable, or cannot cover a source class, abandon
that restore candidate; do not reopen it and do not alter historical backups in place.

### Epic ledger cutover and reconciliation

Before enabling transactional epic producers on a guarded development clone, apply
`migrations/epic_ledger_balance.sql`, run `migrations/verify_epic_ledger_schema.sh`,
then capture opening balances once with `migrations/baseline_epic_balances.sh --apply`.
The baseline command refuses when any ledger row or advanced epic revision exists and
preserves existing baseline rows.

`migrations/reconcile_epic_balances.sh` is read-only. A healthy result reports zero
missing baselines, balance mismatches, and latest-result mismatches. Stop affected epic
gameplay if any count is nonzero, preserve the inbox/outbox/ledger rows, and investigate
the operation history. Do not edit the ledger, invent historical operation IDs, or
rerun the baseline against an active ledger.

The backup command selects the explicit persistence mode and applies the
operator-approved shared policy. See [BACKUPS.md](BACKUPS.md) for independent
scheduling, full generations, bounded retention and verified isolated restores.

## Phase 03 final readiness gate

The integrated 200-player gate requires a separately configured, backed-up,
production-unreachable representative clone, approved RPO/lifecycle policy identities,
200 sanitized test identities, isolated non-default ports, and reversible deployment
adapters. It never reads `.env` implicitly.

Run preflight before any workload:

```bash
python3 scripts/session14_gate.py \
  --config tmp/session14-gate/config.json \
  --preflight-only
```

Follow [`PHASE03_READINESS.md`](../gates/PHASE03_READINESS.md) only after preflight is
`QUALIFIED`. Treat `QUALIFIED` as permission to begin the isolated gate, not as a
readiness result. Every injected fault must be torn down and the target restored before
retry. A repair invalidates affected evidence and requires affected plus complete reruns.

## Disabling a DurisWeb hook

During an incident, any single DurisWeb integration can be cut without
restarting the MUD and without affecting the others.

```
properties set durisweb.hook.<id> 0.000
```

Ids: `auction_new`, `auction_bid`, `auction_close`, `player_presence`,
`mud_shutdown`, `wholist`, `admin_delete_character`, `donation_delivery`.
Requires FORGER or above. Re-enable with `1.000`.

The change takes effect immediately and is pushed to connected DurisWeb peers,
which reflect it in their operator console within about ten seconds. It is
in-memory only -- run `properties save` to persist across a restart, or
`properties revert` to undo before saving.

A disabled hook emits nothing at source. `donation_delivery` additionally logs
one `LOG_SYS` line per pulse naming how many events it dropped, so a hook left
disabled is visible in the logs rather than silent.

`connection_log` is not in this list: DurisWeb's connection ingestion is toggled
on the DurisWeb side, because the underlying `logs/log/comm` lines are the MUD's
own operational records.

### Reconciling from the DurisWeb hook console

The website's Hook Control console uses the authenticated
`durisweb_hook_set` command for the eight ids above. This path persists the MUD
property atomically and pushes a complete state frame before acknowledgement;
do not run `properties save` for a successful console change.

Reconciliation is deliberately directional:

1. To disable, the website closes its own gate first and then asks the MUD to
   disable. A bridge failure therefore leaves delivery stopped locally and the
   row shows the partial state.
2. To enable, the website keeps its gate closed, asks the MUD to enable, waits
   for the pushed state, and opens its own gate last. A timeout or refusal
   cannot open delivery.

For a `MISMATCH`, open the row details and confirm which end differs. Use Set
Both Ends only after verifying the intended direction. For `UNKNOWN`, restore
the authenticated bridge first; do not infer that the MUD is enabled. If the
console reports a persistence error, inspect permissions and free space for
`lib/duris.properties` and its `.new` sibling, then retry. Do not hand-edit the
file while the MUD is running.

The five website-only hooks show `MUD: N/A` and reconcile only the website
gate. The terminal is always-on as the recovery path and cannot be reconciled.

## Runtime tuning

Server behavior knobs are exposed through the properties system:
`get_property()` (`src/world/properties.c`) binary-searches key/value pairs loaded
from `lib/duris.properties`, falling back to per-call defaults, e.g.
`help.cooldown.secs`. Feature config files live in `lib/*.cfg`
(crafting, mining, hardcore, frag caps, account rewards, creation
availability, random equipment). Property/config changes take effect on
restart without recompilation; check the owning subsystem docs before
editing.

One property is a live balance switch worth knowing about:
`artifact.wars.modifier` scales the race-war penalty applied by
`event_artifact_wars` -- each of a violating player's artifacts loses
`modifier x punish_level` of its remaining life, clamped to the whole of it.
The code default is `0.0` (forced drop only), but `lib/duris.properties` ships
`artifact.wars.modifier=0.500`, so a server using that file halves the
offender's artifact timers on a first-level violation. Set it to `0` to disable
the timer penalty.

## Development vs production checklist

- Development: `ENVIRONMENT=local`, a listener distinct from the configured
  production port (for example `4000` via `--dev`), and `BUILD_PROFILE=development`
  (which defines `TEST_MUD`). For MariaDB testing, use an allow-listed loopback
  target and explicit non-production `DB_NAME`. For native flat files, use the
  client-free build and private `FLATFILE_STATE_DIR`. The port is a guard rather
  than the primary authority selector.
- Production: production `ENVIRONMENT`, an explicit allow-listed database target,
  TLS with an absolute trusted `DB_SSL_CA` whenever database traffic leaves loopback,
  a `BUILD_PROFILE=production` binary, a real listener TLS certificate linked as
  `duris.crt`/`duris.key`, secrets supplied through the protected environment or secret
  store, mode-0700 journal/state directories, and regularly restored and verified
  backups of MySQL, legacy player/account material, journals, and erasure tombstones.
  Credentials never belong in source files.

### Release boundary

The repository includes production service templates, public-health workflows,
and a dated topology record in [PRODUCTION_DEPLOYMENT.md](PRODUCTION_DEPLOYMENT.md).
Those files document deployment inputs and prior verification; they do not establish
the current host state or authorize a release. Release approval, actual service
identity, cutover and rollback remain operator-owned decisions.

Validate changes with the applicable local build, focused regressions and
persistence/gameplay journeys. Broader local entry points include
`./scripts/format.sh --check`,
`python3 tests/async/test_compiler_warning_profile.py`, `make test-all`, and
`make security-check`; `.github/workflows/` defines the hosted checks. Follow any
applicable review and branch protections, without treating CI completion as a
prerequisite for finishing local verification.
