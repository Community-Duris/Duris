/*
 * Configurable community spell-up command and its boot-scoped repeat job.
 *
 * This file intentionally keeps the repeat state independent of character
 * pointers.  The scheduler event is global, while players and targets are
 * re-resolved from process-local identifiers immediately before use.
 */

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

#include "core/config.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"

extern P_desc descriptor_list;

/* Most command code uses send_to_char(message, character), while this module
 * keeps its formatted-output calls in the more readable character-first
 * form.  Keep the adapter local to this translation unit. */
static void send_to_char(P_char character, const char *message)
{
	::send_to_char(message, character);
}

/* ne_event_tick is intentionally not part of the public event API.  This
 * module only uses it to make the status display honest about the next due
 * event; scheduling itself still goes through add_event(). */
extern unsigned long long ne_event_tick;

namespace
{

constexpr int COMMUNITY_SPELL_LEVEL = 61;
constexpr int COMMUNITY_MIN_INTERVAL_SECONDS = 10;
constexpr int COMMUNITY_MAX_INTERVAL_SECONDS = 60 * 60;
constexpr std::size_t COMMUNITY_MAX_DRAFTS = 128;
constexpr std::size_t COMMUNITY_MAX_TARGETS = 2048;
constexpr std::size_t COMMUNITY_TARGETS_PER_SLICE = 32;

enum class community_scope : unsigned char
{
	all,
	good,
	evil,
};

enum class caster_mode : unsigned char
{
	command_caster,
	target,
};

enum class effect_id : unsigned char
{
	bless,
	spirit_armor,
	barkskin,
	enhance_armor,
	stone_skin,
	fly,
	haste,
	strength,
	agility,
	dexterity,
	accelerated_healing,
	rest,
	regeneration,
};

constexpr std::size_t COMMUNITY_EFFECT_COUNT =
	static_cast<std::size_t>(effect_id::regeneration) + 1;
constexpr unsigned int COMMUNITY_DEFAULT_SELECTION =
	(1U << static_cast<unsigned int>(effect_id::regeneration)) - 1U;

enum class effect_outcome : unsigned char
{
	applied,
	refreshed,
	upgraded,
	unchanged,
	blocked,
	failed,
};

using spell_cast_function = void (*)(int, P_char, P_char);

struct effect_definition
{
	effect_id id;
	const char *name;
	const char *aliases[3];
	const char *description;
	int spell_type;
	bool refreshable;
	caster_mode legacy_caster;
	spell_cast_function cast;
};

static void cast_bless(int level, P_char caster, P_char target)
{
	spell_bless(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_spirit_armor(int level, P_char caster, P_char target)
{
	spell_spirit_armor(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_barkskin(int level, P_char caster, P_char target)
{
	spell_barkskin(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_enhance_armor(int level, P_char caster, P_char target)
{
	spell_enhance_armor(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_stone_skin(int level, P_char caster, P_char target)
{
	spell_stone_skin(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_fly(int level, P_char caster, P_char target)
{
	spell_fly(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_haste(int level, P_char caster, P_char target)
{
	spell_haste(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_strength(int level, P_char caster, P_char target)
{
	spell_strength(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_agility(int level, P_char caster, P_char target)
{
	spell_agility(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_dexterity(int level, P_char caster, P_char target)
{
	spell_dexterity(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_accelerated_healing(int level, P_char caster, P_char target)
{
	spell_accel_healing(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_rest(int level, P_char caster, P_char target)
{
	spell_rest(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static void cast_regeneration(int level, P_char caster, P_char target)
{
	spell_regeneration(level, caster, nullptr, SPELL_TYPE_SPELL, target, nullptr);
}

static const std::array<effect_definition, COMMUNITY_EFFECT_COUNT> EFFECTS = { {
	{ effect_id::bless,
	  "bless",
	  { "blessing", nullptr, nullptr },
	  "bless",
	  SPELL_BLESS,
	  true,
	  caster_mode::command_caster,
	  cast_bless },
	{ effect_id::spirit_armor,
	  "spirit armor",
	  { "spirit", nullptr, nullptr },
	  "spirit armor",
	  SPELL_SPIRIT_ARMOR,
	  true,
	  caster_mode::target,
	  cast_spirit_armor },
	{ effect_id::barkskin,
	  "barkskin",
	  { "bark skin", nullptr, nullptr },
	  "barkskin",
	  SPELL_BARKSKIN,
	  true,
	  caster_mode::target,
	  cast_barkskin },
	{ effect_id::enhance_armor,
	  "enhance armor",
	  { "enhance", "armor", nullptr },
	  "enhance armor",
	  SPELL_ENHANCE_ARMOR,
	  true,
	  caster_mode::target,
	  cast_enhance_armor },
	{ effect_id::stone_skin,
	  "stone skin",
	  { "stoneskin", "stone", nullptr },
	  "stone skin",
	  SPELL_STONE_SKIN,
	  false,
	  caster_mode::command_caster,
	  cast_stone_skin },
	{ effect_id::fly,
	  "fly",
	  { "flight", nullptr, nullptr },
	  "fly",
	  SPELL_FLY,
	  true,
	  caster_mode::command_caster,
	  cast_fly },
	{ effect_id::haste,
	  "haste",
	  { nullptr, nullptr, nullptr },
	  "haste",
	  SPELL_HASTE,
	  false,
	  caster_mode::command_caster,
	  cast_haste },
	{ effect_id::strength,
	  "strength",
	  { "str", nullptr, nullptr },
	  "strength",
	  SPELL_STRENGTH,
	  false,
	  caster_mode::command_caster,
	  cast_strength },
	{ effect_id::agility,
	  "agility",
	  { "agi", nullptr, nullptr },
	  "agility",
	  SPELL_AGILITY,
	  false,
	  caster_mode::command_caster,
	  cast_agility },
	{ effect_id::dexterity,
	  "dexterity",
	  { "dex", nullptr, nullptr },
	  "dexterity",
	  SPELL_DEXTERITY,
	  false,
	  caster_mode::command_caster,
	  cast_dexterity },
	{ effect_id::accelerated_healing,
	  "accelerated healing",
	  { "accelerated", "accel healing", "accel" },
	  "accelerated healing",
	  SPELL_ACCEL_HEALING,
	  true,
	  caster_mode::command_caster,
	  cast_accelerated_healing },
	{ effect_id::rest,
	  "rest",
	  { "rested", nullptr, nullptr },
	  "rest",
	  SPELL_REST,
	  true,
	  caster_mode::command_caster,
	  cast_rest },
	{ effect_id::regeneration,
	  "regeneration",
	  { "regen", nullptr, nullptr },
	  "regeneration",
	  SPELL_REGENERATION,
	  true,
	  caster_mode::command_caster,
	  cast_regeneration },
} };

struct effect_snapshot
{
	bool present;
	int duration;
	int rest_state;
};

struct effect_statistics
{
	int applied;
	int refreshed;
	int upgraded;
	int unchanged;
	int blocked;
	int failed;
};

struct run_totals
{
	int considered;
	int eligible;
	int processed;
	int skipped;
	int truncated;
	std::array<effect_statistics, COMMUNITY_EFFECT_COUNT> effects;
};

struct draft_slot
{
	bool used;
	int pid;
	uint64_t runtime_id;
	unsigned int selection;
};

struct repeat_job
{
	bool active;
	bool pass_running;
	bool event_armed;
	uint64_t id;
	uint64_t revision;
	uint64_t pass_revision;
	unsigned long long next_due_tick;
	int creator_pid;
	int last_editor_pid;
	int interval_seconds;
	int interval_pulses;
	char creator_name[64];
	char last_editor_name[64];
	char stop_reason[96];
	unsigned int selection;
	unsigned int pass_selection;
	community_scope scope;
	community_scope pass_scope;
	std::array<uint64_t, COMMUNITY_MAX_TARGETS> target_ids;
	std::size_t target_count;
	std::size_t target_index;
	run_totals pass_totals;
	run_totals last_totals;
	bool has_last_result;
	nevent_handle event_handle;
};

struct target_collection
{
	std::array<uint64_t, COMMUNITY_MAX_TARGETS> ids;
	std::size_t count;
	run_totals totals;
};

static std::array<draft_slot, COMMUNITY_MAX_DRAFTS> drafts = {};
static repeat_job job = {};
static uint64_t next_job_id = 1;

static void community_spellup_event(P_char, P_char, P_obj, void *);

static const effect_definition &effect_at(std::size_t index)
{
	return EFFECTS[index];
}

static bool selection_has(unsigned int selection, effect_id id)
{
	return (selection & (1U << static_cast<unsigned int>(id))) != 0;
}

static std::string trim_copy(std::string value)
{
	std::size_t first = 0;
	while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
		++first;
	std::size_t last = value.size();
	while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
		--last;
	return value.substr(first, last - first);
}

static std::string normalize_word(std::string value)
{
	value = trim_copy(value);
	std::string normalized;
	normalized.reserve(value.size());
	bool pending_space = false;
	for (char character : value)
	{
		const unsigned char c = static_cast<unsigned char>(character);
		if (std::isspace(c) || character == '_' || character == '-')
		{
			pending_space = !normalized.empty();
			continue;
		}
		if (pending_space && !normalized.empty() && normalized.back() != ' ')
			normalized.push_back(' ');
		pending_space = false;
		normalized.push_back(static_cast<char>(std::tolower(c)));
	}
	return trim_copy(normalized);
}

static std::string rest_after_first_word(char *argument, std::string *first_word)
{
	char word[MAX_STRING_LENGTH];
	char empty_argument[] = "";
	char *rest = one_argument(argument ? argument : empty_argument, word);
	*first_word = normalize_word(word);
	return trim_copy(rest ? std::string(rest) : std::string());
}

static const char *scope_name(community_scope scope)
{
	switch (scope)
	{
	case community_scope::good:
		return "good";
	case community_scope::evil:
		return "evil";
	case community_scope::all:
	default:
		return "all";
	}
}

static bool parse_scope(const std::string &word, community_scope *scope)
{
	const std::string normalized = normalize_word(word);
	if (normalized == "g" || normalized == "good")
	{
		*scope = community_scope::good;
		return true;
	}
	if (normalized == "e" || normalized == "evil")
	{
		*scope = community_scope::evil;
		return true;
	}
	if (normalized == "all")
	{
		*scope = community_scope::all;
		return true;
	}
	return false;
}

static void remember_name(char (&destination)[64], P_char character)
{
	std::snprintf(destination, sizeof(destination), "%s",
		      character && GET_NAME(character) ? GET_NAME(character) : "unknown");
}

static bool authorized(P_char character)
{
	return character && IS_PC(character) && GET_LEVEL(character) >= LESSER_G;
}

static draft_slot *draft_for(P_char character, bool create)
{
	if (!character || !IS_PC(character))
		return nullptr;

	const int pid = GET_PID(character);
	const uint64_t runtime_id = character->runtime_id;
	for (draft_slot &slot : drafts)
	{
		if (!slot.used || slot.pid != pid)
			continue;
		if (slot.runtime_id != runtime_id)
		{
			if (!create)
				return nullptr;
			slot.runtime_id = runtime_id;
			slot.selection = COMMUNITY_DEFAULT_SELECTION;
		}
		return &slot;
	}

	if (!create)
		return nullptr;
	for (draft_slot &slot : drafts)
	{
		if (slot.used)
			continue;
		slot.used = true;
		slot.pid = pid;
		slot.runtime_id = runtime_id;
		slot.selection = COMMUNITY_DEFAULT_SELECTION;
		return &slot;
	}
	return nullptr;
}

static std::string selection_text(unsigned int selection)
{
	std::string text;
	for (std::size_t index = 0; index < COMMUNITY_EFFECT_COUNT; ++index)
	{
		const effect_id id = effect_at(index).id;
		if (!selection_has(selection, id))
			continue;
		if (!text.empty())
			text += ", ";
		text += effect_at(index).name;
	}
	return text.empty() ? "(none)" : text;
}

static void send_selection(P_char character, const char *label, unsigned int selection)
{
	const std::string text = selection_text(selection);
	send_to_char_f(character, "%s%s\n", label, text.c_str());
}

static int resolve_effect(const std::string &input, std::string *error)
{
	const std::string normalized = normalize_word(input);
	if (normalized.empty())
	{
		*error = "Name an effect to add or remove.";
		return -1;
	}

	for (std::size_t index = 0; index < COMMUNITY_EFFECT_COUNT; ++index)
	{
		const effect_definition &definition = effect_at(index);
		if (normalized == definition.name)
			return static_cast<int>(index);
		for (const char *alias : definition.aliases)
			if (alias && normalized == alias)
				return static_cast<int>(index);
	}

	int match = -1;
	int match_count = 0;
	for (std::size_t index = 0; index < COMMUNITY_EFFECT_COUNT; ++index)
	{
		const effect_definition &definition = effect_at(index);
		bool matches = std::string_view(definition.name).substr(0, normalized.size()) ==
			       normalized;
		for (const char *alias : definition.aliases)
			if (alias &&
			    std::string_view(alias).substr(0, normalized.size()) == normalized)
				matches = true;
		if (matches)
		{
			match = static_cast<int>(index);
			++match_count;
		}
	}

	if (match_count == 1)
		return match;
	if (match_count > 1)
		*error = "That name is ambiguous; use the full effect name.\nAvailable: " +
			 selection_text((1U << COMMUNITY_EFFECT_COUNT) - 1U);
	else
		*error = "Unknown effect.\nAvailable: " +
			 selection_text((1U << COMMUNITY_EFFECT_COUNT) - 1U);
	return -1;
}

static void send_spells(P_char character)
{
	send_to_char(character, "Community spell-up effects:\n");
	for (const effect_definition &definition : EFFECTS)
		send_to_char_f(character, "  %-20s %s\n", definition.name, definition.description);
	if (draft_slot *draft = draft_for(character, true))
		send_selection(character, "Current draft: ", draft->selection);
	send_to_char(character,
		     "The default package omits regeneration; use add/remove to edit it.\n");
}

static void send_help(P_char character)
{
	send_to_char(character,
		     "newbsa - configure and apply the community spell-up package\n"
		     "  newbsa [g|e]                 apply the current package once\n"
		     "  newbsa spells                list the allowlisted effects\n"
		     "  newbsa add <effect>          add an effect to your draft\n"
		     "  newbsa remove <effect>       remove an effect from your draft\n"
		     "  newbsa reset                 restore the twelve-effect default\n"
		     "  newbsa preview [g|e]         count targets without casting\n"
		     "  newbsa repeat <10s..1h> [g|e] start one boot-scoped repeat job\n"
		     "  newbsa status                show the active job and last result\n"
		     "  newbsa update                atomically use your draft next pass\n"
		     "  newbsa stop                  cancel future repeat passes\n"
		     "The target filter remains connected players level 60 or below; the\n"
		     "creator is excluded. Repeat work continues while its creator is offline.\n");
}

static bool scope_matches(P_char character, community_scope scope)
{
	if (scope == community_scope::all)
		return true;
	if (scope == community_scope::good)
		return GET_RACEWAR(character) == RACEWAR_GOOD;
	return GET_RACEWAR(character) == RACEWAR_EVIL;
}

static bool id_already_collected(const target_collection &collection, uint64_t runtime_id)
{
	for (std::size_t index = 0; index < collection.count; ++index)
		if (collection.ids[index] == runtime_id)
			return true;
	return false;
}

static target_collection collect_targets(P_char excluded_character, int excluded_pid,
					 community_scope scope)
{
	target_collection collection = {};
	for (P_desc descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
	{
		if (STATE(descriptor) != CON_PLAYING || !descriptor->character ||
		    !IS_PC(descriptor->character))
			continue;
		P_char target = descriptor->character;
		if (target == excluded_character ||
		    (excluded_pid > 0 && GET_PID(target) == excluded_pid))
			continue;
		if (id_already_collected(collection, target->runtime_id))
			continue;

		++collection.totals.considered;
		if (GET_LEVEL(target) > 60 || !scope_matches(target, scope))
			continue;
		++collection.totals.eligible;
		if (collection.count >= COMMUNITY_MAX_TARGETS)
		{
			++collection.totals.truncated;
			continue;
		}
		collection.ids[collection.count++] = target->runtime_id;
	}
	return collection;
}

static P_char live_target(uint64_t runtime_id, int excluded_pid, community_scope scope)
{
	P_char target = find_character_by_runtime_id(runtime_id);
	if (!target || IS_NPC(target) || !target->desc || STATE(target->desc) != CON_PLAYING ||
	    target->desc->character != target)
		return nullptr;
	if (excluded_pid > 0 && GET_PID(target) == excluded_pid)
		return nullptr;
	if (GET_LEVEL(target) > 60 || !scope_matches(target, scope))
		return nullptr;
	return target;
}

static effect_snapshot snapshot_effect(const effect_definition &definition, P_char target)
{
	effect_snapshot snapshot = {};
	if (!target)
		return snapshot;

	if (definition.id == effect_id::rest)
	{
		if (get_spell_from_char(target, TAG_WELLRESTED))
		{
			snapshot.present = true;
			snapshot.rest_state = 2;
			snapshot.duration = get_spell_from_char(target, TAG_WELLRESTED)->duration;
		}
		else if (get_spell_from_char(target, TAG_RESTED))
		{
			snapshot.present = true;
			snapshot.rest_state = 1;
			snapshot.duration = get_spell_from_char(target, TAG_RESTED)->duration;
		}
		return snapshot;
	}

	if (definition.id == effect_id::regeneration)
	{
		for (struct affected_type *affect = target->affected; affect; affect = affect->next)
		{
			if (affect->type == SPELL_REGENERATION &&
			    !IS_SET(affect->flags, AFFTYPE_ARAMUS_CROWN_REGENERATION))
			{
				snapshot.present = true;
				snapshot.duration = affect->duration;
				break;
			}
		}
		return snapshot;
	}

	if (struct affected_type *affect = get_spell_from_char(target, definition.spell_type))
	{
		snapshot.present = true;
		snapshot.duration = affect->duration;
	}
	return snapshot;
}

static bool incompatible_with_existing_regeneration(P_char target, effect_id id)
{
	if (id != effect_id::regeneration && id != effect_id::accelerated_healing)
		return false;
	return affected_by_spell(target, SKILL_REGENERATE) ||
	       affected_by_spell(target, SPELL_PACTUM_SERPENTIS);
}

static effect_outcome classify_outcome(const effect_definition &definition,
				       const effect_snapshot &before, const effect_snapshot &after,
				       bool preblocked)
{
	if (preblocked)
		return effect_outcome::blocked;
	if (!before.present && after.present)
		return definition.id == effect_id::rest && after.rest_state == 2 ?
			       effect_outcome::upgraded :
			       effect_outcome::applied;
	if (before.present && after.present)
	{
		if (definition.id == effect_id::rest && before.rest_state == 1 &&
		    after.rest_state == 2)
			return effect_outcome::upgraded;
		return definition.refreshable ? effect_outcome::refreshed :
						effect_outcome::unchanged;
	}
	if (before.present && !after.present)
		return effect_outcome::failed;
	return effect_outcome::blocked;
}

static void record_outcome(effect_statistics &statistics, effect_outcome outcome)
{
	switch (outcome)
	{
	case effect_outcome::applied:
		++statistics.applied;
		break;
	case effect_outcome::refreshed:
		++statistics.refreshed;
		break;
	case effect_outcome::upgraded:
		++statistics.upgraded;
		break;
	case effect_outcome::unchanged:
		++statistics.unchanged;
		break;
	case effect_outcome::blocked:
		++statistics.blocked;
		break;
	case effect_outcome::failed:
		++statistics.failed;
		break;
	}
}

static void apply_effect(const effect_definition &definition, P_char caster, P_char target,
			 effect_statistics &statistics)
{
	if (!caster || !target || !definition.cast)
	{
		++statistics.failed;
		return;
	}

	const effect_snapshot before = snapshot_effect(definition, target);
	const bool preblocked = incompatible_with_existing_regeneration(target, definition.id);
	definition.cast(COMMUNITY_SPELL_LEVEL, caster, target);
	const effect_snapshot after = snapshot_effect(definition, target);
	record_outcome(statistics, classify_outcome(definition, before, after, preblocked));
}

static void apply_selection(unsigned int selection, P_char command_caster, P_char target,
			    run_totals &totals, bool repeat_pass)
{
	P_char offline_safe_caster = repeat_pass ? target : command_caster;
	P_char repeat_caster = repeat_pass ? find_player_by_pid(job.creator_pid) : command_caster;
	if (!repeat_caster)
		repeat_caster = offline_safe_caster;

	for (std::size_t index = 0; index < COMMUNITY_EFFECT_COUNT; ++index)
	{
		const effect_definition &definition = effect_at(index);
		if (!selection_has(selection, definition.id))
			continue;
		P_char caster = definition.legacy_caster == caster_mode::target ?
					target :
					(repeat_pass ? repeat_caster : command_caster);
		apply_effect(definition, caster, target, totals.effects[index]);
	}
}

static void apply_legacy_selection(P_char caster, P_char target, unsigned int selection,
				   run_totals &totals)
{
	std::array<effect_snapshot, COMMUNITY_EFFECT_COUNT> before = {};
	for (std::size_t index = 0; index < COMMUNITY_EFFECT_COUNT; ++index)
		if (selection_has(selection, effect_at(index).id))
			before[index] = snapshot_effect(effect_at(index), target);

	/* This is deliberately the old helper: the untouched default package has
	 * exactly the historical spell order, caster choices, logging, and player
	 * message. */
	newb_spellup(caster, target);

	for (std::size_t index = 0; index < COMMUNITY_EFFECT_COUNT; ++index)
	{
		if (!selection_has(selection, effect_at(index).id))
			continue;
		const effect_definition &definition = effect_at(index);
		const effect_snapshot after = snapshot_effect(definition, target);
		const bool preblocked =
			incompatible_with_existing_regeneration(target, definition.id);
		record_outcome(totals.effects[index],
			       classify_outcome(definition, before[index], after, preblocked));
	}
}

static void send_run_totals(P_char character, const char *prefix, unsigned int selection,
			    const run_totals &totals)
{
	if (!character)
		return;
	send_to_char_f(character, "%s Considered %d, eligible %d, processed %d, skipped %d%s.\n",
		       prefix, totals.considered, totals.eligible, totals.processed, totals.skipped,
		       totals.truncated ? " (target cap truncated additional players)" : "");
	for (std::size_t index = 0; index < COMMUNITY_EFFECT_COUNT; ++index)
	{
		if (!selection_has(selection, effect_at(index).id))
			continue;
		const effect_statistics &statistics = totals.effects[index];
		send_to_char_f(character,
			       "  %-20s applied %d, refreshed %d, upgraded %d, unchanged %d, "
			       "blocked %d, failed %d\n",
			       effect_at(index).name, statistics.applied, statistics.refreshed,
			       statistics.upgraded, statistics.unchanged, statistics.blocked,
			       statistics.failed);
	}
}

static std::string interval_text(int seconds)
{
	if (seconds % 3600 == 0)
		return std::to_string(seconds / 3600) + "h";
	if (seconds % 60 == 0)
		return std::to_string(seconds / 60) + "m";
	return std::to_string(seconds) + "s";
}

static bool parse_interval(const std::string &input, int *seconds, std::string *error)
{
	if (input.size() < 2)
	{
		*error = "Use an interval such as 10s, 5m, or 1h.";
		return false;
	}
	const char unit = static_cast<char>(std::tolower(static_cast<unsigned char>(input.back())));
	unsigned long long multiplier = 0;
	if (unit == 's')
		multiplier = 1;
	else if (unit == 'm')
		multiplier = 60;
	else if (unit == 'h')
		multiplier = 60 * 60;
	else
	{
		*error = "The interval unit must be s, m, or h.";
		return false;
	}

	const std::string digits = input.substr(0, input.size() - 1);
	if (digits.empty())
	{
		*error = "The interval needs a positive whole number.";
		return false;
	}
	errno = 0;
	char *end = nullptr;
	const unsigned long long value = std::strtoull(digits.c_str(), &end, 10);
	if (errno == ERANGE || !end || *end != '\0' || value == 0 ||
	    value > static_cast<unsigned long long>(std::numeric_limits<int>::max()) / multiplier)
	{
		*error = "The interval needs a positive whole number.";
		return false;
	}
	const unsigned long long total = value * multiplier;
	if (total < static_cast<unsigned long long>(COMMUNITY_MIN_INTERVAL_SECONDS) ||
	    total > static_cast<unsigned long long>(COMMUNITY_MAX_INTERVAL_SECONDS))
	{
		*error = "Repeat intervals must be between 10 seconds and 1 hour.";
		return false;
	}
	*seconds = static_cast<int>(total);
	return true;
}

static void audit_job(const char *action, const char *reason = "")
{
	logit(LOG_WIZ,
	      "community spell-up %s: job=%llu creator=%s(%d) editor=%s(%d) revision=%llu "
	      "scope=%s interval=%d selection=0x%08x reason=%s",
	      action, static_cast<unsigned long long>(job.id), job.creator_name, job.creator_pid,
	      job.last_editor_name, job.last_editor_pid,
	      static_cast<unsigned long long>(job.revision), scope_name(job.scope),
	      job.interval_seconds, job.selection, reason && *reason ? reason : "none");
}

static void notify_job_owner(const char *message)
{
	P_char recipient = find_player_by_pid(job.last_editor_pid);
	if (!recipient)
		recipient = find_player_by_pid(job.creator_pid);
	if (recipient)
		send_to_char(message, recipient);
}

static void fail_job(const char *reason)
{
	if (job.pass_running)
	{
		job.last_totals = job.pass_totals;
		job.has_last_result = true;
	}
	std::snprintf(job.stop_reason, sizeof(job.stop_reason), "%s", reason);
	job.active = false;
	job.pass_running = false;
	job.event_armed = false;
	audit_job("stopped", reason);
	char message[MAX_STRING_LENGTH];
	std::snprintf(message, sizeof(message), "Community spell-up repeat stopped: %s.\n", reason);
	notify_job_owner(message);
}

static bool schedule_job_event(int delay_pulses)
{
	const int delay = std::max(1, delay_pulses);
	const nevent_schedule_result result =
		add_event(community_spellup_event, delay, nullptr, nullptr, nullptr, 0, nullptr, 0);
	if (!result.was_scheduled())
	{
		fail_job("the event scheduler rejected the next pass");
		return false;
	}
	job.event_handle = result.handle;
	job.event_armed = true;
	job.next_due_tick = ne_event_tick + static_cast<unsigned long long>(delay);
	return true;
}

static bool creator_still_authorized()
{
	P_char creator = find_player_by_pid(job.creator_pid);
	return !creator || authorized(creator);
}

static void finish_pass()
{
	job.pass_running = false;
	job.last_totals = job.pass_totals;
	job.has_last_result = true;
	char message[MAX_STRING_LENGTH];
	std::snprintf(message, sizeof(message),
		      "Community spell-up repeat revision %llu (%s) completed.\n",
		      static_cast<unsigned long long>(job.pass_revision),
		      scope_name(job.pass_scope));
	notify_job_owner(message);
	send_run_totals(find_player_by_pid(job.last_editor_pid),
			"Repeat result:", job.pass_selection, job.last_totals);
	if (job.active)
		schedule_job_event(job.interval_pulses);
}

static void run_job_slice()
{
	if (!job.active || !job.pass_running)
		return;
	if (!creator_still_authorized())
	{
		fail_job("creator authorization was revoked");
		return;
	}

	std::size_t processed = 0;
	while (job.target_index < job.target_count && processed < COMMUNITY_TARGETS_PER_SLICE)
	{
		const uint64_t runtime_id = job.target_ids[job.target_index++];
		P_char target = live_target(runtime_id, job.creator_pid, job.pass_scope);
		if (!target)
		{
			++job.pass_totals.skipped;
			continue;
		}
		++job.pass_totals.processed;
		++processed;
		apply_selection(job.pass_selection, nullptr, target, job.pass_totals, true);
	}

	if (job.target_index < job.target_count)
	{
		schedule_job_event(WAIT_SEC);
		return;
	}
	finish_pass();
}

static void begin_pass()
{
	if (!job.active || job.pass_running)
		return;
	if (!creator_still_authorized())
	{
		fail_job("creator authorization was revoked");
		return;
	}

	job.pass_running = true;
	job.pass_selection = job.selection;
	job.pass_scope = job.scope;
	job.pass_revision = job.revision;
	target_collection collection = collect_targets(nullptr, job.creator_pid, job.pass_scope);
	job.target_ids = collection.ids;
	job.target_count = collection.count;
	job.target_index = 0;
	job.pass_totals = collection.totals;
	run_job_slice();
}

static void community_spellup_event(P_char, P_char, P_obj, void *)
{
	/* A canceled or duplicated callback must not advance a pass.  The armed
	 * bit is set only by the one scheduler handle currently owned by the job. */
	if (!job.active || !job.event_armed)
		return;
	job.event_armed = false;
	job.event_handle = { nullptr, 0 };
	if (!job.pass_running)
		begin_pass();
	else
		run_job_slice();
}

static void show_status(P_char character)
{
	draft_slot *draft = draft_for(character, true);
	if (!job.active)
	{
		send_to_char(character, "No community spell-up repeat job is active.\n");
		if (job.stop_reason[0])
			send_to_char_f(character, "Last job stopped: %s.\n", job.stop_reason);
		if (draft)
			send_selection(character, "Your draft: ", draft->selection);
		if (job.has_last_result)
			send_run_totals(character, "Last result:", job.pass_selection,
					job.last_totals);
		return;
	}
	send_to_char_f(character,
		       "Community spell-up repeat job %llu: creator %s (pid %d), revision %llu.\n",
		       static_cast<unsigned long long>(job.id), job.creator_name, job.creator_pid,
		       static_cast<unsigned long long>(job.revision));
	send_to_char_f(character, "  Scope: %s; interval: %s; state: %s.\n", scope_name(job.scope),
		       interval_text(job.interval_seconds).c_str(),
		       job.pass_running ? "pass in progress" : "waiting for next pass");
	if (draft)
		send_selection(character, "  Your draft: ", draft->selection);
	send_selection(character, "  Active selection: ", job.selection);
	if (job.pass_running)
		send_to_char_f(character,
			       "  Pass revision %llu: %llu/%llu target slots complete.\n",
			       static_cast<unsigned long long>(job.pass_revision),
			       static_cast<unsigned long long>(job.target_index),
			       static_cast<unsigned long long>(job.target_count));
	else if (job.event_armed)
	{
		const unsigned long long remaining_ticks =
			job.next_due_tick > ne_event_tick ? job.next_due_tick - ne_event_tick : 0;
		send_to_char_f(character, "  Next pass due in about %llu seconds.\n",
			       static_cast<unsigned long long>((remaining_ticks + WAIT_SEC - 1) /
							       WAIT_SEC));
	}
	if (job.has_last_result)
		send_run_totals(character, "  Last result:", job.pass_selection, job.last_totals);
}

static void preview(P_char character, unsigned int selection, community_scope scope)
{
	if (!selection)
	{
		send_to_char(character,
			     "Your selection is empty; add at least one effect first.\n");
		return;
	}
	target_collection collection = collect_targets(character, GET_PID(character), scope);
	send_to_char_f(character,
		       "Preview: level %d, scope %s, %d eligible player%s (%d stored for a bounded "
		       "preview; cap %llu).\n",
		       COMMUNITY_SPELL_LEVEL, scope_name(scope), collection.totals.eligible,
		       collection.totals.eligible == 1 ? "" : "s", collection.count,
		       static_cast<unsigned long long>(COMMUNITY_MAX_TARGETS));
	send_selection(character, "Selection: ", selection);
	send_to_char(
		character,
		"Preview does not cast or mutate players. Rest upgrades rested players; "
		"regeneration and accelerated healing honor their normal incompatibilities.\n");
}

static void execute_one_shot(P_char character, unsigned int selection, community_scope scope)
{
	if (!selection)
	{
		send_to_char(character,
			     "Your selection is empty; add at least one effect first.\n");
		return;
	}

	run_totals totals = {};
	if (selection == COMMUNITY_DEFAULT_SELECTION)
	{
		/* Keep the original descriptor traversal and exclusion rule for the
		 * untouched default package. */
		for (P_desc descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
		{
			if (descriptor->connected != CON_PLAYING || !descriptor->character ||
			    character == descriptor->character || !IS_PC(descriptor->character))
				continue;
			P_char target = descriptor->character;
			++totals.considered;
			if (GET_LEVEL(target) > 60 || !scope_matches(target, scope))
				continue;
			++totals.eligible;
			++totals.processed;
			apply_legacy_selection(character, target, selection, totals);
		}
	}
	else
	{
		target_collection collection =
			collect_targets(character, GET_PID(character), scope);
		totals = collection.totals;
		for (std::size_t index = 0; index < collection.count; ++index)
		{
			P_char target =
				live_target(collection.ids[index], GET_PID(character), scope);
			if (!target)
			{
				++totals.skipped;
				continue;
			}
			++totals.processed;
			apply_selection(selection, character, target, totals, false);
		}
	}

	send_to_char_f(character, "Done. Blessed %d player%s.\n", totals.processed,
		       totals.processed == 1 ? "" : "s");
	send_run_totals(character, "Community spell-up result:", selection, totals);
}

static void update_draft(P_char character, bool add, const std::string &effect_name)
{
	draft_slot *draft = draft_for(character, true);
	if (!draft)
	{
		send_to_char(character, "No draft slot is available right now; try again later.\n");
		return;
	}
	std::string error;
	const int index = resolve_effect(effect_name, &error);
	if (index < 0)
	{
		send_to_char(character, error.c_str());
		if (error.empty() || error.back() != '\n')
			send_to_char(character, "\n");
		return;
	}
	const unsigned int bit = 1U << static_cast<unsigned int>(index);
	if (add)
	{
		if (draft->selection & bit)
		{
			send_to_char_f(character, "%s is already selected.\n",
				       effect_at(index).name);
			return;
		}
		draft->selection |= bit;
		send_to_char_f(character, "Added %s to your draft.\n", effect_at(index).name);
	}
	else
	{
		if (!(draft->selection & bit))
		{
			send_to_char_f(character, "%s is not selected.\n", effect_at(index).name);
			return;
		}
		draft->selection &= ~bit;
		send_to_char_f(character, "Removed %s from your draft.\n", effect_at(index).name);
	}
	send_selection(character, "Draft: ", draft->selection);
}

static void start_repeat(P_char character, const std::string &interval_word, community_scope scope)
{
	if (job.active)
	{
		send_to_char(
			character,
			"A community spell-up repeat job is already active; use update or stop.\n");
		return;
	}
	draft_slot *draft = draft_for(character, true);
	if (!draft || !draft->selection)
	{
		send_to_char(character,
			     "Your selection is empty; add at least one effect first.\n");
		return;
	}
	int seconds = 0;
	std::string error;
	if (!parse_interval(interval_word, &seconds, &error))
	{
		send_to_char(character, error.c_str());
		send_to_char(character, "\n");
		return;
	}

	job = {};
	job.active = true;
	job.id = next_job_id++;
	job.revision = 1;
	job.creator_pid = GET_PID(character);
	job.last_editor_pid = GET_PID(character);
	job.interval_seconds = seconds;
	job.interval_pulses = seconds * WAIT_SEC;
	job.selection = draft->selection;
	job.scope = scope;
	remember_name(job.creator_name, character);
	remember_name(job.last_editor_name, character);
	audit_job("started");
	begin_pass();
	if (job.active)
		send_to_char_f(character,
			       "Started community spell-up repeat job %llu at %s (%s).\n",
			       static_cast<unsigned long long>(job.id),
			       interval_text(seconds).c_str(), scope_name(scope));
}

static void update_repeat(P_char character)
{
	if (!job.active)
	{
		send_to_char(character, "No community spell-up repeat job is active.\n");
		return;
	}
	draft_slot *draft = draft_for(character, true);
	if (!draft || !draft->selection)
	{
		send_to_char(character,
			     "Your selection is empty; add at least one effect first.\n");
		return;
	}
	job.selection = draft->selection;
	++job.revision;
	job.last_editor_pid = GET_PID(character);
	remember_name(job.last_editor_name, character);
	audit_job("updated");
	send_to_char_f(character,
		       "Updated repeat job %llu to revision %llu; the current pass keeps its "
		       "snapshot and the next pass uses:\n",
		       static_cast<unsigned long long>(job.id),
		       static_cast<unsigned long long>(job.revision));
	send_selection(character, "  ", job.selection);
}

static void stop_repeat(P_char character)
{
	if (!job.active)
	{
		send_to_char(character, "No community spell-up repeat job is active.\n");
		return;
	}
	if (job.event_armed)
		(void)nevent_cancel(job.event_handle);
	job.event_armed = false;
	job.active = false;
	const bool partial = job.pass_running;
	if (partial)
	{
		job.last_totals = job.pass_totals;
		job.has_last_result = true;
	}
	job.pass_running = false;
	std::snprintf(job.stop_reason, sizeof(job.stop_reason),
		      "stopped by an authorized operator");
	job.last_editor_pid = GET_PID(character);
	remember_name(job.last_editor_name, character);
	audit_job("stopped", job.stop_reason);
	send_to_char(
		character,
		partial ?
			"Stopped future community spell-up work; the current pass was partial.\n" :
			"Stopped future community spell-up work.\n");
}

} // namespace

void community_spellup_reset_for_boot(void)
{
	drafts = {};
	job = {};
	next_job_id = 1;
}

void community_spellup_command(P_char character, char *argument)
{
	if (!authorized(character))
	{
		send_to_char(character, "You are not authorized to use newbsa.\n");
		return;
	}

	std::string command;
	const std::string rest = rest_after_first_word(argument, &command);
	draft_slot *draft = draft_for(character, true);
	if (!draft)
	{
		send_to_char(character, "No draft slot is available right now; try again later.\n");
		return;
	}

	if (command.empty())
	{
		execute_one_shot(character, draft->selection, community_scope::all);
		return;
	}
	if (command == "g" || command == "good" || command == "e" || command == "evil")
	{
		community_scope scope;
		if (!rest.empty() || !parse_scope(command, &scope))
		{
			send_to_char(character, "Usage: newbsa [g|e]\n");
			return;
		}
		execute_one_shot(character, draft->selection, scope);
		return;
	}
	if (command == "help" || command == "?")
	{
		if (!rest.empty())
			send_to_char(character, "Usage: newbsa help\n");
		else
			send_help(character);
		return;
	}
	if (command == "spells")
	{
		if (!rest.empty())
			send_to_char(character, "Usage: newbsa spells\n");
		else
			send_spells(character);
		return;
	}
	if (command == "add" || command == "remove")
	{
		if (rest.empty())
		{
			send_to_char_f(character, "Usage: newbsa %s <effect>\n", command.c_str());
			return;
		}
		update_draft(character, command == "add", rest);
		return;
	}
	if (command == "reset")
	{
		if (!rest.empty())
		{
			send_to_char(character, "Usage: newbsa reset\n");
			return;
		}
		draft->selection = COMMUNITY_DEFAULT_SELECTION;
		send_to_char(character, "Your draft was reset to the twelve-effect default.\n");
		send_selection(character, "Draft: ", draft->selection);
		return;
	}
	if (command == "preview")
	{
		community_scope scope = community_scope::all;
		if (!rest.empty() && !parse_scope(rest, &scope))
		{
			send_to_char(character, "Usage: newbsa preview [g|e]\n");
			return;
		}
		preview(character, draft->selection, scope);
		return;
	}
	if (command == "repeat")
	{
		std::string interval_word;
		std::string repeat_arguments = rest;
		const std::string scope_word =
			rest_after_first_word(repeat_arguments.data(), &interval_word);
		community_scope scope = community_scope::all;
		if (interval_word.empty() ||
		    (!scope_word.empty() && !parse_scope(scope_word, &scope)))
		{
			send_to_char(character, "Usage: newbsa repeat <10s..1h> [g|e]\n");
			return;
		}
		start_repeat(character, interval_word, scope);
		return;
	}
	if (command == "status")
	{
		if (!rest.empty())
		{
			send_to_char(character, "Usage: newbsa status\n");
			return;
		}
		show_status(character);
		return;
	}
	if (command == "update")
	{
		if (!rest.empty())
		{
			send_to_char(character, "Usage: newbsa update\n");
			return;
		}
		update_repeat(character);
		return;
	}
	if (command == "stop")
	{
		if (!rest.empty())
		{
			send_to_char(character, "Usage: newbsa stop\n");
			return;
		}
		stop_repeat(character);
		return;
	}

	send_to_char(character, "Unknown newbsa option. Use `newbsa help`.\n");
}
