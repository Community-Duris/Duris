#include "world/native_mobile_birth_reset_tail.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "economy/shop.h"
#include "world/difficulty.h"
#include "world/events.h"

#include <bit>
#include <memory>
#include <openssl/evp.h>

extern P_index mob_index;
extern P_room world;
extern zone_data *zone_table;
extern int top_of_mobt;
extern int top_of_world;
extern int top_of_zone_table;

namespace
{
class reset_tail_hash
{
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context{ EVP_MD_CTX_new(),
									 EVP_MD_CTX_free };
	bool healthy = context && EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) == 1;

    public:
	bool bytes(const void *data, size_t length) noexcept
	{
		healthy = healthy && (!length || data) &&
			  EVP_DigestUpdate(context.get(), data, length) == 1;
		return healthy;
	}
	bool integer(uint64_t value) noexcept
	{
		std::array<uint8_t, 8> wire{};
		for (size_t i = 0; i < wire.size(); ++i)
			wire[i] = static_cast<uint8_t>(value >> (i * 8));
		return bytes(wire.data(), wire.size());
	}
	bool finish(native_mobile_birth_reset_tail_digest *output) noexcept
	{
		native_mobile_birth_reset_tail_digest candidate{};
		unsigned int size = 0;
		if (!healthy || EVP_DigestFinal_ex(context.get(), candidate.data(), &size) != 1 ||
		    size != candidate.size())
			return false;
		*output = candidate;
		return true;
	}
};
} // namespace

bool native_mobile_birth_reset_tail_capture(int32_t mobile_vnum, int32_t destination_room_vnum,
					    int configured_shop,
					    native_mobile_birth_reset_tail_digest *output) noexcept
{
	if (!output || mobile_vnum <= 0 || destination_room_vnum <= 0 || configured_shop < -1 ||
	    !nevent_is_game_thread() || !mob_index || !world || !zone_table)
		return false;
	const int mobile = real_mobile(mobile_vnum);
	const int room = real_room(destination_room_vnum);
	if (mobile < 0 || mobile > top_of_mobt || room < 0 || room > top_of_world ||
	    mob_index[mobile].virtual_number != mobile_vnum ||
	    world[room].number != destination_room_vnum)
		return false;
	const int zone = world[room].zone;
	if (zone < 0 || zone > top_of_zone_table)
		return false;
	const double hitpoint_dial = difficulty_multiplier(DIFFICULTY_MOB_HITPOINTS);
	const int difficulty = BOUNDED(1, zone_table[zone].difficulty, 10);
	reset_tail_hash hash;
	constexpr char domain[] = "NMT1";
	if (!hash.bytes(domain, sizeof(domain) - 1) || !hash.integer(mobile_vnum) ||
	    !hash.integer(destination_room_vnum) ||
	    !hash.integer(static_cast<uint64_t>(static_cast<int64_t>(zone_table[zone].number))) ||
	    !hash.integer(std::bit_cast<uint64_t>(hitpoint_dial)) || !hash.integer(difficulty))
		return false;
	if (difficulty != 1)
	{
		// Same float-valued getters/defaults/order as original apply_zone_modifier.
		// A difficulty-one tail never accesses or binds these unused properties.
		const float hitpoints = get_property("hitpoints.zoneDifficulty.factor", 0.500);
		const float experience = get_property("exp.zoneDifficulty.factor", 0.500);
		const float damage_mod = get_property("damage.zoneDifficulty.mod.factor", 0.200);
		if (!hash.integer(std::bit_cast<uint32_t>(hitpoints)) ||
		    !hash.integer(std::bit_cast<uint32_t>(experience)) ||
		    !hash.integer(std::bit_cast<uint32_t>(damage_mod)))
			return false;
	}
	shop_native_mobile_birth_reset_selection selection;
	if (!shop_native_mobile_birth_reset_tail_selection(mobile_vnum, destination_room_vnum,
							   configured_shop, &selection) ||
	    !hash.integer(static_cast<uint64_t>(static_cast<int64_t>(selection.index))) ||
	    !hash.integer(selection.shop_slot) || !hash.integer(selection.keeper_vnum) ||
	    !hash.integer(selection.room_vnum) || !hash.integer(selection.replicated ? 1 : 0))
		return false;
	native_mobile_birth_reset_tail_digest candidate{};
	if (!hash.finish(&candidate))
		return false;
	*output = candidate;
	return true;
}
