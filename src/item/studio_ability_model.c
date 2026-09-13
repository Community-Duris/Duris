#include "item/studio_ability_model.h"
#include <cjson/cJSON.h>
#include <cmath>
#include <memory>
#include <set>
#include <stdexcept>
#include <string_view>

namespace
{
struct invalid_catalog : std::runtime_error
{
	using std::runtime_error::runtime_error;
};
void fields(const cJSON *object, std::initializer_list<std::string_view> expected,
	    const std::string &path)
{
	if (!cJSON_IsObject(object))
		throw invalid_catalog(path + ": expected object");
	std::set<std::string_view> names;
	for (const cJSON *field = object->child; field; field = field->next)
	{
		const std::string_view key = field->string ? field->string : "";
		bool known = false;
		for (auto allowed : expected)
			known = known || key == allowed;
		if (!known || !names.insert(key).second)
			throw invalid_catalog(path + "." + std::string(key) +
					      ": unknown or duplicate field");
	}
	for (auto name : expected)
		if (!names.contains(name))
			throw invalid_catalog(path + "." + std::string(name) + ": missing field");
}
uint64_t integer(const cJSON *object, const char *key, uint64_t low, uint64_t high,
		 const std::string &path)
{
	const auto *value = cJSON_GetObjectItemCaseSensitive(object, key);
	if (!cJSON_IsNumber(value) || !std::isfinite(value->valuedouble) ||
	    value->valuedouble < static_cast<double>(low) ||
	    value->valuedouble > static_cast<double>(high) ||
	    std::floor(value->valuedouble) != value->valuedouble)
		throw invalid_catalog(path + "." + key + ": integer outside supported range");
	return static_cast<uint64_t>(value->valuedouble);
}
std::string string(const cJSON *object, const char *key, const std::string &path,
		   size_t limit = 240)
{
	const auto *value = cJSON_GetObjectItemCaseSensitive(object, key);
	if (!cJSON_IsString(value) || !value->valuestring)
		throw invalid_catalog(path + "." + key + ": expected string");
	std::string result = value->valuestring;
	if (result.empty() || result.size() > limit)
		throw invalid_catalog(path + "." + key + ": empty or too long");
	for (unsigned char c : result)
		if (c < 32 || c == 127 || c == '$')
			throw invalid_catalog(
				path + "." + key +
				": control characters and substitution tokens are unsupported");
	return result;
}
studio_ability_definition parse(const cJSON *entry, size_t index)
{
	const std::string path = "abilities[" + std::to_string(index) + "]";
	fields(entry,
	       { "id", "revision", "vnum", "trigger", "mode", "source", "minimumLevel",
		 "windupPulses", "progressPulses", "cooldownMs", "concurrency", "cost", "mana",
		 "effects", "presentation", "ownership" },
	       path);
	studio_ability_definition result;
	result.id = integer(entry, "id", 1, 0x0fffffff, path);
	result.revision = integer(entry, "revision", 1, 1000000000, path);
	result.vnum = integer(entry, "vnum", 1, 1000000000, path);
	result.min_level = integer(entry, "minimumLevel", 0, 60, path);
	result.windup = integer(entry, "windupPulses", 4, 600, path);
	result.progress = integer(entry, "progressPulses", 0, result.windup - 1, path);
	result.cooldown_ms = integer(entry, "cooldownMs", 0, 3600000, path);
	result.cost = integer(entry, "cost", 0, ARTIFACT_MANA_MAXIMUM, path);
	const auto trigger = string(entry, "trigger", path);
	if (trigger != "hit" && trigger != "use")
		throw invalid_catalog(path + ".trigger: supported events are hit and use");
	result.trigger = trigger == "hit" ? studio_ability_trigger::hit :
					    studio_ability_trigger::use;
	if (string(entry, "mode", path) != (trigger == "hit" ? "passive" : "active"))
		throw invalid_catalog(path + ".mode: hit requires passive; use requires active");
	const auto source = string(entry, "source", path);
	if (source != "equipped" && source != "carried")
		throw invalid_catalog(path + ".source: expected equipped or carried");
	result.carried = source == "carried";
	if (result.carried && trigger == "hit")
		throw invalid_catalog(path + ".source: hit requires equipped source");
	if (string(entry, "concurrency", path) != "shared")
		throw invalid_catalog(
			path + ".concurrency: only shared source/actor/global caps are supported");
	if (string(entry, "ownership", path) != "studio-action")
		throw invalid_catalog(
			path +
			".ownership: native callback replacement requires a reviewed native adapter");
	const auto *mana = cJSON_GetObjectItemCaseSensitive(entry, "mana");
	if (!cJSON_IsNull(mana))
	{
		fields(mana, { "id", "revision", "capacity", "regeneration", "passiveFloor" },
		       path + ".mana");
		result.mana.id = integer(mana, "id", 1, 1000000000, path + ".mana");
		result.mana.revision = integer(mana, "revision", 1, 1000000000, path + ".mana");
		result.mana.capacity =
			integer(mana, "capacity", 1, ARTIFACT_MANA_MAXIMUM, path + ".mana");
		result.mana.regeneration =
			integer(mana, "regeneration", 0, ARTIFACT_MANA_MAX_RATE, path + ".mana");
		result.mana.passive_floor =
			integer(mana, "passiveFloor", 0, result.mana.capacity, path + ".mana");
		if (result.cost > result.mana.capacity)
			throw invalid_catalog(path + ".cost: exceeds mana capacity");
	}
	else if (result.cost)
		throw invalid_catalog(path + ".cost: positive cost requires named mana profile");
	const auto *effects = cJSON_GetObjectItemCaseSensitive(entry, "effects");
	if (!cJSON_IsArray(effects) || cJSON_GetArraySize(effects) < 1 ||
	    cJSON_GetArraySize(effects) > 3)
		throw invalid_catalog(path + ".effects: expected one to three ordered effects");
	for (const cJSON *effect = effects->child; effect; effect = effect->next)
	{
		const auto ep = path + ".effects[" + std::to_string(result.effect_count) + "]";
		fields(effect, { "type", "spell", "power", "target", "call" }, ep);
		if (string(effect, "type", ep) != "spell")
			throw invalid_catalog(
				ep + ".type: unsupported effect; use a reviewed native adapter");
		auto &out = result.effects[result.effect_count++];
		out.spell = integer(effect, "spell", 1, 1999,
				    ep); // Runtime checks MAX_SKILLS and the assigned pointer.
		out.power = integer(effect, "power", 0, 60, ep);
		const auto target = string(effect, "target", ep);
		if (target == "activator")
			out.target = studio_ability_target::activator;
		else if (target == "holder")
			out.target = studio_ability_target::holder;
		else if (target == "victim")
			out.target = studio_ability_target::victim;
		else if (target == "item")
			out.target = studio_ability_target::item;
		else if (target == "room")
			out.target = studio_ability_target::room;
		else
			throw invalid_catalog(
				ep +
				".target: expected activator, holder, victim, item or room; world targets unsupported");
		const auto call = string(effect, "call", ep);
		if (call != "spell" && call != "wand")
			throw invalid_catalog(ep + ".call: expected spell or wand");
		out.call = call == "spell" ? studio_ability_call::spell : studio_ability_call::wand;
	}
	const auto *presentation = cJSON_GetObjectItemCaseSensitive(entry, "presentation");
	fields(presentation, { "begin", "progress", "complete", "cancel" }, path + ".presentation");
	result.begin = string(presentation, "begin", path + ".presentation");
	result.beat = string(presentation, "progress", path + ".presentation");
	result.complete = string(presentation, "complete", path + ".presentation");
	result.cancel = string(presentation, "cancel", path + ".presentation");
	return result;
}
}

bool parse_studio_ability_catalog(const std::string &text, studio_ability_catalog &output,
				  std::string &error)
{
	try
	{
		if (text.size() > 1024 * 1024 || text.find('\0') != std::string::npos ||
		    text.find("\\u0000") != std::string::npos)
			throw invalid_catalog("catalog: exceeds one MiB or contains NUL");
		const char *end = nullptr;
		std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(
			cJSON_ParseWithLengthOpts(text.c_str(), text.size() + 1, &end, 1),
			cJSON_Delete);
		if (!root)
			throw invalid_catalog("catalog: invalid JSON at byte " +
					      std::to_string(end ? end - text.c_str() : 0));
		fields(root.get(), { "schemaVersion", "abilities" }, "catalog");
		integer(root.get(), "schemaVersion", 1, 1, "catalog");
		const auto *entries = cJSON_GetObjectItemCaseSensitive(root.get(), "abilities");
		if (!cJSON_IsArray(entries) || cJSON_GetArraySize(entries) > 4096)
			throw invalid_catalog(
				"abilities: expected array with at most 4096 entries");
		studio_ability_catalog candidate;
		std::map<int, artifact_mana_profile> profiles;
		size_t index = 0;
		for (const cJSON *entry = entries->child; entry; entry = entry->next)
		{
			auto definition = parse(entry, index++);
			if (definition.mana.id)
			{
				for (const auto &[vnum, profile] : profiles)
					if ((vnum == definition.vnum ||
					     profile.id == definition.mana.id) &&
					    profile != definition.mana)
						throw invalid_catalog(
							"ability " + std::to_string(definition.id) +
							": conflicting named mana profile");
				profiles[definition.vnum] = definition.mana;
			}
			if (!candidate.emplace(definition.id, std::move(definition)).second)
				throw invalid_catalog("abilities: duplicate ability id");
		}
		output = std::move(candidate);
		error.clear();
		return true;
	}
	catch (const std::exception &failure)
	{
		error = failure.what();
		return false;
	}
}
