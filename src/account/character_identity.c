#include "core/prototypes.h"

#include <unordered_map>

extern P_char character_list;

static uint64_t next_runtime_id = 0;
// Non-owning; publication, lookup and retirement all run on the game thread.
static std::unordered_map<uint64_t, P_char> live_characters_by_runtime_id;

uint64_t allocate_character_runtime_id()
{
	const uint64_t runtime_id = ++next_runtime_id;

	if (!runtime_id)
		panic_corruption("character", "process-local character identity exhausted");
	return runtime_id;
}

P_char find_character_by_runtime_id(uint64_t runtime_id)
{
	if (!nevent_require_game_thread("find_character_by_runtime_id") || !runtime_id)
		return NULL;
	const auto found = live_characters_by_runtime_id.find(runtime_id);
	return found == live_characters_by_runtime_id.end() ? NULL : found->second;
}

void register_character_runtime_id(P_char character)
{
	if (!nevent_require_game_thread("register_character_runtime_id"))
		return;
	if (!character || !character->runtime_id)
	{
		panic_corruption("character", "publishing a character without a runtime identity");
		return;
	}
	const auto [entry, inserted] =
		live_characters_by_runtime_id.emplace(character->runtime_id, character);
	if (!inserted && entry->second != character)
		panic_corruption("character", "duplicate live character runtime identity");
}

void unregister_character_runtime_id(P_char character)
{
	if (!nevent_require_game_thread("unregister_character_runtime_id") || !character)
		return;
	const auto found = live_characters_by_runtime_id.find(character->runtime_id);
	if (found != live_characters_by_runtime_id.end() && found->second == character)
		live_characters_by_runtime_id.erase(found);
}

// Observation-only, at a quiescent lifecycle boundary (never during construction
// or extraction). Bound the walk so a corrupt/cyclic list cannot hang diagnostics.
bool character_runtime_index_is_consistent()
{
	if (!nevent_require_game_thread("character_runtime_index_is_consistent"))
		return false;
	size_t count = 0;
	for (P_char character = character_list; character; character = character->next)
	{
		if (++count > live_characters_by_runtime_id.size() ||
		    find_character_by_runtime_id(character->runtime_id) != character)
			return false;
	}
	return count == live_characters_by_runtime_id.size();
}
