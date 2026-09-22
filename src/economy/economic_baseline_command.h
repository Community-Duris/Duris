#ifndef DURIS_ECONOMIC_BASELINE_COMMAND_H
#define DURIS_ECONOMIC_BASELINE_COMMAND_H
#include "economy/economic_baseline_adapter.h"
#include "economy/economic_accounting_intent.h"

constexpr size_t ECONOMIC_BASELINE_COMMAND_BYTES = 48;
// Serializes baseline preparations with each other, not with gameplay. The
// lifecycle owner must independently hold the frozen domain boundary.
constexpr uint64_t ECONOMIC_BASELINE_FENCE = 0x45434f4e42415345;
// A compact reference to the complete EAB1 witness, with ordinary EAI1 binding.
// Neither building nor rehydrating authorizes persistence or activation. The
// lifecycle owner must retain the full witness atomically with plan and receipt.
// Outputs are unchanged on failure. accepted_at_usec must be nonzero and retained
// on retry; schema-1 execution and coordinator admission remain closed.
economic_accounting_error economic_baseline_command_build(const economic_prepared_baseline &,
							  uint64_t accepted_at_usec,
							  critical_command *);
// Regenerate the exact command from the retained witness before replacing pure
// preparation metadata with EAI1 metadata. Never accepts an arbitrary plan or
// replaces a retained witness with current state. Compare the resulting encoded
// plan with the original receipt during replay.
economic_accounting_error economic_baseline_command_plan(const critical_command &,
							 const economic_prepared_baseline &,
							 economic_accounting_plan *);
#endif
