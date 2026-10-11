#ifndef QUEST_REWARD_CONTINUATION_H
#define QUEST_REWARD_CONTINUATION_H

#include "item/item_transfer_command.h"
#include "economy/economic_accounting_plan.h"
#include <algorithm>

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <string>
#include <openssl/sha.h>

struct quest_reward_goal
{
	uint32_t type = 0;
	uint32_t number = 0;
	uint32_t flags = 0;
	uint32_t frozen_amount = 0;
};

struct quest_reward_xp_award
{
	uint32_t recipient_pid = 0;
	uint32_t reward_index = 0;
	uint32_t amount = 0;
};

struct quest_reward_continuation
{
	uint32_t version = 0;
	uint32_t player_pid = 0;
	uint32_t quester_id = 0;
	uint32_t completion_index = 0;
	uint32_t mobile_vnum = 0;
	uint32_t room_vnum = 0;
	uint64_t completed_at = 0;
	uint32_t root_count = 0;
	std::array<uint64_t, 14> roots = {};
	uint32_t reward_count = 0;
	std::array<quest_reward_goal, 64> rewards = {};
	uint32_t zone_number = 0;
	int32_t player_level = 0;
	int32_t player_racewar = 0;
	uint32_t party_size = 0;
	int32_t strongest_party_level = 0;
	uint32_t credited_count = 0;
	std::array<uint32_t, 64> credited_pids = {};
	uint32_t xp_award_count = 0;
	std::array<quest_reward_xp_award, 64> xp_awards = {};
	std::string character_name;
	std::string definition_id;
	// Explicit fee-only v6 authority, never a synthetic consumed item root.
	critical_operation_id action_operation{}, triggering_acceptance{};
	economic_source_event action_source{};
	uint64_t action_mobile_instance_id = 0;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> original_acceptance_result{};
};

constexpr size_t QUEST_REWARD_MAX_CREDITED_PIDS = 64;
constexpr size_t QUEST_REWARD_MAX_CHARACTER_NAME_BYTES = 64;
constexpr size_t QUEST_REWARD_MAX_DEFINITION_ID_BYTES = 4096;
constexpr uint32_t QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION = 1U << 0;
constexpr uint32_t QUEST_REWARD_CURRENCY_OPERATION_DOMAIN = 0x51524352;

// A consumed UID identifies a one-use quest instance. The same source is used
// by admission and native receipt verification, including repeated item goals.
inline uint64_t quest_item_reward_source_id(uint64_t offering_uid, int vnum,
					    uint32_t duplicate_ordinal)
{
	if (!offering_uid || vnum <= 0)
		return 0;
	uint8_t source[24] = { 'q', 'u', 'e', 's', 't', '-', 'v', '1' };
	for (size_t index = 0; index < 8; ++index)
		source[8 + index] = static_cast<uint8_t>(offering_uid >> (index * 8));
	for (size_t index = 0; index < 4; ++index)
	{
		source[16 + index] =
			static_cast<uint8_t>(static_cast<uint32_t>(vnum) >> (index * 8));
		source[20 + index] = static_cast<uint8_t>(duplicate_ordinal >> (index * 8));
	}
	uint8_t digest[SHA256_DIGEST_LENGTH] = {};
	SHA256(source, sizeof(source), digest);
	uint64_t source_id = 0;
	for (size_t index = 0; index < 8; ++index)
		source_id = (source_id << 8) | digest[index];
	source_id &= INT64_MAX;
	return source_id ? source_id : 1;
}

// v6 derives logical reward source from the actual admitted action and its
// one bound event; the produced item UID still comes from genuine birth reserve.
inline uint64_t quest_fee_item_reward_source_id(const quest_reward_continuation &terms,
						size_t index)
{
	if (terms.version != 6 || terms.root_count || index >= terms.reward_count ||
	    terms.rewards[index].type != 1U || !terms.rewards[index].number ||
	    critical_operation_id_is_zero(terms.action_operation) ||
	    terms.action_source.kind != economic_source_kind::quest_action ||
	    terms.action_source.source.bytes != terms.action_operation.bytes)
		return 0;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> event{};
	if (economic_source_event_encode(terms.action_source, &event) !=
	    economic_accounting_error::ok)
		return 0;
	uint32_t ordinal = 0;
	for (size_t prior = 0; prior < index; ++prior)
		if (terms.rewards[prior].type == 1U &&
		    terms.rewards[prior].number == terms.rewards[index].number)
			++ordinal;
	std::array<uint8_t, 88> source{};
	const char tag[] = "quest-fee-v6";
	std::copy_n(reinterpret_cast<const uint8_t *>(tag), sizeof(tag) - 1, source.begin());
	std::copy(terms.action_operation.bytes.begin(), terms.action_operation.bytes.end(),
		  source.begin() + 16);
	std::copy(event.begin(), event.end(), source.begin() + 32);
	for (size_t i = 0; i < 4; ++i)
	{
		source[80 + i] = static_cast<uint8_t>(terms.rewards[index].number >> (i * 8));
		source[84 + i] = static_cast<uint8_t>(ordinal >> (i * 8));
	}
	uint8_t digest[SHA256_DIGEST_LENGTH]{};
	SHA256(source.data(), source.size(), digest);
	uint64_t result = 0;
	for (size_t i = 0; i < 8; ++i)
		result = (result << 8) | digest[i];
	result &= INT64_MAX;
	return result ? result : 1;
}

inline uint64_t quest_item_reward_source_id(const quest_reward_continuation &terms, size_t index)
{
	if (terms.version == 6)
		return quest_fee_item_reward_source_id(terms, index);
	if (!terms.root_count || index >= terms.reward_count || terms.rewards[index].type != 1U)
		return 0;
	uint32_t ordinal = 0;
	for (size_t prior = 0; prior < index; ++prior)
		if (terms.rewards[prior].type == 1U &&
		    terms.rewards[prior].number == terms.rewards[index].number)
			++ordinal;
	return quest_item_reward_source_id(terms.roots[0],
					   static_cast<int>(terms.rewards[index].number), ordinal);
}

// Version 1 retained exact offering terms. Version 2 also freezes the credited
// party and stable quest identity. Version 3 adds skill eligibility metadata.
// Version 4 freezes solo quest XP. Version 5 freezes per-recipient XP values
// for the already captured group, so recovery need not rediscover the party.
inline bool quest_fee_reward_continuation_decode(const uint8_t *, size_t,
						 quest_reward_continuation *);
inline bool quest_reward_continuation_decode(const uint8_t *data, size_t size,
					     quest_reward_continuation *decoded)
{
	if (data && size >= 4 && data[0] == 6 && !data[1] && !data[2] && !data[3])
		return quest_fee_reward_continuation_decode(data, size, decoded);
	if (!data || !decoded || size < 40 || size > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
		return false;
	auto read32 = [&](size_t offset)
	{
		uint32_t value = 0;
		for (size_t byte = 0; byte < 4; ++byte)
			value |= static_cast<uint32_t>(data[offset + byte]) << (byte * 8);
		return value;
	};
	auto read64 = [&](size_t offset)
	{
		uint64_t value = 0;
		for (size_t byte = 0; byte < 8; ++byte)
			value |= static_cast<uint64_t>(data[offset + byte]) << (byte * 8);
		return value;
	};
	const uint32_t version = read32(0);
	if (version != 1 && version != 2 && version != 3 && version != 4 && version != 5)
		return false;
	quest_reward_continuation value;
	value.version = version;
	value.player_pid = read32(4);
	value.quester_id = read32(8);
	value.completion_index = read32(12);
	value.mobile_vnum = read32(16);
	value.room_vnum = read32(20);
	value.completed_at = read64(24);
	value.root_count = read32(32);
	if (!value.player_pid || value.quester_id > INT32_MAX ||
	    value.completion_index > INT32_MAX || !value.mobile_vnum ||
	    value.mobile_vnum > INT32_MAX || !value.room_vnum || value.room_vnum > INT32_MAX ||
	    !value.completed_at || value.completed_at > INT64_MAX || !value.root_count ||
	    value.root_count > value.roots.size() ||
	    size < 40 + static_cast<size_t>(value.root_count) * sizeof(uint64_t))
		return false;
	size_t offset = 36;
	for (size_t index = 0; index < value.root_count; ++index, offset += 8)
	{
		value.roots[index] = read64(offset);
		if (!value.roots[index])
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (value.roots[prior] == value.roots[index])
				return false;
	}
	value.reward_count = read32(offset);
	offset += 4;
	if (value.reward_count > value.rewards.size())
		return false;
	const size_t reward_record_bytes = version >= 4 ? 16 : (version >= 3 ? 12 : 8);
	const size_t reward_bytes = static_cast<size_t>(value.reward_count) * reward_record_bytes;
	if (size - offset < reward_bytes || (version == 1 && size - offset != reward_bytes))
		return false;
	for (size_t index = 0; index < value.reward_count; ++index, offset += reward_record_bytes)
	{
		value.rewards[index] = { read32(offset), read32(offset + 4),
					 version >= 3 ? read32(offset + 8) : 0,
					 version >= 4 ? read32(offset + 12) : 0 };
		const auto goal = value.rewards[index];
		if ((goal.type != 1 && goal.type != 3 && goal.type != 4 && goal.type != 5) ||
		    goal.number > INT32_MAX || goal.frozen_amount > INT32_MAX ||
		    (goal.type != 4 && !goal.number) ||
		    (goal.flags & ~QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION) ||
		    (goal.type != 4 && goal.flags) || (version < 3 && goal.flags) ||
		    (version >= 4 ? (goal.type == 5 ? (!goal.frozen_amount ||
						       goal.frozen_amount > goal.number) :
						      goal.frozen_amount != 0) :
				    goal.frozen_amount != 0))
			return false;
	}
	if (version >= 2)
	{
		constexpr size_t extension_header_bytes = 6 * sizeof(uint32_t);
		if (size - offset < extension_header_bytes)
			return false;
		value.zone_number = read32(offset);
		value.player_level = static_cast<int32_t>(read32(offset + 4));
		value.player_racewar = static_cast<int32_t>(read32(offset + 8));
		value.party_size = read32(offset + 12);
		value.strongest_party_level = static_cast<int32_t>(read32(offset + 16));
		value.credited_count = read32(offset + 20);
		offset += extension_header_bytes;
		if (!value.zone_number || value.zone_number > INT32_MAX || value.player_level < 0 ||
		    value.strongest_party_level < 0 || !value.credited_count ||
		    value.credited_count > QUEST_REWARD_MAX_CREDITED_PIDS ||
		    value.party_size != value.credited_count ||
		    size - offset < static_cast<size_t>(value.credited_count) * sizeof(uint32_t) +
					    2 * sizeof(uint32_t))
			return false;
		bool includes_player = false;
		for (size_t index = 0; index < value.credited_count;
		     ++index, offset += sizeof(uint32_t))
		{
			const uint32_t pid = read32(offset);
			if (!pid)
				return false;
			for (size_t prior = 0; prior < index; ++prior)
				if (value.credited_pids[prior] == pid)
					return false;
			value.credited_pids[index] = pid;
			includes_player = includes_player || pid == value.player_pid;
		}
		if (!includes_player)
			return false;
		const uint32_t name_length = read32(offset);
		offset += sizeof(uint32_t);
		if (!name_length || name_length > QUEST_REWARD_MAX_CHARACTER_NAME_BYTES ||
		    size - offset < static_cast<size_t>(name_length) + sizeof(uint32_t))
			return false;
		value.character_name.assign(reinterpret_cast<const char *>(data + offset),
					    name_length);
		if (value.character_name.find('\0') != std::string::npos)
			return false;
		offset += name_length;
		const uint32_t definition_length = read32(offset);
		offset += sizeof(uint32_t);
		if (!definition_length ||
		    definition_length > QUEST_REWARD_MAX_DEFINITION_ID_BYTES ||
		    size < offset + static_cast<size_t>(definition_length))
			return false;
		value.definition_id.assign(reinterpret_cast<const char *>(data + offset),
					   definition_length);
		for (unsigned char character : value.definition_id)
			if (character < 0x21 || character > 0x7e)
				return false;
		offset += definition_length;
		if (version >= 5)
		{
			if (value.credited_pids[0] != value.player_pid)
				return false;
			if (size - offset < sizeof(uint32_t))
				return false;
			value.xp_award_count = read32(offset);
			offset += sizeof(uint32_t);
			if (value.xp_award_count > value.xp_awards.size() ||
			    size != offset + static_cast<size_t>(value.xp_award_count) * 3 *
						     sizeof(uint32_t))
				return false;
			for (size_t index = 0; index < value.xp_award_count;
			     ++index, offset += 3 * sizeof(uint32_t))
			{
				auto &award = value.xp_awards[index];
				award = { read32(offset), read32(offset + 4), read32(offset + 8) };
				if (!award.recipient_pid ||
				    award.reward_index >= value.reward_count || !award.amount ||
				    award.amount > value.rewards[award.reward_index].number ||
				    value.rewards[award.reward_index].type != 5U)
					return false;
				if (award.recipient_pid == value.player_pid &&
				    award.amount != value.rewards[award.reward_index].frozen_amount)
					return false;
				bool recipient_found = false;
				for (size_t recipient = 0; recipient < value.credited_count;
				     ++recipient)
					recipient_found = recipient_found ||
							  value.credited_pids[recipient] ==
								  award.recipient_pid;
				if (!recipient_found)
					return false;
				for (size_t prior = 0; prior < index; ++prior)
					if (value.xp_awards[prior].recipient_pid ==
						    award.recipient_pid &&
					    value.xp_awards[prior].reward_index ==
						    award.reward_index)
						return false;
			}
			for (size_t reward = 0; reward < value.reward_count; ++reward)
				if (value.rewards[reward].type == 5U)
					for (size_t recipient = 0; recipient < value.credited_count;
					     ++recipient)
					{
						bool award_found = false;
						for (size_t index = 0; index < value.xp_award_count;
						     ++index)
							award_found =
								award_found ||
								(value.xp_awards[index]
										 .recipient_pid ==
									 value.credited_pids
										 [recipient] &&
								 value.xp_awards[index]
										 .reward_index ==
									 reward);
						if (!award_found)
							return false;
					}
		}
		if (version < 5 && size != offset)
			return false;
	}
	else if (size != offset)
		return false;
	*decoded = value;
	return true;
}

inline bool quest_fee_reward_continuation_decode(const uint8_t *data, size_t size,
						 quest_reward_continuation *decoded)
{
	if (!data || !decoded || size < 40 || size > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
		return false;
	auto read32 = [&](size_t offset)
	{
		uint32_t value = 0;
		for (size_t byte = 0; byte < 4; ++byte)
			value |= static_cast<uint32_t>(data[offset + byte]) << (byte * 8);
		return value;
	};
	auto read64 = [&](size_t offset)
	{
		uint64_t value = 0;
		for (size_t byte = 0; byte < 8; ++byte)
			value |= static_cast<uint64_t>(data[offset + byte]) << (byte * 8);
		return value;
	};
	const uint32_t version = read32(0);
	if (version != 6)
		return false;
	quest_reward_continuation value;
	value.version = version;
	value.player_pid = read32(4);
	value.quester_id = read32(8);
	value.completion_index = read32(12);
	value.mobile_vnum = read32(16);
	value.room_vnum = read32(20);
	value.completed_at = read64(24);
	value.root_count = read32(32);
	if (!value.player_pid || value.quester_id > INT32_MAX ||
	    value.completion_index > INT32_MAX || !value.mobile_vnum ||
	    value.mobile_vnum > INT32_MAX || !value.room_vnum || value.room_vnum > INT32_MAX ||
	    !value.completed_at || value.completed_at > INT64_MAX || value.root_count ||
	    value.root_count > value.roots.size() ||
	    size < 40 + static_cast<size_t>(value.root_count) * sizeof(uint64_t))
		return false;
	size_t offset = 36;
	for (size_t index = 0; index < value.root_count; ++index, offset += 8)
	{
		value.roots[index] = read64(offset);
		if (!value.roots[index])
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (value.roots[prior] == value.roots[index])
				return false;
	}
	value.reward_count = read32(offset);
	offset += 4;
	if (value.reward_count > value.rewards.size())
		return false;
	const size_t reward_record_bytes = version >= 4 ? 16 : (version >= 3 ? 12 : 8);
	const size_t reward_bytes = static_cast<size_t>(value.reward_count) * reward_record_bytes;
	if (size - offset < reward_bytes || (version == 1 && size - offset != reward_bytes))
		return false;
	for (size_t index = 0; index < value.reward_count; ++index, offset += reward_record_bytes)
	{
		value.rewards[index] = { read32(offset), read32(offset + 4),
					 version >= 3 ? read32(offset + 8) : 0,
					 version >= 4 ? read32(offset + 12) : 0 };
		const auto goal = value.rewards[index];
		if ((goal.type != 1 && goal.type != 3 && goal.type != 4 && goal.type != 5) ||
		    goal.number > INT32_MAX || goal.frozen_amount > INT32_MAX ||
		    (goal.type != 4 && !goal.number) ||
		    (goal.flags & ~QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION) ||
		    (goal.type != 4 && goal.flags) || (version < 3 && goal.flags) ||
		    (version >= 4 ? (goal.type == 5 ? (!goal.frozen_amount ||
						       goal.frozen_amount > goal.number) :
						      goal.frozen_amount != 0) :
				    goal.frozen_amount != 0))
			return false;
	}
	if (version >= 2)
	{
		constexpr size_t extension_header_bytes = 6 * sizeof(uint32_t);
		if (size - offset < extension_header_bytes)
			return false;
		value.zone_number = read32(offset);
		value.player_level = static_cast<int32_t>(read32(offset + 4));
		value.player_racewar = static_cast<int32_t>(read32(offset + 8));
		value.party_size = read32(offset + 12);
		value.strongest_party_level = static_cast<int32_t>(read32(offset + 16));
		value.credited_count = read32(offset + 20);
		offset += extension_header_bytes;
		if (!value.zone_number || value.zone_number > INT32_MAX || value.player_level < 0 ||
		    value.strongest_party_level < 0 || !value.credited_count ||
		    value.credited_count > QUEST_REWARD_MAX_CREDITED_PIDS ||
		    value.party_size != value.credited_count ||
		    size - offset < static_cast<size_t>(value.credited_count) * sizeof(uint32_t) +
					    2 * sizeof(uint32_t))
			return false;
		bool includes_player = false;
		for (size_t index = 0; index < value.credited_count;
		     ++index, offset += sizeof(uint32_t))
		{
			const uint32_t pid = read32(offset);
			if (!pid)
				return false;
			for (size_t prior = 0; prior < index; ++prior)
				if (value.credited_pids[prior] == pid)
					return false;
			value.credited_pids[index] = pid;
			includes_player = includes_player || pid == value.player_pid;
		}
		if (!includes_player)
			return false;
		const uint32_t name_length = read32(offset);
		offset += sizeof(uint32_t);
		if (!name_length || name_length > QUEST_REWARD_MAX_CHARACTER_NAME_BYTES ||
		    size - offset < static_cast<size_t>(name_length) + sizeof(uint32_t))
			return false;
		value.character_name.assign(reinterpret_cast<const char *>(data + offset),
					    name_length);
		if (value.character_name.find('\0') != std::string::npos)
			return false;
		offset += name_length;
		const uint32_t definition_length = read32(offset);
		offset += sizeof(uint32_t);
		if (!definition_length ||
		    definition_length > QUEST_REWARD_MAX_DEFINITION_ID_BYTES ||
		    size < offset + static_cast<size_t>(definition_length))
			return false;
		value.definition_id.assign(reinterpret_cast<const char *>(data + offset),
					   definition_length);
		for (unsigned char character : value.definition_id)
			if (character < 0x21 || character > 0x7e)
				return false;
		offset += definition_length;
		if (version >= 5)
		{
			if (value.credited_pids[0] != value.player_pid)
				return false;
			if (size - offset < sizeof(uint32_t))
				return false;
			value.xp_award_count = read32(offset);
			offset += sizeof(uint32_t);
			if (value.xp_award_count > value.xp_awards.size() ||
			    size < offset + static_cast<size_t>(value.xp_award_count) * 3 *
						    sizeof(uint32_t))
				return false;
			for (size_t index = 0; index < value.xp_award_count;
			     ++index, offset += 3 * sizeof(uint32_t))
			{
				auto &award = value.xp_awards[index];
				award = { read32(offset), read32(offset + 4), read32(offset + 8) };
				if (!award.recipient_pid ||
				    award.reward_index >= value.reward_count || !award.amount ||
				    award.amount > value.rewards[award.reward_index].number ||
				    value.rewards[award.reward_index].type != 5U)
					return false;
				if (award.recipient_pid == value.player_pid &&
				    award.amount != value.rewards[award.reward_index].frozen_amount)
					return false;
				bool recipient_found = false;
				for (size_t recipient = 0; recipient < value.credited_count;
				     ++recipient)
					recipient_found = recipient_found ||
							  value.credited_pids[recipient] ==
								  award.recipient_pid;
				if (!recipient_found)
					return false;
				for (size_t prior = 0; prior < index; ++prior)
					if (value.xp_awards[prior].recipient_pid ==
						    award.recipient_pid &&
					    value.xp_awards[prior].reward_index ==
						    award.reward_index)
						return false;
			}
			for (size_t reward = 0; reward < value.reward_count; ++reward)
				if (value.rewards[reward].type == 5U)
					for (size_t recipient = 0; recipient < value.credited_count;
					     ++recipient)
					{
						bool award_found = false;
						for (size_t index = 0; index < value.xp_award_count;
						     ++index)
							award_found =
								award_found ||
								(value.xp_awards[index]
										 .recipient_pid ==
									 value.credited_pids
										 [recipient] &&
								 value.xp_awards[index]
										 .reward_index ==
									 reward);
						if (!award_found)
							return false;
					}
		}
		if (version < 5 && size != offset)
			return false;
	}
	else if (size != offset)
		return false;
	constexpr size_t tail_fixed =
		4 + 16 + ECONOMIC_SOURCE_EVENT_BYTES + 8 + 16 + ITEM_TRANSFER_RESULT_BYTES;
	if (size - offset < tail_fixed || data[offset] != 'Q' || data[offset + 1] != 'R' ||
	    data[offset + 2] != 'F' || data[offset + 3] != '6')
		return false;
	offset += 4;
	std::copy_n(data + offset, 16, value.action_operation.bytes.begin());
	offset += 16;
	if (economic_source_event_decode({ data + offset, ECONOMIC_SOURCE_EVENT_BYTES },
					 &value.action_source) != economic_accounting_error::ok)
		return false;
	offset += ECONOMIC_SOURCE_EVENT_BYTES;
	value.action_mobile_instance_id = read64(offset);
	offset += 8;
	std::copy_n(data + offset, 16, value.triggering_acceptance.bytes.begin());
	offset += 16;
	if (size - offset != ITEM_TRANSFER_RESULT_BYTES ||
	    critical_operation_id_is_zero(value.action_operation) ||
	    critical_operation_id_is_zero(value.triggering_acceptance) ||
	    value.action_operation.bytes == value.triggering_acceptance.bytes ||
	    value.action_source.kind != economic_source_kind::quest_action ||
	    value.action_source.source.bytes != value.action_operation.bytes ||
	    value.action_source.slot != value.completion_index || !value.action_source.sequence ||
	    !value.action_mobile_instance_id || value.action_mobile_instance_id == UINT64_MAX)
		return false;
	std::copy_n(data + offset, ITEM_TRANSFER_RESULT_BYTES,
		    value.original_acceptance_result.begin());
	item_transfer_result result{};
	if (!item_transfer_command_decode_result(value.original_acceptance_result.data(),
						 value.original_acceptance_result.size(),
						 &result) ||
	    !result.root_item_uid || !result.item_count || result.corpse_revision ||
	    result.collector_catalog_changed)
		return false;
	*decoded = std::move(value);
	return true;
}

// Requires the actual full original command borrowed from its retained native
// parent carrier. v6 numeric terms and typed48 alone never prove acceptance.
inline bool quest_fee_reward_trigger_binding_valid(const quest_reward_continuation &value,
						   const critical_command &acceptance)
{
	if (value.version != 6 || value.root_count)
		return false;
	item_transfer_payload offered{};
	item_transfer_result result{};
	if (acceptance.operation_id.bytes != value.triggering_acceptance.bytes ||
	    acceptance.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    acceptance.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION ||
	    !acceptance.accepted_at_usec || !acceptance.publication_required ||
	    !item_transfer_command_decode_payload(acceptance, &offered) ||
	    !item_transfer_native_mobile_recovery_shape_valid(offered) ||
	    offered.native_mobile.action != item_native_mobile_action::acceptance ||
	    offered.native_mobile.final_giver_pid != value.player_pid ||
	    offered.native_mobile.reference.mobile_instance_id != value.action_mobile_instance_id ||
	    offered.native_mobile.reference.mobile_vnum !=
		    static_cast<int32_t>(value.mobile_vnum) ||
	    offered.native_mobile.reference.birth_source.generation.bytes !=
		    value.action_source.generation.bytes ||
	    offered.native_mobile.reference.mobile_revision == UINT64_MAX ||
	    value.action_source.sequence < offered.native_mobile.reference.mobile_revision + 1 ||
	    !item_transfer_command_decode_result(value.original_acceptance_result.data(),
						 value.original_acceptance_result.size(),
						 &result) ||
	    result.root_item_uid != item_transfer_result_root(offered) ||
	    result.item_count != offered.item_count ||
	    offered.expected_from_revision == UINT64_MAX ||
	    offered.expected_to_revision == UINT64_MAX ||
	    result.from_owner_revision != offered.expected_from_revision + 1 ||
	    result.to_owner_revision != offered.expected_to_revision + 1 ||
	    result.corpse_revision || result.collector_catalog_changed)
		return false;
	uint64_t maximum = 0;
	for (size_t i = 0; i < offered.item_count; ++i)
	{
		if (offered.items[i].expected_item_revision == UINT64_MAX)
			return false;
		maximum = std::max(maximum, offered.items[i].expected_item_revision + 1);
	}
	if (result.max_item_revision != maximum)
		return false;
	return true;
}

// Distinct bounded v6 encoder. Original v1–5 encoding is not selected here.
// Immutable numeric terms bind the exact acceptance op/typed48 dependency. Full
// commands remain in the original native carrier; this grants no SQL authority.
inline bool quest_fee_reward_continuation_encode(const quest_reward_continuation &terms,
						 std::vector<uint8_t> *encoded)
try
{
	if (!encoded || terms.version != 6 || terms.root_count ||
	    terms.reward_count > terms.rewards.size() ||
	    terms.credited_count > terms.credited_pids.size() ||
	    terms.xp_award_count > terms.xp_awards.size() ||
	    terms.character_name.size() > QUEST_REWARD_MAX_CHARACTER_NAME_BYTES ||
	    terms.definition_id.size() > QUEST_REWARD_MAX_DEFINITION_ID_BYTES)
		return false;
	std::vector<uint8_t> value;
	auto put32 = [&](uint32_t v)
	{
		for (size_t i = 0; i < 4; ++i)
			value.push_back(static_cast<uint8_t>(v >> (8 * i)));
	};
	auto put64 = [&](uint64_t v)
	{
		for (size_t i = 0; i < 8; ++i)
			value.push_back(static_cast<uint8_t>(v >> (8 * i)));
	};
	put32(6);
	put32(terms.player_pid);
	put32(terms.quester_id);
	put32(terms.completion_index);
	put32(terms.mobile_vnum);
	put32(terms.room_vnum);
	put64(terms.completed_at);
	put32(0);
	put32(terms.reward_count);
	for (size_t i = 0; i < terms.reward_count; ++i)
	{
		const auto &r = terms.rewards[i];
		put32(r.type);
		put32(r.number);
		put32(r.flags);
		put32(r.frozen_amount);
	}
	put32(terms.zone_number);
	put32(static_cast<uint32_t>(terms.player_level));
	put32(static_cast<uint32_t>(terms.player_racewar));
	put32(terms.party_size);
	put32(static_cast<uint32_t>(terms.strongest_party_level));
	put32(terms.credited_count);
	for (size_t i = 0; i < terms.credited_count; ++i)
		put32(terms.credited_pids[i]);
	put32(static_cast<uint32_t>(terms.character_name.size()));
	value.insert(value.end(), terms.character_name.begin(), terms.character_name.end());
	put32(static_cast<uint32_t>(terms.definition_id.size()));
	value.insert(value.end(), terms.definition_id.begin(), terms.definition_id.end());
	put32(terms.xp_award_count);
	for (size_t i = 0; i < terms.xp_award_count; ++i)
	{
		const auto &a = terms.xp_awards[i];
		put32(a.recipient_pid);
		put32(a.reward_index);
		put32(a.amount);
	}
	value.insert(value.end(), { 'Q', 'R', 'F', '6' });
	value.insert(value.end(), terms.action_operation.bytes.begin(),
		     terms.action_operation.bytes.end());
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
	if (economic_source_event_encode(terms.action_source, &source) !=
	    economic_accounting_error::ok)
		return false;
	value.insert(value.end(), source.begin(), source.end());
	put64(terms.action_mobile_instance_id);
	value.insert(value.end(), terms.triggering_acceptance.bytes.begin(),
		     terms.triggering_acceptance.bytes.end());
	value.insert(value.end(), terms.original_acceptance_result.begin(),
		     terms.original_acceptance_result.end());
	if (value.size() > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
		return false;
	quest_reward_continuation decoded;
	if (!quest_fee_reward_continuation_decode(value.data(), value.size(), &decoded))
		return false;
	*encoded = std::move(value);
	return true;
}
catch (const std::bad_alloc &)
{
	return false;
}

// Unselected full continuation parser companions. The authentic outer owns
// input, destination (including both prior string heaps), and caller storage.
#include <new>
#include <utility>
namespace duris_quest_continuation_bounded_detail
{
using reserve_fn = bool (*)(size_t, void *) noexcept;
inline bool add(size_t &value, size_t extra) noexcept
{
	if (extra > SIZE_MAX - value)
		return false;
	value += extra;
	return true;
}
inline bool string_heap(const std::string &value, size_t &bytes) noexcept
{
	return value.capacity() <= 15 ||
	       (value.capacity() < SIZE_MAX && add(bytes, value.capacity() + 1));
}
// Source-declared GCC13 C++20 allocator/traits/new_allocator allocate and
// deallocate parameters, returns; max_size references/results and real locals.
constexpr size_t allocator_frames = 3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) +
				    sizeof(size_t) + sizeof(void *) + sizeof(size_t) +
				    4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
				    sizeof(size_t) + 4 * (sizeof(void *) + sizeof(size_t)) +
				    2 * sizeof(size_t);
// assign(s,n), _M_replace, _M_mutate, _M_create; old/new size, p/how_much,
// capacity/r, length checks; _S_copy/traits::copy, memcpy and char assign;
// dispose/destroy, data/capacity/set-length and final NUL.
// Fitting _M_replace calls _M_disjunct(this,s). Both actual less pointer
// temporaries can coexist through the full || expression; their operator()
// has this/x/y/result and is_constant_evaluated result. Data/size queries.
constexpr size_t disjunct_frames = 2 * sizeof(void *) + sizeof(bool) +
				   2 * sizeof(std::less<const char *>) +
				   2 * (3 * sizeof(void *) + 2 * sizeof(bool)) +
				   2 * (2 * sizeof(void *)) + sizeof(void *) + sizeof(size_t);
constexpr size_t string_assign_frames =
	disjunct_frames + 3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
	5 * sizeof(size_t) + 6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	allocator_frames + 2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) +
	sizeof(size_t) + 2 * sizeof(void *) + sizeof(char) + 6 * sizeof(void *) +
	3 * sizeof(size_t) + sizeof(bool) + sizeof(char);
// Actual two default string constructors/allocator hiders/local-data/length;
// two move assignments: this/source/reference return, equal_allocs/data/capacity,
// allocator-on-move references/tags, data/capacity/local/clear queries, short
// char copy; final two destructors/dispose/destroy and allocator deallocation.
constexpr size_t string_lifetime_frames =
	2 * (6 * sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(std::allocator<char>)) +
	sizeof(void *) +
	2 * (3 * sizeof(void *) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	     6 * sizeof(void *) + 2 * sizeof(std::allocator<char>) + 2 * sizeof(bool) +
	     12 * (sizeof(void *) + sizeof(size_t)) + string_assign_frames) +
	2 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + allocator_frames);
// find(char,pos): this,char,pos,ret,size,data,n,p; traits::find input,n,char,
// pointer result and builtin memchr input/char/n/result. Definition iteration
// begin/end and dereference; fixed array size/begin/end/subscript references.
constexpr size_t query_frames = sizeof(void *) + sizeof(char) + 4 * sizeof(size_t) +
				2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
				sizeof(char) + 2 * sizeof(void *) + sizeof(int) + sizeof(size_t) +
				8 * (sizeof(void *) + sizeof(size_t)) + 4 * sizeof(void *);
// Full original parser source variables, including all mutually exclusive
// loop/extension/XP scopes (sum, not an inferred compiler stack bound).
constexpr size_t parser_frames =
	// Parameters, closure objects for read32/read64, version, offset,
	// reward_record_bytes/reward_bytes/extension_header_bytes.
	5 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(uint32_t) +
	4 * sizeof(size_t) +
	// Root index/prior, reward index + copied goal, credited index/prior,
	// pid/name/definition lengths and includes_player.
	5 * sizeof(size_t) + sizeof(quest_reward_goal) + 3 * sizeof(uint32_t) + sizeof(bool) +
	// XP index, award reference, recipient/prior; reward/recipient/index,
	// recipient_found/award_found; definition iteration begin/end/character.
	7 * sizeof(size_t) + sizeof(void *) + 2 * sizeof(bool) + 2 * sizeof(void *) +
	sizeof(unsigned char) +
	// read32 and read64 call this/offset/local value/byte/returned scalar.
	2 * (sizeof(void *) + 2 * sizeof(size_t)) + 2 * sizeof(uint32_t) + 2 * sizeof(uint64_t);
// v6 tail's copy_n and copy/copy_move_a/a1/a2/copy_m, actual pointer iterators,
// counts/returned iterators, niter/miter/wrap and memmove carriers. All three
// fixed arrays use the same nonallocating chain, one prospectively charged chain.
constexpr size_t fixed_copy_frames = 3 * sizeof(void *) + sizeof(size_t) + sizeof(void *) +
				     3 * sizeof(void *) + sizeof(size_t) +
				     5 * (4 * sizeof(void *)) + 5 * (2 * sizeof(void *)) +
				     sizeof(ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t) +
				     2 * sizeof(bool) + 2 * sizeof(std::random_access_iterator_tag);
// array::operator==, begin/end, equal/aux/aux1/equal<true>, __niter_base
// argument/results, actual __simple/__len and __memcmp/builtin memcmp.
constexpr size_t fixed_equal_frames = 2 * sizeof(void *) + sizeof(bool) + 3 * (2 * sizeof(void *)) +
				      4 * (3 * sizeof(void *) + sizeof(bool)) +
				      3 * (2 * sizeof(void *)) + sizeof(size_t) +
				      2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int));
// Dynamic span constructor first/count, extent constructor extent,
// to_address(ptr) input/returned pointer; subspan this/offset/count/returned
// span, size/extent/data this/return carriers.
constexpr size_t span_frames = 2 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
			       sizeof(void *) + 2 * sizeof(size_t) +
			       sizeof(std::span<const uint8_t>) +
			       3 * (sizeof(void *) + sizeof(size_t));
// Additional source-event reader integer/take/subspan/block/id scopes beyond
// the genuine provider's object bytes, plus original typed48 result decoder,
// returned aggregate and get_u16/get_u64 scalar loop/return scopes; zero-id
// zero-id's direct fixed range loop and fixed-array comparison queries.
constexpr size_t fee_scalar_frames =
	sizeof(size_t) + sizeof(item_transfer_result) + 2 * sizeof(std::span<const uint8_t>) +
	8 * sizeof(void *) + 6 * sizeof(size_t) + 2 * sizeof(uint64_t) + sizeof(bool) +
	sizeof(item_transfer_result) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(void *) +
	2 * sizeof(uint64_t) + sizeof(unsigned int) + sizeof(void *) + sizeof(uint16_t) +
	3 * sizeof(void *) + sizeof(bool) + sizeof(uint8_t) + fixed_copy_frames +
	fixed_equal_frames + span_frames;
struct budget
{
	const quest_reward_continuation &value;
	reserve_fn reserve;
	void *context;
	size_t outer;
	size_t frames;
	bool peak(size_t extra) const noexcept
	{
		size_t bytes = outer;
		// This method, add/string_heap query/return carriers, observer locals,
		// and the actual reserve call's argument/result carriers.
		constexpr size_t observation =
			9 * sizeof(void *) + 5 * sizeof(size_t) + 4 * sizeof(bool);
		return add(bytes, sizeof(*this)) && add(bytes, sizeof(value)) &&
		       add(bytes, frames) && string_heap(value.character_name, bytes) &&
		       string_heap(value.definition_id, bytes) && add(bytes, observation) &&
		       add(bytes, extra) && reserve && reserve(bytes, context);
	}
	bool assign(std::string &destination, const char *data, size_t length) const
	{
		constexpr size_t own = 3 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
		size_t request = string_assign_frames + own;
		if (length > destination.capacity())
		{
			size_t next = length;
			const size_t capacity = destination.capacity();
			// Exact supported _M_create growth, preserving old heap until
			// the successful allocation/copy completes.
			if (capacity > SIZE_MAX / 2)
				return false;
			if (next < 2 * capacity)
				next = 2 * capacity;
			if (next > destination.max_size())
				next = destination.max_size();
			if (next == SIZE_MAX || !add(request, next + 1))
				return false;
		}
		if (!peak(request))
			return false;
		destination.assign(data, length);
		return true;
	}
};
inline bool initial(reserve_fn reserve, void *context, size_t outer, size_t frames) noexcept
{
	constexpr size_t own = 3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
	return add(outer, sizeof(quest_reward_continuation)) && add(outer, sizeof(budget)) &&
	       add(outer, frames) && add(outer, own) && reserve && reserve(outer, context);
}
} // namespace duris_quest_continuation_bounded_detail

inline bool quest_fee_reward_continuation_decode_bounded(const uint8_t *, size_t,
							 quest_reward_continuation *,
							 bool (*)(size_t, void *) noexcept, void *,
							 size_t) noexcept;

inline bool quest_reward_continuation_decode_bounded(const uint8_t *data, size_t size,
						     quest_reward_continuation *decoded,
						     bool (*reserve)(size_t, void *) noexcept,
						     void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)data;
	(void)size;
	(void)decoded;
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#else
	try
	{
		if (data && size >= 4 && data[0] == 6 && !data[1] && !data[2] && !data[3])
		{
			// Only the actual dispatch parameters survive this nested call;
			// the ordinary candidate and its closures have not been constructed.
			size_t nested = outer;
			constexpr size_t dispatch =
				5 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
			if (!duris_quest_continuation_bounded_detail::add(nested, dispatch) ||
			    !reserve || !reserve(nested, context))
				return false;
			return quest_fee_reward_continuation_decode_bounded(
				data, size, decoded, reserve, context, nested);
		}
		if (!data || !decoded || size < 40 || size > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
			return false;

		using namespace duris_quest_continuation_bounded_detail;
		size_t frames = parser_frames + query_frames + string_lifetime_frames;
		if (!initial(reserve, context, outer, frames))
			return false;
		auto read32 = [&](size_t offset)
		{
			uint32_t value = 0;
			for (size_t byte = 0; byte < 4; ++byte)
				value |= static_cast<uint32_t>(data[offset + byte]) << (byte * 8);
			return value;
		};
		auto read64 = [&](size_t offset)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < 8; ++byte)
				value |= static_cast<uint64_t>(data[offset + byte]) << (byte * 8);
			return value;
		};
		const uint32_t version = read32(0);
		if (version != 1 && version != 2 && version != 3 && version != 4 && version != 5)
			return false;
		quest_reward_continuation value;
		const budget owner{ value, reserve, context, outer, frames };
		value.version = version;
		value.player_pid = read32(4);
		value.quester_id = read32(8);
		value.completion_index = read32(12);
		value.mobile_vnum = read32(16);
		value.room_vnum = read32(20);
		value.completed_at = read64(24);
		value.root_count = read32(32);
		if (!value.player_pid || value.quester_id > INT32_MAX ||
		    value.completion_index > INT32_MAX || !value.mobile_vnum ||
		    value.mobile_vnum > INT32_MAX || !value.room_vnum ||
		    value.room_vnum > INT32_MAX || !value.completed_at ||
		    value.completed_at > INT64_MAX || !value.root_count ||
		    value.root_count > value.roots.size() ||
		    size < 40 + static_cast<size_t>(value.root_count) * sizeof(uint64_t))
			return false;
		size_t offset = 36;
		for (size_t index = 0; index < value.root_count; ++index, offset += 8)
		{
			value.roots[index] = read64(offset);
			if (!value.roots[index])
				return false;
			for (size_t prior = 0; prior < index; ++prior)
				if (value.roots[prior] == value.roots[index])
					return false;
		}
		value.reward_count = read32(offset);
		offset += 4;
		if (value.reward_count > value.rewards.size())
			return false;
		const size_t reward_record_bytes = version >= 4 ? 16 : (version >= 3 ? 12 : 8);
		const size_t reward_bytes =
			static_cast<size_t>(value.reward_count) * reward_record_bytes;
		if (size - offset < reward_bytes || (version == 1 && size - offset != reward_bytes))
			return false;
		for (size_t index = 0; index < value.reward_count;
		     ++index, offset += reward_record_bytes)
		{
			value.rewards[index] = { read32(offset), read32(offset + 4),
						 version >= 3 ? read32(offset + 8) : 0,
						 version >= 4 ? read32(offset + 12) : 0 };
			const auto goal = value.rewards[index];
			if ((goal.type != 1 && goal.type != 3 && goal.type != 4 &&
			     goal.type != 5) ||
			    goal.number > INT32_MAX || goal.frozen_amount > INT32_MAX ||
			    (goal.type != 4 && !goal.number) ||
			    (goal.flags & ~QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION) ||
			    (goal.type != 4 && goal.flags) || (version < 3 && goal.flags) ||
			    (version >= 4 ? (goal.type == 5 ? (!goal.frozen_amount ||
							       goal.frozen_amount > goal.number) :
							      goal.frozen_amount != 0) :
					    goal.frozen_amount != 0))
				return false;
		}
		if (version >= 2)
		{
			constexpr size_t extension_header_bytes = 6 * sizeof(uint32_t);
			if (size - offset < extension_header_bytes)
				return false;
			value.zone_number = read32(offset);
			value.player_level = static_cast<int32_t>(read32(offset + 4));
			value.player_racewar = static_cast<int32_t>(read32(offset + 8));
			value.party_size = read32(offset + 12);
			value.strongest_party_level = static_cast<int32_t>(read32(offset + 16));
			value.credited_count = read32(offset + 20);
			offset += extension_header_bytes;
			if (!value.zone_number || value.zone_number > INT32_MAX ||
			    value.player_level < 0 || value.strongest_party_level < 0 ||
			    !value.credited_count ||
			    value.credited_count > QUEST_REWARD_MAX_CREDITED_PIDS ||
			    value.party_size != value.credited_count ||
			    size - offset <
				    static_cast<size_t>(value.credited_count) * sizeof(uint32_t) +
					    2 * sizeof(uint32_t))
				return false;
			bool includes_player = false;
			for (size_t index = 0; index < value.credited_count;
			     ++index, offset += sizeof(uint32_t))
			{
				const uint32_t pid = read32(offset);
				if (!pid)
					return false;
				for (size_t prior = 0; prior < index; ++prior)
					if (value.credited_pids[prior] == pid)
						return false;
				value.credited_pids[index] = pid;
				includes_player = includes_player || pid == value.player_pid;
			}
			if (!includes_player)
				return false;
			const uint32_t name_length = read32(offset);
			offset += sizeof(uint32_t);
			if (!name_length || name_length > QUEST_REWARD_MAX_CHARACTER_NAME_BYTES ||
			    size - offset < static_cast<size_t>(name_length) + sizeof(uint32_t))
				return false;
			if (!owner.assign(value.character_name,
					  reinterpret_cast<const char *>(data + offset),
					  name_length))
				return false;
			if (value.character_name.find('\0') != std::string::npos)
				return false;
			offset += name_length;
			const uint32_t definition_length = read32(offset);
			offset += sizeof(uint32_t);
			if (!definition_length ||
			    definition_length > QUEST_REWARD_MAX_DEFINITION_ID_BYTES ||
			    size < offset + static_cast<size_t>(definition_length))
				return false;
			if (!owner.assign(value.definition_id,
					  reinterpret_cast<const char *>(data + offset),
					  definition_length))
				return false;
			for (unsigned char character : value.definition_id)
				if (character < 0x21 || character > 0x7e)
					return false;
			offset += definition_length;
			if (version >= 5)
			{
				if (value.credited_pids[0] != value.player_pid)
					return false;
				if (size - offset < sizeof(uint32_t))
					return false;
				value.xp_award_count = read32(offset);
				offset += sizeof(uint32_t);
				if (value.xp_award_count > value.xp_awards.size() ||
				    size != offset + static_cast<size_t>(value.xp_award_count) * 3 *
							     sizeof(uint32_t))
					return false;
				for (size_t index = 0; index < value.xp_award_count;
				     ++index, offset += 3 * sizeof(uint32_t))
				{
					auto &award = value.xp_awards[index];
					award = { read32(offset), read32(offset + 4),
						  read32(offset + 8) };
					if (!award.recipient_pid ||
					    award.reward_index >= value.reward_count ||
					    !award.amount ||
					    award.amount >
						    value.rewards[award.reward_index].number ||
					    value.rewards[award.reward_index].type != 5U)
						return false;
					if (award.recipient_pid == value.player_pid &&
					    award.amount !=
						    value.rewards[award.reward_index].frozen_amount)
						return false;
					bool recipient_found = false;
					for (size_t recipient = 0; recipient < value.credited_count;
					     ++recipient)
						recipient_found = recipient_found ||
								  value.credited_pids[recipient] ==
									  award.recipient_pid;
					if (!recipient_found)
						return false;
					for (size_t prior = 0; prior < index; ++prior)
						if (value.xp_awards[prior].recipient_pid ==
							    award.recipient_pid &&
						    value.xp_awards[prior].reward_index ==
							    award.reward_index)
							return false;
				}
				for (size_t reward = 0; reward < value.reward_count; ++reward)
					if (value.rewards[reward].type == 5U)
						for (size_t recipient = 0;
						     recipient < value.credited_count; ++recipient)
						{
							bool award_found = false;
							for (size_t index = 0;
							     index < value.xp_award_count; ++index)
								award_found =
									award_found ||
									(value.xp_awards[index]
											 .recipient_pid ==
										 value.credited_pids
											 [recipient] &&
									 value.xp_awards[index]
											 .reward_index ==
										 reward);
							if (!award_found)
								return false;
						}
			}
			if (version < 5 && size != offset)
				return false;
		}
		else if (size != offset)
			return false;
		// This preflight is the last fallible action. The real default-allocator
		// move has no allocation; prior destination heaps may move into value and
		// remain covered by caller outer through its destruction.
		if (!owner.peak(0))
			return false;
		*decoded = std::move(value);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
#endif
}

inline bool quest_fee_reward_continuation_decode_bounded(const uint8_t *data, size_t size,
							 quest_reward_continuation *decoded,
							 bool (*reserve)(size_t, void *) noexcept,
							 void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)data;
	(void)size;
	(void)decoded;
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#else
	try
	{
		if (!data || !decoded || size < 40 || size > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
			return false;

		using namespace duris_quest_continuation_bounded_detail;
		size_t frames = parser_frames + query_frames + string_lifetime_frames;
		if (!add(frames, fee_scalar_frames) ||
		    !add(frames, economic_source_event_decode_object_bytes()))
			return false;
		if (!initial(reserve, context, outer, frames))
			return false;
		auto read32 = [&](size_t offset)
		{
			uint32_t value = 0;
			for (size_t byte = 0; byte < 4; ++byte)
				value |= static_cast<uint32_t>(data[offset + byte]) << (byte * 8);
			return value;
		};
		auto read64 = [&](size_t offset)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < 8; ++byte)
				value |= static_cast<uint64_t>(data[offset + byte]) << (byte * 8);
			return value;
		};
		const uint32_t version = read32(0);
		if (version != 6)
			return false;
		quest_reward_continuation value;
		const budget owner{ value, reserve, context, outer, frames };
		value.version = version;
		value.player_pid = read32(4);
		value.quester_id = read32(8);
		value.completion_index = read32(12);
		value.mobile_vnum = read32(16);
		value.room_vnum = read32(20);
		value.completed_at = read64(24);
		value.root_count = read32(32);
		if (!value.player_pid || value.quester_id > INT32_MAX ||
		    value.completion_index > INT32_MAX || !value.mobile_vnum ||
		    value.mobile_vnum > INT32_MAX || !value.room_vnum ||
		    value.room_vnum > INT32_MAX || !value.completed_at ||
		    value.completed_at > INT64_MAX || value.root_count ||
		    value.root_count > value.roots.size() ||
		    size < 40 + static_cast<size_t>(value.root_count) * sizeof(uint64_t))
			return false;
		size_t offset = 36;
		for (size_t index = 0; index < value.root_count; ++index, offset += 8)
		{
			value.roots[index] = read64(offset);
			if (!value.roots[index])
				return false;
			for (size_t prior = 0; prior < index; ++prior)
				if (value.roots[prior] == value.roots[index])
					return false;
		}
		value.reward_count = read32(offset);
		offset += 4;
		if (value.reward_count > value.rewards.size())
			return false;
		const size_t reward_record_bytes = version >= 4 ? 16 : (version >= 3 ? 12 : 8);
		const size_t reward_bytes =
			static_cast<size_t>(value.reward_count) * reward_record_bytes;
		if (size - offset < reward_bytes || (version == 1 && size - offset != reward_bytes))
			return false;
		for (size_t index = 0; index < value.reward_count;
		     ++index, offset += reward_record_bytes)
		{
			value.rewards[index] = { read32(offset), read32(offset + 4),
						 version >= 3 ? read32(offset + 8) : 0,
						 version >= 4 ? read32(offset + 12) : 0 };
			const auto goal = value.rewards[index];
			if ((goal.type != 1 && goal.type != 3 && goal.type != 4 &&
			     goal.type != 5) ||
			    goal.number > INT32_MAX || goal.frozen_amount > INT32_MAX ||
			    (goal.type != 4 && !goal.number) ||
			    (goal.flags & ~QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION) ||
			    (goal.type != 4 && goal.flags) || (version < 3 && goal.flags) ||
			    (version >= 4 ? (goal.type == 5 ? (!goal.frozen_amount ||
							       goal.frozen_amount > goal.number) :
							      goal.frozen_amount != 0) :
					    goal.frozen_amount != 0))
				return false;
		}
		if (version >= 2)
		{
			constexpr size_t extension_header_bytes = 6 * sizeof(uint32_t);
			if (size - offset < extension_header_bytes)
				return false;
			value.zone_number = read32(offset);
			value.player_level = static_cast<int32_t>(read32(offset + 4));
			value.player_racewar = static_cast<int32_t>(read32(offset + 8));
			value.party_size = read32(offset + 12);
			value.strongest_party_level = static_cast<int32_t>(read32(offset + 16));
			value.credited_count = read32(offset + 20);
			offset += extension_header_bytes;
			if (!value.zone_number || value.zone_number > INT32_MAX ||
			    value.player_level < 0 || value.strongest_party_level < 0 ||
			    !value.credited_count ||
			    value.credited_count > QUEST_REWARD_MAX_CREDITED_PIDS ||
			    value.party_size != value.credited_count ||
			    size - offset <
				    static_cast<size_t>(value.credited_count) * sizeof(uint32_t) +
					    2 * sizeof(uint32_t))
				return false;
			bool includes_player = false;
			for (size_t index = 0; index < value.credited_count;
			     ++index, offset += sizeof(uint32_t))
			{
				const uint32_t pid = read32(offset);
				if (!pid)
					return false;
				for (size_t prior = 0; prior < index; ++prior)
					if (value.credited_pids[prior] == pid)
						return false;
				value.credited_pids[index] = pid;
				includes_player = includes_player || pid == value.player_pid;
			}
			if (!includes_player)
				return false;
			const uint32_t name_length = read32(offset);
			offset += sizeof(uint32_t);
			if (!name_length || name_length > QUEST_REWARD_MAX_CHARACTER_NAME_BYTES ||
			    size - offset < static_cast<size_t>(name_length) + sizeof(uint32_t))
				return false;
			if (!owner.assign(value.character_name,
					  reinterpret_cast<const char *>(data + offset),
					  name_length))
				return false;
			if (value.character_name.find('\0') != std::string::npos)
				return false;
			offset += name_length;
			const uint32_t definition_length = read32(offset);
			offset += sizeof(uint32_t);
			if (!definition_length ||
			    definition_length > QUEST_REWARD_MAX_DEFINITION_ID_BYTES ||
			    size < offset + static_cast<size_t>(definition_length))
				return false;
			if (!owner.assign(value.definition_id,
					  reinterpret_cast<const char *>(data + offset),
					  definition_length))
				return false;
			for (unsigned char character : value.definition_id)
				if (character < 0x21 || character > 0x7e)
					return false;
			offset += definition_length;
			if (version >= 5)
			{
				if (value.credited_pids[0] != value.player_pid)
					return false;
				if (size - offset < sizeof(uint32_t))
					return false;
				value.xp_award_count = read32(offset);
				offset += sizeof(uint32_t);
				if (value.xp_award_count > value.xp_awards.size() ||
				    size < offset + static_cast<size_t>(value.xp_award_count) * 3 *
							    sizeof(uint32_t))
					return false;
				for (size_t index = 0; index < value.xp_award_count;
				     ++index, offset += 3 * sizeof(uint32_t))
				{
					auto &award = value.xp_awards[index];
					award = { read32(offset), read32(offset + 4),
						  read32(offset + 8) };
					if (!award.recipient_pid ||
					    award.reward_index >= value.reward_count ||
					    !award.amount ||
					    award.amount >
						    value.rewards[award.reward_index].number ||
					    value.rewards[award.reward_index].type != 5U)
						return false;
					if (award.recipient_pid == value.player_pid &&
					    award.amount !=
						    value.rewards[award.reward_index].frozen_amount)
						return false;
					bool recipient_found = false;
					for (size_t recipient = 0; recipient < value.credited_count;
					     ++recipient)
						recipient_found = recipient_found ||
								  value.credited_pids[recipient] ==
									  award.recipient_pid;
					if (!recipient_found)
						return false;
					for (size_t prior = 0; prior < index; ++prior)
						if (value.xp_awards[prior].recipient_pid ==
							    award.recipient_pid &&
						    value.xp_awards[prior].reward_index ==
							    award.reward_index)
							return false;
				}
				for (size_t reward = 0; reward < value.reward_count; ++reward)
					if (value.rewards[reward].type == 5U)
						for (size_t recipient = 0;
						     recipient < value.credited_count; ++recipient)
						{
							bool award_found = false;
							for (size_t index = 0;
							     index < value.xp_award_count; ++index)
								award_found =
									award_found ||
									(value.xp_awards[index]
											 .recipient_pid ==
										 value.credited_pids
											 [recipient] &&
									 value.xp_awards[index]
											 .reward_index ==
										 reward);
							if (!award_found)
								return false;
						}
			}
			if (version < 5 && size != offset)
				return false;
		}
		else if (size != offset)
			return false;
		constexpr size_t tail_fixed =
			4 + 16 + ECONOMIC_SOURCE_EVENT_BYTES + 8 + 16 + ITEM_TRANSFER_RESULT_BYTES;
		if (size - offset < tail_fixed || data[offset] != 'Q' || data[offset + 1] != 'R' ||
		    data[offset + 2] != 'F' || data[offset + 3] != '6')
			return false;
		offset += 4;
		std::copy_n(data + offset, 16, value.action_operation.bytes.begin());
		offset += 16;
		if (economic_source_event_decode({ data + offset, ECONOMIC_SOURCE_EVENT_BYTES },
						 &value.action_source) !=
		    economic_accounting_error::ok)
			return false;
		offset += ECONOMIC_SOURCE_EVENT_BYTES;
		value.action_mobile_instance_id = read64(offset);
		offset += 8;
		std::copy_n(data + offset, 16, value.triggering_acceptance.bytes.begin());
		offset += 16;
		if (size - offset != ITEM_TRANSFER_RESULT_BYTES ||
		    critical_operation_id_is_zero(value.action_operation) ||
		    critical_operation_id_is_zero(value.triggering_acceptance) ||
		    value.action_operation.bytes == value.triggering_acceptance.bytes ||
		    value.action_source.kind != economic_source_kind::quest_action ||
		    value.action_source.source.bytes != value.action_operation.bytes ||
		    value.action_source.slot != value.completion_index ||
		    !value.action_source.sequence || !value.action_mobile_instance_id ||
		    value.action_mobile_instance_id == UINT64_MAX)
			return false;
		std::copy_n(data + offset, ITEM_TRANSFER_RESULT_BYTES,
			    value.original_acceptance_result.begin());
		item_transfer_result result{};
		if (!item_transfer_command_decode_result(value.original_acceptance_result.data(),
							 value.original_acceptance_result.size(),
							 &result) ||
		    !result.root_item_uid || !result.item_count || result.corpse_revision ||
		    result.collector_catalog_changed)
			return false;
		// This preflight is the last fallible action. The real default-allocator
		// move has no allocation; prior destination heaps may move into value and
		// remain covered by caller outer through its destruction.
		if (!owner.peak(0))
			return false;
		*decoded = std::move(value);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
#endif
}

// Additive profiles for the exact existing bounded decoder targets below.
// Input/output (including prior output string heaps) remain caller-owned.
// The old parsers self-retain their old frames, real candidate/budget objects,
// growing string requests and current capacities. Only the disjoint positive
// supplement is retained by their caller; full Source/INITIAL are transient
// preentry requirements. No encoder or trigger-binding predicate is selected.
namespace duris_quest_continuation_source_detail
{
using namespace duris_quest_continuation_bounded_detail;
constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
#if defined(__linux__) && defined(__GNUC__) && __GNUC__ == 13 && defined(__cplusplus) &&         \
	__cplusplus == 202002L && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&         \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                          \
	!defined(_GLIBCXX_PROFILE) && defined(__cpp_sized_deallocation) &&                       \
	__cpp_sized_deallocation >= 201309L
constexpr bool supported =
	P == 8 && N == 8 && sizeof(std::ptrdiff_t) == 8 && sizeof(bool) == 1 &&
	sizeof(std::allocator<char>) == 1 && sizeof(char) == 1 && sizeof(std::string) == 32 &&
	sizeof(reserve_fn) == P && sizeof(std::span<const uint8_t>) == 2 * N &&
	sizeof(std::span<const uint8_t>::iterator) == P && sizeof(std::string::iterator) == P &&
	sizeof(std::random_access_iterator_tag) == 1;
#else
constexpr bool supported = false;
#endif

// Default string: basic_string receiver; hider(this,pointer,allocator-ref);
// allocator/new_allocator defaults and const copies; std::move(ref/result),
// allocator-temp destructor; mutable local_data/pointer_to/addressof/
// __addressof; init-local receiver/public constant-evaluation result;
// set_length/length/data/traits::assign and its internal evaluation result.
// Runtime init-local has NO constexpr-only loop index or zero-char object.
// The exact old default subrow already owns six receiver/reference carriers,
// one length word, the NUL char and actual temporary allocator object. The
// deficit is confined to that same constructor subtree: no move/assign slack.
constexpr size_t string_default_complete =
	28 * P + 2 * N + sizeof(char) + 2 * B + sizeof(std::allocator<char>);
constexpr size_t string_default_credit = 6 * P + N + sizeof(char) + sizeof(std::allocator<char>);
constexpr size_t string_default_missing = string_default_complete - string_default_credit;

// Actual move allocator lane: two policy-result bools, alloc_on_move refs,
// std::move refs/result, explicitly declared/defaulted allocator and base
// assignments, and BOTH actual mutable allocator getters. The old six-P
// allocator lane credits refs/move/one getter; the second getter and declared
// assignments are the remaining eight P. Old allocator objects are preserved
// conservative fields and cannot be substituted for these declared scopes.
constexpr size_t string_move_allocator_missing = 8 * P;
// Const local_data has this/result plus pointer_to/addressof/__addressof.
// The old direct local-data query owns its this/result, leaving six P.
// Its _M_is_local/data/size/length/clear callers remain old matching queries.
constexpr size_t string_const_local_data_missing = 6 * P;
constexpr size_t string_self_address_missing = 2 * P;
// Actual hider and allocator destruction, at each of the two completed
// string lifetimes. No C++20 new_allocator destructor is selected.
constexpr size_t string_hider_cleanup_missing = 2 * (2 * P);
// Continuation move assignment is nontrivial (two strings): this/source/ref
// result. Its direct std::move has ref/result; final nontrivial destructor
// has this. Arrays and scalar aggregates introduce no invented callees.
constexpr size_t continuation_lifetime_missing = 3 * P + 2 * P + P;

// Exact find(char,pos) own row is 3P+5N+char, versus the old 3P+4N+char.
// Actual size and _M_data descendants are P+N and 2P. Traits::find has
// s/reference-a/result3P plus n and its internal evaluation bool; the old
// two-P traits row leaves P+B. Old builtin memchr fields are not new credit.
constexpr size_t string_find_missing = N + (P + N) + 2 * P + P + B;
// Four actual array member types: uint64 roots, goal rewards, uint32 credited,
// award XP. Old direct subscript rows leave their returned-reference P.
// Selected array::operator[] directly indexes _M_elems: no _S_ref call.
constexpr size_t array_access_missing = 4 * P;
// Definition's mutable string begin/end return the actual loop iterators.
// Their two explicit normal-iterator constructors, end's pointer arithmetic
// temporary, rewritten != selecting C++20 operator==/two base calls, ++
// and * are genuine declared scopes.
// _M_data and size are the identical sequential leaves named by find above.
// No implicit trivial iterator copy/dtor or builtin comparison body is priced.
// Old parser_frames first5P owns data/decoded/reserve/context4P and one
// phase-exclusive P: hidden range-for __range reference while parsing,
// or bad_alloc catch reference after that range has unwound. max(P,P)=P.
constexpr size_t definition_iteration_missing = 4 * P + P + (2 * P + B) + 4 * P + 2 * P + 2 * P;

// Old _M_replace owns five size words; actual old_size,new_size,pos,len1,
// len2,how_much are six. Ordinary new/delete declared interfaces are separate
// from allocator wrappers: count/result, then sized-delete pointer/count.
// Their out-of-line bodies and exception implementation remain external.
constexpr size_t string_assignment_missing = N + 2 * (P + N);
// Real automatic constexpr fields are Source objects, not cached estimates:
// parser frames N, parser returned bool B; assign's own N; observation's
// otherwise missing capacity-return N; initial's add extra/result N+B.
constexpr size_t controller_missing = (N + B) + N + N + (N + B);

constexpr size_t common_missing = 2 * string_default_missing + 2 * string_move_allocator_missing +
				  string_const_local_data_missing + string_self_address_missing +
				  string_hider_cleanup_missing + continuation_lifetime_missing +
				  string_find_missing + array_access_missing +
				  definition_iteration_missing + string_assignment_missing +
				  controller_missing;

// Fee-only residuals. Reader take/integer/block/id and source-event-valid
// own refs/scalars already have the dedicated old eight-P/six-N/two-u64/B
// credit, and the named reader/event/id/span objects are owned by the actual
// economic_source_event_decode_object_bytes provider. Its runtime returned
// size N and the source decoder's returned error enum are separate Source.
constexpr size_t source_event_scalar_missing = N + sizeof(economic_accounting_error);
// typed48/legacy40 result reader has no heap or array-library subtree: old
// fee scalar credit owns its params, true aggregate temporary, get_u64
// input/value/loop/result and get_u16 input/result. Its bool return is extra.
constexpr size_t result_decode_missing = B;
// Span pointer/count constructor: old direct/to_address credit leaves real
// extent receiver P plus raw __to_address(input,result)2P. Default failure
// span invokes declared default span/extent receivers2P; reader copies its
// named input span via declared default-copy(this,source)2P. operator[] is
// this/index/returned-ref2P+N. begin/end producers add receiver+pointer ctor
// (3P/4P respectively, end includes its pointer prvalue). Returned iterator
// values are the old copy parameters, not second object copies. Normal
// __niter_base's base() receiver/ref2P is missing from the old raw-pointer row.
constexpr size_t source_span_missing = 3 * P + 2 * P + 2 * P + (2 * P + N) + 3 * P + 4 * P + 2 * P;
// Selected byte-array data() directly casts _M_elems: no _S_ptr call.
// Its actual receiver/returned-pointer2P for extents16/48 is the same
// sequential family already retained by old fixed_equal_frames data rows;
// each typed body has no local/child/extent branch. Physical arrays remain
// distinct old candidate members. No new data-helper charge is required.
// copy_n's real __n2 and __size_to_integer argument/result plus the
// iterator_category reference leave3N+P beyond old pointer-copy fields.
// Existing tag objects stay old owned; no undeclared tag special members.
constexpr size_t fixed_array_copy_missing = 3 * N + P;
// Actual equal_aux1 __simple bool is additional to its declared bool return.
// Public constant-evaluation result is already the exact sequential default
// init-local lane above; internal predicate is the traits::assign/find lane.
constexpr size_t fixed_equal_missing = B;
constexpr size_t fee_missing = common_missing + source_event_scalar_missing +
			       result_decode_missing + source_span_missing +
			       fixed_array_copy_missing + fixed_equal_missing;
// Generic dispatch's live automatic constexpr dispatch N is additional to
// nested,size,outer. The original dispatch5P+3N+B remains child outer-owned.
constexpr size_t dispatch_missing = N;
constexpr size_t generic_missing = fee_missing + dispatch_missing;

// Full preentry Source includes the EXACT unchanged original Source fields
// and the largest genuine initial/assignment/observation request. Original
// conservative builtin/tag fields are preserved, not treated as newly reached
// source or padding available to any different subtree. Candidate/budget
// physical storage is INITIAL below, and every char heap stays old owned.
constexpr size_t assign_own = 3 * P + 4 * N + B;
constexpr size_t observe_own = 9 * P + 5 * N + 4 * B;
constexpr size_t parser_source = parser_frames + query_frames + string_lifetime_frames +
				 string_assign_frames + assign_own + observe_own;
constexpr size_t fee_source_without_objects = parser_source + fee_scalar_frames + fee_missing;
constexpr size_t generic_source_without_objects =
	5 * P + 3 * N + B + dispatch_missing + fee_source_without_objects;
constexpr size_t initial_objects = sizeof(quest_reward_continuation) + sizeof(budget);
} // namespace duris_quest_continuation_source_detail

inline bool quest_fee_reward_continuation_decode_source_frame_bytes(size_t *bytes) noexcept
{
	using namespace duris_quest_continuation_source_detail;
	if (!bytes || !supported)
		return false;
	size_t result = fee_source_without_objects;
	const size_t objects = economic_source_event_decode_object_bytes();
	if (!add(result, objects))
		return false;
	*bytes = result;
	return true;
}
inline bool quest_reward_continuation_decode_source_frame_bytes(size_t *bytes) noexcept
{
	using namespace duris_quest_continuation_source_detail;
	if (!bytes || !supported)
		return false;
	size_t result = generic_source_without_objects;
	const size_t objects = economic_source_event_decode_object_bytes();
	if (!add(result, objects))
		return false;
	*bytes = result;
	return true;
}
inline bool
quest_fee_reward_continuation_decode_source_supplement_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !duris_quest_continuation_source_detail::supported)
		return false;
	*bytes = duris_quest_continuation_source_detail::fee_missing;
	return true;
}
inline bool quest_reward_continuation_decode_source_supplement_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !duris_quest_continuation_source_detail::supported)
		return false;
	*bytes = duris_quest_continuation_source_detail::generic_missing;
	return true;
}
inline bool quest_fee_reward_continuation_decode_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !duris_quest_continuation_source_detail::supported)
		return false;
	*bytes = duris_quest_continuation_source_detail::initial_objects;
	return true;
}
inline bool quest_reward_continuation_decode_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !duris_quest_continuation_source_detail::supported)
		return false;
	*bytes = duris_quest_continuation_source_detail::initial_objects;
	return true;
}
constexpr size_t quest_fee_reward_continuation_decode_source_query_frame_bytes() noexcept
{
	// Largest of the three actual pure getters: output P, result/objects2N,
	// bool return B; object-provider scalar return N; add(P,N,B). Result-size
	// objects retained by the selecting caller are separately caller-owned.
	return 2 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(bool);
}
constexpr size_t quest_reward_continuation_decode_source_query_frame_bytes() noexcept
{
	return 2 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(bool);
}

#endif
