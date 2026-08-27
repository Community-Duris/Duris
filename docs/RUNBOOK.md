# Operations Runbook

Day-to-day operation of a DurisMUD instance. First-time setup is in
[README.md](../README.md).

## Starting and stopping

```bash
./scripts/start_mud.sh     # preferred: systemd user service if installed,
                           # otherwise nohup cycle_mud.sh -> logs/duris-console.log
./scripts/cycle_mud.sh     # foreground supervised run (what start_mud wraps)
./scripts/cycle_mud.sh --dev   # development: port 4000, duris_dev database
```

`cycle_mud.sh`:

- Anchors itself to the repository root; loads `.env` if present.
- Raises core dump limits (`ulimit -c unlimited`) and extracts DB credentials
  from `src/sql.h` as fallback defaults.
- Rebuilds area tools and regenerates `areas/world.*` when the `make_*`
  helpers are missing.
- Regenerates `lib/misc/event_names` (demangled symbol list used by crash
  tooling).
- Promotes `bin/server/dms_new` to `bin/server/dms`, retains the five newest
  prior executables under `bin/server/history/` by default, and runs the active
  binary in an outer loop. Set `DMS_BINARY_HISTORY_LIMIT` to change the limit.
- On each restart it snapshots logs into `logs/old-logs/<timestamp>/`, writes
  the stop reason, backs up pfiles (`scripts/backup_pfiles.sh`), optionally
  emails an alert, and records boot/shutdown times plus reason into the
  database.

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
(`src/actwiz.c`). It writes `logs/shutdown_info.txt`
(`initiated_by|reason`), which `cycle_mud.sh` consumes for its reboot record
and then removes. Copyover (`copyover` command) execs a fresh binary while
keeping player connections alive via `copyover.dat`.

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
stopping a specific local process. A normal shutdown lets the supervisor write
its reboot record and rotate logs.

Do not use the `pwipe` shutdown path for ordinary restarts: exit code `55`
causes `cycle_mud.sh` to run the filesystem player wipe artifact after the
server exits.

## Logs

All under `logs/`; rotated per-run into `logs/old-logs/<timestamp>/`.

| File | Content |
|------|---------|
| `logs/log/status` | Boot progress, MySQL connection status, system messages |
| `logs/log/syslog` | Game events |
| `logs/log/cmdlog` | Player commands |
| `logs/log/wizlog` | Immortal commands |
| `logs/duris-console.log` | stdout/stderr of the supervised process |

Useful checks:

```bash
tail -f logs/log/status                 # boot + DB issues
grep -i 'NEVENT BUDGET' logs/log/syslog # event-callback latency telemetry
```

### Persistence health

A trusted character can run `world persistence` for a fresh, read-only view of
database and save health. Repeating the command takes new snapshots; it does not
cache output or mutate queue, Redis, deferred-save, or query state.

The report includes up to eight deterministically ranked query source sites,
total calls and failures, registry overflow, item/scalar/large queue counters,
player capture/journal/worker depths and ages, exact revision progress, world capture
and publication health, critical-command queue/journal/fence health, and the oldest
aggregate save age. Output is metadata-only and
must not be copied into a workflow that expects SQL, player, account, item, IP, or path
values.

Interpret explicit states as follows:

- `state=empty` means the observed subsystem currently has no pending work or
  has recorded no query calls.
- `state=disabled` means Redis integration is configured off.
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
[CRITICAL_COMMAND_PIPELINE.md](CRITICAL_COMMAND_PIPELINE.md).

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

## Crash recovery

Two automatic paths run at next boot after an unclean exit:

1. **Redis world-state recovery** -- the current immutable generation is accepted only
   if schema, completeness, sequence, checksum, size, and age validate. Floor deltas are
   reconciled with the matching generation, then recovery keys are cleared.
2. **Copyover recovery** -- only with `-C` boot flag / copyover flow.

If Redis recovery fails, the server continues with a normal boot state. Check
`logs/log/status` for `Performing redis crash recovery...` lines after any
crash, and verify player integrity before reopening.

For queue or dependency incidents, use `world persistence` and the detailed `redis`
status command. Do not clear a player save queue: player state is owned by the local
revision coordinator and journal, not a Redis dirty set. A world generation publish
failure preserves the prior current generation and retains floor deltas for retry.

### Known-benign log lines

These are investigated and understood; they are not signs of a failed boot.

| Line | Meaning |
|---|---|
| `Heaven has invalid number: 1 (should be 0)` | `recalc_zone_numbers()` finding and correcting a zone number that disagrees with its lowest room vnum. Self-healing; fixing the data would be zone-numbering surgery with a wide blast radius. |
| `PERSISTENCE: worker_unavailable_flat_fallback` (a few lines at boot) | Item events fired during world load are written to the flat fallback and replayed before the workers start -- followed by `replayed N fallback persistence events; 0 remain queued`. Working as designed. |
| Mob log `RIDICULOUS damage` / `M cmd not executed` | Area data, not engine defects. |

## Backups and maintenance scripts

| Script | Purpose |
|--------|---------|
| `scripts/backup_pfiles.sh` | Snapshot database or legacy player files (run automatically per cycle iteration; see the mode note below). |
| `scripts/delete_corpses.sh` | Inspect and, after confirmation, purge corpse rows and Redis corpse state. |
| `scripts/clear-redis.sh` | Drop database `0` on `redis-cli`'s default endpoint; it does not read `.env`, so use it only on a stopped, dedicated local Redis instance. |
| `scripts/import_help_to_prod.sh` | Import help sources to MySQL; use `--dry-run` first and treat `--clean` as destructive. |
| `scripts/migrate_players_to_accounts.sh`, `scripts/convert_all_pfiles.sh` | One-shot legacy data conversions; back up and review their assumptions before use. |
| `bin/migrations/*` | Offline pfile/schema conversion binaries built from `src-migrate/`. |

Schema operations follow the safety rules in [DATABASE.md](DATABASE.md):
back up, clone, validate replay on the clone -- never against live data.

`backup_pfiles.sh` chooses its database-dump branch only when `REDIS` is the
exact lowercase value `true` or the value `1`; the server itself accepts
case-insensitive `TRUE`. If the automatic backup must use `mysqldump`, set
`REDIS=true` in the environment used by the script and verify the resulting
`db/Backup/` file. Otherwise it falls back to the legacy `Players/Backup/`
layout.

## Runtime tuning

Server behavior knobs are exposed through the properties system:
`get_property()` (`src/properties.c`) binary-searches key/value pairs loaded
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

- Development: non-7777 port (e.g. 4000 via `--dev`), `duris_dev` database,
  `TEST_MUD` build.
- Production: port 7777, real TLS certificate linked as `duris.crt`/
  `duris.key`, hardened DB credentials in `src/sql.h` (requires rebuild),
  regular backups of MySQL + `Players/` + `Accounts/`.
