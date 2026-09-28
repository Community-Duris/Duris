#ifndef DURIS_SPELL_ITEM_LIFECYCLE_H
#define DURIS_SPELL_ITEM_LIFECYCLE_H

#include "item/item_movement_transaction.h"

#include <cstddef>
#include <cstdint>

// Consume up to max_components matching player-carried roots as one custody
// transaction. The continuation runs only after committed items are removed.
bool spell_consume_components(P_char actor, int vnum, size_t max_components,
			      uint32_t reason_id, item_movement_completion_fn continuation,
			      const void *context, size_t context_size);

#endif
