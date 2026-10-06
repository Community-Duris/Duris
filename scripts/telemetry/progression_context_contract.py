"""Independent definition-1 value contract for progression context.

This validates observations, not XP authority, identity attribution or complete
exposure. Exact source/configuration/loss qualification belongs to publication.
"""
from __future__ import annotations

from collections.abc import Mapping
import hashlib

VERSION = 1
SOURCE_INVENTORY_VERSION = 1
UTC_UNKNOWN = -(1 << 63)
FLAGS = (1 << 21) - 1
QUALITY_FLAGS = (1 << 10) - 1

AFFECTS_COMPLETE = 1 << 0
AUTOMATIC_RESTED = 1 << 1
RESTED_PRESENT = 1 << 2
WELLRESTED_PRESENT = 1 << 3
RESTED_STAFF = 1 << 4
WELLRESTED_STAFF = 1 << 5
SELECTION_KNOWN = 1 << 6
APPLICATION_KNOWN = 1 << 7
ASSISTANCE_KNOWN = 1 << 8
GROUP_ELIGIBILITY_KNOWN = 1 << 9
THRESHOLD_KNOWN = 1 << 10
BUILD_KNOWN = 1 << 11
GROUP_ROSTER_KNOWN = 1 << 12
STORAGE_GATE_KNOWN = 1 << 13
STORAGE_GATE_PASSED = 1 << 14
CONTIGUOUS_EXPOSURE = 1 << 15
ALIVE = 1 << 16
CONFIG_CATALOG_KNOWN = 1 << 17
DECISION_POLICY_KNOWN = 1 << 18
HARDCORE = 1 << 19
HARDCORE_BYPASS = 1 << 20

FIELD_LAYOUT = (
    ("pctx_boot_id", 8, False),
    ("pctx_process_id", 8, False),
    ("pctx_sequence", 8, False),
    ("pctx_session_boot_id", 8, False),
    ("pctx_session_process_id", 8, False),
    ("pctx_session_seq", 8, False),
    ("pctx_subject_id", 8, False),
    ("pctx_pid", 4, True),
    ("pctx_environment_id", 8, False),
    ("pctx_season_id", 8, False),
    ("pctx_connection_boot_id", 8, False),
    ("pctx_connection_process_id", 8, False),
    ("pctx_connection_seq", 8, False),
    ("pctx_source_boot_id", 8, False),
    ("pctx_source_process_id", 8, False),
    ("pctx_source_record_seq", 8, False),
    ("pctx_ownership_boot_id", 8, False),
    ("pctx_ownership_process_id", 8, False),
    ("pctx_ownership_record_seq", 8, False),
    ("pctx_account_token", 8, False),
    ("pctx_start_usec", 8, False),
    ("pctx_at_usec", 8, False),
    ("pctx_start_utc_usec", 8, True),
    ("pctx_at_utc_usec", 8, True),
    ("pctx_config_id", 8, False),
    ("pctx_configuration_record_seq", 8, False),
    ("pctx_configuration_digest_0", 8, False),
    ("pctx_configuration_digest_1", 8, False),
    ("pctx_configuration_digest_2", 8, False),
    ("pctx_configuration_digest_3", 8, False),
    ("pctx_decision_level_cap", 4, True),
    ("pctx_decision_good_assistance_gap", 4, True),
    ("pctx_decision_evil_assistance_gap", 4, True),
    ("pctx_decision_max_exp_level", 4, True),
    ("pctx_next_threshold_xp", 8, False),
    ("pctx_current_exp", 8, True),
    ("pctx_primary_class_mask", 4, False),
    ("pctx_secondary_class_mask", 4, False),
    ("pctx_build_version", 4, False),
    ("pctx_content_version", 4, False),
    ("pctx_classifier_version", 4, False),
    ("pctx_policy_version", 4, False),
    ("pctx_quality_flags", 4, False),
    ("pctx_flags", 4, False),
    ("pctx_base_stat_0", 2, True),
    ("pctx_base_stat_1", 2, True),
    ("pctx_base_stat_2", 2, True),
    ("pctx_base_stat_3", 2, True),
    ("pctx_base_stat_4", 2, True),
    ("pctx_base_stat_5", 2, True),
    ("pctx_base_stat_6", 2, True),
    ("pctx_base_stat_7", 2, True),
    ("pctx_base_stat_8", 2, True),
    ("pctx_base_stat_9", 2, True),
    ("pctx_effective_stat_0", 2, True),
    ("pctx_effective_stat_1", 2, True),
    ("pctx_effective_stat_2", 2, True),
    ("pctx_effective_stat_3", 2, True),
    ("pctx_effective_stat_4", 2, True),
    ("pctx_effective_stat_5", 2, True),
    ("pctx_effective_stat_6", 2, True),
    ("pctx_effective_stat_7", 2, True),
    ("pctx_effective_stat_8", 2, True),
    ("pctx_effective_stat_9", 2, True),
    ("pctx_formal_group_size", 4, False),
    ("pctx_current_level", 2, False),
    ("pctx_threshold_level", 2, False),
    ("pctx_threshold_catalog_version", 2, False),
    ("pctx_specialization", 2, False),
    ("pctx_race", 2, False),
    ("pctx_faction", 2, False),
    ("pctx_eligible_group_size", 2, False),
    ("pctx_highest_group_level", 2, False),
    ("pctx_assistance_level", 2, False),
    ("pctx_version", 2, False),
    ("pctx_rested_selection", 1, False),
    ("pctx_rested_application", 1, False),
    ("pctx_assistance", 1, False),
    ("pctx_starting_level", 2, False),
    ("pctx_source_inventory_version", 2, False),
    ("pctx_boundary", 1, False),
    ("pctx_source_kind", 1, False),
)
FIELDS = tuple(name for name, _, _ in FIELD_LAYOUT)
WIRE_BYTES = sum(width for _, width, _ in FIELD_LAYOUT)


class ContextContractError(ValueError):
    """Payload-free invalid value or context contract failure."""


def _require(condition, reason):
    if not condition:
        raise ContextContractError(reason)


def observation_key(row):
    return tuple(row["pctx_" + name] for name in ("boot_id", "process_id", "sequence"))


def source_reference(row):
    return tuple(row["pctx_source_" + name] for name in ("boot_id", "process_id", "record_seq"))


def ownership_reference(row):
    return tuple(row["pctx_ownership_" + name] for name in ("boot_id", "process_id", "record_seq"))


def validate_observation(row: Mapping) -> dict:
    _require(isinstance(row, Mapping) and set(row) == set(FIELDS), "progression context exact fields")
    v = dict(row)
    for name, width, signed in FIELD_LAYOUT:
        value = v[name]
        bound = 1 << (8 * width - int(signed))
        _require(type(value) is int and (-bound if signed else 0) <= value < bound,
            "progression context integer width")

    def value(name):
        return v["pctx_" + name]

    flags, quality = value("flags"), value("quality_flags")
    _require(value("version") == VERSION and value("source_inventory_version") == SOURCE_INVENTORY_VERSION and
        flags & ~FLAGS == quality & ~QUALITY_FLAGS == 0 and value("current_level") > 0,
        "progression context version/flags/level")
    versions = tuple(value(name) for name in ("config_id", "build_version", "content_version",
        "classifier_version", "policy_version"))
    configured = all(versions)
    _require(configured or not any(versions) and quality & 1, "progression context configuration shape")
    _require(not flags & RESTED_STAFF or flags & RESTED_PRESENT,
        "progression context rested staff presence")
    _require(not flags & WELLRESTED_STAFF or flags & WELLRESTED_PRESENT,
        "progression context wellrested staff presence")
    selected, applied = value("rested_selection"), value("rested_application")
    if flags & SELECTION_KNOWN:
        _require(1 <= selected <= 3, "progression context selection")
        if flags & AFFECTS_COMPLETE:
            well = flags & WELLRESTED_PRESENT and (flags & AUTOMATIC_RESTED or
                flags & ALIVE and flags & WELLRESTED_STAFF)
            rested = flags & RESTED_PRESENT and (flags & AUTOMATIC_RESTED or
                flags & ALIVE and flags & RESTED_STAFF)
            _require(selected == (3 if well else 2 if rested else 1),
                "progression context actual selection inputs")
    else:
        _require(selected == 0, "progression context unknown selection")
    if flags & APPLICATION_KNOWN:
        _require(1 <= applied <= 4 and (not flags & SELECTION_KNOWN or applied == 1 or applied == selected + 1),
            "progression context actual application")
    else:
        _require(applied == 0, "progression context unknown application")
    threshold = tuple(value(name) for name in ("next_threshold_xp", "threshold_level", "threshold_catalog_version"))
    if flags & THRESHOLD_KNOWN:
        _require(configured and threshold[0] > 0 and threshold[2] == 1 and
            value("current_level") < 65535 and threshold[1] == value("current_level") + 1,
            "progression context threshold identity")
    else:
        _require(not any(threshold), "progression context unknown threshold")
    if not flags & BUILD_KNOWN:
        _require(not any(value(name) for name in ("primary_class_mask", "secondary_class_mask",
            "specialization", "race", "faction",
            *(kind + "_stat_" + str(index) for kind in ("base", "effective") for index in range(10)))),
            "progression context unavailable build")
    _require(value("formal_group_size") > 0 if flags & GROUP_ROSTER_KNOWN else value("formal_group_size") == 0,
        "progression context formal roster")
    _require(not flags & STORAGE_GATE_PASSED or flags & STORAGE_GATE_KNOWN,
        "progression context storage gate")
    assistance, group_size, highest = (value(name) for name in ("assistance", "eligible_group_size", "highest_group_level"))
    if flags & ASSISTANCE_KNOWN:
        _require(1 <= assistance <= 7, "progression context assistance rule")
        if assistance == 1:
            _require(flags & GROUP_ELIGIBILITY_KNOWN and group_size == 1 and highest == value("current_level"),
                "progression context native solo share")
        if assistance == 2:
            _require(flags & GROUP_ELIGIBILITY_KNOWN and group_size > 0 and highest >= value("current_level"),
                "progression context native group share")
        if assistance == 5:
            _require(flags & GROUP_ELIGIBILITY_KNOWN and group_size >= 2 and highest == 0,
                "progression context native tanking group")
    else:
        _require(assistance == 0 and not flags & GROUP_ELIGIBILITY_KNOWN and value("assistance_level") == 0,
            "progression context unknown assistance")
    _require(flags & GROUP_ELIGIBILITY_KNOWN or group_size == highest == 0,
        "progression context unknown group eligibility")
    producer = observation_key(v)[:2]
    _require(all(observation_key(v)) and all(value(name) > 0 for name in
        ("session_boot_id", "session_process_id", "session_seq", "subject_id", "environment_id", "season_id")) and
        value("pid") > 0, "progression context source/session identity")
    connection = tuple(value("connection_" + name) for name in ("boot_id", "process_id", "seq"))
    _require(connection == (0, 0, 0) or all(connection) and connection[:2] == producer,
        "progression context connection identity")
    source, ownership = source_reference(v), ownership_reference(v)
    for reference in (source, ownership):
        _require(reference == (0, 0, 0) or all(reference) and reference[:2] == producer,
            "progression context optional source identity")
    _require(value("account_token") == 0 or ownership != (0, 0, 0),
        "progression context ownership anchor")
    _require(value("start_usec") <= value("at_usec") and
        (value("start_utc_usec") == UTC_UNKNOWN or value("at_utc_usec") == UTC_UNKNOWN or
            value("start_utc_usec") <= value("at_utc_usec") or quality & 128),
        "progression context source clocks")
    has_digest = any(value("configuration_digest_" + str(index)) for index in range(4))
    if flags & CONFIG_CATALOG_KNOWN:
        _require(configured and value("configuration_record_seq") > 0 and has_digest,
            "progression context catalog reference")
    else:
        _require(value("configuration_record_seq") == 0 and not has_digest,
            "progression context unavailable catalog")
    if not flags & DECISION_POLICY_KNOWN:
        _require(not any(value("decision_" + name) for name in
            ("level_cap", "good_assistance_gap", "evil_assistance_gap", "max_exp_level")) and
            not flags & HARDCORE_BYPASS, "progression context unavailable decision policy")
    _require(not flags & HARDCORE_BYPASS or flags & HARDCORE,
        "progression context actual hardcore bypass")
    boundary, source_kind = value("boundary"), value("source_kind")
    _require(1 <= boundary <= 6 and source_kind in (0, 1, 2, 6) and
        (source != (0, 0, 0)) == (source_kind != 0), "progression context source family")
    if boundary == 1:
        _require(source_kind == 0, "progression context baseline")
    if boundary == 2:
        _require(source_kind == 1 and value("start_usec") < value("at_usec"),
            "progression context interval source")
    if boundary in (3, 4):
        _require(source_kind == 6, "progression context XP/level source")
    if boundary == 5:
        _require(source_kind == 2, "progression context lifecycle source")
    _require(not flags & CONTIGUOUS_EXPOSURE or boundary == 2,
        "progression context exposure boundary")
    _require(not flags & (APPLICATION_KNOWN | STORAGE_GATE_KNOWN | DECISION_POLICY_KNOWN) or boundary == 3,
        "progression context actual XP decision boundary")
    _require(boundary != 6 or quality & 1 and not flags & CONTIGUOUS_EXPOSURE,
        "progression context unknown boundary")
    return v


def encode_observation(row: Mapping) -> bytes:
    v = validate_observation(row)
    return b"".join(v[name].to_bytes(width, "big", signed=signed) for name, width, signed in FIELD_LAYOUT)


def decode_observation(data: bytes) -> dict:
    _require(type(data) is bytes and len(data) == WIRE_BYTES, "progression context exact wire shape")
    offset, row = 0, {}
    for name, width, signed in FIELD_LAYOUT:
        row[name] = int.from_bytes(data[offset:offset + width], "big", signed=signed)
        offset += width
    return validate_observation(row)



# Independent inventory enumeration. IDs refer to the source indices, never
# names, user properties, process-local config IDs or economic reward units.
CONFIGURATION_VERSION = 1
CONFIGURATION_IDS = (
    *range(1, 63), *range(100, 162), *range(200, 301), *range(400, 501),
    *range(600, 611),
)
CONFIGURATION_COUNT = 337
CONFIGURATION_CANONICAL_BYTES = 22 + CONFIGURATION_COUNT * 11
CONFIGURATION_HEADER = (
    ("version", 2), ("source_inventory_version", 2), ("count", 2),
    ("build_version", 4), ("content_version", 4),
    ("classifier_version", 4), ("policy_version", 4),
)


def _configuration_entry(entry: Mapping, expected_id: int) -> dict:
    _require(isinstance(entry, Mapping) and set(entry) == {"id", "kind", "bits"},
        "XP configuration typed value fields")
    e = dict(entry)
    _require(type(e["id"]) is int and e["id"] == expected_id and
        type(e["kind"]) is int and type(e["bits"]) is int and
        0 <= e["bits"] < 1 << 64, "XP configuration exact source indices/widths")
    if (1 <= expected_id <= 62 or expected_id == 600 or
            604 <= expected_id <= 606 or 608 <= expected_id <= 610):
        _require(e["kind"] == 2, "XP configuration signed source")
    elif 601 <= expected_id <= 603:
        _require(e["kind"] == 4 and
            e["bits"] & 0x7ff0000000000000 != 0x7ff0000000000000,
            "XP configuration finite float64 source")
    else:
        _require(e["kind"] == 3 and e["bits"] < 1 << 32 and
            e["bits"] & 0x7f800000 != 0x7f800000,
            "XP configuration finite float32 source")
    return e


def validate_configuration(row: Mapping) -> dict:
    _require(isinstance(row, Mapping) and set(row) ==
        {"config_id", "values", *(name for name, _ in CONFIGURATION_HEADER)},
        "XP configuration exact fields")
    v = dict(row)
    _require(type(v["config_id"]) is int and 0 < v["config_id"] < 1 << 64,
        "XP configuration reference")
    for name, width in CONFIGURATION_HEADER:
        _require(type(v[name]) is int and 0 < v[name] < 1 << (8 * width),
            "XP configuration header width")
    _require(v["version"] == CONFIGURATION_VERSION and
        v["source_inventory_version"] == SOURCE_INVENTORY_VERSION and
        v["count"] == CONFIGURATION_COUNT, "XP configuration definition/inventory")
    _require(type(v["values"]) is list and len(v["values"]) == CONFIGURATION_COUNT,
        "XP configuration complete inventory")
    values = []
    for expected_id, entry in zip(CONFIGURATION_IDS, v["values"]):
        e = _configuration_entry(entry, expected_id)
        values.append(e)
    v["values"] = values
    return v


def encode_configuration(row: Mapping) -> bytes:
    v = validate_configuration(row)
    return (b"".join(v[name].to_bytes(width, "big") for name, width in CONFIGURATION_HEADER) +
        b"".join(entry["id"].to_bytes(2, "big") + entry["kind"].to_bytes(1, "big") +
            entry["bits"].to_bytes(8, "big") for entry in v["values"]))


def decode_configuration(data: bytes, config_id: int) -> dict:
    _require(type(data) is bytes and len(data) == CONFIGURATION_CANONICAL_BYTES,
        "XP configuration exact canonical length")
    offset, row = 0, {"config_id": config_id, "values": []}
    for name, width in CONFIGURATION_HEADER:
        row[name] = int.from_bytes(data[offset:offset + width], "big")
        offset += width
    _require(row["count"] == CONFIGURATION_COUNT, "XP configuration bounded complete count")
    for _ in range(CONFIGURATION_COUNT):
        row["values"].append({"id": int.from_bytes(data[offset:offset + 2], "big"),
            "kind": data[offset + 2], "bits": int.from_bytes(data[offset + 3:offset + 11], "big")})
        offset += 11
    return validate_configuration(row)


def configuration_digest(row: Mapping) -> str:
    return hashlib.sha256(encode_configuration(row)).hexdigest()


CONFIGURATION_CHUNK_VALUES = 24
CONFIGURATION_CHUNKS = (CONFIGURATION_COUNT + CONFIGURATION_CHUNK_VALUES - 1) // CONFIGURATION_CHUNK_VALUES
CONFIGURATION_CHUNK_LAYOUT = (
    *(("pcfg_" + name, 8, False) for name in
        ("boot_id", "process_id", "sequence", "root_record_seq", "config_id", "environment_id", "season_id", "at_usec")),
    ("pcfg_at_utc_usec", 8, True),
    *(("pcfg_digest_" + str(index), 8, False) for index in range(4)),
    *(("pcfg_" + name, 4, False) for name in
        ("build_version", "content_version", "classifier_version", "policy_version")),
    *(("pcfg_" + name, 2, False) for name in
        ("total_values", "chunk_index", "chunk_count", "value_count", "version", "source_inventory_version")),
    *(("pcfg_value_" + name + "_" + str(index), width, False)
        for index in range(CONFIGURATION_CHUNK_VALUES)
        for name, width in (("id", 2), ("kind", 1), ("bits", 8))),
)
CONFIGURATION_CHUNK_WIRE_BYTES = sum(width for _, width, _ in CONFIGURATION_CHUNK_LAYOUT)


def validate_configuration_chunk(row: Mapping) -> dict:
    _require(isinstance(row, Mapping) and set(row) == {name for name, _, _ in CONFIGURATION_CHUNK_LAYOUT},
        "XP configuration chunk exact fields")
    v = dict(row)
    for name, width, signed in CONFIGURATION_CHUNK_LAYOUT:
        bound = 1 << (width * 8 - int(signed))
        _require(type(v[name]) is int and (-bound if signed else 0) <= v[name] < bound,
            "XP configuration chunk integer width")
    _require(all(v["pcfg_" + name] > 0 for name in
        ("boot_id", "process_id", "sequence", "root_record_seq", "config_id", "environment_id", "season_id",
         "build_version", "content_version", "classifier_version", "policy_version")) and
        any(v["pcfg_digest_" + str(index)] for index in range(4)),
        "XP configuration chunk source identity")
    _require(v["pcfg_version"] == CONFIGURATION_VERSION and
        v["pcfg_source_inventory_version"] == SOURCE_INVENTORY_VERSION and
        v["pcfg_total_values"] == CONFIGURATION_COUNT and v["pcfg_chunk_count"] == CONFIGURATION_CHUNKS and
        0 <= v["pcfg_chunk_index"] < CONFIGURATION_CHUNKS,
        "XP configuration chunk inventory/version")
    first = v["pcfg_chunk_index"] * CONFIGURATION_CHUNK_VALUES
    count = min(CONFIGURATION_CHUNK_VALUES, CONFIGURATION_COUNT - first)
    _require(v["pcfg_value_count"] == count, "XP configuration chunk complete ordinals")
    for index in range(CONFIGURATION_CHUNK_VALUES):
        entry = {name: v["pcfg_value_" + name + "_" + str(index)] for name in ("id", "kind", "bits")}
        if index < count:
            _configuration_entry(entry, CONFIGURATION_IDS[first + index])
        else:
            _require(not any(entry.values()), "XP configuration chunk stale unused values")
    return v


def encode_configuration_chunk(row: Mapping) -> bytes:
    v = validate_configuration_chunk(row)
    return b"".join(v[name].to_bytes(width, "big", signed=signed)
        for name, width, signed in CONFIGURATION_CHUNK_LAYOUT)


def decode_configuration_chunk(data: bytes) -> dict:
    _require(type(data) is bytes and len(data) == CONFIGURATION_CHUNK_WIRE_BYTES,
        "XP configuration chunk exact length")
    offset, row = 0, {}
    for name, width, signed in CONFIGURATION_CHUNK_LAYOUT:
        row[name] = int.from_bytes(data[offset:offset + width], "big", signed=signed)
        offset += width
    return validate_configuration_chunk(row)


def reconcile_configuration_chunks(chunks, receipts) -> dict:
    """Require a complete exact native inventory and its original receipt keys.

    Receipt-shape checks do not certify storage, review or authoritative XP.
    The retained-source owner must supply these keys from its qualified inputs.
    """
    _require(type(chunks) in (tuple, list) and type(receipts) in (tuple, list) and
        len(chunks) == len(receipts) == CONFIGURATION_CHUNKS,
        "XP configuration exact retained chunk count")
    values, indexed, seen_receipts = [], {}, set()
    common = None
    common_names = tuple(name for name, _, _ in CONFIGURATION_CHUNK_LAYOUT if
        not name.startswith("pcfg_value_") and name not in ("pcfg_chunk_index", "pcfg_value_count"))
    for packet, key in zip(chunks, receipts):
        v = validate_configuration_chunk(packet)
        _require(type(key) is tuple and len(key) == 3 and
            all(type(value) is int and 0 < value < 1 << 64 for value in key) and
            key[:2] == (v["pcfg_boot_id"], v["pcfg_process_id"]),
            "XP configuration original receipt")
        index = v["pcfg_chunk_index"]
        _require(key not in seen_receipts and index not in indexed and
            (key[2] == v["pcfg_root_record_seq"] if index == 0 else key[2] > v["pcfg_root_record_seq"]),
            "XP configuration root/replay receipt")
        seen_receipts.add(key)
        fields = tuple(v[name] for name in common_names)
        if common is None:
            common = fields
        _require(common == fields, "XP configuration mixed source catalog")
        indexed[index] = v
    ordered_receipts = {packet["pcfg_chunk_index"]: key for packet, key in zip(chunks, receipts)}
    _require(all(ordered_receipts[index - 1][2] < ordered_receipts[index][2]
        for index in range(1, CONFIGURATION_CHUNKS)), "XP configuration source admission order")
    for index in range(CONFIGURATION_CHUNKS):
        v = indexed[index]
        values.extend({name: v["pcfg_value_" + name + "_" + str(ordinal)] for name in ("id", "kind", "bits")}
            for ordinal in range(v["pcfg_value_count"]))
    root = indexed[0]
    result = {name: root["pcfg_" + name] for name, _ in CONFIGURATION_HEADER if name != "count"}
    result.update(config_id=root["pcfg_config_id"], count=CONFIGURATION_COUNT, values=values)
    digest = b"".join(root["pcfg_digest_" + str(index)].to_bytes(8, "big") for index in range(4)).hex()
    _require(configuration_digest(result) == digest, "XP configuration retained digest conflict")
    return validate_configuration(result)


def validate_raw_observation(row: Mapping) -> dict:
    """Bind family 15/16 values to their original admitted transport receipt.

    A matching header is not evidence that referenced XP, ownership or catalog
    packets were retained. Those exact joins belong to source qualification.
    """
    _require(isinstance(row, Mapping), "progression raw mapping")
    kind = row.get("record_kind")
    _require(type(kind) is int and kind in (15, 16) and
        type(row.get("schema_version")) is int and row["schema_version"] == 1,
        "progression raw tag/schema")
    layout = FIELD_LAYOUT if kind == 15 else CONFIGURATION_CHUNK_LAYOUT
    fields = tuple(name for name, _width, _signed in layout)
    value = {name: row.get(name) for name in fields}
    value = validate_observation(value) if kind == 15 else validate_configuration_chunk(value)
    prefix = "pctx_" if kind == 15 else "pcfg_"
    _require(all(type(row.get(name)) is int and 0 < row[name] < (1 << 64)
        for name in ("boot_id", "process_id", "record_seq")) and
        (row["boot_id"], row["process_id"]) ==
        (value[prefix + "boot_id"], value[prefix + "process_id"]) and
        type(row.get("occurrence_utc_usec")) is int and
        row["occurrence_utc_usec"] == value[prefix + "at_utc_usec"],
        "progression raw producer/occurrence")
    if kind == 15:
        references = (source_reference(value), ownership_reference(value))
        _require(all(key == (0, 0, 0) or key[2] < row["record_seq"] for key in references),
            "progression raw forward reference")
        _require(not value["pctx_flags"] & CONFIG_CATALOG_KNOWN or
            value["pctx_configuration_record_seq"] < row["record_seq"],
            "progression raw forward configuration")
    else:
        root = value["pcfg_root_record_seq"]
        _require(root == row["record_seq"] if value["pcfg_chunk_index"] == 0
            else root < row["record_seq"], "XP configuration raw root identity")
    header = {"ingest_id", "boot_id", "process_id", "record_seq", "schema_version",
        "record_kind", "occurrence_utc_usec", "ingested_utc_usec"}
    _require(all(item is None for name, item in row.items() if name not in fields and name not in header),
        "progression raw inactive family payload")
    return value
