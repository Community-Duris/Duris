#include "player/pet_restore_state.h"

#include <algorithm>
#include <limits>
#include <new>
#include <type_traits>

namespace
{
struct writer
{
	std::string bytes;
	template <typename T> void number(T value)
	{
		using U = std::make_unsigned_t<T>;
		U bits = static_cast<U>(value);
		constexpr char hex[] = "0123456789abcdef";
		for (size_t i = 0; i < sizeof(T); ++i, bits >>= 8)
		{
			bytes += hex[(bits & 255) >> 4];
			bytes += hex[bits & 15];
		}
	}
	void string(const std::string &value)
	{
		number<uint32_t>(value.size());
		for (unsigned char byte : value)
			number<uint8_t>(byte);
	}
};

struct reader
{
	const std::string &bytes;
	size_t offset = 0;
	bool valid = true;
	int nibble(char c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		valid = false;
		return 0;
	}
	template <typename T> void number(T &value)
	{
		if (!valid || bytes.size() - offset < sizeof(T) * 2)
		{
			valid = false;
			return;
		}
		using U = std::make_unsigned_t<T>;
		U bits = 0;
		for (size_t i = 0; i < sizeof(T); ++i)
		{
			const unsigned high = nibble(bytes[offset++]);
			const unsigned low = nibble(bytes[offset++]);
			bits |= static_cast<U>((high << 4) | low) << (i * 8);
		}
		value = static_cast<T>(bits);
	}
	void string(std::string &value)
	{
		uint32_t length = 0;
		number(length);
		if (!valid || length > PET_RESTORE_STATE_MAX_TEXT ||
		    length > (bytes.size() - offset) / 2)
		{
			valid = false;
			return;
		}
		value.resize(length);
		for (char &c : value)
		{
			uint8_t byte = 0;
			number(byte);
			c = static_cast<char>(byte);
			if (!c)
				valid = false;
		}
	}
};

template <typename IO, typename State> void fields(IO &io, State &s)
{
	io.number(s.version);
	uint32_t kind = static_cast<uint32_t>(s.kind);
	io.number(kind);
	if constexpr (!std::is_const_v<State>)
		s.kind = static_cast<summoned_pet_kind>(kind);
	io.number(s.charm_expires_at);
	io.number(s.death_expires_at);
	io.string(s.name);
	io.string(s.short_description);
	io.string(s.long_description);
	for (auto &v : s.base_stats)
		io.number(v);
	for (auto &v : s.base_points)
		io.number(v);
	for (auto &v : s.damage_dice)
		io.number(v);
	for (auto &v : s.spell_slots)
		io.number(v);
	for (auto &v : s.intrinsic_affects)
		io.number(v);
	for (auto &v : s.aggression)
		io.number(v);
	io.number(s.act);
	io.number(s.primary_class);
	io.number(s.secondary_class);
	io.number(s.level);
	io.number(s.race);
	io.number(s.sex);
	io.number(s.size);
	io.number(s.alignment);
}

bool valid(const pet_restore_state &s)
{
	const auto text_ok = [](const std::string &v)
	{
		return !v.empty() && v.size() <= PET_RESTORE_STATE_MAX_TEXT &&
		       v.find('\0') == std::string::npos;
	};
	return s.version == 1 && static_cast<uint32_t>(s.kind) <= 22 && s.charm_expires_at >= 0 &&
	       s.death_expires_at >= 0 && s.charm_expires_at <= INT64_MAX / 16 &&
	       s.death_expires_at <= INT64_MAX / 16 && text_ok(s.name) &&
	       text_ok(s.short_description) && text_ok(s.long_description) && s.level > 0 &&
	       s.level <= 255 && s.race >= 0 && s.race <= 255 && s.sex >= 0 && s.sex <= 3 &&
	       s.size >= 0 && s.size <= 255 && s.alignment >= -1000 && s.alignment <= 1000 &&
	       s.base_points[0] > 0 && s.base_points[1] >= 0 && s.base_points[1] <= INT16_MAX &&
	       s.base_points[2] >= 0 && s.base_points[2] <= INT16_MAX &&
	       s.base_points[3] >= INT16_MIN && s.base_points[3] <= INT16_MAX &&
	       s.base_points[6] >= 0 && s.base_points[6] <= INT16_MAX &&
	       std::all_of(s.damage_dice.begin(), s.damage_dice.end(),
			   [](int32_t v) { return v >= 0 && v <= INT8_MAX; }) &&
	       std::all_of(s.base_stats.begin(), s.base_stats.end(),
			   [](int32_t v) { return v >= 0 && v <= 1000; }) &&
	       std::all_of(s.spell_slots.begin(), s.spell_slots.end(),
			   [](int32_t v) { return v >= 0 && v <= INT8_MAX; });
}
}

bool pet_restore_state_encode(const pet_restore_state &state, std::string *encoded)
{
	if (!encoded || !valid(state))
		return false;
	try
	{
		writer out;
		fields(out, state);
		if (out.bytes.size() > PET_RESTORE_STATE_MAX_BYTES)
			return false;
		*encoded = std::move(out.bytes);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool pet_restore_state_decode(const std::string &encoded, pet_restore_state *state)
{
	if (!state || encoded.size() > PET_RESTORE_STATE_MAX_BYTES || encoded.size() % 2)
		return false;
	try
	{
		pet_restore_state candidate;
		reader in{ encoded };
		fields(in, candidate);
		if (!in.valid || in.offset != encoded.size() || !valid(candidate))
			return false;
		*state = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

int summoned_pet_cost(summoned_pet_kind kind)
{
	const auto value = static_cast<uint32_t>(kind);
	constexpr int costs[] = { 10, 10, 10, 22, 28, 28, 65, 10, 10, 10, 22, 28, 28, 65 };
	if (value >= 1 && value <= 14)
		return costs[value - 1];
	if (value >= 15 && value <= 18)
		return 6;
	if (kind == summoned_pet_kind::titan || kind == summoned_pet_kind::dracolich)
		return 37;
	if (kind == summoned_pet_kind::avatar || kind == summoned_pet_kind::greater_dracolich)
		return 75;
	return 0;
}

bool legacy_summon_prototype(int vnum)
{
	return vnum == 1201 || (vnum >= 3 && vnum <= 10) || (vnum >= 78 && vnum <= 85) ||
	       (vnum >= 33 && vnum <= 35) || vnum == 77;
}

bool summoned_pet_matches_prototype(summoned_pet_kind kind, int vnum)
{
	const auto value = static_cast<uint32_t>(kind);
	if (value >= 1 && value <= 14)
		return vnum == 1201;
	constexpr int golems[] = { 34, 35, 33, 77 };
	if (value >= 15 && value <= 18)
		return vnum == golems[value - 15];
	if (kind == summoned_pet_kind::dracolich)
		return vnum >= 3 && vnum <= 6;
	if (kind == summoned_pet_kind::greater_dracolich)
		return vnum >= 7 && vnum <= 10;
	if (kind == summoned_pet_kind::titan)
		return vnum >= 78 && vnum <= 81;
	if (kind == summoned_pet_kind::avatar)
		return vnum >= 82 && vnum <= 85;
	return false;
}
