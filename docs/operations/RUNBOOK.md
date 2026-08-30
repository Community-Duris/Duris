# Operations Runbook

Day-to-day operation of a DurisMUD instance. First-time setup is in
[README.md](../../README.md).

## Starting and stopping

```bash
./scripts/start_mud.sh     # preferred: systemd user service if installed,
                           # otherwise nohup cycle_mud.sh -> logs/duris-console.log
./scripts/cycle_mud.sh     # foreground supervised run (what start_mud wraps)
./scripts/cycle_mud.sh --dev   # development listener/build role on port 4000
./scripts/cycle_mud.sh --production  # require the production role on port 7777
./scripts/cycle_mud.sh --check-config  # validate the selected persistence mode only
```

`cycle_mud.sh`:

- Anchors itself to the repository root; loads `.env` if present.
- Requires `ENVIRONMENT` in every mode. Database-backed modes also require `DB_HOST`,
  `DB_USER`, `DB_PASSWD`, `DB_NAME`, and `DB_ALLOWED_TARGETS`; `flatfile-primary`
  instead requires an absolute `FLATFILE_STATE_DIR`. It has no source-code credential
  fallback.
- Runs migrations, schema verification, and MySQL shutdown logging only for a
  database-backed mode. A `flatfile-primary` launch does not invoke those database
  tools, even when Redis is enabled.
- Raises core dump limits (`ulimit -c unlimited`).
- Rebuilds area tools and regenerates `areas/world.*` when the `make_*`
  helpers are missing.
- Regenerates `lib/misc/event_names` (demangled symbol list used by crash
  tooling).
- Promotes `bin/server/dms_new` to `bin/server/dms`, retains the five newest
  prior executables under `bin/server/history/` by default, and runs the active
  binary in an outer loop. Set `DMS_BINARY_HISTORY_LIMIT` to change the limit.
- On each restart it snapshots logs into `logs/old-logs/<timestamp>/`, writes
  the stop reason, runs `scripts/backup_pfiles.sh`, and optionally emails an alert.
  Database-backed modes create and validate an atomic MySQL backup and record
  boot/shutdown times plus the reason in the database. `flatfile-primary` instead
  snapshots `FLATFILE_STATE_DIR` beneath `FLATFILE_BACKUP_DIR` (default
  `backups/flatfile`). A backup failure in either mode stops the cycle before restart.

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

### Production systemd service

The checked-in production unit is rendered from
`deploy/systemd/duris-mud-production.service.in`. It is a system service, so it starts
from `multi-user.target` without a login session or user lingering. It runs under the
checkout owner, sets `Restart=always`, disables systemd's restart-rate limit, waits ten
seconds between attempts, and invokes `cycle_mud.sh --production`. The launch flag
refuses `ENVIRONMENT=local`; the unit cannot silently publish a development role as
production. A deliberate `systemctl stop` remains stopped because systemd suppresses
restart jobs requested by the service manager.

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
configuration preflight and refuses to proceed while another service owns port 7777.
The `--no-enable` staging path neither needs nor bypasses production credentials; the
service still enforces them whenever it is eventually started.

For the cutover, stop and disable any prior service that owns port 7777, then start the
production unit. Do not run the local and production units concurrently:

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

`DB_NAME` selects the database. `DB_ALLOWED_TARGETS` must explicitly allow that
name. The listener port is only a secondary safety guard: a non-production port
redirects a production-like name to the configured development target, but it is not
the primary database selector. A production role also requires the configured TLS CA
and a non-loopback database transport; a local/development/test role requires a
loopback target. The runtime applies a 10-second connection deadline before listeners
or persistence workers start and fails closed on identity, session-mode, schema, or
migration incompatibility.

Do not start the game if the target name, role, host, allow-list, TLS posture, or
backup status is uncertain. Qualify the exact target first; never probe a migration
script against a configured database to discover its command-line behavior.

### HTTP health probe

After startup, verify process and database-pool readiness without logging in:

```bash
scripts/healthcheck.sh
```

The probe targets `http://127.0.0.1:4050/health` by default. For an isolated local
instance, set `DURIS_WEBSOCKET_PORT` on the server and the matching
`DURIS_HEALTH_URL` for the probe. A healthy response is HTTP 200 with only
`status=healthy` and `database=ready`; the handler performs no database round trip.

## Logs

All under `logs/`; rotated per-run into `logs/old-logs/<timestamp>/`.

| File | Content |
|------|---------|
| `logs/log/status` | Boot progress, MySQL connection status, system messages |
| `logs/log/syslog` | Game events |
| `logs/log/cmdlog` | Player commands |
| `logs/log/wizlog` | Immortal commands |
| `logs/duris-console.log` | stdout/stderr of the supervised process |

In `flatfile-primary`, events sent through the database-backed audit logger remain
available in the ordinary files above: staff events use `logs/player-log/wizcmds`,
experience events use `logs/log/exp`, and player, quest, connection, and session events
use `logs/player-log/player`. Each entry retains its kind, player ID and name, IP, zone,
room, and message. Control characters are flattened so one event cannot forge another
log line.

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
   the current immutable generation is accepted only
   if schema, completeness, sequence, checksum, size, and age validate. Floor deltas are
   decoded with the matching generation into one semantic plan. Every item in each
   bounded tree must exactly match SQL UID, root, parent, room owner, VNUM, and active
   state before rollback-capable materialization; NPC-held items are not recreated. The
   exact restored generation is then consumed. A fenced one-use marker distinguishes
   clean restart from crash recovery.
2. **Copyover recovery** -- only with `-C` boot flag / copyover flow.

If Redis recovery fails, the server runs a full normal reset for every zone. Check
`logs/log/status` for `Performing redis clean restart recovery...` or
`Performing redis crash recovery...` followed by `applying full normal zone boot`, and
verify player integrity before reopening. The rejected generation and floor data are not
cleared by the failed restore.

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
| `scripts/restore_flatfile_backup.sh` | Verify one flat-file backup generation against its manifest and restore it into an empty state root. |
| `scripts/delete_corpses.sh` | Retired safety stub; exits nonzero without reading or changing MySQL or Redis. |
| `scripts/clear-redis.sh` | With the game stopped, use the scoped maintenance ACL identity to delete only the configured `REDIS_NAMESPACE`, legacy `mud:*`, and retired `ship:snapshot:*` keys from an explicitly confirmed, local, allow-listed Redis target; unrelated keys are preserved. |
| `scripts/import_help_to_prod.sh` | Import help sources to MySQL; use `--dry-run` first and treat `--clean` as destructive. |
| `scripts/migrate_players_to_accounts.sh`, `scripts/convert_all_pfiles.sh` | One-shot legacy data conversions; back up and review their assumptions before use. |
| `bin/migrations/*` | Offline pfile/schema conversion binaries built from `src-migrate/`. |

Schema operations follow the safety rules in [DATABASE.md](../reference/DATABASE.md):
back up, clone, validate replay on the clone -- never against live data.

In `flatfile-primary`, `scripts/backup_pfiles.sh` snapshots the complete selected
`FLATFILE_STATE_DIR` instead of inferring database use from `REDIS`. Its backup root
must be absolute and outside the state root. A missing state root on first boot is a
clean no-op; an unsafe target or failed copy stops the supervised launch.

The flat-file snapshot is a point-in-time generation, not a rolling copy. It holds the
same publication locks the server writes under (`identities/names/.identity.lock`,
`domains/.critical-authority.lock`, `identities/accounts/.accounts.lock`) for the whole
copy, waiting at most `FLATFILE_LOCK_WAIT` seconds (default 120) before failing rather
than publishing a mixed-generation backup. Each generation directory carries a
`MANIFEST.sha256` recording the generation id, capture time, source root, whether a
pending authority transaction was captured, and the digest of every file; the backup is
discarded if the state changed mid-copy or the copy does not match the source.

Restore with `scripts/restore_flatfile_backup.sh <generation-dir> <empty-state-dir>`. It
verifies the generation against its manifest, refuses a non-empty target root, restores
with owner-only modes, and re-verifies afterwards. If the manifest reports a pending
authority transaction, the server replays it on the next boot — boot the restored root
before comparing domain state.

### Migration procedure

All migration qualification is offline work on a disposable database or a restored,
backed-up development clone. Stop every writer before cloning. Record the source
backup identity, restore it under an explicitly non-production name on a loopback
host, and set `ENVIRONMENT`, `DB_HOST`, `DB_NAME`, and `DB_ALLOWED_TARGETS` so the
clone is the only permitted target. Keep the original backup untouched.

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

Keep the clone configuration separate from the server's `.env`; the legacy runner
loads the file named by `MIGRATION_ENV_FILE` and rejects symlinks, non-regular files,
and files readable by group or others. That file must describe the allow-listed
loopback clone and must not contain production credentials.

For a fresh disposable bootstrap, import `migrations/bootstrap_multithread_safe.sql`
and use `adopt --kind fresh_bootstrap` instead. The immutable runner rejects
production roles, non-loopback hosts, production-like names, unapproved targets,
manifest drift, incomplete baselines, or broken history. The compatibility verifier
is read-only but database-connected and must receive the same qualified clone target.
Never use production for migration qualification, compatibility probing, or replay.

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

`backup_pfiles.sh` always backs up the authoritative MySQL database; Redis
configuration does not select the backup mode. It requires the same explicit,
allow-listed database identity and transport safety used by the cycle. Dumps
are compressed into an owner-only temporary file, checked for the core Duris
schema, synced, and atomically published under `db/Backup/`. A dump,
compression, validation, or publication failure exits nonzero and leaves no
new backup. `DATABASE_BACKUP_DIR` may select another owner-only directory.

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

- Development: local/development/test `ENVIRONMENT`, loopback database host,
  explicit non-production `DB_NAME` in `DB_ALLOWED_TARGETS`, non-7777 listener
  (for example port 4000 via `--dev`), and a `TEST_MUD` build. The port does not
  select the database.
- Production: production `ENVIRONMENT`, non-loopback database transport with an
  absolute trusted `DB_SSL_CA`, explicit allow-listed database name, real listener
  TLS certificate linked as `duris.crt`/`duris.key`, secrets supplied through the
  protected environment or secret store, mode-0700 journal/state directories, and
  regularly restored and verified backups of MySQL, legacy player/account material,
  journals, and erasure tombstones. Credentials never belong in source files.

### Release boundary

This repository declares no production hosting provider, deployment trigger, service
account, or public URL. Release authorization and platform rollback are operator-owned
decisions made outside it. Repository-owned validation ends at the workflows in
`.github/workflows/`; their local equivalents are `./scripts/format.sh --check`,
`python3 tests/async/test_compiler_warning_profile.py`, `make test-all`, and
`make security-check`.
