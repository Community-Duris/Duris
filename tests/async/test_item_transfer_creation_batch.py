#!/usr/bin/env python3
"""RED/GREEN contract for system-to-player multi-root item creation."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from _paths import rel

ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "item/item_transfer_command.h"

#include <cassert>
#include <cstdint>
#include <vector>

namespace
{
critical_operation_id operation(uint8_t discriminator)
{
    critical_operation_id id = {};
    id.bytes[0] = 0xa5;
    id.bytes.back() = discriminator;
    return id;
}

item_transfer_payload creation_batch()
{
    item_transfer_payload payload = {};
    payload.from_owner = { item_owner_type::system, 0, 0 };
    payload.to_owner = { item_owner_type::player, 42, 0 };
    payload.reason = item_transfer_reason::creation;
    payload.reason_id = 181;
    payload.expected_from_revision = 7;
    payload.expected_to_revision = 12;
    payload.multi_root = true;
    payload.item_count = 3;
    payload.items[0] = { 100, 100, 0, ITEM_TRANSFER_ABSENT_REVISION, 500,
                         item_custody_state::absent };
    payload.items[1] = { 101, 101, 0, ITEM_TRANSFER_ABSENT_REVISION, 501,
                         item_custody_state::absent };
    payload.items[2] = { 102, 102, 0, ITEM_TRANSFER_ABSENT_REVISION, 502,
                         item_custody_state::absent };
    payload.item_blob_size = 3;
    payload.item_blob[0] = 0x12;
    payload.item_blob[1] = 0x34;
    payload.item_blob[2] = 0x56;
    return payload;
}

bool builds(const item_transfer_payload &payload, uint8_t discriminator)
{
    critical_command command = {};
    return item_transfer_command_build(&command, operation(discriminator), payload,
                                       critical_source_site::command,
                                       critical_deadline_class::interactive);
}
} // namespace

int main()
{
    const item_transfer_payload payload = creation_batch();
    critical_command command = {};
    assert(item_transfer_command_build(&command, operation(1), payload,
                                       critical_source_site::command,
                                       critical_deadline_class::interactive));
    assert(command.payload_version == ITEM_TRANSFER_PAYLOAD_VERSION);
    command.accepted_at_usec = 1;
    assert(critical_command_valid(command));

    item_transfer_payload decoded = {};
    assert(item_transfer_command_decode_payload(command, &decoded));
    assert(decoded.multi_root);
    assert(decoded.selected_item_uid == 0);
    assert(decoded.target_root_item_uid == 0);
    assert(decoded.target_parent_item_uid == 0);
    assert(decoded.item_count == 3);
    assert(decoded.items[0].root_item_uid == decoded.items[0].item_uid);
    assert(decoded.items[1].root_item_uid == decoded.items[1].item_uid);
    assert(decoded.items[2].root_item_uid == decoded.items[2].item_uid);
    assert(decoded.items[0].expected_state == item_custody_state::absent);
    assert(decoded.item_blob_size == 3 && decoded.item_blob[2] == 0x56);
    assert(item_transfer_result_root(decoded) == 100);

    auto selected_roots = std::vector<uint64_t>{};
    assert(item_transfer_selected_roots(decoded, &selected_roots));
    assert((selected_roots == std::vector<uint64_t>{ 100, 101, 102 }));

    auto duplicate = payload;
    duplicate.items[2].item_uid = duplicate.items[1].item_uid;
    assert(!builds(duplicate, 2));

    auto unsorted = payload;
    unsorted.items[1].item_uid = 99;
    unsorted.items[1].root_item_uid = 99;
    assert(!builds(unsorted, 3));

    auto selected = payload;
    selected.selected_item_uid = 100;
    assert(!builds(selected, 4));

    auto parent = payload;
    parent.target_parent_item_uid = 900;
    parent.target_root_item_uid = 900;
    assert(!builds(parent, 5));

    auto active = payload;
    active.items[1].expected_state = item_custody_state::active;
    active.items[1].expected_item_revision = 1;
    assert(!builds(active, 6));

    auto too_many = payload;
    too_many.item_count = ITEM_TRANSFER_MAX_ITEMS + 1;
    assert(!builds(too_many, 7));

    // Existing adopted multi-root movement remains a separate valid shape.
    auto adopted = payload;
    adopted.from_owner = { item_owner_type::player, 42, 0 };
    adopted.to_owner = { item_owner_type::room, 500, 0 };
    adopted.reason = item_transfer_reason::player_drop;
    adopted.expected_from_revision = 7;
    adopted.expected_to_revision = 4;
    for (size_t index = 0; index < adopted.item_count; ++index)
    {
        adopted.items[index].expected_item_revision = 5 + index;
        adopted.items[index].expected_state = item_custody_state::active;
    }
    assert(builds(adopted, 8));

    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-item-transfer-creation-batch-") as directory:
    directory = Path(directory)
    source = directory / "creation_batch.cpp"
    binary = directory / "creation_batch"
    source.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(source),
            rel("item_transfer_command.c"),
            rel("critical_command.c"),
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True)

print("[PASS] creation multi-root item-transfer command contract")
