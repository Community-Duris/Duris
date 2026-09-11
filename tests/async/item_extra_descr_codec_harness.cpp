#include "core/defines.h"
#include "sql/item_extra_descr_codec.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

char *sql_escape_string(const char *input)
{
	if (!input)
		return nullptr;
	std::string escaped;
	for (const char *cursor = input; *cursor; ++cursor)
	{
		if (*cursor == '\'' || *cursor == '\\')
			escaped.push_back(*cursor);
		escaped.push_back(*cursor);
	}
	char *result = static_cast<char *>(std::malloc(escaped.size() + 1));
	if (!result)
		return nullptr;
	std::memcpy(result, escaped.c_str(), escaped.size() + 1);
	return result;
}

static void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}

int main()
{
	const char marker[] = { 3, 1, 3, 0 };
	char bits[(MAX_SKILLS + 1) / 8 + 1] = {};
	bits[1 / 8] |= 1 << (1 % 8);
	bits[203 / 8] |= 1 << (203 % 8);
	bits[(MAX_SKILLS - 1) / 8] |= 1 << ((MAX_SKILLS - 1) % 8);

	char *keyword = nullptr;
	char *description = nullptr;
	require(sql_encode_item_extra_descr(marker, bits, &keyword, &description),
		"spellbook encoding failed");
	require(std::strcmp(keyword, "SPELLBOOK") == 0, "spellbook keyword was not canonicalized");
	std::string json(description ? description : "");
	require(json.front() == '[' && json.back() == ']', "spellbook description is not JSON");
	require(json.find("1") != std::string::npos, "spell 1 missing");
	require(json.find("203") != std::string::npos, "spell 203 missing");
	require(json.find(std::to_string(MAX_SKILLS - 1)) != std::string::npos,
		"highest spell missing");
	std::free(keyword);
	std::free(description);

	char decoded[(MAX_SKILLS + 1) / 8 + 1] = {};
	require(sql_decode_stored_spellbook("SPELLBOOK", "[1,203,1999]", decoded,
					    sizeof(decoded)) ==
			sql_spellbook_decode_status::decoded,
		"canonical spellbook decode failed");
	require((decoded[1 / 8] & (1 << (1 % 8))) != 0, "decoded spell 1 missing");
	require((decoded[203 / 8] & (1 << (203 % 8))) != 0, "decoded spell 203 missing");
	require((decoded[1999 / 8] & (1 << (1999 % 8))) != 0, "decoded highest spell missing");

	char short_output = 0x55;
	require(sql_decode_stored_spellbook("SPELLBOOK", "[1]", &short_output, 1) ==
			sql_spellbook_decode_status::invalid,
		"undersized decode output was accepted");
	require(short_output == 0x55, "undersized decode modified its output");

	std::memset(decoded, 0x55, sizeof(decoded));
	require(sql_decode_stored_spellbook(marker, "truncated", decoded, sizeof(decoded)) ==
			sql_spellbook_decode_status::legacy_corrupt,
		"legacy raw marker was not classified as corrupt");
	for (char value : decoded)
		require(value == 0, "legacy raw marker did not become a safe empty bitmap");

	keyword = nullptr;
	description = nullptr;
	require(sql_encode_item_extra_descr("owner's note", "path\\value", &keyword, &description),
		"plain description encoding failed");
	require(std::strcmp(keyword, "owner''s note") == 0, "plain keyword escaping changed");
	require(std::strcmp(description, "path\\\\value") == 0,
		"plain description escaping changed");
	std::free(keyword);
	std::free(description);

	keyword = nullptr;
	description = reinterpret_cast<char *>(1);
	require(sql_encode_item_extra_descr("plain", nullptr, &keyword, &description),
		"nullable description encoding failed");
	require(std::strcmp(keyword, "plain") == 0, "nullable keyword changed");
	require(description == nullptr, "null description was not preserved");
	std::free(keyword);

	std::cout << "item extra-description spellbook codec passed\n";
	return 0;
}
