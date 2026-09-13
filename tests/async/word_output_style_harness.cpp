#include "net/output_style.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}

static const int blue = ATTR_FG(25), green = ATTR_FG(18), white = ATTR_FG(31);
static const WordColorDictionary words = { { "water", blue },	{ "forest", green },
					   { "water's", blue }, { "x_2", green },
					   { "r", green },	{ "snow", white } };

static std::string plain(const AnsiString &text)
{
	char buffer[MAX_STRING_LENGTH];
	text.plain(buffer);
	return buffer;
}

static void expect(const char *source, const char *expected)
{
	AnsiString input(source);
	auto result = style_dictionary_words(input, words);
	if (result != AnsiString(expected))
	{
		fprintf(stderr, "unexpected styling: %s\n", source);
		std::abort();
	}
	assert(plain(result) == plain(input));
	assert(style_dictionary_words(result, words) == result);
}

int main()
{
	expect("water FOREST Water", "&+Bwater&n &+gFOREST&n &+BWater&n");
	expect("&+rwater&n forest", "&+rwater&n &+gforest&n");
	expect("wa&+rte&nr water", "wa&+rte&nr &+Bwater&n");
	expect("&-bwater &nwater", "&-bwater &n&+Bwater&n");
	expect("&+wwater &+Wforest &nwater", "&+wwater &+Wforest &n&+Bwater&n");
	expect("wa&nter &Nforest", "&+Bwater&n &+gforest&n");
	expect("waterfall underwater water2 2water water_ _water",
	       "waterfall underwater water2 2water water_ _water");
	expect("(water),forest-water 'water' water's water'side x_2",
	       "(&+Bwater&n),&+gforest&n-&+Bwater&n '&+Bwater&n' &+Bwater's&n water'side &+gx_2&n");
	expect("water''forest", "&+Bwater&n''&+gforest&n");
	expect("éwater wateré water\xcc\x81 water—forest 水water water’s",
	       "éwater wateré water\xcc\x81 water—forest 水water water’s");
	expect("water\r\nforest\n&+rsnow\nwater", "&+Bwater&n\n&+gforest&n\n&+rsnow&n\n&+Bwater&n");
	expect("", "");
	expect("unchanged", "unchanged");
	for (const char *source : { "&", "&+", "&-", "&=", "&=r", "&+?", "&-?", "&=r?" })
	{
		AnsiString input(source);
		assert(plain(style_dictionary_words(input, words)) == plain(input));
	}

	// An authored or entity span touching one character protects the whole token.
	AnsiString input("water forest water");
	AnsiStyleSpan spans[] = { { 1, 2, StyleOrigin::Authored, 0 },
				  { 6, 12, StyleOrigin::Sender, white },
				  { 0, 18, StyleOrigin::ChannelBase, ATTR_FG(22) } };
	auto styled = style_dictionary_words(input, words, spans);
	assert(styled.attr(0) == ATTR_FG(22) && styled.attr(1) == 0);
	assert(styled.attr(6) == white && styled.attr(13) == blue);
	assert(style_dictionary_words(styled, words, spans) == styled);
	assert(input == AnsiString("water forest water"));
	AnsiStyleSpan invalid[] = { { 0, 100, StyleOrigin::Authored, 0 } };
	assert(style_dictionary_words(input, words, invalid) == input);
	invalid[0] = { 3, 2, StyleOrigin::Entity, 0 };
	assert(style_dictionary_words(input, words, invalid) == input);
	invalid[0] = { 0, 1, StyleOrigin::Sender, 1 };
	assert(style_dictionary_words(input, words, invalid) == input);
	invalid[0] = { 0, 1, static_cast<StyleOrigin>(99), 0 };
	assert(style_dictionary_words(input, words, invalid) == input);
	assert(style_dictionary_words(input, words, {}, ATTR_BG(18)) == input);
	assert(style_dictionary_words(input, words, {}, (int)(0x80000000u | blue)) == input);
	WordColorDictionary bad = { { "water", 1 }, { "forest", ATTR_BG(18) } };
	assert(style_dictionary_words(input, bad) == input);

	OutputContext context{ OutputChannel::RoomDescription, OutputPolicy::Static, &words };
	std::string frozen;
	for (const char *source : { "water", "&nwater&N", "water\r\nforest", "water &+? forest",
				    "water && forest", "water &", "water &+", "water &=r" })
	{
		assert(render_output_message(source, context, frozen));
		assert(AnsiString(frozen.c_str()) ==
		       style_dictionary_words(AnsiString(source), words));
		std::string again;
		assert(!render_output_message(frozen.c_str(), context, again));
	}
	for (const char *source : { "", "nothing", "&+rwater", "wa&+wter", "\x1b[31m water" })
		assert(!render_output_message(source, context, frozen));
	assert(!render_output_message(nullptr, context, frozen));
	context.policy = OutputPolicy::Preserve;
	frozen = "unchanged output";
	assert(!render_output_message("&Nwater\r\n", context, frozen) &&
	       frozen == "unchanged output");
	context.policy = OutputPolicy::Animated;
	assert(render_output_message("water", context, frozen));
	assert(AnsiString(frozen.c_str()) == AnsiString("&+Bwater&n"));
	context.policy = OutputPolicy::Static;
	assert(!render_output_message("water", context, frozen, 5));
	frozen = "water";
	assert(render_output_message(frozen.c_str(), context, frozen));
	assert(frozen == "&+Bwater&n");
	assert(!render_output_message(frozen.c_str(), context, frozen) && frozen == "&+Bwater&n");

	// Byte spans are mapped after markup/Unicode decoding, never into a UTF-8 tail.
	OutputStyleSpan bytes[] = { { 7, 8, StyleOrigin::Entity, white } };
	context.spans = bytes;
	assert(render_output_message("&n雪 water forest", context, frozen));
	assert(AnsiString(frozen.c_str()) == AnsiString("雪 w&+Wa&nter &+gforest&n"));
	bytes[0] = { 3, 3, StyleOrigin::Authored, 0 };
	assert(render_output_message("&n雪 water", context, frozen));
	assert(AnsiString(frozen.c_str()) == AnsiString("雪 &+Bwater&n"));
	context.spans = {};

	// Many matches, literal ampersands, multi-byte glyphs, newlines and backgrounds
	// exercise both expansion thresholds. Accepted output must be complete and exact.
	for (const char *fragment :
	     { "water ", "water & ", "water\n", "water 雪 ", "&=CWx&n water " })
	{
		bool accepted = false, rejected = false;
		std::string source;
		while (source.size() + strlen(fragment) < MAX_STRING_LENGTH)
		{
			source += fragment;
			if (source.size() % 101 > strlen(fragment) && source.size() < 65000)
				continue;
			if (!render_output_message(source.c_str(), context, frozen))
			{
				rejected = true;
				continue;
			}
			accepted = true;
			auto wanted = style_dictionary_words(AnsiString(source.c_str()), words);
			assert(AnsiString(frozen.c_str()) == wanted);
			for (int mode : { TL_BLINK, TL_UNDERLINE, TL_BRIGHT_BG })
			{
				struct
				{
					char data[MAX_STRING_LENGTH];
					char guard[16];
				} output{};
				memset(output.guard, '!', sizeof output.guard);
				wanted.term(output.data, mode);
				for (char c : output.guard)
					assert(c == '!');
				// Strip SGR only, retaining exact UTF-8 and normalized CRLF.
				std::string terminal_plain;
				for (const char *p = output.data; *p; ++p)
				{
					if (*p == '\x1b')
					{
						while (*p && *p != 'm')
							++p;
						assert(*p);
					}
					else if (*p != '\r')
						terminal_plain += *p;
				}
				assert(terminal_plain == plain(wanted));
				assert(strlen(output.data) < MAX_STRING_LENGTH - 64);
			}
		}
		assert(accepted && rejected);
	}
	puts("Word renderer: styles, provenance, boundaries, round trips and expansion limits passed");
}
