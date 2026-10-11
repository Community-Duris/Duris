#ifndef DURIS_HELD_RETIREMENT_PUBLICATION_INTERNAL_H
#define DURIS_HELD_RETIREMENT_PUBLICATION_INTERNAL_H

#include "item/item_movement_state_internal.h"
#include "item/held_retirement_recovery.h"

// Same original private-friend owner. Common Item code retains/erases pending
// entries; the policy never mints a pipeline reservation or coordinator token.
class item_held_retirement_publication_owner final
{
	using pending_movement = item_movement_detail::pending_movement;
	struct publication_attempt
	{
		pending_movement *entry = nullptr;
		held_retirement_publication_snapshot snapshot;
	};
	static bool same_items(const std::vector<player_item_snapshot> &left,
			       const std::vector<player_item_snapshot> &right);
	static P_char actor_for(uint32_t pid) noexcept;
	static bool native_body(P_char actor, std::vector<player_item_snapshot> *items);
	static bool checkpoint(critical_native_recovery_envelope *original,
			       const held_retirement_recovery &context);
	static bool native_publish(const critical_command &command,
				   const critical_completion &completion, void *opaque) noexcept;

    public:
	static bool publish(pending_movement &entry) noexcept;
	static bool restore(const critical_native_recovery_envelope &envelope) noexcept;
};

#endif
