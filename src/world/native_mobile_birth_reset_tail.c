#include <cerrno>
#include <openssl/sha.h>
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

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
namespace
{

// Exact authenticated OpenSSL3.0.13 source inventory shared with the original
// native reference/artifact companions: AVX2 block workspace/alignment and
// C small/normal leaves, with reached Init/Update/Final source carriers.
constexpr size_t native_reset_tail_sha_assembly =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t native_reset_tail_sha_c_small = 16 * sizeof(unsigned) + 12 * sizeof(unsigned) +
						 sizeof(unsigned) + sizeof(int) + sizeof(void *);
constexpr size_t native_reset_tail_sha_c_normal =
	16 * sizeof(unsigned) + 11 * sizeof(unsigned) + 2 * sizeof(int) + 2 * sizeof(void *);
constexpr size_t native_reset_tail_sha_update =
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(unsigned) + sizeof(int);
constexpr size_t native_reset_tail_sha_final = 3 * sizeof(void *) + sizeof(size_t) +
					       sizeof(unsigned long) + sizeof(unsigned) +
					       sizeof(int);
constexpr size_t native_reset_tail_sha_leaf_frames =
	std::max(native_reset_tail_sha_assembly,
		 std::max(native_reset_tail_sha_c_small, native_reset_tail_sha_c_normal)) +
	std::max(sizeof(void *) + sizeof(int),
		 std::max(native_reset_tail_sha_update, native_reset_tail_sha_final));

class reset_tail_bounded_hash
{
	SHA256_CTX context{};
	bool healthy = SHA256_Init(&context) == 1;

    public:
	bool bytes(const void *data, size_t length) noexcept
	{
		healthy = healthy && (!length || data) &&
			  SHA256_Update(&context, data, length) == 1;
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
		if (!healthy || SHA256_Final(candidate.data(), &context) != 1)
			return false;
		*output = candidate;
		return true;
	}
};
}

#pragma GCC diagnostic pop
#endif

bool native_mobile_birth_reset_tail_capture_bounded(int32_t mobile_vnum,
						    int32_t destination_room_vnum,
						    int configured_shop,
						    native_mobile_birth_reset_tail_digest *output,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer_live) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
	{
		errno = ENOTSUP;
		return false;
	}
	constexpr size_t frames =
		native_reset_tail_sha_leaf_frames + sizeof(reset_tail_bounded_hash) +
		sizeof(shop_native_mobile_birth_reset_selection) +
		sizeof(native_mobile_birth_reset_tail_digest) + sizeof(std::array<uint8_t, 8>) +
		18 * sizeof(void *) + 12 * sizeof(int) + 10 * sizeof(size_t) + 5 * sizeof(float) +
		sizeof(double) + 8 * sizeof(bool) + sizeof("NMT1");
	if (!reserve || frames > SIZE_MAX - outer_live || !reserve(outer_live + frames, context))
		return false;
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
	reset_tail_bounded_hash hash;
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
#else
	(void)mobile_vnum;
	(void)destination_room_vnum;
	(void)configured_shop;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer_live;
	errno = ENOTSUP;
	return false;
#endif
}
