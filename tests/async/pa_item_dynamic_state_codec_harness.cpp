#include "player/player_snapshot_codec.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}

static bool same_affects(const std::vector<player_item_dynamic_affect_snapshot> &left,
			 const std::vector<player_item_dynamic_affect_snapshot> &right)
{
	if (left.size() != right.size())
		return false;
	for (size_t index = 0; index < left.size(); ++index)
		if (left[index].type != right[index].type ||
		    left[index].data != right[index].data ||
		    left[index].extra2 != right[index].extra2)
			return false;
	return true;
}

int main()
{
	const uint32_t extra2_flags = UINT32_C(0xfedcba98);
	const std::vector<player_item_dynamic_affect_snapshot> expected = {
		{ -321, 1234, UINT64_C(0x1122334455667788) },
		{ INT16_MAX, INT16_MIN, UINT64_MAX },
	};
	std::string encoded;
	require(player_item_properties_encode(extra2_flags, expected, &encoded) ==
				player_snapshot_codec_result::ok &&
			!encoded.empty(),
		"secondary flags and dynamic affects failed to encode");
	uint32_t decoded_flags = 0;
	std::vector<player_item_dynamic_affect_snapshot> decoded;
	require(player_item_properties_decode(encoded, &decoded_flags, &decoded) ==
				player_snapshot_codec_result::ok &&
			decoded_flags == extra2_flags && same_affects(expected, decoded),
		"complete secondary-flag/dynamic-affect payload failed to round trip");

	std::string uppercase = encoded;
	for (char &digit : uppercase)
		if (digit >= 'a' && digit <= 'f')
			digit = static_cast<char>(digit - 'a' + 'A');
	require(player_item_properties_decode(uppercase, &decoded_flags, &decoded) ==
				player_snapshot_codec_result::ok &&
			decoded_flags == extra2_flags && same_affects(expected, decoded),
		"hex payload did not accept uppercase ASCII digits");

	const uint32_t unchanged_flags = UINT32_C(0x12345678);
	const std::vector<player_item_dynamic_affect_snapshot> unchanged = {
		{ 7, 8, 9 },
	};
	auto rejects_without_mutating_outputs =
		[&](const std::string &bad, player_snapshot_codec_result wanted)
	{
		decoded_flags = unchanged_flags;
		decoded = unchanged;
		return player_item_properties_decode(bad, &decoded_flags, &decoded) == wanted &&
		       decoded_flags == unchanged_flags && same_affects(unchanged, decoded);
	};
	std::string bad_magic = encoded;
	bad_magic[0] = bad_magic[0] == '0' ? '1' : '0';
	require(rejects_without_mutating_outputs(bad_magic,
						 player_snapshot_codec_result::invalid_value),
		"corrupt payload magic was accepted or partially published");
	std::string unsupported_version = encoded;
	unsupported_version[8] = '2';
	require(rejects_without_mutating_outputs(unsupported_version,
						 player_snapshot_codec_result::unsupported_version),
		"unknown payload version was accepted or partially published");
	require(rejects_without_mutating_outputs(encoded.substr(0, encoded.size() - 2),
						 player_snapshot_codec_result::truncated),
		"truncated dynamic affect was accepted or partially published");
	require(rejects_without_mutating_outputs(encoded + "00",
						 player_snapshot_codec_result::invalid_value),
		"trailing property bytes were accepted or partially published");
	require(rejects_without_mutating_outputs(encoded + "a",
						 player_snapshot_codec_result::invalid_value),
		"odd-length hex payload was accepted or partially published");
	std::string bad_digit = encoded;
	bad_digit[0] = 'x';
	require(rejects_without_mutating_outputs(bad_digit,
						 player_snapshot_codec_result::invalid_value),
		"non-hex property payload was accepted or partially published");
	std::string oversized(2 * (16 + PLAYER_SNAPSHOT_MAX_ROWS * 12 + 1), '0');
	require(rejects_without_mutating_outputs(oversized,
						 player_snapshot_codec_result::limit_exceeded),
		"oversized dynamic state was accepted or partially published");

	std::string empty_encoded;
	require(player_item_properties_encode(0, {}, &empty_encoded) ==
				player_snapshot_codec_result::ok &&
			player_item_properties_decode(empty_encoded, &decoded_flags, &decoded) ==
				player_snapshot_codec_result::ok &&
			decoded_flags == 0 && decoded.empty(),
		"empty dynamic-affect state did not round trip");
	const std::string column_suffix = player_item_properties_sql_column_suffix();
	std::string value_suffix;
	require(column_suffix == ",item_properties" &&
			player_item_properties_sql_value_suffix(extra2_flags, expected,
								&value_suffix) ==
				player_snapshot_codec_result::ok &&
			value_suffix == ",\'" + encoded + "\'",
		"player-item INSERT fragment did not retain the versioned payload");
	const std::string insert_sql = "INSERT INTO player_items (pid" + column_suffix +
				       ") VALUES (42" + value_suffix + ")";
	require(insert_sql == "INSERT INTO player_items (pid,item_properties) VALUES (42,\'" +
				      encoded + "\')",
		"player-item query builder produced a malformed payload row");

	const std::string row_payload = value_suffix.substr(2, value_suffix.size() - 3);
	const std::string row_length = std::to_string(row_payload.size());
	decoded_flags = 0;
	decoded.clear();
	bool has_payload = false;
	require(player_item_properties_decode_sql_row(row_payload.c_str(), row_length.c_str(),
						      &decoded_flags, &decoded, &has_payload) ==
				player_snapshot_codec_result::ok &&
			has_payload && decoded_flags == extra2_flags &&
			same_affects(expected, decoded),
		"non-NULL item_properties SQL row did not restore overrides");

	decoded_flags = unchanged_flags;
	decoded = unchanged;
	has_payload = true;
	require(player_item_properties_decode_sql_row(nullptr, nullptr, &decoded_flags, &decoded,
						      &has_payload) ==
				player_snapshot_codec_result::ok &&
			!has_payload && decoded_flags == unchanged_flags &&
			same_affects(unchanged, decoded),
		"legacy NULL item_properties row changed payload or enabled overrides");
	require(player_item_properties_decode_sql_row(nullptr, "0", &decoded_flags, &decoded,
						      &has_payload) ==
			player_snapshot_codec_result::invalid_value,
		"inconsistent NULL item_properties SQL row was accepted");

	std::cout << "player item dynamic-state codec and SQL row helpers passed\n";
	return 0;
}
