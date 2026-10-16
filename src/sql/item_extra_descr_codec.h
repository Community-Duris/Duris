#ifndef ITEM_EXTRA_DESCR_CODEC_H
#define ITEM_EXTRA_DESCR_CODEC_H

#include <cstddef>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <limits>

enum class sql_spellbook_decode_status
{
	not_spellbook,
	decoded,
	legacy_corrupt,
	invalid
};

// Return whether KEYWORD is the native three-byte spellbook marker.
inline bool sql_item_extra_descr_is_spellbook_marker(const char *keyword)
{
	const char marker[] = { 3, 1, 3, 0 };
	return keyword && std::strlen(keyword) == sizeof(marker) - 1 &&
	       std::memcmp(keyword, marker, sizeof(marker) - 1) == 0;
}

constexpr const char *SQL_ITEM_TRAP_STATE_KEYWORD = "DURIS_TRAP_STATE_V1";

enum class sql_item_trap_state_decode_status
{
	not_trap_state,
	decoded,
	invalid
};

inline bool sql_encode_item_trap_state(int16_t effect, int16_t damage, int16_t charges,
				       int16_t level, char *description, size_t description_size)
{
	if (!description || !description_size)
		return false;
	char *cursor = description;
	char *end = description + description_size - 1;
	const int16_t fields[] = { effect, damage, charges, level };
	for (size_t index = 0; index < 4; ++index)
	{
		const auto encoded = std::to_chars(cursor, end, fields[index]);
		if (encoded.ec != std::errc())
			return false;
		cursor = encoded.ptr;
		if (index < 3)
		{
			if (cursor == end)
				return false;
			*cursor++ = ',';
		}
	}
	*cursor = '\0';
	return true;
}

inline sql_item_trap_state_decode_status
sql_decode_item_trap_state(const char *keyword, const char *description, int16_t *effect,
			   int16_t *damage, int16_t *charges, int16_t *level)
{
	if (!keyword || std::strcmp(keyword, SQL_ITEM_TRAP_STATE_KEYWORD) != 0)
		return sql_item_trap_state_decode_status::not_trap_state;
	if (!description || !effect || !damage || !charges || !level)
		return sql_item_trap_state_decode_status::invalid;
	const char *cursor = description;
	const char *end = description + std::strlen(description);
	int16_t *fields[] = { effect, damage, charges, level };
	for (size_t index = 0; index < 4; ++index)
	{
		int value = 0;
		const auto parsed = std::from_chars(cursor, end, value);
		if (parsed.ec != std::errc() || parsed.ptr == cursor ||
		    value < std::numeric_limits<int16_t>::min() ||
		    value > std::numeric_limits<int16_t>::max() ||
		    (index < 3 ? parsed.ptr == end || *parsed.ptr != ',' : parsed.ptr != end))
			return sql_item_trap_state_decode_status::invalid;
		*fields[index] = static_cast<int16_t>(value);
		cursor = parsed.ptr + (index < 3 ? 1 : 0);
	}
	return sql_item_trap_state_decode_status::decoded;
}

// Decode a stored canonical spellbook row. Legacy raw-marker rows cannot be
// reconstructed, so they become a full-sized empty bitmap and are reported as
// legacy_corrupt. No bytes are read from a legacy truncated description.
sql_spellbook_decode_status sql_decode_stored_spellbook(const char *keyword,
							const char *description, char *bits,
							size_t bits_size);

// Convert an in-memory item extra description to the canonical escaped SQL
// representation. Native spellbook descriptions must point to the complete
// fixed-size bitmap used by extra_descr_data. The caller owns both returned
// allocations and must free them. A null plain description remains null; a
// null native spellbook bitmap is normalized to an empty JSON array. Spellbook
// bitsets are encoded as SPELLBOOK plus a JSON array so binary zero bytes never
// reach string-based SQL escaping.
bool sql_encode_item_extra_descr(const char *keyword, const char *description, char **db_keyword,
				 char **db_description);

#endif
