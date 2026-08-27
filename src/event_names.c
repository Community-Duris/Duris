#include "event_names.h"

#include <charconv>
#include <cerrno>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>

namespace
{
constexpr size_t MAX_EVENT_SYMBOL_LENGTH = 4096;

using event_name_map = std::unordered_map<uintptr_t, std::string>;

event_name_map event_names;

bool parse_address(const std::string &text, uintptr_t *address)
{
	uintptr_t parsed = 0;
	const char *begin = text.data();
	const char *end = begin + text.size();
	auto result = std::from_chars(begin, end, parsed, 16);

	if (result.ec != std::errc() || result.ptr != end)
		return false;
	*address = parsed;
	return true;
}
} // namespace

event_name_load_result event_name_registry_load(const char *path, uintptr_t base_address)
{
	event_name_load_result result = {};
	event_name_map loaded_names;
	std::ifstream input;
	std::string line;

	if (!path || !*path)
	{
		result.error_number = EINVAL;
		return result;
	}

	errno = 0;
	input.open(path);
	if (!input.is_open())
	{
		result.error_number = errno ? errno : ENOENT;
		return result;
	}
	result.opened = true;

	while (std::getline(input, line))
	{
		std::istringstream record(line);
		std::string address_text;
		std::string type_text;
		std::string symbol_name;
		std::string trailing;
		uintptr_t offset = 0;

		if (!(record >> address_text >> type_text >> symbol_name) || (record >> trailing) ||
		    (type_text != "T" && type_text != "t") || symbol_name.empty() ||
		    symbol_name.size() > MAX_EVENT_SYMBOL_LENGTH ||
		    !parse_address(address_text, &offset) ||
		    offset > std::numeric_limits<uintptr_t>::max() - base_address)
		{
			result.malformed_lines++;
			continue;
		}

		const uintptr_t address = base_address + offset;
		auto insertion = loaded_names.emplace(address, std::move(symbol_name));
		if (insertion.second)
			result.symbols_loaded++;
		else
			result.duplicate_addresses++;
	}

	if (input.bad())
	{
		result.read_error = true;
		result.error_number = errno ? errno : EIO;
		return result;
	}

	event_names.swap(loaded_names);
	return result;
}

const char *event_name_registry_lookup(const void *func)
{
	const uintptr_t address = reinterpret_cast<uintptr_t>(func);
	auto found = event_names.find(address);

	if (found == event_names.end())
		return nullptr;
	return found->second.c_str();
}

size_t event_name_registry_size()
{
	return event_names.size();
}

void event_name_registry_visit(event_name_registry_visitor visitor, void *context)
{
	if (!visitor)
		return;
	for (const auto &entry : event_names)
	{
		visitor(reinterpret_cast<const void *>(entry.first), entry.second.c_str(), context);
	}
}

void event_name_registry_clear()
{
	event_names.clear();
}
