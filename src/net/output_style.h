#pragma once

#include "net/ansi.h"
#include <map>
#include <span>

enum class OutputPolicy
{
	Preserve,
	Static,
	Animated
};

// Routing only: no channel is automatically adopted by the output queue.
// The profile registry can extend these identifiers before exposing preferences.
enum class OutputChannel
{
	Unspecified,
	RoomDescription,
	Chat,
	Combat,
	SystemFeedback
};
enum class StyleOrigin
{
	ChannelBase,
	Sender,
	Entity,
	Authored
};

// Half-open visible-character offsets into an AnsiString.
struct AnsiStyleSpan
{
	size_t begin = 0;
	size_t end = 0;
	StyleOrigin origin = StyleOrigin::Authored;
	int attr = 0;
};

// Half-open byte offsets into the completed Duris-markup message, after formatting.
struct OutputStyleSpan
{
	size_t begin = 0;
	size_t end = 0;
	StyleOrigin origin = StyleOrigin::Authored;
	int attr = 0;
};

// Lowercase ASCII whole-word keys, foreground attributes only. Construct once
// in the caller/config snapshot, then borrow for the duration of a send.
using WordColorDictionary = std::map<std::string, int, std::less<>>;

struct OutputContext
{
	OutputChannel channel = OutputChannel::Unspecified;
	OutputPolicy policy = OutputPolicy::Preserve;
	const WordColorDictionary *words = nullptr;
	int base_attr = 0;
	std::span<const OutputStyleSpan> spans{};
};

// Pure transformation. Existing attributes and protected words are never erased.
// Invalid bounds/styles leave the entire input unchanged. No per-word match cap.
AnsiString style_dictionary_words(const AnsiString &input, const WordColorDictionary &words,
				  std::span<const AnsiStyleSpan> spans = {}, int base_attr = 0);

// Freeze markup only if both legacy serializers can emit every character and
// the caller's remaining pager capacity admits it. false means use original bytes.
// The output is assigned only on success, so it can also own the input bytes.
// Animated uses the supplied fixed frame here; recipe/sequence ownership is separate.
bool render_output_message(const char *message, const OutputContext &context, std::string &rendered,
			   size_t capacity = MAX_STRING_LENGTH - 1);
