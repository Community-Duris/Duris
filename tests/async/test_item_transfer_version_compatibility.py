#!/usr/bin/env python3
"""Executable compatibility regression for revisioned item-transfer reasons."""

from _paths import rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
COMMAND_SOURCE = (ROOT / "src/item/item_transfer_command.c").read_text(
    encoding="utf-8", errors="replace"
)

HARNESS = r'''
#include "item/item_transfer_command.h"

#include <algorithm>
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

void put_u32(std::vector<uint8_t> *bytes, size_t offset, uint32_t value)
{
	for (unsigned int byte = 0; byte < 4; ++byte)
		(*bytes)[offset + byte] = static_cast<uint8_t>(value >> (byte * 8));
}
} // namespace

int main()
{
	const item_owner_identity collector = { item_owner_type::collector,
						  item_collector_owner_id(987), 0 };
	assert(item_collector_owner_id(0) == 0);
	assert(item_owner_identity_valid(collector));
	assert(!item_owner_identity_valid({ item_owner_type::collector, 0, 0 }));
	assert(!item_owner_identity_valid({ item_owner_type::collector, 987, 1 }));
	critical_entity_key collector_key = {};
	assert(item_owner_key(collector, &collector_key));
	assert(collector_key.type == critical_entity_type::collector && collector_key.id == 987);

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

	auto collector_transfer = payload;
	collector_transfer.from_owner = collector;
	collector_transfer.reason = item_transfer_reason::collector_buyback;
	critical_command rejected_collector = {};
	assert(!item_transfer_command_build(&rejected_collector, operation(), collector_transfer,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	collector_transfer.reason = item_transfer_reason::shop_buy;
	assert(!item_transfer_command_build(&rejected_collector, operation(), collector_transfer,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	collector_transfer = payload;
	collector_transfer.to_owner = collector;
	collector_transfer.reason = item_transfer_reason::collector_collect;
	assert(!item_transfer_command_build(&rejected_collector, operation(), collector_transfer,
					    critical_source_site::command,
					    critical_deadline_class::interactive));

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
	auto truncated_variable = command;
	truncated_variable.payload.resize(ITEM_TRANSFER_HEADER_BYTES +
					  ITEM_TRANSFER_ENTRY_BYTES);
	assert(!item_transfer_command_decode_payload(truncated_variable, &decoded));

	std::vector<uint8_t> legacy_payload(ITEM_TRANSFER_PAYLOAD_BYTES + sizeof(uint32_t) +
					    payload.item_blob_size);
	std::copy_n(command.payload.begin(), ITEM_TRANSFER_HEADER_BYTES + ITEM_TRANSFER_ENTRY_BYTES,
		    legacy_payload.begin());
	put_u32(&legacy_payload, ITEM_TRANSFER_PAYLOAD_BYTES, payload.item_blob_size);
	std::copy_n(payload.item_blob.begin(), payload.item_blob_size,
		    legacy_payload.begin() + ITEM_TRANSFER_PAYLOAD_BYTES + sizeof(uint32_t));
	command.payload = std::move(legacy_payload);
	command.payload_version = ITEM_TRANSFER_EXACT_PAYLOAD_VERSION;
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.item_blob_size == payload.item_blob_size);
	assert(!decoded.corpse.present);
	for (uint16_t version : { ITEM_TRANSFER_EXACT_PAYLOAD_VERSION,
				  ITEM_TRANSFER_CORPSE_PAYLOAD_VERSION })
	{
		auto truncated_fixed = command;
		truncated_fixed.payload_version = version;
		truncated_fixed.payload.resize(ITEM_TRANSFER_PAYLOAD_BYTES);
		assert(!item_transfer_command_decode_payload(truncated_fixed, &decoded));
	}

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

	item_transfer_payload batch = {};
	batch.from_owner = { item_owner_type::room, 50, 0 };
	batch.to_owner = { item_owner_type::player, 42, 0 };
	batch.reason = item_transfer_reason::player_get;
	batch.expected_from_revision = 7;
	batch.expected_to_revision = 9;
	batch.multi_root = true;
	batch.item_count = 2;
	batch.items[0] = { 100, 100, 0, 5, 500, item_custody_state::active };
	batch.items[1] = { 200, 200, 0, 6, 501, item_custody_state::active };
	assert(item_transfer_command_build(&command, operation(), batch,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 3;
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.multi_root && decoded.selected_item_uid == 0 &&
	       item_transfer_result_root(decoded) == 100 &&
	       item_transfer_selected_root(decoded, 200) == 200);
	std::vector<uint64_t> selected_roots;
	assert(item_transfer_selected_roots(decoded, &selected_roots));
	assert((selected_roots == std::vector<uint64_t>{ 100, 200 }));
	uint64_t target_root = 0, target_parent = 1;
	assert(item_transfer_target_topology(decoded, 200, &target_root, &target_parent));
	assert(target_root == 200 && target_parent == 0);
	auto batch_command = command;

	// A bulk `put all` may select several carried roots, including a container with
	// children.  Only each selected root is reparented to the destination; descendants
	// must remain below their original container instead of being flattened beside it.
	item_transfer_payload nested_put = {};
	nested_put.from_owner = { item_owner_type::player, 42, 0 };
	nested_put.to_owner = nested_put.from_owner;
	nested_put.reason = item_transfer_reason::player_put;
	nested_put.reason_id = 900;
	nested_put.expected_from_revision = 9;
	nested_put.expected_to_revision = 9;
	nested_put.target_root_item_uid = 900;
	nested_put.target_parent_item_uid = 900;
	nested_put.expected_target_parent_revision = 3;
	nested_put.multi_root = true;
	nested_put.item_count = 3;
	nested_put.items[0] = { 100, 100, 0, 5, 500, item_custody_state::active };
	nested_put.items[1] = { 101, 100, 100, 6, 501, item_custody_state::active };
	nested_put.items[2] = { 200, 200, 0, 7, 502, item_custody_state::active };
	assert(item_transfer_command_build(&command, operation(), nested_put,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(item_transfer_target_topology(decoded, 100, &target_root, &target_parent));
	assert(target_root == 900 && target_parent == 900);
	assert(item_transfer_target_topology(decoded, 101, &target_root, &target_parent));
	assert(target_root == 900 && target_parent == 100);
	assert(item_transfer_target_topology(decoded, 200, &target_root, &target_parent));
	assert(target_root == 900 && target_parent == 900);

	item_transfer_payload pet = {};
	pet.from_owner = { item_owner_type::player, 42, 0 };
	pet.to_owner = { item_owner_type::pet, 1000, 42 };
	pet.reason = item_transfer_reason::pet_give;
	pet.reason_id = 1000;
	pet.selected_item_uid = 100;
	pet.item_count = 1;
	pet.items[0] = { 100, 100, 0, 5, 500, item_custody_state::active };
	assert(item_transfer_command_build(&command, operation(), pet,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.reason == item_transfer_reason::pet_give &&
	       item_owner_identity_equal(decoded.to_owner, pet.to_owner));
	pet.reason = item_transfer_reason::player_put;
	assert(!item_transfer_command_build(&command, operation(), pet,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	pet.reason = item_transfer_reason::pet_give;
	pet.reason_id = 1001;
	assert(!item_transfer_command_build(&command, operation(), pet,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	pet.reason_id = 1000;
	pet.from_owner = { item_owner_type::pet, 1000, 42 };
	pet.to_owner = { item_owner_type::player, 42, 0 };
	pet.reason = item_transfer_reason::pet_return;
	assert(item_transfer_command_build(&command, operation(), pet,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.reason == item_transfer_reason::pet_return);

	item_transfer_payload trusted_steal = {};
	trusted_steal.from_owner = { item_owner_type::player, 42, 0 };
	trusted_steal.to_owner = { item_owner_type::player, 77, 0 };
	trusted_steal.reason = item_transfer_reason::trusted_steal;
	trusted_steal.reason_id = 42;
	trusted_steal.expected_from_revision = 10;
	trusted_steal.expected_to_revision = 11;
	trusted_steal.selected_item_uid = 100;
	trusted_steal.target_root_item_uid = 100;
	trusted_steal.item_count = 1;
	trusted_steal.items[0] = { 100, 100, 0, 5, 500, item_custody_state::active };
	assert(item_transfer_command_build(&command, operation(), trusted_steal,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.reason == item_transfer_reason::trusted_steal &&
	       decoded.reason_id == trusted_steal.reason_id);
	auto invalid_trusted_steal = trusted_steal;
	invalid_trusted_steal.from_owner = { item_owner_type::room, 500, 0 };
	assert(!item_transfer_command_build(&command, operation(), invalid_trusted_steal,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	invalid_trusted_steal = trusted_steal;
	invalid_trusted_steal.reason_id = 77;
	assert(!item_transfer_command_build(&command, operation(), invalid_trusted_steal,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	invalid_trusted_steal = trusted_steal;
	invalid_trusted_steal.to_owner.id = 42;
	assert(!item_transfer_command_build(&command, operation(), invalid_trusted_steal,
					    critical_source_site::command,
					    critical_deadline_class::interactive));

	// Version 6 batch commands remain replayable: v7 adds one trailing collector
	// context length, which is absent from the older wire contract.
	auto version_six = batch_command;
	version_six.payload_version = ITEM_TRANSFER_BATCH_PAYLOAD_VERSION;
	version_six.payload.resize(version_six.payload.size() - sizeof(uint32_t));
	assert(item_transfer_command_decode_payload(version_six, &decoded));
	assert(decoded.multi_root && decoded.item_count == 2 && !decoded.collector.present);

	item_transfer_payload death = {};
	death.from_owner = { item_owner_type::player, 42, 0 };
	death.to_owner = { item_owner_type::corpse, item_corpse_owner_id(42, 1700000000), 0 };
	death.reason = item_transfer_reason::corpse_create;
	death.reason_id = 1700000000;
	death.expected_from_revision = 9;
	death.expected_to_revision = 0;
	death.multi_root = true;
	death.item_count = 2;
	death.items[0] = { 100, 100, 0, 5, 500, item_custody_state::active };
	death.items[1] = { 200, 200, 0, 6, 501, item_custody_state::active };
	death.corpse.present = true;
	death.corpse.room_vnum = 500;
	death.corpse.weight = 90;
	death.corpse.actor_racewar = 1;
	death.corpse.values[3] = 42;
	death.corpse.values[5] = 1;
	death.corpse.values[6] = 1700000000;
	death.corpse.owner_name = "Hero";
	death.corpse.short_description = "the corpse of Hero";
	death.corpse.description = "The corpse of Hero is lying here.";
	death.corpse.keywords = "hero corpse _pcorpse_";
	death.collector.present = true;
	death.collector.death_operation = operation();
	death.collector.beneficiary_pid = 42;
	death.collector.death_time = 1700000000;
	death.collector.policy = { 10, 20, 30, 200, 100 };
	death.collector.eligible_item_uids = { 100, 200 };
	assert(item_transfer_command_build(&command, death.collector.death_operation, death,
					   critical_source_site::combat,
					   critical_deadline_class::interactive));
	command.accepted_at_usec = 4;
	assert(critical_command_valid(command));
	assert(command.keys.size() == 5 && command.expected_revisions.size() == 5);
	const auto collector_fence = std::find_if(
		command.expected_revisions.begin(), command.expected_revisions.end(),
		[](const critical_expected_revision &revision) {
			return revision.key.type == critical_entity_type::collector &&
			       revision.key.id == UINT64_MAX;
		});
	assert(collector_fence != command.expected_revisions.end() &&
	       collector_fence->revision == 0);
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.collector.present &&
	       critical_operation_id_equal(decoded.collector.death_operation,
					   death.collector.death_operation) &&
	       decoded.collector.beneficiary_pid == 42 &&
	       decoded.collector.death_time == 1700000000 &&
	       decoded.collector.policy.sale_delay == 20 &&
	       decoded.collector.eligible_item_uids ==
		       std::vector<uint64_t>({ 100, 200 }));
	auto invalid_death = death;
	invalid_death.collector.eligible_item_uids = { 200, 100 };
	assert(!item_transfer_command_build(&command, operation(), invalid_death,
					    critical_source_site::combat,
					    critical_deadline_class::interactive));
	invalid_death = death;
	invalid_death.collector.eligible_item_uids = { 999 };
	assert(!item_transfer_command_build(&command, operation(), invalid_death,
					    critical_source_site::combat,
					    critical_deadline_class::interactive));
	return 0;
}
'''

assert (
    "command.payload.size() < item_section_size + sizeof(uint32_t)"
    in COMMAND_SOURCE
), "v4-v7 item blob length reads must be bounds-checked"


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

print("[PASS] item-transfer v2-v7 compatibility, corpse and collector contexts")
