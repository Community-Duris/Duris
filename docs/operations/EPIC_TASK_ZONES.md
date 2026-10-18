# Curated epic-task zones

Issue [#71](https://github.com/LuminariMUD/Duris/issues/71) has two independent
requirements: ready player history and a nonempty, valid task-zone catalogue.
The pending affect saves an earned task while either is unavailable. It cannot
create missing configuration.

## Roster authority

`migrations/seeds/epic_task_zones.json` preserves the **107 explicit task-zone
flags** from the owner-supplied production archive
`DurisMUD-production-database-2026-07-12.tar.xz`, SHA-256
`e3bfb5de3348f04893fc0916e9da6b7095ff0b7d184f34903815d1c9f649b9a7`.
The `prod.sql` member records `Dump completed on 2026-06-23 23:24:20`; the
filename is not the dump date. Its `zones` table contains 355 unique numbers.
Only eligibility numbers and provenance are committed, never the private dump.

`python3 scripts/epic_task_seed.py check` validates every curated number against
active `areas/AREA` sources and their stone loads. It reuses the existing zone
parser. Neither stone presence nor payout amount grants task eligibility; new
zones require a reviewed roster change. In particular 590, 1312 and 1389 have
stones but no historical task flag and remain excluded by this seed.

## Read-only preflight

```bash
python3 scripts/epic_task_seed.py check
python3 scripts/epic_task_seed.py sql-audit > /secure/path/task-zone-audit.sql
MYSQL_PWD="$DB_PASSWD" mysql --protocol=tcp --host="$DB_HOST" --port="$DB_PORT" \
  --user="$DB_USER" --batch "$DB_NAME" < /secure/path/task-zone-audit.sql
```

Load credentials privately from the intended deployment and retain its normal
TLS configuration. The audit reports each curated zone as enabled, disabled,
missing, duplicate or invalid, plus enabled zones outside the roster. Review
disabled entries: intentional exclusions are indistinguishable from lost
defaults. Do not apply the historical roster over a deliberate policy change.

Production on 2026-10-16 had 350 zone rows and no enabled task flags. Its logs
showed `catalog_ready=0 zones=0 player_reads_ready=0` on one deferral. Restoring
flags alone cannot correct unavailable player history. First-session history is
initialized by PR #119; an existing pending character reloads its durable history
on a normal login. Do not zero a returning character's history to make it ready.

## Reviewed provisioning or repair

The generator never connects to a database. Application is an explicit operator
action requiring production authorization and a verified fresh backup of `zones`
(schema and rows, target identity, timestamp and checksum).

1. On fresh setup, create the schema and populate the world's zone rows through
   normal zone synchronization before applying. A missing/empty table is refused;
   this seed does not create partial zone rows. Preserve this configuration table
   in backups/restores. Schema creation alone intentionally defaults flags to zero.
2. Review the roster, current-world check and live audit. Generate target-bound SQL:

   ```bash
   python3 scripts/epic_task_seed.py sql-apply --database "$DB_NAME" \
     > /secure/path/task-zone-apply.sql
   ```

3. After approval, execute that reviewed file with the same target and fail-fast
   client as the audit. **Never use `mysql --force`.** The transaction requires
   InnoDB, unique zone numbers, and exactly one existing row with a boolean flag
   for every curated zone. It sets only those flags from 0 to 1. It preserves
   existing enabled flags, every non-roster row, payouts, claims, timestamps,
   player history, balances and tasks. Reapplication is a no-op.
4. The transaction locks `zones` under serializable isolation with a five-second
   InnoDB lock-wait limit. Current runtime writers do not change task flags.
   This small configuration update can be applied while the game runs; lock
   contention is a failed attempt to investigate, not a reason to stop the game.
   An error before COMMIT rolls back on client disconnect. Read back after any
   ambiguous result; an error after COMMIT does not undo the update.
5. Rerun the audit and compare the other fields to the backup. Normal scheduled
   catalogue maintenance publishes the flags; a restarted game also loads them
   at boot. No restart is necessary solely to refresh task eligibility. Use the
   observed refresh logs/timing rather than promising an immediate assignment.
   Its 14,400-event-tick cadence is nominally **one hour** at four ticks/second,
   not four hours. The initial offset depends on listener ports; busy loops and
   retries can delay publication.

Trusted staff can run `world persistence`. The `epic_tasks` line distinguishes
`catalog_ready`, active `zones`, `last_refresh`, in-memory `pending` tasks and
`pending_reads_unavailable`. `last_refresh=empty_task_flags` means no configured
rows; `unloaded_zone` or `invalid_scheduled_rows` points to configuration
validation; `database_query_failed` / `scheduled_read_failed` points to the read
path. A failed refresh may coexist with `catalog_ready=1`: the last valid snapshot
is deliberately retained. Logs report changed outcomes/counts without per-tick
spam. No player identifiers are included in this health output.

Rollback is a separately authorized correction of only flags proven changed by
this application, by exact row identity and after checking intervening edits.
Do not restore an entire old database or clear rewards/history to undo flags.

## Acceptance and limits

After separately approved operations, verify the already-pending character gets
exactly one real errand without touching another stone or earning extra epics.
Confirm the original 400 epics, completed-zone exclusions, save/rent/relogin and
restart behavior, then task variety for zero/some/all completed zones. Keep #71
open until production evidence is recorded.

Focused checks: `test_epic_task_seed.py` validates roster/CLI behavior;
`run_epic_zone_seed_mysql.py --image mariadb:11.8` (also `mysql:8.0`) exercises
the exact generated SQL, replay, excluded/unrelated fields, missing and duplicate
rows, wrong target/engine, and rollback after an injected update failure.
These disposable-database checks do not establish live player recovery.
