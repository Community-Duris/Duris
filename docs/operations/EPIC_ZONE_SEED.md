# Source-derived epic-zone payout seed

Issue: [#514](https://github.com/Community-Duris/Duris/issues/514).

`python3 scripts/epic_zone_seed.py check` derives the seed from tracked repository
content. The generator **never connects to a database**. Its SQL output is an explicit,
reviewable operator artifact, not a boot-time repair or an automatic migration.
Applying it to production requires separate deployment/data-change authorization.

## Authority and coverage

- `migrations/epic-zone-payout.sql` supplies the explicit historical payouts and
  the group-size policy of 100. The generator accepts only its known grammar;
  duplicate assignments or an unrecognized statement are errors.
- `areas/AREA` and its `areas/zon/*.zon` files identify active zone numbers,
  names, and stone load directives. Object numbers (currently 358, 359, 360)
  and type values are derived from `src/world/epic.h`, not a parallel mapping. Generated
  `areas/world.zon` is not an input; a clean checkout is sufficient.
- `migrations/seeds/epic_zone_payouts.json` is the deterministic, reviewable
  manifest. It contains input SHA-256 hashes, zone coverage, and exceptions.
  No private backup or player/account data is needed to regenerate it.
- At introduction: 119 stone load directives in 110 zones; **107 positive
  payouts**. Ironstar (1389) is explicitly disabled. Ravenloft Catacombs (590)
  and Grumbar's Domain (1312) have no source payout: their manifest payout is `null`,
  **not an invented zero-valued policy**. The generator reports assignments
  with no active stone load separately and does not seed them. Currently this
  includes zone 4200 (historical payout 850), also visible in the SQL audit.
- A historical production dump dated 2026-06-23 corroborated all 107 positive
  payouts and group size 100. It was evidence, not the seed input. This does
  not establish when or how current deployments lost their configured values.

The seed is deliberately independent of the sealed **schema** manifest and
bootstrap checksums. It changes data only. Do not reseal historical schema
migrations, modify the legacy payout SQL, or replay the full legacy migration
merely to repair these values. The old SQL is still an unconditional legacy
policy update and does not have this seed's override-preservation semantics.

## Exact mutation contract

Only source-known, positive-payout zones are candidates:

1. Missing row: insert its source number/name/stone type, payout, and group size.
   Other columns retain schema defaults. This supports an empty fresh `zones`
   table **before first game boot**, avoiding an UPDATE-only seed that affects
   zero rows. Normal `update_zone_db()` subsequently finds those rows by number,
   updates names/types as before, and inserts other world zones.
2. Existing row with **both** `epic_payout=0` and `suggested_group_size=1`: set
   those two fields to the known pair. All other fields remain unchanged.
3. Any other existing pair: preserve it exactly, including positive tuning,
   zero payout with a non-default group size, and partially configured pairs.
   Audit reports it as `override-preserved` unless it already matches.
4. Disabled/unresolved zones: no insert and no update, regardless of their
   current values. Audit displays their current configuration for review.

**An intentional `(0,1)` override is indistinguishable from uninitialized
schema defaults.** Applying the seed explicitly authorizes repairing those
exact pairs for the known zones. Inspect the audit and confirm operator intent
first; do not use the seed to overwrite deliberate default-shaped exclusions.
This is why it is never run automatically at boot.

The SQL binds an exact database name, requires an InnoDB `zones` table, and
refuses duplicate zone numbers (the schema's number index is not unique).
A serializable transaction takes a `FOR UPDATE` lock before checking and
inserting missing entries. This is defense-in-depth: the access plan is
optimizer-dependent, the number index is not unique, and a later writer can
still introduce duplicates. **Stopped server and no other writers are the
primary concurrency guarantee.** Guard failure raises a duplicate-key error before
DML; the DML also tests the guard. Do not use the mysql client's `--force`:
a later SQL failure must end the session and roll back, not continue to COMMIT.
The temporary staging/guard tables disappear when the session ends.

No stone claims, touch history, player balances, items, or durable reward
records are modified. Reapplying the seed does not recharge a stone or pay a
player. Reboot/recharge alone cannot repair a zero payout.

## Read-only preflight

From the repository root:

```bash
python3 scripts/epic_zone_seed.py check
python3 scripts/epic_zone_seed.py sql-audit > /secure/path/epic-zone-audit.sql
```

Use an owner-only reviewed configuration for the intended host/port/user/database.
For example, with those `DB_*` values already loaded from the protected config:

```bash
MYSQL_PWD="$DB_PASSWD" mysql --host="$DB_HOST" --port="${DB_PORT:-3306}" \
  --user="$DB_USER" --batch "$DB_NAME" < /secure/path/epic-zone-audit.sql
```

Use the deployment's required TLS options for remote connections; prefer the
host-local approved connection rather than weakening transport verification.
Confirm the exact host and database independently, not just the filename.
The read-only audit returns **all 110 active stone-bearing zones plus the
excluded legacy assignment for 4200**, with actual row count,
actual values, expected values, and one of:

- `matches`: already configured as the seed.
- `default-repairable`: exact `(0,1)` pair, subject to operator intent review.
- `missing`: known positive zone row absent; seed can insert it.
- `override-preserved`: different existing pair; investigate, do not overwrite.
- `duplicate-number`: ambiguous database rows; resolve separately before apply.
- `disabled-no-seed` / `unresolved-no-seed`: explicitly outside mutation scope.
- `outside-active-source-no-seed`: known legacy assignment with no stone load
  in the active source set; displayed for coverage review, never seeded.

These no-seed statuses are source classifications, not assertions that the
live configuration is safe; inspect their displayed values too. New unknown
zones require a separately reviewed balance decision, not a guessed payout.

## Explicit application (fresh setup or approved repair)

1. Review preflight and source manifest. For an existing database, take and
   verify a fresh backup of `zones` (schema and rows) before applying; keep its
   exact target identity, timestamp, and checksum. Do not use the historical
   production archive as a current rollback snapshot.
2. Stop the game through its supported launcher and prove the target has no
   active game process/writers. For a fresh setup, do this after schema setup
   and before the first boot. The generator's `--server-stopped` is an operator
   acknowledgment, **not process detection**. Coordinate other admin writers.
3. Generate and inspect the target-bound SQL. `DB_NAME` is restricted to letters,
   digits, and underscores. No wildcard targets or fallback database are used.

```bash
python3 scripts/epic_zone_seed.py sql-apply \
  --database "$DB_NAME" --server-stopped > /secure/path/epic-zone-apply.sql
# Inspect the SQL before proceeding. This next command is the actual mutation:
MYSQL_PWD="$DB_PASSWD" mysql --host="$DB_HOST" --port="${DB_PORT:-3306}" \
  --user="$DB_USER" "$DB_NAME" < /secure/path/epic-zone-apply.sql
```

4. Require a zero client exit code. The SQL reports repaired and inserted row
   counts; **read back the audit** on the exact same target. Known zones should
   be `matches` or the previously reviewed preserved overrides, never silently
   unresolved default/missing/duplicate entries. Compare exceptions and
   unrelated columns/rows against the preflight snapshot.
5. Restart normally to load the values into `zone_table`; inspect fresh logs
   and exercise an eligible stone/player journey under separate live-test
   authorization. No runtime gameplay acceptance is implied by SQL success.

Do not apply the broad `migrations/epic-zone-payout.sql` as a substitute: it
unconditionally replaces known payouts and broadly changes group sizes.

## Failure and rollback

If the mysql client exits on a SQL error before COMMIT, connection teardown
rolls back this InnoDB transaction. Inspect/read back before retrying. The
isolated regression injects a late insert failure to verify that earlier payout
updates are rolled back. An error after COMMIT does not undo a committed seed;
the audit, not an assumed exit status, is the final data authority.

After a successful commit, rollback is an **explicit stopped-server data
operation** using the verified pre-application snapshot. Restore original
payout/group pairs by exact row identity; remove only rows proven inserted by
this application and still unchanged. Preserve any intervening tuning or
state. If intervening writes occurred, reconcile them rather than restoring an
old whole-table dump blindly. Never reset claims/history as a rollback step.

## Maintainer and regression commands

```bash
# After an intentional tracked source change, review both input and manifest diff:
python3 scripts/epic_zone_seed.py write
python3 scripts/epic_zone_seed.py check
python3 tests/async/test_epic_zone_seed.py
python3 tests/async/run_epic_zone_seed_mysql.py --image mysql:8.0
python3 tests/async/run_epic_zone_seed_mysql.py --image mariadb:11.4
```

Both SQL tests create uniquely named disposable containers with no external
network or published ports, use the actual bootstrap `zones` schema and
source-derived zone records, and delete their own containers/anonymous volumes
in `finally`. They do not read `.env` or use a private dump. Readiness requires
an authenticated TCP query inside the container, avoiding the image entrypoint's
temporary initialization server. Tests cover exact known values, exceptions,
non-default/partial/zero overrides, empty-table insertion, replay, wrong target,
duplicate numbers, nontransactional engine refusal, and late-failure rollback.
