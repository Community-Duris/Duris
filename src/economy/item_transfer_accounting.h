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
bool item_transfer_accounting_command_supported(const critical_command &command) noexcept;

// Resolve one frozen craft against its locked input custody. Outputs start
// absent; every input retirement and output admission uses the native ledger's
// event ordering. Only effect vectors are replaced in the caller's plan.
economic_accounting_error
item_transfer_craft_accounting_effects(const item_transfer_payload &payload,
				       std::span<const economic_item_snapshot> inputs,
				       economic_accounting_plan *plan);

#endif
