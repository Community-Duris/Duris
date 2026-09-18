#include "artifact/artifact_control.h"

#include <cjson/cJSON.h>

#include <algorithm>
#include <cmath>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <strings.h>
#include <utility>

namespace artifact_control
{
namespace
{
constexpr const char *DEFAULT_PATH = "lib/artifacts/catalog.json";

std::string text_or(cJSON *parent, const char *key, const char *fallback)
{
	const cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, key);
	return cJSON_IsString(item) && item->valuestring ? item->valuestring : fallback;
}

int number_or(cJSON *parent, const char *key, int fallback)
{
	const cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, key);
	if (!cJSON_IsNumber(item) || !std::isfinite(item->valuedouble) ||
	    item->valuedouble < std::numeric_limits<int>::min() ||
	    item->valuedouble > std::numeric_limits<int>::max())
		return fallback;
	return static_cast<int>(item->valuedouble);
}

bool bool_or(cJSON *parent, const char *key, bool fallback)
{
	const cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, key);
	return cJSON_IsBool(item) ? cJSON_IsTrue(item) : fallback;
}

void add_string(cJSON *parent, const char *key, const std::string &value)
{
	cJSON_AddStringToObject(parent, key, value.c_str());
}

void add_number(cJSON *parent, const char *key, int value)
{
	cJSON_AddNumberToObject(parent, key, value);
}

void add_variant(cJSON *parent, const char *key, const std::string &value)
{
	if (!value.empty())
		add_string(parent, key, value);
}

bool valid_id(const std::string &value)
{
	if (value.empty() || value.size() > 63 || !(value[0] >= 'a' && value[0] <= 'z'))
		return false;
	for (const char ch : value)
		if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' ||
		      ch == '-' || ch == '.'))
			return false;
	return true;
}

void issue(std::vector<diagnostic> *diagnostics, const char *code, const std::string &path,
	   const std::string &message)
{
	if (diagnostics)
		diagnostics->push_back({ code, path, message });
}

bool parse_definition(cJSON *node, definition *out, std::vector<diagnostic> *diagnostics,
		      size_t index)
{
	if (!cJSON_IsObject(node) || !out)
	{
		issue(diagnostics, "INVALID_DEFINITION", "/definitions",
		      "definition must be an object");
		return false;
	}
	definition next;
	next.id = text_or(node, "id", "");
	next.vnum = number_or(node, "vnum", 0);
	next.display_name = text_or(node, "displayName", next.id.c_str());
	next.classification = text_or(node, "classification", "");
	next.uniqueness_family = text_or(node, "uniquenessFamily", next.id.c_str());
	next.default_variant = text_or(node, "defaultVariant", "legacy");
	next.legacy_binding = text_or(node, "legacyBinding", "");
	next.source = text_or(node, "source", "");
	next.source_line = number_or(node, "sourceLine", 0);
	next.initial_lifetime_seconds = number_or(node, "initialLifetimeSeconds", 864000);
	next.max_lifetime_seconds = number_or(node, "maxRemainingLifetimeSeconds", 864000);
	next.load_chance = number_or(node, "loadChance", 100);
	next.load_limit = number_or(node, "loadLimit", 1);
	next.load_room = number_or(node, "loadRoom", 0);
	next.load_mob = number_or(node, "loadMob", 0);
	next.load_slot = number_or(node, "loadSlot", -1);
	next.enabled = bool_or(node, "enabled", true);

	const cJSON *holders = cJSON_GetObjectItemCaseSensitive(node, "holderPolicy");
	if (cJSON_IsObject(holders))
	{
		next.player_variant = text_or(const_cast<cJSON *>(holders), "player", "");
		next.wild_npc_variant = text_or(const_cast<cJSON *>(holders), "wildNpc", "");
		next.controlled_npc_variant =
			text_or(const_cast<cJSON *>(holders), "controlledNpc", "");
	}
	if (next.player_variant.empty())
		next.player_variant = next.default_variant;
	if (next.wild_npc_variant.empty())
		next.wild_npc_variant = next.default_variant;
	if (next.controlled_npc_variant.empty())
		next.controlled_npc_variant = next.player_variant;

	const cJSON *variants = cJSON_GetObjectItemCaseSensitive(node, "variants");
	if (cJSON_IsObject(variants))
	{
		cJSON *variant_node = nullptr;
		cJSON_ArrayForEach(variant_node, variants)
		{
			if (!cJSON_IsObject(variant_node) || !variant_node->string)
				continue;
			next.variants.push_back(
				{ variant_node->string, text_or(variant_node, "adapter", "") });
		}
	}

	const cJSON *powers = cJSON_GetObjectItemCaseSensitive(node, "powers");
	if (cJSON_IsArray(powers))
	{
		cJSON *power_node = nullptr;
		cJSON_ArrayForEach(power_node, powers)
		{
			if (!cJSON_IsObject(power_node))
				continue;
			power next_power;
			next_power.id = text_or(power_node, "id", "");
			next_power.enabled = bool_or(power_node, "enabled", true);
			next_power.chance_numerator = number_or(power_node, "chanceNumerator", 1);
			next_power.chance_denominator =
				number_or(power_node, "chanceDenominator", 1);
			next_power.cooldown_seconds = number_or(power_node, "cooldownSeconds", 0);
			next_power.windup_pulses = number_or(power_node, "windupPulses", 0);
			next_power.mana_cost_milliunits =
				number_or(power_node, "manaCostMilliunits", 0);
			next_power.power_level = number_or(power_node, "powerLevel", 0);
			next.powers.push_back(next_power);
		}
	}

	if (next.id.empty())
		issue(diagnostics, "MISSING_ID", "/definitions/" + std::to_string(index) + "/id",
		      "definition id is required");
	if (next.vnum <= 0)
		issue(diagnostics, "INVALID_VNUM",
		      "/definitions/" + std::to_string(index) + "/vnum", "vnum must be positive");
	if (!valid_id(next.id) || !valid_id(next.uniqueness_family))
		issue(diagnostics, "INVALID_ID", "/definitions/" + std::to_string(index),
		      "ids must use lowercase letters, digits, dot, dash, or underscore");
	if (next.classification != "major" && next.classification != "unique" &&
	    next.classification != "ioun")
		issue(diagnostics, "INVALID_CLASSIFICATION",
		      "/definitions/" + std::to_string(index) + "/classification",
		      "classification must be major, unique, or ioun");
	if (next.load_chance < 0 || next.load_chance > 100 || next.load_limit < 1)
		issue(diagnostics, "INVALID_PLACEMENT", "/definitions/" + std::to_string(index),
		      "load chance must be 0..100 and load limit must be positive");
	if (next.initial_lifetime_seconds < 0 || next.max_lifetime_seconds < 0 ||
	    next.initial_lifetime_seconds > 31536000 || next.max_lifetime_seconds > 31536000)
		issue(diagnostics, "INVALID_LIFETIME", "/definitions/" + std::to_string(index),
		      "lifetime must be between 0 and one year");

	*out = std::move(next);
	return true;
}

cJSON *definition_json(const definition &item)
{
	cJSON *node = cJSON_CreateObject();
	add_string(node, "id", item.id);
	add_number(node, "vnum", item.vnum);
	add_string(node, "displayName", item.display_name);
	add_string(node, "classification", item.classification);
	add_string(node, "uniquenessFamily", item.uniqueness_family);
	add_string(node, "defaultVariant", item.default_variant);
	add_string(node, "legacyBinding", item.legacy_binding);
	add_string(node, "source", item.source);
	add_number(node, "sourceLine", item.source_line);
	add_number(node, "initialLifetimeSeconds", item.initial_lifetime_seconds);
	add_number(node, "maxRemainingLifetimeSeconds", item.max_lifetime_seconds);
	add_number(node, "loadChance", item.load_chance);
	add_number(node, "loadLimit", item.load_limit);
	add_number(node, "loadRoom", item.load_room);
	add_number(node, "loadMob", item.load_mob);
	add_number(node, "loadSlot", item.load_slot);
	cJSON_AddBoolToObject(node, "enabled", item.enabled);

	cJSON *holders = cJSON_CreateObject();
	add_variant(holders, "player", item.player_variant);
	add_variant(holders, "wildNpc", item.wild_npc_variant);
	add_variant(holders, "controlledNpc", item.controlled_npc_variant);
	cJSON_AddItemToObject(node, "holderPolicy", holders);

	cJSON *variants = cJSON_CreateObject();
	for (const variant &entry : item.variants)
	{
		cJSON *variant_node = cJSON_CreateObject();
		add_string(variant_node, "adapter", entry.adapter);
		cJSON_AddItemToObject(variants, entry.id.c_str(), variant_node);
	}
	cJSON_AddItemToObject(node, "variants", variants);

	cJSON *powers = cJSON_CreateArray();
	for (const power &entry : item.powers)
	{
		cJSON *power_node = cJSON_CreateObject();
		add_string(power_node, "id", entry.id);
		cJSON_AddBoolToObject(power_node, "enabled", entry.enabled);
		add_number(power_node, "chanceNumerator", entry.chance_numerator);
		add_number(power_node, "chanceDenominator", entry.chance_denominator);
		add_number(power_node, "cooldownSeconds", entry.cooldown_seconds);
		add_number(power_node, "windupPulses", entry.windup_pulses);
		add_number(power_node, "manaCostMilliunits", entry.mana_cost_milliunits);
		add_number(power_node, "powerLevel", entry.power_level);
		cJSON_AddItemToArray(powers, power_node);
	}
	cJSON_AddItemToObject(node, "powers", powers);
	return node;
}

catalog active_catalog;
catalog draft_catalog;
status active_status;

std::string read_file(const char *path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
		return {};
	return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

uint64_t fnv1a(const std::string &value)
{
	uint64_t hash = 1469598103934665603ULL;
	for (const unsigned char byte : value)
	{
		hash ^= byte;
		hash *= 1099511628211ULL;
	}
	return hash;
}

std::string format_hash(uint64_t hash)
{
	std::ostringstream out;
	out << std::hex << std::setfill('0') << std::setw(16) << hash;
	return out.str();
}

std::string catalog_hash_input(const catalog &value)
{
	std::ostringstream out;
	out << value.schema_version << '\x1f' << value.revision << '\x1e';
	std::vector<definition> definitions = value.definitions;
	std::sort(definitions.begin(), definitions.end(),
		  [](const definition &left, const definition &right)
		  { return left.vnum < right.vnum; });
	for (const definition &item : definitions)
	{
		out << item.id << '\x1f' << item.vnum << '\x1f' << item.display_name << '\x1f'
		    << item.classification << '\x1f' << item.uniqueness_family << '\x1f'
		    << item.default_variant << '\x1f' << item.player_variant << '\x1f'
		    << item.wild_npc_variant << '\x1f' << item.controlled_npc_variant << '\x1f'
		    << item.legacy_binding << '\x1f' << item.source << '\x1f' << item.source_line
		    << '\x1f' << item.initial_lifetime_seconds << '\x1f'
		    << item.max_lifetime_seconds << '\x1f' << item.load_chance << '\x1f'
		    << item.load_limit << '\x1f' << item.load_room << '\x1f' << item.load_mob
		    << '\x1f' << item.load_slot << '\x1f' << (item.enabled ? 1 : 0) << '\x1e';
		std::vector<variant> variants = item.variants;
		std::sort(variants.begin(), variants.end(),
			  [](const variant &left, const variant &right)
			  { return left.id < right.id; });
		for (const variant &entry : variants)
			out << entry.id << '\x1f' << entry.adapter << '\x1e';
		out << '\x1d';
		std::vector<power> powers = item.powers;
		std::sort(powers.begin(), powers.end(),
			  [](const power &left, const power &right) { return left.id < right.id; });
		for (const power &entry : powers)
			out << entry.id << '\x1f' << (entry.enabled ? 1 : 0) << '\x1f'
			    << entry.chance_numerator << '\x1f' << entry.chance_denominator
			    << '\x1f' << entry.cooldown_seconds << '\x1f' << entry.windup_pulses
			    << '\x1f' << entry.mana_cost_milliunits << '\x1f' << entry.power_level
			    << '\x1e';
		out << '\x1c';
	}
	return out.str();
}

} // namespace

const char *artifact_control_default_path()
{
	return DEFAULT_PATH;
}

bool catalog_validate(const catalog &value, std::vector<diagnostic> *diagnostics)
{
	const size_t diagnostic_count = diagnostics ? diagnostics->size() : 0;
	if (value.schema_version != 1)
		issue(diagnostics, "UNSUPPORTED_SCHEMA", "/schemaVersion",
		      "only schema version 1 is supported");
	if (value.revision == 0)
		issue(diagnostics, "INVALID_REVISION", "/revision", "revision must be positive");
	std::set<int> vnums;
	std::set<std::string> ids;
	for (size_t index = 0; index < value.definitions.size(); ++index)
	{
		const definition &item = value.definitions[index];
		if (!vnums.insert(item.vnum).second)
			issue(diagnostics, "DUPLICATE_VNUM",
			      "/definitions/" + std::to_string(index) + "/vnum",
			      "vnum is declared more than once");
		if (!ids.insert(item.id).second)
			issue(diagnostics, "DUPLICATE_ID",
			      "/definitions/" + std::to_string(index) + "/id",
			      "definition id is declared more than once");
		std::set<std::string> variant_ids;
		for (const variant &entry : item.variants)
			if (!variant_ids.insert(entry.id).second)
				issue(diagnostics, "DUPLICATE_VARIANT", item.id + "/variants",
				      "variant is duplicated");
		for (const std::string *holder :
		     { &item.default_variant, &item.player_variant, &item.wild_npc_variant,
		       &item.controlled_npc_variant })
			if (!holder->empty() && variant_ids.find(*holder) == variant_ids.end())
				issue(diagnostics, "UNKNOWN_VARIANT", item.id,
				      "holder policy references an unknown variant");
		std::set<std::string> power_ids;
		for (const power &entry : item.powers)
		{
			if (!valid_id(entry.id) || !power_ids.insert(entry.id).second)
				issue(diagnostics, "INVALID_POWER", item.id + "/powers",
				      "power ids must be unique and valid");
			if (entry.chance_numerator < 0 || entry.chance_denominator <= 0 ||
			    entry.chance_numerator > entry.chance_denominator)
				issue(diagnostics, "INVALID_CHANCE",
				      item.id + "/powers/" + entry.id,
				      "chance must be a valid fraction");
			if (entry.cooldown_seconds < 0 || entry.cooldown_seconds > 604800 ||
			    entry.windup_pulses < 0 || entry.windup_pulses > 600 ||
			    entry.mana_cost_milliunits < 0 || entry.power_level < 0 ||
			    entry.power_level > 60)
				issue(diagnostics, "INVALID_POWER_LIMIT",
				      item.id + "/powers/" + entry.id,
				      "power values exceed the supported limits");
		}
	}
	return !diagnostics || diagnostics->size() == diagnostic_count;
}

bool catalog_load_file(const char *path, catalog *out, std::vector<diagnostic> *diagnostics)
{
	if (!path || !out)
		return false;
	const std::string raw = read_file(path);
	if (raw.empty())
	{
		issue(diagnostics, "FILE_NOT_FOUND", path, std::strerror(errno));
		return false;
	}
	cJSON *root = cJSON_ParseWithLength(raw.c_str(), raw.size());
	if (!root)
	{
		issue(diagnostics, "INVALID_JSON", path,
		      cJSON_GetErrorPtr() ? cJSON_GetErrorPtr() : "parse failed");
		return false;
	}
	catalog next;
	next.schema_version = number_or(root, "schemaVersion", 0);
	const std::string declared_hash = text_or(root, "hash", "");
	const cJSON *revision = cJSON_GetObjectItemCaseSensitive(root, "revision");
	if (cJSON_IsNumber(revision) && revision->valuedouble >= 1)
		next.revision = static_cast<uint64_t>(revision->valuedouble);
	const cJSON *definitions = cJSON_GetObjectItemCaseSensitive(root, "definitions");
	if (!cJSON_IsArray(definitions))
		issue(diagnostics, "MISSING_DEFINITIONS", "/definitions",
		      "definitions array is required");
	else
	{
		cJSON *node = nullptr;
		size_t index = 0;
		cJSON_ArrayForEach(node, definitions)
		{
			definition item;
			parse_definition(node, &item, diagnostics, index++);
			next.definitions.push_back(std::move(item));
		}
	}
	cJSON_Delete(root);
	if (!catalog_validate(next, diagnostics))
		return false;
	next.hash = catalog_hash(next);
	if (!declared_hash.empty() && declared_hash != next.hash)
	{
		issue(diagnostics, "HASH_MISMATCH", "/hash",
		      "catalog hash does not match its contents");
		return false;
	}
	*out = std::move(next);
	return true;
}

std::string catalog_to_json(const catalog &value)
{
	cJSON *root = cJSON_CreateObject();
	add_number(root, "schemaVersion", value.schema_version);
	cJSON_AddNumberToObject(root, "revision", static_cast<double>(value.revision));
	/* The hash is deliberately excluded from the canonical input used to
	 * calculate itself.  Callers that need a hash should call catalog_hash();
	 * serializing an unhashed value must never recurse. */
	add_string(root, "hash", value.hash);
	cJSON *definitions = cJSON_CreateArray();
	std::vector<definition> sorted = value.definitions;
	std::sort(sorted.begin(), sorted.end(), [](const definition &left, const definition &right)
		  { return left.vnum < right.vnum; });
	for (const definition &item : sorted)
		cJSON_AddItemToArray(definitions, definition_json(item));
	cJSON_AddItemToObject(root, "definitions", definitions);
	char *rendered = cJSON_Print(root);
	std::string output = rendered ? rendered : "";
	if (rendered)
		free(rendered);
	cJSON_Delete(root);
	return output + (output.empty() || output.back() == '\n' ? "" : "\n");
}

std::string catalog_hash(const catalog &value)
{
	return format_hash(fnv1a(catalog_hash_input(value)));
}

bool catalog_save_file(const char *path, const catalog &value, std::vector<diagnostic> *diagnostics)
{
	if (!path)
		return false;
	std::vector<diagnostic> validation;
	if (!catalog_validate(value, &validation))
	{
		if (diagnostics)
			diagnostics->insert(diagnostics->end(), validation.begin(),
					    validation.end());
		return false;
	}
	if (!value.hash.empty() && value.hash != catalog_hash(value))
	{
		issue(diagnostics, "HASH_MISMATCH", "/hash",
		      "catalog hash does not match its contents");
		return false;
	}
	std::string temporary = std::string(path) + ".new";
	std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
	if (!output)
	{
		issue(diagnostics, "FILE_WRITE", path, "could not open temporary catalog");
		return false;
	}
	output << catalog_to_json(value);
	output.close();
	if (!output)
	{
		issue(diagnostics, "FILE_WRITE", path, "could not write catalog");
		std::remove(temporary.c_str());
		return false;
	}
	if (std::rename(temporary.c_str(), path) != 0)
	{
		issue(diagnostics, "FILE_RENAME", path, std::strerror(errno));
		std::remove(temporary.c_str());
		return false;
	}
	return true;
}

const definition *catalog_find(const catalog &value, int vnum)
{
	for (const definition &item : value.definitions)
		if (item.vnum == vnum)
			return &item;
	return nullptr;
}

definition *catalog_find(catalog &value, int vnum)
{
	for (definition &item : value.definitions)
		if (item.vnum == vnum)
			return &item;
	return nullptr;
}

const char *holder_name(holder_kind holder)
{
	switch (holder)
	{
	case holder_kind::player:
		return "player";
	case holder_kind::wild_npc:
		return "wild NPC";
	case holder_kind::controlled_npc:
		return "controlled NPC";
	}
	return "unknown";
}

bool parse_holder(const char *text, holder_kind *holder)
{
	if (!text || !holder)
		return false;
	if (!strcasecmp(text, "player") || !strcasecmp(text, "pc"))
		*holder = holder_kind::player;
	else if (!strcasecmp(text, "npc") || !strcasecmp(text, "wildnpc") ||
		 !strcasecmp(text, "wild-npc"))
		*holder = holder_kind::wild_npc;
	else if (!strcasecmp(text, "controlled") || !strcasecmp(text, "controllednpc") ||
		 !strcasecmp(text, "controlled-npc"))
		*holder = holder_kind::controlled_npc;
	else
		return false;
	return true;
}

const std::string &variant_for(const definition &item, holder_kind holder)
{
	switch (holder)
	{
	case holder_kind::player:
		return item.player_variant;
	case holder_kind::wild_npc:
		return item.wild_npc_variant;
	case holder_kind::controlled_npc:
		return item.controlled_npc_variant;
	}
	return item.default_variant;
}

bool control_initialize(const char *path, std::string *error)
{
	active_status = {};
	active_status.source_path = path && *path ? path : DEFAULT_PATH;
	std::vector<diagnostic> diagnostics;
	if (!catalog_load_file(active_status.source_path.c_str(), &active_catalog, &diagnostics))
	{
		active_status.last_error = diagnostics.empty() ? "catalog load failed" :
								 diagnostics.front().message;
		if (error)
			*error = active_status.last_error;
		return false;
	}
	active_status.initialized = true;
	active_status.active_revision = active_catalog.revision;
	active_status.active_hash = active_catalog.hash;
	draft_catalog = active_catalog;
	return true;
}

bool control_reload(std::string *error)
{
	return control_initialize(active_status.source_path.empty() ?
					  DEFAULT_PATH :
					  active_status.source_path.c_str(),
				  error);
}

const catalog &control_catalog()
{
	return active_catalog;
}
status control_status()
{
	return active_status;
}

bool control_enabled(int vnum)
{
	if (!active_status.initialized)
		return true; // Preserve the legacy route while boot is still loading.
	const definition *item = catalog_find(active_catalog, vnum);
	return !item || item->enabled;
}

bool control_allows_variant(int vnum, holder_kind holder, const char *variant_id)
{
	if (!variant_id || !*variant_id || !active_status.initialized)
		return true;
	const definition *item = catalog_find(active_catalog, vnum);
	if (!item)
		return true; // Legacy-only artifacts have no catalog policy yet.
	if (!item->enabled)
		return false;
	return variant_for(*item, holder) == variant_id;
}

bool control_power_enabled(int vnum, const char *power_id)
{
	if (!power_id || !*power_id || !active_status.initialized)
		return true;
	const definition *item = catalog_find(active_catalog, vnum);
	if (!item)
		return true;
	for (const power &entry : item->powers)
		if (entry.id == power_id)
			return item->enabled && entry.enabled;
	return true; // A legacy-only/unimported power remains native-owned.
}

int control_power_level(int vnum, const char *power_id, int fallback)
{
	if (!power_id || !*power_id || !active_status.initialized)
		return fallback;
	const definition *item = catalog_find(active_catalog, vnum);
	if (!item)
		return fallback;
	for (const power &entry : item->powers)
		if (entry.id == power_id && entry.power_level > 0)
			return entry.power_level;
	return fallback;
}

std::string control_inspect(int vnum)
{
	const catalog &display_catalog = active_status.dirty ? draft_catalog : active_catalog;
	const definition *item = catalog_find(display_catalog, vnum);
	if (!item)
		return "UNKNOWN_ARTIFACT";
	std::ostringstream out;
	out << "Artifact " << item->display_name << " (#" << item->vnum << ")\n"
	    << "Category: " << item->classification << "  Family: " << item->uniqueness_family
	    << "\n"
	    << "Enabled: " << (item->enabled ? "yes" : "no")
	    << "  Default: " << item->default_variant << "\n"
	    << "Player: " << item->player_variant << "  Wild NPC: " << item->wild_npc_variant
	    << "  Controlled NPC: " << item->controlled_npc_variant << "\n"
	    << "Load: chance " << item->load_chance << "% limit " << item->load_limit << " room "
	    << item->load_room << " mob " << item->load_mob << " slot " << item->load_slot << "\n"
	    << "Lifetime: " << item->initial_lifetime_seconds << "s / max "
	    << item->max_lifetime_seconds << "s\n"
	    << "Legacy binding: " << (item->legacy_binding.empty() ? "none" : item->legacy_binding)
	    << "\n"
	    << "Source: " << (item->source.empty() ? "unknown" : item->source) << ":"
	    << item->source_line << "\n"
	    << "Variants:";
	for (const variant &entry : item->variants)
		out << " " << entry.id << "(" << entry.adapter << ")";
	if (item->powers.empty())
		out << "\nPowers: legacy adapter controls this item";
	else
	{
		out << "\nPowers:";
		for (const power &entry : item->powers)
			out << " " << entry.id << "=" << (entry.enabled ? "on" : "off") << ","
			    << entry.chance_numerator << "/" << entry.chance_denominator
			    << ",cd=" << entry.cooldown_seconds << "s"
			    << ",windup=" << entry.windup_pulses
			    << ",cost=" << entry.mana_cost_milliunits
			    << ",level=" << entry.power_level;
	}
	return out.str();
}

std::string control_preview(int vnum)
{
	if (!active_status.dirty)
		return "No unpublished artifact changes are pending.";
	const definition *draft = catalog_find(draft_catalog, vnum);
	const definition *active = catalog_find(active_catalog, vnum);
	if (!draft || !active)
		return "UNKNOWN_ARTIFACT";
	std::ostringstream out;
	out << control_inspect(vnum) << "\nDraft changes:";
	if (draft->enabled != active->enabled)
		out << " enabled=" << (draft->enabled ? "on" : "off");
	if (draft->player_variant != active->player_variant)
		out << " player=" << draft->player_variant;
	if (draft->wild_npc_variant != active->wild_npc_variant)
		out << " wildNpc=" << draft->wild_npc_variant;
	if (draft->controlled_npc_variant != active->controlled_npc_variant)
		out << " controlledNpc=" << draft->controlled_npc_variant;
	return out.str();
}

std::string control_list(const char *filter)
{
	std::ostringstream out;
	const catalog &display_catalog = active_status.dirty ? draft_catalog : active_catalog;
	for (const definition &item : display_catalog.definitions)
	{
		if (filter && *filter && item.id.find(filter) == std::string::npos &&
		    item.display_name.find(filter) == std::string::npos &&
		    item.classification.find(filter) == std::string::npos &&
		    std::to_string(item.vnum) != filter)
			continue;
		out << item.vnum << " " << item.display_name << " [" << item.classification << "]"
		    << " player=" << item.player_variant << " npc=" << item.wild_npc_variant
		    << "\n";
	}
	return out.str();
}

control_result control_set_variant(int vnum, holder_kind holder, const char *variant_id,
				   std::string *error)
{
	if (!variant_id || !*variant_id)
	{
		if (error)
			*error = "variant is required";
		return control_result::invalid;
	}
	if (!active_status.initialized)
	{
		if (error)
			*error = "catalog is not initialized";
		return control_result::unavailable;
	}
	if (!active_status.dirty)
		draft_catalog = active_catalog;
	definition *item = catalog_find(draft_catalog, vnum);
	if (!item)
	{
		if (error)
			*error = "artifact vnum not found";
		return control_result::not_found;
	}
	if (std::none_of(item->variants.begin(), item->variants.end(),
			 [variant_id](const variant &entry) { return entry.id == variant_id; }))
	{
		if (error)
			*error = "variant is not supported by this artifact";
		return control_result::unsupported;
	}
	switch (holder)
	{
	case holder_kind::player:
		item->player_variant = variant_id;
		break;
	case holder_kind::wild_npc:
		item->wild_npc_variant = variant_id;
		break;
	case holder_kind::controlled_npc:
		item->controlled_npc_variant = variant_id;
		break;
	}
	active_status.dirty = true;
	return control_result::ok;
}

control_result control_set_enabled(int vnum, bool enabled, std::string *error)
{
	if (!active_status.initialized)
	{
		if (error)
			*error = "catalog is not initialized";
		return control_result::unavailable;
	}
	if (!active_status.dirty)
		draft_catalog = active_catalog;
	definition *item = catalog_find(draft_catalog, vnum);
	if (!item)
	{
		if (error)
			*error = "artifact vnum not found";
		return control_result::not_found;
	}
	item->enabled = enabled;
	active_status.dirty = true;
	return control_result::ok;
}

bool control_publish(std::string *error)
{
	if (!active_status.initialized)
	{
		if (error)
			*error = "catalog is not initialized";
		return false;
	}
	if (!active_status.dirty)
		return true;
	draft_catalog.revision = active_catalog.revision + 1;
	draft_catalog.hash = catalog_hash(draft_catalog);
	std::vector<diagnostic> diagnostics;
	if (!catalog_save_file(active_status.source_path.c_str(), draft_catalog, &diagnostics))
	{
		if (error)
			*error = diagnostics.empty() ? "catalog save failed" :
						       diagnostics.front().message;
		return false;
	}
	active_catalog = draft_catalog;
	active_status.dirty = false;
	active_status.active_revision = active_catalog.revision;
	active_status.active_hash = active_catalog.hash;
	return true;
}

bool control_discard(std::string *error)
{
	if (!active_status.initialized)
	{
		if (error)
			*error = "catalog is not initialized";
		return false;
	}
	draft_catalog = active_catalog;
	active_status.dirty = false;
	return true;
}

} // namespace artifact_control
