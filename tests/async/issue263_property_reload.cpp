// Executes real properties.c parsing/commands; unrelated game services are stubs.
#include "world/properties.c"
#include "telemetry/telemetry_config_reload.h"
#include "telemetry/telemetry_config_private.h"

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>
#include <csignal>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>

float hp_mob_con_factor;
float hp_mob_npc_pc_ratio;
int damroll_cap;
int hitroll_cap;
int errand_notch;
static unsigned completed_applies;
static std::string output;
P_char character_list;
static unsigned rebalanced_characters;
static std::string io_failure;
static unsigned saved_logs;

// Inject failures after real I/O so the production error checks must notice them.
extern "C" int __real_fclose(FILE *);
extern "C" int __wrap_fclose(FILE *stream)
{
	bool writing = (fcntl(fileno(stream), F_GETFL) & O_ACCMODE) != O_RDONLY;
	int result = __real_fclose(stream);
	return io_failure == (writing ? "close-target" : "close-source") ? EOF : result;
}
extern "C" int __real_rename(const char *, const char *);
extern "C" int __wrap_rename(const char *from, const char *to)
{
	return io_failure == "rename" ? -1 : __real_rename(from, to);
}
extern "C" char *__real_fgets(char *, int, FILE *);
extern "C" char *__wrap_fgets(char *buf, int size, FILE *stream)
{
	char *result = __real_fgets(buf, size, stream);
	if (result && io_failure == "read")
	{
		// The buffered prefix remains readable, then the real stream sets ferror.
		assert(close(fileno(stream)) == 0);
		io_failure.clear();
	}
	return result;
}
void sql_log(P_char, const char *, const char *format, ...)
{
	if (!strcmp(format, "Saved properties"))
		++saved_logs;
}

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
void world_activity_reload() {}
void item_actions_reload() {}
void debug(const char *, ...) {}
void logit(const char *, const char *, ...) {}
void wizlog(int, const char *, ...) {}

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

static std::string read_file()
{
	std::ifstream file("lib/duris.properties");
	return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

static void failure_case(P_char actor, const std::string &test)
{
	if (test == "keys")
	{
		for (size_t length : { 127, 128, 200 })
		{
			std::string line(length, 'A');
			line += "=1\n";
			struct property parsed
			{
			};
			int result = parse_property(&parsed, line.data());
			assert((result == 1) == (length == 127));
			if (result == 1)
			{
				assert(strlen(parsed.key) == length && parsed.value == 1);
				FREE(parsed.key);
			}
		}
		return;
	}
	property_file(1.0F);
	initialize_properties();
	command(actor, "set epic.touch.PayoutFactor 2");
	const int count = properties_count;
	const unsigned applies = completed_applies;
	const std::string original = read_file();
	const bool reload = test.starts_with("reload-");
	std::string fault = test.substr(test.find('-') + 1);
	if (fault == "missing")
		std::filesystem::rename("lib/duris.properties", "lib/original");
	else if (fault == "open")
		std::filesystem::create_directory("lib/duris.properties.new");
	else if (fault == "malformed" || fault == "limit")
	{
		std::ofstream file("lib/duris.properties");
		file << "epic.touch.PayoutFactor=99\n";
		if (fault == "malformed")
			file << std::string(200, 'A') << "=1\n";
		else
			for (int i = 0; i < MAX_PROPERTIES; ++i)
				file << "key." << i << "=1\n";
	}
	else if (fault == "write" || fault == "flush")
	{
		std::ofstream file("lib/duris.properties", std::ios::app);
		for (int i = 0; i < (fault == "write" ? 20 : 1); ++i)
			file << '#' << std::string(800, 'x') << '\n';
		file.close();
		signal(SIGXFSZ, SIG_IGN);
		struct rlimit limit
		{
		};
		assert(getrlimit(RLIMIT_FSIZE, &limit) == 0);
		limit.rlim_cur = 64;
		assert(setrlimit(RLIMIT_FSIZE, &limit) == 0);
	}
	const std::string before = fault == "missing" ? original : read_file();
	if (fault == "read" || fault.starts_with("close-") || fault == "rename")
		io_failure = fault;
	output.clear();
	command(actor, reload ? "reload" : "save");
	io_failure.clear();
	assert(properties_count == count);
	assert(get_property("epic.touch.PayoutFactor", 0.0) == 2);
	assert(completed_applies == applies && rebalanced_characters == 0);
	assert(saved_logs == 0);
	assert(output.find("failed") != std::string::npos);
	if (fault == "missing")
		std::filesystem::rename("lib/original", "lib/duris.properties");
	assert(read_file() == before);
	if (fault == "open")
		std::filesystem::remove("lib/duris.properties.new");
	assert(!std::filesystem::exists("lib/duris.properties.new"));
	// Both diff and revert must retain the last successful saved baseline.
	output.clear();
	command(actor, "diff");
	assert(output.find("epic.touch.PayoutFactor=2.000(1.000)") != std::string::npos);
	command(actor, "revert");
	assert(get_property("epic.touch.PayoutFactor", 0.0) == 1);
}

int main(int argc, char **argv)
{
	char_data actor{};
	actor.player.level = FORGER;
	actor.player.name = const_cast<char *>("PropertyQa");
	if (argc == 2)
	{
		failure_case(&actor, argv[1]);
		for (int i = 0; i < properties_count; ++i)
			FREE(duris_properties[i].key);
		std::cout << "property failure preserved state: " << argv[1] << '\n';
		return 0;
	}
	observations seen{}, other{};
	property_file(1.0F);
	initialize_properties(); // No observer: existing native behavior.
	const auto initial_content = divine_refusal_content_registry().snapshot();
	assert(initial_content && initial_content->revision() == 1);
	assert(initial_content->find(66026) && initial_content->find(66031));
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
	command(&actor, "save");
	assert(saved_logs == 2);
	assert(read_file().find("epic.touch.PayoutFactor=5.000\n") != std::string::npos);
	output.clear();
	command(&actor, "diff");
	assert(output.empty());
	command(&actor, "set epic.touch.PayoutFactor 9");
	command(&actor, "revert");
	assert(get_property("epic.touch.PayoutFactor", 0.0) == 5);
	command(&actor, "reload");
	assert(get_property("epic.touch.PayoutFactor", 0.0) == 5);
	// The real admin reload must retain good content after a malformed file,
	// then publish a complete valid replacement for subsequent orders.
	actor.player.level = FORGER;
	const auto before_invalid_content = divine_refusal_content_registry().snapshot();
	{
		std::ofstream content(DIVINE_REFUSAL_CONTENT_FILE);
		content << "{malformed";
	}
	command(&actor, "reload");
	assert(divine_refusal_content_registry().snapshot() == before_invalid_content);
	{
		std::ofstream content(DIVINE_REFUSAL_CONTENT_FILE);
		content << R"({"version":1,"revision":2,"entries":{"66026":{"patron":"Garl","message":"{patron} declines."}}})";
	}
	command(&actor, "reload");
	const auto replacement_content = divine_refusal_content_registry().snapshot();
	assert(replacement_content && replacement_content->revision() == 2);
	assert(!replacement_content->find(66031));
	char rendered[128]{};
	assert(divine_refusal_content_render(replacement_content->find(66026), rendered,
					     sizeof(rendered)));
	assert(std::strcmp(rendered, "Garl declines.") == 0);
	// Existing borrowers still see their original immutable content.
	assert(initial_content->revision() == 1 && initial_content->find(66031));
	for (int i = 0; i < properties_count; ++i)
		FREE(duris_properties[i].key);
	std::cout
		<< "ISSUE263_PROPERTY_JOURNEY_OK: initialize/set/revert/save/reload; "
		   "post-cache notification; absent observer; ownership; malformed/unauthorized refusal\n";
}
