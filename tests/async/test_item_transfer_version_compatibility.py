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
#include "item/quest_reward_continuation.h"

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
void put_u64(std::vector<uint8_t> *bytes, size_t offset, uint64_t value)
{
	for (unsigned int byte = 0; byte < 8; ++byte)
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
	payload.logical_source_id = UINT64_C(0x123456789abcdef0);
	assert(item_transfer_command_build(&command, operation(), payload,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.target_root_item_uid == 700 && decoded.target_parent_item_uid == 700 &&
	       decoded.expected_target_parent_revision == 4 &&
	       decoded.logical_source_id == payload.logical_source_id);
	payload.logical_source_id = 0;

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

	// Soulbind replay retains the replacement policy in the item command so a
	// recipient-side metadata update can resume after the source process exits.
	auto soulbind = trusted_steal;
	soulbind.reason = item_transfer_reason::soulbind;
	soulbind.continuation.kind = item_transfer_continuation_kind::soulbind_transfer;
	soulbind.continuation.data = { 1 };
	assert(item_transfer_command_build(&command, operation(), soulbind,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.reason == item_transfer_reason::soulbind &&
	       decoded.continuation.kind ==
		       item_transfer_continuation_kind::soulbind_transfer &&
	       decoded.continuation.data == std::vector<uint8_t>({ 1 }));
	auto invalid_soulbind = soulbind;
	invalid_soulbind.continuation.data = { 2 };
	assert(!item_transfer_command_build(&command, operation(), invalid_soulbind,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	invalid_soulbind = soulbind;
	invalid_soulbind.continuation.kind = item_transfer_continuation_kind::none;
	invalid_soulbind.continuation.data.clear();
	assert(item_transfer_command_build(&command, operation(), invalid_soulbind,
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
	invalid_trusted_steal = trusted_steal;
	invalid_trusted_steal.logical_source_id = 90001;
	assert(!item_transfer_command_build(&command, operation(), invalid_trusted_steal,
					    critical_source_site::command,
					    critical_deadline_class::interactive));

	// Older batch commands remain replayable: v9 adds a typed continuation,
	// v8 adds a source ID, and v7 adds the collector context length.
	auto version_eight = batch_command;
	version_eight.payload_version = ITEM_TRANSFER_SOURCE_PAYLOAD_VERSION;
	version_eight.payload.resize(version_eight.payload.size() - sizeof(uint32_t) * 2);
	assert(item_transfer_command_decode_payload(version_eight, &decoded));
	assert(decoded.multi_root && decoded.continuation.data.empty());
	auto version_seven = version_eight;
	version_seven.payload_version = ITEM_TRANSFER_COLLECTOR_PAYLOAD_VERSION;
	version_seven.payload.resize(version_seven.payload.size() - sizeof(uint64_t));
	assert(item_transfer_command_decode_payload(version_seven, &decoded));
	assert(decoded.multi_root && decoded.item_count == 2 && !decoded.logical_source_id);
	auto version_six = version_seven;
	version_six.payload_version = ITEM_TRANSFER_BATCH_PAYLOAD_VERSION;
	version_six.payload.resize(version_six.payload.size() - sizeof(uint32_t));
	assert(item_transfer_command_decode_payload(version_six, &decoded));
	assert(decoded.multi_root && decoded.item_count == 2 && !decoded.collector.present);

	auto quest_offering = batch;
	quest_offering.from_owner = { item_owner_type::player, 42, 0 };
	quest_offering.to_owner = { item_owner_type::destruction, 0, 0 };
	quest_offering.reason = item_transfer_reason::quest_turnin;
	quest_offering.reason_id = 711;
	quest_offering.continuation.kind = item_transfer_continuation_kind::quest_offering;
	quest_offering.continuation.data.assign(64, 0);
	put_u32(&quest_offering.continuation.data, 0, 1);
	put_u32(&quest_offering.continuation.data, 4, 42);
	put_u32(&quest_offering.continuation.data, 16, 711);
	put_u32(&quest_offering.continuation.data, 20, 500);
	put_u64(&quest_offering.continuation.data, 24, 1700000000);
	put_u32(&quest_offering.continuation.data, 32, 2);
	put_u64(&quest_offering.continuation.data, 36, 100);
	put_u64(&quest_offering.continuation.data, 44, 200);
	put_u32(&quest_offering.continuation.data, 52, 1);
	put_u32(&quest_offering.continuation.data, 56, 1);
	put_u32(&quest_offering.continuation.data, 60, 777);
	quest_reward_continuation terms = {};
	assert(quest_reward_continuation_decode(quest_offering.continuation.data.data(),
					       quest_offering.continuation.data.size(), &terms));
	assert(terms.player_pid == 42 && terms.mobile_vnum == 711 && terms.room_vnum == 500 &&
	       terms.completed_at == 1700000000 && terms.root_count == 2 &&
	       terms.roots[0] == 100 && terms.roots[1] == 200 &&
	       terms.reward_count == 1 && terms.rewards[0].number == 777);
	auto frozen_credit = quest_offering.continuation.data;
	put_u32(&frozen_credit, 0, 2);
	frozen_credit.resize(100);
	put_u32(&frozen_credit, 64, 4);  // zone
	put_u32(&frozen_credit, 68, 15); // completing player's level
	put_u32(&frozen_credit, 72, 2);  // racewar
	put_u32(&frozen_credit, 76, 2);  // party size
	put_u32(&frozen_credit, 80, 20); // strongest party level
	put_u32(&frozen_credit, 84, 2);  // credited players
	put_u32(&frozen_credit, 88, 42);
	put_u32(&frozen_credit, 92, 43);
	put_u32(&frozen_credit, 96, 3);
	frozen_credit.insert(frozen_credit.end(), {'A', 'd', 'a'});
	const size_t definition_length_offset = frozen_credit.size();
	frozen_credit.resize(frozen_credit.size() + sizeof(uint32_t));
	put_u32(&frozen_credit, definition_length_offset, 3);
	frozen_credit.insert(frozen_credit.end(), {'q', 's', 't'});
	assert(quest_reward_continuation_decode(frozen_credit.data(), frozen_credit.size(),
						&terms));
	assert(terms.version == 2 && terms.zone_number == 4 && terms.player_level == 15 &&
	       terms.player_racewar == 2 && terms.party_size == 2 &&
	       terms.strongest_party_level == 20 && terms.credited_count == 2 &&
	       terms.credited_pids[0] == 42 && terms.credited_pids[1] == 43 &&
	       terms.character_name == "Ada" && terms.definition_id == "qst");
	auto frozen_skill = frozen_credit;
	put_u32(&frozen_skill, 0, 3);
	put_u32(&frozen_skill, 56, 4);
	put_u32(&frozen_skill, 60, 12);
	frozen_skill.insert(frozen_skill.begin() + 64, sizeof(uint32_t), 0);
	put_u32(&frozen_skill, 64, QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION);
	assert(quest_reward_continuation_decode(frozen_skill.data(), frozen_skill.size(), &terms));
	assert(terms.version == 3 && terms.rewards[0].type == 4 &&
	       terms.rewards[0].number == 12 &&
	       terms.rewards[0].flags == QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION);
	auto invalid_skill_flags = frozen_skill;
	put_u32(&invalid_skill_flags, 56, 1);
	assert(!quest_reward_continuation_decode(invalid_skill_flags.data(),
							invalid_skill_flags.size(), &terms));
	auto frozen_xp = frozen_skill;
	put_u32(&frozen_xp, 0, 4);
	put_u32(&frozen_xp, 56, 5);
	put_u32(&frozen_xp, 60, 100);
	put_u32(&frozen_xp, 64, 0);
	frozen_xp.insert(frozen_xp.begin() + 68, sizeof(uint32_t), 0);
	put_u32(&frozen_xp, 68, 75);
	assert(quest_reward_continuation_decode(frozen_xp.data(), frozen_xp.size(), &terms));
	assert(terms.version == 4 && terms.rewards[0].type == 5 &&
	       terms.rewards[0].number == 100 && terms.rewards[0].frozen_amount == 75);
	auto frozen_group_xp = frozen_xp;
	put_u32(&frozen_group_xp, 0, 5);
	const size_t xp_award_count_offset = frozen_group_xp.size();
	frozen_group_xp.resize(frozen_group_xp.size() + 7 * sizeof(uint32_t));
	put_u32(&frozen_group_xp, xp_award_count_offset, 2);
	put_u32(&frozen_group_xp, xp_award_count_offset + 4, 42);
	put_u32(&frozen_group_xp, xp_award_count_offset + 8, 0);
	put_u32(&frozen_group_xp, xp_award_count_offset + 12, 75);
	put_u32(&frozen_group_xp, xp_award_count_offset + 16, 43);
	put_u32(&frozen_group_xp, xp_award_count_offset + 20, 0);
	put_u32(&frozen_group_xp, xp_award_count_offset + 24, 100);
	assert(quest_reward_continuation_decode(frozen_group_xp.data(), frozen_group_xp.size(),
						&terms));
	assert(terms.version == 5 && terms.xp_award_count == 2 &&
	       terms.xp_awards[0].recipient_pid == 42 && terms.xp_awards[0].amount == 75 &&
	       terms.xp_awards[1].recipient_pid == 43 && terms.xp_awards[1].amount == 100);
    auto frozen_daily = frozen_group_xp;
    put_u32(&frozen_daily, 0, 6);
    const size_t daily_offset = frozen_daily.size();
    frozen_daily.resize(daily_offset + 20);
    put_u32(&frozen_daily, daily_offset, 7);
    put_u32(&frozen_daily, daily_offset + 4, 2);
    put_u32(&frozen_daily, daily_offset + 8, 1);
    put_u32(&frozen_daily, daily_offset + 12, 1);
    put_u32(&frozen_daily, daily_offset + 16, 43);
    assert(quest_reward_continuation_decode(frozen_daily.data(), frozen_daily.size(), &terms));
    assert(terms.version == 6 && terms.season_id == 7 && terms.catalog_revision == 2 &&
        terms.daily_count == 1 && terms.daily_pids[0] == 43 && terms.xp_award_count == 2);
    auto foreign_daily = frozen_daily;
    put_u32(&foreign_daily, daily_offset + 16, 99);
    assert(!quest_reward_continuation_decode(foreign_daily.data(), foreign_daily.size(), &terms));
    frozen_daily.pop_back();
    assert(!quest_reward_continuation_decode(frozen_daily.data(), frozen_daily.size(), &terms));
	auto inconsistent_completer_xp = frozen_group_xp;
	put_u32(&inconsistent_completer_xp, xp_award_count_offset + 12, 74);
	assert(!quest_reward_continuation_decode(inconsistent_completer_xp.data(),
							 inconsistent_completer_xp.size(), &terms));
	auto missing_group_xp = frozen_group_xp;
	put_u32(&missing_group_xp, xp_award_count_offset, 1);
	missing_group_xp.resize(missing_group_xp.size() - 3 * sizeof(uint32_t));
	assert(!quest_reward_continuation_decode(missing_group_xp.data(), missing_group_xp.size(),
						 &terms));
	auto invalid_group_xp = frozen_group_xp;
	put_u32(&invalid_group_xp, xp_award_count_offset + 24, 101);
	assert(!quest_reward_continuation_decode(invalid_group_xp.data(), invalid_group_xp.size(),
						 &terms));
	auto invalid_frozen_xp = frozen_xp;
	put_u32(&invalid_frozen_xp, 68, 101);
	assert(!quest_reward_continuation_decode(invalid_frozen_xp.data(),
						invalid_frozen_xp.size(), &terms));
	auto quest_v2 = quest_offering;
	quest_v2.continuation.data = frozen_credit;
	critical_command version_two_command = {};
	assert(item_transfer_command_build(&version_two_command, operation(), quest_v2,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(version_two_command, &decoded));
	assert(decoded.continuation.data == frozen_credit);
	auto duplicate_credit = frozen_credit;
	put_u32(&duplicate_credit, 92, 42);
	assert(!quest_reward_continuation_decode(duplicate_credit.data(),
						 duplicate_credit.size(), &terms));
	auto invalid_reward = quest_offering.continuation.data;
	put_u32(&invalid_reward, 60, 0);
	assert(!quest_reward_continuation_decode(invalid_reward.data(), invalid_reward.size(),
						  &terms));
	assert(item_transfer_command_build(&command, operation(), quest_offering,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.continuation.kind == item_transfer_continuation_kind::quest_offering &&
	       decoded.continuation.data == quest_offering.continuation.data);
	auto truncated_continuation = command;
	truncated_continuation.payload.pop_back();
	assert(!item_transfer_command_decode_payload(truncated_continuation, &decoded));
	auto oversized_continuation = command;
	put_u32(&oversized_continuation.payload,
		oversized_continuation.payload.size() - quest_offering.continuation.data.size() -
			sizeof(uint32_t),
		ITEM_TRANSFER_CONTINUATION_MAX_BYTES + 1);
	assert(!item_transfer_command_decode_payload(oversized_continuation, &decoded));
	auto wrong_root = command;
	put_u64(&wrong_root.payload, wrong_root.payload.size() -
		quest_offering.continuation.data.size() + 36, 999);
	assert(!item_transfer_command_decode_payload(wrong_root, &decoded));
	auto wrong_player = command;
	put_u32(&wrong_player.payload, wrong_player.payload.size() -
		quest_offering.continuation.data.size() + 4, 77);
	assert(!item_transfer_command_decode_payload(wrong_player, &decoded));
	quest_offering.continuation.data.clear();
	assert(!item_transfer_command_build(&command, operation(), quest_offering,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	quest_offering.continuation.data = { 1 };
	quest_offering.reason = item_transfer_reason::player_drop;
	quest_offering.to_owner = { item_owner_type::room, 50, 0 };
	assert(!item_transfer_command_build(&command, operation(), quest_offering,
					    critical_source_site::command,
					    critical_deadline_class::interactive));

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

	item_transfer_payload spell_components = {};
	spell_components.from_owner = { item_owner_type::player, 42, 0 };
	spell_components.to_owner = { item_owner_type::destruction, 0, 0 };
	spell_components.reason = item_transfer_reason::destruction;
	spell_components.reason_id = 200;
	spell_components.expected_from_revision = 12;
	spell_components.expected_to_revision = 4;
	spell_components.multi_root = true;
	spell_components.item_count = 1;
	spell_components.items[0] = { 100, 100, 0, 7, 500, item_custody_state::active };
	spell_components.continuation.kind =
		item_transfer_continuation_kind::spell_component_retirement;
	item_transfer_payload legacy_spell_components = spell_components;
	legacy_spell_components.continuation.data = { 6, 0, 0, 0, 9, 8, 7 };
	assert(item_transfer_command_build(&command, operation(), legacy_spell_components,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.continuation.data == legacy_spell_components.continuation.data);
	spell_components.continuation.data = {
		1, static_cast<uint8_t>(item_spell_component_effect::vines), 0, 0, 0, 3, 9, 8, 7 };
	assert(item_transfer_command_build(&command, operation(), spell_components,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.continuation.kind ==
	       item_transfer_continuation_kind::spell_component_retirement);
	assert(decoded.continuation.data == spell_components.continuation.data);
	spell_components.continuation.data[1] = 99;
	assert(!item_transfer_command_build(&command, operation(), spell_components,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	spell_components.continuation.data[1] =
		static_cast<uint8_t>(item_spell_component_effect::vines);
	spell_components.continuation.data[5] = 2;
	assert(!item_transfer_command_build(&command, operation(), spell_components,
					    critical_source_site::command,
					    critical_deadline_class::interactive));

	item_transfer_payload reward_retirement = {};
	reward_retirement.from_owner = { item_owner_type::player, 42, 0 };
	reward_retirement.to_owner = { item_owner_type::destruction, 0, 0 };
	reward_retirement.reason = item_transfer_reason::destruction;
	reward_retirement.expected_from_revision = 12;
	reward_retirement.expected_to_revision = 4;
	reward_retirement.selected_item_uid = 100;
	reward_retirement.item_count = 1;
	reward_retirement.items[0] = { 100, 800, 900, 7, 500, item_custody_state::active };
	reward_retirement.continuation.kind =
		item_transfer_continuation_kind::account_reward_retirement;
	reward_retirement.continuation.data.resize(28);
	reward_retirement.continuation.data[0] = 2;
	put_u64(&reward_retirement.continuation.data, 8, 991);
	put_u32(&reward_retirement.continuation.data, 16, 500);
	put_u64(&reward_retirement.continuation.data, 20, 100);
	assert(item_transfer_command_build(&command, operation(), reward_retirement,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.continuation.kind ==
	       item_transfer_continuation_kind::account_reward_retirement &&
	       decoded.continuation.data == reward_retirement.continuation.data);
	put_u64(&reward_retirement.continuation.data, 20, 101);
	assert(!item_transfer_command_build(&command, operation(), reward_retirement,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	put_u64(&reward_retirement.continuation.data, 20, 100);
	put_u32(&reward_retirement.continuation.data, 4, 2);
	assert(item_transfer_command_build(&command, operation(), reward_retirement,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	put_u32(&reward_retirement.continuation.data, 4, 0);
	reward_retirement.continuation.data[16] = 1;
	assert(!item_transfer_command_build(&command, operation(), reward_retirement,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	reward_retirement.continuation.data.resize(20);
	reward_retirement.continuation.data[0] = 1;
	put_u32(&reward_retirement.continuation.data, 16, 500);
	reward_retirement.items[0] = { 100, 100, 0, 7, 500, item_custody_state::active };
	assert(item_transfer_command_build(&command, operation(), reward_retirement,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	put_u32(&reward_retirement.continuation.data, 16, 500);
	put_u64(&reward_retirement.continuation.data, 8, 0);
	assert(!item_transfer_command_build(&command, operation(), reward_retirement,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	put_u64(&reward_retirement.continuation.data, 8, 991);
	reward_retirement.multi_root = true;
	assert(!item_transfer_command_build(&command, operation(), reward_retirement,
					    critical_source_site::command,
					    critical_deadline_class::interactive));

	item_transfer_payload reward_promotion = {};
	reward_promotion.from_owner = { item_owner_type::player, 42, 0 };
	reward_promotion.to_owner = { item_owner_type::player, 42, 0 };
	reward_promotion.reason = item_transfer_reason::player_get;
	reward_promotion.reason_id = 100;
	reward_promotion.expected_from_revision = 12;
	reward_promotion.expected_to_revision = 12;
	reward_promotion.multi_root = true;
	reward_promotion.item_count = 1;
	reward_promotion.items[0] = { 100, 800, 900, 7, 500, item_custody_state::active };
	reward_promotion.continuation.kind =
		item_transfer_continuation_kind::account_reward_duplicate_promotion;
	reward_promotion.continuation.data.resize(48);
	put_u32(&reward_promotion.continuation.data, 0, 1);
	put_u64(&reward_promotion.continuation.data, 8, 991);
	put_u32(&reward_promotion.continuation.data, 16, 500);
	put_u64(&reward_promotion.continuation.data, 20, 900);
	put_u32(&reward_promotion.continuation.data, 36, 1);
	put_u64(&reward_promotion.continuation.data, 40, 100);
	assert(item_transfer_command_build(&command, operation(), reward_promotion,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(decoded.continuation.kind ==
	       item_transfer_continuation_kind::account_reward_duplicate_promotion);
	put_u64(&reward_promotion.continuation.data, 40, 101);
	assert(!item_transfer_command_build(&command, operation(), reward_promotion,
					    critical_source_site::command,
					    critical_deadline_class::interactive));
	return 0;
}
'''

assert (
    "command.payload.size() < item_section_size + sizeof(uint32_t)"
    in COMMAND_SOURCE
), "v4-v9 item blob length reads must be bounds-checked"


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

print("[PASS] item-transfer v2-v9 compatibility, source, soulbind, corpse and collector contexts")
