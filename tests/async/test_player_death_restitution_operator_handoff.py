#!/usr/bin/env python3
"""Exercise the operator export through the production staff command path."""

from _paths import ROOT

import hashlib
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


SCRIPT = ROOT / "scripts" / "player_death_restitution.py"


def run_cli(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if check and result.returncode != 0:
        raise AssertionError(f"CLI failed ({result.returncode}): {result.stderr}; stdout={result.stdout}")
    return result


def inspection_artifact(cli: Any, artifact: bool = False) -> dict[str, object]:
    name_hex = b"ordinary sword".hex()
    item = {
        "parent_index": 0,
        "equipment_slot": 0,
        "object_uid": 100,
        "generated_key": 100,
        "vnum": 104,
        "type": 1,
        "string_mask": 1,
        "name_hex": name_hex,
        "short_description_hex": "",
        "description_hex": "",
        "action_description_hex": "",
        "values": [0] * 8,
        "timers": [0] * 6,
        "wear_flags": 0,
        "extra_flags": 0,
        "weight": 4,
        "material": 2,
        "cost": 99,
        "condition": 100,
        "bitvectors": [0] * 5,
        "affects": [],
        "extra_descriptions": [],
        # This is the codec-produced standalone item evidence carried as the
        # native command's immutable original_payload.
        "item_payload_hex": "00",
    }
    body: dict[str, object] = {
        "artifact_version": 3,
        "kind": "death_restitution_inspection",
        "backend": "sql",
        "source": {
            "pid": 42,
            "death_revision": 7,
            "operation_id_hex": "20" * 16,
            "corpse_item_uid": 900,
            "corpse_room_vnum": 1234,
            "wallet_revision": 9,
            "wallet_before": [0, 0, 0, 0],
            "wallet_pile_uid": 0,
            "loss_epoch": 1700000000,
        },
        "recipient_pid": 42,
        "payload_hex": "00",
        "payload_digest": "11" * 32,
        "decoded": {
            "pid": 42,
            "revision": 7,
            "death": {
                "operation_id_hex": "20" * 16,
                "wallet_pile_uid": 0,
                "corpse": [
                    {"object_uid": 900, "parent_index": -1},
                    item,
                ],
            },
        },
        "custody_db": [{
            "pid": 42,
            "save_revision": 7,
            "item_uid": 100,
            "root_item_uid": 100,
            "parent_item_uid": 0,
            "expected_item_revision": 10,
            "vnum": 104,
            "expected_state": 1,
            "owner_type": 1,
            "owner_id": 42,
            "owner_context_id": 0,
            "owner_revision": 5,
        }],
        "current_owners": {"100": {
            "item_uid": 100,
            "root_item_uid": 100,
            "parent_item_uid": 0,
            "owner_type": 1,
            "owner_id": 42,
            "owner_context_id": 0,
            "item_revision": 11,
            "vnum": 104,
            "state": 3,
            "owner_revision": 6,
        }},
        "player_projections": [],
        "player_authority": {"42": {
            "pid": 42,
            "save_revision": 8,
            "owner_revision": 6,
        }},
        "recipient_existing_uids": [],
        "related_deaths": [],
        "related_custody": [],
        "item_loss_epochs": {"100": 1700000000},
        "deliveries": {},
        "artifacts": {
            "domain": {},
            "domain_table_present": False,
            "baseline": {},
            "baseline_table_present": False,
            "bind": {},
            "bind_table_present": False,
            "mortal": {},
            "god": {},
            "competitors": {},
        },
        "consistency_errors": [],
    }
    if artifact:
        item["name_hex"] = b"unique sword".hex()
        item["extra_flags"] = 1 << 28
        body["artifacts"] = {
            "domain": {"104": {
                "vnum": 104,
                "owned": 1,
                "loc_type": 5,
                "location": 42,
                "timer_epoch": 0,
                "artifact_type": 2,
                "bind_owner_pid": 42,
                "bind_timer_epoch": 0,
                "item_uid": 100,
                "item_revision": 11,
                "revision": 4,
            }},
            "domain_table_present": True,
            "baseline": {"104": {
                "vnum": 104,
                "opening_timer_epoch": 0,
                "opening_bind_owner_pid": 42,
                "opening_bind_timer_epoch": 0,
                "opening_revision": 4,
            }},
            "baseline_table_present": True,
            "bind": {"104": {"vnum": 104, "owner_pid": 42, "timer": 0}},
            "bind_table_present": True,
            "mortal": {"104": {
                "vnum": 104,
                "owned": "Y",
                "loc_type": 5,
                "location": 42,
                "timer": 0,
                "artifact_type": 2,
            }},
            "god": {},
            "competitors": {},
        }
    body["evidence_digest"] = cli.digest_json(body)
    return body


HARNESS = r'''
#include "core/config.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "persistence/critical_command.h"
#include "persistence/player_death_restitution_command.h"
#include "player/player_death_restitution_staff.h"
#include "player/player_death_restitution_adapter.h"
#include "player/player_save_pipeline.h"

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

P_desc descriptor_list = nullptr;
std::string last_message;

char *one_argument(const char *argument, char *first_arg)
{
    if (!argument || !first_arg)
        return nullptr;
    while (*argument && std::isspace(static_cast<unsigned char>(*argument)))
        ++argument;
    while (*argument && !std::isspace(static_cast<unsigned char>(*argument)))
        *first_arg++ = *argument++;
    *first_arg = 0;
    return const_cast<char *>(argument);
}

void send_to_char(const char *message, P_char)
{
    last_message = message ? message : "";
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
    std::abort();
}

namespace
{
bool target_fence_held = false;
int release_calls = 0;
}

bool is_pid_online(int pid, bool)
{
    assert(pid == 42);
    return false;
}

bool player_load_pipeline_pid_pending(int pid)
{
    assert(pid == 42);
    return false;
}

bool player_save_pipeline_target_save_pending(int pid)
{
    assert(pid == 42);
    return false;
}

bool player_save_pipeline_acquire_target_save_login_fence(int pid,
                                                           player_revision_t revision)
{
    assert(pid == 42 && revision == 8);
    if (target_fence_held)
        return false;
    target_fence_held = true;
    return true;
}

void player_save_pipeline_release_target_save_login_fence(int pid,
                                                           player_revision_t revision)
{
    assert(pid == 42 && revision == 8);
    ++release_calls;
    target_fence_held = false;
}

bool player_save_pipeline_save_admitted(int pid)
{
    assert(pid == 42 || pid == 43);
    return pid != 42 || !target_fence_held;
}

critical_submit_result critical_command_coordinator_submit(critical_command)
{
    return critical_submit_result::journal_uncertain;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const std::string encoded(argv[1]);
    assert(!encoded.empty());

    std::vector<uint8_t> encoded_bytes;
    for (size_t index = 0; index < encoded.size(); index += 2)
        encoded_bytes.push_back(static_cast<uint8_t>(std::stoul(encoded.substr(index, 2), nullptr, 16)));
    critical_command submitted = {};
    assert(critical_command_decode(encoded_bytes.data(), encoded_bytes.size(), &submitted) ==
           critical_command_codec_result::ok);
    player_death_restitution_plan decoded_plan = {};
    assert(player_death_restitution_command_decode_payload(submitted, &decoded_plan));
    critical_command generic_builder_command = {};
    assert(player_death_restitution_command_build(decoded_plan, &generic_builder_command));
    assert(generic_builder_command.source_site == critical_source_site::recovery);
    assert(generic_builder_command.deadline_class == critical_deadline_class::recovery);

    std::string wrong_source = encoded;
    wrong_source.replace(56, 4, "0500");
    assert(player_death_restitution_staff_submit_hex(
               "approved-staff", FORGER, wrong_source.data(), wrong_source.size(), nullptr) ==
           player_death_restitution_runtime_result::invalid_plan);
    std::string wrong_deadline = encoded;
    wrong_deadline.replace(60, 2, "04");
    assert(player_death_restitution_staff_submit_hex(
               "approved-staff", FORGER, wrong_deadline.data(), wrong_deadline.size(), nullptr) ==
           player_death_restitution_runtime_result::invalid_plan);

    char_data staff = {};
    staff.player.level = FORGER;
    staff.player.name = const_cast<char *>("approved-staff");
    const auto invoke_command = [&](const std::string &text) {
        std::vector<char> input(text.begin(), text.end());
        input.push_back(0);
        do_restitution(&staff, input.data(), CMD_RESTITUTION);
    };

    invoke_command("begin");
    assert(last_message.find("staging started") != std::string::npos);
    const size_t chunk_limit = PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNK_HEX;
    assert(chunk_limit == 1004);
    assert(PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNKS == 1045);
    assert(chunk_limit + std::string("restitution chunk ").size() < MAX_INPUT_LENGTH);
    for (size_t offset = 0; offset < encoded.size(); offset += chunk_limit)
    {
        const size_t chunk_size = std::min<size_t>(chunk_limit, encoded.size() - offset);
        const std::string line = "restitution chunk " + encoded.substr(offset, chunk_size);
        // Model the input-reader cap before command dispatch. No reframing to
        // smaller, workaround chunks may hide an invalid exporter contract.
        const std::string transported = line.substr(0, MAX_INPUT_LENGTH - 1);
        assert(transported == line);
        invoke_command(transported.substr(std::string("restitution ").size()));
        assert(last_message.find("chunk accepted") != std::string::npos);
    }
    invoke_command("commit");
    assert(last_message.find("journal-uncertain") != std::string::npos);
    assert(target_fence_held);

    assert(player_death_restitution_staff_submit_hex(
               "approved-staff", LESSER_G, encoded.data(), encoded.size(), nullptr) ==
           player_death_restitution_runtime_result::unauthorized);
    assert(target_fence_held);
    assert(player_death_restitution_staff_submit_hex(
               "other-staff", FORGER, encoded.data(), encoded.size(), nullptr) ==
           player_death_restitution_runtime_result::identity_conflict);
    assert(target_fence_held);

    critical_completion completion = {};
    completion.outcome = critical_apply_outcome::already_applied;
    player_death_restitution_runtime_handle_completions(&completion, 0);
    // The harness only proves the command admission/authentication path; the
    // operation remains fenced because no coordinator completion was supplied.
    assert(release_calls == 0);
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-restitution-handoff-") as temp_dir:
    temp = Path(temp_dir)
    inspect_path = temp / "inspection.json"
    plan_path = temp / "plan.json"
    export_path = temp / "staff-payload.json"
    import importlib.util

    spec = importlib.util.spec_from_file_location("player_death_restitution", SCRIPT)
    assert spec and spec.loader
    cli = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cli)
    cli.atomic_write_json(inspect_path, inspection_artifact(cli), overwrite=False)

    run_cli("plan", "--inspect", str(inspect_path), "--artifact", str(plan_path))
    run_cli(
        "export", "--plan", str(plan_path), "--inspect", str(inspect_path),
        "--artifact", str(export_path), "--approve", "--actor", "approved-staff",
        "--reason", "death_restitution",
    )
    payload = json.loads(export_path.read_text())
    plan = json.loads(plan_path.read_text())
    assert payload["format"] == "duris-player-death-restitution-staff-payload-v1"
    assert payload["source_site"] == "operator_repair"
    assert payload["deadline_class"] == "interactive"
    assert payload["actor"] == "approved-staff"
    assert payload["evidence_digest"] == json.loads(inspect_path.read_text())["evidence_digest"]
    assert "100" not in payload["artifact_timing_approvals"]
    assert "".join(payload["chunks"]) == payload["canonical_hex"]
    # Include the actual command prefix in the transport budget: comm.c keeps
    # MAX_INPUT_LENGTH - 1 characters, not that many payload characters.
    assert all(0 < len(chunk) <= 1004 and len(chunk) % 2 == 0
               and len("restitution chunk " + chunk) <= 1023
               for chunk in payload["chunks"])
    assert cli.STAFF_CHUNK_HEX == 1004
    assert (cli.STAFF_PAYLOAD_MAX_BYTES * 2 + cli.STAFF_CHUNK_HEX - 1) // cli.STAFF_CHUNK_HEX == 1045
    command = bytes.fromhex(payload["canonical_hex"])
    assert hashlib.sha256(command).hexdigest() == payload["command_digest"]
    # The command header is 92 bytes; the native plan places its two distinct
    # digests after the 88-byte scalar/identity prefix.
    native_plan_offset = 92
    digest_offset = native_plan_offset + 88
    assert struct.unpack_from("<H", command, native_plan_offset + 4)[0] == cli.NATIVE_COMMAND_PAYLOAD_VERSION
    assert command[digest_offset:digest_offset + 32] == bytes.fromhex(plan["evidence_digest"])
    assert command[digest_offset + 32:digest_offset + 64] == bytes.fromhex(plan["payload_digest"])
    assert command[digest_offset + 64:digest_offset + 96] == bytes.fromhex(plan["plan_digest"])
    approval_digest = payload.pop("approval_digest")
    assert cli.digest_json(payload) == approval_digest

    # Exercise production-classified native preparation through the real CLI,
    # without mocking plan reconstruction or command encoding. The synthetic
    # target has no connection: planning/export must consume only artifacts.
    native_inspect_path = temp / "native-inspection.json"
    native_plan_path = temp / "native-plan.json"
    native_export_path = temp / "native-export.json"
    native_target_path = temp / "native-target.json"
    native_inspection = inspection_artifact(cli)
    native_target = {
        "host": "no-database.invalid", "port": "3306", "database": "duris",
        "production": True, "server_fingerprint": "ab" * 32,
    }
    native_inspection.pop("evidence_digest")
    native_inspection["target"] = native_target
    native_inspection["evidence_digest"] = cli.digest_json(native_inspection)
    cli.atomic_write_json(native_inspect_path, native_inspection, overwrite=False)
    cli.atomic_write_json(native_target_path, cli.make_target_info(native_target, None), overwrite=False)
    refused = run_cli(
        "plan", "--inspect", str(native_inspect_path), "--native",
        "--approve-production", "--artifact", str(native_plan_path), check=False,
    )
    assert refused.returncode == 2 and not native_plan_path.exists()
    run_cli(
        "plan", "--inspect", str(native_inspect_path),
        "--target-info", str(native_target_path), "--preparation-mode", "native",
        "--artifact", str(native_plan_path),
    )
    unapproved = json.loads(native_plan_path.read_text())
    assert unapproved["eligible_count"] == 1 and unapproved["exportable"] is False
    refused = run_cli(
        "export", "--plan", str(native_plan_path), "--inspect", str(native_inspect_path),
        "--artifact", str(native_export_path), "--approve", "--actor", "approved-staff",
        "--reason", "death_restitution", check=False,
    )
    assert refused.returncode == 2 and not native_export_path.exists()
    run_cli(
        "plan", "--inspect", str(native_inspect_path),
        "--target-info", str(native_target_path), "--preparation-mode", "native",
        "--approve-production", "--artifact", str(native_plan_path), "--overwrite",
    )
    native_plan = json.loads(native_plan_path.read_text())
    assert native_plan["exportable"] is True and native_plan["applyable"] is False
    assert native_plan["target"] == native_target
    assert not native_plan.get("backup_receipt") and not native_plan.get("maintenance_boundary")
    run_cli(
        "export", "--plan", str(native_plan_path), "--inspect", str(native_inspect_path),
        "--artifact", str(native_export_path), "--approve", "--actor", "approved-staff",
        "--reason", "death_restitution",
    )
    native_payload = json.loads(native_export_path.read_text())
    assert native_payload["plan_digest"] == native_plan["plan_digest"]
    assert native_payload["evidence_digest"] == native_inspection["evidence_digest"]
    assert native_payload["restitution_id_hex"] == native_plan["restitution_id_hex"]
    assert native_payload["recipient_pid"] == native_plan["recipient_pid"]
    assert "".join(native_payload["chunks"]) == native_payload["canonical_hex"]
    assert all(len("restitution chunk " + chunk) <= 1023 for chunk in native_payload["chunks"])

    artifact_inspect_path = temp / "artifact-inspection.json"
    artifact_plan_path = temp / "artifact-plan.json"
    artifact_export_path = temp / "artifact-staff-payload.json"
    cli.atomic_write_json(artifact_inspect_path, inspection_artifact(cli, artifact=True), overwrite=False)
    run_cli(
        "plan", "--inspect", str(artifact_inspect_path), "--artifact", str(artifact_plan_path),
        "--artifact-timing-compensation", "100=3600:approval-ticket-100",
    )
    run_cli(
        "export", "--plan", str(artifact_plan_path), "--inspect", str(artifact_inspect_path),
        "--artifact", str(artifact_export_path), "--approve", "--actor", "approved-staff",
        "--reason", "death_restitution",
    )
    artifact_payload = json.loads(artifact_export_path.read_text())
    assert artifact_payload["artifact_timing_approvals"] == {"100": {
        "item_uid": 100,
        "basis": "approved_compensation",
        "approved": True,
        "usable_lifetime_seconds": 3600,
        "approval_reference": "approval-ticket-100",
    }}

    stale = json.loads(inspect_path.read_text())
    stale["current_owners"]["100"]["item_revision"] = 13
    stale_body = dict(stale)
    del stale_body["evidence_digest"]
    stale["evidence_digest"] = cli.digest_json(stale_body)
    stale_path = temp / "stale-inspection.json"
    cli.atomic_write_json(stale_path, stale, overwrite=False)
    rejected = run_cli(
        "export", "--plan", str(plan_path), "--inspect", str(stale_path),
        "--artifact", str(temp / "stale-export.json"), "--approve", "--actor", "approved-staff",
        "--reason", "death_restitution", check=False,
    )
    assert rejected.returncode == 2
    assert "different evidence digests" in rejected.stderr

    forged = json.loads(plan_path.read_text())
    forged["native_fence"]["expected_source_owner_revision"] = 7
    forged_body = dict(forged)
    del forged_body["plan_digest"]
    del forged_body["restitution_id_hex"]
    forged["plan_digest"] = cli.digest_json(forged_body)
    forged["restitution_id_hex"] = hashlib.sha256(
        b"duris-player-death-restitution-v1\0"
        + cli.hex_bytes(forged["evidence_digest"], "evidence digest")
        + int(forged["recipient_pid"]).to_bytes(8, "little")
        + cli.hex_bytes(forged["plan_digest"], "plan digest")
    ).digest()[:16].hex()
    forged_path = temp / "forged-plan.json"
    cli.atomic_write_json(forged_path, forged, overwrite=False)
    forged_rejected = run_cli(
        "export", "--plan", str(forged_path), "--inspect", str(inspect_path),
        "--artifact", str(temp / "forged-export.json"), "--approve", "--actor", "approved-staff",
        "--reason", "death_restitution", check=False,
    )
    assert forged_rejected.returncode == 2
    assert "exact protected result" in forged_rejected.stderr, forged_rejected.stderr

    harness = temp / "staff_handoff.cpp"
    binary = temp / "staff_handoff"
    harness.write_text(HARNESS)
    common = [
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-Isrc", "-pthread",
    ]
    subprocess.run(
        common + [
            "src/cmd/player_death_restitution.c",
            "src/player/player_death_restitution_staff.c",
            "src/player/player_death_restitution_adapter.c",
            "src/player/player_death_restitution_runtime.c",
            "src/persistence/player_death_restitution_command.c",
            "src/persistence/critical_command.c",
            str(harness), "-lcrypto", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), payload["canonical_hex"]], cwd=ROOT, check=True, timeout=10)
    subprocess.run([str(binary), native_payload["canonical_hex"]], cwd=ROOT, check=True, timeout=10)
    subprocess.run([str(binary), artifact_payload["canonical_hex"]], cwd=ROOT, check=True, timeout=10)

print("[PASS] operator export is accepted by the production staff command; stale evidence and auth are fenced")
