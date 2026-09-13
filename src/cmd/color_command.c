#include "cmd/color_command.h"
#include "core/prototypes.h"
#include "player/output_preferences.h"
#include <algorithm>
#include <vector>

namespace
{
// One definition drives discovery, resolution, aliases, samples and valid modes.
constexpr ColorCommandChannel commands[] = {
	{ OutputChannel::ChatSay, "say", "", "Incoming speech and your own speech echo.",
	  "Someone says 'Hello, traveler.'\r\n", false },
	{ OutputChannel::ChatTell, "tell", "reply",
	  "Incoming tells, replies and your own tell echo.",
	  "&+WSomeone tells you 'Hello, traveler.'&N\r\n", false },
	{ OutputChannel::ChatGuild, "guild", "gcc",
	  "Incoming guild conversation and your own guild echo.",
	  "&+cSomeone tells your guild '&+CHello, travelers.&n&+c'\r\n", false },
	{ OutputChannel::ChatShout, "shout", "", "Incoming shouts and your own shout echo.",
	  "Someone shouts 'Hello, travelers.'\r\n", false },
	{ OutputChannel::ChatYell, "yell", "", "Incoming yells and your own yell echo.",
	  "Someone yells 'Over here!'\r\n", false },
	{ OutputChannel::ChatWhisper, "whisper", "",
	  "Whispers visible to you and your own whisper echo.",
	  "Someone whispers to you, 'Hello, traveler.'\r\n", false },
	{ OutputChannel::ChatAsk, "ask", "", "Questions visible to you and your own question echo.",
	  "Someone asks you, 'Which way?'\r\n", false },
	{ OutputChannel::ChatPetition, "petition", "",
	  "Petitions you are permitted to receive and your own echo.",
	  "Someone petitions 'May I have assistance?'\r\n", false },
	{ OutputChannel::RoomDescription, "room", "room.description",
	  "Long room prose; titles and maps have separate presentation. Animated moves only on new eligible output.",
	  "A stream winds through the forest beneath the stars.\r\n", true },
};

std::string words(std::string_view text)
{
	std::string result(text);
	std::replace(result.begin(), result.end(), '_', ' ');
	return result;
}

std::vector<std::string> tokens(std::string_view text)
{
	std::vector<std::string> result;
	std::string token;
	for (unsigned char ch : text)
	{
		if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
		{
			if (!token.empty())
			{
				result.push_back(std::move(token));
				token.clear();
			}
		}
		else
			token += ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch;
	}
	if (!token.empty())
		result.push_back(std::move(token));
	return result;
}

std::string channel_names()
{
	std::string result;
	for (const auto &channel : commands)
	{
		if (!result.empty())
			result += ", ";
		result += channel.name;
	}
	return result;
}

const ColorCommandChannel *resolve(std::string_view name, std::string &error)
{
	for (const auto &channel : commands)
		if (name == channel.name || (!channel.alias.empty() && name == channel.alias))
			return &channel;
	std::vector<const ColorCommandChannel *> candidates;
	for (const auto &channel : commands)
		if (channel.name.starts_with(name))
			candidates.push_back(&channel);
	if (candidates.size() == 1)
		return candidates[0];
	if (candidates.empty())
		error = "Unknown channel. Channels: " + channel_names() + ".\r\n";
	else
	{
		error = "Ambiguous channel. Choose: ";
		for (const auto *candidate : candidates)
			error += std::string(candidate->name) + " ";
		error += "\r\n";
	}
	return nullptr;
}

std::string options(const ColorCommandChannel &channel, bool bright = false)
{
	if (channel.modes)
		return "Choices: default, static, animated. Example: toggle color " +
		       std::string(channel.name) + " animated\r\n";
	std::string result = bright ? "Bright colors: " : "Colors: default";
	for (auto color : output_palette_choices())
		if (!bright || color.name.starts_with("bright_"))
			result += (result.back() == ' ' ? "" : ", ") + words(color.name);
	return result + ". Example: toggle color " + std::string(channel.name) + " bright cyan\r\n";
}

std::string preview(const ColorCommandChannel &channel, const OutputProfilePreferences &preferences,
		    const std::shared_ptr<const OutputProfileSnapshot> &snapshot)
{
	auto resolved = resolve_output_profile(
		snapshot, channel.channel,
		channel.modes ? OutputPolicy::Animated : OutputPolicy::Static, preferences);
	// These fixed samples contain only known template wrappers. Authored player
	// text is never passed through this template conversion.
	std::string source(channel.sample);
	if (resolved.context.policy != OutputPolicy::Preserve)
	{
		char plain[MAX_STRING_LENGTH];
		AnsiString(source.c_str()).plain(plain);
		source = plain;
	}
	std::string styled;
	if (!render_output_message(source.c_str(), resolved.context, styled))
		styled = source;
	std::string result = "Sample (" + std::string(channel.name) + "): " + styled;
	if (channel.modes && preferences.get(channel.channel) != OutputProfileChoice::Default &&
	    resolved.context.policy == OutputPolicy::Preserve)
		result +=
			"This channel has no configured profile; its authored presentation is retained.\r\n";
	return result;
}

bool choose(const ColorCommandChannel &channel, const std::vector<std::string> &args, size_t offset,
	    OutputProfilePreferences &preferences, std::string &error)
{
	const size_t count = args.size() - offset;
	if (!count)
	{
		error = options(channel);
		return false;
	}
	if (count == 1 && args[offset] == "default")
		return preferences.reset(channel.channel);
	if (channel.modes)
	{
		if (count == 1 && (args[offset] == "static" || args[offset] == "animated"))
			return preferences.set(channel.channel,
					       args[offset] == "static" ?
						       OutputProfileChoice::Static :
						       OutputProfileChoice::Animated);
		error = options(channel);
		return false;
	}
	if (count == 1 && args[offset] == "bright")
	{
		error = options(channel, true);
		return false;
	}
	std::string name = args[offset];
	if (count == 2 && name == "bright")
		name += "_" + args[offset + 1];
	else if (count != 1)
	{
		error = "Unexpected extra input. " + options(channel);
		return false;
	}
	int attr = 0;
	if (!parse_output_palette(name, attr))
	{
		error = name == "purple" || name == "bright_purple" ?
				"Use magenta (or bright magenta) for purple. " :
			name == "off" ?
				"Use default to restore original colors; default retains authored colors. " :
				"Unknown color. ";
		error += options(channel);
		return false;
	}
	return preferences.set_color(channel.channel, attr);
}
}

std::span<const ColorCommandChannel> color_command_channels()
{
	return commands;
}

std::string output_preference_label(const OutputProfilePreferences &preferences,
				    OutputChannel channel)
{
	if (int attr = preferences.color(channel))
		for (auto color : output_palette_choices())
			if (color.attr == attr)
				return words(color.name);
	switch (preferences.get(channel))
	{
	case OutputProfileChoice::Default:
		return "default";
	case OutputProfileChoice::Preserve:
		return "preserve";
	case OutputProfileChoice::Static:
		return "static";
	case OutputProfileChoice::Animated:
		return "animated";
	}
	return "default";
}

ColorCommandResult evaluate_color_command(std::string_view arguments,
					  const OutputProfilePreferences &preferences,
					  std::shared_ptr<const OutputProfileSnapshot> snapshot)
{
	ColorCommandResult result{ preferences, {}, false };
	if (arguments.size() > 2048)
	{
		result.text = "Color command is too long. Try toggle color.\r\n";
		return result;
	}
	const auto args = tokens(arguments);
	if (args.empty())
	{
		result.text = "Color settings (default retains original colors):\r\n";
		for (const auto &channel : commands)
			result.text += std::string(channel.name) + ": " +
				       output_preference_label(preferences, channel.channel) +
				       "\r\n";
		result.text +=
			"Motion: " +
			std::string(preferences.motion_enabled ?
					    "on" :
					    "off (animated output uses a stable frame)") +
			"\r\n"
			"Choose a channel for details. Examples: toggle color tell; toggle color tell bright cyan; toggle color room animated\r\n"
			"Preview: toggle color preview [channel [color]]. Reset: toggle color reset [channel|all].\r\n"
			"Motion: toggle color motion off|on. Aliases: reply = tell; gcc = guild.\r\n";
		return result;
	}
	if (args[0] == "motion")
	{
		if (args.size() != 2 || (args[1] != "on" && args[1] != "off"))
		{
			result.text =
				"Motion is " +
				std::string(preferences.motion_enabled ? "on" : "off") +
				". Use toggle color motion off|on. Off freezes decorative animation; text and authored colors remain.\r\n";
			return result;
		}
		result.preferences.motion_enabled = args[1] == "on";
		result.mutation = result.preferences.state() != preferences.state();
		result.text =
			result.mutation ?
				"Motion: " +
					std::string(preferences.motion_enabled ? "on" : "off") +
					" -> " + args[1] + ".\r\n" :
				"Motion already " + args[1] + "; unchanged.\r\n";
		result.text += args[1] == "off" ?
				       "Animated profiles now use a stable frame.\r\n" :
				       "Animated profiles may advance on new eligible output.\r\n";
		return result;
	}
	const bool reset = args[0] == "reset", viewing = args[0] == "preview";
	const size_t offset = (reset || viewing) ? 1 : 0;
	if (reset && args.size() == 1)
	{
		result.text =
			"Reset one: toggle color reset tell. Reset every channel and motion: toggle color reset all.\r\n";
		return result;
	}
	if (reset && args.size() == 2 && args[1] == "all")
	{
		result.preferences.reset_all();
		result.mutation = result.preferences.state() != preferences.state();
		result.text =
			result.mutation ?
				"All channel choices restored to default; motion restored to on.\r\n" :
				"All choices already default; unchanged.\r\n";
		return result;
	}
	if (viewing && args.size() == 1)
	{
		for (const auto &channel : commands)
			result.text += preview(channel, preferences, snapshot);
		result.text += "Preview only; settings and animation phase are unchanged.\r\n";
		return result;
	}
	const auto *channel = resolve(args[offset], result.text);
	if (!channel)
		return result;
	if (reset)
	{
		if (args.size() != 2)
		{
			result.text = "Unexpected extra input. Use toggle color reset " +
				      std::string(channel->name) + ".\r\n";
			return result;
		}
		result.preferences.reset(channel->channel);
	}
	else if (args.size() == offset + 1)
	{
		result.text = std::string(channel->name) + ": " +
			      output_preference_label(preferences, channel->channel) + ". " +
			      std::string(channel->description) + "\r\n";
		result.text += viewing ? preview(*channel, preferences, snapshot) :
					 options(*channel);
		return result;
	}
	else if (!choose(*channel, args, offset + 1, result.preferences, result.text))
		return result;
	const auto before = output_preference_label(preferences, channel->channel),
		   after = output_preference_label(result.preferences, channel->channel);
	if (viewing)
	{
		result.text = preview(*channel, result.preferences, snapshot) +
			      "Preview only. Apply: toggle color " + std::string(channel->name) +
			      " " + after + "\r\n";
		result.preferences = preferences;
		return result;
	}
	result.mutation = result.preferences.state() != preferences.state();
	result.text = std::string(channel->name) +
		      (result.mutation ? ": " + before + " -> " + after + ".\r\n" :
					 " already " + after + "; unchanged.\r\n");
	result.text += preview(*channel, result.preferences, snapshot) + "Restore: toggle color " +
		       std::string(channel->name) + " default\r\n";
	return result;
}

void do_color_preferences(P_char recipient, const char *arguments)
{
	auto result = evaluate_color_command(arguments ? arguments : "",
					     player_output_preferences(recipient),
					     output_profile_registry().snapshot());
	if (result.mutation)
	{
		const auto update = update_player_output_preferences(recipient, result.preferences);
		if (update == OutputPreferenceUpdate::Unavailable)
			result.text =
				"Unable to queue the settings save. Previous choices remain active; try again later.\r\n";
		else if (update == OutputPreferenceUpdate::PendingSave)
			result.text += "Applied for future output; save pending.\r\n";
	}
	// Hints and already-rendered samples never enter automatic styling or advance phases.
	send_to_char(result.text.c_str(), recipient, OutputContext{});
}
