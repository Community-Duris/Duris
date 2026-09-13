#include "net/output_profiles.h"
#include <cjson/cJSON.h>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <thread>

void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}

using Json = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>;
static cJSON *at(cJSON *object, const char *key)
{
	auto found = cJSON_GetObjectItemCaseSensitive(object, key);
	assert(found);
	return found;
}
static std::string encode(cJSON *value)
{
	char *bytes = cJSON_PrintUnformatted(value);
	assert(bytes);
	std::string result(bytes);
	cJSON_free(bytes);
	return result;
}
static std::string changed(const std::string &source, const std::function<void(cJSON *)> &change)
{
	Json root(cJSON_Parse(source.c_str()), cJSON_Delete);
	assert(root);
	change(root.get());
	return encode(root.get());
}
static void replace(cJSON *object, const char *key, cJSON *value)
{
	assert(cJSON_ReplaceItemInObjectCaseSensitive(object, key, value));
}
static std::string display(const OutputContext &context, const char *message)
{
	std::string result;
	return render_output_message(message, context, result) ? result : message;
}

int main(int argc, char **argv)
{
	assert(argc == 3);
	std::ifstream input(argv[1]);
	std::string sample((std::istreambuf_iterator<char>(input)), {});
	assert(!sample.empty());
	OutputProfileRegistry registry;
	assert(!registry.snapshot());
	assert(!registry.reload_json("{}").ok && !registry.snapshot());
	auto loaded = registry.reload_json(sample);
	assert(loaded.ok && loaded.revision == 1 && loaded.diagnostic.empty());
	auto original = registry.snapshot();
	assert(original && original->revision() == 1);
	std::set<int> channel_ids, palette_attributes;
	for (const auto &choice : output_channel_choices())
	{
		OutputChannel channel = OutputChannel::Unspecified;
		assert(parse_output_channel(choice.name, channel) && channel == choice.channel);
		assert(channel_ids.insert((int)channel).second);
	}
	assert(channel_ids.size() == OUTPUT_PROFILE_CHANNEL_COUNT - 1);
	for (const auto &choice : output_palette_choices())
	{
		int attr = 0;
		assert(parse_output_palette(choice.name, attr) && attr == choice.attr);
		assert(GET_FG(attr) > 16 && attr == ATTR_FG(GET_FG(attr)));
		assert(palette_attributes.insert(attr).second);
	}
	assert(palette_attributes.size() == 15);
	OutputChannel parsed;
	int attr = 123;
	assert(parse_output_channel("ROOM.DESCRIPTION", parsed) &&
	       parsed == OutputChannel::RoomDescription);
	assert(!parse_output_channel("room.unknown", parsed));
	assert(!parse_output_palette("black", attr) && attr == 123);
	assert(!parse_output_palette("&+B", attr));
	assert(parse_output_palette("BRIGHT_BLUE", attr) && attr == ATTR_FG(25));

	const auto channel = OutputChannel::RoomDescription;
	OutputProfilePreferences preferences, another_player;
	auto resolve = [&](OutputPolicy caller = OutputPolicy::Static)
	{ return resolve_output_profile(registry.snapshot(), channel, caller, preferences); };
	assert(resolve().context.policy == OutputPolicy::Animated);
	assert(display(resolve().context, "Water waters water's waterfall") ==
	       "&+BWater&n &+Bwaters&n &+Bwater's&n waterfall");
	assert(display(resolve().context, "&+rwater&n wa&+wter") == "&+rwater&n wa&+wter");
	assert(resolve(OutputPolicy::Preserve).context.policy == OutputPolicy::Preserve);
	assert(resolve(static_cast<OutputPolicy>(99)).context.policy == OutputPolicy::Preserve);
	assert(resolve_output_profile(nullptr, channel, OutputPolicy::Animated).context.policy ==
	       OutputPolicy::Preserve);
	assert(resolve_output_profile(original, OutputChannel::Unspecified, OutputPolicy::Animated)
		       .context.policy == OutputPolicy::Preserve);
	assert(resolve_output_profile(original, static_cast<OutputChannel>(999),
				      OutputPolicy::Animated)
		       .context.policy == OutputPolicy::Preserve);
	assert(resolve_output_profile(original, OutputChannel::Weather, OutputPolicy::Animated)
		       .context.policy == OutputPolicy::Preserve);
	assert(preferences.set(channel, OutputProfileChoice::Preserve));
	assert(resolve().context.policy == OutputPolicy::Preserve);
	assert(preferences.set(channel, OutputProfileChoice::Static));
	assert(resolve().context.policy == OutputPolicy::Static);
	assert(preferences.set(channel, OutputProfileChoice::Animated));
	preferences.motion_enabled = false;
	assert(resolve().context.policy == OutputPolicy::Static);
	assert(resolve_output_profile(original, channel, OutputPolicy::Static, another_player)
		       .context.policy == OutputPolicy::Animated);
	assert(preferences.reset(channel));
	assert(resolve().context.policy == OutputPolicy::Static);
	preferences.motion_enabled = true;
	assert(resolve().context.policy == OutputPolicy::Animated);
	assert(!preferences.set(channel, static_cast<OutputProfileChoice>(99)));
	assert(!preferences.reset(OutputChannel::Count));
	assert(!preferences.set(static_cast<OutputChannel>(-1), OutputProfileChoice::Static));

	// Recipient selection can override a server Preserve default, but never the
	// caller's Preserve veto. Reset really returns to the configured server default.
	assert(preferences.set(OutputChannel::ChatSay, OutputProfileChoice::Static));
	auto conversation = resolve_output_profile(original, OutputChannel::ChatSay,
						   OutputPolicy::Static, preferences);
	assert(conversation.context.policy == OutputPolicy::Static);
	assert(conversation.sender_attr == ATTR_FG(27) && conversation.entity_attr == ATTR_FG(31));
	OutputStyleSpan spans[] = { { 0, 5, StyleOrigin::Entity, conversation.entity_attr },
				    { 6, 11, StyleOrigin::Authored, 0 } };
	conversation.context.spans = spans;
	assert(display(conversation.context, "water water water") == "&+Wwater&n water &+Bwater&n");
	assert(resolve_output_profile(original, OutputChannel::ChatSay, OutputPolicy::Preserve,
				      preferences)
		       .context.policy == OutputPolicy::Preserve);
	assert(preferences.reset(OutputChannel::ChatSay));
	assert(resolve_output_profile(original, OutputChannel::ChatSay, OutputPolicy::Static,
				      preferences)
		       .context.policy == OutputPolicy::Preserve);
	assert(display(resolve_output_profile(original, OutputChannel::RoomTitle,
					      OutputPolicy::Static)
			       .context,
		       "An old forest") == "&+GAn old forest&n");

	std::set<OutputRecipeKind> kinds;
	for (const char *name : { "river", "leaves", "flames", "ice", "magic", "snow" })
	{
		auto recipe = original->recipe(name);
		assert(recipe && recipe->palette_size &&
		       recipe->stable_index < recipe->palette_size);
		kinds.insert(recipe->kind);
	}
	assert(kinds.size() == 6 && !original->recipe("missing"));
	assert(resolve().word_recipe("water")->kind == OutputRecipeKind::Flow);
	assert(resolve().word_recipe("water")->width == 2);
	assert(!resolve().word_recipe("waterfall"));

	auto reject = [&](const std::string &candidate)
	{
		auto before = registry.snapshot();
		auto result = registry.reload_json(candidate);
		assert(!result.ok && !result.diagnostic.empty() &&
		       result.revision == before->revision());
		assert(registry.snapshot() == before);
		assert(display(resolve().context, "water") == "&+Bwater&n");
	};
	reject("");
	reject(sample + "garbage");
	reject(sample + std::string(1, '\0'));
	reject(std::string(13, '[') + std::string(13, ']'));
	for (const char *key :
	     { "version", "revision", "recipes", "dictionaries", "profiles", "channels" })
		reject(changed(sample, [=](cJSON *root)
			       { cJSON_DeleteItemFromObjectCaseSensitive(root, key); }));
	reject(changed(sample,
		       [](cJSON *root) { replace(root, "version", cJSON_CreateNumber(2)); }));
	for (double revision : { 0.0, -1.0, 1.5, 4294967296.0 })
		reject(changed(sample, [=](cJSON *root)
			       { replace(root, "revision", cJSON_CreateNumber(revision)); }));
	reject(changed(sample, [](cJSON *root) { cJSON_AddNumberToObject(root, "version", 1); }));
	reject(changed(sample, [](cJSON *root) { cJSON_AddBoolToObject(root, "surprise", true); }));
	for (const char *key : { "", "WATER", "water fall", "éwater", "water''s" })
		reject(changed(sample,
			       [=](cJSON *root) {
				       cJSON_AddStringToObject(at(at(root, "dictionaries"),
								  "terrain"),
							       key, "river");
			       }));
	reject(changed(sample,
		       [](cJSON *root) {
			       cJSON_AddStringToObject(at(at(root, "dictionaries"), "terrain"),
						       "lake", "missing");
		       }));
	reject(changed(
		sample, [](cJSON *root)
		{ cJSON_AddStringToObject(at(root, "channels"), "room.missing", "scenery"); }));
	reject(changed(
		sample, [](cJSON *root)
		{ cJSON_AddStringToObject(at(root, "channels"), "ROOM.DESCRIPTION", "scenery"); }));
	reject(changed(sample,
		       [](cJSON *root) {
			       replace(at(root, "channels"), "room.description",
				       cJSON_CreateString("missing"));
		       }));
	reject(changed(sample,
		       [](cJSON *root) {
			       replace(at(at(root, "profiles"), "scenery"), "dictionary",
				       cJSON_CreateString("missing"));
		       }));
	reject(changed(sample,
		       [](cJSON *root)
		       {
			       cJSON_AddItemToObject(
				       at(root, "profiles"), "SCENERY",
				       cJSON_Duplicate(at(at(root, "profiles"), "scenery"), true));
		       }));
	reject(changed(sample,
		       [](cJSON *root) {
			       replace(at(at(root, "recipes"), "river"), "kind",
				       cJSON_CreateString("unknown"));
		       }));
	for (const char *color : { "black", "blink", "background_blue", "&+B", "invisible" })
		reject(changed(sample,
			       [=](cJSON *root)
			       {
				       cJSON_ReplaceItemInArray(at(at(at(root, "recipes"), "river"),
								   "palette"),
								0, cJSON_CreateString(color));
			       }));
	reject(changed(sample,
		       [](cJSON *root)
		       {
			       cJSON_ReplaceItemInArray(at(at(at(root, "recipes"), "river"),
							   "palette"),
							0, cJSON_CreateNumber(25));
		       }));
	for (auto [key, value] : { std::pair{ "step_every", 0 },
				   { "step_every", 1025 },
				   { "width", 0 },
				   { "width", 33 },
				   { "chance_percent", 101 },
				   { "chance_percent", -1 },
				   { "stable_index", 4 } })
		reject(changed(sample,
			       [=](cJSON *root)
			       {
				       auto river = at(at(root, "recipes"), "river");
				       cJSON_DeleteItemFromObjectCaseSensitive(river, key);
				       cJSON_AddNumberToObject(river, key, value);
			       }));
	reject(changed(sample, [](cJSON *root)
		       { cJSON_AddNumberToObject(at(at(root, "recipes"), "snow"), "width", 1); }));
	reject(changed(
		sample, [](cJSON *root)
		{ replace(at(at(root, "recipes"), "river"), "palette", cJSON_CreateArray()); }));
	reject(changed(sample,
		       [](cJSON *root)
		       {
			       auto palette = at(at(at(root, "recipes"), "river"), "palette");
			       while (cJSON_GetArraySize(palette) <=
				      (int)OUTPUT_PROFILE_MAX_PALETTE)
				       cJSON_AddItemToArray(palette, cJSON_CreateString("blue"));
		       }));
	std::string nul_escape = sample;
	nul_escape.replace(nul_escape.find("\"water\""), 7, "\"water\\u0000ignored\"");
	reject(nul_escape);

	for (const char *raw : { "1e999", "0.5", "true", "null" })
		reject(changed(sample,
			       [=](cJSON *root) {
				       replace(at(at(root, "recipes"), "river"), "step_every",
					       cJSON_CreateRaw(raw));
			       }));
	reject(changed(sample,
		       [](cJSON *root)
		       {
			       auto recipes = at(root, "recipes");
			       for (int i = 0;
				    cJSON_GetArraySize(recipes) <= (int)OUTPUT_PROFILE_MAX_RECIPES;
				    ++i)
				       cJSON_AddItemToObject(
					       recipes, ("recipe" + std::to_string(i)).c_str(),
					       cJSON_Duplicate(at(recipes, "river"), true));
		       }));
	reject(changed(sample,
		       [](cJSON *root)
		       {
			       auto dictionaries = at(root, "dictionaries");
			       for (int i = 0; cJSON_GetArraySize(dictionaries) <=
					       (int)OUTPUT_PROFILE_MAX_DICTIONARIES;
				    ++i)
				       cJSON_AddItemToObject(dictionaries,
							     ("words" + std::to_string(i)).c_str(),
							     cJSON_CreateObject());
		       }));
	reject(changed(sample,
		       [](cJSON *root)
		       {
			       auto profiles = at(root, "profiles");
			       for (int i = 0; cJSON_GetArraySize(profiles) <=
					       (int)OUTPUT_PROFILE_MAX_PROFILES;
				    ++i)
				       cJSON_AddItemToObject(
					       profiles, ("profile" + std::to_string(i)).c_str(),
					       cJSON_Duplicate(at(profiles, "scenery"), true));
		       }));
	reject(changed(sample,
		       [](cJSON *root)
		       {
			       auto terrain = at(at(root, "dictionaries"), "terrain");
			       for (int i = 0; cJSON_GetArraySize(terrain) <=
					       (int)OUTPUT_PROFILE_MAX_WORDS_PER_DICTIONARY;
				    ++i)
				       cJSON_AddStringToObject(
					       terrain, ("w" + std::to_string(i)).c_str(), "river");
		       }));
	auto word_limit = [&](bool overflow)
	{
		return changed(
			sample,
			[=](cJSON *root)
			{
				auto dictionaries = cJSON_CreateObject();
				replace(root, "dictionaries", dictionaries);
				for (int group = 0; group < (overflow ? 5 : 4); ++group)
				{
					auto words = cJSON_CreateObject();
					cJSON_AddItemToObject(
						dictionaries,
						group ? ("words" + std::to_string(group)).c_str() :
							"terrain",
						words);
					for (size_t i = 0;
					     i < (group == 4 ?
							  1 :
							  OUTPUT_PROFILE_MAX_WORDS_PER_DICTIONARY);
					     ++i)
						cJSON_AddStringToObject(
							words, ("w" + std::to_string(i)).c_str(),
							"river");
				}
			});
	};
	OutputProfileRegistry bounds;
	assert(bounds.reload_json(word_limit(false)).ok);
	assert(bounds.snapshot()->word_recipe(channel, "w2047"));
	reject(word_limit(true));
	std::string padded = sample;
	padded.resize(OUTPUT_PROFILE_MAX_BYTES, ' ');
	assert(bounds.reload_json(padded).ok);
	reject(padded + ' ');
	assert(bounds.reload_json(changed(sample,
					  [](cJSON *root)
					  {
						  auto routes = cJSON_CreateObject();
						  replace(root, "channels", routes);
						  for (const auto &entry : output_channel_choices())
							  cJSON_AddStringToObject(
								  routes,
								  std::string(entry.name).c_str(),
								  "scenery");
					  }))
		       .ok);
	assert(bounds.reload_json(
			     changed(sample,
				     [](cJSON *root)
				     {
					     auto terrain = at(at(root, "dictionaries"), "terrain");
					     cJSON_DeleteItemFromObjectCaseSensitive(terrain,
										     "water");
					     cJSON_AddStringToObject(terrain, "WATER", "RIVER");
					     replace(at(at(root, "profiles"), "scenery"),
						     "dictionary", cJSON_CreateString("TERRAIN"));
				     }))
		       .ok);
	assert(display(resolve_output_profile(bounds.snapshot(), channel, OutputPolicy::Static)
			       .context,
		       "water") == "&+Bwater&n");

	// Retained OutputContext owns its dictionary even after a registry is destroyed.
	OutputContext retained;
	{
		OutputProfileRegistry temporary;
		assert(temporary.reload_json(sample).ok);
		retained =
			resolve_output_profile(temporary.snapshot(), channel, OutputPolicy::Static)
				.context;
	}
	assert(display(retained, "water") == "&+Bwater&n");
	const auto path = std::filesystem::path(argv[2]) / "profiles.json";
	assert(!registry.reload_file(path.string()).ok);
	assert(!registry.reload_file(argv[2]).ok);
	{
		std::ofstream file(path);
		file << sample;
	}
	assert(registry.reload_file(path.string()).ok);
	auto file_snapshot = registry.snapshot();
	{
		std::ofstream file(path);
		file << "{invalid";
	}
	assert(!registry.reload_file(path.string()).ok && registry.snapshot() == file_snapshot);
	{
		std::ofstream file(path);
		file << padded << ' ';
	}
	assert(!registry.reload_file(path.string()).ok && registry.snapshot() == file_snapshot);
	assert(std::filesystem::remove(path));
	assert(display(resolve().context, "water") == "&+Bwater&n");

	std::string alternate = changed(
		sample,
		[](cJSON *root)
		{
			replace(root, "revision", cJSON_CreateNumber(2));
			cJSON_ReplaceItemInArray(at(at(at(root, "recipes"), "river"), "palette"), 1,
						 cJSON_CreateString("red"));
		});
	auto frozen_before_reload = resolve().context;
	assert(registry.reload_json(alternate).ok && registry.snapshot()->revision() == 2);
	assert(display(resolve().context, "water") == "&+rwater&n");
	assert(display(frozen_before_reload, "water") == "&+Bwater&n");
	// A single publisher and concurrent readers must see coherent revisions/frames.
	std::atomic<bool> stop{ false };
	std::atomic<size_t> reads{ 0 };
	std::thread reader(
		[&]
		{
			while (!stop.load())
			{
				auto snapshot = registry.snapshot();
				auto selection = resolve_output_profile(snapshot, channel,
									OutputPolicy::Static);
				assert(display(selection.context, "water") ==
				       (snapshot->revision() == 1 ? "&+Bwater&n" : "&+rwater&n"));
				++reads;
			}
		});
	while (!reads.load())
		std::this_thread::yield();
	for (int i = 0; i < 80; ++i)
		assert(registry.reload_json(i % 2 ? sample : alternate).ok);
	stop = true;
	reader.join();
	assert(reads.load());
	std::cout
		<< "Output profiles: validation bounds, precedence, protected styles, file reload, "
		   "snapshot lifetime and concurrent publication passed\n";
}
