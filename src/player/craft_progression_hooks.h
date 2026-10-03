#ifndef CRAFT_PROGRESSION_HOOKS_H
#define CRAFT_PROGRESSION_HOOKS_H

#include "item/craft_recipe_continuation.h"
#include "player/player_snapshot.h"
#include "core/structs.h"

enum class craft_progression_publication_result
{
	ready,
	waiting,
	failed,
};

// The recipe module owns gameplay progression; persistence owns immutable
// receipts. Hooks keep generic item/load/save code independent of recipe UI.
// Install during crafting boot, before admitting or publishing player commands.
struct craft_progression_hook_table
{
	craft_progression_publication_result (*publish)(
		const critical_operation_id &, P_char, const craft_recipe_continuation &) = nullptr;
	bool (*pending)(uint32_t, std::vector<player_craft_receipt_snapshot> *) = nullptr;
	void (*saved)(uint32_t, bool, const player_craft_receipt_snapshot *, size_t) = nullptr;
	bool (*recover)(uint32_t, const player_craft_receipt_snapshot *, size_t) = nullptr;
	void (*acknowledged)(const critical_operation_id &) = nullptr;
	void (*notify)(P_char, bool, const craft_recipe_continuation &) = nullptr;
};

void craft_progression_initialize(void);

inline craft_progression_hook_table craft_progression_hooks;
constexpr player_component_mask_t CRAFT_PROGRESSION_COMPONENTS =
	PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_SKILLS | PLAYER_COMPONENT_AFFECTS |
	PLAYER_COMPONENT_TROPHIES;

inline bool
craft_progression_pending_save_receipts(uint32_t pid,
					std::vector<player_craft_receipt_snapshot> *receipts)
{
	return receipts &&
	       (!craft_progression_hooks.pending || craft_progression_hooks.pending(pid, receipts));
}

#endif
