#include "world/generated_npc_state.h"

#include <cstring>
#include <new>

bool generated_npc_state_encode(const pet_restore_state &state,
				const std::array<int32_t, 4> &wallet, std::string *encoded)
{
	if (!encoded)
		return false;
	try
	{
		std::string identity;
		if (!pet_restore_state_encode(state, &identity))
			return false;
		std::string value;
		for (int32_t coins : wallet)
		{
			if (coins < 0)
				return false;
			for (size_t i = 0; i < 4; ++i)
				value.push_back(
					static_cast<char>(static_cast<uint32_t>(coins) >> (i * 8)));
		}
		value += identity;
		*encoded = std::move(value);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool generated_npc_state_decode(const std::string &encoded, pet_restore_state *state,
				std::array<int32_t, 4> *wallet)
{
	if (!state || !wallet || encoded.size() < GENERATED_NPC_STATE_WALLET_BYTES ||
	    encoded.size() > GENERATED_NPC_STATE_WALLET_BYTES + PET_RESTORE_STATE_MAX_BYTES)
		return false;
	for (size_t coin = 0; coin < wallet->size(); ++coin)
	{
		uint32_t value = 0;
		for (size_t i = 0; i < 4; ++i)
			value |= static_cast<uint32_t>(
					 static_cast<unsigned char>(encoded[coin * 4 + i]))
				 << (i * 8);
		if (value > INT32_MAX)
			return false;
		(*wallet)[coin] = static_cast<int32_t>(value);
	}
	try
	{
		return pet_restore_state_decode(encoded.substr(GENERATED_NPC_STATE_WALLET_BYTES),
						state);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool generated_npc_vnum(int vnum)
{
	return vnum == 1255 || vnum == 1256;
}

bool generated_npc_state_valid(int vnum, const std::string &encoded)
{
	if (encoded.empty())
		return true; // Legacy records have no recoverable generated identity.
	pet_restore_state state;
	std::array<int32_t, 4> wallet;
	return generated_npc_vnum(vnum) && generated_npc_state_decode(encoded, &state, &wallet) &&
	       state.kind == summoned_pet_kind::ordinary && !state.charm_expires_at &&
	       !state.death_expires_at && state.level <= 61 &&
	       state.name.find("random mob prototype") == std::string::npos &&
	       state.short_description.find("random mob prototype") == std::string::npos &&
	       state.long_description.find("random mob prototype") == std::string::npos;
}

bool generated_npc_extension_encode(int vnum, const std::string &encoded, std::string *extension)
{
	if (!extension || !generated_npc_state_valid(vnum, encoded))
		return false;
	try
	{
		extension->assign("GNP1", 4);
		const uint32_t size = static_cast<uint32_t>(encoded.size());
		for (size_t i = 0; i < 4; ++i)
			extension->push_back(static_cast<char>(size >> (i * 8)));
		extension->append(encoded);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool generated_npc_extension_decode(int vnum, const char *data, size_t size, std::string *encoded)
{
	if (!encoded || !data || size < GENERATED_NPC_EXTENSION_HEADER_BYTES ||
	    size > GENERATED_NPC_EXTENSION_MAX_BYTES || memcmp(data, "GNP1", 4))
		return false;
	uint32_t length = 0;
	for (size_t i = 0; i < 4; ++i)
		length |= static_cast<uint32_t>(static_cast<unsigned char>(data[4 + i])) << (i * 8);
	if (length != size - GENERATED_NPC_EXTENSION_HEADER_BYTES)
		return false;
	try
	{
		std::string value(data + GENERATED_NPC_EXTENSION_HEADER_BYTES, length);
		if (!generated_npc_state_valid(vnum, value))
			return false;
		*encoded = std::move(value);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
