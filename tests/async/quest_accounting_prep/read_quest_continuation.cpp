#include "item/quest_reward_continuation.h"

#include <iostream>
#include <vector>

// Read-only adapter to the maintained value decoder. No substitute wire grammar.
int main()
{
	std::vector<uint8_t> bytes;
	char byte;
	while (std::cin.get(byte))
	{
		if (bytes.size() >= ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
			return 2;
		bytes.push_back(static_cast<uint8_t>(byte));
	}
	quest_reward_continuation terms;
	if (!quest_reward_continuation_decode(bytes.data(), bytes.size(), &terms))
		return 3;
	std::cout << "{\"version\":" << terms.version << ",\"pid\":" << terms.player_pid
		  << ",\"mobile_vnum\":" << terms.mobile_vnum << ",\"level\":" << terms.player_level
		  << ",\"party_size\":" << terms.party_size << ",\"xp_awards\":[";
	for (uint32_t index = 0; index < terms.xp_award_count; ++index)
	{
		const auto &award = terms.xp_awards[index];
		if (index)
			std::cout << ',';
		std::cout << "{\"pid\":" << award.recipient_pid
			  << ",\"index\":" << award.reward_index << ",\"amount\":" << award.amount
			  << '}';
	}
	std::cout << "]}\n";
}
