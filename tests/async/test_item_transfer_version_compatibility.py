#!/usr/bin/env python3
"""Executable compatibility regression for revisioned item-transfer reasons."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "item_transfer_command.h"

#include <cassert>

namespace
{
constexpr size_t REASON_OFFSET = 34;

critical_operation_id operation()
{
	critical_operation_id id = {};
	id.bytes[0] = 0xa5;
	id.bytes.back() = 0x67;
	return id;
}

void set_reason(critical_command *command, item_transfer_reason reason)
{
	const uint16_t value = static_cast<uint16_t>(reason);
	command->payload[REASON_OFFSET] = static_cast<uint8_t>(value);
	command->payload[REASON_OFFSET + 1] = static_cast<uint8_t>(value >> 8);
}
} // namespace

int main()
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::shopkeeper, item_shopkeeper_owner_id(7), 0 };
	payload.to_owner = { item_owner_type::player, 42, 0 };
	payload.reason = item_transfer_reason::shop_buy;
	payload.reason_id = 7;
	payload.expected_from_revision = 3;
	payload.expected_to_revision = 9;
	payload.selected_item_uid = 100;
	payload.target_root_item_uid = 100;
	payload.item_count = 1;
	payload.items[0] = { 100, 100, 0, 5, 500, item_custody_state::active };

	critical_command command = {};
	assert(item_transfer_command_build(&command, operation(), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(command.payload_version == ITEM_TRANSFER_PAYLOAD_VERSION);
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));

	item_transfer_payload decoded = {};
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.reason == item_transfer_reason::shop_buy);
	assert(item_owner_identity_equal(decoded.from_owner, payload.from_owner));

	command.payload_version = ITEM_TRANSFER_PREVIOUS_PAYLOAD_VERSION;
	set_reason(&command, item_transfer_reason::player_give);
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.reason == item_transfer_reason::player_give);

	set_reason(&command, item_transfer_reason::shop_buy);
	assert(!item_transfer_command_decode_payload(command, &decoded));
	return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-item-transfer-version-") as temp_dir:
    source = Path(temp_dir) / "item_transfer_version_test.cpp"
    binary = Path(temp_dir) / "item_transfer_version_test"
    source.write_text(HARNESS)
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
            "src/item_transfer_command.c",
            "src/critical_command.c",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

print("[PASS] item-transfer v2 compatibility and v3 shop reasons")
