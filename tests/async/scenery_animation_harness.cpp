#include "net/output_profiles.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>

void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}

static std::string plain(const AnsiString &text)
{
	char buffer[MAX_STRING_LENGTH];
	text.plain(buffer);
	return buffer;
}

static std::string frame(OutputContext context, const char *message, uint64_t sequence,
			 bool expected_match = true)
{
	context.sequence = sequence;
	std::string rendered;
	bool matched = false;
	bool changed =
		render_output_message(message, context, rendered, MAX_STRING_LENGTH - 1, &matched);
	assert(matched == expected_match);
	if (!changed)
		rendered = message;
	assert(plain(AnsiString(rendered.c_str())) == plain(AnsiString(message)));
	return rendered;
}

int main(int argc, char **argv)
{
	assert(argc == 2 || argc == 3);
	OutputProfileRegistry registry;
	assert(registry.reload_file(argv[1]).ok);
	auto selected = resolve_output_profile(registry.snapshot(), OutputChannel::RoomDescription,
					       OutputPolicy::Animated);
	auto context = selected.context;
	assert(context.words && context.words->size() == 64);
	assert(context.recipes && context.recipes->size() == 64);
	if (argc == 3 && std::string_view(argv[2]) == "--preview")
	{
		bool first_fixture = true;
		for (const auto &[name, message] :
		     std::initializer_list<std::pair<const char *, const char *>>{
			     { "Forest", "The forest shivers; moss and vines cover the trees." },
			     { "River", "Water from the river flows into the lake and ocean." },
			     { "Volcanic", "Fire and lava surround the arcane portal." },
			     { "Ice", "Ice and frost cover frozen bones in the snow." },
			     { "Authored", "&+rThe forest and river burn with magic.&n" },
			     { "Partial",
			       "Wa&+rte&nr feeds the forest. &+gMoss&n covers bones." } })
		{
			if (!first_fixture)
				std::cout << '\n';
			first_fixture = false;
			std::cout << name << "\nSource: " << message << '\n';
			OutputContext stable = context;
			stable.policy = OutputPolicy::Static;
			std::cout << "Static: " << frame(stable, message, 0, false) << '\n';
			for (uint64_t sequence = 0; sequence < 4; ++sequence)
				std::cout << "Frame " << sequence << ": "
					  << frame(context, message, sequence,
						   std::string_view(name) != "Authored")
					  << '\n';
		}
		return 0;
	}

	// Audit mode uses the production ANSI parser and static word renderer. The
	// input is one room description per NUL-delimited record, without player data.
	if (argc == 3 && std::string_view(argv[2]) == "--audit")
	{
		std::string description;
		while (std::getline(std::cin, description, '\0'))
		{
			AnsiString original(description.c_str());
			auto styled = style_dictionary_words(original, *context.words);
			size_t matches = 0;
			bool inside = false;
			for (size_t i = 0; i < original.size(); ++i)
			{
				bool changed = styled[i] != original[i];
				if (changed && !inside)
					++matches;
				inside = changed;
			}
			std::cout << matches << '\n';
		}
		return 0;
	}

	// Every exact key styles, with no substring/stemming or occurrence cap.
	for (const auto &[word, attr] : *context.words)
	{
		OutputContext stable = context;
		stable.policy = OutputPolicy::Static;
		auto result = AnsiString(frame(stable, word.c_str(), 999, false).c_str());
		for (auto character : result)
			assert(GET_ATTR(character) == attr);
		assert(frame(context, ("prefix" + word).c_str(), 0, false) == "prefix" + word);
	}
	assert(frame(context, "water's underwater forested WATER2", 0, false) ==
	       "water's underwater forested WATER2");
	assert(frame(context, "&+rwater forest&n", 0, false) == "&+rwater forest&n");
	assert(frame(context, "wa&+rte&nr fo&-gre&nst", 0, false) == "wa&+rte&nr fo&-gre&nst");
	OutputStyleSpan protected_word{ 0, 1, StyleOrigin::Authored, 0 };
	context.spans = std::span(&protected_word, 1);
	assert(frame(context, "water", 0, false) == "water");
	context.spans = {};

	for (const char *word : { "water", "forest", "fire", "ice", "magic", "waterfalls", "sea" })
	{
		std::set<std::string> frames;
		for (uint64_t i = 0; i < 64; ++i)
			frames.insert(frame(context, word, i));
		assert(frames.size() > 1);
	}
	for (uint64_t sequence = 0; sequence < 10; ++sequence)
	{
		AnsiString first(frame(context, "water", sequence).c_str());
		AnsiString next(frame(context, "water", sequence + 1).c_str());
		for (size_t i = 0; i < first.size(); ++i)
			assert(GET_ATTR(first[i]) == GET_ATTR(next[(i + 1) % first.size()]));
		AnsiString repeated(frame(context, "water WATER\nwater", sequence).c_str());
		for (size_t i = 0; i < first.size(); ++i)
		{
			assert(GET_ATTR(first[i]) == GET_ATTR(repeated[i]));
			assert(GET_ATTR(first[i]) == GET_ATTR(repeated[i + 6]));
			assert(GET_ATTR(first[i]) == GET_ATTR(repeated[i + 12]));
		}
		assert(!GET_ATTR(repeated[11]));
	}
	assert(frame(context, "water", 0) == frame(context, "water", 5));
	assert(frame(context, "ocean", 0) == frame(context, "ocean", 1));
	assert(frame(context, "ocean", 1) != frame(context, "ocean", 2));

	OutputProfilePreferences preferences;
	preferences.motion_enabled = false;
	auto still = resolve_output_profile(registry.snapshot(), OutputChannel::RoomDescription,
					    OutputPolicy::Animated, preferences)
			     .context;
	assert(still.policy == OutputPolicy::Static);
	assert(frame(still, "water forest fire ice magic", 0, false) ==
	       frame(still, "water forest fire ice magic", 123, false));
	assert(frame(still, "water", 0, false) == "&+Bwater&n");
	assert(frame(context, "snow blood bone poison", 0, false) ==
	       frame(context, "snow blood bone poison", 55, false));

	// Borrowed contexts tolerate short/long keys, oversized widths, sequence wrap,
	// and invalid recipes without indexing invalid arrays or dividing by zero.
	OutputStyleRecipe recipe = *selected.word_recipe("water");
	recipe.width = 32;
	std::string long_word(64, 'x');
	WordColorDictionary words{ { "x", ATTR_FG(25) }, { long_word, ATTR_FG(25) } };
	WordRecipeDictionary recipes{ { "x", &recipe }, { long_word, &recipe } };
	OutputContext borrowed{ OutputChannel::RoomDescription, OutputPolicy::Animated, &words };
	borrowed.recipes = &recipes;
	for (auto sequence : { uint64_t(0), uint64_t(1), std::numeric_limits<uint64_t>::max() })
	{
		frame(borrowed, "x", sequence);
		frame(borrowed, long_word.c_str(), sequence);
	}
	recipe.step_every = 0;
	assert(frame(borrowed, "x", 0, false) == "&+Bx&n");
	recipe.step_every = 1;
	recipe.palette_size = recipe.palette.size() + 1;
	assert(frame(borrowed, "x", 0, false) == "&+Bx&n");

	std::string rendered = "unchanged";
	bool matched = true;
	assert(!render_output_message("water", context, rendered, 5, &matched));
	assert(!matched && rendered == "unchanged");
	std::string large;
	for (int i = 0; i < 8000; ++i)
		large += "water ";
	assert(!render_output_message(large.c_str(), context, rendered, MAX_STRING_LENGTH - 1,
				      &matched));
	assert(!matched && rendered == "unchanged");
	large.clear();
	for (int i = 0; i < 300; ++i)
		large += "water ";
	AnsiString all(frame(context, large.c_str(), 0).c_str());
	for (size_t i = 0; i < all.size(); ++i)
		assert((GET_CHAR(all[i]) == ' ') == !GET_ATTR(all[i]));

	// Drawing frames must not consume libc/game randomness.
	std::srand(147);
	int expected_random = std::rand();
	std::srand(147);
	frame(context, "forest water fire ice magic", 42);
	assert(std::rand() == expected_random);

	// Retaining an animated context keeps recipe pointers alive through publication
	// and destruction as well as the precomputed static dictionary.
	OutputContext retained;
	{
		OutputProfileRegistry temporary;
		assert(temporary.reload_file(argv[1]).ok);
		retained = resolve_output_profile(temporary.snapshot(),
						  OutputChannel::RoomDescription,
						  OutputPolicy::Animated)
				   .context;
	}
	assert(!registry.reload_json("{}").ok);
	assert(frame(retained, "water forest", 7) == frame(context, "water forest", 7));
	std::cout
		<< "Scenery animation: palette, progression, protection, static, bounds, lifetime and RNG passed\n";
}
