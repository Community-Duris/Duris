#include "world/quest_mobile_native_binding.h"
#include "world/quest_mobile_native_reference.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <span>

extern P_index mob_index;
extern int top_of_mobt;

static_assert(QUEST_MOBILE_NATIVE_BINDING_BYTES == QUEST_MOBILE_NATIVE_REFERENCE_BYTES);

bool quest_mobile_native_reference_copy(const char_data *character,
					std::uint64_t expected_runtime_id,
					quest_mobile_native_reference *output) noexcept
{
	if (!nevent_require_game_thread("quest_mobile_native_reference_copy") || !character ||
	    !output || !expected_runtime_id ||
	    find_character_by_runtime_id(expected_runtime_id) != character)
		return false;
	// The original generation lookup proves the pointer is still live before
	// any access to storage that the character pool may have released or reused.
	if (character->runtime_id != expected_runtime_id || !IS_NPC(character) ||
	    !character->only.npc || !mob_index || character->only.npc->R_num < 0 ||
	    character->only.npc->R_num > top_of_mobt)
		return false;

	const std::span<const uint8_t> bytes(character->native_mobile_binding.encoded_reference_,
					     QUEST_MOBILE_NATIVE_BINDING_BYTES);
	quest_mobile_native_reference candidate;
	if (quest_mobile_native_reference_decode(bytes, &candidate) !=
	    player_snapshot_codec_result::ok)
		return false;
	// Check the observed live type only; it never supplies a missing reference.
	if (candidate.mobile_vnum != mob_index[character->only.npc->R_num].virtual_number)
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> canonical = {};
	if (quest_mobile_native_reference_encode(candidate, &canonical) !=
		    player_snapshot_codec_result::ok ||
	    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
		return false;
	*output = candidate;
	return true;
}
