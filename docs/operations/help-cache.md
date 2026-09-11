# Help catalog operation

MySQL help pages are loaded by one background refresh after world boot. Until
the first successful publication, help returns a temporary-unavailable message;
it never falls back to a synchronous query. Staff can retry a failed initial
load. No database migration is required.

After changing/importing help content, a greater god runs `page help` to queue
a refresh and `page help status` to inspect readiness, generation, pending work,
and the last error. These subcommands return before the legacy news/MOTD page
reload. A queued message means accepted, not completed: confirm the generation
advances. Concurrent refresh requests coalesce into one follow-up load; the earlier candidate
is discarded so it cannot satisfy a later refresh request. Failures preserve the entire
previous catalog. An idle worker refreshes automatically every 60 seconds. Failed automatic loads
retain the old catalog and retry on the next interval; no per-command database probe
is used. A blocked load can delay this freshness interval.

The worker borrows a pool connection, initializes its MySQL thread context,
reads ordered, server-bounded page data, and returns an immutable candidate. The game loop
publishes it during its regular completion pass. Shutdown joins the refresh
before the database pool shuts down. Pool acquisition and database socket timeouts
remain the existing pool defaults.

Limits: 20,000 pages, 256-byte titles, 128 KiB per complete page record, 32 MiB
total source data, 101 search results, and eight redirect lookups. Invalid,
empty, oversized, or incomplete catalogs (including a missing default `help`
page) do not replace working content. Source content and edit metadata are
cached; race/class/skill details still render from current game state. SQL limits the
returned row payload and aggregate source bytes before libmysql allocates receive
buffers. The active and candidate catalogs can coexist (up to 64 MiB of source data
plus bounded container/string overhead); this is not a 32 MiB process-memory cap.

Search retains case-insensitive matching and `%`/`_` wildcards. ASCII help titles
are matched in-process; database-specific accent/collation equivalences are not
emulated. Redirects support complete multiword titles and fail safely on cycles.

The flat-file build retains its existing startup catalog and local file behavior;
`page help` reports that it uses the startup catalog. Restart after flat-file
help changes. Both builds allow normal repeated help/info/rules browsing without
`help.cooldown.secs` or character recovery. Existing input/output bounds still
apply. The obsolete property is ignored if retained in a private properties file.

Validation includes a held-worker test allowing 10,000 simulated other-player
operations, actual help lookup/render regression bodies, and a MySQL loader
harness verifying publication, metadata, refresh failure, bounds, and no new
connection acquisition for repeated reads. This is deterministic pipeline testing,
not a live two-player latency benchmark.

Run `python3 tests/async/test_help_cache.py`,
`python3 tests/async/test_flatfile_help_catalog.py`, and
`python3 tests/async/test_reported_latency_contract.py`.
For the isolated MySQL harness, use a disposable database named `cache_test`
with no `pages` table (the harness creates and drops it): compile
`tests/async/help_cache_mysql_harness.cpp` with `src/cmd/help_cache.c`, `-Isrc`,
`-I/usr/include/mysql`, `-pthread`, and `-lmysqlclient`, then set `TEST_DB_HOST` to the disposable server.
Its test-only login is root / cache-test. Never run it against an existing schema.

The maintained `scripts/import_help_to_prod.sh` stages all three import sections
and optional cleanup into one SQL transaction. It requires existing InnoDB `pages`
and `mud_info` tables, replaces `TRUNCATE` with transactional `DELETE`, and commits
only after all source generation and SQL statements succeed. A SQL error closes
the client connection without COMMIT, rolling back the whole import. The help
loader uses one consistent SELECT, so a refresh sees the old or new committed
catalog, not intermediate deletes/inserts. Direct external editors must likewise
use an InnoDB transaction for multi-statement edits; arbitrary autocommit scripts
cannot provide this atomicity. Such committed edits are discovered by periodic
refresh, or by `page help` after committing.

`TEST_DB_HOST=<disposable-host> python3 tests/async/test_help_import_atomic.py`
checks SQL-error rollback, reader visibility during a delayed three-section import,
and rejection of nontransactional tables. The loader harness also measures response
bytes for oversized rows, aggregate content, and page counts, and verifies the real
60-second automatic refresh interval. Both DB tests use test-only root/cache-test
credentials. Import execution explicitly disables mysql's continue-on-error option
and enables strict SQL and literal backslash handling for generated title literals.
