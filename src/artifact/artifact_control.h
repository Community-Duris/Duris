#ifndef DURIS_ARTIFACT_CONTROL_H
#define DURIS_ARTIFACT_CONTROL_H

#include <cstdint>
#include <string>
#include <vector>

namespace artifact_control
{

enum class holder_kind : uint8_t
{
	player,
	wild_npc,
	controlled_npc,
};

enum class control_result : uint8_t
{
	ok,
	not_found,
	invalid,
	conflict,
	forbidden,
	unavailable,
	unsupported,
};

struct diagnostic
{
	std::string code;
	std::string path;
	std::string message;
};

struct variant
{
	std::string id;
	std::string adapter;
};

struct power
{
	std::string id;
	bool enabled = true;
	int chance_numerator = 1;
	int chance_denominator = 1;
	int cooldown_seconds = 0;
	int windup_pulses = 0;
	int mana_cost_milliunits = 0;
	int power_level = 0;
};

struct definition
{
	std::string id;
	int vnum = 0;
	std::string display_name;
	std::string classification;
	std::string uniqueness_family;
	std::string default_variant;
	std::string player_variant;
	std::string wild_npc_variant;
	std::string controlled_npc_variant;
	std::string legacy_binding;
	std::string source;
	int source_line = 0;
	int initial_lifetime_seconds = 864000;
	int max_lifetime_seconds = 864000;
	int load_chance = 100;
	int load_limit = 1;
	int load_room = 0;
	int load_mob = 0;
	int load_slot = -1;
	bool enabled = true;
	std::vector<variant> variants;
	std::vector<power> powers;
};

struct catalog
{
	int schema_version = 1;
	uint64_t revision = 1;
	std::string hash;
	std::vector<definition> definitions;
};

struct status
{
	bool initialized = false;
	bool dirty = false;
	uint64_t active_revision = 0;
	std::string active_hash;
	std::string source_path;
	std::string last_error;
};

const char *artifact_control_default_path();

bool catalog_validate(const catalog &value, std::vector<diagnostic> *diagnostics);
bool catalog_load_file(const char *path, catalog *out, std::vector<diagnostic> *diagnostics);
bool catalog_save_file(const char *path, const catalog &value,
		       std::vector<diagnostic> *diagnostics);
std::string catalog_to_json(const catalog &value);
std::string catalog_hash(const catalog &value);

const definition *catalog_find(const catalog &value, int vnum);
definition *catalog_find(catalog &value, int vnum);
const char *holder_name(holder_kind holder);
bool parse_holder(const char *text, holder_kind *holder);
const std::string &variant_for(const definition &item, holder_kind holder);

// Runtime catalog service. It deliberately has no database or character
// pointers; callers use the returned immutable snapshot on the game thread.
bool control_initialize(const char *path, std::string *error);
bool control_reload(std::string *error);
const catalog &control_catalog();
status control_status();
bool control_enabled(int vnum);
bool control_allows_variant(int vnum, holder_kind holder, const char *variant_id);
bool control_power_enabled(int vnum, const char *power_id);
int control_power_level(int vnum, const char *power_id, int fallback);
std::string control_inspect(int vnum);
std::string control_preview(int vnum);
std::string control_list(const char *filter);
control_result control_set_variant(int vnum, holder_kind holder, const char *variant_id,
				   std::string *error);
control_result control_set_enabled(int vnum, bool enabled, std::string *error);
bool control_publish(std::string *error);
bool control_discard(std::string *error);

} // namespace artifact_control

#endif
