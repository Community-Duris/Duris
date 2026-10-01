#ifndef QUEST_REWARD_CONTINUATION_H
#define QUEST_REWARD_CONTINUATION_H

#include "item/item_transfer_command.h"

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

inline uint64_t quest_item_reward_source_id(const quest_reward_continuation &terms, size_t index)
{
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
// Version 6 freezes season, catalog revision, and daily-qualified recipients.
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

#endif
