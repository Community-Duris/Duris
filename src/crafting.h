#ifndef CRAFTING_H
#define CRAFTING_H

#include "structs.h"

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
int crafting_level_gate_multiplier(void);
bool crafting_mode_enabled(enum crafting_mode mode);
int crafting_essence_vnum(enum crafting_mode mode);
int crafting_tool_vnum(enum crafting_mode mode);
int *crafting_get_player_recipes(P_char ch, int *count);

/* Reserved public module boundaries for the staged command/config extraction. */
void boot_crafting_system(void);
void crafting_handle_command(P_char ch, enum crafting_mode mode, char *argument);

#endif
