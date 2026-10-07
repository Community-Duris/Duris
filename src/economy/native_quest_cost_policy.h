#ifndef DURIS_NATIVE_QUEST_COST_POLICY_H
#define DURIS_NATIVE_QUEST_COST_POLICY_H

#include <cstdint>

// New primary-owned implementation of the registered reason44 expense contract.
// Only original NPC cash consumed by a frozen COINS requirement may use this
// sink. The NPC holding is its genuine birth-created wallet mapping lifetime.
// This constant supplies no source, mapping, epoch, or admission authority.
constexpr uint32_t ECONOMIC_QUEST_REQUIREMENT_POLICY_VERSION = 1;
constexpr uint64_t ECONOMIC_QUEST_REQUIREMENT_SINK_ID = 44;
constexpr uint64_t ECONOMIC_QUEST_REQUIREMENT_SINK_CONTEXT = 0;

#endif
