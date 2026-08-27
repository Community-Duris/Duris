Short answer: make boot fail loudly on schema drift, instead of letting a bad query surface 200 lines into a log nobody reads.

There are three places to put that gate, cheapest first.

**1. Make `cycle_mud.sh` run the migration runner before exec'ing the binary (cheapest, ~5 lines).**

The runner already exists and is already safe — `scripts/migration_runner.py run` is idempotent, exits 0 with no output when nothing is pending, and its `MysqlExecutor.__init__` guard refuses anything that isn't loopback + non-production. So in the loop in `cycle_mud.sh`, right before `"$RUNTIME_BINARY" ${MUD_PORT}`:

```sh
if ! python3 scripts/migration_runner.py run; then
  echo "migrations not up to date; refusing to boot" >&2
  exit 1
fi
```

One caveat worth knowing: the runner reads `ENVIRONMENT`/`DB_HOST`/`DB_NAME` from `os.environ`, not from `.env` directly. That's exactly what bit the run earlier in this session — invoking it without sourcing `.env` made it report "migration target must be loopback non-production" *and still exit 0*. So this gate only works if `cycle_mud.sh` has already exported those (it does source `.env`), and you'd want to fix that exit-0-on-blocked behavior too, or the gate silently passes.

**2. Catch it at build/CI time with a source contract, which is the pattern this repo already leans on.**

That's essentially what `tests/async/test_saved_items_column_contract.py` now does for one table. The generalizable version: parse every `FROM <table>` / `INSERT INTO <table>` column list out of `src/*.c`, parse the canonical column set out of `migrations/bootstrap_multithread_safe.sql` plus the guarded `ALTER TABLE ... ADD COLUMN` statements, and assert every referenced column exists. That catches the entire class of bug — `item_condition` on `saved_items` was only one instance, and the cross-check run earlier showed the other ~19 call sites happened to be fine. This one needs no database at all, so it runs in `make test`.

**3. Add a startup schema-fingerprint assertion in the server itself.**

`migration_manifest.json` already carries `required_table_count` and `required_table_fingerprint`, and `mud_schema_baselines` stores the adopted fingerprint. Having `boot_db()` recompute the fingerprint and refuse to enter the game loop on mismatch turns silent drift into a hard boot failure. More work, but it's the only one that also protects a production box where nobody ran the tests.

**The deeper issue this particular bug exposes, though, is not migrations at all.** No migration was missing — the runner was fully applied and correct, and `item_condition` had *never* existed on `saved_items` in any schema file. The real defect was that `db_query()` returns `NULL` on failure and `sql_restore_saved_items()` does a bare `return;` on it. A schema gate wouldn't have caught this, because the schema was right and the query was wrong. So the highest-value single change is arguably:

```c
if (!result) {
    logit(LOG_SYSERR, "sql_restore_saved_items: query failed; ...");
    return;
}
```

Silent early-returns on failed persistence queries are how a boot-critical restore path went dead without producing a single `ERROR` line — the only trace was one `outcome=failure error_code=1054` record buried among ~100 `outcome=success` observability rows. Making failed queries log at a severity that a log scan actually surfaces would have made this obvious on the first boot after the db-refactor merged. That's worth auditing across the other `if (!result) return;` sites in `sql_player.c`.

If I had to pick one: **#2**, because it's free, needs no DB, runs in the existing suite, and catches the whole class rather than one instance.