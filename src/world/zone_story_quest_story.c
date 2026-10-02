#include "world/zone_story_quest_story.h"

#include "core/defines.h"
#include <cjson/cJSON.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>

namespace zone_story_quest_story
{
namespace
{
constexpr size_t max_file_bytes = 256 * 1024;

void fields(const cJSON *object, std::initializer_list<std::string_view> expected,
	    const std::string &path)
{
	if (!cJSON_IsObject(object))
		throw std::runtime_error(path + ": expected object");
	std::set<std::string_view> found;
	for (const auto *field = object->child; field; field = field->next)
	{
		const std::string_view name = field->string ? field->string : "";
		bool known = false;
		for (const auto key : expected)
			known = known || key == name;
		if (!known || !found.insert(name).second)
			throw std::runtime_error(path + ": unknown or duplicate field " +
						 std::string(name));
	}
	for (const auto key : expected)
		if (!found.contains(key))
			throw std::runtime_error(path + ": missing field " + std::string(key));
}

std::string text(const cJSON *object, const char *key, const std::string &path,
		 bool allow_empty = false)
{
	const auto *value = cJSON_GetObjectItemCaseSensitive(object, key);
	if (!cJSON_IsString(value) || !value->valuestring)
		throw std::runtime_error(path + ": expected string " + key);
	std::string result = value->valuestring;
	if ((!allow_empty && result.empty()) || result.size() > 1024)
		throw std::runtime_error(path + ": empty or oversized string " + key);
	for (const unsigned char c : result)
		if (c < 32 || c == 127 || c == '$')
			throw std::runtime_error(path + ": control/substitution character in " +
						 key);
	return result;
}

int32_t integer(const cJSON *value, int32_t low, int32_t high, const std::string &path)
{
	if (!cJSON_IsNumber(value) || !std::isfinite(value->valuedouble) ||
	    value->valuedouble < low || value->valuedouble > high ||
	    std::floor(value->valuedouble) != value->valuedouble)
		throw std::runtime_error(path + ": integer outside supported range");
	return static_cast<int32_t>(value->valuedouble);
}

const cJSON *array(const cJSON *object, const char *key, int low, int high, const std::string &path)
{
	const auto *value = cJSON_GetObjectItemCaseSensitive(object, key);
	if (!cJSON_IsArray(value) || cJSON_GetArraySize(value) < low ||
	    cJSON_GetArraySize(value) > high)
		throw std::runtime_error(path + ": array outside supported bounds " + key);
	return value;
}

bool identifier(std::string_view value)
{
	if (value.empty() || value.size() > 64)
		return false;
	for (const char c : value)
		if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'))
			return false;
	return true;
}

std::string hex(std::string_view value)
{
	constexpr char digits[] = "0123456789abcdef";
	std::string result;
	for (const unsigned char c : value)
	{
		result += digits[c >> 4];
		result += digits[c & 15];
	}
	return result;
}

std::vector<std::string> references(const cJSON *object, const char *key,
				    const zone_story_quest_catalog::catalog &catalog,
				    const std::string &path, int32_t owning_zone = -1)
{
	std::vector<std::string> result;
	std::set<std::string> seen;
	for (auto *entry = array(object, key, 1, 4096, path)->child; entry; entry = entry->next)
	{
		fields(entry, { "giver_vnum", "completion_key" }, path + ".contracts");
		const int32_t giver = integer(cJSON_GetObjectItemCaseSensitive(entry, "giver_vnum"),
					      1, INT32_MAX, path + ".giver_vnum");
		const std::string contract = hex(text(entry, "completion_key", path));
		const zone_story_quest_tracking::quest_definition *matched = nullptr;
		for (const auto &definition : catalog.definitions)
			if (definition.giver_vnum == giver &&
			    definition.completion_key == contract && definition.active)
				matched = &definition;
		if (!matched || (owning_zone >= 0 && matched->zone_number != owning_zone))
			throw std::runtime_error(path + ": unknown or wrong-area native contract");
		if (!seen.insert(matched->definition_id).second)
			throw std::runtime_error(path + ": duplicate native contract");
		result.push_back(matched->definition_id);
	}
	return result;
}
} // namespace

bool apply(const std::string &json, std::string_view source_area,
	   zone_story_quest_catalog::catalog *catalog, std::string *error)
{
	try
	{
		if (!catalog || json.size() > max_file_bytes ||
		    json.find('\0') != std::string::npos ||
		    json.find("\\u0000") != std::string::npos)
			throw std::runtime_error(
				"story mapping: oversized, NUL, or missing catalog");
		std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(
			cJSON_ParseWithLengthOpts(json.c_str(), json.size() + 1, nullptr, 1),
			cJSON_Delete);
		fields(root.get(),
		       { "schema_version", "revision", "source_area", "coverage", "stories",
			 "exclusions" },
		       "story mapping");
		integer(cJSON_GetObjectItemCaseSensitive(root.get(), "schema_version"), 1, 1,
			"schema_version");
		zone_story_quest_catalog::story_mapping mapping;
		mapping.source_area = text(root.get(), "source_area", "story mapping");
		if (mapping.source_area != source_area || !identifier(source_area))
			throw std::runtime_error(
				"story mapping: source_area must match its filename");
		mapping.revision = integer(cJSON_GetObjectItemCaseSensitive(root.get(), "revision"),
					   1, INT32_MAX, "revision");
		const std::string coverage = text(root.get(), "coverage", "story mapping");
		if (coverage != "partial" && coverage != "complete")
			throw std::runtime_error("coverage: expected partial or complete");
		mapping.complete = coverage == "complete";
		int32_t zone = -1;
		for (const auto &registry : catalog->zones)
			if (registry.source_area == source_area && registry.discoverable)
				zone = registry.zone_number;
		if (zone <= 0)
			throw std::runtime_error(
				"story mapping: area is not a playable catalog area");
		std::set<std::string> story_ids;
		for (auto *entry = array(root.get(), "stories", 0, 256, "stories")->child; entry;
		     entry = entry->next)
		{
			fields(entry,
			       { "id", "title", "category", "summary", "contracts", "steps" },
			       "story");
			zone_story_quest_catalog::story_definition story;
			const std::string id = text(entry, "id", "story");
			if (!identifier(id) || !story_ids.insert(id).second)
				throw std::runtime_error("story: invalid or duplicate id");
			story.id = "zone-story:story:" + mapping.source_area + ":" + id;
			story.zone_number = zone;
			story.title = text(entry, "title", id);
			story.category = text(entry, "category", id);
			if (story.category != "story" && story.category != "request" &&
			    story.category != "service")
				throw std::runtime_error(
					id + ": category must be story, request or service");
			story.summary = text(entry, "summary", id);
			story.contracts = references(entry, "contracts", *catalog, id, zone);
			std::set<std::string> step_ids;
			for (auto *step = array(entry, "steps", 1, 32, id)->child; step;
			     step = step->next)
			{
				zone_story_quest_catalog::story_step parsed;
				parsed.kind = text(step, "kind", id);
				if (parsed.kind == "completion")
					fields(step, { "id", "text", "kind", "hint", "contracts" },
					       id);
				else if (parsed.kind == "carried_item")
					fields(step,
					       { "id", "text", "kind", "hint", "item_vnums",
						 "count" },
					       id);
				else if (parsed.kind == "equipped_item")
					fields(step,
					       { "id", "text", "kind", "hint", "item_vnums",
						 "count", "slot" },
					       id);
				else
					throw std::runtime_error(id + ": unsupported step kind " +
								 parsed.kind);
				parsed.id = text(step, "id", id);
				if (!identifier(parsed.id) || !step_ids.insert(parsed.id).second)
					throw std::runtime_error(id +
								 ": invalid or duplicate step id");
				parsed.text = text(step, "text", id);
				parsed.hint = text(step, "hint", id, true);
				if (parsed.kind == "completion")
					parsed.contracts =
						references(step, "contracts", *catalog, id);
				else
				{
					parsed.count = integer(
						cJSON_GetObjectItemCaseSensitive(step, "count"), 1,
						3000, id + ".count");
					if (parsed.kind == "equipped_item")
						parsed.slot =
							integer(cJSON_GetObjectItemCaseSensitive(
									step, "slot"),
								-1, MAX_WEAR - 1, id + ".slot");
					std::set<int32_t> items;
					for (auto *item =
						     array(step, "item_vnums", 1, 64, id)->child;
					     item; item = item->next)
					{
						const int32_t vnum = integer(item, 1, INT32_MAX,
									     id + ".item_vnums");
						if (!items.insert(vnum).second)
							throw std::runtime_error(
								id +
								": duplicate item alternative");
						parsed.item_vnums.push_back(vnum);
					}
				}
				story.steps.push_back(std::move(parsed));
			}
			mapping.stories.push_back(std::move(story));
		}
		for (auto *entry = array(root.get(), "exclusions", 0, 256, "exclusions")->child;
		     entry; entry = entry->next)
		{
			fields(entry, { "reason", "contracts" }, "exclusion");
			const std::string reason = text(entry, "reason", "exclusion");
			for (const auto &id :
			     references(entry, "contracts", *catalog, "exclusion", zone))
				if (!mapping.exclusions.emplace(id, reason).second)
					throw std::runtime_error("exclusion: duplicate contract");
		}
		auto candidate = *catalog;
		candidate.story_mappings.push_back(std::move(mapping));
		std::vector<zone_story_quest_catalog::diagnostic> diagnostics;
		if (!zone_story_quest_catalog::validate(candidate, &diagnostics))
			throw std::runtime_error(diagnostics.front().message);
		*catalog = std::move(candidate);
		if (error)
			error->clear();
		return true;
	}
	catch (const std::exception &failure)
	{
		if (error)
			*error = failure.what();
		return false;
	}
}

bool load(zone_story_quest_catalog::catalog *catalog, std::string *error,
	  const std::string &directory)
{
	if (!catalog)
		return false;
	auto candidate = *catalog;
	std::set<std::string> areas;
	for (const auto &zone : catalog->zones)
	{
		if (!identifier(zone.source_area) || !areas.insert(zone.source_area).second)
			continue;
		const auto path =
			std::filesystem::path(directory) / (zone.source_area + ".story.json");
		std::error_code ec;
		if (!std::filesystem::exists(path, ec) && !ec)
			continue;
		const auto size = std::filesystem::file_size(path, ec);
		std::ifstream file(path, std::ios::binary);
		if (ec || size > max_file_bytes || !file)
		{
			if (error)
				*error = path.string() + ": unreadable or oversized story mapping";
			return false;
		}
		std::string json(static_cast<size_t>(size), '\0');
		file.read(json.data(), static_cast<std::streamsize>(json.size()));
		std::string detail;
		if (!file || file.peek() != std::char_traits<char>::eof() ||
		    !apply(json, zone.source_area, &candidate, &detail))
		{
			if (error)
				*error = path.string() + ": " + detail;
			return false;
		}
	}
	*catalog = std::move(candidate);
	if (error)
		error->clear();
	return true;
}
} // namespace zone_story_quest_story
