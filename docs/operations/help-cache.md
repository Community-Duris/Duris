# Help catalog operation

MySQL help pages are loaded by one background refresh after world boot. Until
the first successful publication, help returns a temporary-unavailable message;
it never falls back to a synchronous query. Staff can retry a failed initial
load. No database migration is required.

After changing/importing help content, a greater god runs `page help` to queue
a refresh and `page help status` to inspect readiness, generation, pending work,
and the last error. These subcommands return before the legacy news/MOTD page
reload. A queued message means accepted, not completed: confirm the generation
advances. Concurrent refresh requests are refused. Failures preserve the entire
previous catalog. No automatic TTL or per-command database probe is used.

The worker borrows a pool connection, initializes its MySQL thread context,
reads ordered raw page data, and returns an immutable candidate. The game loop
publishes it during its regular completion pass. Shutdown joins the refresh
before the database pool shuts down. Pool acquisition and database socket timeouts
remain the existing pool defaults.

Limits: 20,000 pages, 256-byte titles, 128 KiB per complete page record, 32 MiB
total source data, 101 search results, and eight redirect lookups. Invalid,
empty, oversized, or incomplete catalogs (including a missing default `help`
page) do not replace working content. Source content and edit metadata are
cached; race/class/skill details still render from current game state.

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
`-pthread`, and `-lmysqlclient`, then set `TEST_DB_HOST` to the disposable server.
Its test-only login is root / cache-test. Never run it against an existing schema.
