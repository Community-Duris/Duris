#pragma once

#include "net/output_style.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <string_view>

inline constexpr size_t OUTPUT_PROFILE_MAX_BYTES = 256 * 1024;
inline constexpr size_t OUTPUT_PROFILE_MAX_RECIPES = 128;
inline constexpr size_t OUTPUT_PROFILE_MAX_DICTIONARIES = 32;
inline constexpr size_t OUTPUT_PROFILE_MAX_PROFILES = 64;
inline constexpr size_t OUTPUT_PROFILE_MAX_WORDS = 8192;
inline constexpr size_t OUTPUT_PROFILE_MAX_WORDS_PER_DICTIONARY = 2048;
inline constexpr size_t OUTPUT_PROFILE_MAX_PALETTE = 16;
inline constexpr size_t OUTPUT_PROFILE_CHANNEL_COUNT = (size_t)OutputChannel::Count;

struct OutputChannelChoice
{
	OutputChannel channel;
	std::string_view name;
};

struct OutputPaletteChoice
{
	std::string_view name;
	int attr;
};

// The parser, future preference commands, and hints share these exact catalogs.
std::span<const OutputChannelChoice> output_channel_choices();
std::span<const OutputPaletteChoice> output_palette_choices();
bool parse_output_channel(std::string_view name, OutputChannel &channel);
bool parse_output_palette(std::string_view name, int &attr);

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
	// Future output-driven animation advances one step per this many eligible sends.
	uint16_t step_every = 1; // 1..1024
	uint16_t width = 1; // 1..32 visible characters, clamped to the word by animation
	uint8_t chance_percent = 20; // 0..100; cosmetic hash threshold, never gameplay RNG
};

enum class OutputProfileChoice
{
	Default,
	Preserve,
	Static,
	Animated
};

// Plain recipient-owned preferences; persistence and commands belong to #283.
class OutputProfilePreferences
{
    public:
	bool motion_enabled = true;
	bool set(OutputChannel channel, OutputProfileChoice choice);
	bool reset(OutputChannel channel);
	OutputProfileChoice get(OutputChannel channel) const;

    private:
	std::array<OutputProfileChoice, OUTPUT_PROFILE_CHANNEL_COUNT> choices_{};
};

struct ResolvedOutputProfile;

class OutputProfileSnapshot
{
    public:
	uint32_t revision() const { return revision_; }
	const OutputStyleRecipe *recipe(std::string_view name) const;
	const OutputStyleRecipe *word_recipe(OutputChannel channel, std::string_view word) const;

    private:
	struct Profile
	{
		OutputPolicy policy = OutputPolicy::Preserve;
		std::string dictionary;
		int base_attr = 0;
		int sender_attr = 0;
		int entity_attr = 0;
	};
	uint32_t revision_ = 0;
	std::map<std::string, OutputStyleRecipe, std::less<>> recipes_;
	struct Dictionary
	{
		std::map<std::string, std::string, std::less<>> words;
		WordColorDictionary stable_words;
	};
	std::map<std::string, Dictionary, std::less<>> dictionaries_;
	std::map<std::string, Profile, std::less<>> profiles_;
	std::array<std::string, OUTPUT_PROFILE_CHANNEL_COUNT> channels_{};
	const Profile *profile(OutputChannel channel) const;
	friend class OutputProfileParser;
	friend struct ResolvedOutputProfile;
	friend ResolvedOutputProfile
	resolve_output_profile(std::shared_ptr<const OutputProfileSnapshot>, OutputChannel,
			       OutputPolicy, const OutputProfilePreferences &);
};

struct ResolvedOutputProfile
{
	OutputContext context;
	int sender_attr = 0;
	int entity_attr = 0;
	// Returned metadata is borrowed from context.snapshot_owner, retained by this value.
	const OutputStyleRecipe *word_recipe(std::string_view word) const;
};

// Preserve is an absolute caller veto. Otherwise choose recipient override or
// server profile, then demote Animated to Static if motion is disabled.
// Missing channels/configurations always resolve to Preserve.
ResolvedOutputProfile resolve_output_profile(std::shared_ptr<const OutputProfileSnapshot> snapshot,
					     OutputChannel channel, OutputPolicy caller_policy,
					     const OutputProfilePreferences &preferences = {});

struct OutputProfileLoadResult
{
	bool ok = false;
	uint32_t revision = 0;
	std::string diagnostic;
};

class OutputProfileRegistry
{
    public:
	std::shared_ptr<const OutputProfileSnapshot> snapshot() const;
	// Explicit initialization/reload boundary only. Neither is called by rendering.
	OutputProfileLoadResult reload_json(std::string_view json);
	OutputProfileLoadResult reload_file(const std::string &path);

    private:
	std::atomic<std::shared_ptr<const OutputProfileSnapshot>> current_{};
};

// Boot/reload service owns publication; eligible output only borrows a snapshot.
OutputProfileRegistry &output_profile_registry();
