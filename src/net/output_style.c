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

AnsiString style_dictionary_words(const AnsiString &input, const WordColorDictionary &words,
				  std::span<const AnsiStyleSpan> spans, int base_attr)
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
				for (size_t i = start; i < end; ++i)
					result[i] |= found->second;
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

bool render_output_message(const char *message, const OutputContext &context, std::string &rendered,
			   size_t capacity)
{
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
	AnsiString styled =
		style_dictionary_words(original, context.words ? *context.words : empty_dictionary,
				       spans, context.base_attr);
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
	return true;
}
