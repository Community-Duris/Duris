#!/usr/bin/env python3
"""Exercise the staff restitution boundary through the live adapter."""

from _paths import ROOT, source
import subprocess
import tempfile
from pathlib import Path


COMMAND = source("cmd/player_death_restitution.c").read_text()
STAFF = source("player/player_death_restitution_staff.c").read_text()
INTERP = source("cmd/interp.c").read_text()
INTERP_H = source("cmd/interp.h").read_text()
MAKEFILE = (ROOT / "src" / "Makefile").read_text()

assert "player_death_restitution_staff_submit_hex" in COMMAND
assert "player_death_restitution_staff_begin" in COMMAND
assert "player_death_restitution_staff_append_hex" in COMMAND
assert "player_death_restitution_staff_commit" in COMMAND
assert "GET_LEVEL(ch) < FORGER" in COMMAND
assert "CMD_GRT(CMD_RESTITUTION" in INTERP
assert '"restitution"' in INTERP
assert "CMD_RESTITUTION 863" in INTERP_H
assert "cmd/player_death_restitution.o" in MAKEFILE
assert "player/player_death_restitution_staff.o" in MAKEFILE
for forbidden in ("mysql", "sql_", "system(", "popen(", "python"):
    assert forbidden not in COMMAND
    assert forbidden not in STAFF
print("[PASS] restricted staff command is registered and delegates without maintenance bypasses")

HARNESS = r'''
#include "core/config.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "cmd/interp.h"
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
#include <openssl/sha.h>

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
critical_submit_result coordinator_result = critical_submit_result::journal_uncertain;
critical_operation_id submitted_operation = {};
bool target_fence_held = false;
int release_calls = 0;

critical_operation_id operation_id(uint8_t seed)
{
    critical_operation_id value = {};
    for (size_t index = 0; index < value.bytes.size(); ++index)
        value.bytes[index] = static_cast<uint8_t>(seed + index);
    return value;
}

player_death_restitution_plan approved_plan()
{
    player_death_restitution_item_state state = {};
    state.item_uid = 101;
    state.vnum = 7101;
    state.quantity = 1;
    state.weight = 4;
    state.cost = 99;
    state.timer = -1;
    state.item_type = 1;
    state.material = 2;
    state.condition = 100;
    std::vector<uint8_t> encoded_state;
    assert(player_death_restitution_item_state_encode(state, &encoded_state));

    player_death_restitution_item item = {};
    item.item_uid = state.item_uid;
    item.source_root_item_uid = state.item_uid;
    item.delivered_root_item_uid = state.item_uid;
    item.source_item_revision = 12;
    item.custody_item_revision = 9;
    item.expected_item_revision = 12;
    item.expected_owner_revision = 4;
    item.expected_owner_state = PLAYER_DEATH_RESTITUTION_QUARANTINED_STATE;
    item.custody_state = 1;
    item.custody_owner_type = PLAYER_DEATH_RESTITUTION_PLAYER_OWNER_TYPE;
    item.custody_owner_id = 10;
    item.vnum = state.vnum;
    item.disposition = player_death_restitution_disposition::deliver;
    item.classification = "ordinary_item";
    item.note = "staff boundary";
    item.metadata_payload = encoded_state;
    item.original_payload = { 0xaa };
    SHA256(encoded_state.data(), encoded_state.size(), item.metadata_digest.data());

    player_death_restitution_plan plan = {};
    plan.source_pid = 10;
    plan.death_revision = 77;
    plan.recipient_pid = 20;
    plan.restitution_id = operation_id(1);
    plan.death_operation_id = operation_id(33);
    plan.evidence_digest.fill(0x11);
    plan.plan_digest.fill(0x22);
    plan.expected_recipient_save_revision = 8;
    plan.expected_source_owner_revision = 4;
    plan.expected_recipient_owner_revision = 6;
    plan.loss_epoch = 1700000000;
    plan.actor = "approved-staff";
    plan.reason = "death restitution";
    plan.items.push_back(item);
    assert(player_death_restitution_plan_valid(plan));
    return plan;
}

std::string canonical_hex(const critical_command &command)
{
    std::vector<uint8_t> encoded;
    assert(critical_command_encode(command, &encoded) == critical_command_codec_result::ok);
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(encoded.size() * 2);
    for (uint8_t value : encoded)
    {
        result.push_back(digits[value >> 4]);
        result.push_back(digits[value & 0x0f]);
    }
    return result;
}
}

bool is_pid_online(int pid, bool)
{
    assert(pid == 20);
    return false;
}

bool player_load_pipeline_pid_pending(int pid)
{
    assert(pid == 20);
    return false;
}

bool player_save_pipeline_target_save_pending(int pid)
{
    assert(pid == 20);
    return false;
}

bool player_save_pipeline_acquire_target_save_login_fence(int pid,
                                                           player_revision_t revision)
{
    assert(pid == 20 && revision == 8);
    if (target_fence_held)
        return false;
    target_fence_held = true;
    return true;
}

void player_save_pipeline_release_target_save_login_fence(int pid,
                                                           player_revision_t revision)
{
    assert(pid == 20 && revision == 8);
    ++release_calls;
    target_fence_held = false;
}

bool player_save_pipeline_save_admitted(int pid)
{
    assert(pid == 20 || pid == 21);
    return pid != 20 || !target_fence_held;
}

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
    submitted_operation = command.operation_id;
    return coordinator_result;
}

int main()
{
    player_death_restitution_plan plan = approved_plan();
    critical_command command = {};
    assert(player_death_restitution_command_build(plan, &command));
    command.source_site = critical_source_site::operator_repair;
    command.deadline_class = critical_deadline_class::interactive;
    assert(critical_command_normalize(&command));
    const std::string encoded = canonical_hex(command);
    assert(encoded.size() <= PLAYER_DEATH_RESTITUTION_STAFF_MAX_COMMAND_BYTES * 2);

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
    for (size_t offset = 0; offset < encoded.size(); offset += 800)
    {
        const size_t chunk_size = std::min<size_t>(800, encoded.size() - offset);
        invoke_command("chunk " + encoded.substr(offset, chunk_size));
        assert(last_message.find("chunk accepted") != std::string::npos);
    }
    invoke_command("commit");
    assert(last_message.find("journal-uncertain") != std::string::npos);
    assert(submitted_operation.bytes == command.operation_id.bytes);
    assert(target_fence_held);
    assert(!player_death_restitution_runtime_login_admit(20));
    assert(player_death_restitution_runtime_login_admit(21));

    assert(player_death_restitution_staff_submit_hex(
               "approved-staff", LESSER_G, encoded.data(), encoded.size(), nullptr) ==
           player_death_restitution_runtime_result::unauthorized);
    assert(target_fence_held);
    assert(player_death_restitution_staff_submit_hex(
               "other-staff", FORGER, encoded.data(), encoded.size(), nullptr) ==
           player_death_restitution_runtime_result::identity_conflict);
    assert(target_fence_held);

    critical_completion completion = {};
    completion.operation_id = submitted_operation;
    completion.outcome = critical_apply_outcome::ambiguous_commit;
    player_death_restitution_runtime_handle_completions(&completion, 1);
    assert(target_fence_held);
    assert(release_calls == 0);

    completion.outcome = critical_apply_outcome::already_applied;
    player_death_restitution_runtime_handle_completions(&completion, 1);
    assert(!target_fence_held);
    assert(release_calls == 1);

    command.source_site = critical_source_site::recovery;
    assert(critical_command_normalize(&command));
    const std::string unapproved = canonical_hex(command);
    assert(player_death_restitution_staff_submit_hex(
               "approved-staff", FORGER, unapproved.data(), unapproved.size(), nullptr) ==
           player_death_restitution_runtime_result::invalid_plan);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-restitution-staff-") as temp_dir:
    temp = Path(temp_dir)
    harness = temp / "staff_boundary.cpp"
    binary = temp / "staff_boundary"
    harness.write_text(HARNESS)
    common = [
        "g++",
        "-std=c++20",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Werror",
        "-Isrc",
        "-pthread",
    ]
    subprocess.run(
        common + [
            "-c",
            "src/cmd/player_death_restitution.c",
            "-o",
            str(temp / "command.o"),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        common
        + [
            "src/cmd/player_death_restitution.c",
            "src/player/player_death_restitution_staff.c",
            "src/player/player_death_restitution_adapter.c",
            "src/player/player_death_restitution_runtime.c",
            "src/persistence/player_death_restitution_command.c",
            "src/persistence/critical_command.c",
            str(harness),
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=False,
        text=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=10)
print("[PASS] production staff boundary submits a canonical approved command and holds the target fence through ambiguity")
