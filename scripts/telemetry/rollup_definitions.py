"""Stable, executable definitions and typed output contracts for telemetry rollups.

The module deliberately has no database or PyMySQL dependency.  It is the small
contract that a report consumer (#269) can import without importing the worker.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
from datetime import date
import math
from types import MappingProxyType
from typing import Any, Mapping

DEFINITION_VERSION = 1
SUPPORTED_DEFINITION_VERSIONS = frozenset({DEFINITION_VERSION, 2, 3, 5})
BATTLE_DEFINITION_VERSION = 5

PUBLICATION_BUILDING = 0
PUBLICATION_PUBLISHED = 1
PUBLICATION_SUPERSEDED = 2
PUBLICATION_FAILED = 3
PUBLICATION_STATUSES = MappingProxyType(
    {
        "building": PUBLICATION_BUILDING,
        "published": PUBLICATION_PUBLISHED,
        "superseded": PUBLICATION_SUPERSEDED,
        "failed": PUBLICATION_FAILED,
    }
)

UNKNOWN_DAY = date(1000, 1, 1)
UTC_UNKNOWN = -(1 << 63)
UINT64_MAX = (1 << 64) - 1
UINT32_MAX = (1 << 32) - 1
UINT16_MAX = (1 << 16) - 1
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1

COUNTER_FIELDS = (
    "connected_usec",
    "active_usec",
    "idle_usec",
    "unknown_usec",
    "resident_usec",
    "linkdead_usec",
)
COVERED_COUNTER_FIELDS = tuple("covered_" + name for name in COUNTER_FIELDS)
COHORT_DIMENSION_FIELDS = (
    "utc_day",
    "level_band",
    "class_id",
    "race_id",
    "faction_id",
    "zone_vnum",
    "config_id",
    "category",
)
SESSION_ID_FIELDS = ("session_boot_id", "session_process_id", "session_seq")
SCOPE_FIELDS = ("definition_version", "generation", "environment_id", "season_id")

# Raw quality flags are bits 0..9 in telemetry_types.h.  Bits 16+ belong to the
# external rollup only and are intentionally documented rather than overloaded.
ROLLUP_QUALITY_UTC_UNKNOWN = 1 << 16
ROLLUP_QUALITY_UTC_MISMATCH = 1 << 17
ROLLUP_QUALITY_UTC_BACKWARD = 1 << 18
ROLLUP_QUALITY_UTC_FANOUT = 1 << 19
ROLLUP_QUALITY_PROCESS_GAP = 1 << 20
ROLLUP_QUALITY_CONTEXT_UNAVAILABLE = 1 << 21
ROLLUP_QUALITY_DIMENSION_INVALID = 1 << 22
ROLLUP_QUALITY_LIFECYCLE_CONFLICT = 1 << 23
ROLLUP_QUALITY_CHECKPOINT_CONFLICT = 1 << 24
ROLLUP_QUALITY_LATE_INPUT = 1 << 25
ROLLUP_QUALITY_SESSION_GAP = 1 << 26
ROLLUP_QUALITY_INCIDENT_GAP = 1 << 27
ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN = 1 << 28
ROLLUP_QUALITY_KNOWN_MASK = (1 << 10) - 1
ROLLUP_QUALITY_MASK = (
    ROLLUP_QUALITY_KNOWN_MASK
    | ROLLUP_QUALITY_UTC_UNKNOWN
    | ROLLUP_QUALITY_UTC_MISMATCH
    | ROLLUP_QUALITY_UTC_BACKWARD
    | ROLLUP_QUALITY_UTC_FANOUT
    | ROLLUP_QUALITY_PROCESS_GAP
    | ROLLUP_QUALITY_CONTEXT_UNAVAILABLE
    | ROLLUP_QUALITY_DIMENSION_INVALID
    | ROLLUP_QUALITY_LIFECYCLE_CONFLICT
    | ROLLUP_QUALITY_CHECKPOINT_CONFLICT
    | ROLLUP_QUALITY_LATE_INPUT
    | ROLLUP_QUALITY_SESSION_GAP
    | ROLLUP_QUALITY_INCIDENT_GAP
    | ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN
)


@dataclass(frozen=True, slots=True)
class RollupTarget:
    """Immutable rollup identity; all four fields are required on every call."""

    definition_version: int
    generation: int
    environment_id: int
    season_id: int

    def __post_init__(self) -> None:
        validate_target(self)

    @property
    def scope_tuple(self) -> tuple[int, int, int, int]:
        return (
            self.definition_version,
            self.generation,
            self.environment_id,
            self.season_id,
        )


@dataclass(frozen=True, slots=True)
class ReportDefinition:
    """Reviewable report metadata plus the aggregate table it reads."""

    name: str
    definition_version: int
    grain: str
    table: str
    dimensions: tuple[str, ...]
    metrics: tuple[str, ...]
    denominator: str
    distinct_semantics: str
    distribution_semantics: str
    account_metrics_available: bool = False
    unavailable_metrics: tuple[str, ...] = (
        "account_distinct_count",
        "account_duration_usec",
        "account_session_union_usec",
    )
    rate_numerator_metrics: tuple[str, ...] = ()
    rate_denominator_metrics: tuple[str, ...] = ()
    rate_unit: str = "fraction"
    zero_denominator_policy: str = "null"
    missing_input_policy: str = "null"

    def public_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "definition_version": self.definition_version,
            "grain": self.grain,
            "table": self.table,
            "dimensions": list(self.dimensions),
            "metrics": list(self.metrics),
            "denominator": self.denominator,
            "rate_numerator_metrics": list(self.rate_numerator_metrics),
            "rate_denominator_metrics": list(self.rate_denominator_metrics),
            "rate_unit": self.rate_unit,
            "zero_denominator_policy": self.zero_denominator_policy,
            "missing_input_policy": self.missing_input_policy,
            "rate": {
                "numerator_metrics": list(self.rate_numerator_metrics),
                "denominator_metrics": list(self.rate_denominator_metrics),
                "unit": self.rate_unit,
                "zero_denominator_policy": self.zero_denominator_policy,
                "missing_input_policy": self.missing_input_policy,
            },
            "distinct_semantics": self.distinct_semantics,
            "distribution_semantics": self.distribution_semantics,
            "account_metrics_available": self.account_metrics_available,
            "unavailable_metrics": list(self.unavailable_metrics),
        }


@dataclass(frozen=True, slots=True)
class RollupCoverage:
    """State/freshness metadata attached to every report read."""

    target: RollupTarget
    input_watermark: int
    snapshot_high_watermark: int
    publication_status: int
    coverage_start_utc_usec: int | None
    coverage_end_utc_usec: int | None
    quality_flags: int
    provisional: bool
    rebuild_from_ingest_id: int
    rebuild_through_ingest_id: int
    incident_coverage: Mapping[str, Any] | None = None
    identity_coverage: Mapping[str, Any] | None = None
    battle_coverage: Mapping[str, Any] | None = None

    @property
    def input_complete_to_snapshot(self) -> bool:
        return self.input_watermark == self.snapshot_high_watermark

    @property
    def occurrence_bounds_known(self) -> bool:
        return (
            self.coverage_start_utc_usec is not None
            and self.coverage_end_utc_usec is not None
            and not self.quality_flags
            & (
                ROLLUP_QUALITY_UTC_UNKNOWN
                | ROLLUP_QUALITY_UTC_MISMATCH
                | ROLLUP_QUALITY_UTC_BACKWARD
                | ROLLUP_QUALITY_UTC_FANOUT
            )
        )

    def public_dict(self) -> dict[str, Any]:
        return {
            "definition_version": self.target.definition_version,
            "generation": self.target.generation,
            "environment_id": self.target.environment_id,
            "season_id": self.target.season_id,
            "input_watermark": self.input_watermark,
            "snapshot_high_watermark": self.snapshot_high_watermark,
            "input_complete_to_snapshot": self.input_complete_to_snapshot,
            "publication_status": self.publication_status,
            "coverage_start_utc_usec": self.coverage_start_utc_usec,
            "coverage_end_utc_usec": self.coverage_end_utc_usec,
            "quality_flags": self.quality_flags,
            "provisional": self.provisional,
            "rebuild_from_ingest_id": self.rebuild_from_ingest_id,
            "rebuild_through_ingest_id": self.rebuild_through_ingest_id,
            "incident_coverage": self.incident_coverage,
            **({"identity_coverage": self.identity_coverage} if self.target.definition_version >= 3 else {}),
            **({"battle_coverage": self.battle_coverage} if self.target.definition_version == BATTLE_DEFINITION_VERSION else {}),
        }


@dataclass(frozen=True, slots=True)
class CheckpointContribution:
    """Latest absolute checkpoint plane for one logical session.

    The six totals are not interval deltas and must never be summed with one
    another or with covered interval totals.
    """

    target: RollupTarget
    subject_id: int
    pid: int
    session_boot_id: int
    session_process_id: int
    session_seq: int
    latest_checkpoint_revision: int
    counters: tuple[int | None, int | None, int | None, int | None, int | None, int | None]
    quality_flags: int
    input_watermark: int
    provisional: bool = True

    @property
    def checkpoint_available(self) -> bool:
        return self.latest_checkpoint_revision > 0

    def counter_dict(self) -> dict[str, int | None]:
        return dict(zip(COUNTER_FIELDS, self.counters, strict=True))

    def public_dict(self) -> dict[str, Any]:
        return {
            "definition_version": self.target.definition_version,
            "generation": self.target.generation,
            "environment_id": self.target.environment_id,
            "season_id": self.target.season_id,
            "subject_id": self.subject_id,
            "pid": self.pid,
            "session_boot_id": self.session_boot_id,
            "session_process_id": self.session_process_id,
            "session_seq": self.session_seq,
            "latest_checkpoint_revision": self.latest_checkpoint_revision,
            "checkpoint_totals_available": self.checkpoint_available,
            "counters": self.counter_dict(),
            "quality_flags": self.quality_flags,
            "input_watermark": self.input_watermark,
            "provisional": self.provisional,
        }


@dataclass(frozen=True, slots=True)
class MembershipContribution:
    """One declared daily cohort member at subject or original-session grain."""

    target: RollupTarget
    utc_day: date
    level_band: int
    class_id: int
    race_id: int
    faction_id: int
    zone_vnum: int
    config_id: int
    category: int
    membership_kind: int
    subject_id: int
    session_boot_id: int
    session_process_id: int
    session_seq: int
    duration_usec: int
    attributable_usec: int
    observed_intervals: int
    quality_flags: int
    input_watermark: int
    provisional: bool = True

    @property
    def cohort_key(self) -> tuple[Any, ...]:
        return (
            self.utc_day,
            self.level_band,
            self.class_id,
            self.race_id,
            self.faction_id,
            self.zone_vnum,
            self.config_id,
            self.category,
        )

    @property
    def identity_key(self) -> tuple[Any, ...]:
        return self.cohort_key + (
            self.membership_kind,
            self.subject_id,
            self.session_boot_id,
            self.session_process_id,
            self.session_seq,
        )

    @property
    def bucket_kind(self) -> str:
        return "unknown" if self.utc_day == UNKNOWN_DAY else "calendar"

    @property
    def public_utc_day(self) -> date | None:
        return None if self.utc_day == UNKNOWN_DAY else self.utc_day

    def public_dict(self) -> dict[str, Any]:
        return {
            "definition_version": self.target.definition_version,
            "generation": self.target.generation,
            "environment_id": self.target.environment_id,
            "season_id": self.target.season_id,
            "utc_day": self.public_utc_day,
            "bucket_kind": self.bucket_kind,
            "level_band": self.level_band,
            "class_id": self.class_id,
            "race_id": self.race_id,
            "faction_id": self.faction_id,
            "zone_vnum": self.zone_vnum,
            "config_id": self.config_id,
            "category": self.category,
            "membership_kind": self.membership_kind,
            "subject_id": self.subject_id,
            "session_boot_id": self.session_boot_id,
            "session_process_id": self.session_process_id,
            "session_seq": self.session_seq,
            "duration_usec": self.duration_usec,
            "attributable_usec": self.attributable_usec,
            "observed_intervals": self.observed_intervals,
            "quality_flags": self.quality_flags,
            "input_watermark": self.input_watermark,
            "provisional": self.provisional,
        }


@dataclass(frozen=True, slots=True)
class ReportSnapshot:
    """Typed handoff for #269: definition, coverage, and named rows."""

    definition: ReportDefinition
    coverage: RollupCoverage
    rows: tuple[Mapping[str, Any], ...]
    truncated: bool = False


def validate_target(target: RollupTarget) -> None:
    if not isinstance(target.definition_version, int) or isinstance(
        target.definition_version, bool
    ):
        raise ValueError("definition_version must be an integer")
    build_versions = SUPPORTED_DEFINITION_VERSIONS
    if target.definition_version not in build_versions:
        raise ValueError(
            f"unsupported rollup definition version {target.definition_version}; "
            f"buildable={sorted(build_versions)}"
        )
    for name in ("generation", "environment_id", "season_id"):
        value = getattr(target, name)
        if not isinstance(value, int) or isinstance(value, bool) or not 1 <= value <= UINT64_MAX:
            raise ValueError(f"{name} must be an unsigned nonzero 64-bit integer")


SESSION_PLAYTIME = ReportDefinition(
    name="session_playtime",
    definition_version=DEFINITION_VERSION,
    grain="logical_session",
    table="telemetry_rollup_session",
    dimensions=("subject_id", "pid", *SESSION_ID_FIELDS),
    metrics=(
        *COUNTER_FIELDS,
        *COVERED_COUNTER_FIELDS,
        "attributable_usec",
        "observed_intervals",
        "entered",
        "exited",
        "end_reason",
        "quality_flags",
    ),
    denominator=(
        "Rates use an explicitly selected covered duration denominator; zero "
        "denominator yields null. Checkpoint totals are a separate absolute plane."
    ),
    rate_numerator_metrics=("attributable_usec",),
    rate_denominator_metrics=("covered_resident_usec",),
    rate_unit="fraction",
    zero_denominator_policy="null",
    missing_input_policy="null",
    distinct_semantics=(
        "Rows are one original logical session. Subject/account distinct counts "
        "are not additive across days or cohorts. No account linkage exists."
    ),
    distribution_semantics=(
        "Only per-session rows support a distribution; sums cannot produce a "
        "median or percentile."
    ),
)

COHORT_ACTIVITY = ReportDefinition(
    name="cohort_activity",
    definition_version=DEFINITION_VERSION,
    grain="utc_day_and_captured_context",
    table="telemetry_cohort_day",
    dimensions=COHORT_DIMENSION_FIELDS,
    metrics=(
        "duration_usec",
        "attributable_usec",
        "observed_intervals",
        "subject_count",
        "session_count",
        "quality_flags",
    ),
    denominator=(
        "Activity rates use duration_usec or attributable_usec selected by the "
        "caller; a zero denominator is null, never zero or infinity."
    ),
    rate_numerator_metrics=("attributable_usec",),
    rate_denominator_metrics=("duration_usec",),
    rate_unit="fraction",
    zero_denominator_policy="null",
    missing_input_policy="null",
    distinct_semantics=(
        "subject_count and session_count are daily cohort cardinalities backed by "
        "telemetry_cohort_member; do not sum them across days/cohorts."
    ),
    distribution_semantics=(
        "Member rows retain subject/session contributions for declared distributions; "
        "cohort sums alone cannot produce medians or percentiles."
    ),
)

REPORT_DEFINITIONS = MappingProxyType(
    {
        SESSION_PLAYTIME.name: SESSION_PLAYTIME,
        COHORT_ACTIVITY.name: COHORT_ACTIVITY,
    }
)

# Aliases are input conveniences only; emitted definition names remain stable.
REPORT_ALIASES = MappingProxyType(
    {
        "session": SESSION_PLAYTIME.name,
        "playtime": SESSION_PLAYTIME.name,
        "cohort": COHORT_ACTIVITY.name,
        "activity": COHORT_ACTIVITY.name,
    }
)

OBSERVATION_REPORT_DEFINITIONS = MappingProxyType({
    "progression_observations": ReportDefinition(
        name="progression_observations", definition_version=2, grain="subject_session_day_and_exact_source_dimensions",
        table="telemetry_rollup_progression_day",
        dimensions=("utc_day", "subject_id", "pid", *SESSION_ID_FIELDS, "level", "level_band", "class_id", "race_id",
                    "faction_id", "zone_vnum", "group_size", "config_id", "classifier_version", "policy_version",
                    "source", "reason", "observation_status", "modifier_flags"),
        metrics=("observations", "zero_applied_observations", "negative_applied_observations", "requested_xp",
                 "computed_xp", "applied_xp", "earned_positive_xp", "death_loss_xp", "restored_positive_xp", "quality_flags"),
        denominator="Amounts observed at the XP storage boundary. No hourly rate or persisted reward is inferred; keep observation statuses and configurations separate.",
        distinct_semantics="Subjects are characters; sessions, accounts and humans are different grains. No account/controller association is available.",
        distribution_semantics="Daily source cells support source totals, not per-award distributions. Level thresholds are excluded from all XP amounts.",
        unavailable_metrics=("xp_per_hour", "durable_reward_total", "account_distinct_count", "controller_distinct_count", "controller_union_time"),
        rate_unit="not_computed"),
    "level_observations": ReportDefinition(
        name="level_observations", definition_version=2, grain="observed_level_transition_replay_key",
        table="telemetry_rollup_level_event", dimensions=("subject_id", "pid", *SESSION_ID_FIELDS, "kind", "source", "reason", "observation_status"),
        metrics=("before_level", "after_level", "threshold_xp", "modifier_flags", "at_monotonic_usec", "occurrence_utc_usec", "quality_flags"),
        denominator="Observed transition points. threshold_xp is threshold consumption, never an XP reward. Missing beginning or end exposure is censored.",
        distinct_semantics="A row is a transition observation, not an independently observed player or attained milestone cohort.",
        distribution_semantics="Transitions are retained for later milestones; time to level is unavailable without compatible covered exposure and censoring.",
        unavailable_metrics=("xp_reward", "time_to_level", "controller_time_to_level"), rate_unit="not_computed"),
    "encounter_observations": ReportDefinition(
        name="encounter_observations", definition_version=2, grain="original_observed_encounter",
        table="telemetry_rollup_encounter", dimensions=("encounter_boot_id", "encounter_process_id", "encounter_seq", "config_id", "zone_vnum", "group_key"),
        metrics=("start_seen", "close_seen", "elapsed_usec", "outcome", "mode", "mode_mask", "participant_count", "expected_credit_count", "source_events", "quality_flags"),
        denominator="Independent start and close observations are retained. Only actual close supplies elapsedness. Expected reward credit is separate from participation.",
        distinct_semantics="Each row is one producer encounter; opposing encounters are not a shared battle, and a generic success is not a full zone clear.",
        distribution_semantics="Closed elapsed durations retain known termination reasons; unclosed rows have NULL elapsedness. Copyover, shutdown and unknown close are censored outcomes.",
        unavailable_metrics=("battle_win_rate", "zone_clear_rate", "unique_human_count"), rate_unit="not_computed"),
    "encounter_participants": ReportDefinition(
        name="encounter_participants", definition_version=2, grain="encounter_subject_and_pid",
        table="telemetry_rollup_encounter_participant", dimensions=("encounter_boot_id", "encounter_process_id", "encounter_seq", "subject_id", "pid"),
        metrics=("participant_usec", "effort_revision", "join_seen", "leave_seen", "summary_seen", "outcome", "quality_flags"),
        denominator="Latest absolute cumulative measured effort. Leave and summary are not summed, rejoins do not reset effort, and join-only effort is NULL.",
        distinct_semantics="Character participation is distinct from expected credit, account ownership and reviewed human control.",
        distribution_semantics="Participant effort distributions require complete declared encounter cohorts; unmeasured effort remains unknown.",
        unavailable_metrics=("expected_credit_effort", "controller_union_time", "unique_human_count"), rate_unit="not_computed"),
    "combat_contributions": ReportDefinition(
        name="combat_contributions", definition_version=2, grain="encounter_actor_with_captured_ownership",
        table="telemetry_rollup_combat_actor", dimensions=("encounter_boot_id", "encounter_process_id", "encounter_seq", "actor_kind", "actor_id", "actor_pid", "owner_subject_id", "mode", "config_id"),
        metrics=("damage_dealt", "damage_taken", "healing_attempted", "effective_healing", "overhealing", "control_applications",
                 "casting_attempts", "casting_completions", "casting_aborts", "casting_elapsed_usec", "tanking_usec", "quality_flags"),
        denominator="Latest absolute actor contribution snapshot. Actor counts and unique_player_count are captured per-encounter context and cannot be summed across actor rows.",
        distinct_semantics="Pets retain their captured owner; NPCs have no player owner. Neither is an extra human. power_band is a captured level proxy, not gear or skill strength.",
        distribution_semantics="Damage, effective healing, control and tanking remain distinct measures. No universal contribution or power score is computed.",
        unavailable_metrics=("battle_win_rate", "gear_strength", "skill_strength", "unique_human_count", "universal_power_score"), rate_unit="not_computed"),
})

IDENTITY_REPORT_DEFINITIONS = MappingProxyType({
    "identity_effort": ReportDefinition(
        name="identity_effort", definition_version=3, grain="dated_configuration_cell_and_explicit_identity_basis",
        table="telemetry_rollup_identity_effort",
        dimensions=("utc_day", "partition_kind", "basis", "identity_token", "config_id", "classifier_version",
                    "policy_version", "faction_id", "level_band", "group_context_mode", "category"),
        metrics=("character_usec", "utc_covered_character_usec", "unknown_clock_character_usec", "covered_union_usec",
                 "union_usec", "distinct_characters", "distinct_accounts", "quality_flags"),
        denominator="Exact observed input-derived intervals, split at ownership/review/day boundaries. Account and confirmed-controller clocks use interval unions; summed character effort remains separate. Unknown identity populations have NULL union clocks. Presence is a separate category when observed.",
        distinct_semantics="Characters, accounts and confirmed controllers are separate bases. Unknown controllers are not combined into one person. Portfolio and faction/level/observed-group partitions overlap and cannot be summed; union cells are not additive.",
        distribution_semantics="These aggregate union and summed-effort cells are not per-session duration samples or a complete human census.",
        account_metrics_available=True, unavailable_metrics=("continuous_human_attention", "complete_controller_population",
            "battle_presence_effort", "xp_per_hour"), rate_unit="not_computed"),
    "portfolio_progression": ReportDefinition(
        name="portfolio_progression", definition_version=3, grain="dated_configuration_identity_and_exact_xp_source_cell",
        table="telemetry_rollup_portfolio_xp",
        dimensions=("utc_day", "partition_kind", "basis", "identity_token", "config_id", "classifier_version",
                    "policy_version", "faction_id", "level_band", "group_context_mode", "source", "reason",
                    "observation_status", "modifier_flags"),
        metrics=("observations", "zero_applied_observations", "negative_applied_observations", "requested_xp", "computed_xp",
                 "applied_xp", "earned_positive_xp", "death_loss_xp", "restored_positive_xp", "linked_observations",
                 "clock_unknown_observations", "incident_affected_observations", "quality_flags"),
        denominator="XP observations attributed at their actual producer-clock point using retained authenticated ownership and the reserved dated review. Earned, lost, restored and administrative/source/status cells retain their separate meanings. No canonical economic reward or hourly rate is inferred.",
        distinct_semantics="Character, account and confirmed-controller bases are separate projections of the same observations; summing bases or overlapping partitions duplicates XP. Missing ownership and linkage stay explicit unknown populations.",
        distribution_semantics="Source aggregates support portfolio amount comparisons, not award distributions, milestones or causal rotation effects.",
        account_metrics_available=True, unavailable_metrics=("xp_per_hour", "durable_economic_reward_total", "time_to_milestone",
            "causal_rotation_advantage", "complete_controller_population"), rate_unit="not_computed"),
})

BATTLE_REPORT_DEFINITIONS = MappingProxyType({
    name: ReportDefinition(name=name, definition_version=BATTLE_DEFINITION_VERSION,
        grain=grain, table="telemetry_rollup_battle_row", dimensions=dimensions, metrics=metrics,
        denominator="Immutable shared-battle observations and exact measured presence. Character-owner counts are not account/controller counts. Amounts crossing identity boundaries are not divided. Unavailable metrics, clocks, identity and decisive outcomes remain explicit; no rate is computed.",
        distinct_semantics="Canonical battles and original alias lineage remain distinct. Actor snapshots, contribution segments and historical exposures are alternative projections; do not sum them together. Pets cannot borrow an unproven owner's authenticated identity.",
        distribution_semantics="Observed and censored lifecycles retain source/linkage/loss quality. No complete battle, player population, winner, gear strength or causal comparison is established by these observations.",
        account_metrics_available=name in ("battle_contributions", "battle_exposure"), unavailable_metrics=("battle_win_rate", "zone_clear_rate", "gear_strength",
            "complete_controller_population", "continuous_human_attention", "economic_reward_rate"), rate_unit="not_computed")
    for name, grain, dimensions, metrics in (
        ("battle_observations", "original_and_canonical_battle", ("battle", "canonical_battle", "canonical"),
            ("start_seen", "close_seen", "packet_history_complete", "contribution_count", "available_metric_mask", "outcome", "quality_flags")),
        ("battle_actors", "canonical_battle_and_latest_actor_snapshot", ("canonical_battle", "battle_actor_id", "battle_actor_kind"),
            ("battle_present_usec", "battle_contributor_usec", "effort_replay_verified", "quality_flags")),
        ("battle_contributions", "original_disjoint_contribution_segment", ("canonical_battle", "bc_segment_seq", "bc_actor_id", "account_token", "controller_token"),
            ("bc_damage_dealt", "bc_damage_taken", "bc_effective_healing", "bc_control_applications", "link_status", "attribution_status", "publication_quality_flags", "attribution_quality_flags")),
        ("battle_exposure", "dated_actor_context_and_observed_identity_interval", ("source_battle", "canonical_battle", "battle_actor_id", "utc_day", "battle_config_id", "battle_actor_class_id", "battle_actor_faction_id", "account_token", "controller_token"),
            ("battle_present_usec", "battle_contributor_usec", "battle_outnumbered_owner_usec", "observed_side_owners", "observed_opposing_owners", "linkage_status", "quality_flags")),
        ("battle_associations", "original_immutable_association_fact", ("battle_boot_id", "battle_process_id", "battle_seq", "battle_revision", "battle_fact_sequence"),
            ("battle_fact_kind", "battle_fact_count", "battle_actor_id", "battle_related_actor_id", "battle_related_battle_seq", "battle_mode", "battle_side_status", "battle_quality_flags", "projection_quality_flags")),
    )
})


def report_definition(name: str, definition_version: int | None = None) -> ReportDefinition:
    canonical = REPORT_ALIASES.get(name, name)
    try:
        definition = REPORT_DEFINITIONS.get(canonical) or OBSERVATION_REPORT_DEFINITIONS.get(canonical) or IDENTITY_REPORT_DEFINITIONS.get(canonical) or BATTLE_REPORT_DEFINITIONS[canonical]
    except KeyError as error:
        raise ValueError(
            f"unknown report definition {name!r}; "
            f"available={sorted((*REPORT_DEFINITIONS, *OBSERVATION_REPORT_DEFINITIONS, *IDENTITY_REPORT_DEFINITIONS, *BATTLE_REPORT_DEFINITIONS))}"
        ) from error
    if definition_version is None:
        return definition
    if isinstance(definition_version, bool) or not isinstance(definition_version, int) or definition_version not in SUPPORTED_DEFINITION_VERSIONS:
        raise ValueError("unsupported report definition version")
    if definition.definition_version > definition_version:
        raise ValueError(f"report requires definition version {definition.definition_version}")
    if canonical in OBSERVATION_REPORT_DEFINITIONS and definition_version != 2:
        raise ValueError("observation report requires definition version 2")
    if (canonical in BATTLE_REPORT_DEFINITIONS) != (definition_version == BATTLE_DEFINITION_VERSION):
        raise ValueError("battle reports require their independent definition version")
    return replace(definition, definition_version=definition_version)


def report_definitions(definition_version: int = 1) -> tuple[ReportDefinition, ...]:
    if definition_version == BATTLE_DEFINITION_VERSION:
        return tuple(report_definition(name, definition_version) for name in sorted(BATTLE_REPORT_DEFINITIONS))
    names = set(REPORT_DEFINITIONS)
    if definition_version == 2:
        names.update(OBSERVATION_REPORT_DEFINITIONS)
    if definition_version >= 3:
        names.update(IDENTITY_REPORT_DEFINITIONS)
    return tuple(report_definition(name, definition_version) for name in sorted(names))


def report_catalog(definition_version: int = 1) -> tuple[dict[str, Any], ...]:
    return tuple(definition.public_dict() for definition in report_definitions(definition_version))


def unknown_dimensions() -> dict[str, int]:
    return {
        "level_band": 0,
        "class_id": 0,
        "race_id": 0,
        "faction_id": 0,
        "zone_vnum": -1,
    }


def counters_from_mapping(row: Mapping[str, Any], prefix: str = "") -> tuple[int, ...]:
    return tuple(int(row[prefix + name]) for name in COUNTER_FIELDS)


def safe_rate(numerator: int | float, denominator: int | float) -> float | None:
    """Return a rate or ``None`` when its explicitly selected denominator is zero."""
    for name, value in (("numerator", numerator), ("denominator", denominator)):
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
            raise ValueError(f"{name} must be a finite number")
        if value < 0:
            raise ValueError(f"{name} must be nonnegative")
    if denominator == 0:
        return None
    return float(numerator) / float(denominator)


rate_or_none = safe_rate


__all__ = [
    "COHORT_ACTIVITY",
    "COHORT_DIMENSION_FIELDS",
    "COUNTER_FIELDS",
    "COVERED_COUNTER_FIELDS",
    "DEFINITION_VERSION",
    "CheckpointContribution",
    "INT32_MAX",
    "INT32_MIN",
    "MembershipContribution",
    "PUBLICATION_BUILDING",
    "PUBLICATION_FAILED",
    "PUBLICATION_PUBLISHED",
    "PUBLICATION_SUPERSEDED",
    "REPORT_DEFINITIONS",
    "REPORT_ALIASES",
    "ROLLUP_QUALITY_MASK",
    "ROLLUP_QUALITY_CHECKPOINT_CONFLICT",
    "ROLLUP_QUALITY_CONTEXT_UNAVAILABLE",
    "ROLLUP_QUALITY_DIMENSION_INVALID",
    "ROLLUP_QUALITY_KNOWN_MASK",
    "ROLLUP_QUALITY_LATE_INPUT",
    "ROLLUP_QUALITY_SESSION_GAP",
    "ROLLUP_QUALITY_INCIDENT_GAP",
    "ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN",
    "ROLLUP_QUALITY_LIFECYCLE_CONFLICT",
    "ROLLUP_QUALITY_PROCESS_GAP",
    "ROLLUP_QUALITY_UTC_BACKWARD",
    "ROLLUP_QUALITY_UTC_FANOUT",
    "ROLLUP_QUALITY_UTC_MISMATCH",
    "ROLLUP_QUALITY_UTC_UNKNOWN",
    "ReportDefinition",
    "ReportSnapshot",
    "RollupCoverage",
    "RollupTarget",
    "SESSION_ID_FIELDS",
    "SESSION_PLAYTIME",
    "SCOPE_FIELDS",
    "UINT16_MAX",
    "UINT32_MAX",
    "UINT64_MAX",
    "UNKNOWN_DAY",
    "UTC_UNKNOWN",
    "counters_from_mapping",
    "report_catalog",
    "report_definition",
    "report_definitions",
    "rate_or_none",
    "safe_rate",
    "unknown_dimensions",
    "validate_target",
]
