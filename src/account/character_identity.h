#ifndef DURIS_CHARACTER_IDENTITY_H
#define DURIS_CHARACTER_IDENTITY_H

#include "core/structs.h"
#include <cstddef>

// Same actual global runtime index, its inline next ID/admission observer and
// every real allocator rebound node/bucket request; caller includes CURRENT
// exactly once, refreshes on EVERY return, and holds the admitted peak.
bool character_runtime_identity_storage_bytes(size_t *) noexcept;

// Genuine original runtime registration only. Scoped game-thread nonowning
// callback intercepts this same map's actual prospective allocator requests;
// no copied hash-growth model or separate index. Callback must not reenter or
// mutate runtime/global ownership. Original zero-ID/duplicate panic survives.
// Before action refusal leaves markers unchanged; successful actual emplace
// writes both markers with no later fallible diagnostic. No source/ACK grant.
bool register_character_runtime_id_bounded(P_char, bool *returned, bool *registered,
					   bool (*)(size_t, void *) noexcept, void *,
					   size_t outer_live) noexcept;

#endif
