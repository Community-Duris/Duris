#!/usr/bin/env python3
"""Executable compatibility regression for revisioned item-transfer reasons."""

from _paths import rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "item/item_transfer_command.h"

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
	payload.item_blob_size = 3;
	payload.item_blob[0] = 0x12;
	payload.item_blob[1] = 0x34;
	payload.item_blob[2] = 0x56;

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
	assert(decoded.item_blob_size == payload.item_blob_size);
	assert(decoded.item_blob[2] == payload.item_blob[2]);
	assert(!decoded.corpse.present);

	command.payload.resize(ITEM_TRANSFER_PAYLOAD_BYTES + sizeof(uint32_t) +
			       payload.item_blob_size);
	command.payload_version = ITEM_TRANSFER_EXACT_PAYLOAD_VERSION;
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.item_blob_size == payload.item_blob_size);
	assert(!decoded.corpse.present);

	command.payload.resize(ITEM_TRANSFER_PAYLOAD_BYTES);
	command.payload_version = ITEM_TRANSFER_PREVIOUS_PAYLOAD_VERSION;
	set_reason(&command, item_transfer_reason::shop_buy);
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.reason == item_transfer_reason::shop_buy);
	assert(decoded.item_blob_size == 0);

	command.payload_version = ITEM_TRANSFER_LEGACY_PAYLOAD_VERSION;
	set_reason(&command, item_transfer_reason::player_give);
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.reason == item_transfer_reason::player_give);

	set_reason(&command, item_transfer_reason::shop_buy);
	assert(!item_transfer_command_decode_payload(command, &decoded));

	payload.from_owner = { item_owner_type::corpse, item_corpse_owner_id(42, 20), 0 };
	payload.to_owner = { item_owner_type::player, 77, 0 };
	payload.reason = item_transfer_reason::corpse_loot;
	payload.expected_from_revision = 4;
	payload.expected_to_revision = 6;
	payload.corpse.present = true;
	payload.corpse.room_vnum = 500;
	payload.corpse.weight = 90;
	payload.corpse.actor_racewar = 2;
	payload.corpse.values[3] = 42;
	payload.corpse.values[5] = 1;
	payload.corpse.values[6] = 20;
	payload.corpse.owner_name = "Hero";
	payload.corpse.short_description = "the corpse of Hero";
	payload.corpse.description = "The corpse of Hero is lying here.";
	payload.corpse.keywords = "hero corpse _pcorpse_";
	assert(item_transfer_command_build(&command, operation(), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 2;
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.corpse.present && decoded.corpse.room_vnum == 500 &&
	       decoded.corpse.actor_racewar == 2 && decoded.corpse.values[6] == 20 &&
	       decoded.corpse.description == payload.corpse.description);
	auto truncated_context = command;
	truncated_context.payload.pop_back();
	assert(!item_transfer_command_decode_payload(truncated_context, &decoded));

	payload.from_owner = { item_owner_type::system, 0, 0 };
	payload.to_owner = { item_owner_type::player, 42, 0 };
	payload.reason = item_transfer_reason::creation;
	payload.selected_item_uid = 200;
	payload.target_root_item_uid = 700;
	payload.target_parent_item_uid = 700;
	payload.expected_target_parent_revision = 4;
	payload.items[0] = { 200, 200, 0, ITEM_TRANSFER_ABSENT_REVISION, 501,
			     item_custody_state::absent };
	payload.corpse = {};
	assert(item_transfer_command_build(&command, operation(), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.target_root_item_uid == 700 && decoded.target_parent_item_uid == 700 &&
	       decoded.expected_target_parent_revision == 4);
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
            rel("item_transfer_command.c"),
            rel("critical_command.c"),
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

print("[PASS] item-transfer v2/v3/v4 compatibility and v5 corpse context")
