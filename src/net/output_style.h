#pragma once

#include "net/ansi.h"
#include "net/output_channel.h"
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string_view>

class OutputProfileSnapshot;

enum class OutputPolicy
{
	Preserve,
	Static,
	Animated
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

inline constexpr size_t OUTPUT_PROFILE_MAX_PALETTE = 16;

enum class OutputRecipeKind
{
	Solid,
	Flow,
	Shimmer,
	Flicker,
	Pulse,
	Glint
};

struct OutputStyleRecipe
{
	OutputRecipeKind kind = OutputRecipeKind::Solid;
	std::array<int, OUTPUT_PROFILE_MAX_PALETTE> palette{};
	size_t palette_size = 0;
	size_t stable_index = 0;
	uint16_t step_every = 1; // 1..1024 eligible sends per phase step
	uint16_t width = 1; // 1..32 visible characters, clamped to the word
	uint8_t chance_percent = 20; // 0..100; cosmetic hash, never gameplay RNG
};

// Immutable recipe pointers owned by the same snapshot as the dictionary.
using WordRecipeDictionary = std::map<std::string, const OutputStyleRecipe *, std::less<>>;

struct OutputContext
{
	OutputChannel channel = OutputChannel::Unspecified;
	OutputPolicy policy = OutputPolicy::Preserve;
	const WordColorDictionary *words = nullptr;
	int base_attr = 0;
	std::span<const OutputStyleSpan> spans{};
	// Registry contexts retain the immutable dictionary through copies and reloads.
	// Hand-built contexts may continue to borrow a caller-owned dictionary.
	std::shared_ptr<const OutputProfileSnapshot> snapshot_owner{};
	const WordRecipeDictionary *recipes = nullptr;
	// Pure rendering accepts an explicit frame. send_to_char supplies the receiving
	// connection's channel sequence; replay never calls the renderer with recipes.
	uint64_t sequence = 0;
	// Explicit adoption only. The delivery boundary resolves this recipient's
	// profile; pure rendering never reads player state.
	bool resolve_recipient_preferences = false;
	// Optional original template for logging and serializer/pager fallback. Borrowed
	// only until send_to_char returns; queues always own their frozen byte copies.
	const char *original_message = nullptr;
};

inline OutputContext recipient_output_context(OutputChannel channel,
					      OutputPolicy policy = OutputPolicy::Static)
{
	OutputContext context;
	context.channel = channel;
	context.policy = policy;
	context.resolve_recipient_preferences = true;
	return context;
}

// Pure transformation. Existing attributes and protected words are never erased.
// Invalid bounds/styles leave the entire input unchanged. No per-word match cap.
AnsiString style_dictionary_words(const AnsiString &input, const WordColorDictionary &words,
				  std::span<const AnsiStyleSpan> spans = {}, int base_attr = 0);

// Freeze markup only if both legacy serializers can emit every character and
// the caller's remaining pager capacity admits it. false means use original bytes.
// The output is assigned only on success, so it can also own the input bytes.
// animated_match is true only for an accepted frame with an eligible moving recipe.
// This pure function does not advance sequences, even when a frame is rejected.
bool render_output_message(const char *message, const OutputContext &context, std::string &rendered,
			   size_t capacity = MAX_STRING_LENGTH - 1, bool *animated_match = nullptr);

// Recheck a frozen page after accumulation, using its actual markup byte length
// as well as terminal expansion and snoop overhead. This does not apply styling.
bool output_message_fits_serializers(std::string_view message);
