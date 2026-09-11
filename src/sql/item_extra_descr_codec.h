#ifndef ITEM_EXTRA_DESCR_CODEC_H
#define ITEM_EXTRA_DESCR_CODEC_H

#include <cstddef>
#include <cstring>

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

// Decode a stored canonical spellbook row. Legacy raw-marker rows cannot be
// reconstructed, so they become a full-sized empty bitmap and are reported as
// legacy_corrupt. No bytes are read from a legacy truncated description.
sql_spellbook_decode_status sql_decode_stored_spellbook(const char *keyword,
							const char *description, char *bits,
							size_t bits_size);

// Convert an in-memory item extra description to the canonical escaped SQL
// representation. Native spellbook descriptions must point to the complete
// fixed-size bitmap used by extra_descr_data. The caller owns both returned
// allocations and must free them. A null description remains null. Spellbook
// bitsets are encoded as SPELLBOOK plus a JSON array so binary zero bytes never
// reach string-based SQL escaping.
bool sql_encode_item_extra_descr(const char *keyword, const char *description, char **db_keyword,
				 char **db_description);

#endif
