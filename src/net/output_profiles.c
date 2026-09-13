#include "net/output_profiles.h"
#include <algorithm>
#include <cjson/cJSON.h>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>

namespace
{
constexpr OutputChannelChoice channels[] = {
	{ OutputChannel::RoomDescription, "room.description" },
	{ OutputChannel::Chat, "chat.generic" },
	{ OutputChannel::Combat, "combat.generic" },
	{ OutputChannel::SystemFeedback, "system.feedback" },
	{ OutputChannel::RoomTitle, "room.title" },
	{ OutputChannel::RoomInspect, "room.inspect" },
	{ OutputChannel::RoomExits, "room.exits" },
	{ OutputChannel::RoomAuras, "room.auras" },
	{ OutputChannel::RoomOccupants, "room.occupants" },
	{ OutputChannel::ItemsList, "items.list" },
	{ OutputChannel::ChatSay, "chat.say" },
	{ OutputChannel::ChatTell, "chat.tell" },
	{ OutputChannel::ChatWhisper, "chat.whisper" },
	{ OutputChannel::ChatAsk, "chat.ask" },
	{ OutputChannel::ChatShout, "chat.shout" },
	{ OutputChannel::ChatYell, "chat.yell" },
	{ OutputChannel::ChatGroup, "chat.group" },
	{ OutputChannel::ChatGuild, "chat.guild" },
	{ OutputChannel::ChatAlliance, "chat.alliance" },
	{ OutputChannel::ChatPetition, "chat.petition" },
	{ OutputChannel::ChatProject, "chat.project" },
	{ OutputChannel::ChatPage, "chat.page" },
	{ OutputChannel::ChatRacewar, "chat.racewar" },
	{ OutputChannel::ChatImmortal, "chat.immortal" },
	{ OutputChannel::Social, "social" },
	{ OutputChannel::Weather, "weather" },
	{ OutputChannel::CombatIncoming, "combat.incoming" },
	{ OutputChannel::CombatOutgoing, "combat.outgoing" },
	{ OutputChannel::CombatObserved, "combat.observed" },
	{ OutputChannel::Prompt, "prompt" },
	{ OutputChannel::ChatAuction, "chat.auction" },
	{ OutputChannel::ChatNchat, "chat.nchat" },
	{ OutputChannel::ChatJchat, "chat.jchat" },
	{ OutputChannel::ChatWizmsg, "chat.wizmsg" },
};
constexpr OutputPaletteChoice colors[] = {
	{ "blue", ATTR_FG(17) },	   { "green", ATTR_FG(18) },
	{ "cyan", ATTR_FG(19) },	   { "red", ATTR_FG(20) },
	{ "magenta", ATTR_FG(21) },	   { "yellow", ATTR_FG(22) },
	{ "white", ATTR_FG(23) },	   { "gray", ATTR_FG(24) },
	{ "bright_blue", ATTR_FG(25) },	   { "bright_green", ATTR_FG(26) },
	{ "bright_cyan", ATTR_FG(27) },	   { "bright_red", ATTR_FG(28) },
	{ "bright_magenta", ATTR_FG(29) }, { "bright_yellow", ATTR_FG(30) },
	{ "bright_white", ATTR_FG(31) },
};
static_assert(std::size(channels) + 1 == OUTPUT_PROFILE_CHANNEL_COUNT);

bool valid_channel(OutputChannel channel)
{
	return channel > OutputChannel::Unspecified && channel < OutputChannel::Count;
}

std::string lowercase(std::string_view text)
{
	std::string result(text);
	for (char &ch : result)
		if (ch >= 'A' && ch <= 'Z')
			ch += 'a' - 'A';
	return result;
}

[[noreturn]] void invalid(const std::string &where, const char *reason)
{
	throw std::invalid_argument(where + ": " + reason);
}

bool ascii_word(char ch)
{
	return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
}

std::string identifier(const char *text, const std::string &where, bool word = false)
{
	std::string result = lowercase(text ? text : "");
	if (result.empty() || result.size() > (word ? 64u : 48u))
		invalid(where, "empty or oversized identifier");
	if (!word && (result.front() < 'a' || result.front() > 'z'))
		invalid(where, "identifier must start with an ASCII letter");
	for (size_t i = 0; i < result.size(); ++i)
	{
		char ch = result[i];
		if (ascii_word(ch) || (!word && (ch == '.' || ch == '-')))
			continue;
		if (word && ch == '\'' && i && i + 1 < result.size() && ascii_word(result[i - 1]) &&
		    ascii_word(result[i + 1]))
			continue;
		invalid(where, "identifier contains unsupported characters");
	}
	return result;
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
		std::string_view name(item->string);
		if (std::find(allowed.begin(), allowed.end(), name) == allowed.end())
			invalid(where, "unknown field");
		if (!seen.insert(name).second)
			invalid(where, "duplicate field");
	}
}

const char *string_value(const cJSON *value, const std::string &where)
{
	if (!cJSON_IsString(value))
		invalid(where, "expected a string");
	return value->valuestring;
}

uint32_t integer(const cJSON *value, uint32_t minimum, uint32_t maximum, const std::string &where)
{
	if (!cJSON_IsNumber(value) || !std::isfinite(value->valuedouble) ||
	    std::floor(value->valuedouble) != value->valuedouble || value->valuedouble < minimum ||
	    value->valuedouble > maximum)
		invalid(where, "expected an integer in the documented range");
	return (uint32_t)value->valuedouble;
}

int color_value(const cJSON *value, const std::string &where)
{
	int attr = 0;
	if (!parse_output_palette(string_value(value, where), attr))
		invalid(where, "unknown foreground color (black/background/blink are unsupported)");
	return attr;
}

void bounded_object(const cJSON *object, size_t maximum, const std::string &where)
{
	if (!cJSON_IsObject(object) || (size_t)cJSON_GetArraySize(object) > maximum)
		invalid(where, "expected an object within the documented entry limit");
}

void preflight(std::string_view json)
{
	if (json.empty() || json.size() > OUTPUT_PROFILE_MAX_BYTES ||
	    json.find('\0') != std::string_view::npos)
		invalid("configuration", "empty, oversized, or embedded NUL input");
	// Bound cJSON's recursive parse before entering it. Also reject decoded NULs,
	// which its C-string representation cannot distinguish from a truncated name.
	int depth = 0;
	bool quoted = false;
	for (size_t i = 0; i < json.size(); ++i)
	{
		char ch = json[i];
		if (quoted && ch == '\\')
		{
			if (json.substr(i, 6) == "\\u0000")
				invalid("configuration", "NUL escapes are unsupported");
			++i;
		}
		else if (ch == '"')
			quoted = !quoted;
		else if (!quoted && (ch == '{' || ch == '['))
		{
			if (++depth > 12)
				invalid("configuration", "nesting exceeds 12 levels");
		}
		else if (!quoted && (ch == '}' || ch == ']'))
			--depth;
	}
}
} // namespace

std::span<const OutputChannelChoice> output_channel_choices()
{
	return channels;
}
std::span<const OutputPaletteChoice> output_palette_choices()
{
	return colors;
}

bool parse_output_channel(std::string_view name, OutputChannel &channel)
{
	if (name.size() > 48)
		return false;
	std::string normalized = lowercase(name);
	for (const auto &choice : channels)
		if (normalized == choice.name)
		{
			channel = choice.channel;
			return true;
		}
	return false;
}

bool parse_output_palette(std::string_view name, int &attr)
{
	if (name.size() > 48)
		return false;
	std::string normalized = lowercase(name);
	for (const auto &choice : colors)
		if (normalized == choice.name)
		{
			attr = choice.attr;
			return true;
		}
	return false;
}

bool OutputProfilePreferences::set(OutputChannel channel, OutputProfileChoice choice)
{
	if (!valid_channel(channel) || choice < OutputProfileChoice::Default ||
	    choice > OutputProfileChoice::Animated)
		return false;
	choices_[(size_t)channel] = choice;
	colors_[(size_t)channel] = 0;
	return true;
}

bool OutputProfilePreferences::set_color(OutputChannel channel, int attr)
{
	if (!valid_channel(channel))
		return false;
	for (const auto &choice : output_palette_choices())
		if (choice.attr == attr)
		{
			choices_[(size_t)channel] = OutputProfileChoice::Static;
			colors_[(size_t)channel] = attr;
			return true;
		}
	return false;
}

int OutputProfilePreferences::color(OutputChannel channel) const
{
	return valid_channel(channel) ? colors_[(size_t)channel] : 0;
}

void OutputProfilePreferences::reset_all()
{
	*this = OutputProfilePreferences{};
}

OutputPreferenceState OutputProfilePreferences::state() const
{
	OutputPreferenceState result{};
	result.motion_off = !motion_enabled;
	for (size_t channel = 1; channel < OUTPUT_PROFILE_CHANNEL_COUNT; ++channel)
		result.choices[channel] = colors_[channel] ?
						  GET_FG(colors_[channel]) :
						  static_cast<uint8_t>(choices_[channel]);
	return result;
}

OutputProfilePreferences OutputProfilePreferences::from_state(const OutputPreferenceState &state)
{
	OutputProfilePreferences result;
	result.motion_enabled = !state.motion_off;
	for (size_t channel = 1; channel < OUTPUT_PROFILE_CHANNEL_COUNT; ++channel)
	{
		unsigned choice = state.choices[channel];
		if (choice <= static_cast<unsigned>(OutputProfileChoice::Animated))
			result.set(static_cast<OutputChannel>(channel),
				   static_cast<OutputProfileChoice>(choice));
		else if (choice >= 17 && choice <= 31)
			result.set_color(static_cast<OutputChannel>(channel), ATTR_FG(choice));
	}
	return result;
}

bool OutputProfilePreferences::reset(OutputChannel channel)
{
	return set(channel, OutputProfileChoice::Default);
}

OutputProfileChoice OutputProfilePreferences::get(OutputChannel channel) const
{
	return valid_channel(channel) ? choices_[(size_t)channel] : OutputProfileChoice::Preserve;
}

const OutputProfileSnapshot::Profile *OutputProfileSnapshot::profile(OutputChannel channel) const
{
	if (!valid_channel(channel))
		return nullptr;
	auto found = profiles_.find(channels_[(size_t)channel]);
	return found == profiles_.end() ? nullptr : &found->second;
}

const OutputStyleRecipe *OutputProfileSnapshot::recipe(std::string_view name) const
{
	auto found = recipes_.find(name);
	return found == recipes_.end() ? nullptr : &found->second;
}

const OutputStyleRecipe *OutputProfileSnapshot::word_recipe(OutputChannel channel,
							    std::string_view word) const
{
	auto selected = profile(channel);
	if (!selected)
		return nullptr;
	auto dictionary = dictionaries_.find(selected->dictionary);
	if (dictionary == dictionaries_.end())
		return nullptr;
	auto found = dictionary->second.words.find(word);
	return found == dictionary->second.words.end() ? nullptr : recipe(found->second);
}

const OutputStyleRecipe *ResolvedOutputProfile::word_recipe(std::string_view word) const
{
	return context.snapshot_owner && context.policy != OutputPolicy::Preserve ?
		       context.snapshot_owner->word_recipe(context.channel, word) :
		       nullptr;
}

ResolvedOutputProfile resolve_output_profile(std::shared_ptr<const OutputProfileSnapshot> snapshot,
					     OutputChannel channel, OutputPolicy caller_policy,
					     const OutputProfilePreferences &preferences)
{
	ResolvedOutputProfile result;
	result.context.channel = channel;
	if (caller_policy != OutputPolicy::Static && caller_policy != OutputPolicy::Animated)
		return result;
	// An explicit recipient foreground needs no dictionary/server configuration.
	// Only an adopted caller can reach this branch; Preserve remains an absolute veto.
	if (int attr = preferences.color(channel))
	{
		result.context.policy = OutputPolicy::Static;
		result.context.base_attr = attr;
		return result;
	}
	if (!snapshot)
		return result;
	auto profile = snapshot->profile(channel);
	if (!profile)
		return result;
	OutputPolicy policy = profile->policy;
	switch (preferences.get(channel))
	{
	case OutputProfileChoice::Default:
		break;
	case OutputProfileChoice::Preserve:
		policy = OutputPolicy::Preserve;
		break;
	case OutputProfileChoice::Static:
		policy = OutputPolicy::Static;
		break;
	case OutputProfileChoice::Animated:
		policy = OutputPolicy::Animated;
		break;
	}
	if (!preferences.motion_enabled && policy == OutputPolicy::Animated)
		policy = OutputPolicy::Static;
	if (policy == OutputPolicy::Preserve)
		return result;
	result.context.policy = policy;
	result.context.base_attr = profile->base_attr;
	auto dictionary = snapshot->dictionaries_.find(profile->dictionary);
	if (dictionary != snapshot->dictionaries_.end())
	{
		result.context.words = &dictionary->second.stable_words;
		result.context.recipes = &dictionary->second.recipes;
	}
	result.sender_attr = profile->sender_attr;
	result.entity_attr = profile->entity_attr;
	result.context.snapshot_owner = std::move(snapshot);
	return result;
}

class OutputProfileParser
{
    public:
	static std::shared_ptr<OutputProfileSnapshot> parse(std::string_view json)
	{
		preflight(json);
		std::string terminated(json);
		std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(
			cJSON_ParseWithOpts(terminated.c_str(), nullptr, true), cJSON_Delete);
		if (!root)
			invalid("configuration", "invalid JSON or trailing content");
		fields(root.get(),
		       { "version", "revision", "recipes", "dictionaries", "profiles", "channels" },
		       "configuration");
		if (integer(field(root.get(), "version"), 1, UINT32_MAX, "version") != 1)
			invalid("version", "unsupported schema version");
		auto result = std::make_shared<OutputProfileSnapshot>();
		result->revision_ =
			integer(field(root.get(), "revision"), 1, UINT32_MAX, "revision");
		read_recipes(*result, field(root.get(), "recipes"));
		read_dictionaries(*result, field(root.get(), "dictionaries"));
		read_profiles(*result, field(root.get(), "profiles"));
		const cJSON *routes = field(root.get(), "channels");
		bounded_object(routes, std::size(channels), "channels");
		for (const cJSON *route = routes->child; route; route = route->next)
		{
			OutputChannel channel;
			if (!parse_output_channel(route->string, channel))
				invalid("channels", "unknown channel identifier");
			auto name = identifier(string_value(route, "channels"), "channels");
			if (!result->profiles_.contains(name))
				invalid("channels", "unknown profile reference");
			auto &destination = result->channels_[(size_t)channel];
			if (!destination.empty())
				invalid("channels", "duplicate normalized channel");
			destination = name;
		}
		return result;
	}

    private:
	static void read_recipes(OutputProfileSnapshot &result, const cJSON *recipes)
	{
		bounded_object(recipes, OUTPUT_PROFILE_MAX_RECIPES, "recipes");
		for (const cJSON *item = recipes->child; item; item = item->next)
		{
			auto name = identifier(item->string, "recipes");
			std::string where = "recipes." + name;
			fields(item,
			       { "kind", "palette", "stable_index", "step_every", "width",
				 "chance_percent" },
			       where);
			OutputStyleRecipe recipe;
			auto kind = identifier(string_value(field(item, "kind"), where), where);
			if (kind == "solid")
				recipe.kind = OutputRecipeKind::Solid;
			else if (kind == "flow")
				recipe.kind = OutputRecipeKind::Flow;
			else if (kind == "shimmer")
				recipe.kind = OutputRecipeKind::Shimmer;
			else if (kind == "flicker")
				recipe.kind = OutputRecipeKind::Flicker;
			else if (kind == "pulse")
				recipe.kind = OutputRecipeKind::Pulse;
			else if (kind == "glint")
				recipe.kind = OutputRecipeKind::Glint;
			else
				invalid(where, "unknown recipe kind");
			const cJSON *palette = field(item, "palette");
			if (!cJSON_IsArray(palette) || !palette->child ||
			    (size_t)cJSON_GetArraySize(palette) > OUTPUT_PROFILE_MAX_PALETTE)
				invalid(where,
					"palette must contain 1..16 named foreground colors");
			for (const cJSON *color = palette->child; color; color = color->next)
				recipe.palette[recipe.palette_size++] = color_value(color, where);
			if (auto value = field(item, "stable_index"))
				recipe.stable_index =
					integer(value, 0, (uint32_t)recipe.palette_size - 1, where);
			if (auto value = field(item, "step_every"))
				recipe.step_every = (uint16_t)integer(value, 1, 1024, where);
			if (auto value = field(item, "width"))
				recipe.width = (uint16_t)integer(value, 1, 32, where);
			if (auto value = field(item, "chance_percent"))
				recipe.chance_percent = (uint8_t)integer(value, 0, 100, where);
			if (recipe.kind == OutputRecipeKind::Solid &&
			    (recipe.palette_size != 1 || field(item, "step_every") ||
			     field(item, "width") || field(item, "chance_percent")))
				invalid(where,
					"solid uses one foreground and no motion parameters");
			if (!result.recipes_.emplace(name, recipe).second)
				invalid(where, "duplicate normalized recipe");
		}
	}

	static void read_dictionaries(OutputProfileSnapshot &result, const cJSON *dictionaries)
	{
		bounded_object(dictionaries, OUTPUT_PROFILE_MAX_DICTIONARIES, "dictionaries");
		size_t words = 0;
		for (const cJSON *item = dictionaries->child; item; item = item->next)
		{
			auto name = identifier(item->string, "dictionaries");
			std::string where = "dictionaries." + name;
			bounded_object(item, OUTPUT_PROFILE_MAX_WORDS_PER_DICTIONARY, where);
			OutputProfileSnapshot::Dictionary dictionary;
			for (const cJSON *entry = item->child; entry; entry = entry->next)
			{
				if (++words > OUTPUT_PROFILE_MAX_WORDS)
					invalid(where, "total dictionary word limit exceeded");
				auto word = identifier(entry->string, where, true);
				auto recipe_name = identifier(string_value(entry, where), where);
				auto recipe = result.recipe(recipe_name);
				if (!recipe)
					invalid(where, "unknown recipe reference");
				if (!dictionary.words.emplace(word, recipe_name).second)
					invalid(where, "duplicate normalized word");
				dictionary.stable_words.emplace(
					word, recipe->palette[recipe->stable_index]);
				dictionary.recipes.emplace(word, recipe);
			}
			if (!result.dictionaries_.emplace(name, std::move(dictionary)).second)
				invalid(where, "duplicate normalized dictionary");
		}
	}

	static void read_profiles(OutputProfileSnapshot &result, const cJSON *profiles)
	{
		bounded_object(profiles, OUTPUT_PROFILE_MAX_PROFILES, "profiles");
		for (const cJSON *item = profiles->child; item; item = item->next)
		{
			auto name = identifier(item->string, "profiles");
			std::string where = "profiles." + name;
			fields(item, { "policy", "dictionary", "base", "roles" }, where);
			OutputProfileSnapshot::Profile profile;
			auto policy = identifier(string_value(field(item, "policy"), where), where);
			if (policy == "preserve")
				profile.policy = OutputPolicy::Preserve;
			else if (policy == "static")
				profile.policy = OutputPolicy::Static;
			else if (policy == "animated")
				profile.policy = OutputPolicy::Animated;
			else
				invalid(where, "unknown output policy");
			if (auto value = field(item, "dictionary"))
			{
				profile.dictionary = identifier(string_value(value, where), where);
				if (!result.dictionaries_.contains(profile.dictionary))
					invalid(where, "unknown dictionary reference");
			}
			if (auto value = field(item, "base"))
				profile.base_attr = color_value(value, where);
			if (auto roles = field(item, "roles"))
			{
				fields(roles, { "sender", "entity" }, where + ".roles");
				if (auto value = field(roles, "sender"))
					profile.sender_attr = color_value(value, where);
				if (auto value = field(roles, "entity"))
					profile.entity_attr = color_value(value, where);
			}
			if (!result.profiles_.emplace(name, std::move(profile)).second)
				invalid(where, "duplicate normalized profile");
		}
	}
};

std::shared_ptr<const OutputProfileSnapshot> OutputProfileRegistry::snapshot() const
{
	return current_.load();
}

OutputProfileRegistry &output_profile_registry()
{
	static OutputProfileRegistry registry;
	return registry;
}

OutputProfileLoadResult OutputProfileRegistry::reload_json(std::string_view json)
{
	try
	{
		auto next = OutputProfileParser::parse(json);
		uint32_t revision = next->revision();
		current_.store(std::move(next));
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

OutputProfileLoadResult OutputProfileRegistry::reload_file(const std::string &path)
{
	try
	{
		std::error_code error;
		if (!std::filesystem::is_regular_file(path, error) || error)
			invalid("file", "expected a readable regular configuration file");
		std::ifstream input(path, std::ios::binary);
		if (!input)
			invalid("file", "cannot open configuration");
		std::string json(OUTPUT_PROFILE_MAX_BYTES + 1, '\0');
		input.read(json.data(), (std::streamsize)json.size());
		if (input.bad())
			invalid("file", "configuration read failed");
		json.resize((size_t)input.gcount());
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
