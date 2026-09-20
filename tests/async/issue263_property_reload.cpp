// Executes real properties.c parsing/commands; unrelated game services are stubs.
#include "world/properties.c"
#include "telemetry/telemetry_config_reload.h"
#include "telemetry/telemetry_config_private.h"

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

float hp_mob_con_factor;
float hp_mob_npc_pc_ratio;
int damroll_cap;
int hitroll_cap;
int errand_notch;
static unsigned completed_applies;
static std::string output;

void update_item_action_properties() {}
void update_weapon_action_properties() {}
void update_device_action_properties() {}
void update_studio_ability_properties() {}
void update_native_artifact_properties() {}
void update_difficulty_dials() {}
void update_stat_data() {}
void update_damage_data() {}
void update_spellpulse_data() {}
void update_racial_shrug_data() {}
void update_racial_exp_mods() {}
void update_racial_exp_mod_victims() {}
void update_exp_mods() {}
void update_exp_table() {}
void update_dam_factors() {}
void update_racial_dam_factors() {}
void update_saving_throws() {}
void update_breath_weapon_properties() {}
void update_regen_properties() {}
void update_misfire_properties()
{
	++completed_applies;
}
void collector_config_reload() {}
void item_actions_reload() {}
void debug(const char *, ...) {}
void logit(const char *, const char *, ...) {}
void wizlog(int, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}
void send_to_char(const char *text, P_char)
{
	output += text;
}
char *str_dup(const char *text)
{
	return ::strdup(text);
}
void __free(void *p, const char *, int)
{
	std::free(p);
}
bool studio_abilities_reload_file(std::string &)
{
	return true;
}
bool ws_is_durisweb_mud_gated_hook(const char *)
{
	return false;
}
void ws_broadcast_durisweb_hook_state() {}

struct observations
{
	std::array<float, 16> values{};
	unsigned count{};
	unsigned last_apply{};
};

static void observe(void *opaque) noexcept
{
	auto &state = *static_cast<observations *>(opaque);
	assert(completed_applies > state.last_apply);
	assert(hp_mob_con_factor == get_property("hitpoints.mob.conFactor", 0.4));
	state.last_apply = completed_applies;
	assert(state.count < state.values.size());
	state.values[state.count++] = get_property("epic.touch.PayoutFactor", 1.0);
}

static bool native_property_read(void *, const char *key, float *value) noexcept
{
	if (std::strcmp(key, "exp.zoneTrophy.observe") == 0)
		*value = get_property(key, 0.0);
	else if (std::strcmp(key, "exp.rested.enabled") == 0)
		*value = get_property(key, 1.0);
	else if (std::strcmp(key, "epic.touch.maxPayoutFactor") == 0)
		*value = get_property(key, 10.0);
	else if (std::strcmp(key, "epic.touch.PayoutFactor") == 0)
		*value = get_property(key, 1.0);
	else if (std::strcmp(key, "epic.zone.alignmentMod") == 0 ||
		 std::strcmp(key, "epic.alignment.minPercentage") == 0)
		*value = get_property(key, 0.10);
	else
		return false;
	return true;
}

static bool overflowing_property_read(void *ctx, const char *key, float *value) noexcept
{
	if (std::strcmp(key, "exp.zoneTrophy.observe") == 0)
	{
		*value = 2147483648.0F;
		return true;
	}
	return native_property_read(ctx, key, value);
}

static void command(P_char actor, const char *text)
{
	std::array<char, 256> args{};
	std::snprintf(args.data(), args.size(), "%s", text);
	do_properties(actor, args.data(), 0);
}

static void property_file(float payout)
{
	std::ofstream file("lib/duris.properties");
	file << "epic.touch.PayoutFactor=" << payout << '\n' << "hitpoints.mob.conFactor=0.75\n";
	assert(file.good());
}

int main()
{
	char_data actor{};
	actor.player.level = FORGER;
	actor.player.name = const_cast<char *>("PropertyQa");
	observations seen{}, other{};
	property_file(1.0F);
	initialize_properties(); // No observer: existing native behavior.
	assert(get_property("epic.touch.PayoutFactor", 1.0) == 1.0F);
	telemetry_config_property_capture capture{};
	capture.reader = { native_property_read, nullptr };
	capture.mode = telemetry_config_property_capture_mode::require_reader;
	telemetry_config_property_snapshot live_properties{};
	assert(telemetry_config_property_snapshot_capture(&capture, &live_properties) ==
	       telemetry_config_build_outcome::built);
	const auto defaults = telemetry_config_property_snapshot_defaults();
	assert(defaults.property_version == live_properties.property_version);
	capture.reader.read = overflowing_property_read;
	assert(telemetry_config_property_snapshot_capture(&capture, &live_properties) ==
	       telemetry_config_build_outcome::property_registry_invalid);
	capture.reader.read = native_property_read;
	assert(!telemetry_config_reload_register(nullptr, &seen));
	assert(telemetry_config_reload_register(observe, &seen));
	assert(telemetry_config_reload_register(observe, &seen));
	assert(!telemetry_config_reload_register(observe, &other));
	apply_properties();
	assert(seen.count == 1 && seen.values[0] == 1.0F);
	command(&actor, "set epic.touch.PayoutFactor 2.0");
	assert(seen.count == 2 && seen.values[1] == 2.0F);
	command(&actor, "revert");
	assert(seen.count == 3 && seen.values[2] == 1.0F);
	assert(output.find("reverted") != std::string::npos);
	command(&actor, "save");
	assert(seen.count == 3); // Saving without an effective change is not a reload.
	property_file(3.0F);
	command(&actor, "reload");
	assert(seen.count == 4 && seen.values[3] == 3.0F);
	command(&actor, "set epic.touch.PayoutFactor bad");
	assert(seen.count == 4);
	actor.player.level = 1;
	command(&actor, "set epic.touch.PayoutFactor 4.0");
	assert(seen.count == 4 && get_property("epic.touch.PayoutFactor", 1.0) == 3.0F);
	actor.player.level = FORGER;
	assert(!telemetry_config_reload_unregister(observe, &other));
	assert(telemetry_config_reload_unregister(observe, &seen));
	command(&actor, "set epic.touch.PayoutFactor 4.0");
	assert(seen.count == 4 && get_property("epic.touch.PayoutFactor", 1.0) == 4.0F);
	// Link both actual modules and use the owner's observer through the hook.
	telemetry_config_state owner{};
	telemetry_config_state_config owner_options{};
	assert(telemetry_config_state_init(&owner, &owner_options) ==
	       telemetry_config_state_outcome::accepted);
	assert(telemetry_config_reload_register(telemetry_config_reload_observer, &owner));
	command(&actor, "set epic.touch.PayoutFactor 5.0");
	assert(telemetry_config_reload_requested(&owner));
	assert(telemetry_config_property_snapshot_capture(&capture, &live_properties) ==
	       telemetry_config_build_outcome::built);
	assert(defaults.property_version != live_properties.property_version);
	telemetry_config_reload_request_clear(&owner);
	assert(!telemetry_config_reload_requested(&owner));
	assert(telemetry_config_reload_unregister(telemetry_config_reload_observer, &owner));
	std::cout
		<< "ISSUE263_PROPERTY_JOURNEY_OK: initialize/set/revert/save/reload; "
		   "post-cache notification; absent observer; ownership; malformed/unauthorized refusal\n";
}
