#ifndef ZONE_RESET_ROOM_NESTING_H
#define ZONE_RESET_ROOM_NESTING_H

#include <cstddef>
struct obj_data;
class quest_mobile_native_item_stage;

enum class zone_reset_room_nest_result
{
	refused,
	original_shell_owner_required,
	nested
};

// Only genuine factory-owned unpublished room topology. No source, SQL,
// admission, callback, activity, dirty marking or publication authority.
class zone_reset_room_local_nesting final
{
	friend class quest_mobile_native_item_stage;
	// Actual owning weight_fits named path, no allocation or authority.
	static size_t mutation_working_bytes() noexcept;
	static bool weight_fits(struct obj_data *target, struct obj_data *root,
				int change) noexcept;
	static zone_reset_room_nest_result nest(struct obj_data *child, struct obj_data *target,
						struct obj_data *root) noexcept;
	static bool detach(struct obj_data *child, struct obj_data *target,
			   struct obj_data *root) noexcept;
};

#endif
