"""Bounded typed projections of collected progression/encounter/combat facts.

These are observations, not gameplay authorities, shared battles, or proof of a
zone clear. All functions are pure; the existing rollup transaction owns replay
and publication. The raw parser follows telemetry_types.h, including bit 9's
cardinality loss and the separate scope/quality prefixes for kinds 7 and 8.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from datetime import date, timedelta
import hashlib
from typing import Any, Mapping

try:
    from .rollup_definitions import (
        RollupTarget, UNKNOWN_DAY, UTC_UNKNOWN, UINT64_MAX,
        ROLLUP_QUALITY_LATE_INPUT, ROLLUP_QUALITY_UTC_UNKNOWN,
        ROLLUP_QUALITY_UTC_MISMATCH,
    )
except ImportError:
    from rollup_definitions import (
        RollupTarget, UNKNOWN_DAY, UTC_UNKNOWN, UINT64_MAX,
        ROLLUP_QUALITY_LATE_INPUT, ROLLUP_QUALITY_UTC_UNKNOWN,
        ROLLUP_QUALITY_UTC_MISMATCH,
    )

DEFINITION_VERSION = 2
INT64_MIN, INT64_MAX = -(1 << 63), (1 << 63) - 1
DAY_USEC = 86_400_000_000
SCOPE = ("definition_version", "generation", "environment_id", "season_id")
SESSION = ("session_boot_id", "session_process_id", "session_seq")
ENCOUNTER = ("encounter_boot_id", "encounter_process_id", "encounter_seq")
SOURCE = ("config_id", "classifier_version", "policy_version", "zone_vnum", "group_key")
DIMENSIONS = ("level_band", "class_id", "race_id", "faction_id", "zone_vnum", "group_size")
XP_METRICS = ("observations", "zero_applied_observations", "negative_applied_observations",
              "requested_xp", "computed_xp", "applied_xp", "earned_positive_xp",
              "death_loss_xp", "restored_positive_xp")
COMBAT_METRICS = ("damage_dealt", "damage_taken", "healing_attempted", "effective_healing",
                  "overhealing", "control_applications", "casting_attempts",
                  "casting_completions", "casting_aborts", "casting_elapsed_usec", "tanking_usec")
PROGRESSION_RAW_COLUMNS = tuple("progression_" + name for name in (
    "kind", "source", "reason", "observation_status", "modifier_flags", "requested_xp",
    "computed_xp", "applied_xp", "before_exp", "after_exp", "before_level", "after_level", "threshold_xp"))
ENCOUNTER_RAW_COLUMNS = tuple("encounter_" + name for name in (
    "boot_id", "process_id", "seq", "event", "mode", "outcome", "revision", "environment_id",
    "season_id", "config_id", "classifier_version", "policy_version", "zone_vnum", "group_key",
    "participant_subject_id", "participant_pid", "start_monotonic_usec", "start_utc_usec", "quality_flags")) + (
    "elapsed_usec", "participant_usec", "participant_count", "expected_credit_count")
COMBAT_RAW_COLUMNS = tuple("combat_" + name for name in (
    "encounter_boot_id", "encounter_process_id", "encounter_seq", "mode", "outcome", "revision",
    "environment_id", "season_id", "config_id", "classifier_version", "policy_version", "zone_vnum",
    "group_key", "actor_id", "actor_pid", "owner_subject_id", "actor_kind", "unique_player_count",
    "participant_count", "dropped_participant_count", "power_band", "opponent_power_band", "opponent_count",
    "modifier_flags", "start_monotonic_usec", "end_monotonic_usec", "start_utc_usec", "end_utc_usec",
    *COMBAT_METRICS, "quality_flags"))
OWNERSHIP_RAW_COLUMNS = ("ownership_account_token", "ownership_source")
OWNERSHIP_ALLOWED_COLUMNS = frozenset(("ingest_id", "boot_id", "process_id", "record_seq",
    "schema_version", "record_kind", "occurrence_utc_usec", "ingested_utc_usec",
    "environment_id", "season_id", *SESSION, "subject_id", "pid", "connection_boot_id",
    "connection_process_id", "connection_seq", "at_monotonic_usec", "at_utc_usec",
    *DIMENSIONS, "config_id", "classifier_version", "policy_version", "quality_flags",
    *OWNERSHIP_RAW_COLUMNS))

XP_DIMENSIONS = ("level", *DIMENSIONS, "config_id", "classifier_version", "policy_version", "source", "reason",
                 "observation_status", "modifier_flags")
# Both supported SQL engines cap an index at 16 columns. Retain all typed
# dimensions and verify them on every merge; the fixed digest only bounds PK
# width and is never a replacement for readable dimensional evidence.
XP_KEY = (*SCOPE, "utc_day", "subject_id", *SESSION, "cell_digest")
LEVEL_KEY = (*SCOPE, "boot_id", "process_id", "record_seq")
EPISODE_KEY = (*SCOPE, *ENCOUNTER)
PARTICIPANT_KEY = (*EPISODE_KEY, "subject_id", "pid")
ACTOR_KEY = (*EPISODE_KEY, "actor_kind", "actor_id")
XP_COLUMNS = (*XP_KEY, "pid", *XP_DIMENSIONS, *XP_METRICS, "quality_flags", "input_watermark")
LEVEL_COLUMNS = (*LEVEL_KEY, "subject_id", "pid", *SESSION, "kind", "source", "reason",
                 "observation_status", "modifier_flags", "before_level", "after_level", "threshold_xp", "at_monotonic_usec",
                 "occurrence_utc_usec", *DIMENSIONS, "config_id", "classifier_version", "policy_version",
                 "quality_flags", "input_watermark")
EPISODE_COLUMNS = (*EPISODE_KEY, *SOURCE, "start_monotonic_usec", "start_utc_usec", "start_seen",
                   "close_seen", "end_monotonic_usec", "end_utc_usec", "elapsed_usec", "outcome",
                   "mode", "mode_mask", "latest_revision", "latest_event", "latest_at_monotonic_usec",
                   "participant_count", "expected_credit_count", "source_events", "quality_flags", "input_watermark")
PARTICIPANT_COLUMNS = (*PARTICIPANT_KEY, "latest_revision", "latest_event", "latest_at_monotonic_usec",
                       "effort_revision", "participant_usec", "join_seen", "leave_seen", "summary_seen", "outcome",
                       "quality_flags", "input_watermark")
ACTOR_COLUMNS = (*ACTOR_KEY, *SOURCE, "actor_pid", "owner_subject_id", "revision", "mode", "outcome",
                 "start_monotonic_usec", "end_monotonic_usec", "start_utc_usec", "end_utc_usec",
                 "unique_player_count", "participant_count", "dropped_participant_count", "power_band",
                 "opponent_power_band", "opponent_count", "modifier_flags", *COMBAT_METRICS,
                 "quality_flags", "input_watermark")


class ObservationError(ValueError):
    """A typed source or cumulative projection cannot be interpreted safely."""


def integer(value: Any, name: str, lower: int = 0, upper: int = UINT64_MAX) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or not lower <= value <= upper:
        raise ObservationError(name + "_range")
    return value


def _get(row: Mapping[str, Any], name: str, lower=0, upper=UINT64_MAX) -> int:
    return integer(row.get(name), name, lower, upper)


def _signed(row: Mapping[str, Any], name: str) -> int:
    return _get(row, name, INT64_MIN, INT64_MAX)


def _quality(row: Mapping[str, Any], name: str) -> int:
    value = _get(row, name, 0, 1023)
    return value | (ROLLUP_QUALITY_LATE_INPUT if value & (1 << 8) else 0)


def validate_observation(row: Mapping[str, Any]) -> dict[str, Any]:
    """Parse exactly the active typed family; never infer absent union fields."""
    kind = _get(row, "record_kind", 6, 9)
    result = {name: _get(row, name, 1) for name in ("ingest_id", "boot_id", "process_id", "record_seq")}
    if _get(row, "schema_version", 1, 1) != 1:
        raise ObservationError("schema_version")
    result.update(record_kind=kind, occurrence_utc_usec=_signed(row, "occurrence_utc_usec"))
    for family, columns in ((6, PROGRESSION_RAW_COLUMNS), (7, ENCOUNTER_RAW_COLUMNS),
                            (8, COMBAT_RAW_COLUMNS), (9, OWNERSHIP_RAW_COLUMNS)):
        if family != kind and any(row.get(name) is not None for name in columns):
            raise ObservationError("inactive_observation_fields")
    prefix = "" if kind in (6, 9) else "encounter_" if kind == 7 else "combat_"
    for name in ("environment_id", "season_id", "config_id"):
        result[name] = _get(row, prefix + name, 1)
    for name in ("classifier_version", "policy_version"):
        result[name] = _get(row, prefix + name, 1, (1 << 32) - 1)
    result["zone_vnum"] = _get(row, prefix + "zone_vnum", -1, (1 << 31) - 1)
    result["quality_flags"] = _quality(row, prefix + "quality_flags")
    if kind in (6, 9):
        for name in SESSION:
            result[name] = _get(row, name, 1)
        result["subject_id"] = _get(row, "subject_id", 1)
        result["pid"] = _get(row, "pid", 1, (1 << 31) - 1)
        connection = [row.get(name) for name in ("connection_boot_id", "connection_process_id", "connection_seq")]
        if kind == 6 and all(value == 0 for value in connection):
            for name, value in zip(("connection_boot_id", "connection_process_id", "connection_seq"), connection):
                integer(value, name, 0, 0)
        else:
            for name, value in zip(("connection_boot_id", "connection_process_id", "connection_seq"), connection):
                result[name] = integer(value, name, 1)
        for name in DIMENSIONS:
            if name != "zone_vnum":
                result[name] = _get(row, name, 0, (1 << (32 if name == "group_size" else 16)) - 1)
        result["at_monotonic_usec"] = _get(row, "at_monotonic_usec")
        result["at_utc_usec"] = _signed(row, "at_utc_usec")
        if kind == 9:
            if any(value is not None and name not in OWNERSHIP_ALLOWED_COLUMNS
                   for name, value in row.items()):
                raise ObservationError("inactive_ownership_fields")
            result["account_token"] = _get(row, "ownership_account_token")
            result["source"] = _get(row, "ownership_source", 1, 5)
            if (result["account_token"] == 0) != (result["source"] == 5):
                raise ObservationError("ownership_token_source")
            if result["at_utc_usec"] != result["occurrence_utc_usec"]:
                raise ObservationError("ownership_clock_pair")
            if (result["connection_boot_id"], result["connection_process_id"]) != (result["boot_id"], result["process_id"]):
                raise ObservationError("ownership_producer_clock")
            return result
        for name, lower, upper in (("kind", 1, 3), ("source", 0, 12), ("reason", 0, 6),
                                    ("observation_status", 1, 3), ("modifier_flags", 0, 511),
                                    ("before_level", 0, 65535), ("after_level", 0, 65535)):
            result[name] = _get(row, "progression_" + name, lower, upper)
        for name in ("requested_xp", "computed_xp", "applied_xp", "before_exp", "after_exp"):
            result[name] = _signed(row, "progression_" + name)
        result["threshold_xp"] = _get(row, "progression_threshold_xp")
        if result["kind"] == 1:
            if result["before_level"] != result["after_level"] or result["threshold_xp"] != 0:
                raise ObservationError("xp_level_or_threshold")
            if result["after_exp"] - result["before_exp"] != result["applied_xp"]:
                raise ObservationError("xp_storage_delta")
        else:
            if any(result[name] != 0 for name in ("requested_xp", "computed_xp", "applied_xp", "before_exp", "after_exp")):
                raise ObservationError("level_has_xp_reward")
            direction = result["after_level"] - result["before_level"]
            if (result["kind"] == 2 and direction <= 0) or (result["kind"] == 3 and direction >= 0):
                raise ObservationError("level_direction")
    else:
        for name in ENCOUNTER:
            result[name] = _get(row, ("combat_" if kind == 8 else "") + name, 1)
        result["group_key"] = _get(row, prefix + "group_key")
        result["mode"] = _get(row, prefix + "mode", 0 if kind == 7 else 1, 3)
        result["outcome"] = _get(row, prefix + "outcome", 0 if kind == 7 else 1, 10)
        result["revision"] = _get(row, prefix + "revision", 1, 65535)
        result["start_monotonic_usec"] = _get(row, prefix + "start_monotonic_usec")
        result["start_utc_usec"] = _signed(row, prefix + "start_utc_usec")
        if kind == 7:
            result["event"] = _get(row, "encounter_event", 1, 5)
            result["subject_id"] = _get(row, "encounter_participant_subject_id")
            result["pid"] = _get(row, "encounter_participant_pid", -(1 << 31), (1 << 31) - 1)
            result["at_monotonic_usec"] = _get(row, "at_monotonic_usec", result["start_monotonic_usec"])
            result["at_utc_usec"] = _signed(row, "at_utc_usec")
            for name in ("elapsed_usec", "participant_usec"):
                result[name] = _get(row, name)
            for name in ("participant_count", "expected_credit_count"):
                result[name] = _get(row, name, 0, 65535)
            participant = result["event"] in (2, 3, 5)
            if participant != (result["subject_id"] > 0 and result["pid"] > 0):
                raise ObservationError("encounter_participant_identity")
            if result["event"] == 1 and result["outcome"] != 0:
                raise ObservationError("start_has_outcome")
            if result["event"] in (3, 4, 5) and result["outcome"] == 0:
                raise ObservationError("terminal_outcome_unknown")
            if result["event"] in (1, 2) and (result["elapsed_usec"] or result["participant_usec"]):
                raise ObservationError("start_or_join_has_elapsed")
            if result["event"] == 4 and (result["subject_id"] or result["pid"] or
                result["elapsed_usec"] != result["at_monotonic_usec"] - result["start_monotonic_usec"]):
                raise ObservationError("close_elapsed_or_participant")
        else:
            result["end_monotonic_usec"] = _get(row, "combat_end_monotonic_usec", result["start_monotonic_usec"])
            result["end_utc_usec"] = _signed(row, "combat_end_utc_usec")
            result["actor_kind"] = _get(row, "combat_actor_kind", 1, 3)
            result["actor_id"] = _get(row, "combat_actor_id", 1)
            result["actor_pid"] = _get(row, "combat_actor_pid", -1, (1 << 31) - 1)
            result["owner_subject_id"] = _get(row, "combat_owner_subject_id")
            if ((result["actor_kind"] == 1 and (result["actor_pid"] <= 0 or result["owner_subject_id"] != result["actor_id"])) or
                (result["actor_kind"] == 2 and (result["actor_pid"] != -1 or result["owner_subject_id"] == 0)) or
                (result["actor_kind"] == 3 and (result["actor_pid"] != -1 or result["owner_subject_id"] != 0))):
                raise ObservationError("combat_ownership")
            for name in ("unique_player_count", "participant_count"):
                result[name] = _get(row, "combat_" + name, 0, 64)
            for name in ("dropped_participant_count", "power_band", "opponent_power_band", "opponent_count"):
                result[name] = _get(row, "combat_" + name, 0, 65535)
            result["modifier_flags"] = _get(row, "combat_modifier_flags", 0, 255)
            for name in COMBAT_METRICS:
                result[name] = _get(row, "combat_" + name)
            if (result["effective_healing"] > result["healing_attempted"] or
                result["casting_completions"] + result["casting_aborts"] > result["casting_attempts"]):
                raise ObservationError("combat_conservation")
    return result


def point_day(observation: Mapping[str, Any]) -> tuple[date, int, int | None]:
    quality = int(observation["quality_flags"])
    header = observation["occurrence_utc_usec"]
    at = observation["at_utc_usec"] if observation["record_kind"] != 8 else observation["end_utc_usec"]
    if header == UTC_UNKNOWN or at == UTC_UNKNOWN:
        return UNKNOWN_DAY, quality | ROLLUP_QUALITY_UTC_UNKNOWN, None
    if header != at or quality & (1 << 7):
        return UNKNOWN_DAY, quality | ROLLUP_QUALITY_UTC_MISMATCH, None
    try:
        day = date(1970, 1, 1) + timedelta(days=header // DAY_USEC)
    except (OverflowError, ValueError):
        return UNKNOWN_DAY, quality | ROLLUP_QUALITY_UTC_MISMATCH, None
    if day <= UNKNOWN_DAY:
        return UNKNOWN_DAY, quality | ROLLUP_QUALITY_UTC_MISMATCH, None
    if observation["record_kind"] == 8 or (observation["record_kind"] == 7 and observation["event"] == 4):
        start = observation["start_utc_usec"]
        elapsed = (observation["end_monotonic_usec"] - observation["start_monotonic_usec"]
                   if observation["record_kind"] == 8 else observation["elapsed_usec"])
        if start == UTC_UNKNOWN:
            quality |= ROLLUP_QUALITY_UTC_UNKNOWN
        elif at - start != elapsed:
            quality |= ROLLUP_QUALITY_UTC_MISMATCH
    return day, quality, header


def checked_add(left: int, right: int, name: str, *, signed=False) -> int:
    lower, upper = (INT64_MIN, INT64_MAX) if signed else (0, UINT64_MAX)
    return integer(left + right, name, lower, upper)


def _identity(values: Mapping[str, Any], names: tuple[str, ...]) -> tuple[Any, ...]:
    return tuple(values[name] for name in names)


@dataclass(slots=True)
class ObservationPage:
    progression: dict[tuple[Any, ...], dict[str, Any]] = field(default_factory=dict)
    levels: dict[tuple[Any, ...], dict[str, Any]] = field(default_factory=dict)
    episodes: dict[tuple[Any, ...], list[dict[str, Any]]] = field(default_factory=dict)
    participants: dict[tuple[Any, ...], list[dict[str, Any]]] = field(default_factory=dict)
    actors: dict[tuple[Any, ...], list[dict[str, Any]]] = field(default_factory=dict)

    @property
    def output_fanout(self) -> int:
        return sum(len(rows) for rows in (self.progression, self.levels, self.episodes, self.participants, self.actors))

    def add(self, observation: Mapping[str, Any], target: RollupTarget, day: date, quality: int,
            occurrence: int | None) -> None:
        if target.definition_version != DEFINITION_VERSION:
            raise ObservationError("observation_definition_version")
        row = dict(observation, **dict(zip(SCOPE, target.scope_tuple)))
        row.update(quality_flags=quality, input_watermark=row.pop("ingest_id"))
        row["occurrence_utc_usec"] = occurrence
        for name in ("start_utc_usec", "end_utc_usec"):
            if row.get(name) == UTC_UNKNOWN:
                row[name] = None
        if row["record_kind"] == 6:
            row.update(utc_day=day, level=row["before_level"])
            if row["kind"] != 1:
                row["occurrence_utc_usec"] = occurrence
                self.levels[_identity(row, LEVEL_KEY)] = {name: row[name] for name in LEVEL_COLUMNS}
                return
            canonical = "telemetry-xp-cell-v2|" + "|".join(name + "=" + str(row[name]) for name in XP_DIMENSIONS)
            row["cell_digest"] = hashlib.sha256(canonical.encode("ascii")).digest()
            key = _identity(row, XP_KEY)
            if key not in self.progression:
                self.progression[key] = {name: row[name] for name in (*XP_KEY, "pid", *XP_DIMENSIONS)}
                self.progression[key].update({name: 0 for name in XP_METRICS})
                self.progression[key].update(quality_flags=0, input_watermark=0)
            delta = self.progression[key]
            if any(delta[name] != row[name] for name in ("pid", *XP_DIMENSIONS)):
                raise ObservationError("progression_cell_identity_conflict")
            amounts = dict(observations=1, zero_applied_observations=int(row["applied_xp"] == 0),
                           negative_applied_observations=int(row["applied_xp"] < 0),
                           requested_xp=row["requested_xp"], computed_xp=row["computed_xp"], applied_xp=row["applied_xp"],
                           earned_positive_xp=max(0, row["applied_xp"]) if row["reason"] == 1 and 1 <= row["source"] <= 10 else 0,
                           death_loss_xp=max(0, -row["applied_xp"]) if row["reason"] == 2 else 0,
                           restored_positive_xp=max(0, row["applied_xp"]) if row["reason"] == 3 else 0)
            for name in XP_METRICS:
                delta[name] = checked_add(delta[name], amounts[name], name,
                                           signed=name in ("requested_xp", "computed_xp", "applied_xp"))
            delta["quality_flags"] |= quality
            delta["input_watermark"] = max(delta["input_watermark"], row["input_watermark"])
        elif row["record_kind"] == 7:
            self.episodes.setdefault(_identity(row, EPISODE_KEY), []).append(row)
            if row["event"] in (2, 3, 5):
                self.participants.setdefault(_identity(row, PARTICIPANT_KEY), []).append(row)
        elif row["record_kind"] == 8:
            self.actors.setdefault(_identity(row, ACTOR_KEY), []).append(row)


def merge_progression(existing: Mapping[str, Any] | None, delta: Mapping[str, Any]) -> dict[str, Any]:
    if existing is None:
        return dict(delta)
    if any(existing[name] != delta[name] for name in (*XP_KEY, "pid", *XP_DIMENSIONS)):
        raise ObservationError("progression_identity_conflict")
    result = dict(existing)
    for name in XP_METRICS:
        result[name] = checked_add(int(existing[name]), delta[name], name,
                                  signed=name in ("requested_xp", "computed_xp", "applied_xp"))
    result["quality_flags"] |= delta["quality_flags"]
    result["input_watermark"] = max(result["input_watermark"], delta["input_watermark"])
    return result


def _same(existing: Mapping[str, Any], row: Mapping[str, Any], fields: tuple[str, ...], name: str) -> None:
    if any(existing[field] != row[field] for field in fields):
        raise ObservationError(name + "_conflict")


def _evidence(result: dict[str, Any], row: Mapping[str, Any]) -> None:
    result["quality_flags"] |= row["quality_flags"]
    result["input_watermark"] = max(result["input_watermark"], row["input_watermark"])


def merge_episode(existing: Mapping[str, Any] | None, rows: list[Mapping[str, Any]]) -> dict[str, Any]:
    """Retain independent start/close evidence; only a close supplies elapsedness."""
    result = None if existing is None else dict(existing)
    anchors = (*EPISODE_KEY, *SOURCE, "start_monotonic_usec", "start_utc_usec")
    for row in rows:
        if result is None:
            result = {name: row[name] for name in anchors}
            result.update(start_seen=0, close_seen=0, end_monotonic_usec=None, end_utc_usec=None,
                          elapsed_usec=None, outcome=0, mode=0, mode_mask=0, latest_revision=0,
                          latest_event=0, latest_at_monotonic_usec=0, participant_count=0,
                          expected_credit_count=0, source_events=0, quality_flags=0, input_watermark=0)
        _same(result, row, anchors, "episode_anchor")
        result["start_seen"] |= int(row["event"] == 1)
        result["mode_mask"] |= 1 << row["mode"]
        if row["event"] == 4:
            close = dict(end_monotonic_usec=row["at_monotonic_usec"], end_utc_usec=row["occurrence_utc_usec"],
                         elapsed_usec=row["elapsed_usec"], outcome=row["outcome"])
            if result["close_seen"]:
                _same(result, close, tuple(close), "episode_close")
            result.update(close, close_seen=1)
        latest = dict(latest_revision=row["revision"], latest_event=row["event"],
                      latest_at_monotonic_usec=row["at_monotonic_usec"], mode=row["mode"],
                      participant_count=row["participant_count"], expected_credit_count=row["expected_credit_count"])
        if row["revision"] > result["latest_revision"]:
            result.update(latest)
        elif row["revision"] == result["latest_revision"]:
            _same(result, latest, tuple(latest), "episode_revision")
        result["source_events"] = checked_add(result["source_events"], 1, "episode_source_events")
        _evidence(result, row)
    if result is None:
        raise ObservationError("empty_episode_delta")
    return result


def merge_participant(existing: Mapping[str, Any] | None, rows: list[Mapping[str, Any]]) -> dict[str, Any]:
    """Leave and summary are absolute cumulative effort, never additive deltas.

    Joins prove roster presence but provide no measured effort. A later rejoin
    cannot reset previously measured effort, including when ingestion is late.
    """
    result = None if existing is None else dict(existing)
    for row in rows:
        if result is None:
            result = {name: row[name] for name in PARTICIPANT_KEY}
            result.update(latest_revision=0, latest_event=0, latest_at_monotonic_usec=0,
                          effort_revision=0, participant_usec=None, join_seen=0, leave_seen=0,
                          summary_seen=0, outcome=0, quality_flags=0, input_watermark=0)
        _same(result, row, PARTICIPANT_KEY, "participant_identity")
        for event, name in ((2, "join_seen"), (3, "leave_seen"), (5, "summary_seen")):
            result[name] |= int(row["event"] == event)
        if row["event"] in (3, 5):
            revision, old = row["revision"], result["effort_revision"]
            amount, prior = row["participant_usec"], result["participant_usec"]
            if old and ((revision > old and amount < prior) or (revision < old and amount > prior)):
                raise ObservationError("participant_cumulative_regression")
            if revision > old:
                result.update(effort_revision=revision, participant_usec=amount, outcome=row["outcome"])
            elif revision == old and (amount != prior or row["outcome"] != result["outcome"]):
                raise ObservationError("participant_effort_revision_conflict")
        latest = dict(latest_revision=row["revision"], latest_event=row["event"],
                      latest_at_monotonic_usec=row["at_monotonic_usec"])
        if row["revision"] > result["latest_revision"]:
            result.update(latest)
        elif row["revision"] == result["latest_revision"]:
            _same(result, latest, tuple(latest), "participant_revision")
        _evidence(result, row)
    if result is None:
        raise ObservationError("empty_participant_delta")
    return result


def merge_actor(existing: Mapping[str, Any] | None, rows: list[Mapping[str, Any]]) -> dict[str, Any]:
    """Project the latest absolute actor summary with immutable captured ownership."""
    result = None if existing is None else dict(existing)
    anchors = (*ACTOR_KEY, *SOURCE, "actor_pid", "owner_subject_id", "start_monotonic_usec", "start_utc_usec")
    payload = tuple(name for name in ACTOR_COLUMNS if name not in ("quality_flags", "input_watermark"))
    for row in rows:
        current = {name: row[name] for name in ACTOR_COLUMNS}
        if result is None:
            result = current
            continue
        _same(result, current, anchors, "actor_identity")
        if current["revision"] == result["revision"]:
            _same(result, current, payload, "actor_revision")
        else:
            earlier, later = (result, current) if current["revision"] > result["revision"] else (current, result)
            if any(later[name] < earlier[name] for name in (*COMBAT_METRICS, "end_monotonic_usec")):
                raise ObservationError("actor_cumulative_regression")
            if current["revision"] > result["revision"]:
                _evidence(current, result)
                result = current
        _evidence(result, current)
    if result is None:
        raise ObservationError("empty_actor_delta")
    return result
