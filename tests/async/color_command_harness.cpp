#include "cmd/color_command.h"
#include "player/output_preferences.h"
#include <cassert>
#include <cstdio>

static OutputProfilePreferences current;
static OutputPreferenceUpdate admission = OutputPreferenceUpdate::PendingSave;
static unsigned saves = 0;
static std::string output;
void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}
OutputProfilePreferences player_output_preferences(char_data *)
{
	return current;
}
OutputPreferenceUpdate update_player_output_preferences(char_data *,
							const OutputProfilePreferences &next)
{
	++saves;
	if (admission == OutputPreferenceUpdate::PendingSave)
		current = next;
	return admission;
}
void send_to_char(const char *text, char_data *, const OutputContext &context)
{
	assert(context.policy == OutputPolicy::Preserve && !context.sequence);
	output = text;
}
int main()
{
	struct Case
	{
		const char *input;
		const char *hint;
		bool mutation;
	};
	const Case cases[] = {
		{ "", "Color settings", false },
		{ "tell", "Incoming tells", false },
		{ "tell bright", "bright cyan", false },
		{ "tell bright cyan", "default -> bright cyan", true },
		{ "tell default", "already default", false },
		{ "preview", "Sample (say)", false },
		{ "preview tell", "Someone tells you", false },
		{ "preview tell bright cyan", "Apply: toggle color tell bright cyan", false },
		{ "reset", "Reset one", false },
		{ "reset tell", "already default", false },
		{ "reset all", "already default", false },
		{ "room", "default, static, animated", false },
		{ "room animated", "default -> animated", true },
		{ "motion off", "stable frame", true },
		{ "motion on", "already on", false },
		{ "REpLy   BRIGHT\tCyAn", "tell: default -> bright cyan", true },
		{ "gcc red", "guild: default -> red", true },
		{ "t bright cyan", "tell: default -> bright cyan", true },
		{ "s red", "Ambiguous channel", false },
		{ "missing red", "Channels:", false },
		{ "tell purple", "Use magenta", false },
		{ "tell b", "Unknown color", false },
		{ "tell bright cyan extra", "Unexpected extra input", false },
		{ "tell default extra", "Unexpected extra input", false },
		{ "motion off extra", "off|on", false },
		{ "reset tell extra", "Unexpected extra input", false },
		{ "reset all extra", "Unknown channel", false },
		{ "room animated extra", "Choices:", false },
		{ "tell off", "retains authored colors", false },
		{ "tell animated", "Unknown color", false },
		{ "preview tell purple", "magenta", false },
		{ "room cyan", "Choices:", false },
		{ "motion", "Motion is on", false },
	};
	for (auto test : cases)
	{
		auto result = evaluate_color_command(test.input, {}, {});
		if (result.text.find(test.hint) == std::string::npos)
		{
			std::fprintf(stderr, "%s: %s\n", test.input, result.text.c_str());
			return 1;
		}
		assert(result.mutation == test.mutation);
		if (!result.mutation)
			assert(result.preferences.state() == OutputPreferenceState{});
	}
	for (const auto &channel : color_command_channels())
	{
		OutputChannel canonical{};
		bool registered = false;
		for (auto known : output_channel_choices())
			if (known.channel == channel.channel)
			{
				canonical = known.channel;
				registered = true;
			}
		assert(registered && canonical != OutputChannel::Unspecified);
		for (const auto &color : output_palette_choices())
		{
			if (channel.modes)
				break;
			std::string name(color.name);
			for (char &ch : name)
				if (ch == '_')
					ch = ' ';
			auto result = evaluate_color_command(std::string(channel.name) + " " + name,
							     {}, {});
			assert(result.mutation &&
			       result.preferences.color(channel.channel) == color.attr);
			auto unchanged = evaluate_color_command(
				std::string(channel.name) + " " + name, result.preferences, {});
			assert(!unchanged.mutation &&
			       unchanged.text.find("unchanged") != std::string::npos);
		}
	}
	// Compare the shown color with the real renderer, including removal of the
	// known white preview wrapper and retention of the readable label outside it.
	auto sample = evaluate_color_command("preview tell bright cyan", {}, {});
	const size_t begin = sample.text.find("Sample (tell): ") + 15;
	AnsiString rendered(sample.text.substr(begin).c_str());
	assert(rendered.ch(0) == 'S' && rendered.attr(0) == ATTR_FG(27));
	auto &registry = output_profile_registry();
	assert(registry.reload_json(
			       R"({"version":1,"revision":1,"recipes":{"water":{"kind":"flow","palette":["cyan","blue"]}},"dictionaries":{"nature":{"stream":"water"}},"profiles":{"nature":{"policy":"animated","dictionary":"nature"}},"channels":{"room.description":"nature"}})")
		       .ok);
	current.set(OutputChannel::RoomDescription, OutputProfileChoice::Animated);
	current.motion_enabled = false;
	auto first = evaluate_color_command("preview room", current, registry.snapshot());
	auto second = evaluate_color_command("preview room", current, registry.snapshot());
	assert(first.text == second.text && first.preferences.state() == current.state());
	current.reset_all();
	do_color_preferences(nullptr, "tell bright cyan");
	assert(saves == 1 && current.color(OutputChannel::ChatTell) == ATTR_FG(27) &&
	       output.find("save pending") != std::string::npos);
	do_color_preferences(nullptr, "tell bright cyan");
	assert(saves == 1 && output.find("unchanged") != std::string::npos);
	do_color_preferences(nullptr, "preview tell red");
	assert(saves == 1 && current.color(OutputChannel::ChatTell) == ATTR_FG(27));
	admission = OutputPreferenceUpdate::Unavailable;
	do_color_preferences(nullptr, "tell red");
	assert(saves == 2 && current.color(OutputChannel::ChatTell) == ATTR_FG(27) &&
	       output.find("Previous choices remain") != std::string::npos);
	admission = OutputPreferenceUpdate::PendingSave;
	current.motion_enabled = false;
	do_color_preferences(nullptr, "reset tell");
	assert(!current.color(OutputChannel::ChatTell) && !current.motion_enabled);
	do_color_preferences(nullptr, "reset all");
	assert(current.state() == OutputPreferenceState{});
	puts("Color commands: discovery, aliases, ambiguity, strict parsing, previews, no-op, resets, motion and persistence failure passed");
}
