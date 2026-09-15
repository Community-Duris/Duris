#include "sql/item_extra_descr_codec.h"

#include "core/defines.h"
#include "sql/sql_player.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
char *duplicate_string(const char *value)
{
	if (!value)
		return nullptr;
	const size_t length = std::strlen(value);
	char *copy = static_cast<char *>(std::malloc(length + 1));
	if (!copy)
		return nullptr;
	std::memcpy(copy, value, length + 1);
	return copy;
}

char *spellbook_to_json(const char *bits)
{
	if (!bits)
		return nullptr;

	constexpr size_t capacity = static_cast<size_t>(MAX_SKILLS) * 12 + 3;
	char *json = static_cast<char *>(std::malloc(capacity));
	if (!json)
		return nullptr;

	size_t used = 0;
	json[used++] = '[';
	json[used] = '\0';
	bool first = true;
	for (int spell = 0; spell < MAX_SKILLS; ++spell)
	{
		if ((static_cast<unsigned char>(bits[spell / 8]) & (1U << (spell % 8))) == 0)
			continue;
		const int written = std::snprintf(json + used, capacity - used, "%s%d",
						  first ? "" : ",", spell);
		if (written < 0 || static_cast<size_t>(written) >= capacity - used)
		{
			std::free(json);
			return nullptr;
		}
		used += static_cast<size_t>(written);
		first = false;
	}
	json[used++] = ']';
	json[used] = '\0';
	return json;
}

bool decode_spellbook_json(const char *json, char *bits)
{
	if (!json || !bits)
		return false;

	std::array<bool, MAX_SKILLS> seen{};
	const char *cursor = json;
	auto skip_whitespace = [&]() {
		while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n')
			++cursor;
	};

	skip_whitespace();
	if (*cursor != '[')
		return false;
	++cursor;
	skip_whitespace();
	if (*cursor == ']')
	{
		++cursor;
		skip_whitespace();
		return *cursor == '\0';
	}

	while (true)
	{
		skip_whitespace();
		if (*cursor < '0' || *cursor > '9')
			return false;

		const char *number_start = cursor;
		unsigned int spell = 0;
		while (*cursor >= '0' && *cursor <= '9')
		{
			const unsigned int digit = static_cast<unsigned int>(*cursor - '0');
			if (spell > static_cast<unsigned int>((MAX_SKILLS - 1 - digit) / 10))
				return false;
			spell = spell * 10 + digit;
			++cursor;
		}

		if ((number_start[0] == '0' && cursor != number_start + 1) ||
		    spell >= static_cast<unsigned int>(MAX_SKILLS) || seen[spell])
			return false;
		seen[spell] = true;
		bits[spell / 8] = static_cast<char>(
			static_cast<unsigned char>(bits[spell / 8]) |
			static_cast<unsigned char>(1U << (spell % 8)));

		skip_whitespace();
		if (*cursor == ']')
		{
			++cursor;
			skip_whitespace();
			return *cursor == '\0';
		}
		if (*cursor != ',')
			return false;
		++cursor;
		skip_whitespace();
		if (*cursor == ']' || *cursor == '\0')
			return false;
	}
}
}

sql_spellbook_decode_status sql_decode_stored_spellbook(const char *keyword,
							const char *description, char *bits,
							size_t bits_size)
{
	const bool canonical = keyword && std::strcmp(keyword, "SPELLBOOK") == 0;
	const bool legacy_raw = sql_item_extra_descr_is_spellbook_marker(keyword);
	if (!canonical && !legacy_raw)
		return sql_spellbook_decode_status::not_spellbook;

	const size_t required = (MAX_SKILLS + 1) / 8 + 1;
	if (!bits || bits_size < required)
		return sql_spellbook_decode_status::invalid;
	std::memset(bits, 0, required);

	if (legacy_raw)
		return sql_spellbook_decode_status::legacy_corrupt;
	if (!decode_spellbook_json(description, bits))
	{
		std::memset(bits, 0, required);
		return sql_spellbook_decode_status::invalid;
	}
	return sql_spellbook_decode_status::decoded;
}

bool sql_encode_item_extra_descr(const char *keyword, const char *description, char **db_keyword,
				 char **db_description)
{
	if (!db_keyword || !db_description)
		return false;

	if (!keyword)
		keyword = "";

	*db_keyword = nullptr;
	*db_description = nullptr;

	const bool native_spellbook = sql_item_extra_descr_is_spellbook_marker(keyword);
	if (native_spellbook)
	{
		*db_keyword = duplicate_string("SPELLBOOK");
		*db_description = description ? spellbook_to_json(description) : duplicate_string("[]");
	}
	else
	{
		*db_keyword = sql_escape_string(keyword);
		if (description)
			*db_description = sql_escape_string(description);
	}

	if (!*db_keyword || (native_spellbook && !*db_description) ||
	    (!native_spellbook && description && !*db_description))
	{
		std::free(*db_keyword);
		std::free(*db_description);
		*db_keyword = nullptr;
		*db_description = nullptr;
		return false;
	}
	return true;
}
