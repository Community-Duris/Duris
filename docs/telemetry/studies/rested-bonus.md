# Rested-bonus study protocol (#273)

This package is the measurement companion for the future rested-bonus review
in #254. It defines a reproducible study export and a fail-closed evaluator;
it does not change `world/limits.c`, `nanny.c`, progression code, balance
properties, migrations, or production activation.

The evaluator is `scripts/telemetry/studies/rested_bonus.py`. It consumes one
bounded, pre-redacted JSON export assembled from approved published telemetry
interfaces. The deployment/report adapter owns the mapping from published
report rows to the study export. The evaluator never opens SQL, reads raw
facts, or reaches live game state.

The source interface manifest is fixed to `telemetry.return.v1`,
`telemetry.playtime.v1`, and `telemetry.progression.v1`. These are reviewed
study-contract identifiers for the adapter boundary, not new SQL endpoints or
permission to read raw facts.

## Question and claim boundary

The study describes how observed return timing, activity exposure, and XP
rates vary across the existing rested tiers. It is an association study, not a
causal estimate of a rested rule. A sufficient synthetic fixture may receive
`fixture_qualified` so the protocol can be exercised; synthetic output is
never real player observation. A real export receives `qualified` only when
all data-quality and sample gates pass.

The study must not claim account retention, causal uplift, or an automatic
balance recommendation. It reports subject-level observations and makes
repeat-account influence visible through an approved redacted repeat-group
token. If that token is absent, the row is excluded and the report abstains.
The token is a sensitivity-control key only; it is not an account identifier
and is never emitted in the report.

## Cohorts, windows, and measurements

The input scope declares the environment, season, published coverage bounds,
coverage quality flags, and every configuration generation in scope. The
evaluator also records the generations actually observed. Generations must
not be silently mixed across before/after comparisons.

Each row represents one redacted subject observation in one cohort/config/tier
cell and carries:

- `cohort`: `solo`, `group`, `pvp`, or `group_pvp`;
- `rested_tier`: `unrested`, `rested`, or `well_rested`;
- `config_generation`: the effective, versioned configuration identity;
- `active_usec`, `connected_usec`, and integer `xp_earned`;
- `return_window`: `under_1h`, `1_4h`, `4_24h`, `1_3d`, or `3d_plus`;
- `returned`, `censored`, `mode_qualified`, and empty `quality_flags`;
- stable redacted `subject_token` and `repeat_group_token`.

The adapter must emit at most one row per subject/cohort/rested-tier/config
cell. If the underlying source has repeated episodes, it must pre-aggregate
them under the approved report contract before producing this study export;
the evaluator rejects duplicate subject/cell rows rather than treating
repeated observations as independent subjects.

The evaluator reports total and subject-rate XP per active hour, connected and
active exposure, return successes and Wilson 95% intervals for each return
window, censored counts, and a deterministic one-subject-per-repeat-group
sensitivity view. Group/PvP rows qualify only when their captured
`mode_qualified` flag is true. A row with a quality flag, an unqualified mode,
invalid coverage, or an invalid identity token is not silently treated as a
negative result.

The default engineering sufficiency gate is at least 8 distinct subjects and
900,000,000 active microseconds (15 minutes) in every observed
cohort/config/tier cell. A return window needs 4 uncensored observations to
receive a fraction; a smaller or empty window is explicitly marked `abstain`.
These are local reproducibility thresholds, not a declaration that a live
study has enough traffic. The observation stop rule must still be chosen from
actual qualified traffic and reviewed before deployment.

## Coverage and abstention

Coverage must identify a published generation, have complete start/end UTC
bounds, and carry no quality flags. The output includes:

- all config generations in scope and those actually observed;
- coverage start/end and duration, publication state, quality flags, and row
  counts;
- each observed cohort/config/tier cell, its sample gate, and its status;
- excluded-row indexes and safe reason codes, never raw identity values;
- return-window censoring, numerator/denominator, and uncertainty metadata.

The report status is `abstain` for empty or small samples, incomplete or
contaminated coverage, duplicate subject/cell observations, invalid or
unqualified rows, or a missing repeat-group control. An abstaining report may
still show descriptive rows that were valid individually, but it is not a
qualified study result. A high ingestion watermark is not treated as complete
occurrence coverage.

The output declares the statistical methods: Wilson score intervals for
return fractions and a normal 95% interval across subject XP rates. Zero
denominators are `null`, never zero or infinity. Both the report and protocol
state `association_not_causal=true` and `causal_effect_proven=false`.

## Future #254 comparison and rollback protocol

Before a rule change is considered, the owner must pre-register either:

1. a complete before/after comparison with the same cohort definitions,
   return windows, censoring rules, coverage gates, and configuration
   generations; or
2. an experiment/holdout protocol with an explicit assignment boundary and
   the same qualified report interface on both sides.

The study itself never turns the comparison into a property mutation. The
review packet must set the primary and guardrail thresholds before looking at
the result, include a reproducible fixture or export manifest, and define an
expiry. Rollback evaluation criteria are:

- rollback if a pre-registered primary guardrail is breached for two
  consecutive complete windows;
- rollback if qualified coverage or sample sufficiency is lost;
- rollback if an owner-approved adverse-outcome or XP-rate bound is crossed in
  a qualified stratum; and
- require reviewer sign-off before any proposal can be considered for
  application.

The evaluator emits these criteria as data, while `automatic_balance_mutation`
and `production_activation` remain false.

## Offline use

Create a pre-redacted export and run:

```sh
python3 scripts/telemetry/studies/rested_bonus.py \
  --input rested-bonus-export.json \
  --output rested-bonus-report.json
```

The command is bounded to an 8 MiB input and 50,000 rows. It exits zero when
it successfully writes either a qualified or abstaining report, and exits
nonzero for malformed JSON/schema input. It has no database credentials,
network, migration, production-load, or game-runtime dependency.

The deterministic tests in
`tests/async/test_telemetry_study_rested.py` cover a sufficient synthetic
fixture, empty/small samples, incomplete/stale/quality coverage, redaction,
repeat-group sensitivity, censored windows, deterministic row-order handling,
and the CLI report. They prove the protocol contract; they do not qualify
real traffic or prove a balance effect.
