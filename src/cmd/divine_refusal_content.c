#include "cmd/divine_refusal_content.h"

#include <algorithm>
#include <charconv>
#include <cjson/cJSON.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>

namespace
{
[[noreturn]] void invalid(const std::string &where, const char *reason)
{
	throw std::invalid_argument(where + ": " + reason);
}

const cJSON *field(const cJSON *object, const char *name)
{
	return cJSON_GetObjectItemCaseSensitive(object, name);
}

void fields(const cJSON *object, std::initializer_list<std::string_view> allowed,
	    const std::string &where)
{
	if (!cJSON_IsObject(object))
		invalid(where, "expected an object");
	std::set<std::string_view> seen;
	for (const cJSON *item = object->child; item; item = item->next)
	{
		if (!item->string || std::find(allowed.begin(), allowed.end(),
					       std::string_view(item->string)) == allowed.end())
			invalid(where, "unknown field");
		if (!seen.insert(item->string).second)
			invalid(where, "duplicate field");
	}
}

void preflight(std::string_view json)
{
	if (json.empty() || json.size() > DIVINE_REFUSAL_CONTENT_MAX_BYTES ||
	    json.find('\0') != std::string_view::npos)
		invalid("configuration", "empty, oversized, or embedded NUL input");

	int depth = 0;
	bool quoted = false;
	bool escaped = false;
	for (size_t i = 0; i < json.size(); ++i)
	{
		const char ch = json[i];
		if (quoted)
		{
			if (escaped)
			{
				if (ch == 'u' && i + 4 < json.size() && json[i + 1] == '0' &&
				    json[i + 2] == '0' && json[i + 3] == '0' && json[i + 4] == '0')
					invalid("configuration", "NUL escapes are unsupported");
				escaped = false;
			}
			else if (ch == '\\')
				escaped = true;
			else if (ch == '"')
				quoted = false;
		}
		else if (ch == '"')
			quoted = true;
		else if (ch == '{' || ch == '[')
		{
			if (++depth > 12)
				invalid("configuration", "nesting exceeds 12 levels");
		}
		else if (ch == '}' || ch == ']')
			--depth;
	}
}

uint32_t integer(const cJSON *value, uint32_t minimum, uint32_t maximum, const std::string &where)
{
	if (!cJSON_IsNumber(value) || !std::isfinite(value->valuedouble) ||
	    std::floor(value->valuedouble) != value->valuedouble || value->valuedouble < minimum ||
	    value->valuedouble > maximum)
		invalid(where, "expected an integer in the documented range");
	return static_cast<uint32_t>(value->valuedouble);
}

std::string text_value(const cJSON *value, const std::string &where, size_t maximum,
		       bool allow_empty = false)
{
	if (!cJSON_IsString(value) || !value->valuestring)
		invalid(where, "expected a string");
	std::string result(value->valuestring);
	if ((!allow_empty && result.empty()) || result.size() > maximum)
		invalid(where, "empty or oversized text");
	for (unsigned char ch : result)
		if (ch < 0x20 || ch == 0x7f)
			invalid(where, "text contains a control character");
	return result;
}

void bounded_object(const cJSON *object, size_t maximum, const std::string &where)
{
	if (!cJSON_IsObject(object) || static_cast<size_t>(cJSON_GetArraySize(object)) > maximum)
		invalid(where, "expected an object within the documented entry limit");
}

int vnum_key(const char *key, const std::string &where)
{
	if (!key || !*key)
		invalid(where, "template key must be a decimal vnum");
	std::string_view text(key);
	if (text.size() > 7 || (text.size() > 1 && text.front() == '0'))
		invalid(where, "template key must be a canonical decimal vnum");
	for (char ch : text)
		if (ch < '0' || ch > '9')
			invalid(where, "template key must be a decimal vnum");

	int result = 0;
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
	if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() ||
	    result < DIVINE_REFUSAL_CONTENT_MIN_VNUM || result > DIVINE_REFUSAL_CONTENT_MAX_VNUM)
		invalid(where, "template vnum is outside the documented range");
	return result;
}

bool valid_message_template(const std::string &message, const std::string &where)
{
	size_t token_count = 0;
	for (size_t i = 0; i < message.size(); ++i)
	{
		if (message[i] == '{')
		{
			constexpr std::string_view token = "{patron}";
			if (message.compare(i, token.size(), token) != 0)
				invalid(where, "only the {patron} placeholder is supported");
			++token_count;
			i += token.size() - 1;
		}
		else if (message[i] == '}')
			invalid(where, "unmatched template brace");
	}
	if (token_count > 1)
		invalid(where, "the {patron} placeholder may appear at most once");
	return true;
}

std::string lowercase(std::string_view value)
{
	std::string result(value);
	for (char &ch : result)
		if (ch >= 'A' && ch <= 'Z')
			ch += 'a' - 'A';
	return result;
}

std::string canonical_patron(std::string_view value)
{
	// This is intentionally a small reviewed catalog. It is not derived from
	// the PC race/spec deity table: the content row itself must establish the
	// NPC's authored patron before a name can be rendered.
	const std::string normalized = lowercase(value);
	if (normalized == "garl" || normalized == "garl glittergold")
		return "Garl";
	return {};
}

} // namespace

std::shared_ptr<DivineRefusalContentSnapshot> divine_refusal_content_parse(std::string_view json)
{
	preflight(json);
	std::string terminated(json);
	std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(
		cJSON_ParseWithOpts(terminated.c_str(), nullptr, true), cJSON_Delete);
	if (!root)
		invalid("configuration", "invalid JSON or trailing content");
	fields(root.get(), { "version", "revision", "entries" }, "configuration");
	if (integer(field(root.get(), "version"), 1, UINT32_MAX, "version") != 1)
		invalid("version", "unsupported schema version");

	auto result = std::make_shared<DivineRefusalContentSnapshot>();
	result->revision_ = integer(field(root.get(), "revision"), 1, UINT32_MAX, "revision");
	const cJSON *entries = field(root.get(), "entries");
	bounded_object(entries, DIVINE_REFUSAL_CONTENT_MAX_ENTRIES, "entries");
	for (const cJSON *item = entries->child; item; item = item->next)
	{
		const std::string where =
			std::string("entries.") + (item->string ? item->string : "");
		const int vnum = vnum_key(item->string, where);
		fields(item, { "patron", "message", "enabled", "percent" }, where);

		DivineRefusalContentEntry entry;
		if (const cJSON *value = field(item, "patron"))
			entry.patron = text_value(value, where + ".patron",
						  DIVINE_REFUSAL_CONTENT_MAX_PATRON);
		if (const cJSON *value = field(item, "message"))
		{
			entry.message = text_value(value, where + ".message",
						   DIVINE_REFUSAL_CONTENT_MAX_MESSAGE);
			valid_message_template(entry.message, where + ".message");
		}
		if (const cJSON *value = field(item, "enabled"))
		{
			if (!cJSON_IsBool(value))
				invalid(where + ".enabled", "expected a boolean");
			entry.has_enabled = true;
			entry.enabled = cJSON_IsTrue(value);
		}
		if (const cJSON *value = field(item, "percent"))
		{
			if (!cJSON_IsNumber(value) || !std::isfinite(value->valuedouble) ||
			    value->valuedouble < 0.0 || value->valuedouble > 100.0)
				invalid(where + ".percent",
					"expected a finite percentage from 0 to 100");
			entry.has_percent = true;
			entry.percent = static_cast<float>(value->valuedouble);
		}

		if (!result->entries_.emplace(vnum, std::move(entry)).second)
			invalid(where, "duplicate template vnum");
	}
	return result;
}

const DivineRefusalContentEntry *DivineRefusalContentSnapshot::find(int vnum) const
{
	const auto found = entries_.find(vnum);
	return found == entries_.end() ? nullptr : &found->second;
}

std::shared_ptr<const DivineRefusalContentSnapshot> DivineRefusalContentRegistry::snapshot() const
{
	return std::atomic_load_explicit(&current_, std::memory_order_acquire);
}

DivineRefusalContentRegistry &divine_refusal_content_registry()
{
	static DivineRefusalContentRegistry registry;
	return registry;
}

DivineRefusalContentLoadResult DivineRefusalContentRegistry::reload_json(std::string_view json)
{
	try
	{
		auto next = divine_refusal_content_parse(json);
		const uint32_t revision = next->revision();
		std::shared_ptr<const DivineRefusalContentSnapshot> published = std::move(next);
		std::atomic_store_explicit(&current_, std::move(published),
					   std::memory_order_release);
		return { true, revision, {} };
	}
	catch (const std::invalid_argument &error)
	{
		auto previous = snapshot();
		return { false, previous ? previous->revision() : 0, error.what() };
	}
	catch (const std::bad_alloc &)
	{
		auto previous = snapshot();
		return { false, previous ? previous->revision() : 0,
			 "configuration allocation failed" };
	}
}

DivineRefusalContentLoadResult DivineRefusalContentRegistry::reload_file(const std::string &path)
{
	try
	{
		std::error_code error;
		if (!std::filesystem::is_regular_file(path, error) || error)
			invalid("file", "expected a readable regular configuration file");
		std::ifstream input(path, std::ios::binary);
		if (!input)
			invalid("file", "cannot open configuration");
		std::string json(DIVINE_REFUSAL_CONTENT_MAX_BYTES + 1, '\0');
		input.read(json.data(), static_cast<std::streamsize>(json.size()));
		if (input.bad())
			invalid("file", "configuration read failed");
		json.resize(static_cast<size_t>(input.gcount()));
		return reload_json(json);
	}
	catch (const std::invalid_argument &error)
	{
		auto previous = snapshot();
		return { false, previous ? previous->revision() : 0, error.what() };
	}
	catch (const std::bad_alloc &)
	{
		auto previous = snapshot();
		return { false, previous ? previous->revision() : 0,
			 "configuration allocation failed" };
	}
}

bool divine_refusal_content_render(const DivineRefusalContentEntry *entry, char *out,
				   size_t out_len)
{
	if (!out || out_len == 0)
		return false;

	std::string rendered = DIVINE_REFUSAL_GENERIC_LINE;
	bool authored = false;
	if (entry && !entry->patron.empty() && !entry->message.empty())
	{
		const std::string patron = canonical_patron(entry->patron);
		if (!patron.empty())
		{
			constexpr std::string_view token = "{patron}";
			const size_t position = entry->message.find(token);
			rendered = entry->message;
			if (position != std::string::npos)
				rendered.replace(position, token.size(), patron);
			authored = true;
		}
	}

	if (rendered.size() >= out_len)
	{
		const std::string fallback = DIVINE_REFUSAL_GENERIC_LINE;
		if (fallback.size() < out_len)
			std::copy(fallback.c_str(), fallback.c_str() + fallback.size() + 1, out);
		else
			out[0] = '\0';
		return false;
	}
	std::copy(rendered.c_str(), rendered.c_str() + rendered.size() + 1, out);
	return authored;
}
