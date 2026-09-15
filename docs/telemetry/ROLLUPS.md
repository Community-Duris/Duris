# External telemetry rollups (#268)

This is an observation-only, bounded consumer of the immutable
`telemetry_interval` fact stream. It never reads live gameplay rows, changes
`telemetry_interval`, runs migrations, resets a generation, or writes a report
website. The external job owns the original player/day, cohort/day and cursor
aggregates plus the additive session-summary/member stores from migration 0017.
It must use a dedicated rollup database identity.

## Public Python API

The scripts are intentionally importable without PyMySQL:

```python
import sys
sys.path.insert(0, "scripts/telemetry")

from db_access import (
    ConnectionSettings,
    PyMySQLConnectionFactory,
    PyMySQLRollupDatabase,
)
from rollup_definitions import RollupTarget
from rollup_engine import RollupBounds, RollupEngine

settings = ConnectionSettings.from_env()  # TELEMETRY_ROLLUP_DB_* only
factory = PyMySQLConnectionFactory(settings)
database = PyMySQLRollupDatabase(factory)
try:
    target = RollupTarget(
        definition_version=1,
        generation=1,
        environment_id=8,
        season_id=7,
    )
    result = RollupEngine(database).run(
        target,
        # Omit this to capture MAX(ingest_id) once at invocation start.
        through_ingest_id=500,
        origin_ingest_id=0,
        bounds=RollupBounds(page_size=100, max_rows=10_000),
    )
    print(result.public_dict())
    # Publishing is a separate explicit operation after result.complete.
    print(RollupEngine(database).publish(
        target, bounds=RollupBounds(max_runtime_s=10.0, max_retries=2)
    ))
finally:
    database.close()
```

`RollupTarget` validates all four identity fields and currently accepts only
`definition_version=1`. `origin_ingest_id` is immutable per generation and is
the initial cursor for a newly bootstrapped state row. `through_ingest_id` is a
fixed inclusive high-water bound; rows arriving later are intentionally left
for a later invocation. A generation can be processed in pages while a
previous published generation remains readable.

Pure helpers require only named mappings and are useful in offline/fake tests:

- `build_page_contributions(rows, target, start_cursor=...)` validates a
  bounded ordered page and produces session, player-day, cohort-day, and
  subject/session membership deltas without SQL. Pass
  `prior_coverage_end_utc_usec` when replaying a page after a prior commit so
  out-of-order occurrence labels remain visible across page boundaries. A
  derived late label compares the known immutable header
  `occurrence_utc_usec` with the prior covered end; it does not compare an
  interval start or guess elapsedness. An explicit raw late bit is also
  promoted to the rollup late bit on every affected output row.
- `split_utc_interval(start_utc_usec, end_utc_usec, duration_usec, ...)`
  returns exact half-open `UtcSlice` values. Unknown, backward, mismatched,
  non-representable, clock-discontinuous, or over-fanout input retains all
  duration in `UNKNOWN_DAY`.
- `merge_session_projection(existing, session_delta)` advances only the
  greatest checkpoint revision and rejects counter regressions or same-revision
  conflicts.
- `checkpoint_contribution_from_row`, `membership_contribution_from_row`, and
  `coverage_from_state_row` provide the typed handoff used by #269.
- `safe_rate(numerator, denominator)` returns `None` for a zero denominator and
  rejects negative/non-finite values; it never turns an unavailable rate into 0.

`PyMySQLRollupDatabase` exposes the report-side interface:

```python
coverage = database.read_coverage(target, max_bytes=32 * 1024 * 1024, max_runtime_s=30.0)
checkpoints = database.read_checkpoint_contributions(
    target, max_rows=10_000, max_bytes=32 * 1024 * 1024, max_runtime_s=30.0
)
members = database.read_membership_contributions(
    target, membership_kind=1, max_rows=10_000,
    max_bytes=32 * 1024 * 1024, max_runtime_s=30.0
)
snapshot = database.read_report(
    target, "cohort_activity", max_rows=10_000,
    max_bytes=32 * 1024 * 1024, max_runtime_s=30.0
)
```

`CheckpointContribution` is the latest absolute checkpoint plane; its counters
must not be summed with one another or with `covered_*` interval totals. A
session with `latest_checkpoint_revision=0` is exposed by `read_report()` as
`checkpoint_totals_available=false` with null checkpoint counters; storage
defaults of zero are not reported as observed totals.
`MembershipContribution` retains each declared subject (`membership_kind=1`,
all session IDs zero) and original logical session (`membership_kind=2`) grain.
`RollupCoverage` carries the committed input watermark, fixed snapshot bound,
occurrence bounds, quality flags, publication status, rebuild origin/bound, and
provisional state.

## Stable report definitions

`rollup_definitions.REPORT_DEFINITIONS` contains the executable v1 catalog:

| Name | Table | Grain | Counts/metrics |
| --- | --- | --- | --- |
| `session_playtime` | `telemetry_rollup_session` | Original logical session | Absolute resident/connected/active/idle/unknown/linkdead checkpoint totals; separately sealed `covered_*` totals; attribution, interval count, lifecycle, quality |
| `cohort_activity` | `telemetry_cohort_day` | UTC day plus captured level/class/race/faction/zone/config/activity category | Duration, known-context attribution, observed interval count, daily subject/session counts, quality |

`report_definition(name)` accepts the canonical names plus the input
aliases `session`, `playtime`, `cohort`, and `activity`; emitted metadata always
uses the canonical name. `report_catalog()` returns JSON-ready metadata. The
CLI/API report read is bounded (10,000 rows by default) and exposes
`ReportSnapshot.truncated` / CLI `truncated=true` rather than silently treating
a partial distribution as complete. The typed checkpoint/member contribution
readers instead raise `BoundsExceeded` if their row limit would truncate a
population; they never return an apparently complete partial distribution.
`read_report()` serves only published generations and reads state plus rows in
one read-only REPEATABLE READ snapshot, even while an incremental writer advances.
The default report caps are 10,000 rows, 32 MiB, and 30 seconds; callers can
lower them explicitly as shown above.

Rates have explicit denominator policy: a zero denominator is `null`, never an
invented zero or infinity. Each catalog entry also exposes machine-readable
`rate_numerator_metrics`, `rate_denominator_metrics`, `rate_unit`,
`zero_denominator_policy`, and `missing_input_policy` fields (plus the nested
`rate` object). The existing `denominator` field remains prose-compatible for
callers that need the selection explanation. Missing inputs are `null`, never
an invented zero. Distinct counts are only at their declared daily
cohort grain and are not additive across days/cohorts. Member rows, not cohort
sums, are the input for a median or percentile. Account metrics are explicitly
unavailable: no account linkage is present or inferred, even if a test fixture
has a test-only account mapping. XP, rewards, and average-of-ratios metrics are
not defined here.

## Semantics

- Category codes are `unknown=0`, `idle=1`, `active=2`, `linkdead=3`.
  Every interval contributes exactly once to resident. Linkdead contributes to
  resident/linkdead only; every other category contributes to
  connected and exactly one active/idle/unknown bucket.
- `telemetry_rollup_session.latest_checkpoint_revision` and its six counters
  are a greatest-revision absolute projection. Checkpoint rows never add
  duration. A stale revision is retained as raw evidence but does not regress
  the projection.
- `observed_intervals` on a session counts raw interval facts exactly once;
  it does not count UTC slices. Daily/cohort/member observed counts are local
  day contributions and are not globally additive.
- `attributable_usec` is conservative known-context, known-UTC coverage. It
  requires `context_quality=observed`, a non-overflow context, nonzero captured
  level/class/race/faction, a nonnegative zone, and a known group size. Unknown
  context receives no attributable coverage, but valid captured cohort
  dimensions remain visible even when context quality is partial or unavailable.
  Typed unknown dimensions use `0`/`-1` sentinels. The SQL raw table is
  nullable by record tag because non-interval tags omit interval fields; a
  `NULL` interval dimension is not an admitted typed interval and fails closed,
  rather than being reinterpreted as unknown.
- UTC splitting is half-open and conserves monotonic duration. The SQL
  `DATE '1000-01-01'` value is reserved for the unknown/unattributable bucket;
  the typed interface exposes it as `utc_day=null` / `bucket_kind=unknown`.
  No ingestion timestamp is substituted for an unknown occurrence timestamp.
- Process-wide gap facts have no session contribution. They still set a
  rollup-only quality flag on the target state because the global input prefix
  is incomplete. Session-scoped gaps set the dedicated session-gap rollup bit
  on both that session and the target state, but do not fabricate an interval,
  duration, or context.
- Session/day/cohort aggregate rows remain `provisional=1` (the membership
  support table has no separate provisional column; its generation is covered
  by the state row). Reaching a fixed input watermark does not prove a crash
  tail was observed or that arbitrarily late facts can never arrive.

Raw quality bits 0..8 are preserved. The engine reserves these v1 rollup bits:

| Bit | Meaning |
| ---: | --- |
| 16 | UTC unknown/sentinel/reserved-day attribution |
| 17 | UTC endpoint/duration mismatch or non-representable date |
| 18 | UTC moved backward |
| 19 | UTC split exceeded the fanout budget |
| 20 | Process-wide coverage gap |
| 21 | Context unavailable/overflow/unknown |
| 22 | Captured attribution dimension invalid/unknown |
| 23 | Conflicting lifecycle end reason |
| 24 | Conflicting checkpoint revision/counters |
| 25 | Conservative event-label late input: known header occurrence precedes the prior covered end; raw quality late bit 8 is promoted here |
| 26 | Session-scoped coverage gap |

Late input is an event-label quality signal, not proof that an interval's
elapsed time arrived after another interval. Concurrent intervals may overlap;
the engine does not infer lateness from interval starts or from wall-clock
elapsedness. Unknown occurrence labels remain unlabeled unless the raw record
explicitly carries the late bit.

## Atomic page and replay rules

For every page the adapter:

1. obtains the bounded advisory lock for definition/environment/season;
2. starts one transaction, inserts/locks the state row, and locks the current
   cursor;
3. fetches explicit named raw columns with the indexed keyset predicate
   `ingest_id > cursor AND ingest_id <= fixed_high_water ORDER BY ingest_id
   LIMIT n` using `FORCE INDEX (PRIMARY)`;
4. computes the pure page deltas from that locked cursor;
5. updates session projection, player-day/cohort sums, member contributions, and
   subject/session counts in the same transaction; and
6. advances `telemetry_rollup_state.input_watermark` in that same commit.

There is no `OFFSET`, no raw `UPDATE`/`DELETE`, no `SELECT *`, and no current
character/context lookup. The adapter scans all stream scopes to retain a
stable global committed prefix, aggregates only the requested environment and
season, and marks process-wide gaps conservatively.

A commit exception is ambiguous. The socket is discarded, a new serial socket
reacquires the bounded lock, and the committed cursor is reread before any
retry. A cursor at or past the page end acknowledges the page; a cursor still
at the original page start retries the retained page; an intermediate cursor
fails closed. A later page is never issued while ambiguity is unresolved.

Publication is separate and explicit. A SHA-256 lock name covers the complete
database/definition/environment/season identity without truncating large IDs.
Publication reads at most `PUBLICATION_STATE_LIMIT + 1` state rows (capacity 4096
per definition) using the existing PRIMARY prefix and fails closed above capacity.
This preserves sealed migration 0014's exact index contract. It filters the bounded
catalog for the requested scope and refuses multiple published rows or an older
generation replacing a newer one. Only exact primary-key point updates publish the
new row and supersede its predecessor in one transaction; no unindexed broad
UPDATE or implicit `MAX(generation)` selection occurs. No delete/truncate/reset or
automatic retention is authorized. Reaching state capacity requires an explicitly
reviewed capacity/storage change, not an unbounded fallback scan.

Retry and acknowledgement reconciliation retain the original page deadline. A
local pre-COMMIT budget failure is a definite rollback, not a fabricated commit
ambiguity; no COMMIT was sent in that case. A COMMIT call that returns after its
deadline is treated as ambiguous and the connection is discarded, because the
server may have committed before its acknowledgement was observed.

## Bounds and connection safety

`RollupBounds` limits page rows, total fetched rows/bytes, output fanout,
transaction statements, monotonic runtime, retries, and lock/socket/statement
policies. Defaults are deliberately bounded and are not a load qualification.
Publication accepts the same validated bounds object and consumes one fixed
deadline plus one retry ceiling across the entire operation; it does not grant
each ambiguous-commit retry a fresh time budget. The adapter also rejects
`max_commit_retries` above the same small hard retry maximum.
A raw fact that cannot fit the minimum page/byte/fanout budget raises
`BoundsExceeded` before cursor movement. The SQL page limit reserves a conservative
fixed-column byte bound before fetching; validation also checks actual estimated
bytes. A budget smaller than one maximum-width fact is rejected, not overfetched.
Timeout values must be finite; `max_retries=0` disables replay attempts while still
permitting a fresh-connection acknowledgement check. Per-page statement/socket
settings are applied to the real connection. InnoDB lock timeouts have whole-second
granularity, with socket/deadline checks providing additional caller bounds. If
budget expiry prevents advisory-lock release, the connection is discarded.

Report and typed distribution readers require explicit positive row, byte, and
monotonic-runtime caps. Report SQL fetches at most one extra row as a truncation
sentinel, and report snapshots return `truncated=true` when the row/byte capacity
is reached. Typed checkpoint and membership distributions fail closed rather than
returning partial populations. The fixed row-width envelope is applied before
materialization; a byte budget too small for one row plus the sentinel is rejected.

`PyMySQLConnectionFactory` is one-connection-only and imports PyMySQL lazily.
The dedicated namespace is:

```text
TELEMETRY_ROLLUP_DB_HOST
TELEMETRY_ROLLUP_DB_PORT       (optional, default 3306)
TELEMETRY_ROLLUP_DB_DATABASE
TELEMETRY_ROLLUP_DB_USER
TELEMETRY_ROLLUP_DB_PASSWORD   (or TELEMETRY_ROLLUP_DB_PASSWD)
TELEMETRY_ROLLUP_DB_TLS_VERIFY
TELEMETRY_ROLLUP_DB_SSL_CA
TELEMETRY_ROLLUP_DB_SECURE_TUNNEL
```

The adapter never reads the game's `.env`, `DB_USER`, or `DB_PASSWD`, and the
CLI has no password option. Loopback may use explicitly disabled TLS. A direct
non-loopback endpoint requires an existing CA and verified TLS, unless the
operator explicitly sets `SECURE_TUNNEL=true` for a reviewed private tunnel.
The connector sets UTF-8, UTC, READ COMMITTED, InnoDB lock wait timeout, and the
server-specific statement-time limit. Socket timeouts and post-call deadline
checks are defensive connector bounds; they are not cancellation of an already
running DML or COMMIT. The CLI is the hard finite-time boundary: `run`, `publish`,
and `report` each execute in a fresh killable worker, and the parent terminates
that worker at `max_runtime_s`. A direct library caller must place the operation
in an equivalent killable process when a hard wall-clock guarantee is required.

## CLI

Print the stable catalog without connecting:

```sh
python3 scripts/telemetry/rollup.py definitions
```

Run one explicit bounded generation. Credentials remain in the environment;
host/database/user may be flags or the dedicated environment values:

```sh
export TELEMETRY_ROLLUP_DB_PASSWORD='provided-out-of-band'
python3 scripts/telemetry/rollup.py run \
  --host 127.0.0.1 \
  --database duris_268_rollup_test \
  --user duris268_rollup \
  --definition-version 1 \
  --generation 1 \
  --environment-id 8 \
  --season-id 7 \
  --origin-ingest-id 0 \
  --through-ingest-id 36 \
  --page-size 10 \
  --max-rows 1000 \
  --max-runtime-s 30
```

Omit `--through-ingest-id` to snapshot the raw maximum once. Publish only after
one or more `run` calls report `complete=true`:

```sh
python3 scripts/telemetry/rollup.py publish \
  --host 127.0.0.1 --database duris_268_rollup_test --user duris268_rollup \
  --definition-version 1 --generation 1 --environment-id 8 --season-id 7 \
  --max-runtime-s 10 --max-retries 2
```

Read a named report and its coverage metadata:

```sh
python3 scripts/telemetry/rollup.py report \
  --host 127.0.0.1 --database duris_268_rollup_test --user duris268_rollup \
  --definition-version 1 --generation 1 --environment-id 8 --season-id 7 \
  --name cohort_activity --max-rows 10000 --max-bytes 33554432 \
  --max-runtime-s 30
```

The CLI does not create schema, provision grants, mutate raw facts, or target a
production database by default. Parent/integration work must provision the
exact #268 support tables and dedicated role before exercising the SQL path.
