#include "persistence/player_death_restitution_command.h"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <iostream>
#include <openssl/sha.h>

namespace
{
critical_operation_id id(uint8_t seed)
{
	critical_operation_id value = {};
	for (size_t index = 0; index < value.bytes.size(); ++index)
		value.bytes[index] = static_cast<uint8_t>(seed + index);
	return value;
}

player_death_restitution_item make_item(uint64_t uid, uint64_t parent = 0)
{
	player_death_restitution_item_state state = {};
	state.item_uid = uid;
	state.vnum = 7000 + static_cast<uint32_t>(uid);
	state.quantity = 1;
	state.weight = 4;
	state.cost = 99;
	state.timer = -1;
	state.item_type = 1;
	state.material = 2;
	state.condition = 100;
	state.string_present[0] = true;
	state.strings[0] = { 'r', 'e', 's', 't', 'o', 'r', 'e' };
	state.bitvector_present[0] = true;
	state.bitvectors[0] = 0x22;
	state.affects.push_back({ 1, 2 });
	state.extra_descriptions.push_back({ { 'k', 'e', 'y' }, { 'v', 'a', 'l' } });
	std::vector<uint8_t> state_payload;
	assert(player_death_restitution_item_state_encode(state, &state_payload));

	player_death_restitution_item item = {};
	item.item_uid = uid;
	item.source_root_item_uid = parent ? 100 : uid;
	item.source_parent_item_uid = parent;
	item.delivered_root_item_uid = parent ? 100 : uid;
	item.delivered_parent_item_uid = parent;
	item.source_item_revision = 12;
	item.custody_item_revision = 9;
	item.expected_item_revision = 12;
	item.expected_owner_revision = 4;
	item.expected_owner_state = PLAYER_DEATH_RESTITUTION_QUARANTINED_STATE;
	item.custody_state = 1;
	item.custody_owner_type = PLAYER_DEATH_RESTITUTION_PLAYER_OWNER_TYPE;
	item.custody_owner_id = 10;
	item.custody_owner_context_id = 0;
	item.custody_owner_revision = 4;
	item.vnum = state.vnum;
	item.disposition = player_death_restitution_disposition::deliver;
	item.classification = "ordinary_item";
	item.note = "native first slice";
	item.metadata_payload = state_payload;
	item.original_payload = { 0xaa, 0xbb, 0xcc };
	SHA256(state_payload.data(), state_payload.size(), item.metadata_digest.data());
	return item;
}

player_death_restitution_plan make_plan()
{
	player_death_restitution_plan plan = {};
	plan.source_pid = 10;
	plan.death_revision = 77;
	plan.recipient_pid = 20;
	plan.restitution_id = id(1);
	plan.death_operation_id = id(33);
	plan.evidence_digest.fill(0x11);
	plan.payload_digest.fill(0x33);
	plan.plan_digest.fill(0x22);
	plan.expected_recipient_save_revision = 8;
	plan.expected_source_owner_revision = 4;
	plan.expected_recipient_owner_revision = 6;
	plan.loss_epoch = 1700000000;
	plan.accepted_at_usec = 1700000000000000ULL;
	plan.actor = "native-test";
	plan.reason = "death restitution";
	plan.items.push_back(make_item(101));
	return plan;
}
}

int main()
{
	player_death_restitution_item_state state = {};
	state.item_uid = 44;
	state.vnum = 9001;
	state.quantity = 2;
	state.weight = 3;
	state.cost = 4;
	state.timer = 5;
	state.item_type = 6;
	state.string_present[1] = true;
	state.strings[1] = { 's', 'h', 'o', 'r', 't' };
	state.bitvector_present[2] = true;
	state.bitvectors[2] = 0x1234;
	state.affects.push_back({ 4, -5 });
	state.extra_descriptions.push_back({ { 'x' }, { 'y', 'z' } });
	std::vector<uint8_t> encoded_state;
	assert(player_death_restitution_item_state_encode(state, &encoded_state));
	player_death_restitution_item_state decoded_state = {};
	assert(player_death_restitution_item_state_decode(encoded_state.data(),
							  encoded_state.size(), &decoded_state));
	assert(decoded_state.item_uid == state.item_uid);
	assert(decoded_state.strings[1] == state.strings[1]);
	assert(decoded_state.bitvectors[2] == state.bitvectors[2]);
	assert(decoded_state.affects.size() == 1);

	player_death_restitution_plan plan = make_plan();
	assert(player_death_restitution_plan_valid(plan));
	assert(plan.evidence_digest != plan.payload_digest);
	critical_command command = {};
	assert(player_death_restitution_command_build(plan, &command));
	assert(command.type == critical_command_type::player_death_restitution);
	assert(command.keys.size() == 1);
	assert(command.keys[0].type == critical_entity_type::player);
	player_death_restitution_plan decoded_plan = {};
	assert(player_death_restitution_command_decode_payload(command, &decoded_plan));
	assert(decoded_plan.recipient_pid == plan.recipient_pid);
	assert(decoded_plan.items[0].metadata_payload == plan.items[0].metadata_payload);
	assert(decoded_plan.evidence_digest == plan.evidence_digest);
	assert(decoded_plan.payload_digest == plan.payload_digest);
	assert(decoded_plan.plan_digest == plan.plan_digest);
	assert(decoded_plan.accepted_at_usec == command.accepted_at_usec);

	critical_command tampered = command;
	tampered.payload[0] ^= 0xff;
	assert(!player_death_restitution_command_decode_payload(tampered, &decoded_plan));

	player_death_restitution_plan parent_plan = plan;
	parent_plan.items.clear();
	parent_plan.items.push_back(make_item(100));
	parent_plan.items.push_back(make_item(101, 100));
	assert(player_death_restitution_plan_valid(parent_plan));
	player_death_restitution_plan reversed_plan = parent_plan;
	std::swap(reversed_plan.items[0], reversed_plan.items[1]);
	assert(!player_death_restitution_plan_valid(reversed_plan));

	player_death_restitution_plan artifact_plan = plan;
	artifact_plan.items[0].artifact_vnum = artifact_plan.items[0].vnum;
	assert(!player_death_restitution_plan_valid(artifact_plan));
	artifact_plan.items[0].artifact_timing_uid_approved = true;
	artifact_plan.items[0].artifact_approval_uid = artifact_plan.items[0].item_uid;
	artifact_plan.items[0].artifact_source_location_type =
		PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_PLAYER;
	artifact_plan.items[0].artifact_source_location =
		static_cast<int32_t>(artifact_plan.source_pid);
	artifact_plan.items[0].artifact_type = PLAYER_DEATH_RESTITUTION_ARTIFACT_TYPE_UNIQUE;
	artifact_plan.items[0].artifact_source_timer_epoch = 1700000456;
	artifact_plan.items[0].artifact_usable_lifetime_seconds = 3600;
	artifact_plan.items[0].artifact_domain_present = true;
	artifact_plan.items[0].artifact_domain_item_uid_present = true;
	artifact_plan.items[0].artifact_domain_item_uid = artifact_plan.items[0].item_uid;
	artifact_plan.items[0].artifact_domain_item_revision =
		artifact_plan.items[0].expected_item_revision;
	artifact_plan.items[0].artifact_domain_revision = 4;
	artifact_plan.items[0].artifact_legacy_projection_mask =
		PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_MORTAL;
	assert(player_death_restitution_plan_valid(artifact_plan));
	uint64_t delivered_timer_epoch = 0;
	assert(player_death_restitution_artifact_delivery_timer(artifact_plan.items[0], 1700000000,
								&delivered_timer_epoch));
	assert(delivered_timer_epoch == 1700003600);
	artifact_plan.items[0].artifact_timing_uid_approved = false;
	assert(!player_death_restitution_artifact_delivery_timer(artifact_plan.items[0], 1700000000,
								 &delivered_timer_epoch));

	player_death_restitution_result result = {};
	result.restitution_id = command.operation_id;
	result.source_pid = plan.source_pid;
	result.recipient_pid = plan.recipient_pid;
	result.delivery_epoch = 1700000001;
	result.durable_revision = 13;
	result.candidate_count = 2;
	result.delivered_count = 1;
	result.unresolved_count = 1;
	result.mutation_applied = true;
	std::array<uint8_t, PLAYER_DEATH_RESTITUTION_RESULT_BYTES> encoded_result = {};
	assert(player_death_restitution_command_encode_result(result, &encoded_result));
	player_death_restitution_result decoded_result = {};
	assert(player_death_restitution_command_decode_result(
		encoded_result.data(), encoded_result.size(), &decoded_result));
	assert(decoded_result.durable_revision == result.durable_revision);
	assert(decoded_result.mutation_applied);
	encoded_result.back() = 1;
	assert(!player_death_restitution_command_decode_result(
		encoded_result.data(), encoded_result.size(), &decoded_result));

	std::cout << "player death restitution command tests passed\n";
	return 0;
}
