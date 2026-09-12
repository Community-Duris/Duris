# Production burn-in — 2026-09-12

Status: complete. The stopped clean qualification sequence and final live
burn-in passed after the code and production-service repairs. The MUD and web
backend are running and healthy.

The starting checkout was clean on `master` at `ea16e31d7`. The user authorized
the full burn-in, production migration and rollback, and committing and pushing
all eligible changes to `master`.

## Repairs

- Backups now freeze the selected runtime schema in each generation and verify
  database compatibility before and after capture. An owner-only deployed
  manifest can be selected for the backup preceding an upgrade. Historical
  dumps are verified against their recorded schema instead of the current
  checkout's table inventory.
- The restore-wrapper regression now uses an explicitly isolated environment
  file, preventing it from reading the host's production backup policy.
- The runtime-compatibility database fixture now derives files and history from
  the immutable manifest, applies and verifies every step twice, and tests a
  missing epic-stone claim table. Its old fixture stopped at migration 0011.
- The immutable-migration guide now identifies head 0012 and the 178-table
  runtime contract.

## Production preparation

The existing service is `systemctl --user`'s
`duris-mud-production.service`, invoking `scripts/cycle_mud.sh --production`.
It is enabled, and user lingering is enabled. The existing production binary,
matching source revision `694c7494d`, build stamp, service definition, and
maintenance-scheduler state were preserved outside `bin/` before cleaning.
The saved executable is a regular file and its SHA-256 matches the original
running executable; all embedded schema fingerprints and migration checksums
match the preserved source contract.

The MUD and the identified web database writer were stopped through their
existing user services. Their processes and listeners closed, and the database
reported zero other connections. A fresh full backup was restored into an
isolated MariaDB container; migration application, compatibility verification,
and a no-op replay passed there before production was changed. The complete
pre-migration generation is preserved outside backup rotation.

Production `127.0.0.1:3307/duris` advanced from `0011_player_death_disposition` to
`0012_epic_stone_claim` through the guarded runner with its explicit target and
fresh-backup arguments. The read-only runtime verifier passed against the
178-table contract. A post-migration full backup also passed. The web service
was restored and its database/cache health checks passed.

An owner-only local backup policy is configured with an hourly schedule, a
two-hour recovery-point objective, 48 hourly/14 daily/8 weekly retention
buckets, and a 20 GiB budget. Remote replication is not configured. The
independent user timer `duris-backup-backup.timer` is enabled and active. It
evaluates the hourly policy every minute; its first scheduled full capture
succeeded, and `scripts/backup_pfiles.sh status` returned `result=ok`. No
production restore-drill receipt is claimed; the explicit recovery integration
suite used synthetic isolated fixtures.

## Validation

The canonical sequence uses the production build profile and the existing
user-local dependency headers/libraries recorded in the private `build-env.sh`:

```sh
python3 scripts/migration_runner.py inspect
./migrations/verify_runtime_compatibility.sh
./scripts/format.sh --check
make clean-all
make -j4 BUILD_PROFILE=production
make test-all BUILD_PROFILE=production \
  PYTHON="python3 /home/duris/.local/state/duris-burnin-20260912/capture_python.py"
make test-db
```

Successful Python-test output is also retained by a recording wrapper around
the normal runner; the wrapper does not change its tests or assertions.

| Gate | Result |
| --- | --- |
| Fresh production server, area editor, and area-tool build | Final clean build passed without compiler diagnostics |
| Complete Python and native regression gate | Final: 471 Python files passed, zero failures (1616.41 s); native signal-handler regression passed |
| Canonical isolated MySQL gate | All ten isolated wrappers passed, including the 143-step legacy upgrade, replay, bootstrap equivalence, and runtime compatibility |
| Runtime compatibility, replay, and drift rejection | Repaired fixture passed on MySQL 8 and MariaDB 10.11 |
| Eight additional Docker database wrappers | Passed |
| Critical-command MariaDB and currency MySQL parity | Passed |
| Ten externally provisioned database wrappers | Passed against disposable databases; player-load fixture used the migrated clone |
| Epic-stone atomic award/replay/rollback database test | Passed on disposable MariaDB |
| Atomic help-import database test | Passed on disposable MariaDB |
| Actual database-backed combat journeys | All three variants passed: ordinary coins, reset coins, and boons enabled |
| Locker receipt/payment crash recovery | Passed on both flatfile and MariaDB, using wallet and bank payment |
| Explicit recovery integration | All eight tests passed, including both persistence backends, journal replay, and isolated service boot |

Additional Docker wrapper names were `account_bank_delta`,
`account_locker_conversion_check`, `combat_baseline_repair`,
`critical_command_schema`, `currency_transaction_schema`,
`legacy_personal_locker_access_repair`, `player_death_disposition`, and
`player_replacement_state`, each run as
`tests/async/run_<name>_mysql.sh`. Runtime, critical-command, and currency
parity used `RUNTIME_DB_IMAGE=mariadb:10.11`,
`CRITICAL_DB_IMAGE=mariadb:10.11`, and `CURRENCY_DB_IMAGE=mysql:8.0`.

The externally provisioned wrappers were `account_character_identity`,
`account_character_projection`, `artifact_guild_schema`,
`auction_transaction_schema`, `boon_reward_zone_schema`,
`combat_outcome_schema`, `epic_transaction_schema`,
`item_transfer_schema`, `player_load_repository`, and `session_audit_schema`.
They ran from an isolated source copy with clone-only configuration. No test
wrapper was pointed at the configured production database.

Optional journeys were explicitly enabled with `EPIC_STONE_MYSQL_TEST=1`,
`TEST_DB_HOST` and the synthetic database credentials, or
`DURIS_RUN_BACKUP_INTEGRATION=1`. Recovery ran in an isolated Ubuntu 24.04
Docker container with the namespace/mount capabilities the tests require.
The host itself cannot create those namespaces. An initially missing container
runtime library was installed before the successful database journeys. The
player-load harness also required explicit account/character identity from the
migrated clone; its complete rerun then passed. Both disposable migration-clone
containers were removed after qualification. The preserved rollback generation
and executable remain available outside build cleanup and backup rotation.

The explicitly enabled Python entry points were:

```sh
python3 tests/async/test_epic_stone_transaction.py
python3 tests/async/test_help_import_atomic.py
python3 tests/async/test_mysql_combat_journey.py
python3 tests/async/test_locker_receipt_recovery.py
python3 tests/async/test_persistence_backup_integration.py
```

Default-run skips for these external prerequisites were covered by the above
passing, isolated runs; they were not counted as exercised database branches
merely because the default Python file exited successfully.

## Production service ownership correction

The first live launch found a host configuration incompatibility. The custom
user service's namespace settings implicitly enabled `PrivateUsers`. A probe
under those exact restrictions showed UID 1021 mapped only to itself; the
root-owned `/` and `/home` ancestors appeared as UID 65534. The backup guard
correctly refused the environment file with `unexpected_owner`. The independent
backup timer, which retained host UID visibility, had already passed.

A host-only `backup-ownership.conf` drop-in for the existing production user
service disables `PrivateUsers`, `PrivateTmp`, `ProtectSystem`,
`ProtectKernelTunables`, `ProtectControlGroups`, and `ProtectHostname`, and uses
`ProtectProc=default` and `ProcSubset=all`. The same probe then passed with the
host UID map. This custom user service no longer uses namespace-based
isolation; ordinary filesystem permissions, `NoNewPrivileges`, and the existing
SUID, namespace-creation, realtime, personality, address-family, and syscall
architecture restrictions remain. The authoritative ownership checks were not
weakened. The repository's root-managed system-service template is unchanged.

The corrected service passed its required full pre-boot backup, artifact and
callback-map checks, all three listeners, verified-TLS staff login and ten safe
inspection commands, clean logout, and health. A further stopped clean
qualification sequence passed after this operational correction and before the
final live verification. Before stopping for that repeat, the healthy head-0012
executable was copied from `/proc/<game-pid>/exe`, its SHA-256 was verified, and
its build stamp, source revision/diff, identity evidence, and updated maintenance
state were preserved. This compatible artifact can restart against the migrated
database without rewinding to the original head-0011 backup.

## Web reconnect repair

The restored web backend restarted once during MUD maintenance after an
uncaught `RangeError: Invalid time value`. Its broad process-name search picked
up isolated test servers, and a separate uptime lookup could return empty output
when a process exited. The process monitor now requires the configured checkout
and executable identity, accepts only one candidate, and reads validated finite
uptime from the same process snapshot. Zero-duration CPU samples also stay finite.

The tested backport is DurisWeb commit `e223840`, deployed through the existing
`durisweb-production.service` with the installed release's dependencies. Its
previous compiled release is preserved for rollback. The same monitor logic is
published on DurisWeb `master` at `9925dbd`, integrated with that branch's newer
dependency baseline in an isolated worktree. The deployment branch is also
published as `burnin/production-uptime-20260912` so its exact source is retained.
No web migrations were run.

Validation passed on the deployment base (15 focused cases) and current web
master (21 cases, including the earlier identity regressions):

```sh
pnpm --dir backend test --runInBand processMonitorSnapshot
pnpm --dir backend format:check
pnpm --dir backend lint
pnpm --dir backend type-check
pnpm --dir backend verify:mud-writes
pnpm --dir backend exec tsc --outDir .burnin-dist-stage
```

The master integration used `test --runInBand processMonitor` and the standard
`pnpm --dir backend build`. Both build and type-check passed. The staged release
passed production configuration/dependency preflights and a read-only probe that
selected the actual MUD PID. After cutover, database/cache health, authenticated
MUD WebSocket reconnection, and hook-state synchronization passed with zero web
restarts. The MUD stayed running throughout this separate web repair.

## Live verification

The final observation interval was `2026-09-12T03:22:24.337571+00:00` through
`2026-09-12T03:48:10.397535+00:00`. Monitoring followed all regular files under
`logs/log/`, including new files and rotated open handles, plus
`logs/duris-console.log` and the production user-service journal. Covered game
logs were `artifact`, `cmd.debug`, `comm`, `debug`, `exit`, `file`, `kingdom`,
`mob`, `status`, and `sys`. The console file had no new bytes; this service sends
console output to journald.

The service started at `Sat 2026-09-12 03:22:35 UTC` and reached
healthy/persistence-ready state. The supervisor PID was `721481`;
the game PID was `723574`. The final executable SHA-256 was:

```text
e511668024732aebf3c15a6cb5176643c09461f45babdc814134b0abcd4138ea
```

The loaded `/proc/<game-pid>/exe` matched the fresh candidate and runtime file,
the build stamp was `mariadb/production`, and `lib/misc/event_names` exactly
matched the launcher's `nm --demangle | grep " T " | sed` output. The game owned
plain port 7777, verified-TLS port 4001, and loopback WebSocket/health port 4050.
Host UID visibility and `NoNewPrivileges` were verified in the running process.

The configured staff account logged in over certificate-verified TLS and ran
these randomized safe commands, in order:

```text
attributes
score
users nonplaying
inventory
equipment
users
look
who
time
exits
```

All responses were sensible. Gameplay quit, account logout, socket closure,
and health checks passed at `2026-09-12T03:23:48.384109+00:00`. Monitoring continued well
beyond the required two-minute post-logout soak, including the web cutover.
The MUD retained the same PIDs with `NRestarts=0`; no new core files, assertions,
crashes, or persistence failures were observed.

Expected profiling and event-budget/catch-up records were reviewed. One
25.315 ms budget report used the existing `unknown function` label fallback;
its 856 deferred callbacks cleared on the next tick. The callback map matched
the executable, and this was a normal budget diagnostic, not a failed check.

Both production user services are enabled and healthy. The independent hourly
backup timer is enabled/active, and the final backup status is `ok`. Backups and
rollback artifacts remain local; remote replication and a production restore-
drill receipt are not claimed. The explicit isolated restore suites passed.

Private command output, rollback identities, full test outputs, log captures,
and sanitized smoke evidence are retained under
`/home/duris/.local/state/duris-burnin-20260912/`. Credentials, player data,
raw logs, backups, local service configuration, and generated binaries are
excluded from Git.
