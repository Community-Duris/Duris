#include "net/output_style.h"
#include "net/unicode.h"
#include <algorithm>
#include <cstring>

namespace
{
constexpr size_t max_spans = MAX_STRING_LENGTH;

bool foreground(int attr)
{
	return !attr || (GET_FG(attr) >= 16 && attr == ATTR_FG(GET_FG(attr)));
}

// Fixed unsigned arithmetic gives identical cosmetic frames on every platform.
// Neither helper touches the game's RNG, a clock, a room ID, or message position.
uint64_t word_seed(std::string_view word)
{
	uint64_t hash = 14695981039346656037ULL;
	for (unsigned char ch : word)
		hash = (hash ^ ch) * 1099511628211ULL;
	return hash;
}

uint64_t cosmetic_hash(uint64_t value)
{
	value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
	value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
	return value ^ (value >> 31);
}

bool valid_recipe(const OutputStyleRecipe &recipe)
{
	if (!recipe.palette_size || recipe.palette_size > recipe.palette.size() ||
	    recipe.stable_index >= recipe.palette_size || !recipe.step_every ||
	    recipe.step_every > 1024 || !recipe.width || recipe.width > 32 ||
	    recipe.chance_percent > 100)
		return false;
	for (size_t i = 0; i < recipe.palette_size; ++i)
		if (!recipe.palette[i] || !foreground(recipe.palette[i]))
			return false;
	return recipe.kind >= OutputRecipeKind::Solid && recipe.kind <= OutputRecipeKind::Glint;
}

int recipe_foreground(const OutputStyleRecipe &recipe, uint64_t seed, uint64_t sequence,
		      size_t position, size_t length)
{
	size_t colors = recipe.palette_size;
	if (recipe.kind == OutputRecipeKind::Solid || colors == 1)
		return recipe.palette[recipe.stable_index];
	uint64_t phase = sequence / recipe.step_every;
	size_t crest = (seed % length + phase % length) % length;
	size_t distance = (position + length - crest) % length;
	// Leave a base-colored character on short words so a wide band still travels.
	size_t width = std::min<size_t>(recipe.width, length > 1 ? length - 1 : 1);
	size_t index = 0;
	switch (recipe.kind)
	{
	case OutputRecipeKind::Solid:
		break;
	case OutputRecipeKind::Flow:
		index = length == 1 ? (seed % colors + phase % colors) % colors :
				      (distance < width ? 1 + distance % (colors - 1) : 0);
		break;
	case OutputRecipeKind::Shimmer:
	{
		// A sparse pattern drifts through the word instead of rerolling each letter.
		uint64_t patch = cosmetic_hash(seed + (length == 1 ? phase : distance / width));
		if (patch % 100 < recipe.chance_percent)
			index = 1 + (patch / 100) % (colors - 1);
		break;
	}
	case OutputRecipeKind::Flicker:
	{
		uint64_t patch =
			cosmetic_hash(seed + phase * 0x9e3779b97f4a7c15ULL + position / width);
		if (patch % 100 < recipe.chance_percent)
			index = 1 + (patch / 100) % (colors - 1);
		break;
	}
	case OutputRecipeKind::Pulse:
	{
		size_t period = 2 * (colors - 1);
		size_t step = (seed % period + phase % period) % period;
		index = step < colors ? step : period - step;
		break;
	}
	case OutputRecipeKind::Glint:
		// Keep the gate fixed for an entire sweep, allowing a coherent moving glint.
		if (cosmetic_hash(seed + phase / length) % 100 < recipe.chance_percent &&
		    distance < width)
			index = colors - 1 - distance % (colors - 1);
		break;
	}
	return recipe.palette[index];
}

bool word_character(wchar_t ch)
{
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
	       ch == '_' || ch >= 128;
}

bool token_character(const AnsiString &text, size_t index)
{
	wchar_t ch = GET_CHAR(text[index]);
	return word_character(ch) || (ch == '\'' && index && index + 1 < text.size() &&
				      word_character(GET_CHAR(text[index - 1])) &&
				      word_character(GET_CHAR(text[index + 1])));
}

bool valid_origin(StyleOrigin origin)
{
	return origin == StyleOrigin::ChannelBase || origin == StyleOrigin::Sender ||
	       origin == StyleOrigin::Entity || origin == StyleOrigin::Authored;
}

size_t utf8_size(wchar_t ch)
{
	char encoded[5];
	char *end = encoded;
	put_utf8(end, ch);
	return end - encoded;
}

// Match the *early stopping thresholds* of ansi()/term(), not just buffer size.
// Budget the largest terminal mode, including snoop prefixes. The latter keeps
// an opted-in message safe even if a snooper attaches after it has been queued.
bool serializers_fit(const AnsiString &text)
{
	size_t markup = 0, terminal = 0, lines = 1;
	int previous_markup = 0, previous_terminal = 0;
	for (size_t i = 0; i < text.size(); ++i)
	{
		if (markup >= MAX_STRING_LENGTH - 11 || terminal >= MAX_STRING_LENGTH - 64)
			return false;
		wchar_t ch = GET_CHAR(text[i]);
		int attr = GET_ATTR(text[i]);
		if (attr != previous_markup)
		{
			markup += !attr ? 2 : (GET_FG(attr) && GET_BG(attr) ? 4 : 3);
			previous_markup = attr;
		}
		markup += utf8_size(ch);
		if (ch == '&')
			previous_markup = -1;

		if (ch == '\n')
		{
			terminal += 2 + (previous_terminal ? 3 : 0);
			previous_terminal = 0;
			if (i + 1 < text.size())
				++lines;
		}
		else if (ch != '\r')
		{
			if (attr != previous_terminal)
			{
				terminal += 4; // ESC [ 0 m
				if (GET_FG(attr))
					terminal += 3 + ((GET_FG(attr) & 8) ? 2 : 0);
				if (GET_BG(attr))
					terminal += (GET_BG(attr) & 8) ? 6 : 3;
				previous_terminal = attr;
			}
			terminal += utf8_size(ch);
		}
	}
	markup += previous_markup ? 2 : 0;
	terminal += previous_terminal ? 3 : 0;
	// Each snoop line adds seven markup bytes, at most fifteen terminal bytes.
	return markup + lines * 7 < MAX_STRING_LENGTH - 11 &&
	       terminal + lines * 15 < MAX_STRING_LENGTH - 64;
}
} // namespace

bool output_message_fits_serializers(std::string_view message)
{
	if (message.size() >= MAX_STRING_LENGTH || message.find('\0') != std::string_view::npos)
		return false;
	// Preserved sends can contain redundant markup, so budget the actual frozen
	// bytes too, rather than only the canonical markup measured by serializers_fit.
	size_t lines = 1 + std::count(message.begin(), message.end(), '\n');
	return message.size() + lines * 7 < MAX_STRING_LENGTH - 11 &&
	       serializers_fit(AnsiString(std::string(message).c_str()));
}

static AnsiString style_words(const AnsiString &input, const WordColorDictionary &words,
			      std::span<const AnsiStyleSpan> spans, int base_attr,
			      const WordRecipeDictionary *recipes, uint64_t sequence,
			      bool *animated_match)
{
	if (input.size() >= MAX_STRING_LENGTH || spans.size() > max_spans || !foreground(base_attr))
		return input;
	for (const auto &span : spans)
		if (span.begin > span.end || span.end > input.size() || !foreground(span.attr) ||
		    !valid_origin(span.origin))
			return input;

	// Difference arrays keep overlapping spans bounded by message + span length.
	std::vector<int> protection(input.size() + 1);
	for (const auto &span : spans)
		if (span.origin != StyleOrigin::ChannelBase)
		{
			++protection[span.begin];
			--protection[span.end];
		}
	for (size_t i = 1; i < protection.size(); ++i)
		protection[i] += protection[i - 1];

	AnsiString result = input;
	std::string key;
	for (size_t start = 0; start < input.size();)
	{
		if (!token_character(input, start))
		{
			++start;
			continue;
		}
		size_t end = start;
		bool eligible = true;
		key.clear();
		while (end < input.size() && token_character(input, end))
		{
			wchar_t ch = GET_CHAR(input[end]);
			if (GET_ATTR(input[end]) || protection[end] || ch >= 128)
				eligible = false;
			if (ch < 128)
				key += (char)(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
			++end;
		}
		if (eligible)
		{
			auto found = words.find(key);
			if (found != words.end() && foreground(found->second))
			{
				const OutputStyleRecipe *recipe = nullptr;
				if (recipes)
				{
					auto effect = recipes->find(key);
					if (effect != recipes->end() && effect->second &&
					    valid_recipe(*effect->second))
						recipe = effect->second;
				}
				uint64_t seed = recipe ? word_seed(key) : 0;
				if (recipe && recipe->kind != OutputRecipeKind::Solid &&
				    recipe->palette_size > 1 && animated_match)
					*animated_match = true;
				for (size_t i = start; i < end; ++i)
					result[i] |= recipe ? recipe_foreground(*recipe, seed,
										sequence, i - start,
										end - start) :
							      found->second;
			}
		}
		start = end;
	}

	// Roles win over channel spans; authored protection wins over all added styles.
	// A sweep of endpoints avoids quadratic work for many overlapping spans.
	struct Edge
	{
		size_t offset;
		size_t span;
		bool begin;
	};
	std::vector<Edge> edges;
	edges.reserve(spans.size() * 2);
	for (size_t i = 0; i < spans.size(); ++i)
	{
		edges.push_back({ spans[i].begin, i, true });
		edges.push_back({ spans[i].end, i, false });
	}
	std::sort(edges.begin(), edges.end(), [](const Edge &a, const Edge &b)
		  { return a.offset < b.offset || (a.offset == b.offset && a.begin > b.begin); });
	std::map<std::pair<int, size_t>, int> active;
	size_t edge = 0;
	for (size_t i = 0; i < result.size(); ++i)
	{
		while (edge < edges.size() && edges[edge].offset == i)
		{
			const auto &event = edges[edge++];
			const auto &span = spans[event.span];
			int priority = span.origin == StyleOrigin::Authored ?
					       2 :
					       (span.origin == StyleOrigin::ChannelBase ? 0 : 1);
			auto identity = std::make_pair(priority, event.span);
			if (event.begin)
				active[identity] = span.origin == StyleOrigin::Authored ? 0 :
											  span.attr;
			else
				active.erase(identity);
		}
		if (!GET_ATTR(result[i]) && GET_CHAR(result[i]) != '\n' &&
		    GET_CHAR(result[i]) != '\r')
			result[i] |= active.empty() ? base_attr : active.rbegin()->second;
	}
	return result;
}

AnsiString style_dictionary_words(const AnsiString &input, const WordColorDictionary &words,
				  std::span<const AnsiStyleSpan> spans, int base_attr)
{
	return style_words(input, words, spans, base_attr, nullptr, 0, nullptr);
}

bool render_output_message(const char *message, const OutputContext &context, std::string &rendered,
			   size_t capacity, bool *animated_match)
{
	if (animated_match)
		*animated_match = false;
	if (!message || context.policy == OutputPolicy::Preserve ||
	    (context.policy != OutputPolicy::Static && context.policy != OutputPolicy::Animated) ||
	    context.spans.size() > max_spans)
		return false;
	size_t length = strnlen(message, MAX_STRING_LENGTH);
	// Raw terminal control sequences have no AnsiString provenance. Preserve them.
	if (!length || length >= MAX_STRING_LENGTH || memchr(message, '\x1b', length))
		return false;
	std::vector<size_t> offsets;
	AnsiString original;
	original.set(message, context.spans.empty() ? nullptr : &offsets);
	std::vector<AnsiStyleSpan> spans;
	for (const auto &span : context.spans)
	{
		if (span.begin > span.end || span.end > length || !foreground(span.attr) ||
		    !valid_origin(span.origin))
			return false;
		if (span.begin == span.end)
			continue;
		auto first = std::lower_bound(offsets.begin(), offsets.end(), span.begin);
		auto last = std::lower_bound(offsets.begin(), offsets.end(), span.end);
		// Metadata that starts inside a UTF-8 character protects that character too.
		if (span.begin < length && IS_UTF8_TAIL((unsigned char)message[span.begin]) &&
		    first != offsets.begin())
			--first;
		spans.push_back({ (size_t)(first - offsets.begin()),
				  (size_t)(last - offsets.begin()), span.origin, span.attr });
	}
	static const WordColorDictionary empty_dictionary;
	bool matched = false;
	AnsiString styled =
		style_words(original, context.words ? *context.words : empty_dictionary, spans,
			    context.base_attr,
			    context.policy == OutputPolicy::Animated ? context.recipes : nullptr,
			    context.sequence, &matched);
	if (styled == original || !serializers_fit(styled))
		return false;
	char markup[MAX_STRING_LENGTH];
	styled.ansi(markup);
	if (strlen(markup) > capacity)
		return false;
	// Preserve canonical visible characters/attributes through literal ampersands,
	// newline resets, and any malformed source markup before freezing the message.
	if (AnsiString(markup) != styled)
		return false;
	rendered = markup;
	if (animated_match)
		*animated_match = matched;
	return true;
}
