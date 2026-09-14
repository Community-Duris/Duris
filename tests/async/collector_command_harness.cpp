#include "economy/collector_command.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{
constexpr char death_operation[] = "123456789abcdef0123456789abcdef0";

critical_operation_id operation()
{
	critical_operation_id result = {};
	result.bytes[0] = 0xc0;
	result.bytes[1] = 0x11;
	result.bytes.back() = 0x77;
	return result;
}

collector_command_payload collection()
{
	collector_command_payload payload;
	payload.action = collector_action::collect;
	payload.target_state = item_custody_state::active;
	payload.listing = 77;
	payload.expected_listing_revision = 5;
	payload.observed_at = 45000;
	payload.from_owner = { item_owner_type::corpse, item_corpse_owner_id(42, 3), 0 };
	payload.to_owner = { item_owner_type::collector, item_collector_owner_id(77), 0 };
	payload.expected_from_owner_revision = 9;
	payload.expected_to_owner_revision = 0;
	payload.selected_item_uid = 101;
	payload.item_count = 3;
	payload.items[0] = { 100, 100, 0, 4, 500, item_custody_state::active };
	payload.items[1] = { 101, 100, 100, 5, 501, item_custody_state::active };
	payload.items[2] = { 102, 100, 101, 6, 502, item_custody_state::active };
	payload.item_blob_size = 4;
	payload.item_blob[0] = 0xde;
	payload.item_blob[1] = 0xad;
	payload.item_blob[2] = 0xbe;
	payload.item_blob[3] = 0xef;
	return payload;
}

collector_command_payload purchase()
{
	collector_command_payload payload;
	payload.action = collector_action::purchase;
	payload.target_state = item_custody_state::active;
	payload.capacity_admitted = true;
	payload.listing = 77;
	payload.expected_listing_revision = 7;
	payload.observed_at = 90000;
	payload.actor_pid = 42;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), "CollectorTester");
	payload.expected_wallet_revision = 10;
	payload.expected_bank_revision = 11;
	payload.from_owner = { item_owner_type::collector, item_collector_owner_id(77), 0 };
	payload.to_owner = { item_owner_type::player, 42, 0 };
	payload.expected_from_owner_revision = 1;
	payload.expected_to_owner_revision = 20;
	payload.selected_item_uid = 101;
	payload.item_count = 1;
	payload.items[0] = { 101, 101, 0, 6, 501, item_custody_state::active };
	payload.item_blob_size = 4;
	payload.item_blob[0] = 1;
	payload.item_blob[1] = 2;
	payload.item_blob[2] = 3;
	payload.item_blob[3] = 4;
	return payload;
}

collector::record collected_record()
{
	collector::rules rules;
	rules.enabled = true;
	collector::record entry;
	assert(collector::enroll(77, death_operation, 42, 101, 5, 1000, rules, &entry) ==
	       collector::outcome::applied);
	assert(collector::collect(&entry, entry.revision, 5, 5, true, 75, entry.collect_at) ==
	       collector::outcome::applied);
	return entry;
}

collector::record paused_record()
{
	auto entry = collected_record();
	assert(collector::activate(&entry, entry.revision, entry.sale_at) ==
	       collector::outcome::applied);
	assert(collector::pause(&entry, entry.revision, entry.available_at + 10) ==
	       collector::outcome::applied);
	return entry;
}

bool has_fence(const critical_command &command, critical_entity_type type, uint64_t id,
	       uint64_t revision)
{
	return std::any_of(command.expected_revisions.begin(), command.expected_revisions.end(),
			   [&](const critical_expected_revision &candidate) {
				   return candidate.key.type == type && candidate.key.id == id &&
					  candidate.revision == revision;
			   });
}
} // namespace

int main()
{
	auto collect = collection();
	critical_command command;
	assert(collector_command_build(&command, operation(), collect,
				       critical_source_site::recovery,
				       critical_deadline_class::background));
	assert(command.type == critical_command_type::collector &&
	       command.payload_version == COLLECTOR_COMMAND_PAYLOAD_VERSION &&
	       command.keys.size() == 5 && command.expected_revisions.size() == 5);
	assert(has_fence(command, critical_entity_type::collector, 77, 5));
	assert(has_fence(command, critical_entity_type::corpse, item_corpse_owner_id(42, 3), 9));
	assert(!has_fence(command, critical_entity_type::collector, 77, 0));
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));

	std::vector<uint8_t> encoded_command;
	assert(critical_command_encode(command, &encoded_command) ==
	       critical_command_codec_result::ok);
	critical_command command_copy;
	assert(critical_command_decode(encoded_command.data(), encoded_command.size(),
				       &command_copy) == critical_command_codec_result::ok);
	collector_command_payload decoded;
	assert(collector_command_decode_payload(command_copy, &decoded));
	assert(decoded.action == collector_action::collect && decoded.listing == 77 &&
	       decoded.item_count == 3 && decoded.items[2].parent_item_uid == 101 &&
	       decoded.item_blob_size == 4 && decoded.item_blob[3] == 0xef);

	critical_command bad_fence = command_copy;
	for (auto &fence : bad_fence.expected_revisions)
		if (fence.key.type == critical_entity_type::collector)
			++fence.revision;
	assert(!collector_command_decode_payload(bad_fence, &decoded));
	critical_command trailing = command_copy;
	trailing.payload.push_back(0);
	assert(!collector_command_decode_payload(trailing, &decoded));

	auto buy = purchase();
	assert(collector_command_build(&command, operation(), buy, critical_source_site::command,
				       critical_deadline_class::interactive));
	assert(command.keys.size() == 4 && command.expected_revisions.size() == 4);
	assert(has_fence(command, critical_entity_type::collector, 77, 7));
	assert(has_fence(command, critical_entity_type::player, 42, 10));
	assert(has_fence(command, critical_entity_type::item, 101, 6));
	assert(collector_command_decode_payload(command, &decoded) &&
	       decoded.expected_to_owner_revision == 20 && decoded.capacity_admitted);
	buy.capacity_admitted = false;
	assert(!collector_command_build(&command, operation(), buy, critical_source_site::command,
					critical_deadline_class::interactive));
	buy = purchase();
	buy.from_owner.id = 78;
	assert(!collector_command_build(&command, operation(), buy, critical_source_site::command,
					critical_deadline_class::interactive));
	buy = purchase();
	buy.items[0].parent_item_uid = 999;
	assert(!collector_command_build(&command, operation(), buy, critical_source_site::command,
					critical_deadline_class::interactive));

	auto invalid_tree = collection();
	invalid_tree.items[2].parent_item_uid = 102;
	assert(!collector_command_build(&command, operation(), invalid_tree,
					critical_source_site::recovery,
					critical_deadline_class::background));
	invalid_tree = collection();
	invalid_tree.from_owner = { item_owner_type::player, 42, 0 };
	assert(!collector_command_build(&command, operation(), invalid_tree,
					critical_source_site::recovery,
					critical_deadline_class::background));
	invalid_tree = collection();
	invalid_tree.from_owner.context_id = 1;
	assert(!collector_command_build(&command, operation(), invalid_tree,
					critical_source_site::recovery,
					critical_deadline_class::background));
	invalid_tree = collection();
	invalid_tree.from_owner.id = item_corpse_owner_id(42, 0);
	assert(!collector_command_build(&command, operation(), invalid_tree,
					critical_source_site::recovery,
					critical_deadline_class::background));
	invalid_tree = collection();
	invalid_tree.from_owner.id =
		(static_cast<uint64_t>(static_cast<uint32_t>(INT32_MAX) + 1) << 32) | 3;
	assert(!collector_command_build(&command, operation(), invalid_tree,
					critical_source_site::recovery,
					critical_deadline_class::background));
	invalid_tree = collection();
	invalid_tree.from_owner = { item_owner_type::room, 123, 1 };
	assert(!collector_command_build(&command, operation(), invalid_tree,
					critical_source_site::recovery,
					critical_deadline_class::background));

	collector_command_payload metadata;
	metadata.action = collector_action::activate;
	metadata.listing = 77;
	metadata.expected_listing_revision = 6;
	metadata.observed_at = 87400;
	assert(collector_command_build(&command, operation(), metadata,
				       critical_source_site::recovery,
				       critical_deadline_class::background));
	assert(command.keys.size() == 1 && command.expected_revisions.size() == 1 &&
	       collector_command_decode_payload(command, &decoded));
	metadata.action = collector_action::cancel;
	metadata.cancel_reason = collector::reason::claimed;
	assert(collector_command_build(&command, operation(), metadata,
				       critical_source_site::command,
				       critical_deadline_class::interactive));

	auto cancel_held = purchase();
	cancel_held.action = collector_action::cancel;
	cancel_held.cancel_reason = collector::reason::quarantined;
	cancel_held.target_state = item_custody_state::quarantined;
	cancel_held.capacity_admitted = false;
	cancel_held.actor_pid = 0;
	cancel_held.racewar = 0;
	cancel_held.account_name.fill(0);
	cancel_held.expected_wallet_revision = 0;
	cancel_held.expected_bank_revision = 0;
	cancel_held.to_owner = { item_owner_type::system, 0, 0 };
	assert(collector_command_build(&command, operation(), cancel_held,
				       critical_source_site::operator_repair,
				       critical_deadline_class::recovery));
	cancel_held.cancel_reason = collector::reason::claimed;
	assert(!collector_command_build(&command, operation(), cancel_held,
					critical_source_site::operator_repair,
					critical_deadline_class::recovery));

	auto expire = purchase();
	expire.action = collector_action::expire;
	expire.target_state = item_custody_state::destroyed;
	expire.capacity_admitted = false;
	expire.actor_pid = 0;
	expire.racewar = 0;
	expire.account_name.fill(0);
	expire.expected_wallet_revision = 0;
	expire.expected_bank_revision = 0;
	expire.to_owner = { item_owner_type::destruction, 0, 0 };
	assert(collector_command_build(&command, operation(), expire,
				       critical_source_site::recovery,
				       critical_deadline_class::background));
	assert(command.keys.size() == 3 && collector_command_decode_payload(command, &decoded));

	auto maximum = collection();
	maximum.item_count = COLLECTOR_COMMAND_MAX_ITEMS;
	maximum.selected_item_uid = COLLECTOR_COMMAND_MAX_ITEMS;
	for (uint64_t uid = 1; uid <= COLLECTOR_COMMAND_MAX_ITEMS; ++uid)
		maximum.items[uid - 1] = { uid, 1,   uid == 1 ? uint64_t{ 0 } : uint64_t{ 1 },
					   uid, 500, item_custody_state::active };
	assert(collector_command_build(&command, operation(), maximum,
				       critical_source_site::recovery,
				       critical_deadline_class::background));
	assert(command.keys.size() == COLLECTOR_COMMAND_MAX_ITEMS + 2 &&
	       command.payload.size() < CRITICAL_COMMAND_MAX_PAYLOAD_BYTES);
	command.accepted_at_usec = 2;
	assert(critical_command_valid(command));

	collector_command_result result;
	result.action = collector_action::pause;
	result.record_present = true;
	result.catalog_revision = 12;
	result.from_owner_revision = 1;
	result.to_owner_revision = 2;
	result.wallet = { { 4, 3, 2, 1 } };
	result.bank = { { 8, 7, 6, 5 } };
	result.wallet_revision = 13;
	result.bank_revision = 14;
	result.entry = paused_record();
	std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> encoded_result;
	assert(collector_command_encode_result(result, &encoded_result));
	collector_command_result decoded_result;
	assert(collector_command_decode_result(encoded_result.data(), encoded_result.size(),
					       &decoded_result));
	assert(decoded_result.action == collector_action::pause && decoded_result.record_present &&
	       decoded_result.catalog_revision == 12 && decoded_result.entry.listing == 77 &&
	       decoded_result.entry.holding_paused && decoded_result.wallet.amount[3] == 1 &&
	       decoded_result.bank.amount[0] == 8);

	auto corrupt_result = encoded_result;
	corrupt_result[3] = 1;
	assert(!collector_command_decode_result(corrupt_result.data(), corrupt_result.size(),
						&decoded_result));
	corrupt_result = encoded_result;
	corrupt_result[112 + 152] = static_cast<uint8_t>(collector::state::candidate);
	assert(!collector_command_decode_result(corrupt_result.data(), corrupt_result.size(),
						&decoded_result));
	corrupt_result = encoded_result;
	corrupt_result.back() = 1;
	assert(!collector_command_decode_result(corrupt_result.data(), corrupt_result.size(),
						&decoded_result));

	collector_command_result empty_result;
	empty_result.action = collector_action::collect;
	assert(collector_command_encode_result(empty_result, &encoded_result));
	assert(collector_command_decode_result(encoded_result.data(), encoded_result.size(),
					       &decoded_result) &&
	       !decoded_result.record_present);

	std::cout << "collector command: versioned payloads, disjoint revisions, complete-root "
		     "fences, and canonical results passed\n";
}
