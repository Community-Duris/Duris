#!/usr/bin/env python3
"""Focused #267 progression fact, boundary and migration contract tests."""

from __future__ import annotations

import json
import hashlib
import copy
import argparse
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
HARNESS = ROOT / "tests" / "async" / "telemetry_progression_harness.cc"
PROGRESSION = SRC / "telemetry" / "telemetry_progression.c"
LIMITS = SRC / "world" / "limits.c"
MIGRATION = ROOT / "migrations" / "immutable" / "0022_telemetry_progression.sql"
VERIFIER = ROOT / "migrations" / "immutable" / "0022_telemetry_progression.sh"
sys.path.insert(0, str(ROOT))
from scripts.telemetry import progression_context_contract as context_contract
from scripts.telemetry import progression_publication as publication


def source_contract() -> None:
    limits = LIMITS.read_text()
    gain = limits[limits.index("int gain_exp(") : limits.index("int gain_condition(")]

    storage = gain.index("GET_EXP(ch) += (int)XP_final")
    capture = gain.index("telemetry_runtime_game_progression", storage)
    display = gain.index("display_gain(ch", capture)
    assert storage < capture < display
    assert gain.index("const int computed_xp") < gain.index("BOUNDED", gain.index("const int computed_xp"))
    assert gain.index("TELEMETRY_PROGRESSION_MODIFIER_FINAL_CAP") > gain.index("BOUNDED")
    assert "progression_source_for_type(type)" in gain
    assert "progression_reason_for_type(type)" in gain
    assert "static_cast<std::int64_t>(before_exp)" in gain
    assert "static_cast<std::int64_t>(after_exp)" in gain
    assert "lose_level_impl(ch," in gain
    assert "static_cast<std::uint64_t>(new_exp_table[GET_LEVEL(ch)])" in gain
    assert "progression_source_for_type(type), progression_reason_for_type(type)" in gain

    level = limits[limits.index("static void advance_level_impl") : limits.index("void clear_title")]
    assert level.count("telemetry_runtime_game_progression(") == 2
    assert "telemetry_progression_kind::level_advanced" in level
    assert "telemetry_progression_kind::level_lost" in level
    assert "telemetry_progression_reason::level_threshold" in level
    assert "telemetry_progression_reason::system_adjustment" in level
    assert "void lose_level(P_char ch)" in level

    fight = (SRC / "combat" / "fight.c").read_text()
    assert "telemetry_runtime_game_progression(" not in fight
    assert "telemetry_runtime_game_progression_adjustment(" in fight


def schema_contract() -> None:
    sql = MIGRATION.read_text()
    columns = (
        "progression_kind",
        "progression_source",
        "progression_reason",
        "progression_observation_status",
        "progression_modifier_flags",
        "progression_requested_xp",
        "progression_computed_xp",
        "progression_applied_xp",
        "progression_before_exp",
        "progression_after_exp",
        "progression_before_level",
        "progression_after_level",
        "progression_threshold_xp",
    )
    assert sql.count("ALTER TABLE telemetry_interval ADD COLUMN") == len(columns)
    for column in columns:
        assert f"column_name='{column}'" in sql
        assert f"ADD COLUMN {column} " in sql
        assert f"column_name='{column}'" in VERIFIER.read_text()
    assert "CREATE TABLE" not in sql.upper()
    assert "CREATE TRIGGER" not in sql.upper()

    manifest = json.loads((ROOT / "migrations" / "migration_manifest.json").read_text())
    head = next(item for item in manifest["migrations"]
                if item["id"] == "0022_telemetry_progression")
    assert head["id"] == "0022_telemetry_progression"
    assert head["sequence"] == 22
    assert head["apply"] == "immutable/0022_telemetry_progression.sql"
    assert head["verify"] == "immutable/0022_telemetry_progression.sh"
    assert set(head) >= {"apply_checksum", "verify_checksum"}
    assert head["apply_checksum"] != "0" * 64
    assert head["verify_checksum"] != "0" * 64


def context_values(binary: Path) -> None:
    wire = bytes.fromhex(subprocess.check_output([str(binary), "--export-context"], text=True).strip())
    original = context_contract.decode_observation(wire)
    assert context_contract.encode_observation(original) == wire
    assert len(wire) == 373 and len(original) == 82
    raw = raw_context(original, 15, 1000)
    assert context_contract.validate_raw_observation(raw) == original
    for name, value in (("record_kind", 16), ("schema_version", 2), ("record_seq", 0),
            ("record_seq", original["pctx_source_record_seq"]), ("process_id", 999),
            ("occurrence_utc_usec", original["pctx_at_utc_usec"] + 1),
            ("progression_applied_xp", 0), ("pcfg_version", 1)):
        rejected_raw(dict(raw, **{name: value}))

    def check(candidate, expected):
        raw = b"".join(candidate[name].to_bytes(width, "big", signed=signed)
            for name, width, signed in context_contract.FIELD_LAYOUT)
        try:
            context_contract.validate_observation(candidate)
            accepted = True
        except context_contract.ContextContractError:
            accepted = False
        assert accepted == expected, "Python progression context qualification"
        native = subprocess.check_output([str(binary), "--verify-context", raw.hex()], text=True).strip()
        assert native == str(int(expected)), "native progression context qualification"

    check(original, True)
    catalog = dict(original, pctx_flags=original["pctx_flags"] | context_contract.CONFIG_CATALOG_KNOWN,
        pctx_configuration_record_seq=5, pctx_configuration_digest_0=123)
    check(catalog, True)
    check(dict(catalog, pctx_configuration_record_seq=0), False)
    check(dict(catalog, pctx_configuration_digest_0=0), False)
    check(dict(original, pctx_configuration_record_seq=5), False)
    policy = dict(original, pctx_flags=original["pctx_flags"] | context_contract.DECISION_POLICY_KNOWN,
        pctx_decision_level_cap=50, pctx_decision_good_assistance_gap=15,
        pctx_decision_evil_assistance_gap=15, pctx_decision_max_exp_level=46)
    check(policy, True)
    check(dict(original, pctx_decision_level_cap=50), False)
    check(dict(policy, pctx_flags=policy["pctx_flags"] | context_contract.HARDCORE_BYPASS), False)
    check(dict(policy, pctx_flags=policy["pctx_flags"] | context_contract.HARDCORE |
        context_contract.HARDCORE_BYPASS), True)
    # Source keys, identity anchors, versions and family semantics cannot be
    # promoted by a correctly shaped wire packet.
    for name, value in (
        ("pctx_boot_id", 0), ("pctx_sequence", 0), ("pctx_subject_id", 0), ("pctx_pid", -1),
        ("pctx_connection_process_id", 999), ("pctx_source_process_id", 999),
        ("pctx_source_record_seq", 0), ("pctx_ownership_boot_id", 0),
        ("pctx_version", 2), ("pctx_source_inventory_version", 2),
        ("pctx_content_version", 0), ("pctx_threshold_level", 12),
        ("pctx_threshold_catalog_version", 0), ("pctx_next_threshold_xp", 0),
        ("pctx_eligible_group_size", 0), ("pctx_highest_group_level", 9),
        ("pctx_formal_group_size", 0), ("pctx_boundary", 1), ("pctx_source_kind", 14),
        ("pctx_rested_selection", 3), ("pctx_rested_application", 4),
        ("pctx_assistance", 0), ("pctx_flags", 1 << 31), ("pctx_quality_flags", 1 << 31),
    ):
        check(dict(original, **{name: value}), False)

    # An inert ordinary effect remains visible. Staff grants can override the
    # automatic gate; wellrested selection takes precedence; resurrection is
    # explicitly exempt even when an active bonus is present.
    flags = original["pctx_flags"] & ~context_contract.AUTOMATIC_RESTED
    inert = dict(original, pctx_flags=flags, pctx_rested_selection=1, pctx_rested_application=2)
    check(inert, True)
    check(dict(original, pctx_flags=flags | context_contract.RESTED_STAFF), True)
    check(dict(inert, pctx_flags=(flags | context_contract.RESTED_STAFF) & ~context_contract.ALIVE), True)
    well = dict(original, pctx_flags=original["pctx_flags"] | context_contract.WELLRESTED_PRESENT,
        pctx_rested_selection=3, pctx_rested_application=4)
    check(well, True)
    check(dict(well, pctx_rested_application=1), True)
    check(dict(inert, pctx_flags=flags | context_contract.WELLRESTED_STAFF), False)

    # Presence counts and eligible shares are separate: this observation has
    # three formal members and two eligible recipients. A solo native share
    # also needs its explicit eligibility inputs.
    check(dict(original, pctx_assistance=1, pctx_eligible_group_size=1,
        pctx_highest_group_level=10), True)
    check(dict(original, pctx_assistance=5, pctx_eligible_group_size=2,
        pctx_highest_group_level=0), True)
    check(dict(original, pctx_assistance=5, pctx_eligible_group_size=1,
        pctx_highest_group_level=0), False)

    # Signed observed XP and stat extremes survive exactly. These values still
    # confer no save/durable award status.
    extreme = dict(original, pctx_current_exp=-(1 << 63), pctx_base_stat_0=-32768,
        pctx_effective_stat_9=32767)
    check(extreme, True)
    unknown_owner = dict(original, pctx_account_token=0, pctx_ownership_boot_id=0,
        pctx_ownership_process_id=0, pctx_ownership_record_seq=0)
    check(unknown_owner, True)
    check(dict(unknown_owner, pctx_account_token=77), False)
    reversed_clock = dict(original, pctx_start_utc_usec=1_700_001)
    check(reversed_clock, False)
    check(dict(reversed_clock, pctx_quality_flags=128), True)
    check(dict(original, pctx_at_utc_usec=context_contract.UTC_UNKNOWN), True)

    interval = dict(original, pctx_boundary=2, pctx_source_kind=1, pctx_start_usec=1200,
        pctx_start_utc_usec=1_699_966, pctx_rested_application=0,
        pctx_flags=(original["pctx_flags"] & ~(context_contract.APPLICATION_KNOWN |
            context_contract.STORAGE_GATE_KNOWN | context_contract.STORAGE_GATE_PASSED)) |
            context_contract.CONTIGUOUS_EXPOSURE)
    check(interval, True)
    check(dict(interval, pctx_start_usec=1234), False)
    check(dict(interval, pctx_boundary=6, pctx_quality_flags=1), False)
    check(dict(interval, pctx_boundary=6, pctx_quality_flags=1,
        pctx_flags=interval["pctx_flags"] & ~context_contract.CONTIGUOUS_EXPOSURE), True)
    for invalid_wire in (wire[:-1], wire + b"\0"):
        try:
            context_contract.decode_observation(invalid_wire)
        except context_contract.ContextContractError:
            pass
        else:
            raise AssertionError("invalid progression context wire length accepted")
    print("progression context native/Python source, eligibility, clock and uncertainty cases passed")


def configuration_values(binary: Path) -> None:
    digest, encoded = subprocess.check_output([str(binary), "--export-configuration"], text=True).splitlines()
    wire = bytes.fromhex(encoded)
    row = context_contract.decode_configuration(wire, 55)
    assert len(wire) == 3729 and row["count"] == 337
    assert context_contract.encode_configuration(row) == wire
    assert context_contract.configuration_digest(row) == digest
    other = copy.deepcopy(row)
    other["config_id"] = 99
    assert context_contract.configuration_digest(other) == digest
    other["content_version"] += 1
    assert context_contract.configuration_digest(other) != digest
    invalids = []
    for field, value in (("version", 2), ("source_inventory_version", 2),
            ("count", 336), ("config_id", 0), ("build_version", True)):
        invalids.append(dict(row, **{field: value}))
    for index, field, value in ((0, "id", 2), (62, "bits", 0x7f800000),
            (62, "bits", 0x7fc00000), (62, "bits", 1 << 32), (327, "bits", 0x7ff0000000000000),
            (327, "kind", 3), (0, "bits", -1), (0, "bits", 1 << 64), (0, "bits", True)):
        other = copy.deepcopy(row)
        other["values"][index][field] = value
        invalids.append(other)
    for invalid in invalids:
        try:
            context_contract.validate_configuration(invalid)
        except context_contract.ContextContractError:
            pass
        else:
            raise AssertionError("invalid XP configuration accepted")
    for invalid in (wire[:-1], wire + b"\0"):
        try:
            context_contract.decode_configuration(invalid, 55)
        except context_contract.ContextContractError:
            pass
        else:
            raise AssertionError("unbounded XP configuration accepted")
    print("XP configuration native/Python exact 337-value canonical source and digest passed")


def configuration_chunk_values(binary: Path) -> None:
    wires = [bytes.fromhex(line) for line in subprocess.check_output(
        [str(binary), "--export-configuration-chunks"], text=True).splitlines()]
    assert len(wires) == 15 and all(len(wire) == 396 for wire in wires)
    packets = [context_contract.decode_configuration_chunk(wire) for wire in wires]
    keys = [(11, 22, 5 + index) for index in range(15)]
    for packet, key in zip(packets, keys):
        assert context_contract.validate_raw_observation(raw_context(packet, 16, key[2])) == packet
    rejected_raw(raw_context(packets[0], 16, 6))
    rejected_raw(raw_context(packets[1], 16, 5))
    rejected_raw(dict(raw_context(packets[1], 16, 6), pctx_version=1))
    combined = context_contract.reconcile_configuration_chunks(packets, keys)
    digest, _ = subprocess.check_output([str(binary), "--export-configuration"], text=True).splitlines()
    assert context_contract.configuration_digest(combined) == digest
    assert combined["values"][-1]["id"] == 610
    assert context_contract.reconcile_configuration_chunks(packets[::-1], keys[::-1]) == combined
    for wire, packet in zip(wires, packets):
        assert context_contract.encode_configuration_chunk(packet) == wire

    # Earlier definitions must keep their selected source and metrics while
    # advancing over valid new families in the immutable mixed-kind stream.
    from scripts.telemetry.rollup_engine import build_page_contributions, SemanticError
    from scripts.telemetry.rollup_definitions import RollupTarget
    context = context_contract.decode_observation(bytes.fromhex(subprocess.check_output(
        [str(binary), "--export-context"], text=True).strip()))
    raw_rows = [dict(raw_context(packet, 16, 5 + index), ingest_id=index + 1)
        for index, packet in enumerate(packets)]
    raw_rows.append(dict(raw_context(context, 15, 1000), ingest_id=16))
    for definition in (1, 2, 3, 5, 6, 7, 8):
        target = RollupTarget(definition, 1, packets[0]["pcfg_environment_id"], packets[0]["pcfg_season_id"])
        page = build_page_contributions(raw_rows, target, max_page_bytes=4 * 1024 * 1024)
        assert page.cursor == 16 and not page.sessions and not page.player_days and not page.cohorts
        assert not page.identity_inputs and not page.battle_inputs and page.output_fanout == 0
        assert page.state_quality_flags == 0
    invalid = dict(raw_rows[-1], pcfg_version=1)
    try:
        build_page_contributions([invalid], RollupTarget(3, 1, 1, 1))
    except SemanticError:
        pass
    else:
        raise AssertionError("sealed identity cursor skipped an invalid new payload")

    def check(candidate, expected):
        raw = b"".join(candidate[name].to_bytes(width, "big", signed=signed)
            for name, width, signed in context_contract.CONFIGURATION_CHUNK_LAYOUT)
        try:
            context_contract.validate_configuration_chunk(candidate)
            accepted = True
        except context_contract.ContextContractError:
            accepted = False
        assert accepted == expected
        native = subprocess.check_output([str(binary), "--verify-configuration-chunk", raw.hex()], text=True).strip()
        assert native == str(int(expected)), "native XP configuration chunk validation"

    for field, value in (("pcfg_boot_id", 0), ("pcfg_sequence", 0), ("pcfg_root_record_seq", 0),
            ("pcfg_config_id", 0), ("pcfg_environment_id", 0), ("pcfg_build_version", 0),
            ("pcfg_total_values", 336), ("pcfg_chunk_count", 16), ("pcfg_chunk_index", 15),
            ("pcfg_version", 2), ("pcfg_source_inventory_version", 2), ("pcfg_value_count", 23),
            ("pcfg_value_id_0", 2), ("pcfg_value_kind_0", 3)):
        check(dict(packets[0], **{field: value}), False)
    check(dict(packets[14], pcfg_value_id_1=610), False)
    check(dict(packets[14], pcfg_value_bits_23=1), False)
    check(dict(packets[2], pcfg_value_bits_14=0x7f800000), False)
    check(dict(packets[0], pcfg_at_utc_usec=context_contract.UTC_UNKNOWN), True)
    bad_sets = [(packets[:-1], keys[:-1]), (packets + [packets[0]], keys + [keys[0]])]
    for field in ("pcfg_sequence", "pcfg_at_usec", "pcfg_config_id", "pcfg_environment_id", "pcfg_content_version", "pcfg_digest_0"):
        changed = copy.deepcopy(packets)
        changed[1][field] += 1
        bad_sets.append((changed, keys))
    changed = copy.deepcopy(packets)
    changed[0]["pcfg_value_bits_0"] += 1
    bad_sets.append((changed, keys))
    changed_keys = list(keys)
    changed_keys[1], changed_keys[2] = changed_keys[2], changed_keys[1]
    bad_sets.append((packets, changed_keys))
    bad_sets.append((packets, [keys[0], (99, 22, 6), *keys[2:]]))
    for candidate, candidate_keys in bad_sets:
        try:
            context_contract.reconcile_configuration_chunks(candidate, candidate_keys)
        except context_contract.ContextContractError:
            pass
        else:
            raise AssertionError("incomplete/mixed/reordered/conflicting XP catalog accepted")
    print("XP configuration exact 15-chunk source/replay/version/digest reconciliation passed")


def raw_context(value, kind, receipt):
    prefix = "pctx_" if kind == 15 else "pcfg_"
    return dict(value, boot_id=value[prefix + "boot_id"], process_id=value[prefix + "process_id"],
        record_seq=receipt, schema_version=1, record_kind=kind,
        occurrence_utc_usec=value[prefix + "at_utc_usec"], ingested_utc_usec=123456)


def rejected_raw(row):
    try:
        context_contract.validate_raw_observation(row)
    except context_contract.ContextContractError:
        return
    raise AssertionError("unbound or inactive progression raw payload accepted")


def retained_context_values(binary: Path):
    """Exact retained value fixtures; no actual native gameplay qualification."""
    packets = [context_contract.decode_configuration_chunk(bytes.fromhex(line)) for line in
        subprocess.check_output([str(binary), "--export-configuration-chunks"], text=True).splitlines()]
    value = context_contract.decode_observation(bytes.fromhex(subprocess.check_output(
        [str(binary), "--export-context"], text=True).strip()))
    configuration = context_contract.reconcile_configuration_chunks(packets, [(11, 22, 5 + i) for i in range(15)])
    general = {name: packets[0]["pcfg_" + name] for name in publication.CONFIG_COLUMNS}
    scope = (9, 1, general["environment_id"], general["season_id"])
    value.update({"pctx_" + name: general[name] for name in publication.CONFIG_COLUMNS})
    value.update(pctx_boot_id=11, pctx_process_id=22, pctx_connection_boot_id=11, pctx_connection_process_id=22,
        pctx_source_boot_id=11, pctx_source_process_id=22, pctx_source_record_seq=101,
        pctx_source_kind=6, pctx_boundary=3, pctx_ownership_boot_id=11, pctx_ownership_process_id=22,
        pctx_ownership_record_seq=100, pctx_configuration_record_seq=5)
    value.update({"pctx_configuration_digest_" + str(i): packets[0]["pcfg_digest_" + str(i)] for i in range(4)})
    value["pctx_flags"] |= context_contract.CONFIG_CATALOG_KNOWN
    threshold = next(entry for entry in configuration["values"] if entry["id"] == value["pctx_threshold_level"])
    value["pctx_next_threshold_xp"] = int.from_bytes(threshold["bits"].to_bytes(8, "big"), "big", signed=True)
    context_contract.validate_observation(value)
    rows = [dict(raw_context(packet, 16, 5 + index), ingest_id=index + 1) for index, packet in enumerate(packets)]

    def old_point(kind, receipt, ingest):
        row = dict.fromkeys(publication.SOURCE_COLUMNS[kind], 0)
        row.update({name: value["pctx_" + name] for name in (*publication.observations.SESSION,
            "subject_id", "pid", "environment_id", "season_id", "config_id", "classifier_version", "policy_version")})
        row.update(ingest_id=ingest, boot_id=11, process_id=22, record_seq=receipt,
            schema_version=1, record_kind=kind, zone_vnum=-1, ingested_utc_usec=123456,
            at_monotonic_usec=value["pctx_at_usec"], at_utc_usec=value["pctx_at_utc_usec"],
            occurrence_utc_usec=value["pctx_at_utc_usec"])
        row.update({"connection_" + name: value["pctx_connection_" + name] for name in ("boot_id", "process_id", "seq")})
        return row

    owner = old_point(9, 100, 16)
    owner.update(ownership_account_token=value["pctx_account_token"],
                 ownership_source=1 if value["pctx_account_token"] else 5)
    xp = old_point(6, 101, 17)
    xp.update(progression_kind=1, progression_source=5, progression_reason=1,
        progression_observation_status=1, progression_before_level=value["pctx_current_level"],
        progression_after_level=value["pctx_current_level"], progression_before_exp=value["pctx_current_exp"] - 125,
        progression_after_exp=value["pctx_current_exp"], progression_requested_xp=1000,
        progression_computed_xp=2000, progression_applied_xp=125)
    rows += [owner, xp, dict(raw_context(value, 15, 102), ingest_id=18)]
    from scripts.telemetry.rollup_definitions import RollupTarget
    from scripts.telemetry.rollup_engine import build_page_contributions
    selected = build_page_contributions(rows, RollupTarget(*scope), start_cursor=0)
    assert len(selected.progression_inputs) == 18 and not selected.sessions and not selected.observations.progression

    def resolve(candidates, *, retain_general=True, max_bytes=publication.DEFAULT_BYTE_LIMIT):
        inputs = [publication.retain_input(row, scope, 0, configuration=general if retain_general and
            row["record_kind"] in (15, 16) else None) for row in candidates]
        header = publication.advance_header(publication.initial_header(scope), inputs, 18)
        window = publication.verify_source(header, inputs, expected_scope=scope, max_total_bytes=max_bytes)
        return publication.resolve_contexts(window), header, inputs

    resolved, header, inputs = resolve(rows)
    assert len(resolved) == 1 and resolved[0].source == xp and resolved[0].ownership == owner
    assert resolved[0].configuration == configuration and resolved[0].unknown == ()
    assert header["input_origin"] == 0 and header["input_watermark"] == 18 and header["source_fact_count"] == 18
    assert header["configuration_chunk_count"] == 15 and header["context_count"] == 1
    for index, unknown in ((15, "missing_ownership"), (16, "missing_source"), (4, "incomplete_configuration")):
        candidate = rows[:index] + rows[index+1:]
        assert unknown in resolve(candidate)[0][0].unknown
    assert "unretained_general_configuration" in resolve(rows, retain_general=False)[0][0].unknown

    def rejected(function):
        try:
            function()
        except publication.PublicationError:
            return
        raise AssertionError("conflicting retained progression source accepted")

    rejected(lambda: resolve(rows, max_bytes=publication.HEADER_BYTE_BOUND))
    for index, field, replacement in ((15, "ownership_account_token", owner["ownership_account_token"] + 1),
            (16, "subject_id", xp["subject_id"] + 1), (16, "at_monotonic_usec", xp["at_monotonic_usec"] + 1),
            (16, "connection_boot_id", xp["connection_boot_id"] + 1),
            (16, "connection_process_id", xp["connection_process_id"] + 1),
            (16, "connection_seq", xp["connection_seq"] + 1),
            (16, "progression_after_exp", xp["progression_after_exp"] + 1),
            (4, "pcfg_config_id", packets[4]["pcfg_config_id"] + 1)):
        candidate = copy.deepcopy(rows)
        candidate[index][field] = replacement
        rejected(lambda: resolve(candidate))
    rejected(lambda: publication.retain_input(dict(rows[-1], progression_applied_xp=0), scope, 0, configuration=general))
    rejected(lambda: publication.verify_source(header, inputs[:-1], expected_scope=scope))
    changed = copy.deepcopy(inputs)
    changed[-1]["payload"] += b" "
    rejected(lambda: publication.verify_source(header, changed, expected_scope=scope))
    rejected(lambda: publication.verify_source(header, list(reversed(inputs)), expected_scope=scope))
    # Exact roots/owners can predate a rebuild's origin. Retain those original
    # receipts independently; do not invent cursor facts to make them fit.
    selected = inputs[-1:]
    narrow = publication.advance_header(publication.initial_header(scope, 17), selected, 18)
    bound = publication.bind_references(narrow, selected, inputs[:-1])
    referenced = publication.verify_source(bound, selected, references=inputs[:-1], expected_scope=scope)
    assert publication.resolve_contexts(referenced)[0].unknown == ()
    assert bound["source_fact_count"] == 1 and bound["reference_count"] == 17
    assert bound["input_origin"] == 17 and bound["input_watermark"] == 18
    assert len(referenced.inputs) == 18 and len(referenced.reference_receipts) == 17
    rejected(lambda: publication.verify_source(bound, selected, references=inputs[:-2], expected_scope=scope))
    rejected(lambda: publication.bind_references(narrow, selected, list(reversed(inputs[:-1]))))
    unrelated = dict(xp, record_seq=777, ingest_id=19)
    rejected(lambda: publication.bind_references(narrow, selected, [*inputs[:-1], publication.retain_input(unrelated, scope, 0)]))
    # Synthetic value journeys qualify censoring and dated effort arithmetic;
    # the maintained actual-server journey is separate gameplay evidence.
    origin_utc = 1_000_000_000_000
    sequence = [200]

    def next_key():
        sequence[0] += 1
        return sequence[0]

    def fixture_point(kind, at, subject=None):
        row = old_point(kind, next_key(), 1)
        row.update(at_monotonic_usec=at, at_utc_usec=origin_utc + at, occurrence_utc_usec=origin_utc + at)
        if subject is not None:
            row.update(subject_id=subject, pid=subject, session_seq=subject, connection_seq=subject)
        return row

    def fixture_context(level, at, boundary, source=None):
        v = dict(value)
        decision_flags = (context_contract.APPLICATION_KNOWN | context_contract.ASSISTANCE_KNOWN |
            context_contract.GROUP_ELIGIBILITY_KNOWN | context_contract.STORAGE_GATE_KNOWN |
            context_contract.STORAGE_GATE_PASSED | context_contract.DECISION_POLICY_KNOWN |
            context_contract.HARDCORE | context_contract.HARDCORE_BYPASS | context_contract.CONTIGUOUS_EXPOSURE)
        v["pctx_flags"] &= ~decision_flags
        for field in ("rested_application", "assistance", "eligible_group_size", "highest_group_level", "assistance_level",
                      "decision_level_cap", "decision_good_assistance_gap", "decision_evil_assistance_gap", "decision_max_exp_level"):
            v["pctx_" + field] = 0
        v.update(pctx_current_level=level, pctx_starting_level=10, pctx_threshold_level=level+1,
            pctx_at_usec=at, pctx_start_usec=at, pctx_at_utc_usec=origin_utc+at, pctx_start_utc_usec=origin_utc+at,
            pctx_boundary=boundary, pctx_ownership_record_seq=anchor["record_seq"], pctx_quality_flags=0)
        v.update({"pctx_source_" + name: 0 if source is None else source[name] for name in publication.REPLAY})
        v["pctx_source_kind"] = 0 if source is None else source["record_kind"]
        v["pctx_next_threshold_xp"] = next(entry["bits"] for entry in configuration["values"] if entry["id"] == level+1)
        key = next_key()
        v["pctx_sequence"] = key
        return raw_context(v, 15, key)

    def fixture_interval(first, last, category=2, subject=None):
        row = {name: owner.get(name, 0) for name in publication.SOURCE_COLUMNS[1]}
        row.update(ingest_id=1, record_kind=1, record_seq=next_key(), start_monotonic_usec=first,
            end_monotonic_usec=last, duration_usec=last-first, start_utc_usec=origin_utc+first,
            end_utc_usec=origin_utc+last, occurrence_utc_usec=origin_utc+last, category=category)
        if subject is not None:
            row.update(subject_id=subject, pid=subject, session_seq=subject)
        return row

    def fixture_level(before, at):
        row = fixture_point(6, at)
        row.update(progression_kind=2, progression_source=5, progression_reason=4, progression_observation_status=1,
            progression_before_level=before, progression_after_level=before+1,
            progression_threshold_xp=next(entry["bits"] for entry in configuration["values"] if entry["id"] == before+1))
        return row

    anchor = fixture_point(9, 0)
    anchor.update(ownership_account_token=owner["ownership_account_token"], ownership_source=owner["ownership_source"])
    level_11, level_12 = fixture_level(10, 10), fixture_level(11, 25)
    journey = [*rows[:15], anchor, fixture_context(10, 0, 1), fixture_interval(0, 10), level_11,
        fixture_context(11, 10, 4, level_11), fixture_interval(10, 20), fixture_interval(20, 25, 1), level_12,
        fixture_context(12, 25, 4, level_12), fixture_interval(25, 40)]
    reviewed = {"registry_schema_version": 8, "status": "reviewed_inventory", "registry_version": 1,
        "reviewed_from_utc_usec": origin_utc-1, "reviewed_through_utc_usec": origin_utc+1000, "incidents": []}

    def evidence(candidates, review=reviewed, registry=None, *, publish=False, **kwargs):
        candidates = sorted(copy.deepcopy(candidates), key=lambda row: row["record_seq"])
        for index, row in enumerate(candidates, 1):
            row["ingest_id"] = index
        retained = [publication.retain_input(row, scope, 0, configuration=general if row["record_kind"] in (15, 16) else None)
                    for row in candidates]
        current = publication.advance_header(publication.initial_header(scope), retained, len(candidates))
        window = publication.verify_source(current, retained, expected_scope=scope)
        return (publication.build_publication if publish else publication.build_evidence)(window, registry, review, **kwargs)

    proof = evidence(journey)
    assert len(proof.milestones) == 3
    first, complete, unfinished = proof.milestones
    assert first["status"] == "observed_completion" and first["left_censored"] and first["already_past_through_level"] == 10
    assert first["full_stage_elapsed_usec"] is None
    assert complete["status"] == "observed_completion" and not complete["left_censored"] and not complete["right_censored"]
    assert complete["unknown"] == [] and complete["full_stage_elapsed_usec"] == 15
    assert complete["full_stage_connected_usec"] == 15 and complete["full_stage_heuristic_active_usec"] == 10
    assert unfinished["status"] == "unfinished" and unfinished["right_censored"] and unfinished["observed_elapsed_usec"] == 15
    assert unfinished["full_stage_elapsed_usec"] is None and all(row["time_to_level_median"] is None for row in proof.milestones)
    missing = [row for row in journey if not (row["record_kind"] == 1 and row["start_monotonic_usec"] == 20)]
    partial = evidence(missing).milestones[1]
    assert partial["missing_interval_usec"] == 5 and "missing_intervals" in partial["unknown"]
    assert partial["full_stage_connected_usec"] is None and partial["observed_connected_usec"] == 10
    interval_configuration = copy.deepcopy(journey)
    next(row for row in interval_configuration if row["record_kind"] == 1 and row["start_monotonic_usec"] == 10)["config_id"] += 1
    changed_stage = evidence(interval_configuration).milestones[1]
    assert "interval_configuration_change" in changed_stage["unknown"] and changed_stage["full_stage_elapsed_usec"] is None
    assert changed_stage["observed_connected_usec"] == 15
    partial_context = copy.deepcopy(journey)
    next(row for row in partial_context if row["record_kind"] == 15 and row["pctx_current_level"] == 11)["pctx_quality_flags"] |= 1 << 6
    assert "context_quality" in evidence(partial_context).milestones[1]["unknown"]
    assert evidence(partial_context).milestones[1]["full_stage_elapsed_usec"] is None
    missing_baseline = [row for row in journey if not (row["record_kind"] == 15 and row["pctx_boundary"] == 1)]
    first_retained = evidence(missing_baseline).milestones[0]
    assert first_retained["left_censored"] and "missing_baseline" in first_retained["unknown"]
    assert first_retained["already_past_through_level"] == 11
    absent_review = dict(reviewed, status="not_published", registry_version=None)
    assert "unreviewed_loss_inventory" in evidence(journey, absent_review).milestones[1]["unknown"]
    outside = dict(reviewed, reviewed_through_utc_usec=origin_utc+20)
    assert "loss_review_clock_or_range" in evidence(journey, outside).milestones[1]["unknown"]
    clock_cut = copy.deepcopy(journey)
    next(row for row in clock_cut if row["record_kind"] == 1 and row["start_monotonic_usec"] == 10)["end_utc_usec"] += 1
    assert "clock_ambiguity" in evidence(clock_cut).milestones[1]["unknown"]
    no_level_context = [row for row in journey if not (row["record_kind"] == 15 and row["pctx_boundary"] == 4 and row["pctx_current_level"] == 11)]
    assert any("missing_progression_context" in row["unknown"] for row in evidence(no_level_context).milestones)
    administration = copy.deepcopy(journey)
    next(row for row in administration if row["record_seq"] == level_12["record_seq"]).update(progression_source=11, progression_reason=5)
    assert evidence(administration).milestones[1]["status"] == "censored"
    native_thresholds = copy.deepcopy(journey)
    for row in native_thresholds:
        if row["record_kind"] == 6 and row["progression_kind"] == 2:
            row["progression_source"] = 12
    assert evidence(native_thresholds).milestones[1]["status"] == "observed_completion"
    assert evidence(native_thresholds).milestones[1]["full_stage_elapsed_usec"] == 15
    for field, reason in (("rested_selection", "observed_rested_selection_change"),
                          ("formal_group_size", "observed_formal_group_size_change")):
        changed_selection = copy.deepcopy(journey)
        endpoint = next(row for row in changed_selection if row["record_kind"] == 15 and row["pctx_current_level"] == 12)
        endpoint["pctx_" + field] = 1 if endpoint["pctx_" + field] != 1 else 2
        if field == "rested_selection":
            endpoint["pctx_flags"] &= ~(context_contract.RESTED_PRESENT | context_contract.WELLRESTED_PRESENT |
                context_contract.RESTED_STAFF | context_contract.WELLRESTED_STAFF)
            if endpoint["pctx_rested_selection"] == 2:
                endpoint["pctx_flags"] |= context_contract.RESTED_PRESENT | context_contract.RESTED_STAFF
        changed_stage = evidence(changed_selection).milestones[1]
        assert reason in changed_stage["unknown"] and changed_stage["full_stage_elapsed_usec"] is None
    overlap = [*journey, fixture_interval(18, 22)]
    rejected(lambda: evidence(overlap))
    rejected(lambda: evidence(journey, max_output_rows=2))

    # Same authenticated account, separate characters; no controller is inferred.
    other = value["pctx_subject_id"] + 1
    second_owner = fixture_point(9, 49, other)
    second_owner.update(ownership_account_token=anchor["ownership_account_token"], ownership_source=anchor["ownership_source"])
    switched = evidence([*journey, second_owner, fixture_interval(50, 60, subject=other)])
    account = next(row for row in switched.rotations if row["basis"] == "account" and row["effort_category"] == "heuristic_active")
    assert account["observed_characters"] == 2 and account["summed_character_usec"] == 45
    assert account["covered_union_usec"] == 45 and account["observed_simultaneous_session_usec"] == 0
    assert account["sequential_switches"] == [dict(from_character=value["pctx_subject_id"], to_character=other,
        previous_observed_through_utc_usec=origin_utc+40, next_observed_from_utc_usec=origin_utc+50,
        elapsed_gap_usec=10, gap_activity_observed=False)]
    assert any(row["basis"] == "unknown_controller" and row["sequential_switches"] is None and
               row["union_usec"] is None for row in switched.rotations)
    second_owner["at_monotonic_usec"], second_owner["at_utc_usec"] = 29, origin_utc+29
    second_owner["occurrence_utc_usec"] = second_owner["at_utc_usec"]
    concurrent = evidence([*journey, second_owner, fixture_interval(30, 50, subject=other)])
    account = next(row for row in concurrent.rotations if row["basis"] == "account" and row["effort_category"] == "heuristic_active")
    assert account["summed_character_usec"] == 55 and account["covered_union_usec"] == 45
    assert account["observed_simultaneous_character_usec"] == 10 and account["maximum_observed_simultaneous_sessions"] == 2
    assert account["sequential_switches"] == [] and account["ambiguous_region_transitions"] == 1
    lifecycle = fixture_point(2, 35)
    lifecycle.update(lifecycle_kind=2, end_reason=1)
    native_lifecycle = dict(lifecycle)
    native_lifecycle["lifecycle"] = native_lifecycle.pop("lifecycle_kind")
    assert publication.retain_input(native_lifecycle, scope, 0) == publication.retain_input(lifecycle, scope, 0)
    rejected(lambda: publication.retain_input(dict(native_lifecycle, lifecycle_kind=2), scope, 0))
    closed = evidence([*journey, lifecycle]).milestones[-1]
    assert closed["status"] == "censored" and closed["observed_elapsed_usec"] == 10
    assert "missing_lifecycle_context" in closed["unknown"]
    lifecycle_context = fixture_context(12, 35, 5, lifecycle)
    closed = evidence([*journey, lifecycle, lifecycle_context]).milestones[-1]
    assert closed["status"] == "censored" and "lifecycle_cut" in closed["unknown"]
    unsupported = copy.deepcopy(journey)
    next(row for row in unsupported if row["record_seq"] == level_12["record_seq"])["progression_observation_status"] = 3
    assert "unsupported_progression_durability" in evidence(unsupported).milestones[1]["unknown"]
    assert evidence(unsupported).milestones[1]["full_stage_elapsed_usec"] is None
    rejected(lambda: evidence(journey, max_total_bytes=publication.HEADER_BYTE_BOUND))

    packet = dict(registry_schema_version=1, environment_id=scope[2], season_id=scope[3], registry_version=1,
        previous_registry_version=0, previous_packet_digest=None, reviewed_from_utc_usec=origin_utc-1,
        reviewed_through_utc_usec=origin_utc+1000, reviewed_at_utc_usec=origin_utc+1000,
        reviewer_token="b"*64, review_evidence_digest="a"*64, associations=[dict(association_id=1,
            account_token=anchor["ownership_account_token"], controller_token=500, valid_from_utc_usec=origin_utc-1,
            valid_through_utc_usec=origin_utc+32, status="confirmed", provenance="staff_review", evidence_digest="e"*64)])
    registry = publication.identity.Registry.from_packet(packet)
    dated = evidence([*journey, second_owner, fixture_interval(30, 50, subject=other)], registry=registry)
    controller = next(row for row in dated.rotations if row["basis"] == "controller" and row["effort_category"] == "heuristic_active")
    assert controller["summed_character_usec"] == 29 and controller["covered_union_usec"] == 27
    assert controller["observed_simultaneous_character_usec"] == 2
    assert any(row["basis"] == "unknown_controller" for row in dated.rotations)

    def fixture_xp(at, source, reason, applied, requested=None, computed=None):
        row = fixture_point(6, at)
        row.update(progression_kind=1, progression_source=source, progression_reason=reason, progression_observation_status=1,
            progression_before_level=11, progression_after_level=11, progression_before_exp=value["pctx_current_exp"]-applied,
            progression_after_exp=value["pctx_current_exp"], progression_requested_xp=applied if requested is None else requested,
            progression_computed_xp=applied if computed is None else computed, progression_applied_xp=applied)
        return row, fixture_context(11, at, 3, row)

    awards = [row for pair in (fixture_xp(15, 5, 1, 125, 1000, 2000), fixture_xp(16, 4, 2, -50),
        fixture_xp(17, 6, 3, 40), fixture_xp(18, 10, 4, -25), fixture_xp(19, 11, 5, 500),
        fixture_xp(20, 12, 6, -20)) for row in pair]
    progress = evidence([*journey, *awards])
    character = [row for row in progress.portfolio if row["basis"] == "character"]
    earned = next(row for row in character if row["source"] == 5 and row["reason"] == 1)
    assert (earned["requested_xp"], earned["computed_xp"], earned["observed_applied_xp"]) == (1000, 2000, 125)
    assert sum(row["observed_earned_positive_xp"] for row in character) == 125
    assert sum(row["death_loss_xp"] for row in character) == 50
    assert sum(row["restored_positive_xp"] for row in character) == 40
    assert sum(row["administrative_positive_xp"] for row in character) == 500
    assert sum(row["system_negative_xp"] for row in character) == 20
    assert all(row["observed_earned_xp_per_heuristic_active_hour"] is None and not row["XP_award_committed"] and
               not row["character_save_committed"] and row["economic_authority_dependency_issue"] == 487 for row in character)
    assert "no_contiguous_stratum_exposure" in earned["unknown"]
    all_missing = evidence([row for row in [*journey, *awards] if row["record_kind"] != 15])
    assert len(all_missing.milestones) == 1 and all_missing.milestones[0]["status"] == "unknown_context"
    assert all_missing.milestones[0]["starting_level"] is None and "missing_baseline" in all_missing.milestones[0]["unknown"]
    assert next(row for row in character if row["source"] == 10 and row["reason"] == 4)["threshold_consumed_xp"] == 25
    no_context = [row for row in [*journey, *awards] if not (row["record_kind"] == 15 and row["pctx_at_usec"] == 15)]
    unknown_earned = next(row for row in evidence(no_context).portfolio if row["basis"] == "character" and row["source"] == 5 and row["reason"] == 1)
    assert unknown_earned["observed_earned_positive_xp"] == 125 and "missing_progression_context" in unknown_earned["unknown"]
    # An explicit contiguous exposure value is necessary even in fixtures.
    # This checks the pure rate gate; native exposure capture is not implied.
    active_interval = next(row for row in journey if row["record_kind"] == 1 and row["start_monotonic_usec"] == 10)
    exposure = fixture_context(11, 20, 2, active_interval)
    exposure.update(pctx_start_usec=10, pctx_start_utc_usec=origin_utc+10)
    exposure["pctx_flags"] |= context_contract.CONTIGUOUS_EXPOSURE
    idle_interval = next(row for row in journey if row["record_kind"] == 1 and row["start_monotonic_usec"] == 20)
    idle_exposure = fixture_context(11, 25, 2, idle_interval)
    idle_exposure.update(pctx_start_usec=20, pctx_start_utc_usec=origin_utc+20)
    idle_exposure["pctx_flags"] |= context_contract.CONTIGUOUS_EXPOSURE
    # Other stages are explicitly different level strata, rather than omitted.
    other_exposures = []
    for interval in journey:
        if interval["record_kind"] != 1 or interval["start_monotonic_usec"] in (10, 20):
            continue
        stage = 10 if interval["start_monotonic_usec"] == 0 else 12
        packet = fixture_context(stage, interval["end_monotonic_usec"], 2, interval)
        packet.update(pctx_start_usec=interval["start_monotonic_usec"], pctx_start_utc_usec=interval["start_utc_usec"])
        packet["pctx_flags"] |= context_contract.CONTIGUOUS_EXPOSURE
        other_exposures.append(packet)
    exposed = evidence([*journey, *awards, exposure, idle_exposure, *other_exposures])
    # Definition 9 keeps native interval connection keys, including a wide
    # SQL row whose other record-family fields are NULL. A different transport
    # cannot satisfy an otherwise identical exposure's original source link.
    from scripts.telemetry.db_access import RAW_SOURCE_COLUMNS
    native_interval = dict.fromkeys(RAW_SOURCE_COLUMNS)
    native_interval.update(active_interval)
    native_page = build_page_contributions([native_interval], RollupTarget(*scope))
    assert publication.decode_input(native_page.progression_inputs[0], scope).source == active_interval
    for field in ("connection_boot_id", "connection_process_id", "connection_seq"):
        mismatched = copy.deepcopy([*journey, *awards, exposure, idle_exposure, *other_exposures])
        next(row for row in mismatched if row["record_kind"] == 1 and row["record_seq"] == active_interval["record_seq"])[field] += 1
        rejected(lambda: evidence(mismatched))
    rejected(lambda: publication.retain_input(dict(active_interval, connection_seq=0), scope, 0))
    rate = next(row for row in exposed.portfolio if row["basis"] == "character" and row["source"] == 5 and row["reason"] == 1)
    assert rate["unknown"] == [] and rate["covered_connected_union_usec"] == 15 and rate["covered_heuristic_active_union_usec"] == 10
    assert rate["observed_earned_xp_per_heuristic_active_hour"] == dict(numerator_xp=125, denominator_usec=10, scale_usec_per_hour=3_600_000_000)
    assert not rate["assistance_duration_implied"] and not rate["rested_application_duration_implied"]
    public_rows = publication.evidence_rows(scope, exposed)
    assert {row["row_kind"] for row in public_rows} == {1, 2, 3, 4, 5}
    public_values = [publication.decode_row(scope, row) for row in public_rows]
    assert len(public_values) == len(public_rows) and all(len(row["payload"]) <= publication.MAX_PAYLOAD_BYTES for row in public_rows)
    rejected(lambda: publication.decode_row((9, 2, *scope[2:]), public_rows[0]))
    rejected(lambda: publication.decode_row(scope, dict(public_rows[0], payload=public_rows[0]["payload"]+b" ")))
    rejected(lambda: publication.encode_row(scope, 1, dict(exposed.contexts[0], character_save_committed=True)))
    rejected(lambda: publication.encode_row(scope, 2, dict(exposed.milestones[-1], time_to_level_median=15)))
    rejected(lambda: publication.encode_row(scope, 5, dict(exposed.portfolio[0], universal_progression_score=100)))
    rejected(lambda: publication.encode_row(scope, 5, dict(exposed.portfolio[0], denominator_scope="assistance_duration")))
    rejected(lambda: publication.evidence_rows(scope, publication.replace(exposed, contexts=(*exposed.contexts, exposed.contexts[0]))))
    complete_output = evidence([*journey, *awards, exposure, idle_exposure, *other_exposures], publish=True)
    row_receipts = [{name: row[name] for name in publication.SNAPSHOT_COLUMNS} for row in complete_output.rows]
    reservation = publication.identity.generation_row(scope, None)
    public_coverage = publication.public_header(complete_output.header, reservation, row_receipts)
    assert public_coverage["publication_complete"] == 1 and public_coverage["qualified_full_stage_count"] == 1
    assert public_coverage["completed_milestone_count"] == 2 and public_coverage["unfinished_milestone_count"] == 1
    assert not public_coverage["character_save_committed"] and public_coverage["identity"]["registry_version"] is None
    rejected(lambda: publication.public_header(complete_output.header, reservation, row_receipts[:-1]))
    rejected(lambda: publication.public_header(dict(complete_output.header, publication_complete=0), reservation, row_receipts))
    rejected(lambda: publication.public_header(complete_output.header, dict(reservation, generation=2), row_receipts))
    changed_receipts = copy.deepcopy(row_receipts)
    changed_receipts[-1]["payload_digest"] = bytes(32)
    rejected(lambda: publication.public_header(complete_output.header, reservation, changed_receipts))
    print("milestone completed/unfinished/left cuts, missing intervals, clocks, review limits, sequential/overlap/unknown rotation values passed")
    print("dated controller/lifecycle cuts, unsupported durability, XP units, exact rebuild references, rate gates and typed row readback passed")
    print("retained progression exact XP/ownership/catalog references, missing evidence, replay, digest, scope and capacity passed")
    return dict(scope=scope, general=general, journey=copy.deepcopy(journey), reviewed=reviewed)


def qualify_publication(binary, query, environment, command, database):
    """Disposable value fixtures exercise the real cursor/publication/report TX."""
    import pymysql
    from scripts.telemetry.db_access import (AmbiguousCommit, ConnectionSettings, PyMySQLConnectionFactory,
        PyMySQLRollupDatabase, RAW_COLUMNS)
    from scripts.telemetry.rollup_definitions import RollupTarget
    from scripts.telemetry.rollup_engine import BoundsExceeded, RollupBounds, RollupEngine
    fixture = retained_context_values(binary)
    general = fixture["general"]
    migration = ROOT / "migrations/immutable/0072_telemetry_progression_publication.sql"
    verifier = ROOT / "migrations/immutable/0072_telemetry_progression_publication.sh"
    subprocess.run(command + [database], input=migration.read_bytes(), env=environment, check=True)
    subprocess.run(["bash", str(verifier)], env=environment, check=True)
    config_columns = [row["name"] for row in query("SELECT column_name AS name FROM information_schema.columns "
        "WHERE table_schema=DATABASE() AND table_name='telemetry_config' ORDER BY ordinal_position")]
    configuration = dict.fromkeys(config_columns, 1)
    configuration.update(general, fingerprint=b"c" * 32, effective_utc_usec=123456,
        interval_usec=100, checkpoint_interval_usec=200, active_window_usec=100, pulse_slot_count=256,
        context_segments_per_minute=64, backend=1)
    query("INSERT INTO telemetry_config (" + ",".join(config_columns) + ") VALUES (" +
        ",".join(["%s"] * len(config_columns)) + ")", tuple(configuration[name] for name in config_columns))
    origin = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
    for row in fixture["journey"]:
        if row["record_kind"] == 16:
            continue  # Exact original catalog remains before the rebuild origin.
        header = {name: value for name, value in row.items() if name in RAW_COLUMNS and name != "ingest_id"}
        query("INSERT INTO telemetry_interval (" + ",".join(header) + ") VALUES (" +
            ",".join(["%s"] * len(header)) + ")", tuple(header.values()))
        if row["record_kind"] == 15:
            payload = {name: value for name, value in row.items() if name not in ("ingest_id", "ingested_utc_usec")}
            query("INSERT INTO telemetry_progression_context (" + ",".join(payload) + ") VALUES (" +
                ",".join(["%s"] * len(payload)) + ")", tuple(payload.values()))
    through = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
    token = hashlib.sha256(database.encode()).hexdigest()[:10]
    password = "synthetic-progression-publication-" + token
    private = ("telemetry_progression_source_v9", "telemetry_progression_input_v9", "telemetry_progression_reference_v9")
    public = ("telemetry_rollup_progression_coverage_v9", "telemetry_rollup_progression_row_v9")
    review_public = ("telemetry_rollup_incident_coverage", "telemetry_rollup_incident")
    grants = {
        "rollup": {"telemetry_rollup_state": "SELECT,INSERT,UPDATE", "telemetry_generation_identity": "SELECT,INSERT",
            **{table: "SELECT" for table in ("telemetry_interval", "telemetry_progression_context",
                "telemetry_progression_configuration", "telemetry_config", "telemetry_identity_registry", "telemetry_identity_association",
                "telemetry_incident_registry_v8", "telemetry_incident_v8")},
            **{table: "SELECT,INSERT" for table in (*public, *review_public, private[1], private[2])}, private[0]: "SELECT,INSERT,UPDATE"},
        "review": {table: "SELECT,INSERT" for table in ("telemetry_incident_registry_v8", "telemetry_incident_v8")},
        "report": {table: "SELECT" for table in ("telemetry_rollup_state", "telemetry_generation_identity", *public, *review_public)},
    }
    users, adapters = [], []
    try:
        for role, tables in grants.items():
            user = "tpp_" + role + "_" + token
            query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
            users.append(user)
            for table, permissions in tables.items():
                query(f"GRANT {permissions} ON `{database}`.`{table}` TO %s@'%%'", (user,))
        def adapter(role):
            settings = ConnectionSettings(host=environment["DB_HOST"], port=int(environment["DB_PORT"]), database=database,
                user="tpp_" + role + "_" + token, password=password)
            result = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings))
            adapters.append(result)
            return result
        rollup, reviewer, reporter = adapter("rollup"), adapter("review"), adapter("report")
        packet = publication.incident.template(8)
        packet.update(environment_id=general["environment_id"], season_id=general["season_id"], reviewer_token="a" * 64,
            review_evidence_digest="b" * 64, reviewed_from_utc_usec=0,
            reviewed_through_utc_usec=fixture["reviewed"]["reviewed_through_utc_usec"], incidents=[])
        assert reviewer.register_incident_packet(packet)["status"] == "registered"
        bounds = RollupBounds(page_size=4, max_runtime_s=20, max_output_fanout=512)
        target = RollupTarget(9, 1, general["environment_id"], general["season_id"])
        rollup.reserve_identity_generation(target.scope_tuple, None)
        assert RollupEngine(rollup).run(target, origin_ingest_id=origin, through_ingest_id=through, bounds=bounds).final_cursor == through
        header = query("SELECT * FROM telemetry_progression_source_v9 WHERE generation=1")[0]
        assert header["source_fact_count"] == sum(row["record_kind"] != 16 for row in fixture["journey"])
        assert header["reference_count"] == 0 and not header["publication_complete"]
        insert = rollup._insert_review_rows
        def fail_after_header(table, columns, rows, **kwargs):
            if table == public[1]:
                raise RuntimeError("injected progression publication child failure")
            return insert(table, columns, rows, **kwargs)
        rollup._insert_review_rows = fail_after_header
        try:
            rollup.publish_generation(target, bounds=bounds)
        except RuntimeError as error:
            assert str(error) == "injected progression publication child failure"
        else:
            raise AssertionError("injected progression publication did not abort")
        finally:
            rollup._insert_review_rows = insert
        assert query("SELECT COUNT(*) AS n FROM " + public[0])[0]["n"] == 0
        assert query("SELECT COUNT(*) AS n FROM " + private[2])[0]["n"] == 0
        assert query("SELECT * FROM " + private[0] + " WHERE generation=1")[0] == header
        commit = rollup._commit
        lost = [True]
        def lost_committed_ack():
            commit()
            if lost[0]:
                lost[0] = False
                raise AmbiguousCommit("injected lost progression publication acknowledgement")
        rollup._commit = lost_committed_ack
        try:
            assert rollup.publish_generation(target, bounds=bounds)["status"] == "published"
        finally:
            rollup._commit = commit
        assert not lost[0]
        assert rollup.publish_generation(target, bounds=bounds)["status"] == "published"
        published = reporter.read_coverage(target).public_dict()["progression_coverage"]
        assert published["reference_count"] == 15 and published["context_row_count"] == 3
        assert published["completed_milestone_count"] == 2 and published["unfinished_milestone_count"] == 1
        assert published["qualified_full_stage_count"] == 1
        snapshots = {name: reporter.read_report(target, name) for name in publication.ROW_KINDS}
        assert all(snapshot.coverage.progression_coverage == published for snapshot in snapshots.values())
        assert len(snapshots["progression_milestones"].rows) == 3
        assert any(row["full_stage_elapsed_usec"] == 15 for row in snapshots["progression_milestones"].rows)
        assert not any(row["XP_award_committed"] for row in snapshots["progression_context"].rows)
        assert reporter.read_report(target, "progression_milestones", max_rows=1).truncated
        try:
            reporter.read_report(target, "progression_milestones", max_bytes=publication.REPORT_METADATA_BYTE_BOUND)
        except BoundsExceeded:
            pass
        else:
            raise AssertionError("progression report omitted its row/sentinel reservation")
        for table in (*private, "telemetry_interval", "telemetry_progression_context", "telemetry_progression_configuration", "telemetry_config"):
            principal = reporter.connection_factory.connect()
            try:
                with principal.cursor() as cursor:
                    try:
                        cursor.execute("SELECT * FROM " + table + " LIMIT 1")
                    except pymysql.err.OperationalError as error:
                        assert error.args[0] == 1142
                    else:
                        raise AssertionError("public report accessed private progression " + table)
            finally:
                reporter.connection_factory.close(principal)
        immutable_updates = 0
        for table in (*private, *public):
            row = query("SELECT * FROM " + table + " WHERE generation=1 LIMIT 1")[0]
            for column, value in row.items():
                if value is None:
                    continue
                replacement = b"z" * len(value) if type(value) is bytes else value + 1
                try:
                    query("UPDATE " + table + " SET " + column + "=%s WHERE generation=1", (replacement,))
                except (pymysql.err.OperationalError, pymysql.err.IntegrityError) as error:
                    assert error.args[0] in (1644, 3819, 4025, 1451, 1062)
                    immutable_updates += 1
                else:
                    raise AssertionError("progression publication accepted altered " + table + "." + column)
            try:
                query("DELETE FROM " + table + " WHERE generation=1")
            except pymysql.err.OperationalError as error:
                assert error.args[0] == 1644
            else:
                raise AssertionError("progression retained deletion accepted")
        second = RollupTarget(9, 2, target.environment_id, target.season_id)
        rollup.reserve_identity_generation(second.scope_tuple, None)
        RollupEngine(rollup).run(second, origin_ingest_id=origin, through_ingest_id=through, bounds=bounds)
        lost = [True]
        def lost_rolled_back_ack():
            if lost[0]:
                lost[0] = False
                rollup._rollback()
                raise AmbiguousCommit("injected rolled back progression publication acknowledgement")
            commit()
        rollup._commit = lost_rolled_back_ack
        try:
            assert rollup.publish_generation(second, bounds=bounds)["status"] == "published"
        finally:
            rollup._commit = commit
        assert not lost[0] and reporter.read_coverage(target).publication_status == 2
        assert list(reporter.read_report(target, "progression_milestones").rows) == list(snapshots["progression_milestones"].rows)
        retained_snapshot = query("SELECT * FROM " + public[1] + " ORDER BY generation,row_kind,row_key")
        subprocess.run(command + [database], input=migration.read_bytes(), env=environment, check=True)
        subprocess.run(["bash", str(verifier)], env=environment, check=True)
        assert query("SELECT * FROM " + public[1] + " ORDER BY generation,row_kind,row_key") == retained_snapshot
        # Retained snapshots and private input verification survive raw retention.
        query("DELETE FROM telemetry_interval WHERE ingest_id <= %s", (through,))
        assert list(reporter.read_report(target, "progression_milestones").rows) == list(snapshots["progression_milestones"].rows)
        assert rollup.read_progression_source(target).header["reference_count"] == 15
        return dict(status="passed", original_configuration_references=15, atomic_child_failure_rollback=True,
            lost_committed_ack_recovered=True, lost_rolled_back_ack_recovered=True, immutable_updates=immutable_updates,
            restricted_public_readback=True, old_generation_preserved=True, rerun_with_data=True, raw_retention_readback=True,
            source_fact_count=header["source_fact_count"], publication=published,
            migration_sha256=hashlib.sha256(migration.read_bytes()).hexdigest(),
            verifier_sha256=hashlib.sha256(verifier.read_bytes()).hexdigest())
    finally:
        for db in adapters:
            db.close()
        for user in users:
            query("DROP USER IF EXISTS %s@'%%'", (user,))


def qualify_storage(binary: Path) -> None:
    """Native value fixtures qualify SQL shape; actual gameplay is a separate journey."""
    if os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") != "1":
        raise RuntimeError("explicit disposable progression storage qualification required")
    import pymysql
    from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture
    from scripts.telemetry.db_access import RAW_SELECT_COLUMNS, RAW_PAYLOAD_JOIN

    environment, command, database = prepare_sql_fixture()
    connection = None
    migration = ROOT / "migrations/immutable/0071_telemetry_progression_context.sql"
    verifier = ROOT / "migrations/immutable/0071_telemetry_progression_context.sh"
    try:
        connection = pymysql.connect(host=environment["DB_HOST"], port=int(environment["DB_PORT"]),
            user=environment["DB_USER"], password=environment["DB_PASSWD"], database=database,
            autocommit=True, cursorclass=pymysql.cursors.DictCursor,
            connect_timeout=3, read_timeout=10, write_timeout=10)

        def query(sql, parameters=None):
            with connection.cursor() as cursor:
                cursor.execute(sql, parameters)
                return list(cursor.fetchall()) if cursor.description else []

        def sealed_metadata():
            return query("SELECT * FROM information_schema.triggers WHERE trigger_schema=DATABASE() "
                "AND trigger_name NOT LIKE 'telemetry_progression_%' ORDER BY trigger_name"), query(
                "SELECT * FROM information_schema.columns WHERE table_schema=DATABASE() "
                "AND (table_name IN ('telemetry_incident_registry_v7','telemetry_incident_v7',"
                "'telemetry_identity_input','telemetry_rollup_identity_coverage','telemetry_rollup_portfolio_xp') "
                "OR (table_name='telemetry_interval' AND column_name NOT LIKE 'pctx_%' AND column_name NOT LIKE 'pcfg_%')) "
                "ORDER BY table_name,ordinal_position")

        before = sealed_metadata()
        # The complete immutable chain installs the new source stores. Reapply
        # their migration in this owned fixture to qualify rerun with data;
        # report definition 9 is not activated by source schema registration.
        subprocess.run(command + [database], input=migration.read_bytes(), env=environment, check=True)
        subprocess.run(["bash", str(verifier)], env=environment, check=True)
        assert sealed_metadata() == before, "progression source changed a sealed schema or trigger"
        packets = [context_contract.decode_configuration_chunk(bytes.fromhex(line)) for line in
            subprocess.check_output([str(binary), "--export-configuration-chunks"], text=True).splitlines()]
        value = context_contract.decode_observation(bytes.fromhex(subprocess.check_output(
            [str(binary), "--export-context"], text=True).strip()))
        rows = [raw_context(packet, 16, 5 + index) for index, packet in enumerate(packets)]
        rows.append(raw_context(value, 15, 1000))

        def read_source():
            return query("SELECT " + ",".join(RAW_SELECT_COLUMNS) + RAW_PAYLOAD_JOIN + " ORDER BY r.ingest_id")

        def insert_header(row):
            header = tuple(name for name in row if not name.startswith(("pctx_", "pcfg_")))
            query("INSERT INTO telemetry_interval (" + ",".join(header) + ") VALUES (" +
                ",".join(["%s"] * len(header)) + ")", tuple(row[name] for name in header))

        def insert_payload(row):
            names = tuple(name for name in row if name != "ingested_utc_usec")
            table = "telemetry_progression_context" if row["record_kind"] == 15 else "telemetry_progression_configuration"
            query("INSERT INTO " + table + " (" + ",".join(names) + ") VALUES (" +
                ",".join(["%s"] * len(names)) + ")", tuple(row[name] for name in names))

        for row in rows:
            context_contract.validate_raw_observation(row)
            query("START TRANSACTION")
            insert_header(row)
            insert_payload(row)
            query("COMMIT")
        retained = read_source()
        assert [context_contract.validate_raw_observation(row) for row in retained] == packets + [value]
        assert context_contract.reconcile_configuration_chunks(
            [context_contract.validate_raw_observation(row) for row in retained[:-1]],
            [(row["boot_id"], row["process_id"], row["record_seq"]) for row in retained[:-1]]) == \
            context_contract.reconcile_configuration_chunks(packets, [(11, 22, 5 + i) for i in range(15)])
        negative_count = 0

        def reject_update(receipt, name, value):
            nonlocal negative_count
            table = "telemetry_progression_context" if receipt == 1000 else "telemetry_progression_configuration"
            try:
                query("UPDATE " + table + " SET " + name + "=%s WHERE record_seq=%s", (value, receipt))
            except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                assert error.args[0] in (1644, 3819, 4025), (name, error.args[0])
            else:
                raise AssertionError("progression SQL accepted invalid " + name)
            negative_count += 1

        for layout, receipt in ((context_contract.FIELD_LAYOUT, 1000),
                (context_contract.CONFIGURATION_CHUNK_LAYOUT, 5)):
            for name, _width, _signed in layout:
                reject_update(receipt, name, None)
        for receipt, changes in ((1000, (("pctx_version", 2), ("pctx_source_inventory_version", 2),
                ("pctx_boot_id", 99), ("pctx_at_utc_usec", 0), ("pctx_flags", 1 << 21),
                ("pctx_quality_flags", 1024), ("pctx_current_level", 0), ("pctx_source_kind", 14),
                ("record_kind", 6))),
                (5, (("pcfg_version", 2), ("pcfg_root_record_seq", 6), ("pcfg_total_values", 336),
                ("pcfg_chunk_count", 16), ("pcfg_value_count", 23), ("pcfg_boot_id", 99),
                ("pcfg_at_utc_usec", 0), ("record_kind", 15)))):
            for name, replacement in changes:
                reject_update(receipt, name, replacement)
        for row in rows:
            try:
                query("UPDATE telemetry_interval SET progression_applied_xp=0 WHERE record_seq=%s", (row["record_seq"],))
            except pymysql.err.OperationalError as error:
                assert error.args[0] == 1644
            else:
                raise AssertionError("typed progression receipt accepted another raw family")
            negative_count += 1
        assert read_source() == retained
        fresh_receipt = 2000
        rejected_inserts = 0

        def fresh_row(kind):
            nonlocal fresh_receipt
            fresh_receipt += 1
            row = dict(rows[-1] if kind == 15 else rows[0], record_seq=fresh_receipt)
            prefix = "pctx_" if kind == 15 else "pcfg_"
            row[prefix + "sequence"] = fresh_receipt
            if kind == 16:
                row["pcfg_root_record_seq"] = fresh_receipt
            context_contract.validate_raw_observation(row)
            return row

        def source_exists(row):
            return query("SELECT COUNT(*) AS n FROM telemetry_interval WHERE boot_id=%s AND process_id=%s "
                "AND record_seq=%s", tuple(row[name] for name in ("boot_id", "process_id", "record_seq")))[0]["n"]

        def reject_insert(kind, changes, *, parent=True, parent_changes=None):
            nonlocal rejected_inserts
            row = fresh_row(kind)
            query("START TRANSACTION")
            try:
                if parent:
                    insert_header(dict(row, **(parent_changes or {})))
                insert_payload(dict(row, **changes))
            except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                assert error.args[0] in (1048, 1644, 3819, 4025), (changes, error.args[0])
            else:
                raise AssertionError("progression SQL accepted fresh invalid source " + str(changes))
            finally:
                query("ROLLBACK")
            assert source_exists(row) == 0, "failed payload left a committed header"
            rejected_inserts += 1

        for kind, layout in ((15, context_contract.FIELD_LAYOUT), (16, context_contract.CONFIGURATION_CHUNK_LAYOUT)):
            for name, _width, _signed in layout:
                reject_insert(kind, {name: None})
            reject_insert(kind, {}, parent=False)
            reject_insert(kind, {}, parent_changes={"occurrence_utc_usec": 1})
            reject_insert(kind, {}, parent_changes={"record_kind": 16 if kind == 15 else 15})
        for name, replacement in (("pctx_version", 2), ("pctx_source_inventory_version", 2),
                ("pctx_boot_id", 99), ("pctx_at_utc_usec", 0), ("pctx_flags", 1 << 21),
                ("pctx_quality_flags", 1024), ("pctx_current_level", 0), ("pctx_source_kind", 14)):
            reject_insert(15, {name: replacement})
        for name, replacement in (("pcfg_version", 2), ("pcfg_root_record_seq", 0),
                ("pcfg_total_values", 336), ("pcfg_chunk_count", 16), ("pcfg_value_count", 23),
                ("pcfg_boot_id", 99), ("pcfg_at_utc_usec", 0)):
            reject_insert(16, {name: replacement})

        for kind in (15, 16):
            row = fresh_row(kind)
            query("START TRANSACTION")
            insert_header(row)
            insert_payload(row)
            assert source_exists(row) == 1
            query("ROLLBACK")
            assert source_exists(row) == 0
            insert_header(row)  # Simulate an independently committed incomplete receipt.
            incomplete = next(item for item in read_source() if item["record_seq"] == row["record_seq"])
            rejected_raw(incomplete)
            query("DELETE FROM telemetry_interval WHERE record_seq=%s", (row["record_seq"],))
            query("START TRANSACTION")
            insert_header(row)
            insert_payload(row)
            query("COMMIT")
            table = "telemetry_progression_context" if kind == 15 else "telemetry_progression_configuration"
            query("DELETE FROM telemetry_interval WHERE record_seq=%s", (row["record_seq"],))
            assert query("SELECT COUNT(*) AS n FROM " + table + " WHERE record_seq=%s", (row["record_seq"],))[0]["n"] == 0
        assert read_source() == retained
        # Rerun with data present and verify exact metadata, values and original
        # pre-existing trigger bodies rather than accepting CREATE IF NOT EXISTS.
        subprocess.run(command + [database], input=migration.read_bytes(), env=environment, check=True)
        subprocess.run(["bash", str(verifier)], env=environment, check=True)
        assert sealed_metadata() == before
        assert read_source() == retained
        # Measure the exact maintained runtime metadata query on each engine.
        # The current fingerprint is a runtime contract, never a guessed hash
        # of migration text. This fixture has no production credentials.
        runtime = json.loads((ROOT / "migrations/runtime_compatibility_manifest.json").read_text())
        verification = (ROOT / "migrations/verify_runtime_compatibility.sh").read_text()
        begin = verification.index('query="SELECT CONCAT(\'T\'')
        end = verification.index('fingerprint=$(', begin)
        measurement = ('set -euo pipefail\nexport MYSQL_PWD="$DB_PASSWD"\nMYSQL=(' +
            shlex.join(command + ["--raw", database]) + ')\nruntime_tables=' +
            shlex.quote(runtime["runtime_table_sql_list"]) + '\n' + verification[begin:end] +
            '\n"${MYSQL[@]}" -e "$query" | sha256sum | cut -d " " -f1\n')
        fingerprint = subprocess.check_output(["bash", "-c", measurement], env=environment, text=True).strip()
        assert len(fingerprint) == 64 and all(character in "0123456789abcdef" for character in fingerprint)
        receipt = dict(status="passed", evidence="native value fixtures; no actual gameplay or save claim",
            engine=os.environ.get("TELEMETRY_REPOSITORY_DB_IMAGE"), raw_columns=len(retained[0]),
            physical_raw_columns=len(query("SELECT * FROM telemetry_interval LIMIT 1")[0]),
            context_count=1, configuration_chunks=15, rejected_updates=negative_count,
            rejected_fresh_inserts=rejected_inserts, atomic_rollback=True,
            missing_typed_payload_refused=True, retention_cascade=True,
            normalized_metadata_fingerprint=fingerprint,
            migration_count=int(environment["TELEMETRY_REPOSITORY_MIGRATION_COUNT"]),
            migration_head=environment["TELEMETRY_REPOSITORY_MIGRATION_HEAD"],
            rerun_with_data=True, sealed_metadata_unchanged=True,
            migration_sha256=hashlib.sha256(migration.read_bytes()).hexdigest(),
            verifier_sha256=hashlib.sha256(verifier.read_bytes()).hexdigest())
        receipt["progression_publication"] = qualify_publication(binary, query, environment, command, database)
        output = os.environ.get("TELEMETRY_PROGRESSION_STORAGE_RESULT")
        if output:
            Path(output).write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(receipt, sort_keys=True), flush=True)
    finally:
        if connection:
            connection.close()
        drop_sql_fixture(environment, command, database)


def compile_and_run(*, sanitize=False, sql_fixture=False) -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix="telemetry-progression-") as directory:
        binary = Path(directory) / "telemetry_progression"
        command = compiler + [
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
            "-pthread",
            "-I",
            str(SRC),
            str(HARNESS),
            str(PROGRESSION),
            str(SRC / "telemetry/telemetry_config.c"),
            "-lcrypto",
            "-o",
            str(binary),
        ]
        if sanitize:
            command[1:1] = ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                "-fno-pie", "-no-pie"]
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True, timeout=10)
        context_values(binary)
        configuration_values(binary)
        configuration_chunk_values(binary)
        retained_context_values(binary)
        if sql_fixture:
            qualify_storage(binary)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--sql-fixture", action="store_true")
    args = parser.parse_args()
    source_contract()
    schema_contract()
    compile_and_run(sanitize=args.sanitize, sql_fixture=args.sql_fixture)
    print("telemetry progression source, schema and arithmetic contracts passed")


if __name__ == "__main__":
    main()
