# Dated identity effort and XP portfolio publication

Definition 3 extends the existing bounded rollup worker with two reports:
`identity_effort` and `portfolio_progression`. Migration
`0060_telemetry_identity_publication` adds four protected stores. The implementation
passed focused local SQL qualification on MariaDB 10.11.14 and MySQL 8.0.46;
the full balance expansion
remains tracked in [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md).

This layer makes overlapping characters measurable without counting every
character as a separate player-hour. Six characters with a confirmed common
controller, active during the same hour, yield six summed character-hours and
one observed controller-hour. Authenticated account ownership and the reviewed
controller association have separate histories. Unknown identity remains visible.

## Retained source and atomic publication

The existing cursor transaction retains selected activity, XP/level and
authenticated ownership facts (kinds 1, 6 and 9) in `telemetry_identity_input`.
Each retained record has its original ingestion/replay key, scoped subject and
canonical bounded payload with a SHA-256 digest. Only fixed typed source fields
are retained; account names, credentials and account lifetime bindings are absent.
The `telemetry_rollup_identity_coverage` header advances its watermark, fact count
and rolling digest alongside the cursor and existing activity projections. A
failure rolls back all of them. Publication verifies the ordered retained input
against that count, watermark and digest, so later raw retention does not change
the selected source window.

Reserve the generation's identity first. The reservation pins an exact registered
review version and digest, or explicit NULL identity. The publisher revalidates
that retained version; a later review or current account ownership cannot
rewrite the reserved history. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md) for
the authenticated reviewer, correction and reservation contracts.

The existing publication transaction copies incident schema 2, computes both
identity reports, seals their coverage header and marks the generation published.
These writes commit together. A source conflict, missing input, capacity refusal
or later-family write failure publishes none of them. A lost commit reply uses
the existing fresh-connection reconciliation path and validates the published
header and detail counts. A published definition-3 source window cannot advance;
later facts or a corrected review require a new generation. Explicit reads of a
superseded definition-3 generation retain its earlier results and coverage.

The four new stores are:

| Store | Role |
| --- | --- |
| `telemetry_identity_input` | Private, append-only selected source facts for one generation. |
| `telemetry_rollup_identity_coverage` | Cursor/source digest while building; complete counts, measured linkage and uncertainty after publication. |
| `telemetry_rollup_identity_effort` | Public dated effort cells for character, account, confirmed-controller and unknown identity bases. |
| `telemetry_rollup_portfolio_xp` | Public dated observed XP cells with exact source, reason, observation status and modifiers. |

## Interpretation

Effort intervals split at actual producer-clock ownership boundaries, dated
review boundaries and UTC-day boundaries. Overlapping intervals for the same
character and producer refuse publication before cohort partitioning. Copyover
keeps the original logical session, but a new producer needs its own observed
ownership anchor; old monotonic clocks do not establish new-process ownership.
If a selected window begins after an ownership observation that it does not
retain, attribution before its first matching anchor remains unknown.
Unknown, discontinuous or conflicting UTC labels retain measured duration while
preventing unsupported clock union and controller attribution.

`character_usec` sums measured character effort. `utc_covered_character_usec`
and `unknown_clock_character_usec` partition that amount.
`covered_union_usec` merges comparable observed intervals for a known character,
account or confirmed controller. `union_usec` is NULL when that cell also contains
unknown-clock effort. Both union fields are NULL for unknown account/controller
populations; all unknown controllers are never treated as one person. These
clocks describe the activity heuristic, not continuous human attention.

Ownership-loss incidents propagate into identity attribution. A known-ended
kind-9 loss remains uncertain until an actual positive ownership observation
from the same producer supplies a fresh anchor. An unknown incident end stays
unknown. A reviewed incident fix or reconstruction does not manufacture an
authenticated ownership observation. Publication retains the incident version
and uncertainty used for that generation.

Each report offers two overlapping partitions: portfolio per configuration/day,
and captured faction/level-band/group-context cells. Configurations, classifier
and policy versions stay separate. The captured group context is unknown, solo
or grouped; it does not establish a shared battle roster. Bases and partitions
are alternative projections of the same source and must not be summed together.
Union cells and distinct counts are not additive across cells or categories.
The header supplies conserved summed-character linkage coverage without adding
the duplicate projections.

XP uses ownership at its actual observation point and the reserved dated review.
Requested, computed and observed applied XP stay separate. Positive earned XP,
death loss and restoration retain their existing source/reason rules;
administrative changes cannot inflate earned XP. Observation statuses and
modifier flags form separate cells. Level-threshold consumption is never counted
as an XP reward. This report does not infer a saved character state, canonical
economic reward, hourly progression rate, milestone time or causal advantage
from rotating characters.

Publication metadata distinguishes a successfully published report from complete
identity coverage. `balance_report_published=true` describes the committed
generation. `complete_identity_coverage_implied=false` and
`controller_population_complete_implied=false` remain explicit, including an
empty generation or a generation reserved with unknown identity. Linkage counts
and quality must accompany comparisons of confirmed controllers.

## Bounds and roles

Hard limits are 16,384 selected inputs, 4,096 bytes per canonical payload, 1,024
ownership observations per subject/session, 16,384 attributed effort slices and
32 UTC-day slices per source interval. A larger or unrepresentable UTC fanout
retains duration in unknown-clock cells. The existing invocation row, byte,
statement, output fanout, runtime and retry limits also apply.

Input and output memory envelopes are reserved before the buffering SQL fetch.
The default 32-MiB invocation permits fewer sources than the hard input ceiling;
a capacity refusal requires an explicitly suitable bounded generation/budget.
The worker does not silently truncate inputs and publish a complete portfolio.
The byte envelopes and focused fixtures do not replace the final personal-local
memory/performance qualification.

The rollup credential needs SELECT/INSERT on retained inputs, generation identity
and public output/detail tables; SELECT/INSERT/UPDATE on the new coverage header;
its existing cursor/activity grants; SELECT on exact private identity review
history and both incident review families; and the existing incident publication
grants. Review history needs no UPDATE/DELETE grant. The publisher needs no
account-name/lifetime mapping. Table permissions trust the maintained registrar
and publisher to enforce scope; they do not provide SQL row-level isolation.

The report credential reads only published state/activity, generation reservation
metadata, public incident copies and the three public identity tables. It cannot
read retained source payloads, reviewer authority, association evidence, raw
telemetry or private account stores. The native raw writer and identity reviewer
need no new publication-table permissions. Migrations create no users or grants.
All four stores are registered in the protected lifecycle inventory; pending
retention/erasure policy does not permit ad hoc destructive cleanup.

## Local commands

Use explicit local disposable/test credentials. The worker uses
`TELEMETRY_ROLLUP_DB_*`; identity review/reservation uses its separately documented
`TELEMETRY_IDENTITY_REVIEW_DB_*` / `TELEMETRY_IDENTITY_ROLLUP_DB_*` settings.

```sh
python3 scripts/telemetry/rollup.py definitions --definition-version 3
python3 scripts/telemetry/identity_history.py /private/identity-v1.json --reserve-generation 1 --definition-version 3
python3 scripts/telemetry/rollup.py run --definition-version 3 --generation 1 --environment-id 8 --season-id 7 --page-size 100 --through-ingest-id 500
python3 scripts/telemetry/rollup.py publish --definition-version 3 --generation 1 --environment-id 8 --season-id 7
python3 scripts/telemetry/rollup.py report --definition-version 3 --generation 1 --environment-id 8 --season-id 7 --name identity_effort
python3 scripts/telemetry/rollup.py report --definition-version 3 --generation 1 --environment-id 8 --season-id 7 --name portfolio_progression

python3 tests/async/test_telemetry_identity_publication.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --identity-publication
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --identity-publication
```

Definitions 1 and 2 keep their earlier schemas and meanings. The five observation
reports remain definition 2. Definition 3 exposes the two new identity reports
and established session/cohort activity reports. The separate administrator
`report.py` interface keeps its definition-1 contract. Shared battles, complete
zone attempts, canonical rewards, rested/assistance/milestone measurements, four
balance suites and the real personal-local gameplay/save/readback gate remain
requirements of the full accepted expansion.
