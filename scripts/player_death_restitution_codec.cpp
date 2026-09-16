#include "player/player_snapshot_codec.h"
#include "core/defines.h"

#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::string hex_bytes(const uint8_t *data, size_t size)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result;
	result.reserve(size * 2);
	for (size_t index = 0; index < size; ++index)
	{
		result.push_back(digits[data[index] >> 4]);
		result.push_back(digits[data[index] & 0x0f]);
	}
	return result;
}

std::string hex_bytes(const std::string &value)
{
	return hex_bytes(reinterpret_cast<const uint8_t *>(value.data()), value.size());
}

void json_string(std::ostream &out, const std::string &value)
{
	out << '"';
	for (const unsigned char byte : value)
	{
		switch (byte)
		{
		case '"':
			out << "\\\"";
			break;
		case '\\':
			out << "\\\\";
			break;
		case '\b':
			out << "\\b";
			break;
		case '\f':
			out << "\\f";
			break;
		case '\n':
			out << "\\n";
			break;
		case '\r':
			out << "\\r";
			break;
		case '\t':
			out << "\\t";
			break;
		default:
			if (byte < 0x20)
			{
				out << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
				    << static_cast<unsigned int>(byte) << std::dec
				    << std::setfill(' ');
			}
			else
				out << static_cast<char>(byte);
			break;
		}
	}
	out << '"';
}

void json_hex_string(std::ostream &out, const std::string &value)
{
	json_string(out, hex_bytes(value));
}

template <typename T, typename Write>
void json_array(std::ostream &out, const T &values, Write write)
{
	out << '[';
	bool first = true;
	for (const auto &value : values)
	{
		if (!first)
			out << ',';
		first = false;
		write(out, value);
	}
	out << ']';
}

bool has_name_token(const std::string &value, const std::string &token)
{
	size_t begin = 0;
	while (begin < value.size())
	{
		while (begin < value.size() &&
		       !std::isalnum(static_cast<unsigned char>(value[begin])))
			++begin;
		const size_t end = begin;
		while (begin < value.size() &&
		       std::isalnum(static_cast<unsigned char>(value[begin])))
			++begin;
		if (begin - end == token.size())
		{
			bool matches = true;
			for (size_t index = 0; index < token.size(); ++index)
			{
				const auto character =
					static_cast<unsigned char>(value[end + index]);
				if (std::tolower(character) !=
				    static_cast<unsigned char>(token[index]))
				{
					matches = false;
					break;
				}
			}
			if (matches)
				return true;
		}
	}
	return false;
}

void write_item(std::ostream &out, const player_item_snapshot &item, bool include_payload)
{
	out << '{';
	out << "\"parent_index\":" << item.parent_index;
	out << ",\"equipment_slot\":" << item.equipment_slot;
	out << ",\"object_uid\":" << item.object_uid;
	out << ",\"generated_key\":" << item.generated_key;
	out << ",\"vnum\":" << item.vnum;
	out << ",\"type\":" << static_cast<int>(item.type);
	out << ",\"string_mask\":" << static_cast<unsigned int>(item.string_mask);
	out << ",\"name_hex\":";
	json_hex_string(out, item.name);
	out << ",\"short_description_hex\":";
	json_hex_string(out, item.short_description);
	out << ",\"description_hex\":";
	json_hex_string(out, item.description);
	out << ",\"action_description_hex\":";
	json_hex_string(out, item.action_description);
	out << ",\"values\":";
	json_array(out, item.values, [](std::ostream &stream, int32_t value) { stream << value; });
	out << ",\"timers\":";
	json_array(out, item.timers, [](std::ostream &stream, int64_t value) { stream << value; });
	out << ",\"wear_flags\":" << item.wear_flags;
	out << ",\"extra_flags\":" << item.extra_flags;
	const bool native_artifact_flag = (item.extra_flags & ITEM_ARTIFACT) != 0;
	const bool native_unique_name = has_name_token(item.name, "unique") &&
					!has_name_token(item.name, "powerunique");
	out << ",\"native_artifact_flag\":" << (native_artifact_flag ? "true" : "false");
	out << ",\"native_unique_name_marker\":" << (native_unique_name ? "true" : "false");
	out << ",\"artifact_identity_vnum\":" << item.vnum;
	out << ",\"anti_flags\":" << item.anti_flags;
	out << ",\"anti2_flags\":" << item.anti2_flags;
	out << ",\"extra2_flags\":" << item.extra2_flags;
	out << ",\"weight\":" << item.weight;
	out << ",\"material\":" << static_cast<int>(item.material);
	out << ",\"cost\":" << item.cost;
	out << ",\"condition\":" << item.condition;
	out << ",\"craftsmanship\":" << item.craftsmanship;
	out << ",\"bitvectors\":";
	json_array(out, item.bitvectors,
		   [](std::ostream &stream, uint64_t value) { stream << value; });
	out << ",\"affects\":[";
	for (size_t index = 0; index < item.affects.size(); ++index)
	{
		if (index)
			out << ',';
		out << '[' << item.affects[index][0] << ',' << item.affects[index][1] << ']';
	}
	out << ']';
	out << ",\"dynamic_affects\":[";
	for (size_t index = 0; index < item.dynamic_affects.size(); ++index)
	{
		if (index)
			out << ',';
		const auto &affect = item.dynamic_affects[index];
		out << "{\"type\":" << affect.type << ",\"data\":" << affect.data
		    << ",\"extra2\":" << affect.extra2 << '}';
	}
	out << "],\"extra_descriptions\":[";
	for (size_t index = 0; index < item.extra_descriptions.size(); ++index)
	{
		if (index)
			out << ',';
		const auto &description = item.extra_descriptions[index];
		out << "{\"keyword_hex\":";
		json_hex_string(out, description.keyword);
		out << ",\"description_hex\":";
		json_hex_string(out, description.description);
		out << ",\"spellbook\":" << (description.spellbook ? "true" : "false")
		    << ",\"spell_ids\":";
		json_array(out, description.spell_ids,
			   [](std::ostream &stream, int32_t value) { stream << value; });
		out << '}';
	}
	out << ']';
	if (include_payload)
	{
		player_item_snapshot standalone = item;
		standalone.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		std::vector<player_item_snapshot> one = { standalone };
		std::vector<uint8_t> encoded;
		if (player_item_snapshot_list_encode(one, &encoded) !=
		    player_snapshot_codec_result::ok)
		{
			std::cerr << "item encoding failed\n";
			std::exit(2);
		}
		out << ",\"item_payload_hex\":";
		json_string(out, hex_bytes(encoded.data(), encoded.size()));
	}
	out << '}';
}

void write_operation(std::ostream &out, const critical_operation_id &operation_id)
{
	json_string(out, hex_bytes(operation_id.bytes.data(), operation_id.bytes.size()));
}
}

int main(int argc, char **argv)
{
	if (argc != 2 || std::string(argv[1]) != "decode-death")
	{
		std::cerr << "usage: player_death_restitution_codec decode-death < payload\n";
		return 2;
	}
	const std::vector<uint8_t> encoded((std::istreambuf_iterator<char>(std::cin)),
					   std::istreambuf_iterator<char>());
	if (encoded.size() < sizeof(uint32_t))
	{
		std::cerr << "payload is truncated\n";
		return 3;
	}
	const uint32_t wire_version = static_cast<uint32_t>(encoded[0]) |
				      (static_cast<uint32_t>(encoded[1]) << 8) |
				      (static_cast<uint32_t>(encoded[2]) << 16) |
				      (static_cast<uint32_t>(encoded[3]) << 24);
	player_snapshot snapshot = {};
	const auto result = player_snapshot_decode(encoded.data(), encoded.size(), &snapshot);
	if (result != player_snapshot_codec_result::ok || !snapshot.death)
	{
		std::cerr << "death payload rejected by player_snapshot_codec\n";
		return 4;
	}
	const auto &death = *snapshot.death;
	std::cout << "{\"wire_version\":" << wire_version
		  << ",\"schema_version\":" << snapshot.schema_version
		  << ",\"pid\":" << snapshot.pid << ",\"revision\":" << snapshot.revision
		  << ",\"save_intent\":" << snapshot.save_intent << ",\"death\":{"
		  << "\"operation_id_hex\":";
	write_operation(std::cout, death.operation_id);
	std::cout << ",\"corpse_room_vnum\":" << death.corpse_room_vnum
		  << ",\"wallet_revision\":" << death.wallet_revision << ",\"wallet_before\":";
	json_array(std::cout, death.wallet_before,
		   [](std::ostream &stream, int32_t value) { stream << value; });
	std::cout << ",\"wallet_pile_uid\":" << death.wallet_pile_uid << ",\"corpse\":[";
	for (size_t index = 0; index < death.corpse.size(); ++index)
	{
		if (index)
			std::cout << ',';
		write_item(std::cout, death.corpse[index], true);
	}
	std::cout << "],\"custody\":[";
	for (size_t index = 0; index < death.custody.size(); ++index)
	{
		if (index)
			std::cout << ',';
		const auto &row = death.custody[index];
		std::cout << "{\"item_uid\":" << row.item.item_uid
			  << ",\"root_item_uid\":" << row.item.root_item_uid
			  << ",\"parent_item_uid\":" << row.item.parent_item_uid
			  << ",\"expected_item_revision\":" << row.item.expected_item_revision
			  << ",\"vnum\":" << row.item.vnum << ",\"expected_state\":"
			  << static_cast<unsigned int>(row.item.expected_state)
			  << ",\"owner_type\":" << static_cast<unsigned int>(row.owner.type)
			  << ",\"owner_id\":" << row.owner.id
			  << ",\"owner_context_id\":" << row.owner.context_id
			  << ",\"owner_revision\":" << row.owner_revision << '}';
	}
	std::cout << "],\"unresolved_operations\":[";
	for (size_t index = 0; index < death.unresolved_operations.size(); ++index)
	{
		if (index)
			std::cout << ',';
		write_operation(std::cout, death.unresolved_operations[index]);
	}
	std::cout << "]}}\n";
	return 0;
}
