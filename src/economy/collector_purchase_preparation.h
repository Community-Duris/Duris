#ifndef DURIS_COLLECTOR_PURCHASE_PREPARATION_H
#define DURIS_COLLECTOR_PURCHASE_PREPARATION_H

#include "economy/collector_command.h"
#include "economy/collector_storage.h"
#include "player/player_snapshot.h"

#include <array>
#include <cstddef>
#include <cstdint>

struct collector_purchase_actor_snapshot
{
	uint32_t pid = 0;
	uint8_t racewar = 0;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name = {};
	uint64_t wallet_revision = 0;
	uint64_t bank_revision = 0;
	uint64_t item_owner_revision = 0;
	uint64_t carried_currency_value = 0;
	int64_t carried_weight = 0;
	int64_t carry_weight_limit = 0;
	size_t carried_items = 0;
	size_t carry_item_limit = 0;
	uint64_t observed_at = 0;
};

enum class collector_purchase_prepare_outcome : uint8_t
{
	prepared,
	invalid_actor,
	stale_listing,
	unavailable,
	forbidden,
	insufficient_funds,
	invalid_item,
	stale_custody,
	capacity,
	allocation_failure,
};

enum class collector_listing_decode_outcome : uint8_t
{
	decoded,
	stale_listing,
	invalid_item,
	allocation_failure,
};

collector_listing_decode_outcome
collector_listing_decode_item(const collector::record &runtime_entry,
			      const collector_listing_detail &detail,
			      player_item_snapshot *decoded_item);

// Converts one immutable detail read into the fully fenced command sent to the
// durable coordinator. Inputs contain no live pointers, and outputs have a
// strong guarantee: a rejected preparation leaves them unchanged.
collector_purchase_prepare_outcome
collector_purchase_prepare(const collector::record &runtime_entry,
			   const collector_listing_detail &detail,
			   const item_ownership_runtime_entry &held_item,
			   const collector_purchase_actor_snapshot &actor,
			   collector_command_payload *payload, player_item_snapshot *decoded_item);

#endif
