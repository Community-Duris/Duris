// These existing value/SQL fixtures do not provide a live native world.
// This is link support only. Any unexpected world proof or mutation aborts;
// no unavailable capture or template ever becomes successful evidence.
#include "core/structs.h"
#include "player/player_snapshot_capture.h"
#include "world/object_template.h"
#include <cstdlib>

P_index mob_index = nullptr;
int top_of_mobt = -1;

player_snapshot_capture_result
player_item_snapshot_tree_capture_literal(P_obj, std::vector<player_item_snapshot> *, size_t *)
{
	std::abort();
}

#ifndef __NO_MYSQL__
P_index obj_index = nullptr;
player_snapshot_capture_result
player_item_snapshot_tree_capture(P_obj, std::vector<player_item_snapshot> *, size_t *)
{
	std::abort();
}
player_snapshot_capture_result
player_item_snapshot_list_capture(P_char, bool, bool, bool, std::vector<player_item_snapshot> *,
				  size_t *)
{
	std::abort();
}
const object_template *find_recovery_object_template(int) noexcept
{
	std::abort();
}
char *str_dup(const char *)
{
	std::abort();
}
void str_free(const char *)
{
	std::abort();
}
void __free(void *, const char *, int)
{
	std::abort();
}
#endif
