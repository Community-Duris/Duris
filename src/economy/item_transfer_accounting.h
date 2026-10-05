#ifndef DURIS_ITEM_TRANSFER_ACCOUNTING_H
#define DURIS_ITEM_TRANSFER_ACCOUNTING_H

#include "economy/economic_accounting_intent.h"

// Stable typed writer capability for player item custody movement and sourced
// item creation or retirement.
constexpr uint32_t ECONOMIC_WRITER_ITEM_TRANSFER = 6;

// Freeze and validate schema-2 facts for ordinary player item moves, sourced
// creation grants, and item-action retirements. Lifecycle source identity is
// tied to the command, epoch, selected root UID, and reason.
economic_accounting_error item_transfer_accounting_intent(
	const critical_command &command, const critical_operation_id &lineage,
	const critical_operation_id &epoch, uint32_t actor_pid, std::vector<uint8_t> *encoded,
	economic_source_kind lifecycle_source = {});
// Explicit v11 structural freeze, independent of schema1 execution. Actor is
// the frozen final giver. Consumption requires the parent's original quest-action
// or quest-completion event; accepting an offering has no reward/source sidecar.
// This does not authenticate native lifetime/stock, epoch or source, and does not
// open the existing admission predicate. The original atomic parent must prove
// those facts and bind player reward/progression locks before submitting.
economic_accounting_error
item_native_mobile_accounting_intent(const critical_command &, const critical_operation_id &lineage,
				     const critical_operation_id &epoch, uint32_t actor_pid,
				     const economic_source_event *original_quest_event,
				     std::vector<uint8_t> *encoded) noexcept;

bool item_transfer_accounting_command_supported(const critical_command &command) noexcept;

// Resolve one frozen craft against its locked input custody. Outputs start
// absent; consumed inputs retire and an optional pouch retains its custody at
// the next revision. Unchanged container ancestors witness the complete forest
// without events. Only effect vectors are replaced in the caller's plan.
economic_accounting_error
item_transfer_craft_accounting_effects(const item_transfer_payload &payload,
				       std::span<const economic_item_snapshot> inputs,
				       economic_accounting_plan *plan);

#endif
