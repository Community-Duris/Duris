#include "world/zone_story_quest_tracking.h"

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <system_error>
#include <utility>

namespace zone_story_quest_tracking
{
namespace
{
bool fail(std::string *error, std::string message)
{
	if (error)
		*error = std::move(message);
	return false;
}

bool contains_pid(const std::vector<uint32_t> &pids, uint32_t pid)
{
	return std::find(pids.begin(), pids.end(), pid) != pids.end();
}

bool has_duplicate_pid(const std::vector<uint32_t> &pids)
{
	for (size_t index = 0; index < pids.size(); ++index)
	{
		if (std::find(pids.begin(), pids.begin() + index, pids[index]) !=
		    pids.begin() + index)
			return true;
	}
	return false;
}

char hex_digit(uint8_t value)
{
	return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('a' + (value - 10));
}

int hex_value(char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

std::string hex_encode(std::string_view value)
{
	std::string encoded;
	encoded.reserve(value.size() * 2);
	for (unsigned char byte : value)
	{
		encoded.push_back(hex_digit(static_cast<uint8_t>(byte >> 4)));
		encoded.push_back(hex_digit(static_cast<uint8_t>(byte & 0x0f)));
	}
	return encoded;
}

bool hex_decode(std::string_view encoded, std::string *decoded)
{
	if (!decoded || encoded.size() % 2 != 0)
		return false;
	decoded->clear();
	decoded->reserve(encoded.size() / 2);
	for (size_t index = 0; index < encoded.size(); index += 2)
	{
		const int high = hex_value(encoded[index]);
		const int low = hex_value(encoded[index + 1]);
		if (high < 0 || low < 0)
			return false;
		decoded->push_back(static_cast<char>((high << 4) | low));
	}
	return true;
}

template <typename T> bool parse_integer(std::string_view token, T *value)
{
	if (!value || token.empty())
		return false;
	T parsed = {};
	const char *begin = token.data();
	const char *end = begin + token.size();
	const auto result = std::from_chars(begin, end, parsed);
	if (result.ec != std::errc() || result.ptr != end)
		return false;
	*value = parsed;
	return true;
}

std::vector<std::string_view> split_fields(std::string_view encoded)
{
	std::vector<std::string_view> fields;
	size_t begin = 0;
	while (begin <= encoded.size())
	{
		const size_t separator = encoded.find('|', begin);
		if (separator == std::string_view::npos)
		{
			fields.push_back(encoded.substr(begin));
			break;
		}
		fields.push_back(encoded.substr(begin, separator - begin));
		begin = separator + 1;
	}
	return fields;
}

std::string serialize_pids(const std::vector<uint32_t> &pids)
{
	std::vector<uint32_t> ordered = pids;
	std::sort(ordered.begin(), ordered.end());
	std::string encoded;
	for (size_t index = 0; index < ordered.size(); ++index)
	{
		if (index)
			encoded.push_back(',');
		encoded += std::to_string(ordered[index]);
	}
	return encoded;
}
} // namespace

bool validate_definition(const quest_definition &definition, std::string *error)
{
	if (definition.definition_id.empty())
		return fail(error, "definition_id must be non-empty");
	if (definition.source_system != ZONE_STORY_QUEST_SOURCE_SYSTEM)
		return fail(error, "source_system must be zone_story");
	if (definition.zone_number <= 0)
		return fail(error, "zone_number must be positive");
	if (definition.source_area.empty())
		return fail(error, "source_area must be non-empty");
	if (definition.giver_vnum <= 0)
		return fail(error, "giver_vnum must be positive");
	if (definition.completion_key.empty())
		return fail(error, "completion_key must be non-empty");
	if (definition.content_revision == 0)
		return fail(error, "content_revision must be positive");
	if (!definition.repeatable)
		return fail(error, "zone-story definitions must be repeatable");
	return true;
}

bool validate_transaction(const completion_transaction &transaction, std::string *error)
{
	if (transaction.schema_version != ZONE_STORY_QUEST_TRACKING_SCHEMA_VERSION)
		return fail(error, "unsupported transaction schema_version");
	if (transaction.transaction_id.empty())
		return fail(error, "transaction_id must be non-empty");
	if (transaction.quest_definition_id.empty())
		return fail(error, "quest_definition_id must be non-empty");
	if (transaction.zone_number <= 0)
		return fail(error, "zone_number must be positive");
	if (transaction.direct_completer_pid == 0)
		return fail(error, "direct_completer_pid must be positive");
	if (transaction.credited_pids.empty())
		return fail(error, "credited_pids must not be empty");
	if (transaction.room_vnum <= 0)
		return fail(error, "room_vnum must be positive");
	if (transaction.completed_at <= 0)
		return fail(error, "completed_at must be positive");
	if (transaction.season_id == 0)
		return fail(error, "season_id must be positive");
	if (transaction.content_revision == 0)
		return fail(error, "content_revision must be positive");
	for (uint32_t pid : transaction.credited_pids)
		if (pid == 0)
			return fail(error, "credited_pids must contain positive PIDs");
	if (has_duplicate_pid(transaction.credited_pids))
		return fail(error, "credited_pids must not contain duplicates");
	if (!contains_pid(transaction.credited_pids, transaction.direct_completer_pid))
		return fail(error, "direct_completer_pid must receive credit");
	return true;
}

uint32_t credit_mask_for_pid(const completion_transaction &transaction, uint32_t pid)
{
	if (pid == 0 || !contains_pid(transaction.credited_pids, pid))
		return ZONE_STORY_CREDIT_NONE;

	uint32_t mask = ZONE_STORY_CREDIT_NONE;
	if (pid == transaction.direct_completer_pid)
		mask |= ZONE_STORY_CREDIT_PERSONAL;
	if (transaction.credited_pids.size() == 1)
	{
		if (pid == transaction.direct_completer_pid)
			mask |= ZONE_STORY_CREDIT_SOLO;
	}
	else
	{
		mask |= ZONE_STORY_CREDIT_GROUP_PARTICIPANT;
		if (pid == transaction.direct_completer_pid)
			mask |= ZONE_STORY_CREDIT_LEADERSHIP;
	}
	return mask;
}

bool is_solo_transaction(const completion_transaction &transaction)
{
	return transaction.credited_pids.size() == 1 &&
	       transaction.credited_pids.front() == transaction.direct_completer_pid;
}

bool is_leadership_transaction(const completion_transaction &transaction)
{
	return transaction.credited_pids.size() > 1 &&
	       contains_pid(transaction.credited_pids, transaction.direct_completer_pid);
}

std::string serialize_transaction(const completion_transaction &transaction, std::string *error)
{
	if (!validate_transaction(transaction, error))
		return {};
	return "v" + std::to_string(transaction.schema_version) + "|" +
	       hex_encode(transaction.transaction_id) + "|" +
	       hex_encode(transaction.quest_definition_id) + "|" +
	       std::to_string(transaction.zone_number) + "|" +
	       std::to_string(transaction.direct_completer_pid) + "|" +
	       std::to_string(transaction.room_vnum) + "|" +
	       std::to_string(transaction.completed_at) + "|" +
	       std::to_string(transaction.season_id) + "|" +
	       std::to_string(transaction.content_revision) + "|" +
	       serialize_pids(transaction.credited_pids);
}

bool deserialize_transaction(std::string_view encoded, completion_transaction *transaction,
			     std::string *error)
{
	if (!transaction)
		return fail(error, "transaction output must not be null");
	*transaction = {};

	const std::vector<std::string_view> fields = split_fields(encoded);
	if (fields.size() != 10 || fields[0].size() < 2 || fields[0][0] != 'v')
		return fail(error, "invalid transaction field count or version");
	if (!parse_integer(fields[0].substr(1), &transaction->schema_version))
		return fail(error, "invalid transaction schema version");
	if (!hex_decode(fields[1], &transaction->transaction_id) ||
	    !hex_decode(fields[2], &transaction->quest_definition_id))
		return fail(error, "invalid hexadecimal transaction identity");
	if (!parse_integer(fields[3], &transaction->zone_number) ||
	    !parse_integer(fields[4], &transaction->direct_completer_pid) ||
	    !parse_integer(fields[5], &transaction->room_vnum) ||
	    !parse_integer(fields[6], &transaction->completed_at) ||
	    !parse_integer(fields[7], &transaction->season_id) ||
	    !parse_integer(fields[8], &transaction->content_revision))
		return fail(error, "invalid transaction numeric field");

	if (fields[9].empty())
		return fail(error, "credited PID field must not be empty");
	size_t begin = 0;
	while (begin < fields[9].size())
	{
		const size_t separator = fields[9].find(',', begin);
		const size_t end = separator == std::string_view::npos ? fields[9].size() :
									 separator;
		uint32_t pid = 0;
		if (!parse_integer(fields[9].substr(begin, end - begin), &pid))
			return fail(error, "invalid credited PID");
		transaction->credited_pids.push_back(pid);
		if (separator == std::string_view::npos)
			break;
		begin = separator + 1;
		if (begin == fields[9].size())
			return fail(error, "trailing credited PID separator");
	}
	return validate_transaction(*transaction, error);
}
} // namespace zone_story_quest_tracking
