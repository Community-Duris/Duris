#ifndef DURIS_COLLECTOR_EXPIRY_PREPARATION_H
#define DURIS_COLLECTOR_EXPIRY_PREPARATION_H

#include "economy/collector_command.h"
#include "economy/collector_storage.h"

#include <cstdint>
#include <memory>

enum class collector_expiry_prepare_outcome : uint8_t
{
	prepared,
	invalid_request,
	stale_listing,
	not_due,
	invalid_item,
	stale_custody,
	allocation_failure,
};

// Turns an immutable listing read plus the game-thread custody projection into
// a fully fenced destruction command. A rejected preparation leaves payload
// unchanged.
collector_expiry_prepare_outcome collector_expiry_prepare(
	const collector::record &runtime_entry, const collector_listing_detail &detail,
	const item_ownership_runtime_entry &held_item, uint64_t destruction_owner_revision,
	uint64_t observed_at, std::unique_ptr<collector_command_payload> *payload);

#endif
