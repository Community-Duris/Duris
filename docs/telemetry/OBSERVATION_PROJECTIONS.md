# Collected progression, encounter and combat observations

Rollup definition 2 publishes the existing typed record kinds 6, 7 and 8 together
with the established activity projections. Migration
`0055_telemetry_observation_projections` adds five stores. It leaves the sealed raw
schema, replay identities, source validators and existing definition 1 metrics
intact. Both versions scan and validate all eight known record families before
advancing their global ingestion cursor. Definition 1 gives the additional
families no playtime contribution; it no longer stops at the first valid kind 6.
Unknown schema versions, record families, reserved quality bits, partial required
identities and malformed active payloads fail before cursor acknowledgement.

This layer supplies observations for the accepted balance expansion. Shared
battles, full zone attempts, durable reward linkage, account/controller portfolios
and the four balance suites remain separate requirements tracked in
[IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md).

## Published grains and interpretation

| Report | Store | Retained meaning |
| --- | --- | --- |
| `progression_observations` | `telemetry_rollup_progression_day` | Subject/original session/day/exact level and captured source dimensions. Requested, computed and applied signed XP, positive earned XP, death loss, restoration, zero/negative observation counts, source/reason/status/modifier flags and quality. |
| `level_observations` | `telemetry_rollup_level_event` | One observed level transition at its original raw replay key. Before/after levels, threshold consumption, modifier flags, point time, source/status/configuration and captured dimensions. |
| `encounter_observations` | `telemetry_rollup_encounter` | One original producer encounter, with independent start/close evidence, latest mode and observed mode mask, actual close elapsedness/outcome, participant and expected-credit counts. |
| `encounter_participants` | `telemetry_rollup_encounter_participant` | One encounter/subject/PID, with roster evidence and greatest measured cumulative effort revision. |
| `combat_contributions` | `telemetry_rollup_combat_actor` | Latest absolute contribution snapshot per encounter/actor, retaining captured player/pet/NPC ownership, actor PID, context and bounded quality. |

Daily XP cells retain all readable typed dimensions. A fixed domain-separated
SHA-256 cell digest keeps the primary key within both engines' 16-column index
limit. Every merge also checks the original dimensions and PID; a digest does
not replace that evidence or authorize a conflicting cell.

XP kinds and statuses remain distinct. Positive earned XP requires the earned
reason and a known gameplay source 1–10. Administration, system and unknown-source
changes retain their own signed totals and cannot inflate that earned measure.
Death loss is a positive magnitude in its own measure; restoration is separate.
Level advancement/loss has no XP reward amount: `threshold_xp` describes consumed
thresholds and must never be added to earned XP. Current producers normally emit
`observed_mutable` status 1. A committed telemetry fact does not prove the character
save or an economic operation committed. Do not sum different observation status
planes as if they were independent rewards or silently promote status 1 to durable
reconciliation.

Participant leave and summary rows are absolute cumulative measurements. A leave
with 20 units followed by a summary with 40 projects 40, not 60. A rejoin supplies
roster evidence without resetting measured effort. A join-only participant has
NULL measured effort. Later-ingested older revisions can add previously missing
roster evidence without regressing effort; contradictory or decreasing cumulative
measurements fail and roll back the page.

An unclosed encounter has NULL end and elapsed values. A close carrying its start
anchor does not invent a missing start observation. Outcomes retain the generic
source vocabulary: a generic success cannot establish a shared battle winner or
a full zone clear. Copyover, shutdown and unknown-close outcomes are censored
boundaries. `mode_mask` retains observed modes, but does not reconstruct the time
spent in each mode or a complete changing roster. Expected credit count describes
eligibility context and supplies no participation duration.

Actor snapshots replace older absolute contributions rather than adding them.
Captured ownership cannot change under the same encounter/actor identity. PCs
retain positive PIDs; pet/NPC PIDs use the source's -1 unknown sentinel. Pets retain
their player owner, while NPC owner 0 means no player attribution. Neither is an
extra human. Per-actor copies of `unique_player_count`, `participant_count` and
opponent counts cannot be summed to produce encounter cardinality. `power_band`
is a captured level proxy; it contains no verified gear or skill strength. Damage,
effective healing, overhealing, control applications, casting and tanking remain
separate measures with no universal contribution score.

## Coverage, bounds and replay

The raw SELECT explicitly names the active family columns; no payload blob or
`SELECT *` is introduced. Row width is reserved before fetching, and each page
retains the existing row/byte/fanout/statement/runtime/retry limits. A transaction
locks the rollup cursor, reads at most one primary-key cell per output, validates
an exact merge, writes absolute projections and advances the cursor together.
Conflicts and budget failures roll back all affected families and the cursor.
Lost commit acknowledgements reconcile the same cached page against the cursor
on a new connection before replay or further input.

UTC point labels require agreement between the raw header and active payload.
Unknown, discontinuous, mismatched or unrepresentable labels retain explicit
quality. XP uses the existing indexed unknown-day sentinel internally and returns
NULL plus `bucket_kind=unknown` publicly. Known point labels do not prove continuous
activity coverage. Closed windows retain separate uncertainty when the UTC span
does not match monotonic elapsedness. Raw cardinality loss bit 9 and explicit or
derived late input remain visible. Missing incident registration is unknown
coverage, and reviewed incident gaps remain attached to published output.

Every report uses a consistent read-only transaction and requires the requested
generation to be published. Rows, state and incident coverage belong to that
snapshot. New reads reserve a fixed-width envelope from their explicit columns
and a truncation sentinel before materializing output; a partial page makes no
complete cohort/distribution claim. Hourly progression rates, battle win rates,
zone clear rates, unique human counts and gear/skill matching are unavailable
until the corresponding balance requirements supply the evidence and denominator.

## Roles and local use

The rollup role needs SELECT/INSERT/UPDATE on the five new stores, in addition to
the existing cursor/activity permissions and reviewed-incident publication
permissions. The report role needs SELECT on these five stores, the published
rollup state and published incident copies; established playtime/cohort stores are
needed only for their corresponding reads. It requires no raw, gameplay, account,
registry, INSERT, UPDATE or DELETE permission. The incident registrar and writer
roles do not change. Migrations never create users or grants.

Use the current rollup CLI, supplying explicit scoped generation identities:

```sh
python3 scripts/telemetry/rollup.py definitions --definition-version 2
python3 scripts/telemetry/rollup.py run --definition-version 2 --generation 1 --environment-id 8 --season-id 7
python3 scripts/telemetry/rollup.py publish --definition-version 2 --generation 1 --environment-id 8 --season-id 7
python3 scripts/telemetry/rollup.py report --definition-version 2 --generation 1 --environment-id 8 --season-id 7 --name progression_observations
```

The worker uses explicit `TELEMETRY_ROLLUP_DB_*` settings. For the read-only `report`
command, supply the dedicated SELECT-only role in that namespace. No game settings
or credentials are a fallback. Definition 1 remains the default offline catalog.
The existing `report.py` administrator commands retain their version 1 catalog;
the balance suites will integrate the new definitions with their own scoped
filters and statistical contracts.

The portable wrapper creates a fresh uniquely named loopback fixture and loads
the authoritative bootstrap and complete immutable manifest. It does not read
checkout `.env`, production or staging settings. It destroys only that fixture.

```sh
python3 tests/async/test_telemetry_observations.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --observations
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --observations
```

The SQL fixture checks mixed-stream pagination, all five published reports,
unknown tails/dates, original playtime, lost commit replies, repeat consumption,
restricted roles, byte/fanout/statement budgets, cumulative-conflict rollback and
direct SQL constraint negatives. It also exercises the additive schema verifier
and engine-measured runtime metadata. This fixture does not establish live balance
or replace the final personal-server gameplay, persistence and latency gate.
