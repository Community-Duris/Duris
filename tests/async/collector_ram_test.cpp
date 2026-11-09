#include "economy/collector_ram.h"
#include "economy/collector_runtime.h"
#include "economy/economic_state.h"
#include "player/player_snapshot_codec.h"

#include <cassert>
#include <cerrno>
#include <cstring>

static critical_command request(const collector_command_payload &payload, uint8_t id)
{
	critical_operation_id operation{};
	operation.bytes[0] = id;
	critical_command command;
	assert(collector_command_build(&command, operation, payload, critical_source_site::command,
				       critical_deadline_class::interactive));
	command.accepted_at_usec = payload.observed_at * 1000000;
	return command;
}

static collector_command_result accept(const collector_command_payload &payload, uint8_t id)
{
	auto command = request(payload, id);
	collector_ram_begin();
	const auto prepared = collector_ram_prepare(command);
	assert(prepared.outcome == critical_apply_outcome::applied && !prepared.error_code);
	assert(collector_ram_seal(command, prepared));
	command.schema_version = CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION;
	command.outcome_revision = prepared.durable_revision;
	command.outcome_payload.assign(prepared.result_payload.begin(),
				       prepared.result_payload.begin() + prepared.result_size);
	auto corrupted = command;
	corrupted.outcome_payload.back() ^= 1;
	assert(!collector_ram_can_publish(corrupted));
	assert(!collector_ram_publish(corrupted));
	assert(collector_ram_publish(command));
	collector_command_result result;
	assert(collector_command_decode_result(prepared.result_payload.data(), prepared.result_size,
					       &result));
	if (payload.item_count)
		assert(item_ownership_runtime_apply_collector(payload, result));
	return result;
}

int main()
{
	item_ownership_runtime_reset();
	collector_runtime_reset();
	economic_state_reset();
	collector_ram_set_flatfile(false);
	collector::rules rules{ true, 10, 20, 100, 200, 1 };
	collector_death_snapshot death;
	death.operation_id.bytes[0] = 9;
	death.beneficiary_pid = 42;
	death.death_time = 100;
	death.policy = rules;
	char hex[CRITICAL_COMMAND_ID_HEX_SIZE]{};
	assert(critical_operation_id_to_hex(death.operation_id, hex, sizeof(hex)));
	collector::record candidate;
	assert(collector::enroll(1, hex, 42, 101, 5, 100, rules, &candidate) ==
	       collector::outcome::applied);
	collector::catalog initial{ 1, 2, { candidate } };
	assert(collector_runtime_rebuild_authoritative(initial, nullptr, 0, &death, 1));
	const item_owner_identity room{ item_owner_type::room, 300, 0 };
	const item_owner_identity holding{ item_owner_type::collector, 1, 0 };
	assert(item_ownership_runtime_hydrate(
		{ 100, 100, 0, room, 4, 7, 1000, item_custody_state::active }));
	assert(item_ownership_runtime_hydrate(
		{ 101, 100, 100, room, 5, 7, 1001, item_custody_state::active }));
	player_item_snapshot item;
	item.object_uid = 101;
	item.vnum = 1001;
	item.cost = 100;
	item.weight = 1;
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = 0;
	std::vector<uint8_t> blob;
	assert(player_item_snapshot_list_encode({ item }, &blob) ==
	       player_snapshot_codec_result::ok);
	collector_command_payload collect;
	collect.action = collector_action::collect;
	collect.listing = 1;
	collect.expected_listing_revision = 1;
	collect.observed_at = 110;
	collect.from_owner = room;
	collect.to_owner = holding;
	collect.target_state = item_custody_state::active;
	collect.expected_from_owner_revision = 7;
	collect.selected_item_uid = 101;
	collect.item_count = 2;
	collect.items[0] = { 100, 100, 0, 4, 1000, item_custody_state::active };
	collect.items[1] = { 101, 100, 100, 5, 1001, item_custody_state::active };
	collect.item_blob_size = blob.size();
	std::copy(blob.begin(), blob.end(), collect.item_blob.begin());
	collector_ram_begin();
	auto prepared = collector_ram_prepare(request(collect, 1));
	assert(prepared.outcome == critical_apply_outcome::applied);
	collector::record unchanged;
	assert(collector_runtime_find(1, &unchanged) &&
	       unchanged.status == collector::state::candidate);
	// Abandoned admission leaves both custody and catalog untouched.
	collector_ram_begin();
	item_ownership_runtime_entry custody;
	assert(item_ownership_runtime_lookup(101, &custody) &&
	       custody.owner.type == item_owner_type::room);
	auto collected = accept(collect, 2);
	assert(collected.entry.price_value == 200 && collected.entry.item_revision == 6);
	collector_command_payload activate;
	activate.action = collector_action::activate;
	activate.listing = 1;
	activate.expected_listing_revision = 2;
	activate.observed_at = 119;
	collector_ram_begin();
	assert(collector_ram_prepare(request(activate, 3)).error_code == EAGAIN);
	activate.observed_at = 120;
	auto available = accept(activate, 4);
	assert(available.entry.expires_at == 220 && available.entry.revision == 3);
	collector_command_payload buy;
	buy.action = collector_action::purchase;
	buy.listing = 1;
	buy.expected_listing_revision = 3;
	buy.observed_at = 121;
	buy.actor_pid = 42;
	buy.racewar = 1;
	strcpy(buy.account_name.data(), "buyer");
	buy.capacity_admitted = true;
	buy.from_owner = holding;
	buy.to_owner = { item_owner_type::player, 42, 0 };
	buy.target_state = item_custody_state::active;
	buy.expected_from_owner_revision = 1;
	buy.selected_item_uid = 101;
	buy.item_count = 1;
	buy.items[0] = { 101, 101, 0, 6, 1001, item_custody_state::active };
	buy.item_blob_size = blob.size();
	std::copy(blob.begin(), blob.end(), buy.item_blob.begin());
	assert(economic_state_seed({ 42, "buyer", "Buyer", 1, { { { 0, 0, 0, 1 } }, {}, 0, 0 } }));
	auto purchased = accept(buy, 5);
	assert(purchased.entry.status == collector::state::purchased &&
	       purchased.wallet.amount[2] == 8 && purchased.wallet.amount[3] == 0);
	assert(purchased.materialized_item_id == 0 && purchased.wallet_revision == 1 &&
	       purchased.bank_revision == 1);
	assert(item_ownership_runtime_lookup(101, &custody) &&
	       custody.owner.type == item_owner_type::player && custody.item_revision == 7);
	collector_ram_begin();
	assert(collector_ram_prepare(request(buy, 6)).error_code == ESTALE);
}
