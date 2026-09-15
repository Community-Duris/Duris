# Administrator telemetry reports (#269)

This document describes the repository-local report interface built on the
frozen #268 rollup outputs. It is a read-only administrator/reporting path, not
a game command and not a second ingestion or schema system.

## Boundary and connection policy

Use `scripts/telemetry/report.py`. It creates one dedicated connection from
only the `TELEMETRY_REPORT_DB_*` namespace:

```text
TELEMETRY_REPORT_DB_HOST
TELEMETRY_REPORT_DB_PORT       (optional, default 3306)
TELEMETRY_REPORT_DB_DATABASE
TELEMETRY_REPORT_DB_USER
TELEMETRY_REPORT_DB_PASSWORD   (or TELEMETRY_REPORT_DB_PASSWD)
TELEMETRY_REPORT_DB_TLS_VERIFY
TELEMETRY_REPORT_DB_SSL_CA
TELEMETRY_REPORT_DB_SECURE_TUNNEL
TELEMETRY_REPORT_CACHE_DIR     (optional cache directory)
```

There is no password option and no fallback to the game connection settings.
The report role must have `SELECT` only on the published aggregate tables:
`telemetry_rollup_state`, `telemetry_rollup_session`,
`telemetry_cohort_day`, and `telemetry_cohort_member`. It must not have access
to the immutable fact stream, gameplay tables, or schema mutation privileges.
The CLI never selects raw facts, account/chat/IP data, or live character
locations. The role/grant setup is an operator/deployment concern; the CLI
never creates tables, users, grants, or migrations.

A report read starts one `REPEATABLE READ, READ ONLY` transaction, selects a
published generation, and reads its state and rows in that snapshot. Omitting
`--generation` selects the newest published generation for the exact
`definition-version/environment-id/season-id` scope. A rebuild can therefore
advance a building generation without making it visible. A requested exact
generation must itself be published.

## CLI

Print the catalog without opening a database connection:

```sh
python3 scripts/telemetry/report.py catalog --format json
python3 scripts/telemetry/report.py catalog --format text
```

Read the newest published playtime page:

```sh
python3 scripts/telemetry/report.py report \
  --name playtime \
  --definition-version 1 \
  --environment-id 8 \
  --season-id 7 \
  --max-rows 1000 \
  --max-bytes 33554432 \
  --max-runtime-s 30 \
  --format json
```

The connection values are intentionally omitted from the example. Supply them
through `TELEMETRY_REPORT_DB_*`, or explicitly pass `--host`, `--port`,
`--database`, and `--user`; the password remains environment-only.

The accepted report names are:

| Name | Frozen #268 source | Meaning |
| --- | --- | --- |
| `playtime` (`session_playtime`, `session`) | `telemetry_rollup_session` | Per-original-logical-session sealed connected/active/idle/unknown/linkdead/resident time. |
| `cohort` (`cohort_activity`, `activity`) | `telemetry_cohort_day` | UTC-day, captured dimensions, and activity-category rows. |
| `return` (`returns`) | `telemetry_cohort_member` | A bounded multi-session indicator for telemetry subjects in the filtered member population. |
| `time` (`time_band`, `time-band`, `timeband`) | `telemetry_cohort_day` | Activity category by UTC day. It is not a local-clock hour report. |
| `faction` | `telemetry_cohort_day` | Duration and coverage grouped by captured faction ID. |

Use `--format text` for an administrator-readable metadata/table view. JSON is
the stable handoff format for a future UI. The response keeps the machine
metadata at the top level (`definition`, `scope`, `versions`, `filters`,
`coverage`, `watermark`, `provisional`, `summary`, `page`) and includes `rows`.
The `page` object is duplicated by the top-level `truncated`, `has_more`, and
`next_cursor` fields for simple clients.

## Filters and keyset pagination

All filters are validated and placed in the SQL `WHERE` clause before the
bounded `LIMIT`. A filtered-out row is not consumed as a page slot. There is no
`OFFSET` pagination and no Python-side first-page filtering.

Supported options are:

```text
--date-from YYYY-MM-DD       inclusive UTC day
--date-to YYYY-MM-DD         exclusive UTC day
--level-band N
--class-id N
--race-id N
--faction-id N
--zone-vnum N
--config-id N
--category 0|1|2|3           unknown, idle, active, linkdead
--subject-id N               playtime and return only
--after CURSOR               opaque cursor returned by the prior page
```

Date and cohort filters apply to `cohort`, `return`, `time`, and `faction`.
`playtime` has no session-start date column in the #268 session projection, so
it supports only `subject_id`. Unsupported filter/report combinations fail
closed instead of being silently ignored.

The cursor binds the canonical report, published generation, complete scope,
filter set, ordering, and the last aggregate key. Reusing a cursor with a
changed scope or filter set is rejected. Each query uses the exact composite
seek predicate for its fixed ordering:

- playtime: `subject_id,session_boot_id,session_process_id,session_seq`
- cohort: `utc_day,level_band,class_id,race_id,faction_id,zone_vnum,config_id,category`
- time: `utc_day,category`
- faction: `faction_id`
- return source scan: the #268 member primary-key suffix, beginning at `utc_day`

The report query fetches at most one extra row as a truncation sentinel. A
`true` `truncated`/`has_more` is not a complete population claim; request the
returned `next_cursor` to continue. `max-rows`, `max-bytes`, and
`max-runtime-s` are positive bounded values. The report worker runs in a
separate killable process, so the CLI parent can terminate a connector/query
that outlives the requested wall-clock budget. A direct library caller should
put its call in an equivalent process when a hard wall-clock guarantee is
required. Parent and worker deadline failures exit with status 2 and a concise
message, not an uncaught exception.

`max-rows` bounds returned source rows (or returned groups for `time`/`faction`),
**not SQL rows examined**. Grouping may read all matching published aggregate
buckets within the requested definition/generation/environment/season and date
filters. Database work is bounded by the dedicated connection's
`max_statement_time` (MariaDB) or `MAX_EXECUTION_TIME` (MySQL), default two
seconds, independently of the CLI process deadline. Narrow the date/cohort
scope or use the last-published cache when a grouped query exceeds that budget.
No fixed rows-examined or production performance guarantee is claimed.

Newest-publication lookup returns only `ORDER BY generation DESC LIMIT 1`;
it does not materialize old publication history or reject otherwise valid scopes
because many generations exist. Its scope filtering also uses the database
statement deadline; the frozen schema is not changed by this report layer.

`max-bytes` includes the complete serialized JSON response and trailing newline:
definition, summary, coverage, cursor, rows, and return-session identities. Text
output receives a final byte check too. If metadata plus a page cannot fit, the
request fails closed; increase the byte budget or reduce `max-rows`. Cache reads
are length-limited and cached responses are checked again before return.

## Metrics, units, denominators, and counts

Durations are integer microseconds (`*_usec`). Display hour fields are a
rounded presentation conversion; the integer microsecond field remains the
authoritative value. Rates are JSON objects with `value`, integer `numerator`,
integer `denominator`, `unit: "fraction"`, and
`zero_denominator_policy: "null"`. Missing inputs and zero denominators are
`null`, never invented zeroes or infinity.

### Playtime

The `playtime` rows expose both the checkpoint plane and sealed coverage. The
summary sums only the additive `covered_*_usec` interval plane across selected
logical-session rows:

- `covered_connected_usec`, `covered_active_usec`, `covered_idle_usec`,
  `covered_unknown_usec`, `covered_resident_usec`, and `covered_linkdead_usec`;
- `attributable_usec`, `observed_intervals`, `entered_sessions`, and
  `exited_sessions`;
- `active_fraction = covered_active_usec / covered_resident_usec`;
- `attributable_fraction = attributable_usec / covered_resident_usec`.

`latest_checkpoint_revision` and checkpoint counter columns are absolute totals
for the latest checkpoint of one session. They are exposed per row but are not
summed with sealed coverage or with another checkpoint. A row with revision
zero reports checkpoint counters as unavailable/null in the frozen #268
`session_playtime` interface.

`session_count` counts logical-session rows. `subject_count` is distinct
telemetry subject cardinality for the page; it is not a distinct account
count. A truncated page labels its summary incomplete.

### Cohort

A `cohort` row is one UTC day plus captured level/class/race/faction/zone/config
and activity category. It carries `duration_usec`, `attributable_usec`,
`observed_intervals`, `subject_count`, `session_count`, `quality_flags`, the
row watermark, and provisional state. Its rate is:

```text
attributable_fraction = attributable_usec / duration_usec
```

The row-level subject/session values are daily cohort cardinalities backed by
member rows. They are not additive across days, dimensions, categories, or
pages. The cohort summary therefore reports row/bucket counts and duration
sums, but does not claim a global distinct population from sums of those
cardinalities.

### Return

`return` reads only declared `membership_kind=2` original logical-session
members. For each subject in the filtered member population it counts distinct
`(session_boot_id, session_process_id, session_seq)` identities. A subject is
`returning_subject=true` at two or more identities. The summary defines:

```text
return_fraction = returning_subject_count / subject_count
```

where the denominator is subjects with at least one matching logical-session
member row. This is a bounded multi-session indicator, not account retention,
login-date retention, or an account metric. Member ordering is by UTC date and
cohort dimensions before subject, so a subject can reappear on nonadjacent
pages. **Every paginated response, including the last cursor page, is an
incomplete population.** It reports `partial_subject_at_boundary=true`,
`summary.complete=false`, and `return_fraction.value=null` with
`unavailable_reason="incomplete_population"`. A subject with only one observed
session has `returning_subject=null` rather than false. Two observed distinct
sessions prove a return even in a partial population.

Rows include `logical_session_ids` triples. Consumers collecting all pages must
union these triples by subject before calculating population counts and rates;
never sum page distinct counts. `count_scope="page_only_not_additive"` marks
incomplete summaries. Only an untruncated first page has complete population
counts. Playtime and cohort summaries likewise mark cursor pages incomplete.

Class, race, and level-band are captured cohort dimensions and exact filters on
`cohort`; they are not separate CLI report names or additive distinct-population
rollups. Connected idle time is available, but dedicated AFK-only duration cannot
be distinguished from other idle time in these aggregates and is not invented.

### Time and faction

`time` groups `telemetry_cohort_day` by UTC day and activity category. Category
codes are `unknown=0`, `idle=1`, `active=2`, and `linkdead=3`. This is the
available activity-band projection; #268 does not retain a clock-hour or local
time-zone dimension, so no such metric is fabricated.

`faction` groups the same aggregate by captured `faction_id`. Both reports
return duration/attributable microseconds, observed interval count, aggregate
bucket count, quality flags, maximum input watermark, and provisional state.
They deliberately omit globally distinct subjects/sessions because the
cohort-day cardinalities are not additive at this grouping.

## Metadata and quality

Every response includes:

- `definition.schema_version`/`definition.definition_version` and `versions.definition_version`;
- captured `config_ids` where the selected source rows retain them; classifier
  and policy versions are `null` because those columns are not in the #268
  aggregates;
- `coverage.input_watermark`/`watermark`, fixed snapshot high-water mark,
  occurrence-time start/end when known, publication status, quality flags,
  rebuild bounds, and `occurrence_bounds_known`;
- `coverage.provisional` and top-level `provisional`;
- `quality_flags` in the summary and source rows/groups.

A high ingestion watermark does not prove that all historical facts have
arrived. The provisional bit remains visible. UTC unknown/sentinel buckets are
returned with `utc_day: null` and `bucket_kind: "unknown"`; no ingestion time
is substituted.

The report catalog marks account distinct/duration, XP/hour, progression,
reward, raw-fact drilldown, chat/IP, and live-location metrics unavailable.
No progression, reward, weighted-XP/hour, or account linkage is inferred from
these aggregates.

## Last-published cache

Pass `--cache-dir` or set `TELEMETRY_REPORT_CACHE_DIR` to enable the optional
cache. A successful page is written atomically as a mode-0600 JSON file in a
mode-0700 directory, keyed by the exact request filters, scope, limits, and
cursor, plus a hash of the configured database host, port, database name,
report username, and report schema version. An outage cannot reuse cached data
from a different database or SQL principal sharing the directory. Cache files
contain aggregate output and metadata only; they do not contain credentials
or raw facts.

If a fresh read cannot open the dedicated connection, cannot find a published
generation, or encounters a report SQL read failure, the service returns the
last successful exact page with:

```json
{
  "served_from_cache": true,
  "cache": {"served": true, "status": "last_published"}
}
```

The cached generation and watermark remain visible. With generation omitted,
the cache is keyed to the logical scope and can keep the prior published
output available while a newer generation is rebuilding or while the database
is unavailable. An explicit generation remains exact and does not silently
change to another generation. Cache failure does not make a fresh published
read fail; the response reports `cache.status: "write_failed"`.

## Validation and handoff status

Offline contract/fixture tests are in `tests/async/test_telemetry_reports_*.py`
and `tests/async/telemetry_reports_*`. The opt-in real SQL suite uses a new
`duris_269_*test` database, a dedicated read-only role, and a task-labeled
network-isolated MariaDB 10.11 or MySQL 8.0 container. It checks the published
aggregate read, exact filtered keyset pagination, empty/missing scope behavior,
role denials, scoped query plans, provisional-flag propagation, incomplete
return populations, and database-bound cache outage behavior. It also exercises
all report types with combined date/class/race/level/faction/category filters,
large grouped fixtures, newest selection across 4,100 published generations,
complete response byte budgets, actual server-side SQL interruption, and a real
CLI report blocked behind a disposable table lock (including connection cleanup
after the deadline). It never mutates the shared #268 fixture database.

Run from the repository root with Docker, a locally installed PyMySQL, and an
existing Python-capable client container whose network mode is `none`:

```sh
python3 tests/async/test_telemetry_reports_contract.py
python3 tests/async/run_telemetry_reports_mysql.py --client-container CLIENT --image mariadb:10.11
python3 tests/async/run_telemetry_reports_mysql.py --client-container CLIENT --image mysql:8.0
```

The runner refuses a network-enabled client or an occupied SQL port (default
3309), creates a uniquely named disposable SQL server sharing that isolated
network namespace, applies the actual 0014/0017 migrations, creates a restricted
report role, and removes only its own container, data volume, and staging files
on exit. Credentials are generated per run and are never printed. It does not
read an existing database, run game-server code, or require production access.

The report interface is ready for a future UI to consume JSON. There is no
frontend, no runtime/schema change, and no local-clock time-of-day report in
this slice. Those are explicit integration/definition gaps rather than hidden
fallback behavior.
