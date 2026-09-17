# Deterministic balance shadow policy (#274)

This package defines one report-only recommendation policy. It is an
engineering shadow slice for the future reviewed application/rollback work in
#275. It cannot write a property, call gameplay calculation code, apply a
proposal, or activate a live balance change.

The implementation is
`scripts/telemetry/balance/recommend.py`. It consumes a bounded, pre-redacted
JSON export representing one exact published report generation, configuration
generation, and policy version. The adapter that produces that export is
responsible for using the approved report interface; the evaluator never opens
SQL or a game connection.

## Policy identity and target

The policy version is `balance-shadow-v1`. It owns exactly one target:

```text
parameter: payout.epic.zone.alignmentMod
unit: fraction
value encoding: integer milli-fraction (1000 = 1.000)
bounds: 0..1000
step: 50 (0.050)
```

The decision metric is the descriptive fraction of eligible shadow outcome
observations with `reward_success=true`. The target is 0.600 with a 0.050
deadband:

- below 0.550: propose one bounded step upward;
- above 0.650: propose one bounded step downward;
- inside the band: hold the current value;
- at a bound: hold at the bound and record `target_bound_reached`.

The direction is an explicit policy hypothesis for review, not a causal claim
about the alignment property. Existing trophy and alignment adjustments are
not erased or folded into an invented scalar: each row carries an
`adjustment_context` (`none`, `trophy`, `alignment`, or `trophy_alignment`),
and the report exposes those strata separately.

## Exact input contract

The export contains:

- `policy_version`, `as_of_utc`, and the fixed schema version;
- `report.name=balance_shadow_outcome_v1`, definition version `1`, positive
  generation, config generation, environment and season IDs;
- report coverage start/end, `published`, `complete`, quality flags, and an
  expiry timestamp;
- the fixed target identity/current value/bounds/step;
- redacted rows with unique `event_token`, `subject_token`,
  `repeat_group_token`, source generation/config generation, cohort,
  adjustment context, eligibility, reward success/amount, and quality flags;
- shadow-only recommendation history with `applied_mutations=0` and an
  optional last shadow record; and
- no raw account/character/IP/chat/location/credential fields.

Rows must reference the exact report generation and config generation. A
different generation, unknown quality flag, duplicate event, invalid amount,
unqualified observation, or raw identity field is rejected or causes an
abstaining record. A valid input is never silently reinterpreted as belonging
to another report/config.

## Evidence gates and bounded repeat influence

The default local gate requires at least 20 capped observations, 8 distinct
redacted subjects, 8 distinct redacted repeat groups, and at least 5 rows in
every observed cohort/adjustment stratum. A report is fresh only when its
coverage is no more than 14 days behind `as_of_utc`, it is published and
complete, its expiry has not passed, and it has no quality flags.

Each repeat group contributes at most 4 observations, selected by sorted
`event_token`. The report retains raw and capped sample counts, whether the
cap was applied, distinct subject/repeat-group counts, and the maximum group
share. A repeat group may not exceed 25% of the capped sample. This makes
repeat-account influence bounded without claiming that a redacted repeat
group is a full account identity.

The report includes per-stratum sample count, distinct-subject count,
successes, success fraction and reward amount total. The global success
fraction includes a Wilson 95% interval. Zero denominators are `null`, never
zero or infinity. Evidence is fingerprinted from the exact report/config/
policy/target and capped rows.

## Stale, manipulated, feedback, cooldown, and hysteresis behavior

The evaluator returns `abstain` for sparse, stale, incomplete, unpublished,
quality-flagged, mismatched, duplicate, feedback-loop, or otherwise
unqualified input. It includes safe reason codes and row indexes, never raw
identity values.

Recommendations are deterministic and churn-resistant:

- a previous `applied` record or nonzero `applied_mutations` is a
  `feedback_loop_detected` abstention; shadow output must not feed gameplay;
- a valid prior shadow record blocks a new change for 14 days with
  `cooldown_hold`;
- after cooldown, a candidate less than one policy step away from the prior
  proposal becomes `hysteresis_hold`;
- target arithmetic is fixed-point and clamped to the target bounds; and
- the record always references the exact source generation/config/policy and
  carries `shadow_only=true`, `can_apply=false`,
  `gameplay_mutation=false`, and `automatic_balance_mutation=false`.

`recommendation_id` and the evidence fingerprint are derived from canonical
sorted input, so identical report/config/policy/evidence produces identical
output independent of row order. The record contains current and proposed
values, evidence window, samples, distinct redacted subjects/repeat groups,
strata, uncertainty, reason codes, and the exact input reference.

## Promotion boundary for #275

This shadow policy is not an application path. Before any future application
proposal is considered, the owner must provide qualified baseline evidence,
review the policy direction, define an expiry, and specify a before/after or
holdout comparison. Promotion must separately verify bounds, stale-data
abstention, cooldown/hysteresis, repeat-group influence, existing
trophy/alignment context, and rollback criteria. No automatic activation or
live class/race combat tuning is part of this package.

## Offline validation

Run the deterministic contract suite with:

```sh
python3 tests/async/test_telemetry_balance_shadow_recommend.py
```

The suite covers replay identity, row-order determinism, repeat-group capping,
sparse and low-distinctness abstention, stale/incomplete/expired/quality
coverage, generation mismatch and duplicate manipulation, target bounds,
cooldown, hysteresis, applied-feedback isolation, raw-field rejection, and
the CLI JSON output. It uses only synthetic redacted fixtures; no production
data, migration, load, or activation is involved.
