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
	OutputProfileSnapshot() = default;
	// Dictionaries borrow recipe nodes in this immutable snapshot. Share ownership
	// through the registry; copying/moving nodes would invalidate those pointers.
	OutputProfileSnapshot(const OutputProfileSnapshot &) = delete;
	OutputProfileSnapshot &operator=(const OutputProfileSnapshot &) = delete;
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
		WordRecipeDictionary recipes;
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
