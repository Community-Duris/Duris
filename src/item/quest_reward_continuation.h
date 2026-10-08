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
	uint32_t season_id = 0;
	uint32_t catalog_revision = 0;
	uint32_t daily_policy_revision = 0;
	uint32_t daily_count = 0;
	std::array<uint32_t, 64> daily_pids = {};
	std::string character_name;
	std::string definition_id;
	// Explicit fee-only v6 authority, never a synthetic consumed item root.
	critical_operation_id action_operation{}, triggering_acceptance{};
	economic_source_event action_source{};
	uint64_t action_mobile_instance_id = 0;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> original_acceptance_result{};
};

// v6 item records retain daily terms; v6 fee records retain native action terms.
// The original consumed-root field distinguishes their immutable layouts.
inline bool quest_reward_is_fee_only(const quest_reward_continuation &terms)
{
	return terms.version == 6 && terms.root_count == 0;
}

inline bool quest_reward_has_daily_context(const quest_reward_continuation &terms)
{
	return terms.version == 6 && terms.root_count != 0;
}

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

// Fee-only v6 derives logical reward source from the actual admitted action and its
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
	if (quest_reward_is_fee_only(terms))
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
// Rooted version 6 freezes season, catalog revision, and daily-qualified recipients.
// Rootless version 6 retains accounting's fee-only QRF6 action binding. The root
// count selects the established wire shape; neither shape can decode the other.
inline bool quest_fee_reward_continuation_decode(const uint8_t *, size_t,
						 quest_reward_continuation *);
inline bool quest_reward_continuation_decode(const uint8_t *data, size_t size,
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
	// The established v6 layouts are disjoint: fee-only has no consumed roots.
	if (version == 6 && read32(32) == 0)
		return quest_fee_reward_continuation_decode(data, size, decoded);
	if (version != 1 && version != 2 && version != 3 && version != 4 && version != 5 &&
	    version != 6)
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
		if (value.zone_number > INT32_MAX || value.player_level < 0 ||
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
			    (size < offset + static_cast<size_t>(value.xp_award_count) * 3 *
						     sizeof(uint32_t) ||
			     (version == 5 &&
			      size != offset + static_cast<size_t>(value.xp_award_count) * 3 *
						       sizeof(uint32_t))))
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
		if (version == 6)
		{
			if (size - offset < 16)
				return false;
			value.season_id = read32(offset);
			value.catalog_revision = read32(offset + 4);
			value.daily_policy_revision = read32(offset + 8);
			value.daily_count = read32(offset + 12);
			offset += 16;
			if (!value.season_id || !value.catalog_revision ||
			    value.daily_policy_revision != 1 ||
			    value.daily_count > value.credited_count ||
			    size - offset != value.daily_count * 4)
				return false;
			for (size_t i = 0; i < value.daily_count; ++i, offset += 4)
			{
				value.daily_pids[i] = read32(offset);
				bool member = false;
				for (size_t j = 0; j < value.credited_count; ++j)
					member |= value.daily_pids[i] == value.credited_pids[j];
				if (!member)
					return false;
				for (size_t j = 0; j < i; ++j)
					if (value.daily_pids[i] == value.daily_pids[j])
						return false;
			}
		}
		if (version >= 5 && size != offset)
			return false;
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

#endif
