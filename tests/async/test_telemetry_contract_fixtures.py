#!/usr/bin/env python3
"""Standalone #260 golden-fixture validator and specification oracle.

The JSON files are named-field semantic fixtures for the public telemetry
headers.  Top-level configurations are explicit pre-admitted control state;
configuration records are used where publication order itself is under test.
Config fingerprints are known answers for the documented fixed-width,
big-endian SHA-256; effective_utc_usec remains excluded from that identity.
This is deliberately not a runtime, SQL, or gameplay test; the oracle
validates the contract's record invariants and simulates only the repository
outcomes needed by the fixtures.
"""

from __future__ import annotations

from collections import Counter, defaultdict
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import re
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
FIXTURE_DIR = ROOT / "tests" / "async" / "fixtures" / "telemetry" / "contract"
FIXTURE_SCHEMA = "duris.telemetry.contract"
FIXTURE_SCHEMA_VERSION = 1
TELEMETRY_SCHEMA_VERSION = 1
DAY_USEC = 86_400_000_000
UINT64_MAX = (1 << 64) - 1
UINT32_MAX = (1 << 32) - 1
UINT16_MAX = (1 << 16) - 1
INT64_MIN = -(1 << 63)
INT64_MAX = (1 << 63) - 1
QUALITY_KNOWN = (1 << 9) - 1
CONFIG_FINGERPRINT_RE = re.compile(r"^[0-9a-f]{64}$")

RECORD_KINDS = {
    "interval",
    "session_lifecycle",
    "session_checkpoint",
    "coverage_gap",
    "configuration",
}
LIFECYCLE_KINDS = {
    "session_entered",
    "session_exited",
    "connection_attached",
    "connection_detached",
}
END_REASONS = {"unknown", "logout", "disconnect", "shutdown", "copyover", "process_restart"}
TRANSITION_KINDS = {"attached", "detached", "copyover_resumed"}
INTERVAL_CATEGORIES = {"unknown", "connected_idle", "connected_active", "resident_linkdead"}
ACTIVITY_CONTEXTS = {
    "unknown",
    "none",
    "combat",
    "travel",
    "social",
    "crafting",
    "administration",
    "other",
    "overflow_unknown",
}
CONTEXT_QUALITIES = {"unknown", "observed", "partial", "overflow", "unavailable"}
GAP_REASONS = {
    "detail_queue_drop",
    "control_queue_drop",
    "sequence_gap",
    "telemetry_disabled",
    "unclosed_tail",
    "clock_discontinuity",
}
BACKENDS = {"sql", "flatfile_disabled"}
ACKNOWLEDGEMENTS = {"confirmed", "ambiguous"}
SERVER_EFFECTS = {"committed", "rolled_back"}
BATCH_OUTCOMES = {
    "committed",
    "committed_with_rejections",
    "invalid_batch",
    "retryable_failure",
    "commit_ambiguous",
    "unavailable",
    "disabled",
}
RECORD_OUTCOMES = {
    "applied",
    "duplicate_identical",
    "checkpoint_older",
    "rejected_invalid",
    "duplicate_conflict",
    "retryable_failure",
    "commit_ambiguous",
    "unavailable",
    "disabled",
}

EXPECTED_COVERAGE = {
    "normal_interval": {"normal_interval", "conservation"},
    "replay_identical_conflict": {"replay_identical", "replay_conflict"},
    "commit_ambiguity_retry": {"commit_ambiguity", "immutable_retry"},
    "checkpoint_revision_conflict": {
        "checkpoint_stale",
        "checkpoint_new",
        "checkpoint_revision_conflict",
        "counter_conservation",
    },
    "drop_recovery_no_invented_context": {
        "detail_drop",
        "recovered_totals",
        "no_invented_context",
    },
    "level_zone_config_boundary": {"level_boundary", "zone_boundary", "config_boundary"},
    "midnight_clock_jump": {"midnight", "utc_clock_jump", "monotonic_duration"},
    "detach_reconnect": {"detach", "reconnect", "session_retained", "new_connection"},
    "copyover_handoff": {"copyover", "new_producer", "session_retained", "new_connection"},
    "unclosed_crash_tail": {"unclosed_crash_tail"},
}

# These are the only aggregate metric names used by the current fixtures.
METRIC_NAMES = {
    "conservation",
    "replay",
    "retry",
    "checkpoint",
    "coverage",
    "boundaries",
    "time",
    "session",
    "copyover",
    "tail",
}
EXPECTED_METRICS = {
    "normal_interval": {"conservation"},
    "replay_identical_conflict": {"conservation", "replay"},
    "commit_ambiguity_retry": {"conservation", "retry"},
    "checkpoint_revision_conflict": {"conservation", "checkpoint"},
    "drop_recovery_no_invented_context": {"conservation", "coverage"},
    "level_zone_config_boundary": {"conservation", "boundaries"},
    "midnight_clock_jump": {"conservation", "time"},
    "detach_reconnect": {"conservation", "session"},
    "copyover_handoff": {"conservation", "copyover"},
    "unclosed_crash_tail": {"conservation", "tail"},
}


class Diagnostics:
    def __init__(self) -> None:
        self.items: list[str] = []

    def add(self, path: str, message: str) -> None:
        self.items.append(f"{path}: {message}")

    def extend(self, other: "Diagnostics") -> None:
        self.items.extend(other.items)


def canonical(value: Any) -> str:
    """Canonical typed JSON content; object order is not record identity."""
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def object_value(
    value: Any,
    path: str,
    required: set[str],
    optional: set[str],
    diagnostics: Diagnostics,
) -> dict[str, Any]:
    if not isinstance(value, dict):
        diagnostics.add(path, "expected an object")
        return {}
    keys = set(value)
    unknown = sorted(keys - required - optional)
    missing = sorted(required - keys)
    if unknown:
        diagnostics.add(path, f"unknown fields: {', '.join(unknown)}")
    if missing:
        diagnostics.add(path, f"missing fields: {', '.join(missing)}")
    return value


def list_value(value: Any, path: str, diagnostics: Diagnostics) -> list[Any]:
    if not isinstance(value, list):
        diagnostics.add(path, "expected an array")
        return []
    return value


def string_value(value: Any, path: str, diagnostics: Diagnostics) -> str | None:
    if not isinstance(value, str):
        diagnostics.add(path, "expected a string")
        return None
    return value


def integer_value(
    value: Any,
    path: str,
    diagnostics: Diagnostics,
    minimum: int | None = None,
    maximum: int | None = None,
) -> int | None:
    if not isinstance(value, int) or isinstance(value, bool):
        diagnostics.add(path, "expected an integer")
        return None
    if minimum is not None and value < minimum:
        diagnostics.add(path, f"must be >= {minimum}")
    if maximum is not None and value > maximum:
        diagnostics.add(path, f"must be <= {maximum}")
    return value


def bool_value(value: Any, path: str, diagnostics: Diagnostics) -> bool | None:
    if not isinstance(value, bool):
        diagnostics.add(path, "expected a boolean")
        return None
    return value


def enum_value(value: Any, path: str, allowed: set[str], diagnostics: Diagnostics) -> str | None:
    value = string_value(value, path, diagnostics)
    if value is not None and value not in allowed:
        diagnostics.add(path, f"unknown value {value!r}")
    return value


def validate_producer(value: Any, path: str, diagnostics: Diagnostics) -> tuple[int, int] | None:
    value = object_value(value, path, {"boot_id", "process_id"}, set(), diagnostics)
    boot = integer_value(value.get("boot_id"), f"{path}.boot_id", diagnostics, 1, UINT64_MAX)
    process = integer_value(value.get("process_id"), f"{path}.process_id", diagnostics, 1, UINT64_MAX)
    if boot is None or process is None:
        return None
    return boot, process


def producer_token(value: dict[str, Any]) -> tuple[int, int]:
    return int(value["boot_id"]), int(value["process_id"])


def validate_record_key(value: Any, path: str, diagnostics: Diagnostics) -> tuple[Any, ...] | None:
    value = object_value(value, path, {"producer", "record_seq"}, set(), diagnostics)
    producer = validate_producer(value.get("producer"), f"{path}.producer", diagnostics)
    sequence = integer_value(value.get("record_seq"), f"{path}.record_seq", diagnostics, 1, UINT64_MAX)
    if producer is None or sequence is None:
        return None
    return (*producer, sequence)


def validate_session_id(value: Any, path: str, diagnostics: Diagnostics) -> tuple[Any, ...] | None:
    value = object_value(value, path, {"producer", "session_seq"}, set(), diagnostics)
    producer = validate_producer(value.get("producer"), f"{path}.producer", diagnostics)
    sequence = integer_value(value.get("session_seq"), f"{path}.session_seq", diagnostics, 1, UINT64_MAX)
    if producer is None or sequence is None:
        return None
    return (*producer, sequence)


def validate_session_ref(value: Any, path: str, diagnostics: Diagnostics) -> tuple[tuple[Any, ...], tuple[Any, ...]] | None:
    value = object_value(
        value,
        path,
        {"id", "subject_id", "pid", "season_id", "environment_id"},
        set(),
        diagnostics,
    )
    session_id = validate_session_id(value.get("id"), f"{path}.id", diagnostics)
    subject = integer_value(value.get("subject_id"), f"{path}.subject_id", diagnostics, 1, UINT64_MAX)
    pid = integer_value(value.get("pid"), f"{path}.pid", diagnostics, 1, 2**31 - 1)
    season = integer_value(value.get("season_id"), f"{path}.season_id", diagnostics, 1, UINT64_MAX)
    environment = integer_value(
        value.get("environment_id"), f"{path}.environment_id", diagnostics, 1, UINT64_MAX
    )
    if session_id is None or None in (subject, pid, season, environment):
        return None
    session_key = session_id
    scope = (session_id, subject, pid, season, environment)
    return session_key, scope


def validate_zero_session_ref(value: Any, path: str, diagnostics: Diagnostics) -> bool:
    value = object_value(
        value,
        path,
        {"id", "subject_id", "pid", "season_id", "environment_id"},
        set(),
        diagnostics,
    )
    expected_id = {"producer": {"boot_id": 0, "process_id": 0}, "session_seq": 0}
    if value != {
        "id": expected_id,
        "subject_id": 0,
        "pid": 0,
        "season_id": 0,
        "environment_id": 0,
    }:
        diagnostics.add(path, "process-wide zero session reference must be all zero")
        return False
    return True


def validate_connection(value: Any, path: str, diagnostics: Diagnostics) -> tuple[Any, ...] | None:
    value = object_value(value, path, {"producer", "connection_seq"}, set(), diagnostics)
    producer_value = value.get("producer")
    if not isinstance(producer_value, dict):
        diagnostics.add(f"{path}.producer", "expected an object")
        return None
    producer_fields = object_value(
        producer_value, f"{path}.producer", {"boot_id", "process_id"}, set(), diagnostics
    )
    boot = integer_value(producer_fields.get("boot_id"), f"{path}.producer.boot_id", diagnostics, 0, UINT64_MAX)
    process = integer_value(
        producer_fields.get("process_id"), f"{path}.producer.process_id", diagnostics, 0, UINT64_MAX
    )
    sequence = integer_value(value.get("connection_seq"), f"{path}.connection_seq", diagnostics, 0, UINT64_MAX)
    if None in (boot, process, sequence):
        return None
    if (boot, process, sequence) == (0, 0, 0):
        return (0, 0, 0)
    if boot == 0 or process == 0 or sequence == 0:
        diagnostics.add(path, "connection identity must be fully zero or fully nonzero")
        return None
    return boot, process, sequence


def connection_is_zero(value: dict[str, Any]) -> bool:
    producer = value["producer"]
    return producer["boot_id"] == 0 and producer["process_id"] == 0 and value["connection_seq"] == 0


def validate_time_window(value: Any, path: str, diagnostics: Diagnostics) -> dict[str, Any] | None:
    value = object_value(
        value,
        path,
        {"start_monotonic_usec", "end_monotonic_usec", "start_utc_usec", "end_utc_usec"},
        set(),
        diagnostics,
    )
    start = integer_value(
        value.get("start_monotonic_usec"), f"{path}.start_monotonic_usec", diagnostics, 0, UINT64_MAX
    )
    end = integer_value(
        value.get("end_monotonic_usec"), f"{path}.end_monotonic_usec", diagnostics, 0, UINT64_MAX
    )
    integer_value(value.get("start_utc_usec"), f"{path}.start_utc_usec", diagnostics, INT64_MIN, INT64_MAX)
    integer_value(value.get("end_utc_usec"), f"{path}.end_utc_usec", diagnostics, INT64_MIN, INT64_MAX)
    if start is not None and end is not None and end <= start:
        diagnostics.add(path, "interval monotonic end must be greater than start")
    return value


def validate_dimensions(value: Any, path: str, diagnostics: Diagnostics) -> dict[str, Any] | None:
    value = object_value(
        value,
        path,
        {"level_band", "class_id", "race_id", "faction_id", "zone_vnum", "group_size"},
        set(),
        diagnostics,
    )
    integer_value(value.get("level_band"), f"{path}.level_band", diagnostics, 0, 2**16 - 1)
    integer_value(value.get("class_id"), f"{path}.class_id", diagnostics, 0, 2**16 - 1)
    integer_value(value.get("race_id"), f"{path}.race_id", diagnostics, 0, 2**16 - 1)
    integer_value(value.get("faction_id"), f"{path}.faction_id", diagnostics, 0, 2**16 - 1)
    integer_value(value.get("zone_vnum"), f"{path}.zone_vnum", diagnostics, -(2**31), 2**31 - 1)
    integer_value(value.get("group_size"), f"{path}.group_size", diagnostics, 0, 2**32 - 1)
    return value


def validate_quality(value: Any, path: str, diagnostics: Diagnostics) -> int | None:
    value = integer_value(value, path, diagnostics, 0, UINT32_MAX)
    if value is not None and value & ~QUALITY_KNOWN:
        diagnostics.add(path, "contains unknown quality bits")
    return value


def canonical_config_fingerprint(config: dict[str, Any]) -> str:
    """Return the documented effective-config SHA-256 known answer.

    `effective_utc_usec`, config id, revision, fingerprint, and reserved
    padding are publication/transport metadata and intentionally do not enter
    this byte stream.
    """
    backend_code = {"sql": 1, "flatfile_disabled": 2}[config["backend"]]
    fields = (
        (config["schema_version"], 2),
        (config["build_version"], 4),
        (config["content_version"], 4),
        (config["property_version"], 4),
        (config["classifier_version"], 4),
        (config["policy_version"], 4),
        (config["season_id"], 8),
        (config["environment_id"], 8),
        (config["interval_usec"], 8),
        (config["checkpoint_interval_usec"], 8),
        (config["active_window_usec"], 8),
        (config["context_segments_per_minute"], 4),
        (config["pulse_slot_count"], 2),
        (backend_code, 1),
        (config["enabled"], 1),
    )
    encoded = b"".join(value.to_bytes(width, "big", signed=False) for value, width in fields)
    return hashlib.sha256(encoded).hexdigest()


def validate_config(value: Any, path: str, diagnostics: Diagnostics) -> int | None:
    value = object_value(
        value,
        path,
        {
            "schema_version",
            "reserved",
            "config_id",
            "revision",
            "build_version",
            "content_version",
            "property_version",
            "classifier_version",
            "policy_version",
            "season_id",
            "environment_id",
            "fingerprint",
            "effective_utc_usec",
            "interval_usec",
            "checkpoint_interval_usec",
            "active_window_usec",
            "context_segments_per_minute",
            "pulse_slot_count",
            "backend",
            "enabled",
        },
        set(),
        diagnostics,
    )
    schema = integer_value(value.get("schema_version"), f"{path}.schema_version", diagnostics, 0, UINT16_MAX)
    reserved = integer_value(value.get("reserved"), f"{path}.reserved", diagnostics, 0, UINT16_MAX)
    if schema is not None and schema != TELEMETRY_SCHEMA_VERSION:
        diagnostics.add(f"{path}.schema_version", "must equal telemetry schema version 1")
    if reserved is not None and reserved != 0:
        diagnostics.add(f"{path}.reserved", "must be zero")
    config_id = integer_value(value.get("config_id"), f"{path}.config_id", diagnostics, 1, UINT64_MAX)
    integer_value(value.get("revision"), f"{path}.revision", diagnostics, 1, UINT64_MAX)
    for field in ("build_version", "content_version", "property_version", "classifier_version", "policy_version"):
        integer_value(value.get(field), f"{path}.{field}", diagnostics, 1, UINT32_MAX)
    integer_value(value.get("season_id"), f"{path}.season_id", diagnostics, 1, UINT64_MAX)
    integer_value(value.get("environment_id"), f"{path}.environment_id", diagnostics, 1, UINT64_MAX)
    fingerprint = string_value(value.get("fingerprint"), f"{path}.fingerprint", diagnostics)
    if fingerprint is not None:
        if not CONFIG_FINGERPRINT_RE.fullmatch(fingerprint):
            diagnostics.add(path + ".fingerprint", "must be exactly 32 bytes of lowercase hex")
        elif set(fingerprint) == {"0"}:
            diagnostics.add(path + ".fingerprint", "must not be all zero")
    integer_value(value.get("effective_utc_usec"), f"{path}.effective_utc_usec", diagnostics, INT64_MIN, INT64_MAX)
    interval_usec = integer_value(value.get("interval_usec"), f"{path}.interval_usec", diagnostics, 0, UINT64_MAX)
    checkpoint_interval_usec = integer_value(
        value.get("checkpoint_interval_usec"), f"{path}.checkpoint_interval_usec", diagnostics, 0, UINT64_MAX
    )
    active_window_usec = integer_value(
        value.get("active_window_usec"), f"{path}.active_window_usec", diagnostics, 0, UINT64_MAX
    )
    context_segments = integer_value(
        value.get("context_segments_per_minute"),
        f"{path}.context_segments_per_minute",
        diagnostics,
        0,
        UINT32_MAX,
    )
    pulse_slots = integer_value(value.get("pulse_slot_count"), f"{path}.pulse_slot_count", diagnostics, 0, UINT16_MAX)
    backend = enum_value(value.get("backend"), f"{path}.backend", BACKENDS, diagnostics)
    enabled = integer_value(value.get("enabled"), f"{path}.enabled", diagnostics, 0, 1)
    if backend == "flatfile_disabled" and enabled == 1:
        diagnostics.add(path, "flatfile_disabled cannot be enabled")
    if backend == "sql" and enabled == 1:
        if interval_usec == 0:
            diagnostics.add(path + ".interval_usec", "enabled SQL config must have a nonzero interval")
        if checkpoint_interval_usec == 0:
            diagnostics.add(path + ".checkpoint_interval_usec", "enabled SQL config must have a nonzero checkpoint interval")
        if active_window_usec == 0:
            diagnostics.add(path + ".active_window_usec", "enabled SQL config must have a nonzero active window")
        if context_segments is not None and not 1 <= context_segments <= 64:
            diagnostics.add(path + ".context_segments_per_minute", "enabled value must be in 1..64")
        if pulse_slots is not None and not 1 <= pulse_slots <= 256:
            diagnostics.add(path + ".pulse_slot_count", "enabled value must be in 1..256")
    if interval_usec is not None and interval_usec > 3_600_000_000:
        diagnostics.add(path + ".interval_usec", "exceeds the one-hour proposal bound")
    if checkpoint_interval_usec is not None and checkpoint_interval_usec > 3_600_000_000:
        diagnostics.add(path + ".checkpoint_interval_usec", "exceeds the one-hour proposal bound")
    if active_window_usec is not None and active_window_usec > 3_600_000_000:
        diagnostics.add(path + ".active_window_usec", "exceeds the one-hour proposal bound")
    identity_fields = (
        "schema_version",
        "build_version",
        "content_version",
        "property_version",
        "classifier_version",
        "policy_version",
        "season_id",
        "environment_id",
        "interval_usec",
        "checkpoint_interval_usec",
        "active_window_usec",
        "context_segments_per_minute",
        "pulse_slot_count",
        "enabled",
    )
    identity_values = [value.get(field) for field in identity_fields]
    if (
        fingerprint is not None
        and CONFIG_FINGERPRINT_RE.fullmatch(fingerprint)
        and backend in BACKENDS
        and all(isinstance(item, int) and not isinstance(item, bool) and item >= 0 for item in identity_values)
    ):
        try:
            expected_fingerprint = canonical_config_fingerprint(value)
        except (KeyError, OverflowError):
            expected_fingerprint = None
        if expected_fingerprint is not None and fingerprint != expected_fingerprint:
            diagnostics.add(path + ".fingerprint", "does not match the documented canonical SHA-256")
    return config_id


def validate_cumulative(value: Any, path: str, diagnostics: Diagnostics) -> dict[str, Any] | None:
    fields = {"connected_usec", "active_usec", "idle_usec", "unknown_usec", "resident_usec", "linkdead_usec"}
    value = object_value(value, path, fields, set(), diagnostics)
    for field in fields:
        integer_value(value.get(field), f"{path}.{field}", diagnostics, 0, UINT64_MAX)
    connected = value.get("connected_usec")
    active = value.get("active_usec")
    idle = value.get("idle_usec")
    unknown = value.get("unknown_usec")
    resident = value.get("resident_usec")
    linkdead = value.get("linkdead_usec")
    if all(isinstance(v, int) and not isinstance(v, bool) for v in (connected, active, idle, unknown)):
        if connected != active + idle + unknown:
            diagnostics.add(path, "connected != active + idle + unknown")
    if all(isinstance(v, int) and not isinstance(v, bool) for v in (resident, connected, linkdead)):
        if resident != connected + linkdead:
            diagnostics.add(path, "resident != connected + linkdead")
    return value


def validate_interval_payload(value: Any, path: str, diagnostics: Diagnostics) -> dict[str, Any] | None:
    value = object_value(
        value,
        path,
        {
            "session",
            "connection",
            "window",
            "duration_usec",
            "category",
            "context",
            "context_quality",
            "reserved",
            "dimensions",
            "config_id",
            "classifier_version",
            "policy_version",
            "quality_flags",
        },
        set(),
        diagnostics,
    )
    session = validate_session_ref(value.get("session"), f"{path}.session", diagnostics)
    connection = validate_connection(value.get("connection"), f"{path}.connection", diagnostics)
    window = validate_time_window(value.get("window"), f"{path}.window", diagnostics)
    duration = integer_value(value.get("duration_usec"), f"{path}.duration_usec", diagnostics, 1, UINT64_MAX)
    category = enum_value(value.get("category"), f"{path}.category", INTERVAL_CATEGORIES, diagnostics)
    context = enum_value(value.get("context"), f"{path}.context", ACTIVITY_CONTEXTS, diagnostics)
    context_quality = enum_value(
        value.get("context_quality"), f"{path}.context_quality", CONTEXT_QUALITIES, diagnostics
    )
    reserved = integer_value(value.get("reserved"), f"{path}.reserved", diagnostics, 0, 255)
    dimensions = validate_dimensions(value.get("dimensions"), f"{path}.dimensions", diagnostics)
    config_id = integer_value(value.get("config_id"), f"{path}.config_id", diagnostics, 1, UINT64_MAX)
    integer_value(value.get("classifier_version"), f"{path}.classifier_version", diagnostics, 1, UINT32_MAX)
    integer_value(value.get("policy_version"), f"{path}.policy_version", diagnostics, 1, UINT32_MAX)
    quality = validate_quality(value.get("quality_flags"), f"{path}.quality_flags", diagnostics)
    if connection is not None and isinstance(value.get("connection"), dict):
        zero = connection == (0, 0, 0)
        if category == "resident_linkdead" and not zero:
            diagnostics.add(path + ".connection", "linkdead interval must use an all-zero connection")
        if category != "resident_linkdead" and zero:
            diagnostics.add(path + ".connection", "connected interval requires a nonzero connection")
    if window is not None and duration is not None:
        start = window.get("start_monotonic_usec")
        end = window.get("end_monotonic_usec")
        if isinstance(start, int) and isinstance(end, int) and end - start != duration:
            diagnostics.add(path, "duration_usec must equal monotonic end-start")
    if context == "overflow_unknown":
        if context_quality != "overflow":
            diagnostics.add(path, "overflow_unknown context requires overflow quality")
        if not isinstance(quality, int) or not quality & (1 << 1) or not quality & (1 << 2):
            diagnostics.add(path, "overflow_unknown context requires overflow and dimension-unknown flags")
        if dimensions != {
            "level_band": 0,
            "class_id": 0,
            "race_id": 0,
            "faction_id": 0,
            "zone_vnum": -1,
            "group_size": 0,
        }:
            diagnostics.add(path + ".dimensions", "overflow context must clear attribution dimensions")
    return value


def validate_lifecycle_payload(value: Any, path: str, diagnostics: Diagnostics) -> dict[str, Any] | None:
    value = object_value(
        value,
        path,
        {
            "session",
            "connection",
            "lifecycle",
            "end_reason",
            "reserved",
            "at_monotonic_usec",
            "at_utc_usec",
            "dimensions",
            "config_id",
            "classifier_version",
            "policy_version",
            "quality_flags",
        },
        set(),
        diagnostics,
    )
    session = validate_session_ref(value.get("session"), f"{path}.session", diagnostics)
    connection = validate_connection(value.get("connection"), f"{path}.connection", diagnostics)
    lifecycle = enum_value(value.get("lifecycle"), f"{path}.lifecycle", LIFECYCLE_KINDS, diagnostics)
    reason = enum_value(value.get("end_reason"), f"{path}.end_reason", END_REASONS, diagnostics)
    integer_value(value.get("reserved"), f"{path}.reserved", diagnostics, 0, UINT16_MAX)
    integer_value(value.get("at_monotonic_usec"), f"{path}.at_monotonic_usec", diagnostics, 0, UINT64_MAX)
    integer_value(value.get("at_utc_usec"), f"{path}.at_utc_usec", diagnostics, INT64_MIN, INT64_MAX)
    validate_dimensions(value.get("dimensions"), f"{path}.dimensions", diagnostics)
    integer_value(value.get("config_id"), f"{path}.config_id", diagnostics, 1, UINT64_MAX)
    integer_value(value.get("classifier_version"), f"{path}.classifier_version", diagnostics, 1, UINT32_MAX)
    integer_value(value.get("policy_version"), f"{path}.policy_version", diagnostics, 1, UINT32_MAX)
    validate_quality(value.get("quality_flags"), f"{path}.quality_flags", diagnostics)
    if lifecycle in {"session_entered", "connection_attached", "connection_detached"}:
        if connection == (0, 0, 0):
            diagnostics.add(path + ".connection", "socket transition requires a nonzero connection")
        if reason != "unknown":
            diagnostics.add(path + ".end_reason", "non-exit lifecycle must use unknown end reason")
    if lifecycle == "session_exited" and reason == "unknown":
        diagnostics.add(path + ".end_reason", "session exit requires a concrete end reason")
    return value


def validate_checkpoint_payload(value: Any, path: str, diagnostics: Diagnostics) -> dict[str, Any] | None:
    value = object_value(
        value,
        path,
        {
            "session",
            "connection",
            "revision",
            "at_monotonic_usec",
            "at_utc_usec",
            "cumulative",
            "config_id",
            "quality_flags",
        },
        set(),
        diagnostics,
    )
    validate_session_ref(value.get("session"), f"{path}.session", diagnostics)
    validate_connection(value.get("connection"), f"{path}.connection", diagnostics)
    integer_value(value.get("revision"), f"{path}.revision", diagnostics, 1, UINT64_MAX)
    integer_value(value.get("at_monotonic_usec"), f"{path}.at_monotonic_usec", diagnostics, 0, UINT64_MAX)
    integer_value(value.get("at_utc_usec"), f"{path}.at_utc_usec", diagnostics, INT64_MIN, INT64_MAX)
    validate_cumulative(value.get("cumulative"), f"{path}.cumulative", diagnostics)
    integer_value(value.get("config_id"), f"{path}.config_id", diagnostics, 1, UINT64_MAX)
    validate_quality(value.get("quality_flags"), f"{path}.quality_flags", diagnostics)
    return value


def validate_gap_payload(value: Any, path: str, diagnostics: Diagnostics) -> dict[str, Any] | None:
    value = object_value(
        value,
        path,
        {
            "session",
            "connection",
            "reason",
            "reserved",
            "start_monotonic_usec",
            "end_monotonic_usec",
            "start_utc_usec",
            "end_utc_usec",
            "first_missing_record_seq",
            "last_missing_record_seq",
            "duration_usec",
            "dropped_records",
            "quality_flags",
        },
        set(),
        diagnostics,
    )
    session_value = value.get("session")
    if isinstance(session_value, dict):
        if session_value.get("id") == {"producer": {"boot_id": 0, "process_id": 0}, "session_seq": 0}:
            validate_zero_session_ref(session_value, f"{path}.session", diagnostics)
        else:
            validate_session_ref(session_value, f"{path}.session", diagnostics)
    else:
        diagnostics.add(path + ".session", "expected a session reference object")
    connection = validate_connection(value.get("connection"), f"{path}.connection", diagnostics)
    reason = enum_value(value.get("reason"), f"{path}.reason", GAP_REASONS, diagnostics)
    reserved = list_value(value.get("reserved"), f"{path}.reserved", diagnostics)
    if len(reserved) != 3:
        diagnostics.add(path + ".reserved", "must contain exactly three bytes")
    for index, byte in enumerate(reserved):
        integer_value(byte, f"{path}.reserved[{index}]", diagnostics, 0, 255)
        if byte != 0:
            diagnostics.add(f"{path}.reserved[{index}]", "must be zero")
    start = integer_value(value.get("start_monotonic_usec"), f"{path}.start_monotonic_usec", diagnostics, 0, UINT64_MAX)
    end = integer_value(value.get("end_monotonic_usec"), f"{path}.end_monotonic_usec", diagnostics, 0, UINT64_MAX)
    integer_value(value.get("start_utc_usec"), f"{path}.start_utc_usec", diagnostics, INT64_MIN, INT64_MAX)
    integer_value(value.get("end_utc_usec"), f"{path}.end_utc_usec", diagnostics, INT64_MIN, INT64_MAX)
    integer_value(value.get("first_missing_record_seq"), f"{path}.first_missing_record_seq", diagnostics, 0, UINT64_MAX)
    integer_value(value.get("last_missing_record_seq"), f"{path}.last_missing_record_seq", diagnostics, 0, UINT64_MAX)
    duration = integer_value(value.get("duration_usec"), f"{path}.duration_usec", diagnostics, 0, UINT64_MAX)
    integer_value(value.get("dropped_records"), f"{path}.dropped_records", diagnostics, 0, UINT64_MAX)
    quality = validate_quality(value.get("quality_flags"), f"{path}.quality_flags", diagnostics)
    if start is not None and end is not None and end < start:
        diagnostics.add(path, "gap monotonic end cannot precede start")
    if start is not None and end is not None and duration is not None:
        if duration and end - start != duration:
            diagnostics.add(path, "known gap duration must equal monotonic end-start")
        if not duration and end > start:
            diagnostics.add(path, "zero-duration gap cannot carry a positive monotonic span")
    first = value.get("first_missing_record_seq")
    last = value.get("last_missing_record_seq")
    dropped = value.get("dropped_records")
    if isinstance(first, int) and isinstance(last, int):
        if (first == 0) != (last == 0):
            diagnostics.add(path, "missing sequence bounds must be both known or both zero")
        if first and last < first:
            diagnostics.add(path, "last missing sequence must not precede first")
        if first and isinstance(dropped, int) and dropped != last - first + 1:
            diagnostics.add(path, "fixture uses contiguous bounds, so dropped_records must equal the range length")
    required_quality = {
        "detail_queue_drop": 1 << 4,
        "control_queue_drop": 1 << 4,
        "sequence_gap": 1 << 3,
        "unclosed_tail": 1 << 6,
        "clock_discontinuity": 1 << 7,
    }.get(reason)
    if required_quality is not None and isinstance(quality, int) and not quality & required_quality:
        diagnostics.add(path + ".quality_flags", f"{reason} requires its quality flag")
    if connection == (0, 0, 0) and isinstance(session_value, dict) and session_value.get("pid", 0) == 0:
        # The process-wide form is intentionally permitted; no further scope is inferred.
        pass
    return value


def validate_record(record: Any, path: str, diagnostics: Diagnostics) -> dict[str, Any] | None:
    record = object_value(record, path, {"header", "payload"}, set(), diagnostics)
    header = object_value(
        record.get("header"),
        path + ".header",
        {"schema_version", "kind", "reserved", "key", "occurrence_utc_usec"},
        set(),
        diagnostics,
    )
    schema = integer_value(header.get("schema_version"), path + ".header.schema_version", diagnostics, 0, UINT16_MAX)
    if schema is not None and schema != TELEMETRY_SCHEMA_VERSION:
        diagnostics.add(path + ".header.schema_version", "must equal telemetry schema version 1")
    reserved = integer_value(header.get("reserved"), path + ".header.reserved", diagnostics, 0, 255)
    if reserved is not None and reserved != 0:
        diagnostics.add(path + ".header.reserved", "must be zero")
    kind = enum_value(header.get("kind"), path + ".header.kind", RECORD_KINDS, diagnostics)
    key = validate_record_key(header.get("key"), path + ".header.key", diagnostics)
    integer_value(
        header.get("occurrence_utc_usec"),
        path + ".header.occurrence_utc_usec",
        diagnostics,
        INT64_MIN,
        INT64_MAX,
    )
    payload = object_value(record.get("payload"), path + ".payload", set(), RECORD_KINDS, diagnostics)
    if kind is None:
        return record
    if set(payload) != {kind}:
        diagnostics.add(path + ".payload", f"tagged payload must contain only {kind!r}")
    payload_value = payload.get(kind)
    if kind == "interval":
        validate_interval_payload(payload_value, path + ".payload.interval", diagnostics)
    elif kind == "session_lifecycle":
        validate_lifecycle_payload(payload_value, path + ".payload.session_lifecycle", diagnostics)
    elif kind == "session_checkpoint":
        validate_checkpoint_payload(payload_value, path + ".payload.session_checkpoint", diagnostics)
    elif kind == "coverage_gap":
        validate_gap_payload(payload_value, path + ".payload.coverage_gap", diagnostics)
    elif kind == "configuration":
        payload_value = object_value(
            payload_value, path + ".payload.configuration", {"config"}, set(), diagnostics
        )
        validate_config(payload_value.get("config"), path + ".payload.configuration.config", diagnostics)
    return record


def validate_transition(value: Any, path: str, diagnostics: Diagnostics) -> dict[str, Any] | None:
    value = object_value(
        value,
        path,
        {"session", "connection", "at_monotonic_usec", "at_utc_usec", "kind", "reserved", "quality_flags"},
        set(),
        diagnostics,
    )
    session = validate_session_ref(value.get("session"), path + ".session", diagnostics)
    connection = validate_connection(value.get("connection"), path + ".connection", diagnostics)
    integer_value(value.get("at_monotonic_usec"), path + ".at_monotonic_usec", diagnostics, 0, UINT64_MAX)
    integer_value(value.get("at_utc_usec"), path + ".at_utc_usec", diagnostics, INT64_MIN, INT64_MAX)
    kind = enum_value(value.get("kind"), path + ".kind", TRANSITION_KINDS, diagnostics)
    reserved = list_value(value.get("reserved"), path + ".reserved", diagnostics)
    if len(reserved) != 3:
        diagnostics.add(path + ".reserved", "must contain exactly three bytes")
    for index, byte in enumerate(reserved):
        integer_value(byte, f"{path}.reserved[{index}]", diagnostics, 0, 255)
        if byte != 0:
            diagnostics.add(f"{path}.reserved[{index}]", "must be zero")
    validate_quality(value.get("quality_flags"), path + ".quality_flags", diagnostics)
    if kind == "copyover_resumed" and session is not None and connection is not None:
        if session[0][0:2] == connection[0:2]:
            diagnostics.add(path, "copyover_resumed must use a new producer")
    return value


def session_key_from_payload(payload: dict[str, Any]) -> tuple[Any, ...] | None:
    session = payload.get("session")
    if not isinstance(session, dict) or not isinstance(session.get("id"), dict):
        return None
    sid = session["id"]
    producer = sid.get("producer", {})
    if not isinstance(producer, dict):
        return None
    return producer.get("boot_id"), producer.get("process_id"), sid.get("session_seq")


def scope_from_payload(payload: dict[str, Any]) -> tuple[Any, ...] | None:
    session = payload.get("session")
    if not isinstance(session, dict):
        return None
    sid = session.get("id", {})
    producer = sid.get("producer", {}) if isinstance(sid, dict) else {}
    if not isinstance(producer, dict):
        return None
    return (
        producer.get("boot_id"),
        producer.get("process_id"),
        sid.get("session_seq"),
        session.get("subject_id"),
        session.get("pid"),
        session.get("season_id"),
        session.get("environment_id"),
    )


def cumulative_non_decreasing(new: dict[str, Any], old: dict[str, Any]) -> bool:
    return all(new[field] >= old[field] for field in old)


def batch_outcome_for(record_outcomes: list[str]) -> str:
    if any(outcome in {"rejected_invalid", "duplicate_conflict", "retryable_failure", "unavailable", "disabled"} for outcome in record_outcomes):
        return "committed_with_rejections"
    return "committed"


class OracleState:
    def __init__(self, initial_configs: dict[int, dict[str, Any]]) -> None:
        self.configs: dict[int, dict[str, Any]] = deepcopy(initial_configs)
        self.record_store: dict[tuple[Any, ...], dict[str, Any]] = {}
        self.session_scopes: dict[tuple[Any, ...], tuple[Any, ...]] = {}
        self.checkpoints: dict[tuple[Any, ...], dict[str, Any]] = {}
        self.operation_results: list[dict[str, Any]] = []
        self.projection_history: list[dict[str, int]] = []
        self.admission_checks: list[bool] = []

    def apply_record(self, record: dict[str, Any]) -> str:
        header = record["header"]
        key_value = header["key"]
        producer = key_value["producer"]
        key = producer_token(producer) + (key_value["record_seq"],)
        record_content = canonical(record)
        stored = self.record_store.get(key)
        if stored is not None:
            return "duplicate_identical" if canonical(stored) == record_content else "duplicate_conflict"

        kind = header["kind"]
        payload = record["payload"][kind]
        if kind == "configuration":
            config = payload["config"]
            config_id = config["config_id"]
            current = self.configs.get(config_id)
            if current is not None and canonical(current) != canonical(config):
                return "duplicate_conflict"
            self.configs[config_id] = deepcopy(config)
            self.record_store[key] = deepcopy(record)
            return "applied"

        config_id = payload.get("config_id")
        admitted = config_id in self.configs if kind != "coverage_gap" else True
        if kind != "coverage_gap":
            self.admission_checks.append(admitted)
        if not admitted:
            return "rejected_invalid"

        scope_key = session_key_from_payload(payload)
        scope = scope_from_payload(payload)
        if scope_key is not None and scope is not None:
            previous_scope = self.session_scopes.get(scope_key)
            if previous_scope is not None and previous_scope != scope:
                return "rejected_invalid"
            self.session_scopes[scope_key] = scope

        if kind == "session_checkpoint":
            revision = payload["revision"]
            existing = self.checkpoints.get(scope_key)
            if existing is not None:
                old_revision = existing["revision"]
                if revision < old_revision:
                    self.record_store[key] = deepcopy(record)
                    return "checkpoint_older"
                if revision == old_revision:
                    if canonical(payload["cumulative"]) == canonical(existing["cumulative"]):
                        self.record_store[key] = deepcopy(record)
                        return "checkpoint_older"
                    return "duplicate_conflict"
                if not cumulative_non_decreasing(payload["cumulative"], existing["cumulative"]):
                    return "duplicate_conflict"
            self.checkpoints[scope_key] = deepcopy(payload)
            self.record_store[key] = deepcopy(record)
            return "applied"

        self.record_store[key] = deepcopy(record)
        return "applied"

    def projection_snapshot(self) -> dict[str, int]:
        return {canonical(key): payload["revision"] for key, payload in self.checkpoints.items()}


def compare_exact(actual: Any, expected: Any, path: str, diagnostics: Diagnostics) -> None:
    if actual != expected:
        diagnostics.add(path, f"expected {expected!r}, computed {actual!r}")


def validate_fixture(data: Any, source: Path | str = "fixture") -> list[str]:
    diagnostics = Diagnostics()
    source_name = str(source)
    top = object_value(
        data,
        "$",
        {"fixture_schema", "schema_version", "fixture_id", "covers", "records", "operations", "expected"},
        {"configurations", "transitions"},
        diagnostics,
    )
    if top.get("fixture_schema") != FIXTURE_SCHEMA:
        diagnostics.add("$.fixture_schema", f"must equal {FIXTURE_SCHEMA!r}")
    if top.get("schema_version") != FIXTURE_SCHEMA_VERSION:
        diagnostics.add("$.schema_version", "must equal fixture schema version 1")
    fixture_id = top.get("fixture_id")
    if not isinstance(fixture_id, str) or not fixture_id:
        diagnostics.add("$.fixture_id", "must be a nonempty string")
        fixture_id = ""
    covers = list_value(top.get("covers"), "$.covers", diagnostics)
    if any(not isinstance(value, str) for value in covers):
        diagnostics.add("$.covers", "all coverage labels must be strings")
    expected_covers = EXPECTED_COVERAGE.get(fixture_id)
    if expected_covers is None:
        diagnostics.add("$.fixture_id", f"unknown fixture id {fixture_id!r}")
    elif set(covers) != expected_covers or len(covers) != len(expected_covers):
        diagnostics.add("$.covers", f"must contain exactly {sorted(expected_covers)!r}")

    initial_configs: dict[int, dict[str, Any]] = {}
    configurations = list_value(top.get("configurations", []), "$.configurations", diagnostics)
    for index, config in enumerate(configurations):
        config_id = validate_config(config, f"$.configurations[{index}]", diagnostics)
        if config_id is not None:
            if config_id in initial_configs:
                diagnostics.add(f"$.configurations[{index}].config_id", "duplicate admitted config id")
            initial_configs[config_id] = deepcopy(config)

    transitions = list_value(top.get("transitions", []), "$.transitions", diagnostics)
    for index, transition in enumerate(transitions):
        validate_transition(transition, f"$.transitions[{index}]", diagnostics)

    records = list_value(top.get("records"), "$.records", diagnostics)
    validated_records: list[dict[str, Any]] = []
    for index, record in enumerate(records):
        validated = validate_record(record, f"$.records[{index}]", diagnostics)
        if validated is not None:
            validated_records.append(validated)

    operations = list_value(top.get("operations"), "$.operations", diagnostics)
    operation_names: set[str] = set()
    parsed_operations: list[dict[str, Any]] = []
    for index, operation in enumerate(operations):
        path = f"$.operations[{index}]"
        operation = object_value(
            operation,
            path,
            {"name", "record_indexes", "acknowledgement"},
            {"retry_of", "server_effect"},
            diagnostics,
        )
        name = string_value(operation.get("name"), path + ".name", diagnostics)
        if name is not None:
            if not name or name in operation_names:
                diagnostics.add(path + ".name", "operation names must be nonempty and unique")
            operation_names.add(name)
        indexes = list_value(operation.get("record_indexes"), path + ".record_indexes", diagnostics)
        parsed_indexes: list[int] = []
        for item_index, item in enumerate(indexes):
            record_index = integer_value(item, f"{path}.record_indexes[{item_index}]", diagnostics, 0)
            if record_index is not None:
                if record_index >= len(validated_records):
                    diagnostics.add(f"{path}.record_indexes[{item_index}]", "record index is out of range")
                else:
                    parsed_indexes.append(record_index)
        if not parsed_indexes:
            diagnostics.add(path + ".record_indexes", "operation batch must not be empty")
        acknowledgement = enum_value(
            operation.get("acknowledgement"), path + ".acknowledgement", ACKNOWLEDGEMENTS, diagnostics
        )
        retry_of = operation.get("retry_of")
        if retry_of is not None:
            retry_of = string_value(retry_of, path + ".retry_of", diagnostics)
        server_effect = operation.get("server_effect")
        if acknowledgement == "ambiguous":
            server_effect = enum_value(server_effect, path + ".server_effect", SERVER_EFFECTS, diagnostics)
        elif "server_effect" in operation:
            diagnostics.add(path + ".server_effect", "only ambiguous operations declare a server effect")
        parsed_operations.append(
            {
                "name": name,
                "record_indexes": parsed_indexes,
                "acknowledgement": acknowledgement,
                "retry_of": retry_of,
                "server_effect": server_effect,
            }
        )

    expected = object_value(top.get("expected"), "$.expected", {"operations", "metrics"}, set(), diagnostics)
    expected_operations = list_value(expected.get("operations"), "$.expected.operations", diagnostics)
    expected_by_name: dict[str, dict[str, Any]] = {}
    for index, operation in enumerate(expected_operations):
        path = f"$.expected.operations[{index}]"
        operation = object_value(
            operation,
            path,
            {"name", "batch_outcome", "record_outcomes", "counts"},
            set(),
            diagnostics,
        )
        name = string_value(operation.get("name"), path + ".name", diagnostics)
        batch = enum_value(operation.get("batch_outcome"), path + ".batch_outcome", BATCH_OUTCOMES, diagnostics)
        outcomes = list_value(operation.get("record_outcomes"), path + ".record_outcomes", diagnostics)
        for outcome_index, outcome in enumerate(outcomes):
            enum_value(
                outcome,
                f"{path}.record_outcomes[{outcome_index}]",
                RECORD_OUTCOMES,
                diagnostics,
            )
        counts = object_value(operation.get("counts"), path + ".counts", set(), RECORD_OUTCOMES, diagnostics)
        for outcome, count in counts.items():
            integer_value(count, f"{path}.counts.{outcome}", diagnostics, 1, UINT64_MAX)
        if name is not None:
            if name in expected_by_name:
                diagnostics.add(path + ".name", "duplicate expected operation name")
            expected_by_name[name] = operation
        if batch == "commit_ambiguous" and outcomes and any(outcome != "commit_ambiguous" for outcome in outcomes):
            diagnostics.add(path, "ambiguous batch record outcomes must be commit_ambiguous")

    metrics = object_value(expected.get("metrics"), "$.expected.metrics", set(), METRIC_NAMES, diagnostics)
    expected_metric_names = EXPECTED_METRICS.get(fixture_id)
    if expected_metric_names is not None and set(metrics) != expected_metric_names:
        diagnostics.add(
            "$.expected.metrics",
            f"must contain exactly {sorted(expected_metric_names)!r}",
        )
    if diagnostics.items:
        # Structural/type errors are already decisive.  Do not execute the
        # semantic oracle against malformed input and risk hiding the finding.
        return [f"{source_name}: {item}" for item in diagnostics.items]

    state = OracleState(initial_configs)
    pending: dict[str, Any] | None = None
    computed_expected_operations: list[dict[str, Any]] = []
    operation_by_name: dict[str, dict[str, Any]] = {}
    observed_keys: set[str] = set()
    producer_frontier: dict[str, int] = {}

    for index, operation in enumerate(parsed_operations):
        path = f"$.operations[{index}]"
        name = operation["name"]
        if name is None:
            name = f"<operation-{index}>"
        selected = [validated_records[record_index] for record_index in operation["record_indexes"]]
        selected_canonical = [canonical(record) for record in selected]
        # First observations follow the single-writer stream. Replays may revisit
        # an old key (including deliberate conflicting-payload repository probes).
        for record in selected:
            key = record["header"]["key"]
            token = canonical(key)
            producer = producer_token(key["producer"])
            sequence = key["record_seq"]
            if token not in observed_keys:
                if sequence <= producer_frontier.get(producer, 0):
                    diagnostics.add(path, "first-observed record violates producer sequence order")
                producer_frontier[producer] = max(sequence, producer_frontier.get(producer, 0))
                observed_keys.add(token)
        retry_of = operation["retry_of"]
        if retry_of is not None:
            previous = operation_by_name.get(retry_of)
            if previous is None:
                diagnostics.add(path + ".retry_of", "must name a preceding operation")
            elif previous["acknowledgement"] != "ambiguous":
                diagnostics.add(path + ".retry_of", "must name an ambiguous operation")
            elif pending is None or pending["name"] != retry_of:
                diagnostics.add(path + ".retry_of", "does not match the one unresolved batch")
            elif selected_canonical != pending["records"]:
                diagnostics.add(path, "immutable retry changed the replay key or typed payload")
        elif pending is not None:
            diagnostics.add(path, "one unresolved ambiguous batch must be retried before later work")

        if operation["acknowledgement"] == "ambiguous":
            if pending is not None:
                diagnostics.add(path, "only one ambiguous batch may remain unresolved")
            normal_outcomes: list[str] = []
            if operation["server_effect"] == "committed":
                for record in selected:
                    normal_outcomes.append(state.apply_record(record))
                if any(outcome not in {"applied"} for outcome in normal_outcomes):
                    diagnostics.add(path, "fixture ambiguity model requires a wholly applicable batch")
            pending = {"name": name, "records": selected_canonical}
            record_outcomes = ["commit_ambiguous"] * len(selected)
            batch_outcome = "commit_ambiguous"
        else:
            record_outcomes = [state.apply_record(record) for record in selected]
            batch_outcome = batch_outcome_for(record_outcomes)
            if retry_of is not None and pending is not None and pending["name"] == retry_of:
                pending = None

        counts = dict(sorted(Counter(record_outcomes).items()))
        computed_expected_operations.append(
            {
                "name": name,
                "batch_outcome": batch_outcome,
                "record_outcomes": record_outcomes,
                "counts": counts,
            }
        )
        operation_by_name[name] = operation
        state.operation_results.append(computed_expected_operations[-1])
        state.projection_history.append(state.projection_snapshot())

    if pending is not None:
        diagnostics.add("$.operations", "ambiguous batch has no immutable retry")

    compare_exact(
        computed_expected_operations,
        expected_operations,
        "$.expected.operations",
        diagnostics,
    )

    durable_records = list(state.record_store.values())
    for record in durable_records:
        kind = record["header"]["kind"]
        payload = record["payload"][kind]
        connection = payload.get("connection")
        if isinstance(connection, dict) and not connection_is_zero(connection):
            header_producer = producer_token(record["header"]["key"]["producer"])
            connection_producer = producer_token(connection["producer"])
            if header_producer != connection_producer:
                diagnostics.add(
                    "$.records",
                    "a connected payload must use the current producer in its replay key",
                )
    intervals = [record["payload"]["interval"] for record in durable_records if record["header"]["kind"] == "interval"]
    gaps = [record["payload"]["coverage_gap"] for record in durable_records if record["header"]["kind"] == "coverage_gap"]
    checkpoints = [record["payload"]["session_checkpoint"] for record in durable_records if record["header"]["kind"] == "session_checkpoint"]
    lifecycles = [record["payload"]["session_lifecycle"] for record in durable_records if record["header"]["kind"] == "session_lifecycle"]

    active = sum(item["duration_usec"] for item in intervals if item["category"] == "connected_active")
    idle = sum(item["duration_usec"] for item in intervals if item["category"] == "connected_idle")
    unknown = sum(item["duration_usec"] for item in intervals if item["category"] == "unknown")
    linkdead = sum(item["duration_usec"] for item in intervals if item["category"] == "resident_linkdead")
    connected = active + idle + unknown
    resident = connected + linkdead
    conservation = {
        "interval_total_usec": sum(item["duration_usec"] for item in intervals),
        "connected_usec": connected,
        "active_usec": active,
        "idle_usec": idle,
        "unknown_usec": unknown,
        "resident_usec": resident,
        "linkdead_usec": linkdead,
        "connected_identity_holds": connected == active + idle + unknown,
        "resident_identity_holds": resident == connected + linkdead,
    }
    compare_exact(conservation, metrics.get("conservation"), "$.expected.metrics.conservation", diagnostics)

    session_keys = set()
    connection_keys = set()
    producers = set()
    for record in durable_records:
        producer = record["header"]["key"]["producer"]
        producers.add(producer_token(producer))
        kind = record["header"]["kind"]
        payload = record["payload"][kind]
        if "session" in payload and payload["session"]["pid"] != 0:
            session_keys.add(canonical(payload["session"]["id"]))
        connection = payload.get("connection")
        if isinstance(connection, dict) and not connection_is_zero(connection):
            connection_keys.add(canonical(connection))
        if kind == "interval":
            connection_keys.add(canonical(payload["connection"])) if not connection_is_zero(payload["connection"]) else None
    transition_values = transitions
    for transition_index, transition in enumerate(transition_values):
        expected_lifecycle = "connection_detached" if transition["kind"] == "detached" else "connection_attached"
        matches = [
            lifecycle
            for lifecycle in lifecycles
            if lifecycle["lifecycle"] == expected_lifecycle
            and canonical(lifecycle["session"]) == canonical(transition["session"])
            and canonical(lifecycle["connection"]) == canonical(transition["connection"])
            and lifecycle["at_monotonic_usec"] == transition["at_monotonic_usec"]
            and lifecycle["at_utc_usec"] == transition["at_utc_usec"]
        ]
        if not matches:
            diagnostics.add(f"$.transitions[{transition_index}]", "has no matching admitted lifecycle record")

    metrics_actual: dict[str, Any] = {}
    if fixture_id == "replay_identical_conflict":
        results = {item["name"]: item for item in state.operation_results}
        key_tokens = []
        for record in validated_records[:3]:
            key = record["header"]["key"]
            key_tokens.append(canonical(key))
        metrics_actual["replay"] = {
            "same_key": len(set(key_tokens)) == 1,
            "identical_outcome": results[parsed_operations[1]["name"]]["record_outcomes"][0],
            "conflict_outcome": results[parsed_operations[2]["name"]]["record_outcomes"][0],
            "stored_interval_usec": conservation["interval_total_usec"],
        }
    elif fixture_id == "commit_ambiguity_retry":
        first, retry = state.operation_results[0], state.operation_results[1]
        metrics_actual["retry"] = {
            "same_replay_key": canonical(validated_records[0]["header"]["key"])
            == canonical(validated_records[1]["header"]["key"]),
            "same_typed_payload": canonical(validated_records[0]) == canonical(validated_records[1]),
            "ambiguous_batch_outcome": first["batch_outcome"],
            "retry_record_outcome": retry["record_outcomes"][0],
            "durable_interval_usec": conservation["interval_total_usec"],
        }
    elif fixture_id == "checkpoint_revision_conflict":
        latest = max(state.checkpoints.values(), key=lambda item: item["revision"])
        metrics_actual["checkpoint"] = {
            "latest_revision": latest["revision"],
            "latest_cumulative": latest["cumulative"],
            "stale_projection_unchanged": state.projection_history[1] == state.projection_history[0],
            "stale_outcome": state.operation_results[1]["record_outcomes"][0],
            "conflict_outcome": state.operation_results[3]["record_outcomes"][0],
        }
    elif fixture_id == "drop_recovery_no_invented_context":
        latest = max(state.checkpoints.values(), key=lambda item: item["revision"])
        gap_duration = sum(gap["duration_usec"] for gap in gaps if gap["reason"] == "detail_queue_drop")
        known_context = sum(
            item["duration_usec"]
            for item in intervals
            if item["context_quality"] in {"observed", "partial"}
            and item["dimensions"] != {
                "level_band": 0,
                "class_id": 0,
                "race_id": 0,
                "faction_id": 0,
                "zone_vnum": -1,
                "group_size": 0,
            }
        )
        invented_context = 0
        for gap in gaps:
            if gap["duration_usec"]:
                for interval in intervals:
                    window = interval["window"]
                    overlap_start = max(window["start_monotonic_usec"], gap["start_monotonic_usec"])
                    overlap_end = min(window["end_monotonic_usec"], gap["end_monotonic_usec"])
                    if overlap_end > overlap_start:
                        invented_context += overlap_end - overlap_start
        recovered_connected = latest["cumulative"]["connected_usec"]
        if recovered_connected < connected:
            diagnostics.add(
                "$.expected.metrics.coverage",
                "recovered checkpoint cannot be smaller than admitted connected detail",
            )
        metrics_actual["coverage"] = {
            "detail_gap_count": sum(gap["reason"] == "detail_queue_drop" for gap in gaps),
            "gap_duration_usec": gap_duration,
            "known_context_usec": known_context,
            "recovered_connected_usec": recovered_connected,
            "recovered_cumulative": latest["cumulative"],
            "unattributed_connected_usec": recovered_connected - connected,
            "invented_context_usec": invented_context,
            "coverage_incomplete": bool(gaps),
        }
    elif fixture_id == "level_zone_config_boundary":
        ordered = sorted(intervals, key=lambda item: item["window"]["start_monotonic_usec"])
        boundaries: list[int] = []
        reasons: list[str] = []
        for previous, current in zip(ordered, ordered[1:]):
            if previous["window"]["end_monotonic_usec"] != current["window"]["start_monotonic_usec"]:
                continue
            changes = []
            if previous["dimensions"]["level_band"] != current["dimensions"]["level_band"]:
                changes.append("level")
            if previous["dimensions"]["zone_vnum"] != current["dimensions"]["zone_vnum"]:
                changes.append("zone")
            if previous["config_id"] != current["config_id"]:
                changes.append("config")
            if changes:
                boundaries.append(current["window"]["start_monotonic_usec"])
                reasons.append(changes[0])
        metrics_actual["boundaries"] = {
            "sealed_boundaries_monotonic_usec": boundaries,
            "boundary_reasons": reasons,
            "level_change_count": reasons.count("level"),
            "zone_change_count": reasons.count("zone"),
            "config_change_count": reasons.count("config"),
            "config_publication_before_use": all(state.admission_checks),
        }
    elif fixture_id == "midnight_clock_jump":
        day_buckets: defaultdict[str, int] = defaultdict(int)
        ambiguous = 0
        monotonic_total = 0
        for interval in intervals:
            monotonic_total += interval["duration_usec"]
            window = interval["window"]
            utc_start = window["start_utc_usec"]
            utc_end = window["end_utc_usec"]
            clock_quality = interval["quality_flags"] & (1 << 7)
            if (utc_start == INT64_MIN) != (utc_end == INT64_MIN):
                diagnostics.add("$.records", "unknown UTC must mark both interval endpoints unknown")
                continue
            if utc_start == INT64_MIN:
                continue
            if utc_end < utc_start or clock_quality:
                ambiguous += interval["duration_usec"]
                continue
            if utc_end - utc_start != interval["duration_usec"]:
                diagnostics.add(
                    "$.records",
                    "a stable UTC interval must conserve its monotonic duration",
                )
                continue
            cursor = utc_start
            while cursor < utc_end:
                day = cursor // DAY_USEC
                boundary = (day + 1) * DAY_USEC
                amount = min(utc_end, boundary) - cursor
                day_buckets[str(day)] += amount
                cursor += amount
        metrics_actual["time"] = {
            "monotonic_duration_usec": monotonic_total,
            "utc_day_duration_usec": dict(sorted(day_buckets.items())),
            "ambiguous_clock_jump_usec": ambiguous,
            "utc_attributed_duration_usec": sum(day_buckets.values()),
            "negative_elapsed_usec": 0,
            "duration_conserved": monotonic_total == ambiguous + sum(day_buckets.values()),
        }
    elif fixture_id == "detach_reconnect":
        metrics_actual["session"] = {
            "session_count": len(session_keys),
            "connection_count": len(connection_keys),
            "connection_sequences": sorted(
                {
                    connection["connection_seq"]
                    for record in durable_records
                    for kind in [record["header"]["kind"]]
                    for connection in [record["payload"][kind].get("connection")]
                    if isinstance(connection, dict) and not connection_is_zero(connection)
                }
            ),
            "session_retained": len(session_keys) == 1,
            "new_connection": len(connection_keys) == 2,
            "transition_kinds": [transition["kind"] for transition in transition_values],
            "connected_usec": connected,
            "linkdead_usec": linkdead,
            "session_exit_count": sum(lifecycle["lifecycle"] == "session_exited" for lifecycle in lifecycles),
        }
    elif fixture_id == "copyover_handoff":
        interval_header_producers = {
            producer_token(record["header"]["key"]["producer"])
            for record in durable_records
            if record["header"]["kind"] == "interval"
        }
        connection_producers = {
            producer_token(record["payload"]["interval"]["connection"]["producer"])
            for record in durable_records
            if record["header"]["kind"] == "interval"
        }
        copyover = transition_values[0] if transition_values else None
        old_producer = copyover["session"]["id"]["producer"] if copyover else None
        new_producer = copyover["connection"]["producer"] if copyover else None
        checkpoint_values = [
            record["payload"]["session_checkpoint"]
            for record in durable_records
            if record["header"]["kind"] == "session_checkpoint"
        ]
        checkpoint_values.sort(key=lambda value: value["revision"])
        handoff_checkpoint = checkpoint_values[0] if checkpoint_values else None
        post_copyover_checkpoint = checkpoint_values[-1] if checkpoint_values else None
        # This fixture has complete observed detail on both sides of a stable
        # UTC handoff, so checkpoint deltas must equal ONLY new-process detail.
        # Never subtract monotonic endpoints belonging to different producers.
        if not (copyover and handoff_checkpoint and post_copyover_checkpoint):
            diagnostics.add("$.records", "copyover requires transition and both checkpoints")
            return [f"{source_name}: {item}" for item in diagnostics.items]
        old_entries = [item for item in lifecycles
                       if item["lifecycle"] == "session_entered"
                       and item["connection"]["producer"] == old_producer]
        if len(old_entries) != 1:
            diagnostics.add("$.records", "copyover requires one old-process observation anchor")
            return [f"{source_name}: {item}" for item in diagnostics.items]
        old_start = old_entries[0]["at_monotonic_usec"]
        old_end = handoff_checkpoint["at_monotonic_usec"]
        new_start = copyover["at_monotonic_usec"]
        new_end = post_copyover_checkpoint["at_monotonic_usec"]
        gap_start = handoff_checkpoint["at_utc_usec"]
        gap_end = copyover["at_utc_usec"]
        if old_end < old_start or new_end < new_start or gap_end < gap_start:
            diagnostics.add("$.records", "copyover fixture observation boundaries are reversed")
        cross_process_duration = 0
        downtime_overlap = 0
        new_totals = dict.fromkeys(handoff_checkpoint["cumulative"], 0)
        for record in durable_records:
            if record["header"]["kind"] != "interval":
                continue
            interval = record["payload"]["interval"]
            window = interval["window"]
            producer = record["header"]["key"]["producer"]
            if producer == old_producer:
                lower, upper = old_start, old_end
            elif producer == new_producer:
                lower, upper = new_start, new_end
                duration = interval["duration_usec"]
                new_totals["resident_usec"] += duration
                category = interval["category"]
                if category == "resident_linkdead":
                    new_totals["linkdead_usec"] += duration
                else:
                    new_totals["connected_usec"] += duration
                    bucket = {"connected_active": "active_usec", "connected_idle": "idle_usec",
                              "unknown": "unknown_usec"}[category]
                    new_totals[bucket] += duration
            else:
                diagnostics.add("$.records", "copyover interval has an unrelated producer")
                cross_process_duration += interval["duration_usec"]
                continue
            inside = max(0, min(upper, window["end_monotonic_usec"]) -
                         max(lower, window["start_monotonic_usec"]))
            cross_process_duration += interval["duration_usec"] - inside
            downtime_overlap += max(0, min(gap_end, window["end_utc_usec"]) -
                                    max(gap_start, window["start_utc_usec"]))
        deltas = {key: post_copyover_checkpoint["cumulative"][key] - amount
                  for key, amount in handoff_checkpoint["cumulative"].items()}
        excess_checkpoint = max(0, deltas["resident_usec"] - new_totals["resident_usec"])
        downtime_invented = max(downtime_overlap, excess_checkpoint)
        if deltas != new_totals:
            diagnostics.add("$.records", "copyover checkpoint delta differs from observed new-process time")
        if cross_process_duration or downtime_invented:
            diagnostics.add("$.records", "copyover counted time outside its incarnation or inside handoff downtime")
        metrics_actual["copyover"] = {
            "session_count": len(session_keys),
            "producer_count": len(interval_header_producers | connection_producers),
            "old_producer": old_producer,
            "new_producer": new_producer,
            "session_retained": len(session_keys) == 1,
            "new_connection": len(connection_keys) == 2,
            "observed_interval_usec": conservation["interval_total_usec"],
            "cross_process_duration_usec": cross_process_duration,
            "downtime_invented_usec": downtime_invented,
            "transition_kind": copyover["kind"] if copyover else None,
            "handoff_checkpoint_revision": handoff_checkpoint["revision"] if handoff_checkpoint else None,
            "handoff_checkpoint_cumulative": handoff_checkpoint["cumulative"] if handoff_checkpoint else None,
            "post_copyover_checkpoint_revision": post_copyover_checkpoint["revision"] if post_copyover_checkpoint else None,
            "post_copyover_checkpoint_cumulative": post_copyover_checkpoint["cumulative"] if post_copyover_checkpoint else None,
            "handoff_quality_flags": handoff_checkpoint["quality_flags"] if handoff_checkpoint else None,
            "post_copyover_quality_flags": post_copyover_checkpoint["quality_flags"] if post_copyover_checkpoint else None,
            "fresh_monotonic_anchor": bool(
                handoff_checkpoint
                and post_copyover_checkpoint
                and post_copyover_checkpoint["at_monotonic_usec"] < handoff_checkpoint["at_monotonic_usec"]
            ),
        }
    elif fixture_id == "unclosed_crash_tail":
        unclosed = [gap for gap in gaps if gap["reason"] == "unclosed_tail"]
        metrics_actual["tail"] = {
            "session_exit_count": sum(lifecycle["lifecycle"] == "session_exited" for lifecycle in lifecycles),
            "unclosed_tail_gap_count": len(unclosed),
            "tail_duration_known": any(gap["duration_usec"] > 0 for gap in unclosed),
            "claimed_tail_usec": sum(gap["duration_usec"] for gap in unclosed),
            "complete": not unclosed and all(lifecycle["lifecycle"] == "session_exited" for lifecycle in lifecycles),
            "unclosed_quality_flag": all(gap["quality_flags"] & (1 << 6) for gap in unclosed),
        }

    for metric_name, actual in metrics_actual.items():
        compare_exact(actual, metrics.get(metric_name), f"$.expected.metrics.{metric_name}", diagnostics)

    if fixture_id == "drop_recovery_no_invented_context":
        missing_sequences = set()
        present_sequences = defaultdict(set)
        for record in durable_records:
            key = record["header"]["key"]
            producer = producer_token(key["producer"])
            present_sequences[producer].add(key["record_seq"])
        for gap in gaps:
            if gap["first_missing_record_seq"]:
                producer = producer_token(
                    next(
                        record["header"]["key"]["producer"]
                        for record in durable_records
                        if record["header"]["kind"] == "coverage_gap"
                        and record["payload"]["coverage_gap"] is gap
                    )
                )
                missing_sequences.update(
                    (producer, sequence)
                    for sequence in range(gap["first_missing_record_seq"], gap["last_missing_record_seq"] + 1)
                )
        if any(sequence in present_sequences[producer] for producer, sequence in missing_sequences):
            diagnostics.add("$.records", "coverage gap claims a sequence that is present")

    if fixture_id == "level_zone_config_boundary" and not state.admission_checks:
        diagnostics.add("$.operations", "config-boundary fixture must exercise publication order")

    return [f"{source_name}: {item}" for item in diagnostics.items]


def load_json(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as stream:
        value = json.load(stream)
    if not isinstance(value, dict):
        raise AssertionError(f"{path}: top-level JSON must be an object")
    return value


def expect_mutation_rejected(data: dict[str, Any], label: str, mutate: Any) -> None:
    mutated = deepcopy(data)
    mutate(mutated)
    diagnostics = validate_fixture(mutated, f"mutation:{label}")
    if not diagnostics:
        raise AssertionError(f"mutation-negative {label} unexpectedly passed")


def run_mutation_negatives(fixtures: dict[str, dict[str, Any]]) -> int:
    expect_mutation_rejected(
        fixtures["normal_interval"],
        "normal expected duration",
        lambda value: value["expected"]["metrics"]["conservation"].__setitem__("active_usec", 101),
    )
    expect_mutation_rejected(
        fixtures["normal_interval"],
        "config fingerprint known answer",
        lambda value: value["configurations"][0].__setitem__("fingerprint", "f" * 64),
    )
    expect_mutation_rejected(
        fixtures["replay_identical_conflict"],
        "replay expected conflict outcome",
        lambda value: value["expected"]["operations"][2]["record_outcomes"].__setitem__(0, "applied"),
    )
    expect_mutation_rejected(
        fixtures["commit_ambiguity_retry"],
        "immutable retry payload",
        lambda value: value["records"][1]["payload"]["interval"].__setitem__("policy_version", 999),
    )
    expect_mutation_rejected(
        fixtures["checkpoint_revision_conflict"],
        "checkpoint expected revision",
        lambda value: value["expected"]["metrics"]["checkpoint"].__setitem__("latest_revision", 4),
    )
    expect_mutation_rejected(
        fixtures["drop_recovery_no_invented_context"],
        "recovered cumulative field",
        lambda value: value["records"][3]["payload"]["session_checkpoint"]["cumulative"].__setitem__("active_usec", 90),
    )
    expect_mutation_rejected(
        fixtures["level_zone_config_boundary"],
        "zone boundary field",
        lambda value: value["records"][4]["payload"]["interval"]["dimensions"].__setitem__("zone_vnum", 102),
    )
    expect_mutation_rejected(
        fixtures["midnight_clock_jump"],
        "clock jump expected duration",
        lambda value: value["expected"]["metrics"]["time"].__setitem__("ambiguous_clock_jump_usec", 99),
    )
    expect_mutation_rejected(
        fixtures["detach_reconnect"],
        "reconnect connection identity",
        lambda value: value["transitions"][1]["connection"].__setitem__("connection_seq", 3),
    )
    expect_mutation_rejected(
        fixtures["copyover_handoff"],
        "copyover producer identity",
        lambda value: value["transitions"][0]["connection"]["producer"].__setitem__("boot_id", 801),
    )
    expect_mutation_rejected(
        fixtures["unclosed_crash_tail"],
        "unclosed tail duration field",
        lambda value: value["records"][2]["payload"]["coverage_gap"].__setitem__("duration_usec", 1),
    )
    expect_mutation_rejected(
        fixtures["normal_interval"], "out-of-order first observation",
        lambda value: value["records"][0]["header"]["key"].__setitem__("record_seq", 10),
    )
    def inflate_checkpoint_and_expected(value):
        # Keep the expected checkpoint in sync: only independent elapsed-time
        # accounting can expose this fabrication, not an expected-value mismatch.
        for key in ("connected_usec", "active_usec", "resident_usec"):
            value["records"][5]["payload"]["session_checkpoint"]["cumulative"][key] += 10
            value["expected"]["metrics"]["copyover"]["post_copyover_checkpoint_cumulative"][key] += 10
    expect_mutation_rejected(fixtures["copyover_handoff"],
                             "invented copyover checkpoint time", inflate_checkpoint_and_expected)
    def cross_old_anchor(value):
        window = value["records"][1]["payload"]["interval"]["window"]
        # Preserve duration, UTC endpoints and every expected aggregate. Only the
        # old-process entry/checkpoint bounds reveal these invented endpoints.
        window["start_monotonic_usec"] -= 10
        window["end_monotonic_usec"] -= 10
    expect_mutation_rejected(fixtures["copyover_handoff"],
                             "interval outside old incarnation anchor", cross_old_anchor)
    return 14


def run_identity_exclusion_check(fixtures: dict[str, dict[str, Any]]) -> int:
    mutated = deepcopy(fixtures["normal_interval"])
    mutated["configurations"][0]["effective_utc_usec"] += 12345
    diagnostics = validate_fixture(mutated, "identity-exclusion:effective_utc_usec")
    if diagnostics:
        raise AssertionError("effective_utc_usec unexpectedly changed the effective config identity")
    return 1


def main() -> int:
    expected_ids = set(EXPECTED_COVERAGE)
    paths = sorted(FIXTURE_DIR.glob("*.json"))
    actual_ids = {path.stem for path in paths}
    if actual_ids != expected_ids:
        missing = sorted(expected_ids - actual_ids)
        extra = sorted(actual_ids - expected_ids)
        raise AssertionError(f"fixture discovery mismatch; missing={missing}, extra={extra}")
    fixtures = {path.stem: load_json(path) for path in paths}
    all_diagnostics: list[str] = []
    for path in paths:
        diagnostics = validate_fixture(fixtures[path.stem], path)
        all_diagnostics.extend(diagnostics)
    if all_diagnostics:
        raise AssertionError("\n".join(all_diagnostics))
    mutation_count = run_mutation_negatives(fixtures)
    identity_exclusion_count = run_identity_exclusion_check(fixtures)
    print(
        f"telemetry contract fixtures passed: {len(paths)} fixtures, "
        f"{sum(len(value['records']) for value in fixtures.values())} records, "
        f"{sum(len(value['operations']) for value in fixtures.values())} apply operations, "
        f"{mutation_count} mutation-negative checks, "
        f"{identity_exclusion_count} identity-exclusion check"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, json.JSONDecodeError) as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)
