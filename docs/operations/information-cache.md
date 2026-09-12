# Informational page cache

`credits`, `faq`, and `wizlist` read a memory snapshot instead of querying MySQL
or reading files during a command. Their former shared two-second cooldown is
removed. The first load is requested after world boot; until it completes the
commands report temporary unavailability, without a synchronous fallback.

Greater gods run `page info` after changing/importing these pages. Check
`page info status` until its generation advances: queuing is not completion.
The status includes readiness, generation, an outstanding refresh, and the last
failure. A failed initial load can be retried with the same command. Refresh
does not run the legacy news/MOTD reads. One refresh may be outstanding; concurrent requests coalesce into one follow-up
and supersede the earlier candidate. Each
complete candidate replaces the previous snapshot on the game thread. Missing
pages, SQL/file errors, or oversized content retain the previous snapshot.

All three pages must exist; an empty page is valid. Each page is limited to
128 KiB. The worker uses an isolated pooled MySQL connection with the pool's
existing acquisition/socket timeouts, or the flat-file information reader.
Shutdown joins the worker before database teardown. Pages handed to output are
owned strings; an active pager never borrows a replaceable cache pointer.

Idle workers refresh automatically every 60 seconds on both backends; failed loads
retain the old content and retry at the next interval. A blocked worker can delay
freshness. After editing `lib/information/{credits,faq,wizlist}`
in a flat-file build, run the same refresh command. In a MySQL build edit/import
the corresponding `mud_info` rows, then refresh. The maintained help importer now
writes credits/FAQ/wizlist to both `mud_info` and `pages` in the same transaction
as the rest of its import. No migration is needed.

Other `get_mud_info` callers were intentionally audited separately:

- Boot reads for news/MOTD/wizmotd remain existing boot behavior.
- The legacy plain `page` news/MOTD/wizmotd refresh remains unchanged.
- The account-creation `lock` read remains direct and keeps its existing
  authorization freshness.
- Generic `send_mud_info` also remains direct rather than silently changing
  unknown callers' freshness requirements.

This uses the same `refresh_cache` state machine introduced for help in #221.
Both help and information caches, including their staff refresh commands, are
part of the maintained server. The information commands add no burst limiter,
shared cooldown, or character wait; ordinary command scheduling still applies.

Validation: `python3 tests/async/test_information_cache.py` runs held-worker,
publication, command navigation, empty/missing/oversized flat-file content, and
owned-output tests. The isolated MariaDB harness is
`tests/async/information_cache_mysql_harness.cpp`, compiled with
`src/cmd/information_cache.c`, `-Isrc`, `-I/usr/include/mysql`, `-pthread`,
and `-lmysqlclient`.
Use only a disposable `cache_test` database without a `mud_info` table, with
root / cache-test and `TEST_DB_HOST`; the harness creates and drops its table.
It verifies failures and that 10,000 cached reads acquire no new connection.
These deterministic tests complement the live two-player journey below.

The SQL query replaces oversized content with NULL before transmission, so an
8 MiB source value cannot cause an 8 MiB client receive allocation. The flat-file
reader opens only regular files using nonblocking/close-on-exec descriptors,
checks their size, and caps actual bytes read even if a file grows after the size
check. Information refresh passes its 128 KiB cap into that reader rather than
applying it after the generic 8 MiB read. Three active pages and three candidates
can coexist (768 KiB of content plus container overhead). Replace individual live
files with atomic rename; concurrent in-place writes or multi-file edits do not
provide a transaction across all three filesystem sources.

The MariaDB harness additionally verifies wire-byte bounds and the actual periodic
refresh interval. The importer test covers atomic publication of the information
rows alongside help pages. Flat-file tests cover missing files, empty pages,
128 KiB boundaries, oversize rejection, FIFO rejection, and owned pager output.

## Live acceptance journey

Run the real server with two independently created mortal accounts:

```sh
python3 tests/async/test_information_cache_journey.py
```

The default builds an isolated flat-file executable using the shared regression
artifact helper. `--server /absolute/path/to/server` uses an already built binary
of the selected backend. The normal regression runner discovers this journey and
runs it in the resource-intensive partition, outside the parallel test batch.

For the SQL backend, explicitly select a disposable loopback database server:

```sh
TEST_DB_HOST=127.0.0.1 TEST_DB_USER=root TEST_DB_PASSWORD='<disposable-password>' \
  python3 tests/async/test_information_cache_journey.py --backend mariadb
```

The SQL journey creates a unique `information_journey_test_*` schema, applies the
normal fresh-bootstrap migrations, and drops that schema on exit. Both backends
use temporary world data, independent loopback client addresses, local listeners,
and private journals. The test never reads the checkout `.env` or existing player
state. Use LF line endings for the immutable migration files so their recorded
checksums match the repository manifest.

The journey checks all of the following through real command dispatch and output:

- Twelve credits/FAQ/wizlist sequences (36 information commands), each completing
  in less than two seconds and displaying the expected distinct content.
- A second player repeatedly requests `score`; timing starts at command submission
  and ends after the score-specific response and its prompt, preventing a stale
  prompt from producing a false latency measurement. Every response must complete
  within two seconds, with at least twelve samples.
- The first player opens `more credits` and leaves the real pager active while
  content changes and the normal 60-second automatic refresh publishes. The second
  player continues requesting scores and checks FAQ for the new generation.
- Continuing the first player's pager still displays the original generation.
  Quitting it and requesting each information command displays the new content.

The JSON result reports navigation and observer timing, sample counts, refresh
delay, and pager continuity. The observed refresh delay depends on where editing
falls within the periodic interval. These are local regression measurements with
two players and synthetic pages, not production-load guarantees. Deterministic
cache/SQL tests separately establish failure retention, bounds, and zero database
connection acquisitions for 10,000 cached reads; socket timings alone do not prove
the absence of synchronous I/O.

Closeout validation for #222 on master `ea16e31d7` plus the journey tests, in an
isolated Linux container (2026-09-11):

| Backend | Three-page sequence maximum | Observer score maximum | Observer samples | Pager survives refresh |
| --- | --- | --- | --- | --- |
| Flat-file | 0.751 s | 0.250 s | 62 | Yes |
| MariaDB 10.11 | 0.751 s | 0.251 s | 60 | Yes |

Both runs completed twelve navigation sequences, observed automatic publication,
and shut down normally. They used built server executables with the maintained
warning-as-error profile. No production deployment or data changes were part of
this validation.
