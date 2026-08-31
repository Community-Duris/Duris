#ifndef CRAFTING_H
#define CRAFTING_H

#include "core/structs.h"

enum crafting_mode
{
	CRAFTING_MODE_CRAFT,
	CRAFTING_MODE_FORGE
};

struct crafting_plan
{
	int item_value;
	int low_material_vnum;
	int high_material_vnum;
	int low_material_count;
	int high_material_count;
	bool magical;
};

/* Loads no player state. Returns FALSE when the item has no salvage family. */
bool crafting_build_plan(P_obj item, struct crafting_plan *plan);
/* Recipe scrolls must not teach targets that modern Craft/Forge can never make. */
bool crafting_validate_recipe_target(P_obj item);
/* Includes every permanent all-player gate used when recipes are created or loaded. */
bool crafting_recipe_target_is_available(P_obj item);
/* Adds operator-enabled material requirements to a dynamically created recipe scroll. */
void crafting_configure_recipe_scroll(P_obj recipe, P_obj target);
void crafting_examine_support_item(P_char ch, P_obj item);
int crafting_level_gate_multiplier(void);
int crafting_experience_per_ival(void);
bool crafting_mode_enabled(enum crafting_mode mode);
int crafting_essence_vnum(enum crafting_mode mode);
int crafting_tool_vnum(enum crafting_mode mode);
int crafting_scientific_tools_vnum(void);
bool crafting_scientific_tools_prevent_breakage(void);
int crafting_scientific_tools_recipe_roll_divisor(void);
int crafting_scientific_tools_recipe_player_multiplier(void);
double crafting_salvage_essence_luck_multiplier(void);
double crafting_salvage_essence_chance_multiplier(void);
int *crafting_get_player_recipes(P_char ch, int *count);

/* Reserved public module boundaries for the staged command/config extraction. */
void boot_crafting_system(void);
void crafting_handle_command(P_char ch, enum crafting_mode mode, char *argument);

#endif
