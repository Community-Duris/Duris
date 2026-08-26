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
- Runs `./dms <port>` in an outer loop, interpreting the exit code to decide
  whether to restart.
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

## Crash recovery

Two automatic paths run at next boot after an unclean exit:

1. **Redis world-state recovery** — if a snapshot exists, world state
   (including combat) is restored, then the snapshot is cleared.
2. **Copyover recovery** — only with `-C` boot flag / copyover flow.

If Redis recovery fails, the server continues with a normal boot state. Check
`logs/log/status` for `Performing redis crash recovery...` lines after any
crash, and verify player integrity before reopening.

## Backups and maintenance scripts

| Script | Purpose |
|--------|---------|
| `scripts/backup_pfiles.sh` | Snapshot database or legacy player files (run automatically per cycle iteration; see the mode note below). |
| `scripts/delete_corpses.sh` | Inspect and, after confirmation, purge corpse rows and Redis corpse state. |
| `scripts/clear-redis.sh` | Drop the entire selected Redis database; use only on a stopped, dedicated development Redis instance. |
| `scripts/import_help_to_prod.sh` | Import help sources to MySQL; use `--dry-run` first and treat `--clean` as destructive. |
| `scripts/migrate_players_to_accounts.sh`, `scripts/convert_all_pfiles.sh` | One-shot legacy data conversions; back up and review their assumptions before use. |
| `src-migrate/*` | Offline pfile/schema conversion binaries. |

Schema operations follow the safety rules in [DATABASE.md](DATABASE.md):
back up, clone, validate replay on the clone — never against live data.

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

## Development vs production checklist

- Development: non-7777 port (e.g. 4000 via `--dev`), `duris_dev` database,
  `TEST_MUD` build.
- Production: port 7777, real TLS certificate linked as `duris.crt`/
  `duris.key`, hardened DB credentials in `src/sql.h` (requires rebuild),
  regular backups of MySQL + `Players/` + `Accounts/`.
