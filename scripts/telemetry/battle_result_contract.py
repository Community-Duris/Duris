"""Definition-1 native participant outcomes and exact objective identities.

Intrinsic validation proves a value's shape. Battle/alias/roster linkage, source
inventory, retained configuration and independent loss review must be qualified
before any comparison or denominator can use it. A committed zone touch is a
specific objective, never a zone clear or a whole-battle victory.
"""
from __future__ import annotations

from collections.abc import Mapping

try:
    from . import battle_contract as battle, control_contract as actors
except ImportError:
    import battle_contract as battle
    import control_contract as actors

DEFINITION_VERSION = 1
PRODUCER_VERSION = 1
ESCAPE_MIN_USEC = 30_000_000
FLAGS = 511
UTC_UNKNOWN = -(1 << 63)

# The actor/association value domain is shared with typed controls. This Python
# layout is independent of the native result descriptor, including its new
# event fields and the exact operation identity bytes.
FIELD_LAYOUT = (
    ("bout_boot_id", 8, False), ("bout_process_id", 8, False),
    ("bout_sequence", 8, False), ("bout_parent_sequence", 8, False),
    ("bout_environment_id", 8, False), ("bout_season_id", 8, False), ("bout_config_id", 8, False),
    ("bout_classifier_version", 4, False), ("bout_policy_version", 4, False),
    ("bout_scope_zone_vnum", 4, True), ("bout_scope_group_key", 8, False),
    ("bout_build_version", 4, False), ("bout_content_version", 4, False),
    *((name.replace("ctl_", "bout_", 1), width, signed) for name, width, signed in actors.FIELD_LAYOUT
        if name.startswith(("ctl_source_", "ctl_target_"))),
    ("bout_start_usec", 8, False), ("bout_start_utc_usec", 8, True),
    ("bout_at_usec", 8, False), ("bout_at_utc_usec", 8, True),
    ("bout_operation_id", 16, None), ("bout_source_object_uid", 8, False),
    ("bout_proof_window_usec", 8, False), ("bout_credited_zone_vnum", 4, True),
    ("bout_from_room_vnum", 4, True), ("bout_to_room_vnum", 4, True),
    ("bout_participant_count", 2, False), ("bout_source_payload_version", 2, False),
    ("bout_definition_version", 2, False), ("bout_producer_version", 2, False),
    ("bout_flags", 2, False), ("bout_kind", 1, False), ("bout_authority", 1, False),
    ("bout_reason", 1, False), ("bout_quality_flags", 4, False),
)
FIELDS = tuple(name for name, _, _ in FIELD_LAYOUT)
BYTE_FIELDS = ("bout_operation_id",)
WIRE_BYTES = sum(width for _, width, _ in FIELD_LAYOUT)


class ResultContractError(ValueError):
    """Invalid result value; no private source payload in diagnostics."""


def _require(condition, reason):
    if not condition:
        raise ResultContractError(reason)


def observation_key(row: Mapping) -> tuple[int, int, int]:
    return tuple(row["bout_" + name] for name in ("boot_id", "process_id", "sequence"))


def association(row: Mapping, prefix: str) -> tuple[int, int, int]:
    _require(prefix in ("source", "target"), "result actor prefix")
    return tuple(row["bout_" + prefix + "_" + name] for name in (
        "battle_seq", "association_revision", "association_fact_sequence"))


def actor_context(row: Mapping, prefix: str) -> dict:
    _require(prefix in ("source", "target"), "result actor prefix")
    fields = {name.replace("bout_", "ctl_", 1): row[name] for name in FIELDS
        if name.startswith("bout_" + prefix + "_")}
    return actors.actor_context(fields, prefix)


def validate_observation(row: Mapping) -> dict:
    _require(isinstance(row, Mapping) and set(row) == set(FIELDS), "result exact fields")
    v = dict(row)
    for name, width, signed in FIELD_LAYOUT:
        value = v[name]
        if signed is None:
            _require(type(value) is bytes and len(value) == width, "result exact bytes")
        else:
            limit = 1 << (8 * width - int(signed))
            _require(type(value) is int and (-limit if signed else 0) <= value < limit, "result integer width")

    def value(name):
        return v["bout_" + name]

    kind, authority, reason, quality, flags = (value(name) for name in ("kind", "authority", "reason", "quality_flags", "flags"))
    uncertain = kind in (7, 8)
    versions = tuple(value(name) for name in ("config_id", "classifier_version", "policy_version", "build_version", "content_version"))
    _require(all(value(name) > 0 for name in ("boot_id", "process_id", "sequence", "environment_id", "season_id")) and
        value("parent_sequence") < value("sequence") and value("definition_version") == DEFINITION_VERSION and
        value("producer_version") == PRODUCER_VERSION and value("scope_zone_vnum") == -1 and value("scope_group_key") == 0 and
        (all(item == 0 for item in versions) if uncertain and reason == 9 else all(versions)), "result scope/version/identity")
    context = {name.replace("bout_", "ctl_", 1): item for name, item in v.items()
        if name.startswith(("bout_source_", "bout_target_"))}
    source_zero = all(item == 0 for name, item in context.items() if name.startswith("ctl_source_") and
        name not in ("ctl_source_object_uid", "ctl_source_payload_version"))
    _require(actors._actor_valid(context, "target") and (source_zero or actors._actor_valid(context, "source")),
        "result actor lifetime/context")
    for prefix in ("source", "target"):
        reference = actors._association(context, prefix)
        _require(reference == (0, 0, 0) or all(reference), "result complete optional association")
    _require(quality & ~battle.QUALITY_KNOWN == 0 and flags & ~FLAGS == 0 and
        (value("source_quality_flags") | value("target_quality_flags")) & ~quality == 0 and
        value("start_usec") <= value("at_usec") and 1 <= kind <= 8 and 1 <= authority <= 9 and reason <= 14 and
        all(value(name) >= -1 for name in ("credited_zone_vnum", "from_room_vnum", "to_room_vnum")),
        "result quality/clocks/type")
    first, last = value("start_utc_usec"), value("at_utc_usec")
    _require(first == UTC_UNKNOWN or last == UTC_UNKNOWN or first <= last or quality & 128, "result explicit UTC discontinuity")
    operation = any(value("operation_id"))
    objective = kind in (5, 6) or kind == 7 and 6 <= authority <= 8
    if objective:
        _require(operation and value("credited_zone_vnum") >= 0 and 1 <= value("participant_count") <= 15 and
            source_zero and value("parent_sequence") == value("proof_window_usec") == 0 and
            value("target_actor_kind") == 1 and
            (reason != 0 and quality & 1 if uncertain else reason == 0) and
            value("from_room_vnum") == value("to_room_vnum") == -1 and flags & ~(1 | 64 | 128 | 256) == 0 and
            (value("source_payload_version") == 1 and value("source_object_uid") == 0 and flags & 128 or
                value("source_payload_version") == 2 and value("source_object_uid") != 0) and
            (kind == 7 or (authority == 6 and not flags & 256 if kind == 5 else authority in (7, 8))),
            "result objective request/committed/unresolved shape")
        return v
    _require(not operation and value("source_object_uid") == value("participant_count") == value("source_payload_version") == 0 and
        value("credited_zone_vnum") == -1 and flags & (64 | 128 | 256) == 0, "result absent objective fields")
    if uncertain:
        _require(reason != 0 and quality & 1 and value("proof_window_usec") == 0 and flags & (8 | 16 | 32) == 0 and
            (kind != 8 or value("parent_sequence") != 0), "result unresolved/censored evidence")
        return v
    _require(reason == 0, "result observed reason")
    if kind == 1:
        _require(authority == 1 and value("parent_sequence") == value("proof_window_usec") == 0 and flags & ~(1 | 2) == 0 and
            value("from_room_vnum") == value("to_room_vnum") == -1, "result accepted death shape")
        return v
    _require(source_zero and not flags & 2, "result movement has no guessed causal actor")
    moved = flags & 4 and value("from_room_vnum") >= 0 and value("to_room_vnum") >= 0 and value("from_room_vnum") != value("to_room_vnum")
    if kind == 4:
        _require(authority == 5 and value("parent_sequence") != 0 and moved and value("proof_window_usec") == ESCAPE_MIN_USEC and
            value("at_usec") - value("start_usec") >= value("proof_window_usec") and flags & 60 == 60 and
            value("target_actor_kind") == 1 and all(actors._association(context, "target")) and
            all(value("target_session_" + name) for name in ("boot_id", "process_id", "seq")) and
            (value("target_session_boot_id"), value("target_session_process_id")) == observation_key(v)[:2],
            "result bounded escape confirmation")
        return v
    _require(value("parent_sequence") == value("proof_window_usec") == 0 and flags & ~(1 | 4) == 0 and
        (kind == 2 and authority == 2 and moved or kind == 3 and (authority == 3 and moved or
            authority == 4 and not flags & 4 and value("from_room_vnum") >= 0 and value("from_room_vnum") == value("to_room_vnum"))),
        "result accepted flee/withdrawal shape")
    return v


def encode_observation(row: Mapping) -> bytes:
    v = validate_observation(row)
    return b"".join(v[name] if signed is None else v[name].to_bytes(width, "big", signed=signed)
        for name, width, signed in FIELD_LAYOUT)


def decode_observation(data: bytes) -> dict:
    _require(type(data) is bytes and len(data) == WIRE_BYTES, "result exact wire length/type")
    offset, row = 0, {}
    for name, width, signed in FIELD_LAYOUT:
        part = data[offset:offset + width]
        row[name] = part if signed is None else int.from_bytes(part, "big", signed=signed)
        offset += width
    return validate_observation(row)


def validate_raw_observation(row: Mapping) -> dict:
    """Exact kind-14 transport identity; no battle/operation linkage inferred."""
    _require(isinstance(row, Mapping) and type(row.get("record_kind")) is int and row["record_kind"] == 14 and
        type(row.get("schema_version")) is int and row["schema_version"] == 1, "result raw tag/schema")
    value = validate_observation({name: row.get(name) for name in FIELDS})
    _require(all(type(row.get(name)) is int and 0 < row[name] <= battle.UINT64_MAX
        for name in ("boot_id", "process_id", "record_seq")) and
        (row["boot_id"], row["process_id"]) == observation_key(value)[:2] and
        type(row.get("occurrence_utc_usec")) is int and row["occurrence_utc_usec"] == value["bout_at_utc_usec"],
        "result raw replay/producer/occurrence")
    header = {"ingest_id", "boot_id", "process_id", "record_seq", "schema_version", "record_kind",
        "occurrence_utc_usec", "ingested_utc_usec"}
    _require(all(item is None for name, item in row.items() if name not in FIELDS and name not in header),
        "result raw inactive family payload")
    return value
