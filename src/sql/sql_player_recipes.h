#ifndef DURIS_SQL_PLAYER_RECIPES_H_INCLUDED
#define DURIS_SQL_PLAYER_RECIPES_H_INCLUDED

// Crafting recipes are stored outside the player snapshot.
bool sql_add_player_recipe(int pid, int recipe_vnum);
bool sql_delete_player_recipes(int pid);
bool sql_has_player_recipe(int pid, int recipe_vnum);
int *sql_get_player_recipes(int pid, int *count);

#endif // DURIS_SQL_PLAYER_RECIPES_H_INCLUDED
