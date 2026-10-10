#ifndef DURIS_CHARACTER_MAINTENANCE_H
#define DURIS_CHARACTER_MAINTENANCE_H

#include "core/structs.h"

void character_maintenance_init();
void character_maintenance_enter(P_char character);
void character_maintenance_leave(P_char character);
void character_maintenance_changed(P_char character);

// Actual inline maintenance cadence/readiness; no scheduler/pool duplicates.
bool character_maintenance_storage_bytes(size_t *) noexcept;
// Complete original enter/changed policy, actual schedule/cancel/reschedule.
// Caller includes shared pool/pending/output/cancellation CURRENT once;
// reobserve EVERY return. Original fatal branches retain original behavior;
// valid native producer authenticates their preconditions before action.
bool character_maintenance_enter_bounded(P_char, bool *, bool *, bool (*)(size_t, void *) noexcept,
					 void *, size_t) noexcept;

#endif
